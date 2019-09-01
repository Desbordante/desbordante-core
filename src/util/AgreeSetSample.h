//
// Created by kek on 17.08.2019.
//

#pragma once

#include <type_traits>
#include "../model/ColumnLayoutRelationData.h"
#include "../model/Vertical.h"
#include <boost/dynamic_bitset.hpp>
#include <map>

using std::enable_if, std::is_base_of, boost::dynamic_bitset, std::map;

class AgreeSetSample {
public:

protected:

    shared_ptr<ColumnLayoutRelationData> relationData;
    shared_ptr<Vertical> focus;
    int sampleSize;
    long populationSize;
    AgreeSetSample(shared_ptr<ColumnLayoutRelationData> relationData, shared_ptr<Vertical> focus, int sampleSize, long populationSize);

    template<typename T, typename enable_if<is_base_of<AgreeSetSample, T>::value>::type* = nullptr>
    static shared_ptr<T> createFor(shared_ptr<ColumnLayoutRelationData> relationData, int sampleSize);

    template<typename T, typename enable_if<is_base_of<AgreeSetSample, T>::value>::type* = nullptr>
    static shared_ptr<T> createFocusedFor(shared_ptr<ColumnLayoutRelationData> relation,
                                          shared_ptr<Vertical> restrictionVertical,
                                          shared_ptr<PositionListIndex> restrictionPli,
                                          int sampleSize);
};
