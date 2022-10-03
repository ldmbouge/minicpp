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

#include "search.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "tracer.hpp"

class StopException {};

typedef std::function<void(void)> VVFun;

SearchStatistics DFSearch::solve(SearchStatistics& stats,Limit limit)
{
    stats.setIntVars(_cp->getNbVars());
    stats.setPropagators(_cp->getNbProp());
    _sm->withNewState(VVFun([this,&stats,&limit]() {
                               try {
                                  dfs(stats,limit);
                               } catch(StopException& sx) {
                                  stats.setNotCompleted();
                               }
                            }));
    stats.setSolveTime();
    stats.setPropagations(_cp->getPropagations());
    return stats;
}

SearchStatistics DFSearch::solve(Limit limit)
{
    SearchStatistics stats;
    solve(stats,limit);
    return stats;
}

SearchStatistics DFSearch::solve()
{
    SearchStatistics stats;
    solve(stats,[](const SearchStatistics& ss) { return false;});
    return stats;
}

SearchStatistics DFSearch::solveSubjectTo(Limit limit,std::function<void(void)> subjectTo)
{
    SearchStatistics stats;
    _sm->withNewState(VVFun([this,&stats,&limit,&subjectTo]() {
                               try {
                                  subjectTo();
                                  dfs(stats,limit);
                               } catch(StopException& sx) {}
                            }));
    return stats;
}

SearchStatistics DFSearch::optimize(Objective::Ptr obj,SearchStatistics& stats,Limit limit)
{
   onSolution([obj] { obj->tighten();});
   return solve(stats,limit);
}

SearchStatistics DFSearch::optimize(Objective::Ptr obj,SearchStatistics& stats)
{
   onSolution([obj] { obj->tighten();});
   return solve(stats,[](const SearchStatistics& ss) { return false;});
}

SearchStatistics DFSearch::optimize(Objective::Ptr obj,Limit limit)
{
   SearchStatistics stats;
   onSolution([obj] { obj->tighten();});
   return solve(stats,limit);
}

SearchStatistics DFSearch::optimize(Objective::Ptr obj)
{
   return optimize(obj,[](const SearchStatistics& ss) { return false;});
}

SearchStatistics DFSearch::optimizeSubjectTo(Objective::Ptr obj,Limit limit,std::function<void(void)> subjectTo)
{
   SearchStatistics stats;
   _sm->withNewState(VVFun([this,&stats,obj,&limit,&subjectTo]() {
                              try {
                                 subjectTo();
                                 stats = optimize(obj,limit);
                              } catch(StopException& sx) {}
                           }));
   return stats;
}

void DFSearch::dfs(SearchStatistics& stats,const Limit& limit)
{
   //static int nS = 0;
    Branches branches = _branching();
    if (branches.size() == 0) {
       //nS++;
       //std::cout<< "DFSFail -> sol " << nS << std::endl;
       stats.incrSolutions();
       notifySolution();
    }
    else {
       // if (branches.size() > 1)
       //    stats.incrNodes();
       auto last = std::prev(branches.end()); // for proper counting of choices.
       for(auto cur = branches.begin(); cur != branches.end() and !limit(stats); cur++)
       {
          const auto& alt = *cur;
          _sm->saveState();
          try {
             TRYFAIL {
                if (cur != last)
                   stats.incrNodes();
                alt();
                dfs(stats, limit);
             } ONFAIL {
                stats.incrFailures();
                notifyFailure();
             }
             ENDFAIL {
                _sm->restoreState();
             }
          } catch(...) {  // the C++ exception catching is to stay compatible with python interfaces. 0-cost for C++
             stats.incrFailures();
             notifyFailure();
             _sm->restoreState();
          }
       }
       if (limit(stats)) {
          throw StopException();
       }
    }
}

SearchStatistics BFSearch::solve(SearchStatistics& stats,Limit limit)
{
    stats.setIntVars(_cp->getNbVars());
    stats.setPropagators(_cp->getNbProp());
    _cp->startSearch();
    Trailer::Ptr trail = _sm;
    trail->withNewState(VVFun([this,&stats,&limit]() {
                               try {
                                  bfs(stats,limit);
                               } catch(StopException& sx) {
                                  stats.setNotCompleted();
                               }
                            }));
    stats.setSolveTime();
    stats.setPropagations(_cp->getPropagations());
    return stats;
}

SearchStatistics BFSearch::solve(Limit limit)
{
    SearchStatistics stats;
    solve(stats,limit);
    return stats;
}

SearchStatistics BFSearch::solve()
{
    SearchStatistics stats;
    solve(stats,[](const SearchStatistics& ss) { return false;});
    return stats;
}

SearchStatistics BFSearch::optimize(Objective::Ptr obj,SearchStatistics& stats,Limit limit)
{
   _objective = obj;
   onSolution([obj] { obj->tighten();});
   return solve(stats,limit);
}

SearchStatistics BFSearch::optimize(Objective::Ptr obj,SearchStatistics& stats)
{
   _objective = obj;
   onSolution([obj] { obj->tighten();});
   return solve(stats,[](const SearchStatistics& ss) { return false;});
}

SearchStatistics BFSearch::optimize(Objective::Ptr obj,Limit limit)
{
   _objective = obj;
   SearchStatistics stats;
   onSolution([obj] { obj->tighten();});
   return solve(stats,limit);
}

SearchStatistics BFSearch::optimize(Objective::Ptr obj)
{
   _objective = obj;
   return optimize(obj,[](const SearchStatistics& ss) { return false;});
}

void BFSearch::bfs(SearchStatistics& stats,const Limit& limit)
{
   Tracer* tracer = _cp->tracer();
   while (true) {
      Branches branches = _branching();
      if (branches.size() == 0) {
         //nS++;
         //std::cout<< "BFSFail -> sol " << nS << std::endl;
         stats.incrSolutions();
         //Catch fail from tightening objective
         TRYFAIL {
            notifySolution();
         } ONFAIL {
         } ENDFAIL {
         }
      } else {
         // if (branches.size() > 1)
         //    stats.incrNodes();
         auto last = std::prev(branches.end()); // for proper counting of choices.
         for(auto cur = branches.begin(); cur != branches.end() and !limit(stats); cur++)
         {
            const auto& alt = *cur;
            _before = tracer->captureCheckpoint();
            try {
               TRYFAIL {
                  if (cur != last)
                     stats.incrNodes();
                  alt();
                  _frontier.push(BFSNode(tracer->captureCheckpoint(), _objective ? _objective->value() : 0, tracer->level()));
               } ONFAIL {
                  stats.incrFailures();
                  notifyFailure();
               }
               ENDFAIL {
                  tracer->restoreCheckpoint(_before,_cp);
               }
            } catch(...) {  // the C++ exception catching is to stay compatible with python interfaces. 0-cost for C++
               stats.incrFailures();
               notifyFailure();
               tracer->restoreCheckpoint(_before,_cp);
            }
         }
         if (limit(stats)) {
            throw StopException();
         }
      }
      BFSNode node;
      bool feasibleNodeFound = false;
      bool successfulRestore = false;
      while (!successfulRestore) {
         while (!_frontier.empty()) {
            node = _frontier.top();
            _frontier.pop();
            if (!_objective || _objective->betterThanPrimal(node._objectiveValue)) {
               feasibleNodeFound = true;
               break;
            }
         }
         if (feasibleNodeFound)
            successfulRestore = tracer->restoreCheckpoint(node._checkpoint,_cp);
         else
            return;
      }
   }
}
