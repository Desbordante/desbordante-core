//
// Created by kek on 30.07.2019.
//

#include "PositionListIndex.h"
#include "../model/RelationData.h"
#include <map>
#include <cmath>
#include <algorithm>
#include <utility>

using namespace std;

const int PositionListIndex::singletonValueId = 0;

PositionListIndex::PositionListIndex(vector<vector<int>> index, vector<int> nullCluster, int size, double entropy,
                                     long nep, int relationSize, int originalRelationSize):
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
        index[RelationData::nullValueId].clear();
    }
    double keyGap = 0.0;
    long nep = 0;
    int size = 0;
    for (auto & iter : index){
        if (iter.second.size() <= 1){
            iter.second.clear();
            continue;
        }

        keyGap += iter.second.size() * log(iter.second.size());
        nep += calculateNep(iter.second.size());
        size += iter.second.size();
    }

    double entropy = log(data.size()) - keyGap / data.size();
    vector<vector<int>> clusters = vector<vector<int>>();
    for (auto & iter : index) {
        if (!iter.second.empty())
            clusters.emplace_back(std::move(iter.second));
    }

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