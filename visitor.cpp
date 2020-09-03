#include "visitor.hpp"
#include "conListener.hpp"

void ExpVisitor::visitClause(Constraint* c)
{   
    ClauseExpListener* cl = new ClauseExpListener(c);
}

void ExpVisitor::visitAllDifferentAC(Constraint* c)
{   
    AllDiffACExpListener* cl = new AllDiffACExpListener(c);
}