//
// Created by kek on 26.07.2019.
//

#include "model/RelationData.h"

const int RelationData::nullValueId = -1;

RelationData::RelationData(shared_ptr<RelationalSchema>& schema): schema(schema) {}

int RelationData::getNumColumns() {
    return schema->getNumColumns();
}

shared_ptr<RelationalSchema> RelationData::getSchema() {
    return schema;
}