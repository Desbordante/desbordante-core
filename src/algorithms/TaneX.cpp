// Strutovsky, 29.08.2019

#include "TaneX.h"
#include "../model/ColumnLayoutRelationData.h"
#include "../model/RelationalSchema.h"
#include "../model/ColumnData.h"
#include "../model/ColumnCombination.h"
#include "../util/LatticeVertex.h"
#include "../util/LatticeLevel.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <list>

using boost::dynamic_bitset, std::make_shared, std::shared_ptr, std::cout, std::endl, std::setw, std::vector, std::list;


void Tane::execute(){

  shared_ptr<ColumnLayoutRelationData> relation = ColumnLayoutRelationData::createFrom(inputGenerator, true);
  //TODO: this row won't work at all, so dig into this topic (because of inputGen initialization absence)

  shared_ptr<RelationalSchema> schema = relation->getSchema();
  cout << schema->getName() << " has " << relation->getNumColumns() << " columns, "
       << relation->getNumRows() << " rows, and a maximum NIP of " << setw(2)
       << relation->getMaximumNip() << "." << endl;

  for (auto column : schema->getColumns()){
    shared_ptr<ColumnData> columnData = relation->getColumnData(column->getIndex());
    double avgPartners = columnData->getPositionListIndex()->getNep() * 2.0 / relation->getNumRows();
    cout << "* " << column->toString() << ": every tuple has " << setw(2)
         << avgPartners << " partners on average." << endl;
  }

  vector<shared_ptr<LatticeLevel>> levels;
  shared_ptr<LatticeLevel> level0 = make_shared<LatticeLevel>(0);
  shared_ptr<LatticeVertex> emptyVertex = make_shared<LatticeVertex>(schema->emptyVertical);
  level0->add(emptyVertex);
  levels.push_back(level0);

  dynamic_bitset zeroaryFdRhs;
  shared_ptr<LatticeLevel> level1 = make_shared<LatticeLevel>(1);
  for (auto column : schema->getColumns()){
    shared_ptr<ColumnData> columnData = relation->getColumnData(column->getIndex());
    shared_ptr<LatticeVertex> vertex = make_shared<LatticeVertex>(column);
    vertex->addRhsCandidates(schema->getColumns());
    vertex->getParents().push_back(emptyVertex);
    vertex->setKeyCandidate(true);
    vertex->setPositionListIndex(columnData->getPositionListIndex());
    level1->add(vertex);

    double fdError = fdErrorMeasure;  //TODO: error
    if (fdError <= maxFdError){  //TODO: max_error
      zeroaryFdRhs.set(column->getIndex());
      //TODO: registerFd
      vertex->getRhsCandidates().set(column->getIndex(), false);
      if (fdError == 0){
        vertex->getRhsCandidates().clear();
      }
    }
  }

  for (auto [key_map, vertex] : level1->getVertices()){
    shared_ptr<Column> column = (Column) vertex->getVertical();
    //TODO: tricky conversion: it looks like we originally constructed  vertex using columns,
    //so level1 has pointer to Vertices that actually point to columns, threfore we should be able to cast them somehow back to *columns
    vertex->getRhsCandidates() &= ~zeroaryFdRhs;  //~ returns flipped copy

    shared_ptr<ColumnData> columnData = relation->getColumnData(column->getIndex());
    double uccError = uccErrorMeasure;  //TODO: uccErrorMeasure
    if (uccError <= maxUccError){
      //TODO: do something with discovered UCC
      vertex->setKeyCandidate(false);
      if (uccError == 0){
        for (int rhsIndex = vertex->getRhsCandidates().find_first();
             rhsIndex != dynamic_bitset<>::npos;
             rhsIndex = vertex->getRhsCandidates().find_next(rhsIndex + 1)){
          if (rhsIndex != column->getIndex()){
            //TODO: do smth
          }
        }
        vertex->getRhsCandidates() &= column->getColumnIndices();
        if (maxFdError == 0 && maxUccError == 0){     //TODO: errors
          vertex->setInvalid(true);
        }
      }
    }
  }
  levels.push_back(level1);

  //TODO: configuration.maxArity
  for (int arity = 2; arity <= maxArity || maxArity <= 0; arity++){
    auto startTime = std::chrono::system_clock::now();
    LatticeLevel::clearLevelsBelow(levels, arity - 1);
    LatticeLevel::generateNextLevel(levels);
    std::chrono::duration<double> elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime);
    aprioriMillis += elapsed_milliseconds.count();

    shared_ptr<LatticeLevel> level = levels[arity];   //TODO: careful, should write arity-1?
    cout << "Checking " << level->getVertices().size() << " " << arity << "-ary lattice vertices." << endl;
    if (level->getVertices().empty()){
      break;
    }

    for (auto [key_map, xaVertex] : level->getVertices()){
      if (xaVertex->getIsInvalid()){
        continue;
      }

      //TODO: check the following conversion:
      shared_ptr<ColumnCombination> xa = ((ColumnCombination) xaVertex->getVertical());
      if (xaVertex->getPositionListIndex() == nullptr){
        shared_ptr<PositionListIndex> parentPLI1 = xaVertex->getParents()[0]->getPositionListIndex();   //OK??
        shared_ptr<PositionListIndex> parentPLI2 = xaVertex->getParents()[1]->getPositionListIndex();
        xaVertex->setPositionListIndex(parentPLI1->intersect(parentPLI2));
      }

      dynamic_bitset xaIndices = xa->getColumnIndices();
      dynamic_bitset aCandidates = xaVertex->getRhsCandidates();

      for (auto xVertex : xaVertex->getParents()){
        shared_ptr<Vertical> lhs = xVertex->getVertical();
        //TODO: faster to return shared_ptr to a lhs and use it afterwards or to return a & and call getVertical everytime?

        int aIndex = xaIndices.find_first();
        dynamic_bitset xIndices = lhs->getColumnIndices();
        while (xIndices[aIndex]){
          aIndex = xaIndices.find_next(aIndex + 1);
        }
        if (!aCandidates[aIndex]){
          continue;
        }

        double error = 0.2; //TODO: implement calculateFdError(...)
        if (error <= maxFdError){  //TODO: and again...
          shared_ptr<Column> rhs = schema->getColumns()[aIndex];
          //TODO: register FD
          xaVertex->getRhsCandidates().set(rhs->getIndex(), false);
          if (error == 0){
            xaVertex->getRhsCandidates() &= lhs->getColumnIndices();
          }
        }
      }
    }

    list<shared_ptr<LatticeVertex>> keyVertices;
    for (auto [map_key, vertex] : level->getVertices()){
      shared_ptr<ColumnCombination> columns = vertex->getVertical();

      if (vertex->getIsKeyCandidate()){
        double uccError = uccErrorMeasure; //TODO
        if (uccError <= maxUccError){
          //TODO: smth with UCC
          vertex->setKeyCandidate(false);
          if (uccError == 0){
            for (int rhsIndex = vertex->getRhsCandidates().find_first();
                 rhsIndex != dynamic_bitset<>::npos;
                 rhsIndex = vertex->getRhsCandidates().find_next(rhsIndex + 1)){
              shared_ptr<Column> rhs = schema->getColumn(rhsIndex);
              if (!columns->contains(rhs)){         //TODO: again this conversion
                bool isRhsCandidate = true;
                for (auto column : columns->getColumns()){ //TODO: implement getColumns
                  shared_ptr<Vertical> sibling = columns->without(column).union(rhs);   //check types carfully
                  shared_ptr<LatticeVertex> siblingVertex = level->getLatticeVertex(sibling->getColumnIndices());
                  if (siblingVertex == nullptr || !siblingVertex->getRhsCandidates()[rhs->getIndex()]){
                    isRhsCandidate = false;
                    break;
                  }
                }
                if (isRhsCandidate){
                  //TODO: smth with FD
                }
              }
              keyVertices.push_back(vertex);
            }
          }
        }
      }
      if (maxFdError == 0 && maxUccError == 0){
        for (auto keyVertex : keyVertices){
          keyVertex->getRhsCandidates() &= keyVertex->getVertical().getColumnIndices();
          keyVertex->setInvalid(true);
        }
      }
    }

    //TODO: printProfilingData
  }
}
