#include "visitor.hpp"
#include "conListener.hpp"
#include "constraint.hpp"

void ExpVisitor::visitClause(Clause::Ptr c)
{   
    ClauseExpListener* cl = new ClauseExpListener(c);
}

void ExpVisitor::visitAllDifferentAC(AllDifferentAC::Ptr c)
{   
    AllDiffACExpListener* cl = new AllDiffACExpListener(c);
}
