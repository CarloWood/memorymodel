#pragma once

namespace ast {

enum assignment_operators { ao_eq, ao_mul, ao_div, ao_mod, ao_add, ao_sub, ao_shr, ao_shl, ao_and, ao_xor, ao_or };
enum equality_operators { eo_eq, eo_ne };
enum relational_operators { ro_lt, ro_gt, ro_ge, ro_le };
enum shift_operators { so_shl, so_shr };
enum additive_operators { ado_add, ado_sub };
enum multiplicative_operators { mo_mul, mo_div, mo_mod };
enum unary_operators { uo_inc, uo_dec, uo_dereference, uo_reference, uo_plus, uo_minus, uo_not, uo_invert };
enum postfix_operators { po_inc, po_dec };

} // namespace ast
