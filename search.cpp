#include "search.hpp"
#include "intvar.hpp"
#include "constraint.hpp"

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
         cps->incrNbChoices();
         alt();
         Status s = cps->status();
         if (s != Failure)
            dfsAll(cps,c,onSol);
         ctx->restoreState();
      }
   }
}
