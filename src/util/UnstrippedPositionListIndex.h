#pragma once

#include "Column.h"
#include "PositionListIndex.h"

class UnstrippedPositionListIndex : private PositionListIndex {
  public:
    static std::unique_ptr<PositionListIndex> createUnstrippedFor(std::vector<int>& data, bool isNullEqNull);
    UnstrippedPositionListIndex(std::deque<std::vector<int>> index, std::vector<int> nullCluster,
                      unsigned int size, double entropy,
                      unsigned long long nep, unsigned int relationSize, unsigned int originalRelationSize,
                      double invertedEntropy = 0, double giniImpurity = 0) : PositionListIndex(index, nullCluster,
                      size, entropy,
                      nep, relationSize, originalRelationSize,
                      invertedEntropy, giniImpurity) {};

    unsigned int getNumCluster()  const override { return index.size() + originalRelationSize - size; }
    // unsigned int getNumCluster()  const override { unsigned int sum = 0;
    // for (auto &i : index) {
    //     sum += i.size();
    // }
    // // std::cout <<  originalRelationSize - sum << " " << singletones << std::endl;
    // return  index.size() + originalRelationSize - sum;}

    std::unique_ptr<PositionListIndex> probe(std::shared_ptr<const std::vector<int>> probingTable) const override;
    std::unique_ptr<PositionListIndex> intersect(PositionListIndex const* that) const override;
    ~UnstrippedPositionListIndex() {};
    // virtual ~UnstrippedPositionListIndex()  = default;
};