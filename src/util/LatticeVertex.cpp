//Strutovsky, 20.08

#include "LatticeVertex.h"

using boost::dynamic_bitset, std::list, std::shared_ptr, std::make_shared, std::string;
//70% right analogy
void LatticeVertex::addRhsCandidates(const list<shared_ptr<Vertical>>& candidates) {
  for (auto candPtr : candidates){
    rhsCandidates |= candPtr->getColumnIndices();
  }
}

//need to make getColumnIndices a const method => pass 'const& that'
bool LatticeVertex::comesBeforeAndSharePrefixWith(LatticeVertex& that) {
  dynamic_bitset<> thisIndices = vertical.getColumnIndices();
  dynamic_bitset<> thatIndices = that.vertical.getColumnIndices();

  int thisIndex = thisIndices.find_first();
  int thatIndex = thatIndices.find_first();

  int arity = thisIndices.count();
  for (int i = 0; i < arity - 1; i++){
    if (thisIndex != thatIndex) return false;
    thisIndex = thisIndices.find_next(thisIndex + 1);
    thatIndex = thatIndices.find_next(thatIndex + 1);
  }

  return thisIndex < thatIndex;
}

//same here
bool LatticeVertex::operator> (LatticeVertex& that) {
  int result = vertical.getArity() - that.vertical.getArity();
  if (result)
    return result;

  dynamic_bitset thisIndices = vertical.getColumnIndices();
  int thisIndex = thisIndices.find_first();
  dynamic_bitset thatIndices = that.vertical.getColumnIndices();
  int thatIndex = thatIndices.find_first();

  while (true){
    result = thisIndex - thatIndex;
    if (result)
      return result;
    thisIndex = thisIndices.find_next(thisIndex + 1);
    thatIndex = thatIndices.find_next(thatIndex + 1);
  }
}
