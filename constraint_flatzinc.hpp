#pragma once

#include "solver.hpp"
#include <flatzinc/flatzinc.h>
#include "constraint_flatzinc_int.hpp"
#include "constraint_flatzinc_bool.hpp"

Constraint::Ptr FznConstraint(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, FlatZinc::Constraint const & c);