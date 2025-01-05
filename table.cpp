/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 * 
 * TableCT Implementation: Tim Curry
 */

#include "table.hpp"
#include <string.h>

void TableCT::post()
{
   int currIndex = 0;
   for (const auto& vp : _vars) {
      _entries.emplace(vp->getId(), Entry(currIndex, vp->min(), vp->max()));  // build entries for each var
      _lastSizes.push_back(trail<int>(vp->getSolver()->getStateManager(), vp->size()));  // record initial var domain size
      _supports.emplace_back(std::vector<StaticBitSet>(0));  // init vector of bitsets for var's supports
      _residues.emplace_back(std::vector<int>());
      std::vector<StaticBitSet>& v = _supports.back();
      std::vector<int>& r = _residues.back();
      for (int val=vp->min(); val <= vp->max(); val++) {
         v.emplace_back(StaticBitSet((int)_table.size()));  // make bitset for this value
         // loop through table to see which table vectors support this value
         if (vp->contains(val)) { // if the value is in the domain.... We need to collate all the tuples supporting it.
            for (auto i=0u; i < _table.size(); i++) {
               const std::vector<int>& row = _table[i];
               if (row[currIndex] != val) // if the tuple in this row does not use the value, it does not support it. 
                  v.back().remove(i);  // so remove the tuple fom the supports of this value (since we start with all supports)
            }
         } else { // [ldm] this was missing: val NOT IN  D(vp) => all tuples using val for this column must be removed.
            for (auto i=0u; i < _table.size(); i++) {
               const std::vector<int>& row = _table[i];
               if (row[currIndex] == val)
                  v.back().remove(i);
            }
            // v.back()_i == 0 iff tuple_i(vp) == val AND val NOT IN D(vp) 
         }
         r.push_back(_currTable.intersectIndex(v.back()));
      }
      currIndex++;
   }
   // [ldm] this was missing. We need to clear tuples that use values not in the domain of the variables.
   // So loop over tuples in table. Check each var, its domain must contain the value appearing in the tuple
   // Assuming D(x) = min..max, since we already cleared tuples for values between min..max, only clear outside now.
   for (auto i=0u; i < _table.size(); i++) {
      for (const auto& vp : _vars) {
         const auto& rangeVP = _entries.at(vp->getId());
         const auto rowVal = _table[i][rangeVP.getIndex()];
         if (rowVal < rangeVP.getMin()  || rowVal > rangeVP.getMax()) {            
            _currTable.clearBit(i);
            //std::cout << "var " << rangeVP.getIndex() << " -> clear " << i << '/' << rowVal <<  " CT:" << _currTable << "\n";
         }
      }
   }
   //std::cout << "table::post \tCT:" << _currTable << "\n";
   propagate();
   for (const auto& vp : _vars)
      vp->propagateOnDomainChange(this); // for each variable, propagate on change to domain
}

void TableCT::propagate()  // enforceGAC
{
   // calculate Sval
   _Sval.clear();
   _Ssup.clear();
   for (const auto& vp : _vars) {
      const int varIndex = _entries.at(vp->getId()).getIndex();
      if (vp->size() != _lastSizes[varIndex]) {
         _Sval.push_back(vp);
         _lastSizes[varIndex] = vp->size();  // update lastSizes for vars in Sval
      }
      // calculate Ssup
      if (vp->size() > 1)
         _Ssup.push_back(vp);
   }
   updateTable();
   if (_currTable.isEmpty())
      failNow();
   filterDomains();
}

void TableCT::filterDomains()
{
   for (const auto& vp : _Ssup) {
      const int varIndex = _entries.at(vp->getId()).getIndex();
      const int iMin = _entries.at(vp->getId()).getMin();
      for (int val = vp->min(); val < vp->max() + 1; val++) {
         const int valIndex = val - iMin;
         int wIndex = _residues[varIndex][valIndex];
         if (wIndex == -1) 
            vp->remove(val);         
         else {
            if ((_currTable[wIndex] & _supports[varIndex][valIndex][wIndex]) == 0) {
               wIndex = _currTable.intersectIndex(_supports[varIndex][valIndex]);
               if (wIndex != -1) 
                  _residues[varIndex][valIndex] = wIndex;              
               else 
                  vp->remove(val);               
            }
         }
      }
      _lastSizes[varIndex] = vp->size();
   }
}

void TableCT::updateTable()
{
   for (const auto& vp : _Sval) {
      const int varIndex = _entries.at(vp->getId()).getIndex();
      const int iMin = _entries.at(vp->getId()).getMin();
      _currTable.clearMask();
      for (int val = vp->min(); val < vp->max() + 1; val++) {
         const int valIndex = val - iMin;
         if (vp->contains(val))
            _currTable.addToMask(_supports[varIndex][valIndex]);
      }
      _currTable.intersectWithMask();
      if (_currTable.isEmpty())
         break;
   }
}
