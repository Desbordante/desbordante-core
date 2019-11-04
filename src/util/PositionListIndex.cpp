//
// Created by kek on 30.07.2019.
//

#include "PositionListIndex.h"

#include <map>
#include <cmath>
#include <algorithm>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "model/ColumnLayoutRelationData.h"

using namespace std;

const int PositionListIndex::singletonValueId = 0;

PositionListIndex::PositionListIndex(vector<vector<int>> index, vector<int> nullCluster, int size, double entropy,
                                     long nep, unsigned int relationSize, unsigned int originalRelationSize):
                                     index(std::move(index)),
                                     nullCluster(std::move(nullCluster)),
                                     size(size),
                                     entropy(entropy),
                                     nep(nep),
                                     relationSize(relationSize),
                                     originalRelationSize(originalRelationSize),
                                     probingTableCache(){}

shared_ptr<PositionListIndex> PositionListIndex::createFor(vector<int>& data, bool isNullEqNull) {
    map<int, vector<int>> index;
    for (unsigned long position = 0; position < data.size(); ++position){
        int valueId = data[position];
        index[valueId].push_back(position);
    }

    vector<int> nullCluster;
    if (isNullEqNull){
        nullCluster = index[RelationData::nullValueId];
    } else {
        index.erase(RelationData::nullValueId);
    }

    double keyGap = 0.0;
    long nep = 0;
    int size = 0;
    vector<vector<int>> clusters = vector<vector<int>>();

    for (auto & iter : index){
        if (iter.second.size() >= 2){
            keyGap += iter.second.size() * log(iter.second.size());
            nep += calculateNep(iter.second.size());
            size += iter.second.size();

            clusters.emplace_back(std::move(iter.second));
        }
    }
    double entropy = log(data.size()) - keyGap / data.size();

    sortClusters(clusters);
    auto pli = shared_ptr<PositionListIndex>(new PositionListIndex(clusters, nullCluster, size, entropy, nep, data.size(), data.size()));
    return pli;
}

long PositionListIndex::calculateNep(long numElements) {
    return numElements * (numElements - 1) / 2;
}

void PositionListIndex::sortClusters(vector<vector<int>> &clusters) {
    sort(clusters.begin(), clusters.end(), [](vector<int> & a, vector<int> & b){
        return a[0] < b[0];
    });
}

vector<int> PositionListIndex::getProbingTable() {
    return getProbingTable(false);
}

vector<int> PositionListIndex::getProbingTable(bool isCaching) {
    if (!probingTableCache.empty()) return probingTableCache;
    vector<int> probingTable = vector<int>(originalRelationSize);
    int nextClusterId = singletonValueId + 1;
    for (auto & cluster : index){
        int valueId = nextClusterId++;
        assert(valueId != singletonValueId);
        for(int & position : cluster){
            probingTable[position] = valueId;
        }
    }
    if (isCaching) {
        probingTableCache = std::move(probingTable);
        return probingTableCache;
    }
    return probingTable;
}

vector<vector<int>> & PositionListIndex::getIndex() {
    return index;
}

shared_ptr<PositionListIndex> PositionListIndex::intersect(shared_ptr<PositionListIndex> that) {
    assert(this->relationSize == that->relationSize);
    auto result = this->size > that->size ?
            that->probe(this->getProbingTable()) :
            this->probe(that->getProbingTable());
    return result;
}

//TODO: nullCluster некорректен
shared_ptr<PositionListIndex> PositionListIndex::probe(vector<int> probingTable) {
    assert(this->relationSize == probingTable.size());
    vector<vector<int>> newIndex;
    int newSize = 0;
    double newKeyGap = 0.0;
    long newNep = 0;
    vector<int> nullCluster;

    map<int, vector<int>> partialIndex;

    for (auto & positions : index){
        for (int & position : positions){
            int probingTableValueId = probingTable[position];
            if (probingTableValueId == singletonValueId) continue;
            partialIndex[probingTableValueId].push_back(position);
        }

        for (auto & iter : partialIndex){
            auto & cluster = iter.second;
            if (cluster.size() == 1) continue;

            newSize += cluster.size();
            newKeyGap += cluster.size() * log(cluster.size());
            newNep += calculateNep(cluster.size());
            newIndex.emplace_back(std::move(cluster));
        }

        partialIndex.clear();
    }

    double newEntropy = log(relationSize) - newKeyGap / relationSize;

    sortClusters(newIndex);

    auto ans = shared_ptr<PositionListIndex>(new PositionListIndex(newIndex, nullCluster, newSize, newEntropy, newNep, relationSize, relationSize));
    return ans;
}


//TODO: nullCluster не поддерживается
shared_ptr<PositionListIndex> PositionListIndex::probeAll(Vertical probingColumns, ColumnLayoutRelationData & relationData) {
    assert(this->relationSize == relationData.getNumRows());

    vector<vector<int>> newIndex;
    int newSize = 0;
    double newKeyGap = 0.0;
    long newNep = 0;

    map<vector<int>, vector<int>> partialIndex;
    vector<int> nullCluster;
    vector<int> probe;

    for (auto & cluster : this->index){
        for (int & position : cluster){
            if (!takeProbe(position, relationData, probingColumns, probe)){
                probe.clear();
                continue;
            }

            partialIndex[probe].push_back(position);
            probe.clear();
        }

        for (auto & iter : partialIndex){
            auto & newCluster = iter.second;
            if (newCluster.size() == 1) continue;

            newSize += newCluster.size();
            newKeyGap += newCluster.size() * log(newCluster.size());
            newNep += calculateNep(newCluster.size());

            newIndex.emplace_back(std::move(newCluster));
        }
        partialIndex.clear();
    }

    double newEntropy = log(this->relationSize) - newKeyGap / this->relationSize;

    sortClusters(newIndex);

    auto ans = shared_ptr<PositionListIndex>(new PositionListIndex(newIndex, nullCluster, newSize, newEntropy, newNep, this->relationSize, this->relationSize));
    return ans;
}

bool PositionListIndex::takeProbe(int position, ColumnLayoutRelationData & relationData, Vertical & probingColumns, vector<int> & probe){
    dynamic_bitset<> probingIndices = probingColumns.getColumnIndices();
    for (unsigned long index = probingIndices.find_first(); index < probingIndices.size(); index = probingIndices.find_next(index)){
        int value = relationData.getColumnData(index)->getProbingTableValue(position);
        if (value == PositionListIndex::singletonValueId) return false;
        probe.push_back(value);
    }
    return true;
}
