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

#include "constraint.hpp"
#include <string.h>

void printCstr(Constraint::Ptr c) {
   c->print(std::cout);
   std::cout << std::endl;
}


void EQc::post()
{
   _x->assign(_c);
}

void NEQc::post()
{
   _x->remove(_c);
}

void EQBinBC::post()
{
   if (_x->isBound())
      _y->assign(_x->min() - _c);
   else if (_y->isBound())
      _x->assign(_y->min() + _c);
   else {
      _x->updateBounds(_y->min() + _c,_y->max() + _c);
      _y->updateBounds(_x->min() - _c,_x->max() - _c);
      _x->whenBoundsChange([this] {
                              _y->updateBounds(_x->min() - _c,_x->max() - _c);
                           });
      _y->whenBoundsChange([this] {
                              _x->updateBounds(_y->min() + _c,_y->max() + _c);
                           });
   }
}
void NEQBinBC::print(std::ostream& os) const
{
   os << _x << " != " << _y << " + " << _c << std::endl;
}

void NEQBinBC::post()
{
   if (_x->isBound())
      _y->remove(_x->min() - _c);
   else if (_y->isBound())
      _x->remove(_y->min() + _c);
   else {
      hdl[0] = _x->whenBind([this] {
            _y->remove(_x->min() - _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
      hdl[1] = _y->whenBind([this] {
            _x->remove(_y->min() + _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
   }
}

void NEQBinBCLight::print(std::ostream& os) const
{
   os << _x << " !=L " << _y << " + " << _c << std::endl;
}
void NEQBinBCLight::post()
{
    if (_y->isBound())
        _x->remove(_y->min() + _c);
    else if (_x->isBound())
        _y->remove(_x->min() - _c);
    else {
        _x->propagateOnBind(this);
        _y->propagateOnBind(this);
    }
}
void NEQBinBCLight::propagate()
{
    if (_y->isBound())
        _x->remove(_y->min() + _c);
    else
        _y->remove(_x->min() - _c);
    setActive(false);
}

void EQBinDC::post()
{
   if (_x->isBound())
      _y->assign(_x->min() - _c);
   else if (_y->isBound())
      _x->assign(_y->min() + _c);
   else {
      _x->updateBounds(_y->min() + _c,_y->max() + _c);
      _y->updateBounds(_x->min() - _c,_x->max() - _c);
      int lx = _x->min(), ux = _x->max();
      for(int i = lx ; i <= ux; i++)
         if (!_x->contains(i))
            _y->remove(i - _c);
      int ly = _y->min(),uy = _y->max();
      for(int i= ly;i <= uy; i++)
         if (!_y->contains(i))
            _x->remove(i + _c);
      _x->whenBind([this] { _y->assign(_x->min() - _c);});
      _y->whenBind([this] { _x->assign(_y->min() + _c);});
   }
}

void LessOrEqual::post()
{
    _x->propagateOnBoundChange(this);
    _y->propagateOnBoundChange(this);
    propagate();
}
    
void LessOrEqual::propagate()
{
    _x->removeAbove(_y->max());
    _y->removeBelow(_x->min());
    setActive(_x->max() >= _y->min());
}

Minimize::Minimize(var<int>::Ptr& x)
    : _obj(x),_primal(0x7FFFFFFF)
{
    auto todo = std::function<void(void)>([this]() {
                                             _obj->removeAbove(_primal);
                                         });
    _obj->getSolver()->onFixpoint(todo);
}

void Minimize::print(std::ostream& os) const
{
   os << "minimize(" << *_obj << ", primal = " << _primal << ")";
}

void Minimize::tighten()
{
   assert(_obj->isBound());
   _primal = _obj->max() - 1;
   throw Failure;
}

void IsEqual::post() 
{
    propagate();
    if (isActive()) {
        _x->propagateOnDomainChange(this);
        _b->propagateOnBind(this);
    }
}

void IsEqual::propagate() 
{
    if (_b->isTrue()) {
        _x->assign(_c);
        setActive(false);
    } else if (_b->isFalse()) {
        _x->remove(_c);
        setActive(false);
    } else if (!_x->contains(_c)) {
        _b->assign(false);
        setActive(false);
    } else if (_x->isBound()) {
        _b->assign(true);
        setActive(false);
    }
}

void IsLessOrEqual::post()
{
    if (_b->isTrue())
        _x->removeAbove(_c);
    else if (_b->isFalse())
        _x->removeBelow(_c + 1);
    else if (_x->max() <= _c)
        _b->assign(1);
    else if (_x->min() > _c)
        _b->assign(0);
    else {
        _b->whenBind([b=_b,x=_x,c=_c] {
                         if (b->isTrue())
                             x->removeAbove(c);
                         else x->removeBelow(c + 1);
                     });
        _x->whenBoundsChange([b=_b,x=_x,c=_c] {
                                 if (x->max() <= c)
                                     b->assign(1);
                                 else if (x->min() > c)
                                     b->assign(0);
                             });        
    }
}
      
void Sum::post()
{
   for(auto& var : _x)
      var->propagateOnBoundChange(this);
   propagate();
}

void Sum::propagate()
{  
   int nU = _nUnBounds;
   int sumMin = _sumBounds,sumMax = _sumBounds;
   for(int i = nU - 1; i >= 0;i--) {
      int idx = _unBounds[i];
      sumMin += _x[idx]->min();
      sumMax += _x[idx]->max();
      if (_x[idx]->isBound()) {
         _sumBounds = _sumBounds + _x[idx]->min();
         _unBounds[i] = _unBounds[nU - 1];
         _unBounds[nU - 1] = idx;
         nU--;
      }
   }
   _nUnBounds = nU;
   if (0 < sumMin ||  sumMax < 0)
      throw Failure;
   for(int i = nU - 1; i >= 0;i--) {
      int idx = _unBounds[i];
      _x[idx]->removeAbove(-(sumMin - _x[idx]->min()));
      _x[idx]->removeBelow(-(sumMax - _x[idx]->max()));
   }
}

Clause::Clause(const std::vector<var<bool>::Ptr>& x)
    : Constraint(x[0]->getSolver()),
      _wL(x[0]->getSolver()->getStateManager(),0),
      _wR(x[0]->getSolver()->getStateManager(),(int)x.size() - 1)
{
    for(auto xi : x) _x.push_back(xi);
}

void Clause::propagate()
{
    const long n = _x.size();
    int i = _wL;
    while (i < n && _x[i]->isBound()) {
        if (_x[i]->isTrue()) {
            setActive(false);
            return;
        }
        i += 1;
    }
    _wL = i;
    i = _wR;
    while (i>=0 && _x[i]->isBound()) {
        if (_x[i]->isTrue()) {
            setActive(false);
            return;
        }
        i -= 1;
    }
    _wR = i;
    if (_wL > _wR) throw Failure;
    else if (_wL == _wR) {
        _x[_wL]->assign(true);
        setActive(false);
    } else {
        assert(_wL != _wR);
        assert(!_x[_wL]->isBound());
        assert(!_x[_wR]->isBound());
        _x[_wL]->propagateOnBind(this);
        _x[_wR]->propagateOnBind(this);
    }
}

IsClause::IsClause(var<bool>::Ptr b,const std::vector<var<bool>::Ptr>& x)
    : Constraint(x[0]->getSolver()),
      _b(b),
      _unBounds(x.size()),
      _nUnBounds(x[0]->getSolver()->getStateManager(),(int)x.size())
{
    for(auto xi : x) _x.push_back(xi);
    for(int i = 0; i < _x.size();i++)
        _unBounds[i] = i;
    _clause = new (x[0]->getSolver()) Clause(x);
}

void IsClause::post()
{
    _b->propagateOnBind(this);
    for(auto& xi : _x)
        xi->propagateOnBind(this);
}

void IsClause::propagate()
{
    auto cp = _x[0]->getSolver();
    if (_b->isTrue()) {
        setActive(false);
        cp->post(_clause,false);
    } else if (_b->isFalse()) {
        for(auto& xi : _x)
            xi->assign(false);
        setActive(false);
    } else {
        int nU = _nUnBounds;
        for(int i = nU - 1;i >=0;i--) {
            int idx = _unBounds[i];
            auto y = _x[idx];
            if (y->isBound()) {
                if (y->isTrue()) {
                    _b->assign(true);
                    setActive(false);
                    return;
                }
                _unBounds[i] = _unBounds[nU -1];
                _unBounds[nU - 1] = idx;
                nU--;
            }
        }
        if (nU == 0) {
            _b->assign(false);
            setActive(false);
        }
        _nUnBounds = nU;
    }
}

void AllDifferentBinary::post()
{
    CPSolver::Ptr cp = _x[0]->getSolver();
    const long n = _x.size();
    for(int i=0;i < n;i++) 
        for(int j=i+1;j < n;j++)
            cp->post(new (cp) NEQBinBCLight(_x[i],_x[j]));    
}


void AllDifferentAC::post() 
{
    CPSolver::Ptr cp = _x[0]->getSolver();
    _nVar    = (int)_x.size();
    _nVal    = updateRange();
    _mm.setup();
    for(int i=0;i < _nVar;i++)
        _x[i]->propagateOnDomainChange(this);
    _match   = new (cp) int[_nVar];
    _matched = new (cp) bool[_nVal];
    _nNodes  = _nVar + _nVal + 1;
    _rg = Graph(_nNodes);
    propagate();
}

int AllDifferentAC::updateRange()
{
   _minVal = INT32_MAX;
   _maxVal = INT32_MIN;
   for(int i=0;i < _nVar;i++) {
       _minVal = std::min(_minVal,_x[i]->min());
       _maxVal = std::max(_maxVal,_x[i]->max());
   }
   return _maxVal - _minVal + 1;      
}

void AllDifferentAC::updateGraph()
{
   const int sink = _nNodes - 1;
   _rg.clear();
   memset(_matched,0,_nVal);
   for(int i=0;i < _nVar;i++) {
      _rg.addEdge(valNode(_match[i]),i);
      _matched[_match[i] - _minVal] = true;
   }
   for(int i = 0; i < _nVar;i++) 
      for(int v = _x[i]->min();v <= _x[i]->max();v++) 
         if (_x[i]->contains(v) && _match[i] != v) 
            _rg.addEdge(i,valNode(v));
   for(int v = _minVal;v <= _maxVal;v++) {
      if (!_matched[v - _minVal])
         _rg.addEdge(valNode(v),sink);
      else _rg.addEdge(sink,valNode(v));
   }  
}

void AllDifferentAC::propagate()
{
   int size = _mm.compute(_match);
   if (size < _nVar)
      throw Failure;
   updateRange();
   updateGraph();
   int nc = 0;
   int scc[_nNodes];
   _rg.SCC([&scc,&nc](int n,int nd[]) {
               for(int i=0;i < n;i++)
                   scc[nd[i]] = nc;
               ++nc;
           });
   for(int i=0;i < _nVar;i++) 
       for(int v = _minVal; v <= _maxVal;v++) 
           if (_match[i] != v && scc[i] != scc[valNode(v)])
               _x[i]->remove(v);
}

void Circuit::setup(CPSolver::Ptr cp)
{
   _dest = new (cp) trail<int>[_x.size()];
   _orig = new (cp) trail<int>[_x.size()];
   _lengthToDest = new (cp) trail<int>[_x.size()];
   for(int i=0;i<_x.size();i++) {
      new (_dest+i) trail<int>(cp->getStateManager(),i);
      new (_orig+i) trail<int>(cp->getStateManager(),i);
      new (_lengthToDest+i) trail<int>(cp->getStateManager(),0);
   }
}

void Circuit::post()
{
   auto cp = _x[0]->getSolver();
   cp->post(Factory::allDifferent(_x));
   if (_x.size() == 1) {
      _x[0]->assign(0);
      return ;      
   }
   for(int i=0;i < _x.size();i++)
      _x[i]->remove(i);
   for(int i=0;i < _x.size();i++) {
      if (_x[i]->isBound())
         bind(i);
      else 
         _x[i]->whenBind([i,this]() { bind(i);});      
   }
}

void Circuit::bind(int i)
{
   int j = _x[i]->min();
   int origi = _orig[i];
   int destj = _dest[j];
   _dest[origi] = destj;
   _orig[destj] = origi;
   int length = _lengthToDest[origi] + _lengthToDest[j] + 1;
   _lengthToDest[origi] = length;
   if (length < _x.size() - 1)
      _x[destj]->remove(origi);                     
}

Element2D::Element2D(const matrix<int,2>& mat,var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z)
    : Constraint(x->getSolver()),
      _matrix(mat),
      _x(x),_y(y),_z(z),
      _n(mat.size(0)),
      _m(mat.size(1)),
      _low(x->getSolver()->getStateManager(),0),
      _up(x->getSolver()->getStateManager(),_n * _m - 1)
{
    //_xyz.resize(_up + 1);
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
      if (l > u) throw Failure;      
   }
   while (_xyz[u].z > zMax || !_x->contains(_xyz[u].x) || !_y->contains(_xyz[u].y)) {
      updateSupport(u--);
      if (l > u) throw Failure;
   }
   _z->updateBounds(_xyz[l].z,_xyz[u].z);
   _low = l;
   _up  = u;
}

void Element2D::print(std::ostream& os) const
{
   os << "element2D(" << _x << ',' << _y << ',' << _z << ')' << std::endl;
}

Element1D::Element1D(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(y->getSolver()),_t(array),_y(y),_z(z)
{}

void Element1D::post()
{
   matrix<int,2> t2({1,(int)_t.size()});
   for(int j=0;j< _t.size();j++)
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

void TableCT::post()
{
    propagate();
    // for each variable, propagate on change to domain
    for (const auto& vp : _vars)
        vp->propagateOnDomainChange(this);
}

void TableCT::propagate()  // enforceGAC
{
    // std::cout << "all vars:" << std::endl;
    // for (const auto& vp : _vars) {
    //     std::cout << "\t" << vp << std::endl;
    // }
    // calculate Sval
    int varIndex;
    _Sval.clear();
    _Ssup.clear();
    for (const auto& vp : _vars) {
        varIndex = _entries.at(vp->getId2()).getIndex();
        if (vp->size() != _lastSizes[varIndex]) {
            _Sval.push_back(vp);
            _lastSizes[varIndex] = vp->size();  // update lastSizes for vars in Sval
        }
        // calculate Ssup
        if (vp->size() > 1)
            _Ssup.push_back(vp);
    }
    // call updateTable
    updateTable();
    // check size of currTable
    if (_currTable.isEmpty())
        throw Failure;
    // call filterDomains
    filterDomains();
}

void TableCT::filterDomains()
{
    int varIndex, valIndex, wIndex, iMin;
    for (const auto& vp : _Ssup) {
        // std::cout << "currVar: " << vp << std::endl;
        varIndex = _entries.at(vp->getId2()).getIndex();
        iMin = _entries.at(vp->getId2()).getMin();
        for (int val = vp->min(); val < vp->max() + 1; val++) {
            valIndex = val - iMin;
            wIndex = _residues[varIndex][valIndex];
            if (wIndex == -1) {
                vp->remove(val);
                // std::cout << "\tremoved " << val << " from " << vp << std::endl;
            }
            else {
                if ((_currTable[wIndex] & _supports[varIndex][valIndex][wIndex]) == 0) {
                    wIndex = _currTable.intersectIndex(_supports[varIndex][valIndex]);
                    if (wIndex != -1) {
                        _residues[varIndex][valIndex] = wIndex;
                    }
                    else {
                        vp->remove(val);
                        // std::cout << "\tremoved " << val << " from " << vp << std::endl;
                    }
                }
            }
        }
        _lastSizes[varIndex] = vp->size();
    }
    // std::cout << "-------------------------------------" << std::endl;
}

void TableCT::updateTable()
{
    int varIndex, valIndex, iMin;
    for (const auto& vp : _Sval) {
        varIndex = _entries.at(vp->getId2()).getIndex();
        iMin = _entries.at(vp->getId2()).getMin();
        _currTable.clearMask();
        for (int val = vp->min(); val < vp->max() + 1; val++) {
            valIndex = val - iMin;
            if (vp->contains(val))
                _currTable.addToMask(_supports[varIndex][valIndex]);
        }
        _currTable.intersectWithMask();
        if (_currTable.isEmpty())
            break;
    }
}