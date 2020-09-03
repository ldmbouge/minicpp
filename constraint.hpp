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

#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include <set>
#include <array>
#include <algorithm>
#include <iomanip>
#include <stdint.h>
#include <map>
#include <vector>
#include <utility>
#include "matrix.hpp"
#include "intvar.hpp"
#include "acstr.hpp"
#include "matching.hpp"
#include "bitset.hpp"
#include "literal.hpp"
#include "explainer.hpp"

class Visitor;

class EQc : public Constraint { // x == c
   var<int>::Ptr _x;
   int           _c;
   EQcExplainer _ex;
public:
   EQc(var<int>::Ptr x,int c) : Constraint(x->getSolver()),_x(x),_c(c), _ex(x->getSolver()->getExpSolver(), this) {}
   friend class EQcExplainer;
   void post() override;
   std::vector<Literal*> explain(Literal* lp) override { return _ex.explain(lp);} 
};

class NEQc : public Constraint { // x != c
   var<int>::Ptr _x;
   int           _c;
public:
   NEQc(var<int>::Ptr x,int c) : Constraint(x->getSolver()),_x(x),_c(c) {}
   void post() override;
};

class EQBinBC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c)
      : Constraint(x->getSolver()),_x(x),_y(y),_c(c) {}
   void post() override;
};


class EQTernBC : public Constraint { // x == y + z
  var<int>::Ptr _x,_y,_z;
public:
   EQTernBC(var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z)
       : Constraint(x->getSolver()),_x(x),_y(y),_z(z) {}
   void post() override;
};

class EQTernBCbool : public Constraint { // x == y + b
  var<int>::Ptr _x,_y;
  var<bool>::Ptr _b;
public:
  EQTernBCbool(var<int>::Ptr x,var<int>::Ptr y,var<bool>::Ptr b)
    : Constraint(x->getSolver()),_x(x),_y(y),_b(b) {}
  void post() override;
};

class NEQBinBC : public Constraint { // x != y + c
   var<int>::Ptr _x,_y;
   int _c;
   trailList<Constraint::Ptr>::revNode* hdl[2];
   void print(std::ostream& os) const override;
public:
   NEQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
};

class NEQBinBCLight : public Constraint { // x != y + c
   var<int>::Ptr _x,_y;
   int _c;
   void print(std::ostream& os) const override;
public:
   NEQBinBCLight(var<int>::Ptr& x,var<int>::Ptr& y,int c=0)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
   void propagate() override;
};

class EQBinDC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinDC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
};

class LessOrEqual :public Constraint { // x <= y
   var<int>::Ptr _x,_y;
public:
   LessOrEqual(var<int>::Ptr x,var<int>::Ptr y)
      : Constraint(x->getSolver()),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};

class NEQBool : public Constraint {
   var<bool>::Ptr _b1, _b2;
public:
   NEQBool(var<bool>::Ptr b1, var<bool>::Ptr b2)
     : Constraint(b1->getSolver()), _b1(b1), _b2(b2) {}
   void post() override;
   void propagate() override;
};

class IsEqual : public Constraint { // b <=> x == c
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   int _c;
public:
   IsEqual(var<bool>::Ptr b,var<int>::Ptr x,int c)
      : Constraint(x->getSolver()),_b(b),_x(x),_c(c) {}
   void post() override;
   void propagate() override;
};

class IsMember : public Constraint { // b <=> x in S
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   std::set<int> _S;
public:
   IsMember(var<bool>::Ptr b,var<int>::Ptr x,std::set<int> S)
      : Constraint(x->getSolver()),_b(b),_x(x),_S(S) {}
   void post() override;
   void propagate() override;
};

class IsLessOrEqual : public Constraint { // b <=> x <= c
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   int _c;
public:
   IsLessOrEqual(var<bool>::Ptr b,var<int>::Ptr x,int c)
      : Constraint(x->getSolver()),_b(b),_x(x),_c(c) {}
   void post() override;
};

class Sum : public Constraint { // s = Sum({x0,...,xk})
   Factory::Veci _x;
   //std::vector<var<int>::Ptr> _x;
   trail<int>    _nUnBounds;
   trail<int>    _sumBounds;
   unsigned int _n;
   std::vector<unsigned long> _unBounds;
public:
   template <class Vec> Sum(const Vec& x,var<int>::Ptr s)
      : Constraint(s->getSolver()),
        //_x(x.size() + 1),
        _x(x.size() + 1,Factory::alloci(s->getStore())), 
        _nUnBounds(s->getSolver()->getStateManager(),(int)x.size()+1),
        _sumBounds(s->getSolver()->getStateManager(),0),
        _n((int)x.size() + 1),
        _unBounds(_n)
   {
     for(typename Vec::size_type i=0;i < x.size();i++)
         _x[i] = x[i];
      _x[_n-1] = Factory::minus(s);
      for(typename Vec::size_type i=0; i < _n;i++)
         _unBounds[i] = i;
   }        
   void post() override;
   void propagate() override;
};

class Clause : public Constraint { // x0 OR x1 .... OR xn
   std::vector<var<bool>::Ptr> _x;
   trail<int> _wL,_wR;
public:
   Clause(const std::vector<var<bool>::Ptr>& x);
   void post() override { propagate();}
   void propagate() override;
   void visit(Visitor& v) override { v.visitClause(this);}
};

class LitClause : public Constraint { // x0 OR x1 .... OR xn
    std::vector<LitVar::Ptr> _x;
    trail<int> _wL,_wR;
public:
    LitClause(const std::vector<LitVar::Ptr>& x);
    void post() override { propagate();}
    void propagate() override;
};

class IsTrue : public Constraint {
    LitVar::Ptr _x;
public:
    IsTrue(const LitVar::Ptr& x);
    void post() override;
};

class IsFalse : public Constraint {
    LitVar::Ptr _x;
public:
    IsFalse(const LitVar::Ptr& x);
    void post() override;
};

class IsClause : public Constraint { // b <=> x0 OR .... OR xn
   var<bool>::Ptr _b;
   std::vector<var<bool>::Ptr> _x;
   std::vector<int> _unBounds;
   trail<int>      _nUnBounds;
   Clause::Ptr        _clause;
public:
   IsClause(var<bool>::Ptr b,const std::vector<var<bool>::Ptr>& x);
   void post() override;
   void propagate() override;
};

class AllDifferentBinary :public Constraint {
   Factory::Veci _x;
public:
   template <class Vec> AllDifferentBinary(const Vec& x)
      : Constraint(x[0]->getSolver()),
        _x(x.size(),Factory::alloci(x[0]->getStore()))
   {
     for(typename Vec::size_type i=0;i < x.size();i++)
         _x[i] = x[i];
   }
   void post() override;
};

class AllDifferentAC : public Constraint {
    Factory::Veci    _x;
    MaximumMatching _mm;
    Graph           _rg;
    AllDiffExplainer _ex;
    int* _match;
    bool* _matched;
    int _minVal,_maxVal;
    int _nVar,_nVal,_nNodes;
    int updateRange();
    void updateGraph();
    int valNode(int vid) const { return vid - _minVal + _nVar;}
    friend class AllDiffExplainer;
public:
    template <class Vec> AllDifferentAC(const Vec& x)
        : Constraint(x[0]->getSolver()),
          _x(x.begin(),x.end(),Factory::alloci(x[0]->getStore())),
          _mm(_x,x[0]->getStore()), 
          _ex(x[0]->getSolver()->getExpSolver(),this) {}
    ~AllDifferentAC() {}
    void post() override;
    void propagate() override;
    void visit(Visitor& v) override { v.visitAllDifferentAC(this);}
    std::vector<Literal*> explain(Literal* lp) override { return _ex.explain(lp);} 
};

class Circuit :public Constraint {
   Factory::Veci  _x;
   trail<int>* _dest;
   trail<int>* _orig;
   trail<int>* _lengthToDest;
   void bind(int i);
   void setup(CPSolver::Ptr cp);
public:
   template <class Vec> Circuit(const Vec& x)
      : Constraint(x[0]->getSolver()),
        _x(x.size(),Factory::alloci(x[0]->getStore()))
   {
      auto cp = x[0]->getSolver();
      for(auto i=0u;i < x.size();i++)
         _x[i] = x[i];
      setup(cp);
   }
   void post() override;
};

class Minimize : public Objective {
   var<int>::Ptr _obj;
   int        _primal;
   void print(std::ostream& os) const;
public:
   Minimize(var<int>::Ptr& x);
   void tighten() override;
   int value() const override { return _obj->min();}
};

class Maximize : public Objective {
   var<int>::Ptr _obj;
   int        _primal;
   void print(std::ostream& os) const;
public:
   Maximize(var<int>::Ptr& x);
   void tighten() override;
   int value() const override { return _obj->max();}
};

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

namespace Factory {
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
      return new (x->getSolver()) EQBinBC(x,y,c);
   }
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z) {
      return new (x->getSolver()) EQTernBC(x,y,z);
   }
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,var<bool>::Ptr b) {
     return new (x->getSolver()) EQTernBCbool(x,y,b);
   }
   inline Constraint::Ptr notEqual(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
      return new (x->getSolver()) NEQBinBC(x,y,c);
   }
   inline Constraint::Ptr operator==(var<int>::Ptr x,int c) {
      auto cp = x->getSolver();
      x->assign(c);
      cp->fixpoint();
      return nullptr;
      //return new (x->getSolver()) EQc(x,c);
   }
   inline Constraint::Ptr operator!=(var<int>::Ptr x,int c) {
      auto cp = x->getSolver();
      x->remove(c);
      cp->fixpoint();
      return nullptr;
      //return new (x->getSolver()) NEQc(x,c);
   }
   inline Constraint::Ptr inside(var<int>::Ptr x,std::set<int> S) {
      auto cp = x->getSolver();
      for(int v = x->min();v <= x->max();++v) {
         if (!x->contains(v)) continue;
         if (S.find(v) == S.end())
            x->remove(v);
      }
      cp->fixpoint();
      return nullptr;
   }
   inline Constraint::Ptr outside(var<int>::Ptr x,std::set<int> S) {
      auto cp = x->getSolver();
      for(int v : S) {
         if (x->contains(v))
            x->remove(v);        
      }
      cp->fixpoint();
      return nullptr;
   }
   
   inline Constraint::Ptr operator==(var<bool>::Ptr x,bool c) {
      return new (x->getSolver()) EQc((var<int>::Ptr)x,c);
   }
   inline Constraint::Ptr operator==(var<bool>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) EQBinBC(x,y,0);
   }
   inline Constraint::Ptr operator!=(var<bool>::Ptr x,bool c) {
      return new (x->getSolver()) NEQc((var<int>::Ptr)x,c);
   }
   inline Constraint::Ptr operator!=(var<int>::Ptr x,var<int>::Ptr y) {
      return Factory::notEqual(x,y,0);
   }
   inline Constraint::Ptr operator!=(var<bool>::Ptr b1, var<bool>::Ptr b2) {
      return new (b1->getSolver()) NEQBool(b1,b2);
   }
   inline Constraint::Ptr operator<=(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(x,y);
   }
   inline Constraint::Ptr operator>=(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(y,x);
   }
   inline Constraint::Ptr operator<=(var<int>::Ptr x,const int c) {
      x->removeAbove(c);
      x->getSolver()->fixpoint();
      return nullptr;
   }
   inline Constraint::Ptr operator>=(var<int>::Ptr x,const int c) {
      x->removeBelow(c);
      x->getSolver()->fixpoint();
      return nullptr;
   }
   inline Objective::Ptr minimize(var<int>::Ptr x) {
      return new Minimize(x);
   }
   inline Objective::Ptr maximize(var<int>::Ptr x) {
      return new Maximize(x);
   }
   inline var<int>::Ptr operator+(var<int>::Ptr x,var<int>::Ptr y) { // x + y
      int min = x->min() + y->min();
      int max = x->max() + y->max();
      var<int>::Ptr z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(equal(z,x,y));
      return z;
   }
   inline var<int>::Ptr operator-(var<int>::Ptr x,var<int>::Ptr y) { // x - y
      int min = x->min() - y->max();
      int max = x->max() - y->max();
      var<int>::Ptr z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(equal(x,z,y));
      return z;
   }
   inline var<bool>::Ptr isEqual(var<int>::Ptr x,const int c) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      try {
         x->getSolver()->post(new (x->getSolver()) IsEqual(b,x,c));
      } catch(Status s) {}
      return b;
   }
   inline Constraint::Ptr isMember(var<bool>::Ptr b, var<int>::Ptr x, const std::set<int> S) {
     return new (x->getSolver()) IsMember(b,x,S);
   }
   inline var<bool>::Ptr isMember(var<int>::Ptr x,const std::set<int> S) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      try {
         x->getSolver()->post(new (x->getSolver()) IsMember(b,x,S));
      } catch(Status s) {}
      return b;
   }
   inline var<bool>::Ptr isLessOrEqual(var<int>::Ptr x,const int c) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      try {
         x->getSolver()->post(new (x->getSolver()) IsLessOrEqual(b,x,c));
      } catch(Status s) {}
      return b;
   }
   inline var<bool>::Ptr isLess(var<int>::Ptr x,const int c) {
      return isLessOrEqual(x,c - 1);
   }
   inline var<bool>::Ptr isLargerOrEqual(var<int>::Ptr x,const int c) {
      return isLessOrEqual(- x,- c);        
   }
   inline var<bool>::Ptr isLarger(var<int>::Ptr x,const int c) {
      return isLargerOrEqual(x , c + 1);
   }
   template <class Vec> var<int>::Ptr sum(Vec& xs) {
      int sumMin = 0,sumMax = 0;
      for(const auto& x : xs) {
         sumMin += x->min();
         sumMax += x->max();
      }
      auto cp = xs[0]->getSolver();
      auto s = Factory::makeIntVar(cp,sumMin,sumMax);
      cp->post(new (cp) Sum(xs,s));
      return s;        
   }
   template <class Vec> Constraint::Ptr sum(const Vec& xs,var<int>::Ptr s) {
      return new (xs[0]->getSolver()) Sum(xs,s);
   }
   template <class Vec> Constraint::Ptr sum(const Vec& xs,int s) {
      auto sv = Factory::makeIntVar(xs[0]->getSolver(),s,s);
      return new (xs[0]->getSolver()) Sum(xs,sv);
   }
   template <class Vec> Constraint::Ptr clause(const Vec& xs) {
      return new (xs[0]->getSolver()) Clause(xs);
   }
   template <class Vec> Constraint::Ptr isClause(var<bool>::Ptr b,const Vec& xs) {
      return new (b->getSolver()) IsClause(b,xs);
   }
   inline var<bool>::Ptr implies(var<bool>::Ptr a,var<bool>::Ptr b) { // a=>b is not(a) or b is (1-a)+b >= 1
      std::vector<var<int>::Ptr> left = {1- (var<int>::Ptr)a,b};
      return isLargerOrEqual(sum(left),1);
   }
   template <class Vec> Constraint::Ptr allDifferent(const Vec& xs) {
      return new (xs[0]->getSolver()) AllDifferentBinary(xs);
   }
   template <class Vec> Constraint::Ptr allDifferentAC(const Vec& xs) {
      return new (xs[0]->getSolver()) AllDifferentAC(xs);
   }
   template <class Vec>  Constraint::Ptr circuit(const Vec& xs) {
      return new (xs[0]->getSolver()) Circuit(xs);
   }
   inline var<int>::Ptr element(Matrix<int,2>& d,var<int>::Ptr x,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      for(int i=0;i<d.size(0);i++)
         for(int j=0;j < d.size(1);j++) {
            min = min < d[i][j] ? min : d[i][j];
            max = max > d[i][j] ? max : d[i][j];
         }
      auto z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(new (x->getSolver()) Element2D(d,x,y,z));
      return z;
   }
   inline Constraint::Ptr element(const VMSlice<int,2,1>& array,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<int> flat(array.size());
      for(int i=0;i < array.size();i++) 
         flat[i] = array[i];
      return new (y->getSolver()) Element1D(flat,y,z);
   }
   template <class Vec> inline var<int>::Ptr element(const Vec& array,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      std::vector<int> flat(array.size());
      for(auto i=0u;i < array.size();i++) {
         const int v = flat[i] = array[i];
         min = min < v ? min : v;
         max = max > v ? max : v;
      }
      auto z = makeIntVar(y->getSolver(),min,max);
      y->getSolver()->post(new (y->getSolver()) Element1D(flat,y,z));
      return z;
   }
   template <class Vec> Constraint::Ptr elementVar(const Vec& xs,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<var<int>::Ptr> flat(xs.size());
      for(int i=0;i<xs.size();i++)
         flat[i] = xs[i];
      return new (y->getSolver()) Element1DVar(flat,y,z);
   }
   template <class Vec> var<int>::Ptr elementVar(const Vec& xs,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      std::vector<var<int>::Ptr> flat(xs.size());
      for(auto i=0u;i < xs.size();i++) {
         const auto& v = flat[i] = xs[i];
         min = min < v->min() ? min : v->min();
         max = max > v->max() ? max : v->max();
      }
      auto z = makeIntVar(y->getSolver(),min,max);
      y->getSolver()->post(new (y->getSolver()) Element1DVar(flat,y,z));
      return z;
   }
};

void printCstr(Constraint::Ptr c); 

#endif
