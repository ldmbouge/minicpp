#include "search.hpp"
#include "intvar.hpp"
#include "constraint.hpp"

void dfsAll(CPSolver::Ptr cps,Chooser& c,std::function<void(void)> onSol) {
    Context::Ptr ctx = cps->context();
    Branches b = c();
    if (b.size() == 0) {
        cps->incrNbSol();
        onSol();
        cps->tighten();
    } else {
        for(auto& alt : b) {
            ctx->push();
            cps->incrNbChoices();
            alt();
            Status s = cps->status();
            if (s != Failure)
                dfsAll(cps,c,onSol);
            ctx->pop();
        }
    }
}
