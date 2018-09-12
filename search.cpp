#include "search.hpp"
#include "intvar.hpp"
#include "constraint.hpp"

void DFSearch::solve()
{
   _sm->withNewState(std::function<void(void)>([this]() {
                                                  try {
                                                     dfs();
                                                  } catch(...) {}
                                               }));
}

void DFSearch::dfs()
{
   Branches branches = _branching();
   if (branches.size() == 0) {
      notifySolution();
   } else {
      for(auto& alt : branches) {
         _sm->saveState();
         try {
            alt();
            dfs();         
         } catch(Status e) {
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
