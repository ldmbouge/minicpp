#pragma once

#include <solver.hpp>
#include <fz_parser/flatzinc.h>
#include <fz_constraints/bool_array.hpp>
#include <fz_constraints/bool_bin.hpp>
#include <fz_constraints/bool_lin.hpp>
#include <fz_constraints/bool_misc.hpp>
#include <fz_constraints/int_array.hpp>
#include <fz_constraints/int_bin.hpp>
#include <fz_constraints/int_lin.hpp>
#include <fz_constraints/int_tern.hpp>

namespace Factory
{
    Constraint::Ptr makeConstraint(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
}