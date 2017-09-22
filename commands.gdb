set print static off

define pbe
call gdb_print_boolean_expression($arg0)
end

define pex
call gdb_print_expression($arg0)
end

define pev
call gdb_print_evaluation($arg0)
end
