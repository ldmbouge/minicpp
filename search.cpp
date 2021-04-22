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

#include <iostream>
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
                std::cout << "going down to depth " << stats.getDepth() << "\n";
                alt();
                dfs(stats,limit);         
            } catch(Status e) {
                stats.incrFailures();
                notifyFailure();
            }
            std::cout << "restoring and going back to depth " << stats.getDepth()-1 << "\n";
            _sm->restoreState();
            stats.decrDepth();
        }
    }   
}

ExpDFSearch::ExpDFSearch(ExpSolver::Ptr exp,std::function<Branches(void)>&& b) 
  : DFSearch(exp->getSolver(),std::move(b)), _exp(exp) 
{ 
    _exp->getExplainer()->injectListeners();
}

SearchStatistics ExpDFSearch::solve()
{
    SearchStatistics stats;
    _exp->setSearchStats(&stats);
    return solve(stats,[](const SearchStatistics& ss) { return false;});
}

SearchStatistics ExpDFSearch::solve(SearchStatistics& stats,Limit limit)
{
    _sm->withNewState(VVFun([this,&stats,&limit]() {
                               try {
                                  dfs(stats,limit);
                               } catch(StopException& sx) {}
                            }));
    return stats;
}

void ExpDFSearch::dfs(SearchStatistics& stats,const Limit& limit)
{
    if (limit(stats))
        throw StopException();
    bool redoBranching;
    int const myDepth = stats.getDepth();
    do {
        // _sm->saveState();
        redoBranching = false;
        Branches branches = _branching();
        if (branches.size() == 0) {
            stats.incrSolutions();
            notifySolution();
        } else {
            for(auto& alt : branches) {
                std::cout << "saving state\n";
                _sm->saveState();
                try {
                    stats.incrNodes();
                    stats.incrDepth();
                    std::cout << "going down to depth " << stats.getDepth() << "\n";
                    alt();
                    dfs(stats,limit);         
                } catch(Status e) {
                    _exp->setStatus(e);
                    if (e.type() == Failure) {
                        std::cout << "encountered failure status\n";
                        stats.incrFailures();
                        notifyFailure();
                    }
                    else {
                        if (e.type() == BackJump) {
                            std::cout << "caught backjump status from dfs\n";
                            if (myDepth == e.depth()) {
                                redoBranching = true;
                                _exp->purgeCuts();
                                try {
                                    _exp->postCuts();
                                }
                                catch(Status e) {
                                    _exp->setStatus(e);
                                    if(e.type() == Failure) {
                                        std::cout << "failed from posting cut at depth " << myDepth << "\n";
                                        stats.decrDepth();
                                        throw e;  
                                    }
                                        else
                                            assert(false);
                                    }
                                // _exp->checkCuts();
                                stats.decrDepth();
                                break;
                            }
                            else {
                                std::cout << "restoring state\n";
                                _sm->restoreState();
                                stats.decrDepth();
                                backJumpTo(e.depth());
                            }
                        }
                    }
                }
                std::cout << "restoring to go back to depth " << stats.getDepth()-1 << "\n";
                try {
                    std::cout << "restoring state\n";
                    _sm->restoreState();
                    stats.decrDepth();
                } catch (Status e) {
                    _exp->setStatus(e);
                    stats.decrDepth();
                    if (e.type() == BackJump) {
                        std::cout << "caught backjump status from restoreState\n";
                        if (myDepth == e.depth()) {  
                            redoBranching = true;
                            // _exp->checkCuts();
                            _exp->purgeCuts();
                            try {
                                _exp->postCuts();
                            }
                            catch(Status e) {
                                _exp->setStatus(e);
                                if(e.type() == Failure) {
                                    std::cout << "failed from posting cut at depth " << myDepth << "\n";
                                    throw e;  
                                }
                                else
                                    assert(false);
                            }
                            break;
                        }
                        else {
                            backJumpTo(e.depth());
                        }
                    }
                }
                _exp->purgeCuts();
                // _exp->checkCuts();
            }
        }   
        // _sm->restoreState();
    } while (redoBranching);
}

// ExpDFSearch::ExpDFSearch(ExpSolver::Ptr exp,std::function<Branches(void)>&& b) 
//   : _exp(exp), _dfs(exp->getSolver(),std::move(b)) 
// { 
//     exp->getExplainer()->injectListeners();
// }


// SearchStatistics ExpDFSearch::solve() 
// {
//     SearchStatistics stats;
//     _exp->setSearchStats(&stats);
//     return _dfs.solve(stats,[](const SearchStatistics& ss) { return false;});
// }

// ExpTestSearch::ExpTestSearch(ExpSolver::Ptr exp)
//   : _exp(exp), _sm(exp->getStateManager())
// {
//     _exp->getExplainer()->injectListeners();
//     _sm->enable();
// }

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
            stats.decrDepth();
            _exp->getExplainer()->checkCuts();
        }
    }
    return stats;
}
