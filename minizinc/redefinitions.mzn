% Implications
predicate int_eq_imp(var int: a, var int: b, var bool: r);
predicate int_le_imp(var int: a, var int: b, var bool: r);
predicate int_lin_eq_imp(array [int] of int: as, array [int] of var int: bs, int: c, var bool: r);
predicate int_lin_le_imp(array [int] of int: as, array [int] of var int: bs, int: c, var bool: r);
predicate int_lin_ne_imp(array [int] of int: as, array [int] of var int: bs, int: c, var bool: r);
predicate array_bool_and_imp(array [int] of var bool: as, var bool: r);
predicate array_bool_or_imp(array [int] of var bool: as, var bool: r);
predicate bool_and_imp(var bool: a, var bool: b, var bool: r);
predicate bool_eq_imp(var bool: a, var bool: b, var bool: r);
predicate bool_le_imp(var bool: a, var bool: b, var bool: r);
predicate bool_lt_imp(var bool: a, var bool: b, var bool: r);
predicate bool_or_imp(var bool: a, var bool: b, var bool: r);
predicate bool_xor_imp(var bool: a, var bool: b, var bool: r);