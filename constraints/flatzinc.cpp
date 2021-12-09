#include <constraints/flatzinc.hpp>

Constraint::Ptr FznConstraint(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, FlatZinc::Constraint const & c)
{

    switch (c.type)
    {
        case FlatZinc::Constraint::array_int_element:
            return new (cp) array_int_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_int_maximum:
            return new (cp) array_int_maximum(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_int_minimum:
            return new (cp) array_int_minimum(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_var_int_element:
            return new (cp) array_var_int_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_abs:
            return new (cp) int_abs(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_div:
            return new (cp) int_div(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_eq:
            return new (cp) int_eq(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_eq_reif:
            return new (cp) int_eq_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_le:
            return new (cp) int_le(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_le_reif:
            return new (cp) int_le_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_eq:
            return new (cp) int_lin_eq(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_eq_reif:
            return new (cp) int_lin_eq_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_le:
            return new (cp) int_lin_le(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_le_reif:
            return new (cp) int_lin_le_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_ne:
            return new (cp) int_lin_ne(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_ne_reif:
            return new (cp) int_lin_ne_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lt:
            return new (cp) int_lt(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lt_reif:
            return new (cp) int_lt_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_max:
            return new (cp) int_max(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_min:
            return new (cp) int_min(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_mod:
            return new (cp) int_mod(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_ne:
            return new (cp) int_ne(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_ne_reif:
            return new (cp) int_ne_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_plus:
            return new (cp) int_plus(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_pow:
            return new (cp) int_pow(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_times:
            return new (cp) int_times(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_bool_and_reif:
            return new (cp) array_bool_and_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_bool_element:
            return new (cp) array_bool_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_bool_or_reif:
            return new (cp) array_bool_or_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_bool_xor:
            return new (cp) array_bool_xor(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_var_bool_element:
            return new (cp) array_var_bool_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool2int:
            return new (cp) bool2int(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_and_reif:
            return new (cp) bool_and_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_clause:
            return new (cp) bool_clause(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_eq:
            return new (cp) bool_eq(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_eq_reif:
            return new (cp) bool_eq_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_le:
            return new (cp) bool_le(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_le_reif:
            return new (cp) bool_le_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_lin_eq:
            return new (cp) bool_lin_eq(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_lin_le:
            return new (cp) bool_lin_le(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_lt:
            return new (cp) bool_lt(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_lt_reif:
            return new (cp) bool_lt_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_not:
            return new (cp) bool_not(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_or_reif:
            return new (cp) bool_or_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_xor:
            return new (cp) bool_xor(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_xor_reif:
            return new (cp) bool_xor_reif(cp, intVars, boolVars, c.vars, c.consts);

        default:
            printf("[ERROR] Constraint %s not supported.", FlatZinc::Constraint::type2str[c.type]);
            exit(EXIT_FAILURE);
    }
}
