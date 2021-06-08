#pragma once

#include "Column.h"
#include "ColumnLayoutRelationData.h"
#include "PositionListIndex.h"
#include "UnstrippedPositionListIndex.h"
#include "Vertical.h"


std::unique_ptr<PositionListIndex> UnstrippedPositionListIndex::createUnstrippedFor(std::vector<int>& data, bool isNullEqNull) {
    std::unordered_map<int, std::vector<int>> index;
    for (unsigned long position = 0; position < data.size(); ++position){
        int valueId = data[position];
        index[valueId].push_back(position);
    }

    std::vector<int> nullCluster;
    if (index.count(RelationData::nullValueId) != 0) {
        nullCluster = index[RelationData::nullValueId];
    }
    if (!isNullEqNull){
        index.erase(RelationData::nullValueId); // move?
    }

    double keyGap = 0.0;
    double invEnt = 0;
    double giniGap = 0;
    unsigned long long nep = 0;
    unsigned int size = 0;
    std::deque<std::vector<int>> clusters;

    for (auto & iter : index){
        if (iter.second.size() == 1){
            giniGap += std::pow(1 / static_cast<double>(data.size()), 2);
            continue;
        }
        keyGap += iter.second.size() * log(iter.second.size());
        nep += PositionListIndex::calculateNep(iter.second.size());
        size += iter.second.size();
        invEnt += -(1 - iter.second.size() / static_cast<double>(data.size()))
                * std::log(1 - (iter.second.size() / static_cast<double>(data.size())));
        giniGap += std::pow(iter.second.size() / static_cast<double>(data.size()), 2);

        clusters.emplace_back(std::move(iter.second));
    }
    double entropy = log(data.size()) - keyGap / data.size();

    double giniImpurity = 1 - giniGap;
    if (giniImpurity == 0) {
        invEnt = 0;
    }
    
    PositionListIndex::sortClusters(clusters);
    return std::unique_ptr<PositionListIndex>(new UnstrippedPositionListIndex(std::move(clusters), std::move(nullCluster),
                                               size, entropy, nep, data.size(), data.size(), invEnt, giniImpurity));
}

std::unique_ptr<PositionListIndex> UnstrippedPositionListIndex::intersect(PositionListIndex const* that) const {
    assert(this->relationSize == that->relationSize);
    return this->size > that->size ?
            that->probe(this->calculateAndGetProbingTable()) :
            this->probe(that->calculateAndGetProbingTable());
}

std::unique_ptr<PositionListIndex> UnstrippedPositionListIndex::probe(std::shared_ptr<const std::vector<int>> probingTable) const {
    assert(this->relationSize == probingTable->size());
    std::deque<std::vector<int>> newIndex;
    unsigned int newSize = 0;
    double newKeyGap = 0.0;
    unsigned long long newNep = 0;
    std::vector<int> nullCluster;

    std::unordered_map<int, std::vector<int>> partialIndex;

    
    for (auto & positions : index){
        for (int position : positions){
            int probingTableValueId = (*probingTable)[position];
            if (probingTableValueId == singletonValueId)
                continue;
            intersectionCount++;
            partialIndex[probingTableValueId].push_back(position);
        }

        for (auto & iter : partialIndex){
            auto & cluster = iter.second;
            if (cluster.size() <= 1) {
                continue;
            }

            newSize += cluster.size();
            newKeyGap += cluster.size() * log(cluster.size());
            newNep += calculateNep(cluster.size());

            newIndex.push_back(std::move(cluster));
        }
        partialIndex.clear();
    }

    double newEntropy = log(relationSize) - newKeyGap / relationSize;
    sortClusters(newIndex);

    return std::unique_ptr<PositionListIndex>(new UnstrippedPositionListIndex(std::move(newIndex), std::move(nullCluster),
                                               newSize, newEntropy, newNep, relationSize, relationSize, 0, 0));
}