#include <fz_constraints/flatzinc.hpp>
#include <constraint.hpp>
#include <utils.hpp>

Constraint::Ptr Factory::makeConstraint(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars)
{

    switch (fzConstraint.type)
    {
        //Builtins
        case FlatZinc::Constraint::array_int_element:
            return new (cp) array_int_element(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_int_maximum:
            return new (cp) array_int_maximum(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_int_minimum:
            return new (cp) array_int_minimum(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_var_int_element:
            return new (cp) array_var_int_element(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_abs:
            return new (cp) int_abs(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_div:
            return new (cp) int_div(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_eq:
            return new (cp) int_eq(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_eq_reif:
            return new (cp) int_eq_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_le:
            return new (cp) int_le(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_le_reif:
            return new (cp) int_le_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_eq:
            return new (cp) int_lin_eq(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_eq_reif:
            return new (cp) int_lin_eq_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_le:
            return new (cp) int_lin_le(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_le_reif:
            return new (cp) int_lin_le_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_ne:
            return new (cp) int_lin_ne(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lin_ne_reif:
            return new (cp) int_lin_ne_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lt:
            return new (cp) int_lt(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_lt_reif:
            return new (cp) int_lt_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_max:
            return new (cp) int_max(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_min:
            return new (cp) int_min(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_mod:
            return new (cp) int_mod(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_ne:
            return new (cp) int_ne(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_ne_reif:
            return new (cp) int_ne_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_plus:
            return new (cp) int_plus(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_pow:
            return new (cp) int_pow(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::int_times:
            return new (cp) int_times(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_bool_and_reif:
            return new (cp) array_bool_and_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_bool_element:
            return new (cp) array_bool_element(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_bool_or_reif:
            return new (cp) array_bool_or_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_bool_xor:
            return new (cp) array_bool_xor(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::array_var_bool_element:
            return new (cp) array_var_bool_element(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool2int:
            return new (cp) bool2int(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_and_reif:
            return new (cp) bool_and_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_clause:
            return new (cp) bool_clause(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_eq:
            return new (cp) bool_eq(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_eq_reif:
            return new (cp) bool_eq_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_le:
            return new (cp) bool_le(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_le_reif:
            return new (cp) bool_le_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_lin_eq:
            return new (cp) bool_lin_eq(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_lin_le:
            return new (cp) bool_lin_le(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_lt:
            return new (cp) bool_lt(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_lt_reif:
            return new (cp) bool_lt_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_not:
            return new (cp) bool_not(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_or_reif:
            return new (cp) bool_or_reif(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_xor:
            return new (cp) bool_xor(cp, fzConstraint, int_vars, bool_vars);

        case FlatZinc::Constraint::bool_xor_reif:
            return new (cp) bool_xor_reif(cp, fzConstraint, int_vars, bool_vars);

        //Globals
        case FlatZinc::Constraint::all_different:
        {
            std::vector<var<int>::Ptr> x;
            for(size_t i = 0; i < fzConstraint.vars.size(); i += 1)
            {
                x.push_back(int_vars[fzConstraint.vars[i]]);
            }
            return new (cp) AllDifferentAC(x);
        }

        default:
            printError("Unsupported constraint");
            exit(EXIT_FAILURE);
    }
}
