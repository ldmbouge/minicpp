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

#ifndef __SEARCH_H
#define __SEARCH_H

#include <vector>
#include <initializer_list>
#include <functional>
#include <iostream>
#include <iomanip>
#include <queue>

#include "solver.hpp"
#include "constraint.hpp"
#include "RuntimeMonitor.hpp"
#include "commandList.hpp"
#include <utils.hpp>

class Branches {
   std::vector<std::function<void(void)>> _alts;
public:
   Branches(const Branches& b) : _alts(std::move(b._alts)) {}
   Branches(std::vector<std::function<void(void)>> alts) : _alts(alts) {}
   Branches(std::initializer_list<std::function<void(void)>> alts) { _alts.insert(_alts.begin(),alts.begin(),alts.end());}
   ~Branches() {}
   std::vector<std::function<void(void)>>::iterator begin() { return _alts.begin();}
   std::vector<std::function<void(void)>>::iterator end()   { return _alts.end();}
   size_t size() const { return _alts.size();}
};

/*
class Chooser {
   std::function<Branches(void)> _sel;
public:
   Chooser(std::function<Branches(void)> sel) : _sel(sel) {}
   Branches operator()() { return _sel();}
};
*/

class SearchStatistics {
protected:
   int nodes;
   int solutions;
   int failures;
   int intVariables;
   int boolVariables;
   int propagators;
   unsigned long long propagations;
   RuntimeMonitor::HRClock _startTime;
   RuntimeMonitor::HRClock _initTime;
   RuntimeMonitor::HRClock _solveTime;
   bool completed;

public:
   SearchStatistics() :
      nodes(0),
      solutions(0),
      failures(0),
      intVariables(0),
      boolVariables(0),
      propagators(0),
      propagations(0),
      completed(true)
   {
      _initTime = _startTime = RuntimeMonitor::now();
   }
   void incrFailures()  noexcept {failures += 1; extern int __nbf; __nbf = failures; }
   void incrNodes()     noexcept {nodes += 1; extern int __nbn; __nbn = nodes;}
   void incrSolutions() noexcept {solutions += 1;}
   void setIntVars(int count) noexcept { intVariables = count;}
   void setBoolVars(int count) noexcept { boolVariables = count;}
   void setPropagators(int count) noexcept {propagators = count;}
   void setInitTime() noexcept { _initTime = RuntimeMonitor::now();}
   void setSolveTime() noexcept { _solveTime = RuntimeMonitor::now();}
   void setPropagations(unsigned long long p) noexcept {propagations = p;}
   void setNotCompleted() noexcept { completed = false;}
   bool getCompleted() noexcept {return completed;}
   int getSolutions() const noexcept {return solutions;}
   int numberOfSolutions() const noexcept {return solutions;}
   int numberOfFailures() const noexcept { return failures;}
   int numberOfNodes() const noexcept { return nodes;}
   RuntimeMonitor::HRClock getStartTime() const noexcept {return _startTime;}
   RuntimeMonitor::HRClock startTime() const noexcept {return _startTime;}
   friend std::ostream& operator<<(std::ostream& os,const SearchStatistics& ss) {
      return os
         << std::fixed << std::setprecision(3)
         << "%%%mzn-stat: initTime=" << RuntimeMonitor::elapsedSeconds(ss._startTime, ss._initTime) << std::endl
         << "%%%mzn-stat: solveTime=" << RuntimeMonitor::elapsedSeconds(ss._initTime, ss._solveTime) << std::endl
         << "%%%mzn-stat: solutions=" << ss.solutions << std::endl
         << "%%%mzn-stat: variables=" << ss.intVariables + ss.boolVariables << std::endl
         << "%%%mzn-stat: propagators=" << ss.propagators << std::endl
         << "%%%mzn-stat: propagations=" << ss.propagations << std::endl
         << "%%%mzn-stat: nodes=" << ss.nodes << std::endl
         << "%%%mzn-stat: failures=" << ss.failures << std::endl
         << "%%%mzn-stat-end" << std::endl;          
   }
};

typedef std::function<bool(const SearchStatistics&)> Limit;

class DFSearch {
   StateManager::Ptr                      _sm;
   CPSolver::Ptr                          _cp;
   std::function<Branches(void)>   _branching;
   std::vector<std::function<void(void)>>    _solutionListeners;
   std::vector<std::function<void(void)>>    _failureListeners;
   void dfs(SearchStatistics& stats,const Limit& limit);
public:
   DFSearch(CPSolver::Ptr cp,std::function<Branches(void)>&& b)
      : _sm(cp->getStateManager()),_cp(cp),_branching(std::move(b)) {
      _sm->enable();
   }
   // DFSearch(StateManager::Ptr sm,std::function<Branches(void)>&& b)
   //    : _sm(sm),_branching(std::move(b)) {
   //    _sm->enable();
   // }
   template <class B> void onSolution(B c) { _solutionListeners.emplace_back(std::move(c));}
   template <class B> void onFailure(B c)  { _failureListeners.emplace_back(std::move(c));}
   void notifySolution() { for_each(_solutionListeners.begin(),_solutionListeners.end(),[](std::function<void(void)>& c) { c();});}
   void notifyFailure()  { for_each(_failureListeners.begin(),_failureListeners.end(),[](std::function<void(void)>& c) { c();});}
   SearchStatistics solve(SearchStatistics& stat,Limit limit);
   SearchStatistics solve(Limit limit);
   SearchStatistics solve();
   SearchStatistics solveSubjectTo(Limit limit,std::function<void(void)> subjectTo);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat);
   SearchStatistics optimize(Objective::Ptr obj,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj);
   SearchStatistics optimizeSubjectTo(Objective::Ptr obj,Limit limit,std::function<void(void)> subjectTo);
};

class BFSNode {
public:
   std::shared_ptr<Checkpoint> _checkpoint;
   int _objectiveValue;
   int _depth;
   BFSNode() { }
   BFSNode(std::shared_ptr<Checkpoint> checkpoint, int objectiveValue, int depth)
      : _checkpoint(checkpoint), _objectiveValue(objectiveValue), _depth(depth) { }
};

struct BFSNodeCompare
{
   bool _minimize;
   BFSNodeCompare(bool minimize = 0) : _minimize(minimize) {}
   bool operator()(const BFSNode& lhs, const BFSNode& rhs) const {
      return (_minimize ? lhs._objectiveValue > rhs._objectiveValue : lhs._objectiveValue < rhs._objectiveValue) || (lhs._objectiveValue == rhs._objectiveValue && lhs._depth < rhs._depth);
   }
};

class BFSearch {
   Trailer::Ptr                      _sm;
   CPSemSolver::Ptr                          _cp;
   std::function<Branches(void)>   _branching;
   std::vector<std::function<void(void)>>    _solutionListeners;
   std::vector<std::function<void(void)>>    _failureListeners;
   Objective::Ptr _objective;
   void bfs(SearchStatistics& stats,const Limit& limit);
   std::shared_ptr<Checkpoint> _before;
public:
   BFSearch(CPSemSolver::Ptr cp,std::function<Branches(void)>&& b)
      : _sm(cp->getStateManager()),_cp(cp),_branching(std::move(b)) {
      _sm->enable();
   }
   SearchStatistics solve(SearchStatistics& stat,Limit limit);
   SearchStatistics solve(Limit limit);
   SearchStatistics solve();
   template <class B> void onSolution(B c) { _solutionListeners.emplace_back(std::move(c));}
   template <class B> void onFailure(B c)  { _failureListeners.emplace_back(std::move(c));}
   void notifySolution() { for_each(_solutionListeners.begin(),_solutionListeners.end(),[](std::function<void(void)>& c) { c();});}
   void notifyFailure()  { for_each(_failureListeners.begin(),_failureListeners.end(),[](std::function<void(void)>& c) { c();});}
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat);
   SearchStatistics optimize(Objective::Ptr obj,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj);
};

template<class B>
std::function<Branches(void)> land(std::initializer_list<B> allB) {
   std::vector<B> vec(allB);
   return land(vec);
}

template<class B>
std::function<Branches(void)> land(std::vector<B> vec)
{
   return [vec]()
   {
      for(size_t i = 0; i < vec.size(); i += 1)
         {
            auto br = vec[i]();
            if (br.size() != 0)
               return br;
         }
      return Branches({});
   };
}

inline Branches operator|(std::function<void(void)> b0, std::function<void(void)> b1) {
   return Branches({ b0,b1 });
}


template<class Container,typename Predicate, typename Fun>
typename Container::value_type selectMin(const Container& c,Predicate test, Fun f,
                                         typename Container::value_type def = typename Container::value_type())
{
   auto from = c.begin();
   auto to = c.end();
   auto min = to;
   for(; from != to; from++)
      {
         if (test(*from))
            {
               auto fv = f(*from);
               if (min == to || fv < f(*min))
                  min = from;
            }
      }
   if (min == to)
      return def;
   else 
      return *min;
}

template<class Container,typename Predicate, typename Fun>
typename Container::value_type selectMin3(const Container& c,Predicate test, Fun f) {
   return selectMin(c,test,f,typename Container::value_type());
}

template<class Container,typename Predicate>
typename Container::value_type selectFirst(const Container& c,Predicate test)
{
   auto from = c.begin();
   auto to = c.end();
   for(; from != to; from++) {
      if (test(*from))
         return *from;
   }
   return typename Container::value_type();
}


template <class Container> std::function<Branches(void)> firstFail(CPSolver::Ptr cp,Container& c) {
   using namespace Factory;
   return [=]() {
      auto sx = selectMin(c,
                          [](const auto& x) { return x->size() > 1;},
                          [](const auto& x) { return x->size();});
      if (sx) {
         int v = sx->min();
         return [cp,sx,v] { return cp->post(sx == v);}
            |  [cp,sx,v] { return cp->post(sx != v);};
      } else return Branches({});
   };
}

template<class Container,typename Predicate, typename Fun>
typename Container::value_type selectMax(const Container& c,Predicate test, Fun f,
                                         typename Container::value_type def = typename Container::value_type())
{
   auto from = c.begin();
   auto to = c.end();
   auto max = to;
   for(; from != to; from++) {
      if (test(*from)) {
         auto fv = f(*from);
         if (max == to || fv > f(*max))
            max = from;
      }
   }
   if (max == to)
      return def;
   else
      return *max;
}


template<class Vars, class Var>
Var first_fail(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->size();}
                    );
}

template<class Vars, class Var>
Var input_order(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [&](const auto& x)
                    {
                       auto it = std::find(vars.begin(), vars.end(), x);
                       return std::distance(vars.begin(), it);
                    }
                    );
}

template<class Vars, class Var>
Var smallest(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->min();}
                    );
}

template<class Vars, class Var>
Var largest(Vars const & vars) {
   return selectMax(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->max();}
                    );
}


// Values selections
template<class Var> Branches indomain_min(CPSolver::Ptr cp, Var var) {
   using namespace Factory;
   if (var) {
      auto val = var->min();
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " == "<< val << std::endl;);
         return cp->post(var == val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " != "<< val << std::endl;);
            return cp->post(var != val);
         };
   } else
      return Branches({});
}

template <class Var> Branches indomain_max(CPSolver::Ptr cp, Var var) {
   using namespace Factory;

   if (var) {
      auto val = var->max();
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " == "<< val << std::endl;);
         return cp->post(var == val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " != "<< val << std::endl;);
            return cp->post(var != val);
         };
   } else
      return Branches({});
}

template <class Var> Branches indomain_split(CPSolver::Ptr cp, Var var) {
   using namespace Factory;

   if (var) {
      auto val = (var->max() + var->min()) / 2;
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " <= "<< val << std::endl;);
         return cp->post(var <= val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " > "<< val << std::endl;);
            return cp->post(var >= val + 1);
         };
   } else
      return Branches({});
}


#endif
