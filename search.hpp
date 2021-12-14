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

#include "solver.hpp"
#include "constraint.hpp"
#include "RuntimeMonitor.hpp"
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

class SearchStatistics
{
    protected:
        int nodes;
        int solutions;
        int failures;
        int variables;
        int intVariables;
        int boolVariables;
        int propagators;
        int propagations;
        int peakDepth;
        RuntimeMonitor::HRClock startTime;
        RuntimeMonitor::HRClock initTime;
        RuntimeMonitor::HRClock solveTime;
        bool completed;

    public:
       SearchStatistics() :
        nodes(0),
        solutions(0),
        failures(0),
        variables(0),
        intVariables(0),
        boolVariables(0),
        propagators(0),
        propagations(0),
        peakDepth(0),
        completed(false)
       {
          startTime = RuntimeMonitor::now();
       }
       void incrFailures()  noexcept {failures += 1; extern int __nbf; __nbf = failures; }
       void incrNodes()     noexcept {nodes += 1; extern int __nbn; __nbn = nodes;}
       void incrSolutions() noexcept {solutions += 1;}
       void setCompleted()  noexcept {completed = true;}
       void setInitTime() noexcept {initTime = RuntimeMonitor::now();}
       void setSolveTime() noexcept {solveTime = RuntimeMonitor::now();}
       friend std::ostream& operator<<(std::ostream& os,const SearchStatistics& ss)
       {
            return os << "%%%mzn-stat: nodes=" << ss.nodes << std::endl
                      << "%%%mzn-stat: solutions=" << ss.solutions << std::endl
                      << "%%%mzn-stat: failures=" << ss.failures << std::endl
                      << "%%%mzn-stat: variables=" << ss.variables << std::endl
                      << "%%%mzn-stat: intVariables=" << ss.intVariables << std::endl
                      << "%%%mzn-stat: boolVariables=" << ss.boolVariables << std::endl
                      << "%%%mzn-stat: propagators=" << ss.propagators << std::endl
                      << "%%%mzn-stat: propagations=" << ss.propagations << std::endl
                      << "%%%mzn-stat: peakDepth=" << ss.peakDepth << std::endl
                      << std::fixed
                      << "%%%mzn-stat: initTime=" << RuntimeMonitor::elapsedSeconds(ss.startTime, ss.initTime) << std::endl
                      << "%%%mzn-stat: solveTime=" <<  RuntimeMonitor::elapsedSeconds(ss.startTime, ss.solveTime) << std::endl
                      << std::defaultfloat
                      << "%%%mzn-stat-end" << std::endl;
       }
    };

typedef std::function<bool(const SearchStatistics&)> Limit;

class DFSearch {
    StateManager::Ptr                      _sm;
    std::function<Branches(void)>   _branching;
    std::vector<std::function<void(void)>>    _solutionListeners;
    std::vector<std::function<void(void)>>    _failureListeners;
    void dfs(SearchStatistics& stats,const Limit& limit);
public:
   DFSearch(CPSolver::Ptr cp,std::function<Branches(void)>&& b)
      : _sm(cp->getStateManager()),_branching(std::move(b)) {
      _sm->enable();
   }
   DFSearch(StateManager::Ptr sm,std::function<Branches(void)>&& b)
      : _sm(sm),_branching(std::move(b)) {
      _sm->enable();
   }
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

template<class B> std::function<Branches(void)> land(std::initializer_list<B> allB) {
   std::vector<B> vec(allB);
   return [vec]() {
             for(const B& brs : vec) {
                auto br = brs();
                if (br.size() != 0)
                   return br;
             }
             return Branches({});
          };
}

inline Branches operator|(std::function<void(void)> b0, std::function<void(void)> b1) {
    return Branches({ b0,b1 });
}


template<class Container,typename Predicate,typename Fun>
inline typename Container::value_type selectMin(Container& c,Predicate test,Fun f) {
   auto from = c.begin(),to  = c.end();
   auto min = to;
   for(;from != to;++from) {
       if (test(*from)) {
           auto fv = f(*from);
           if (min == to || fv < f(*min))
              min = from;
       }
   }
   if (min == to)
      return typename Container::value_type();
   else 
      return *min;
}

template <class Container> std::function<Branches(void)> firstFail(CPSolver::Ptr cp,Container& c) {
    using namespace Factory;
    return [=]() {
               auto sx = selectMin(c,
                                   [](const auto& x) { return x->size() > 1;},
                                   [](const auto& x) { return x->size();});
               if (sx) {
                   int v = sx->min();
                   return [cp,sx,v] { TRACE(std::cout << "Choosing (" << sx->size() << ") x" << sx->getId() << " == "<< v << std::endl;) return cp->post(sx == v);}
                       |  [cp,sx,v] { TRACE(std::cout << "Choosing (" << sx->size() << ") x" << sx->getId() << " != "<< v << std::endl;)  return cp->post(sx != v);};
               } else return Branches({});
           };
}

#endif
