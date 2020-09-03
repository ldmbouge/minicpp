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

class StopException {};

typedef std::function<void(void)> VVFun;

SearchStatistics DFSearch::solve(SearchStatistics& stats,Limit limit)
{
    _sm->withNewState(VVFun([this,&stats,&limit]() {
                               try {
                                  dfs(stats,limit);
                               } catch(StopException& sx) {}
                            }));
    return stats;
}

SearchStatistics DFSearch::solve(Limit limit)
{
    SearchStatistics stats;
    return solve(stats,limit);
}

SearchStatistics DFSearch::solve()
{
    SearchStatistics stats;
    return solve(stats,[](const SearchStatistics& ss) { return false;});
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
    if (limit(stats))
        throw StopException();
    Branches branches = _branching();
    if (branches.size() == 0) {
        stats.incrSolutions();
        notifySolution();
        // throw Success;
    } else {
        for(auto& alt : branches) {
            _sm->saveState();
            try {
                stats.incrNodes();
                stats.incrDepth();
                alt();
                dfs(stats,limit);         
            } catch(Status e) {
                stats.incrFailures();
                notifyFailure();
            }
            _sm->restoreState();
            stats.decrDepth();
        }
    }   
}

ExpDFSearch::ExpDFSearch(ExpSolver::Ptr exp,std::function<Branches(void)>&& b) 
  : _exp(exp), _dfs(exp->getSolver(),std::move(b)) 
{ 
    exp->getExplainer()->injectListeners();
}


SearchStatistics ExpDFSearch::solve() 
{
    SearchStatistics stats;
    _exp->setSearchStats(&stats);
    return _dfs.solve(stats,[](const SearchStatistics& ss) { return false;});
}

ExpTestSearch::ExpTestSearch(ExpSolver::Ptr exp)
  : _exp(exp), _sm(exp->getStateManager())
{
    _exp->getExplainer()->injectListeners();
}

SearchStatistics ExpTestSearch::solve(std::vector<std::function<void(void)>> choices)
{
    SearchStatistics stats;
    _exp->setSearchStats(&stats);
    for (auto& c : choices) {
        try {
            _sm->saveState();
            stats.incrNodes();
            stats.incrDepth();
            c();
        }
        catch (Status e) {
            stats.incrFailures();
            _sm->restoreState();
        }
    }
    return stats;
}
