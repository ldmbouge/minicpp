#include "acstr.hpp"
#include "solver.hpp"

Constraint::Constraint(CPSolver::Ptr cp)
   : _scheduled(false),_active(cp->getStateManager(),true)
{}
