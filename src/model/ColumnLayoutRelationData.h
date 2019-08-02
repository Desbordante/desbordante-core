//
// Created by kek on 26.07.2019.
//

#pragma once

#include "RelationData.h"
#include "RelationalSchema.h"
#include "ColumnData.h"
#include "../parser/CSVParser.h"
#include <vector>

using std::vector;

class ColumnLayoutRelationData : public RelationData {
private:
    vector<shared_ptr<ColumnData>> columnData;
    ColumnLayoutRelationData(shared_ptr<RelationalSchema>& schema, vector<shared_ptr<ColumnData>> columnData);

public:
    vector<shared_ptr<ColumnData>> getColumnData() override;
    shared_ptr<ColumnData> getColumnData(int columnIndex) override;
    int getNumRows() override;
    vector<int> getTuple(int tupleIndex) override;
    void shuffleColumns() override;

    static shared_ptr<ColumnLayoutRelationData> createFrom(CSVParser& fileInput, bool isNullEqNull);
    static shared_ptr<ColumnLayoutRelationData> createFrom(CSVParser& fileInput, bool isNullEqNull, int maxCols, long maxRows);
};