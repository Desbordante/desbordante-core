// Strutovsky, 21.08

#include "LatticeLevel.h"
#include "../model/ColumnCombination.h"
#include <algorithm>
#include <iostream>


using std::move, std::min, std::shared_ptr, std::vector, std::cout, std::endl, std::sort, std::make_shared;

void LatticeLevel::add(shared_ptr<LatticeVertex> vertex) {
  vertices.emplace(vertex->getVertical().getColumnIndices(), vertex);
}

//necessary to check ''!= nullptr'?
std::shared_ptr<LatticeVertex> LatticeLevel::getLatticeVertex(const boost::dynamic_bitset<>& columnIndices) {
  auto it = vertices.find(columnIndices);
  if (it != vertices.end()){
    return it->second;
  }
  else{
    return nullptr;
  }
}

void LatticeLevel::generateNextLevel(vector<shared_ptr<LatticeLevel>>& levels) {
  int arity = (int)levels.size() - 1;
  cout << "Creating level " << arity + 1 << "..." << endl;
  shared_ptr<LatticeLevel> currentLevel = levels[arity];

  //using vector because of 'get()''
  vector<shared_ptr<LatticeVertex>> currentLevelVertices;
  for (const auto& [map_key, vertice] : currentLevel->getVertices()){
    currentLevelVertices.push_back(vertice);
  }
  std::sort(currentLevelVertices.begin(), currentLevelVertices.end());
  LatticeLevel nextLevel(arity + 1);

  for (unsigned int vertexIndex1 = 0; vertexIndex1 < currentLevelVertices.size(); vertexIndex1++){
    shared_ptr<LatticeVertex> vertex1 = currentLevelVertices[vertexIndex1];
    if (vertex1->getRhsCandidates().empty() && !vertex1->getIsKeyCandidate()) {
      continue;
    }

    for (unsigned int vertexIndex2 = vertexIndex1 + 1; vertexIndex2 < currentLevelVertices.size(); vertexIndex2++){
      shared_ptr<LatticeVertex> vertex2 = currentLevelVertices[vertexIndex2];

      if (!vertex1->comesBeforeAndSharePrefixWith(*vertex2)){
        //TODO: perhaps remake comesB... using pointer instead of reference -- check it out
        break;
      }

      if (!vertex1->getRhsCandidates().none() && !vertex2->getIsKeyCandidate()){
        continue;
      }

      shared_ptr<ColumnCombination> childColumns = make_shared<ColumnCombination>(ColumnCombination(vertex1->getVertical().Union(vertex2->getVertical())));
       //TODO: check out if this cast is OK - had to add conversion constructor to CC
      shared_ptr<LatticeVertex> childVertex = make_shared<LatticeVertex>(*childColumns);

      dynamic_bitset parentIndices;
      parentIndices |= vertex1->getVertical().getColumnIndices();
      parentIndices |= vertex2->getVertical().getColumnIndices();

      childVertex->getRhsCandidates() |= vertex1->getRhsCandidates();
      childVertex->getRhsCandidates() &= vertex2->getRhsCandidates();
      childVertex->setKeyCandidate(vertex1->getIsKeyCandidate() && vertex2->getIsKeyCandidate());
      childVertex->setInvalid(vertex1->getIsInvalid() || vertex2->getIsInvalid());

      for (int i = 0, skipIndex = parentIndices.find_first(); i < arity - 1; i++, skipIndex = parentIndices.find_next(skipIndex + 1)){
        parentIndices[skipIndex] = false;
        shared_ptr<LatticeVertex> parentVertex = currentLevel->getLatticeVertex(parentIndices);
        if (parentVertex == nullptr){
          goto continueMidOuter;
        }
        childVertex->getRhsCandidates() &= parentVertex->getRhsCandidates();
        if (childVertex->getRhsCandidates().empty()){
          goto continueMidOuter;
        }
        childVertex->getParents().push_back(parentVertex);
        parentIndices[skipIndex] = true;

        childVertex->setKeyCandidate(childVertex->getIsKeyCandidate() && parentVertex->getIsKeyCandidate());
        childVertex->setInvalid(childVertex->getIsInvalid() || parentVertex->getIsInvalid());

        if (!childVertex->getIsKeyCandidate() && childVertex->getRhsCandidates().empty()){
          goto continueMidOuter;
        }
      }

      childVertex->getParents().push_back(vertex1);
      childVertex->getParents().push_back(vertex2);

continueMidOuter:
      continue;
    }
  }

  levels.push_back(make_shared<LatticeLevel>(nextLevel));
}

void LatticeLevel::clearLevelsBelow(vector<shared_ptr<LatticeLevel>>& levels, int arity) {
  // Clear the levels from the level list
  auto it = levels.begin();
  for (int i = 0; i < min((int)levels.size(), arity); i++){
    (*(it++))->getVertices().clear();
  }

  //Clear child references
  if (arity < (signed int)levels.size()){ //(signed int) => no warning
    for (auto [map_key, retainedVertex] : levels[arity]->getVertices()){
      retainedVertex->getParents().clear();
    }
  }
}
