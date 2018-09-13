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

void DFSearch::dfs(SearchStatistics& stats,Limit limit)
{
    if (limit(stats))
        throw StopException();
    Branches branches = _branching();
    if (branches.size() == 0) {
        stats.incrSolutions();
        notifySolution();
    } else {
        for(auto& alt : branches) {
            _sm->saveState();
            try {
                stats.incrNodes();
                alt();
                dfs(stats,limit);         
            } catch(Status e) {
                stats.incrFailures();
                notifyFailure();
            }
            _sm->restoreState();
        }
    }   
}

void dfsAll(CPSolver::Ptr cps,Chooser& c,std::function<void(void)> onSol) {
    auto ctx = cps->getStateManager();
    Branches b = c();
    if (b.size() == 0) {
        cps->incrNbSol();
        onSol();
        cps->tighten();
    } else {
        for(auto& alt : b) {
            ctx->saveState();
            try {
                cps->incrNbChoices();
                alt();
                dfsAll(cps,c,onSol);
            } catch(Status s) {
            }
            ctx->restoreState();
        }
    }
}
