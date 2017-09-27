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

define test
call gdb_test($arg0, $arg1, $arg2, $arg3)
end
