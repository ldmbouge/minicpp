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
 */

#ifndef __TABLE_H
#define __TABLE_H

#include <set>
#include <array>
#include <map>
#include <algorithm>
#include <iomanip>
#include <stdint.h>
#include "matrix.hpp"
#include "intvar.hpp"
#include "acstr.hpp"
#include "bitset.hpp"

class TableCT : public Constraint {
   class Entry {
      const int _index;
      const int _min;
      const int _max;
   public:
      Entry(int index, int min, int max) :
         _index(index),
         _min(min),
         _max(max)
      {}
      int getIndex() const noexcept  { return _index;}
      int getMin() const noexcept    { return _min;}
      int getMax() const noexcept    { return _max;}
   };
   std::map<int, Entry>                                _entries;
   std::vector<std::vector<StaticBitSet>>              _supports;
   std::vector<std::vector<int>>                       _table;
   std::vector<std::vector<int>>                       _residues;
   SparseBitSet                                        _currTable;
   std::vector<var<int>::Ptr>                          _vars;
   std::vector<var<int>::Ptr>                          _Sval;
   std::vector<var<int>::Ptr>                          _Ssup;
   std::vector<trail<int>>                             _lastSizes;
   void                                                filterDomains();
   void                                                updateTable();
public:
   template <class Vec> TableCT(const Vec& x, const std::vector<std::vector<int>>& table)
      : Constraint(x[0]->getSolver()),
        _table(table),
        _currTable(x[0]->getSolver()->getStateManager(), x[0]->getStore(), table.size())  // build SparseBitSet for vectors in table
   {
      for (const auto& vp : x) 
         _vars.push_back(vp);
   }
   ~TableCT() {}
   void post() override;
   void propagate() override;
};

namespace Factory {
   template <class Vec, class T> Constraint::Ptr table(const Vec& x, const T& table) {
      return new (x[0]->getSolver()) TableCT(x, table);
   }
};

#endif
