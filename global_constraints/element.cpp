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

#include <string.h>
#include <limits>
#include "constraint.hpp"

Element2D::Element2D(const Matrix<int,2>& mat,var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(x->getSolver()),
     _matrix(mat),
     _x(x),_y(y),_z(z),
     _n(mat.size(0)),
     _m(mat.size(1)),
     _low(x->getSolver()->getStateManager(),0),
     _up(x->getSolver()->getStateManager(),_n * _m - 1)
{
   for(int i=0;i < _matrix.size(0);i++)
      for(int j=0;j < _matrix.size(1);j++)
         _xyz.push_back(Triplet(i,j,_matrix[i][j]));
   std::sort(_xyz.begin(),_xyz.end(),[](const Triplet& a,const Triplet& b) {
      return a.z < b.z;
   });
   _nColsSup = new (x->getSolver()) trail<int>[_n];
   _nRowsSup = new (x->getSolver()) trail<int>[_m];
   auto sm = x->getSolver()->getStateManager();
   for(int i=0;i<_n;i++)
      new (_nColsSup + i) trail<int>(sm,_m);
   for(int j=0;j <_m;j++)
      new (_nRowsSup + j) trail<int>(sm,_n);
}

void Element2D::updateSupport(int lostPos)
{
   int nv1 = _nColsSup[_xyz[lostPos].x] = _nColsSup[_xyz[lostPos].x] - 1;
   if (nv1 == 0)
      _x->remove(_xyz[lostPos].x);
   int nv2 = _nRowsSup[_xyz[lostPos].y] = _nRowsSup[_xyz[lostPos].y] - 1;
   if (nv2==0)
      _y->remove(_xyz[lostPos].y);
}


void Element2D::post()
{
   _x->updateBounds(0,_n-1);
   _y->updateBounds(0,_m-1);
   _x->propagateOnDomainChange(this);
   _y->propagateOnDomainChange(this);
   _z->propagateOnBoundChange(this);
   propagate();
}

void Element2D::propagate()
{
   int l =  _low,u = _up;
   int zMin = _z->min(),zMax = _z->max();
   while (_xyz[l].z < zMin || !_x->contains(_xyz[l].x) || !_y->contains(_xyz[l].y)) {
      updateSupport(l++);
      if (l > u) failNow();
   }
   while (_xyz[u].z > zMax || !_x->contains(_xyz[u].x) || !_y->contains(_xyz[u].y)) {
      updateSupport(u--);
      if (l > u) failNow();
   }
   _z->updateBounds(_xyz[l].z,_xyz[u].z);
   _low = l;
   _up  = u;
}

void Element2D::print(std::ostream& os) const
{
   os << "element2D(" << _x << ',' << _y << ',' << _z << ')' << std::endl;
}

Element1DBasic::Element1DBasic(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(y->getSolver()),_t(array),_y(y),_z(z),
     _from(_y->getSolver()->getStateManager(),-1),
     _to(_y->getSolver()->getStateManager(),-1) // z == array[y]
{
   _kv = NULL;
}

void Element1DBasic::post() 
{
   if (_y->isBound()) {
      _z->assign(_t[_y->min()]);
   } else if (_z->isBound()) {
      const int zv = _z->min();
      for(int v = _y->min(); v <= _y->max();v++) { // loop on index
         if (v < 0 || v >= (int)_t.size() || _t[v] != zv) // does not yield val in dom
            _y->remove(v);
      }
   } else { // nobody bound.
      _kv = new (_y->getSolver()) Pair[_t.size()];
      for(int k = 0;k < (int)_t.size();k++) 
         _kv[k] = (Pair){k,_t[k]};
      qsort(_kv,_t.size(),sizeof(Pair),[](const void* a,const void* b) {
         const Pair* ap = reinterpret_cast<const Pair*>(a);
         const Pair* bp = reinterpret_cast<const Pair*>(b);
         int d = ap->_v - bp->_v;
         return d ? d : (ap->_k - bp->_k);
      });
      for(int k=0;k < (int)_t.size();k++) {
         if (!_z->contains(_kv[k]._v))
            _y->remove(_kv[k]._k);
         else {
            if (_from == -1)
               _from = k;
            _to = k;
         }
      }
      if (_y->isBound())
         _z->assign(_t[_y->min()]);
      else {
         _y->propagateOnDomainChange(this);
         _z->propagateOnBoundChange(this);
      }      
   }
}

void Element1DBasic::propagate() 
{
   if (_y->isBound())
      _z->assign(_t[_y->min()]);
   else {
      int k = _from;
      while (k < (int)_t.size() && !_y->contains(_kv[k]._k)) ++k;
      if (k < (int)_t.size()) {
         _z->removeBelow(_kv[k]._v);
         _from = k;
      } else failNow();
      k = _to;
      while (k>=0 && !_y->contains(_kv[k]._k)) --k;
      if (k >= 0) {
         _z->removeAbove(_kv[k]._v);
         _to = k;
      } else failNow();
      k = _from;
      while (k < (int)_t.size() && _kv[k]._v < _z->min())
         _y->remove(_kv[k++]._k);
      _from = k;
      k = _to;
      while (k>=0 && _kv[k]._v > _z->max())
         _y->remove(_kv[k--]._k);
      _to = k;
   }
}

void Element1DBasic::print(std::ostream& os) const 
{
   os << "element1DBasic(" << _t << ',' << _y << ',' << _z << ')' << std::endl;
}

Element1DDC::Element1DDC(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(y->getSolver()),_t(array),_y(y),_z(z)
{}

int Element1DDC::findIndex(int target) const
{
  int l = 0,u = _nbv-1;
  while (l <= u) {
    int m = (l+u)/2;
    if (_values[m]._v == target)
      return m;
    else if (_values[m]._v > target)
      u = m - 1;
    else l = m + 1;
  }
  return _endOfList;
}

struct NoNotify : IntNotifier {
  void empty() {}
  void bind()  {}
  void change() {}
  void changeMin() {}
  void changeMax() {}
};

void Element1DDC::post()
{
  auto cps = _z->getSolver();
  _zOld = new (cps) BitDomain(cps->getStateManager(),cps->getStore(),_z->min(),_z->max());
  _yOld = new (cps) BitDomain(cps->getStateManager(),cps->getStore(),_y->min(),_y->max());
  _y->updateBounds(0,_t.size()-1);
  int yMin = _y->min(),yMax = _y->max();
  _endOfList = _y->min() - 1;
  std::vector<int> sorted;
  for(int k = yMin; k <= yMax;k++) 
    if (_y->contains(k)) 
      sorted.push_back(_t[k]);    
  std::sort(sorted.begin(),sorted.end());
  _nbv = 0;
  _values = new (cps) DCIndex[sorted.size()];
  int previous = sorted[0]-1; // so that the test below is false on first iteration
  for(auto v : sorted) {
    if (v!=previous) {
      _values[_nbv]._v = v;
      _values[_nbv]._k = _endOfList; // end-of-list marker. _list is built after
      _values[_nbv]._s = trail<int>(cps->getStateManager(),1);
      _nbv++;
    } else _values[_nbv-1]._s++; // one more index reaches that same previous value.
    previous = v;
  }
  int tMin = std::max(sorted[0],_z->min());         // use z.min and smallest in sorted
  int tMax = std::min(*(sorted.end()-1),_z->max()); // use z.max and largest in sorted
  _z->updateBounds(tMin,tMax);
  int last = 0;
  // prune _z based on the values that do appear in _values (i.e., in _t)
  for(int zk = tMin;zk <= tMax;zk++) {
    while (_values[last]._v < zk) last++; // skip until we are >= zk
    if (_values[last]._v > zk) // then zk is not in _values, delete it from _z. otherwise do nothing.
      _z->remove(zk);
  }
  // build all the linked lists in _list (header in _values._k set at EOL marker at start)
  _yMax = yMax;
  _yMin = yMin;
  _list = new (cps) int[yMax - yMin + 1];
  for(int yk=yMin;yk <= yMax;yk++) 
    if (_y->contains(yk)) {
      int idx = findIndex(_t[yk]);       // locate the list that carries the value reachable by yk in _t
      //assert(0 <= yk && yk < yMax - yMin + 1);
      _list[yk-_yMin] = _values[idx]._k;       // set the list entry to that list
      _values[idx]._k = yk;             // and insert yk in front of that list (supports of _v)
    }
  // prune _y based on the values NOT in D(_z) that have supports.
  for(int k=0;k < _nbv;k++)
    if (!_z->contains(_values[k]._v)) {
      int link = _values[k]._k;
      while(link != _endOfList) {
        _y->remove(link);
        assert(0 <= link && link < yMax - yMin + 1);
        link = _list[link-_yMin];
      }
    }
  // setup a copy of the domains of z,y to be able to identify which values were lost.
  // This is a quick reuse of BitDomain, which is why a Notifier is needed
  NoNotify nn;
  _zOld->removeBelow(_z->min(),nn);
  _zOld->removeAbove(_z->max(),nn);
  for(int k = _z->min(); k <= _z->max();k++)
    if (!_z->contains(k))
      _zOld->remove(k,nn);

  _yOld->removeBelow(_y->min(),nn);
  _yOld->removeAbove(_y->max(),nn);
  for(int k = _y->min(); k <= _y->max();k++)
    if (!_y->contains(k))
      _yOld->remove(k,nn);
  
  // Hook up listeners
  _y->propagateOnDomainChange(this);
  _z->propagateOnDomainChange(this);
}

void Element1DDC::zLostValue(int v)
{
  int k = findIndex(v);
  int link = _values[k]._k;
  while (link != _endOfList) {
    _y->remove(link);
    //assert(0 <= link && link < _yMax -_yMin + 1);
    link = _list[link-_yMin];
  }
}

void Element1DDC::yLostValue(int v)
{
  int k = findIndex(_t[v]);
  _values[k]._s--;
  if (_values[k]._s == 0)
    _z->remove(_values[k]._v);
}

void Element1DDC::propagate() 
{
  NoNotify nn;
  for(int i=_zOld->min(); i <= _zOld->max();i++) {
    if (_zOld->member(i) && !_z->contains(i)) { // i was lost from D(z) but we didn't know that -> value Lost Event
      zLostValue(i);
      _zOld->remove(i,nn);
    }
  }
  for(int i=_yOld->min();i <= _yOld->max();i++) {
    if (_yOld->member(i) && !_y->contains(i)) {
      yLostValue(i);
      _yOld->remove(i,nn);
    }
  }
}

void Element1DDC::print(std::ostream& os) const 
{
   os << "element1DDC(" << _t << ',' << _y << ',' << _z << ')' << std::endl;
}


Element1D::Element1D(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(y->getSolver()),_t(array),_y(y),_z(z)
{}

void Element1D::post()
{
   Matrix<int,2> t2({1,(int)_t.size()});
   for(auto j=0u;j< _t.size();j++)
      t2[0][j] = _t[j];
   auto x = Factory::makeIntVar(_y->getSolver(),0,0);
   auto c = new (_y->getSolver()) Element2D(t2,x,_y,_z);
   _y->getSolver()->post(c,false);
}


void Element1DVar::post()
{
   _y->updateBounds(0,(int)_array.size() - 1);
   for(auto t : _array) 
       t->propagateOnBoundChange(this);
   _y->propagateOnDomainChange(this);
   _z->propagateOnBoundChange(this);
   propagate();
}

void Element1DVar::propagate()
{
   _zMin = _z->min();
   _zMax = _z->max();
   if (_y->isBound()) equalityPropagate();
   else {
      filterY();
      if (_y->isBound())
         equalityPropagate();
      else
         _z->updateBounds(_supMin->min(),_supMax->max());
   }
}

void Element1DVar::equalityPropagate()
{
   auto tVar = _array[_y->min()];
   tVar->updateBounds(_zMin,_zMax);
   _z->updateBounds(tVar->min(),tVar->max());
}

void Element1DVar::filterY()
{
   int min = INT32_MAX,max = INT32_MIN;
   int i = 0;
   for (int k=_y->min();k <= _y->max();k++)
      if (_y->contains(k))
         _yValues[i++] = k;
   while (i > 0) {
      int id = _yValues[--i];
      auto tVar =  _array[id];
      int tMin = tVar->min(),tMax = tVar->max();
      if (tMax < _zMin || tMin > _zMax)
         _y->remove(id);
      else {
         if (tMin < min) {
            min = tMin;
            _supMin = tVar;
         }
         if (tMax > max) {
            max = tMax;
            _supMax = tVar;
         }
      }
   }
}
