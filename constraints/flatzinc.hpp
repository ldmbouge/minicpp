#pragma once

#include <solver.hpp>
#include <flatzinc/flatzinc.h>
#include <constraints/bool_array.hpp>
#include <constraints/bool_bin.hpp>
#include <constraints/bool_lin.hpp>
#include <constraints/bool_misc.hpp>
#include <constraints/int_array.hpp>
#include <constraints/int_bin.hpp>
#include <constraints/int_lin.hpp>
#include <constraints/int_tern.hpp>

namespace Factory
{
    Constraint::Ptr makeConstraint(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
}