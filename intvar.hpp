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

#ifndef __INTVAR_H
#define __INTVAR_H

#include <iostream>
#include <vector>
#include <assert.h>
#include "avar.hpp"
#include "acstr.hpp"
#include "solver.hpp"
#include "BitDomain.hpp"
#include "trailList.hpp"
#include "matrix.hpp"

template<typename T> class var {};

template<> class var<int> : public AVar {
public:
   typedef handle_ptr<var<int>> Ptr;
   virtual CPSolver::Ptr getSolver() = 0;
   virtual int min() const  = 0;
   virtual int max() const  = 0;
   virtual int size() const = 0;
   virtual bool isBound() const = 0;
   virtual bool contains(int v) const = 0;
   
   virtual void assign(int v) = 0;
   virtual void remove(int v) = 0;
   virtual void removeBelow(int newMin) = 0;
   virtual void removeAbove(int newMax) = 0;
   virtual void updateBounds(int newMin,int newMax) = 0;
   
   virtual trailList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f) = 0;
   virtual trailList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f) = 0;
   virtual trailList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f) = 0;
   virtual trailList<Constraint::Ptr>::revNode* propagateOnBind(Constraint::Ptr c)          = 0;
   virtual trailList<Constraint::Ptr>::revNode* propagateOnBoundChange(Constraint::Ptr c)   = 0;
   virtual trailList<Constraint::Ptr>::revNode* propagateOnDomainChange(Constraint::Ptr c ) = 0;
   virtual std::ostream& print(std::ostream& os) const = 0;
   friend std::ostream& operator<<(std::ostream& os,const var<int>& x) { return x.print(os);}
};

class IntVarImpl :public var<int> { 
   CPSolver::Ptr           _solver;
   BitDomain::Ptr             _dom;
   int                         _id;
   trailList<Constraint::Ptr> _onBindList;
   trailList<Constraint::Ptr> _onBoundsList;
   trailList<Constraint::Ptr> _onDomList;
   struct DomainListener :public IntNotifier {
      IntVarImpl* theVar;
      DomainListener(IntVarImpl* x) : theVar(x) {}
      void empty() override;
      void bind() override;
      void change()  override;
      void changeMin() override;
      void changeMax() override;      
   };
   DomainListener*       _domListener;
protected:
    void setId(int id) override { _id = id;}
    int getId() const { return _id;}
public:
    IntVarImpl(CPSolver::Ptr& cps,int min,int max);
    IntVarImpl(CPSolver::Ptr& cps,int n) : IntVarImpl(cps,0,n-1) {}
   ~IntVarImpl();
   CPSolver::Ptr getSolver() override { return _solver;}
   int min() const override { return _dom->min();}
   int max() const override { return _dom->max();}
   int size() const override { return _dom->size();}
   bool isBound() const override { return _dom->isBound();}
   bool contains(int v) const override { return _dom->member(v);}

   void assign(int v) override;
   void remove(int v) override;
   void removeBelow(int newMin) override;
   void removeAbove(int newMax) override;
   void updateBounds(int newMin,int newMax) override;
   
   trailList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f) override;
   trailList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f) override;
   trailList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f) override;
   trailList<Constraint::Ptr>::revNode* propagateOnBind(Constraint::Ptr c)  override         { return _onBindList.emplace_back(std::move(c));}
   trailList<Constraint::Ptr>::revNode* propagateOnBoundChange(Constraint::Ptr c)  override  { return _onBoundsList.emplace_back(std::move(c));}
   trailList<Constraint::Ptr>::revNode* propagateOnDomainChange(Constraint::Ptr c ) override { return _onDomList.emplace_back(std::move(c));}

    std::ostream& print(std::ostream& os) const override {
        if (size() == 1)
            os << min();
        else
            os << "x_" << getId() << '(' << *_dom << ')';
        //os << "\n\tonBIND  :" << x._onBindList << std::endl;
        //os << "\tonBOUNDS:" << x._onBoundsList << std::endl;
        return os;
    }
};

class IntVarViewOpposite :public var<int> {
   int _id;
   var<int>::Ptr _x;
protected:
   void setId(int id) override { _id = id;}
   int getId() const { return _id;}
public:
   IntVarViewOpposite(const var<int>::Ptr& x) : _x(x) {}
   CPSolver::Ptr getSolver() override { return _x->getSolver();}
   int min() const  override { return - _x->max();}
   int max() const  override { return - _x->min();}
   int size() const override { return _x->size();}
   bool isBound() const override { return _x->isBound();}
   bool contains(int v) const override { return _x->contains(-v);}
   
   void assign(int v) override { _x->assign(-v);}
   void remove(int v) override { _x->remove(-v);}
   void removeBelow(int newMin) override { _x->removeAbove(-newMin);}
   void removeAbove(int newMax) override { _x->removeBelow(-newMax);}
   void updateBounds(int newMin,int newMax) override { _x->updateBounds(-newMax,-newMin);}
   
   trailList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f) override { return _x->whenBind(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f) override { return _x->whenBoundsChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f) override { return _x->whenDomainChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* propagateOnBind(Constraint::Ptr c)          override { return _x->propagateOnBind(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnBoundChange(Constraint::Ptr c)   override { return _x->propagateOnBoundChange(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnDomainChange(Constraint::Ptr c ) override { return _x->propagateOnDomainChange(c);}
   std::ostream& print(std::ostream& os) const override {
      os << '{';
      for(int i = min();i <= max() - 1;i++) 
         if (contains(i)) os << i << ',';
      if (size()>0) os << max();
      return os << '}';      
   }
};

static inline int floorDiv(int a,int b) {
   const int q = a/b;
   return (a < 0 && q * b != a) ? q - 1 : q;
}
static inline int ceilDiv(int a,int b) {
   const int q = a / b;
   return (a > 0 && q * b != a) ? q + 1 : q;
}

class IntVarViewMul :public var<int> {
   int _id;
   int _a;
   var<int>::Ptr _x;
protected:
   void setId(int id) override { _id = id;}
   int getId() const { return _id;}
public:
   IntVarViewMul(const var<int>::Ptr& x,int a) : _x(x),_a(a) { assert(a> 0);}
   CPSolver::Ptr getSolver() override { return _x->getSolver();}
   int min() const  override { return _a * _x->min();}
   int max() const  override { return _a * _x->max();}
   int size() const override { return _x->size();}
   bool isBound() const override { return _x->isBound();}
   bool contains(int v) const override { return (v % _a != 0) ? false : _x->contains(v / _a);}
   
   void assign(int v) override {
      if (v % _a == 0)
         _x->assign(v / _a);
      else throw Failure;
   }
   void remove(int v) override {
      if (v % _a == 0)
         _x->remove(v / _a);
   }
   void removeBelow(int v) override { _x->removeBelow(ceilDiv(v,_a));}
   void removeAbove(int v) override { _x->removeAbove(floorDiv(v,_a));}
   void updateBounds(int min,int max) override { _x->updateBounds(ceilDiv(min,_a),floorDiv(max,_a));}   
   trailList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f) override { return _x->whenBind(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f) override { return _x->whenBoundsChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f) override { return _x->whenDomainChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* propagateOnBind(Constraint::Ptr c)          override { return _x->propagateOnBind(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnBoundChange(Constraint::Ptr c)   override { return _x->propagateOnBoundChange(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnDomainChange(Constraint::Ptr c ) override { return _x->propagateOnDomainChange(c);}
   std::ostream& print(std::ostream& os) const override {
      os << '{';
      for(int i = min();i <= max() - 1;i++) 
         if (contains(i)) os << i << ',';
      if (size()>0) os << max();
      return os << '}';      
   }
};

class IntVarViewOffset :public var<int> {
   int _id;
   int _o;
   var<int>::Ptr _x;
protected:
   void setId(int id) override { _id = id;}
   int getId() const { return _id;}
public:
    IntVarViewOffset(const var<int>::Ptr& x,int o) : _x(x),_o(o) {}
   CPSolver::Ptr getSolver() override { return _x->getSolver();}
   int min() const  override { return _o + _x->min();}
   int max() const  override { return _o + _x->max();}
   int size() const override { return _x->size();}
   bool isBound() const override { return _x->isBound();}
   bool contains(int v) const override { return _x->contains(v - _o);}
   
   void assign(int v) override { _x->assign(v - _o);}
   void remove(int v) override { _x->remove(v - _o);}
   void removeBelow(int v) override { _x->removeBelow(v - _o);}
   void removeAbove(int v) override { _x->removeAbove(v - _o);}
   void updateBounds(int min,int max) override { _x->updateBounds(min - _o,max - _o);}   
   trailList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f) override { return _x->whenBind(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f) override { return _x->whenBoundsChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f) override { return _x->whenDomainChange(std::move(f));}
   trailList<Constraint::Ptr>::revNode* propagateOnBind(Constraint::Ptr c)          override { return _x->propagateOnBind(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnBoundChange(Constraint::Ptr c)   override { return _x->propagateOnBoundChange(c);}
   trailList<Constraint::Ptr>::revNode* propagateOnDomainChange(Constraint::Ptr c ) override { return _x->propagateOnDomainChange(c);}
   std::ostream& print(std::ostream& os) const override {
      os << '{';
      for(int i = min();i <= max() - 1;i++) 
         if (contains(i)) os << i << ',';
      if (size()>0) os << max();
      return os << '}';      
   }
};


template <>
class var<bool> :public IntVarImpl {
public:
    typedef handle_ptr<var<bool>> Ptr;
    var<bool>(CPSolver::Ptr& cps) : IntVarImpl(cps,0,1) {}
    bool isTrue() const { return min()==1;}
    bool isFalse() const { return max()==0;}
    void assign(bool b)  { IntVarImpl::assign(b);}
};

inline std::ostream& operator<<(std::ostream& os,const var<int>::Ptr& xp) {
    return xp->print(os);
}

template <class T,class A> inline std::ostream& operator<<(std::ostream& os,const std::vector<T,A>& v) {
   os << '[';
   for(auto& e : v)
      os << e << ',';
   return os << '\b' << ']';
}

namespace Factory {
   using alloci = stl::StackAdapter<var<int>::Ptr,Storage>;
   using allocb = stl::StackAdapter<var<bool>::Ptr,Storage>;
   using Vecv   = std::vector<var<int>::Ptr,alloci>;
   using Vecb   = std::vector<var<bool>::Ptr,allocb>;
   var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max);
   var<bool>::Ptr makeBoolVar(CPSolver::Ptr cps);
   inline var<int>::Ptr minus(var<int>::Ptr x)     { return new (x->getSolver()) IntVarViewOpposite(x);}
   inline var<int>::Ptr operator-(var<int>::Ptr x) { return minus(x);}
   inline var<int>::Ptr operator*(var<int>::Ptr x,int a) {
      if (a == 0)
         return makeIntVar(x->getSolver(),0,0);
      else if (a==1)
         return x;
      else if (a <0)
         return minus(new (x->getSolver()) IntVarViewMul(x,-a));
      else return new (x->getSolver()) IntVarViewMul(x,a);
   }
   inline var<int>::Ptr operator*(int a,var<int>::Ptr x) { return x * a;}
   inline var<int>::Ptr operator*(var<bool>::Ptr x,int a)  { return Factory::operator*((var<int>::Ptr)x,a);}
   inline var<int>::Ptr operator*(int a,var<bool>::Ptr x)  { return x * a;}
   inline var<int>::Ptr operator+(var<int>::Ptr x,int a) { return new (x->getSolver()) IntVarViewOffset(x,a);}
   inline var<int>::Ptr operator+(int a,var<int>::Ptr x) { return new (x->getSolver()) IntVarViewOffset(x,a);}
   inline var<int>::Ptr operator-(var<int>::Ptr x,const int a) { return new (x->getSolver()) IntVarViewOffset(x,-a);}
   inline var<int>::Ptr operator-(const int a,var<int>::Ptr x) { return new (x->getSolver()) IntVarViewOffset(-x,a);}
   Vecv intVarArray(CPSolver::Ptr cps,int sz,int min,int max);
   Vecv intVarArray(CPSolver::Ptr cps,int sz,int n);
   Vecv intVarArray(CPSolver::Ptr cps,int sz);
   Vecb boolVarArray(CPSolver::Ptr cps,int sz);
   template<typename Fun> Vecv intVarArray(CPSolver::Ptr cps,int sz,Fun body) {
      auto x = intVarArray(cps,sz);
      for(int i=0;i < x.size();i++)
         x[i] = body(i);
      return x;
   }
};

template<class ForwardIt> ForwardIt min_dom(ForwardIt first, ForwardIt last)
{
   if (first == last) return last;

   int ds = 0x7fffffff;
   ForwardIt smallest = last;
   for (; first != last; ++first) {
      auto fsz = (*first)->size();
      if (fsz > 1 && fsz < ds) {
         smallest = first;
         ds = fsz;
      }
   }
   return smallest;
}

template<class Container> auto min_dom(Container& c) {
   return min_dom(c.begin(),c.end());
}

void printVar(var<int>* x);

#endif
