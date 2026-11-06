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

// Element constraints: z == t[x][y] (2D), z == t[y] (bounds, basic and domain
// consistent variants) and z == xs[y] over an array of variables.
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"
#include "matrix.hpp"
#include "domain.hpp"

class Element2D : public Constraint {
   struct Triplet {
      int x,y,z;
      Triplet() : x(0),y(0),z(0) {}
      Triplet(int a,int b,int c) : x(a),y(b),z(c) {}
      Triplet(const Triplet& t) : x(t.x),y(t.y),z(t.z) {}
   };
   Matrix<int,2> _matrix;
   var<int>::Ptr _x,_y,_z;
   int _n,_m;
   trail<int>* _nRowsSup;
   trail<int>* _nColsSup;
   trail<int> _low,_up;
   std::vector<Triplet> _xyz;
   void updateSupport(int lostPos);
public:
   Element2D(const Matrix<int,2>& mat,var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z);
   void post() override;;
   void propagate() override;
   void print(std::ostream& os) const override;
};

class Element1D : public Constraint {
   std::vector<int> _t;
   var<int>::Ptr _y;
   var<int>::Ptr _z;
public:
   Element1D(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z);
   void post() override;
};

class Element1DBasic : public Constraint { // z == t[y]
   std::vector<int> _t;
   var<int>::Ptr _y;
   var<int>::Ptr _z;
   struct Pair {
      int _k;
      int _v;
      Pair() : _k(0),_v(0) {}
      Pair(int k,int v) : _k(k),_v(v) {}
      Pair(const Pair& p) : _k(p._k),_v(p._k) {}
   };
   Pair* _kv;
   trail<int> _from,_to;
public:
   Element1DBasic(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z);
   void post() override;
   void propagate() override;
   void print(std::ostream& os) const override;
};

class Element1DDC : public Constraint { // z =DC= t[y] (Domain consistent version)
   std::vector<int> _t;
   var<int>::Ptr    _y;
   var<int>::Ptr    _z;
   // Internal state
   struct DCIndex {    // for each value v in D(z) we maintain the support and the head of a list
      int        _v;   // the actual value from D(z)
      int        _k;   // the first index in t s.t. t[k]==v. The next index is in _list[k] (i.e., t[list[k]]==v)
      trail<int> _s;   // |{j in D(y) : t[j] == v }|
   }; 
   int      _endOfList; // endoflist marker
   DCIndex*    _values; // an array that holds, for each value in D(z), the support structure
   int*          _list; // holds _all_ the list indices (one list per index value) |_list| = range(t)
   int            _nbv; // number of values in _values
   int     _yMin,_yMax;
   BitDomain::Ptr _zOld,_yOld;
   int findIndex(int target) const;
   void zLostValue(int v);
   void yLostValue(int v);
public:
   Element1DDC(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z);
   void post() override;
   void propagate() override;
   void print(std::ostream& os) const override;
};

class Element1DVar : public Constraint {  // _z = _array[y]
   Factory::Veci  _array;
   var<int>::Ptr   _y,_z;
   std::vector<int> _yValues;
   var<int>::Ptr _supMin,_supMax;
   int _zMin,_zMax;
   void equalityPropagate();
   void filterY();
public:
   template <class Vec> Element1DVar(const Vec& array,var<int>::Ptr y,var<int>::Ptr z)
      : Constraint(y->getSolver()),
        _array(array.size(),Factory::alloci(z->getStore())),
        _y(y),
        _z(z),
        _yValues(_y->size())
   {
      for(auto i = 0u;i < array.size();i++)
         _array[i] = array[i];
   }
   void post() override;
   void propagate() override;
};
