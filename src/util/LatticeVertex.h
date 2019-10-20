// Strutovsky, 20.08.2019

#pragma once

#include <list>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "model/RelationalSchema.h"
#include "util/PositionListIndex.h"
//#include "../model/Column.h"
//#include "../model/Vertical.h"


//enable_shared_from_this - if LV needs to create shared_ptr
class LatticeVertex{
private:
  //use pointer to vertical?
  Vertical vertical;
  std::shared_ptr<PositionListIndex> positionListIndex;
  dynamic_bitset<> rhsCandidates;
  // Using member initialization?
  bool isKeyCandidate = false;
  //list => vector due to usage of opertor[] in Tane
  std::vector<std::shared_ptr<LatticeVertex>> parents;
  bool isInvalid = false;

public:
  //TODO: no default initialization of PLI
  explicit LatticeVertex(Vertical&& _vertical) : vertical(_vertical), rhsCandidates(vertical.getSchema()->getNumColumns()) {}
  explicit LatticeVertex(Vertical& _vertical) : vertical(_vertical), rhsCandidates(vertical.getSchema()->getNumColumns()) {}

  std::vector<std::shared_ptr<LatticeVertex>>& getParents() { return parents; }
  //TODO: const - usually other Metanome classes use these otputs, so returning const isn't possible
  Vertical& getVertical() { return vertical; }
  boost::dynamic_bitset<>& getRhsCandidates() { return rhsCandidates; }

  void addRhsCandidates(std::vector<std::shared_ptr<Column>>&& candidates);

  //dynamic_bitset getBlockingPrefix();
  bool comesBeforeAndSharePrefixWith(LatticeVertex& that);
  bool getIsKeyCandidate() const { return isKeyCandidate; }
  void setKeyCandidate(bool m_isKeyCandidate) { isKeyCandidate = m_isKeyCandidate; }
  bool getIsInvalid() const { return isInvalid; }
  void setInvalid(bool m_isInvalid) { isInvalid = m_isInvalid; }

  //OK to store AND return ptr to PLI?
  std::shared_ptr<PositionListIndex> getPositionListIndex() { return positionListIndex; }
  void setPositionListIndex(shared_ptr<PositionListIndex> m_positionListIndex) { positionListIndex = m_positionListIndex; }

  //right analogy to compareTo?
  bool operator> (LatticeVertex& that);

  string toString() { return "Vtx" + vertical.toString(); }
};
