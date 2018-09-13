#ifndef __INTVAR_H
#define __INTVAR_H

#include <iostream>
#include <vector>
#include "avar.hpp"
#include "acstr.hpp"
#include "solver.hpp"
#include "BitDomain.hpp"
#include "revList.hpp"

template<typename T> class var {};

template<>
class var<int> :public AVar { 
   handle_ptr<CPSolver>    _solver;
   BitDomain::Ptr             _dom;
   int                         _id;
   revList<Constraint::Ptr> _onBindList;
   revList<Constraint::Ptr> _onBoundsList;
   revList<Constraint::Ptr> _onDomList;
   struct DomainListener :public IntNotifier {
      var<int>* theVar;
      DomainListener(var<int>* x) : theVar(x) {}
      void empty() override;
      void bind() override;
      void change()  override;
      void changeMin() override;
      void changeMax() override;      
   };
   DomainListener*       _domListener;
protected:
   void setId(int id) override { _id = id;}
public:
   typedef handle_ptr<var<int>> Ptr;
   var<int>(CPSolver::Ptr& cps,int min,int max);
   var<int>(CPSolver::Ptr& cps,int n) : var<int>(cps,0,n-1) {}
   ~var<int>();
   auto& getSolver()  { return _solver;}
   int min() const { return _dom->min();}
   int max() const { return _dom->max();}
   int size() const { return _dom->size();}
   bool isBound() const { return _dom->isBound();}
   bool contains(int v) const { return _dom->member(v);}

   void assign(int v);
   void remove(int v);
   void removeBelow(int newMin);
   void removeAbove(int newMax);
   void updateBounds(int newMin,int newMax);
   
   revList<Constraint::Ptr>::revNode* whenBind(std::function<void(void)>&& f);
   revList<Constraint::Ptr>::revNode* whenBoundsChange(std::function<void(void)>&& f);
   revList<Constraint::Ptr>::revNode* whenDomainChange(std::function<void(void)>&& f);
   auto propagateOnBind(Constraint::Ptr c)          { return _onBindList.emplace_back(std::move(c));}
   auto propagateOnBoundChange(Constraint::Ptr c)   { return _onBoundsList.emplace_back(std::move(c));}
   auto propagateOnDomainChange(Constraint::Ptr c ) { return _onDomList.emplace_back(std::move(c));}
    
   friend std::ostream& operator<<(std::ostream& os,const var<int>& x) {
      if (x.size() == 1)
         os << x.min();
      else
         os << "x_" << x._id << '(' << *x._dom << ')';
      //os << "\n\tonBIND  :" << x._onBindList << std::endl;
      //os << "\tonBOUNDS:" << x._onBoundsList << std::endl;
      return os;
   }
};

inline std::ostream& operator<<(std::ostream& os,const var<int>::Ptr& xp) {
   return os << *xp;
}

template <class T,class A> inline std::ostream& operator<<(std::ostream& os,const std::vector<T,A>& v) {
   os << '[';
   for(auto& e : v)
      os << e << ',';
   return os << '\b' << ']';
}

namespace Factory {
   using alloc = stl::StackAdapter<var<int>::Ptr,Storage>;
   var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max);
   std::vector<var<int>::Ptr,alloc> intVarArray(CPSolver::Ptr cps,int sz,int min,int max);
   std::vector<var<int>::Ptr,alloc> intVarArray(CPSolver::Ptr cps,int sz);
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

#endif
