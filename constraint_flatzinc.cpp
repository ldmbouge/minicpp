#include "constraint_flatzinc.hpp"

Constraint::Ptr FznConstraint(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, FlatZinc::Constraint const & c)
{

    switch (c.type)
    {
        case FlatZinc::Constraint::array_int_element:
            return new (cp) array_int_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_var_int_element:
            return new (cp) array_var_int_element(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_eq_reif:
            return new (cp) int_eq_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_eq:
            return new (cp) int_lin_eq(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_eq_reif:
            return new (cp) int_lin_eq_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::int_lin_ne:
            return new (cp) int_lin_ne(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::array_bool_or_reif:
            return new (cp) array_bool_or_reif(cp, intVars, boolVars, c.vars, c.consts);

        case FlatZinc::Constraint::bool_clause:
            return new (cp) bool_clause(cp, intVars, boolVars, c.vars, c.consts);

        default:
            printf("[ERROR] Constraint %s not supported.", FlatZinc::Constraint::type2str[c.type]);
            exit(EXIT_FAILURE);
    }
}
