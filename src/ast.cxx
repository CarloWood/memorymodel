#include "sys.h"
#include "ast.h"
#include "Symbols_parser.h"
#include "utils/macros.h"
#include <iostream>
#include <boost/variant/get.hpp>

bool constexpr add_parentheses_around_operator_expressions = false;

namespace ast {

std::ostream& operator<<(std::ostream& os, tag const& tag)
{
  os << parser::Symbols::instance().tag_to_string(tag);
  return os;
}

std::ostream& operator<<(std::ostream& os, type const& type)
{
  switch (type.m_type)
  {
    case type_bool:
      os << "bool";
      break;
    case type_int:
      os << "int";
      break;
    case type_atomic_int:
      os << "std::atomic_int";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, mutex_decl const& mutex_decl)
{
  os << "std::mutex " << static_cast<tag const&>(mutex_decl) << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, condition_variable_decl const& condition_variable_decl)
{
  os << "std::condition_variable " << static_cast<tag const&>(condition_variable_decl) << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, unique_lock_decl const& unique_lock_decl)
{
  os << "std::unique_lock<std::mutex> " << static_cast<tag const&>(unique_lock_decl) << '(' << unique_lock_decl.m_mutex << ");";
  return os;
}

std::ostream& operator<<(std::ostream& os, register_location const& register_location)
{
  os << 'r' << register_location.m_id;
  return os;
}

std::ostream& operator<<(std::ostream& os, memory_location const& memory_location)
{
  os << memory_location.m_name;
  return os;
}

std::string declaration_statement::name() const
{
  switch (m_declaration_statement_node.which())
  {
    case DS_mutex_decl:
    {
      auto const& mutex_decl(boost::get<mutex_decl>(m_declaration_statement_node));
      return mutex_decl.m_name;
    }
    case DS_condition_variable_decl:
    {
      auto const& condition_variable_decl(boost::get<condition_variable_decl>(m_declaration_statement_node));
      return condition_variable_decl.m_name;
    }
    case DS_unique_lock_decl:
    {
      auto const& unique_lock_decl(boost::get<unique_lock_decl>(m_declaration_statement_node));
      return unique_lock_decl.m_name;
    }
    case DS_vardecl:
    {
      auto const& vardecl(boost::get<vardecl>(m_declaration_statement_node));
      return vardecl.m_memory_location.m_name;
    }
  }
  // Suppress compiler warning.
  assert(false);
  return "UNKNOWN declaration type";
}

tag declaration_statement::tag() const
{
  switch (m_declaration_statement_node.which())
  {
    case DS_mutex_decl:
    {
      auto const& mutex_decl(boost::get<mutex_decl>(m_declaration_statement_node));
      return mutex_decl;
    }
    case DS_condition_variable_decl:
    {
      auto const& condition_variable_decl(boost::get<condition_variable_decl>(m_declaration_statement_node));
      return condition_variable_decl;
    }
    case DS_unique_lock_decl:
    {
      auto const& unique_lock_decl(boost::get<unique_lock_decl>(m_declaration_statement_node));
      return unique_lock_decl;
    }
    case DS_vardecl:
    {
      auto const& vardecl(boost::get<vardecl>(m_declaration_statement_node));
      return vardecl.m_memory_location;
    }
  }
  // Suppress compiler warning.
  assert(false);
  return tag();
}

std::ostream& operator<<(std::ostream& os, declaration_statement const& declaration_statement)
{
  os << declaration_statement.m_declaration_statement_node;
  return os;
}

std::ostream& operator<<(std::ostream& os, vardecl const& vardecl)
{
  os << vardecl.m_type << ' ' << vardecl.m_memory_location;
  if (vardecl.m_initial_value)
    os << " = " << vardecl.m_initial_value.get();
  os << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, statement const& statement)
{
  os << statement.m_statement_node;
  int w = statement.m_statement_node.which();
  if (w == SN_store_call || w == SN_function_call || w == SN_wait_call)
    os << ';';
  return os;
}

char const* code(assignment_operators op)
{
  switch (op)
  {
    case ao_eq: return "=";
    case ao_mul: return "*=";
    case ao_div: return "/=";
    case ao_mod: return "%=";
    case ao_add: return "+=";
    case ao_sub: return "-=";
    case ao_shr: return ">>=";
    case ao_shl: return "<<=";
    case ao_and: return "&=";
    case ao_xor: return "^=";
    case ao_or: return "|=";
  }
  return "UNKNOWN assignment_operator";
}

char const* code(equality_operators op)
{
  switch (op)
  {
    case eo_eq: return "==";
    case eo_ne: return "!=";
  }
  return "UNKNOWN equality_operator";
}

char const* code(relational_operators op)
{
  switch (op)
  {
    case ro_lt: return "<";
    case ro_gt: return ">";
    case ro_ge: return ">=";
    case ro_le: return "<=";
  }
  return "UNKNOWN relational_operator";
}

char const* code(shift_operators op)
{
  switch (op)
  {
    case so_shl: return "<<";
    case so_shr: return ">>";
  }
  return "UNKNOWN shift_operator";
}

char const* code(additive_operators op)
{
  switch (op)
  {
    case ado_add: return "+";
    case ado_sub: return "-";
  }
  return "UNKNOWN additive_operator";
}

char const* code(multiplicative_operators op)
{
  switch (op)
  {
    case mo_mul: return "*";
    case mo_div: return "/";
  }
  return "UNKNOWN multiplicative_operator";
}

char const* code(unary_operators op)
{
  switch (op)
  {
    case uo_inc: return "++";
    case uo_dec: return "--";
    case uo_dereference: return "*";
    case uo_reference: return "&";
    case uo_plus: return "+";
    case uo_minus: return "-";
    case uo_not: return "!";
    case uo_invert: return "~";
  }
  return "UNKNOWN unary_operator";
}

char const* code(postfix_operators op)
{
  switch (op)
  {
    case po_inc: return "++";
    case po_dec: return "--";
  }
  return "UNKNOWN postfix_operator";
}

std::ostream& operator<<(std::ostream& os, postfix_expression const& postfix_expression)
{
  os << postfix_expression.m_postfix_expression_node;
  bool first = true;
  for (auto& postfix_operator : postfix_expression.m_postfix_operators)
  {
    if (first)
      first = false;
    else
      os << ' ';
    os << code(postfix_operator);
  }
  return os;
}

#if 0
std::ostream& operator<<(std::ostream& os, cast_expression const& cast_expression)
{
  return os;
}

std::ostream& operator<<(std::ostream& os, pm_expression const& pm_expression)
{
  return os;
}
#endif

std::ostream& operator<<(std::ostream& os, multiplicative_expression const& multiplicative_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !multiplicative_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << multiplicative_expression.m_other_expression;
  for (auto& other_expression : multiplicative_expression.m_chained)
    os << ' ' << code(boost::fusion::get<0>(other_expression)) << ' ' << boost::fusion::get<1>(other_expression);
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, additive_expression const& additive_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !additive_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << additive_expression.m_other_expression;
  for (auto& other_expression : additive_expression.m_chained)
    os << ' ' << code(boost::fusion::get<0>(other_expression)) << ' ' << boost::fusion::get<1>(other_expression);
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, shift_expression const& shift_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !shift_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << shift_expression.m_other_expression;
  for (auto& other_expression : shift_expression.m_chained)
    os << ' ' << code(boost::fusion::get<0>(other_expression)) << ' ' << boost::fusion::get<1>(other_expression);
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, relational_expression const& relational_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !relational_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << relational_expression.m_other_expression;
  for (auto& other_expression : relational_expression.m_chained)
    os << ' ' << code(boost::fusion::get<0>(other_expression)) << ' ' << boost::fusion::get<1>(other_expression);
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, equality_expression const& equality_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !equality_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << equality_expression.m_other_expression;
  for (auto& other_expression : equality_expression.m_chained)
    os << ' ' << code(boost::fusion::get<0>(other_expression)) << ' ' << boost::fusion::get<1>(other_expression);
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, and_expression const& and_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !and_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << and_expression.m_other_expression;
  for (auto& other_expression : and_expression.m_chained)
    os << " & " << other_expression;
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, exclusive_or_expression const& exclusive_or_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !exclusive_or_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << exclusive_or_expression.m_other_expression;
  for (auto& other_expression : exclusive_or_expression.m_chained)
    os << " ^ " << other_expression;
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, inclusive_or_expression const& inclusive_or_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !inclusive_or_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << inclusive_or_expression.m_other_expression;
  for (auto& other_expression : inclusive_or_expression.m_chained)
    os << " | " << other_expression;
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, logical_and_expression const& logical_and_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !logical_and_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << logical_and_expression.m_other_expression;
  for (auto& other_expression : logical_and_expression.m_chained)
    os << " && " << other_expression;
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, logical_or_expression const& logical_or_expression)
{
  bool const apao = add_parentheses_around_operator_expressions && !logical_or_expression.m_chained.empty();
  if (apao)
    os << '(';
  os << logical_or_expression.m_other_expression;
  for (auto& other_expression : logical_or_expression.m_chained)
    os << " || " << other_expression;
  if (apao)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, conditional_expression const& conditional_expression)
{
  if (add_parentheses_around_operator_expressions)
    os << '(';
  os << conditional_expression.m_logical_or_expression;
  if (conditional_expression.m_conditional_expression_tail)
    os << " ? " << boost::fusion::get<0>(conditional_expression.m_conditional_expression_tail.get()) <<
          " : " << boost::fusion::get<1>(conditional_expression.m_conditional_expression_tail.get());
  if (add_parentheses_around_operator_expressions)
    os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, assignment_expression const& assignment_expression)
{
  return os << assignment_expression.m_assignment_expression_node;
}

std::ostream& operator<<(std::ostream& os, primary_expression const& primary_expression)
{
  if (primary_expression.m_primary_expression_node.which() == PE_expression)
    return os << '(' << primary_expression.m_primary_expression_node << ')';
  if (primary_expression.m_primary_expression_node.which() == PE_bool)
  {
    bool val = boost::get<bool>(primary_expression.m_primary_expression_node);
    return os << (val ? "true" : "false");
  }
  return os << primary_expression.m_primary_expression_node;
}

std::ostream& operator<<(std::ostream& os, unary_expression const& unary_expression)
{
  bool first = true;
  for (auto& unary_operator : unary_expression.m_unary_operators)
  {
    if (first)
      first = false;
    else
      os << ' ';
    os << code(unary_operator);
  }
  return os << unary_expression.m_postfix_expression;
}

std::ostream& operator<<(std::ostream& os, expression const& expression)
{
  os << expression.m_assignment_expression;
  for (auto& assignment_expression : expression.m_chained)
    os << ", " << assignment_expression;
  return os;
}

std::ostream& operator<<(std::ostream& os, expression_statement const& expression_statement)
{
  if (expression_statement.m_expression)
    os << expression_statement.m_expression.get();
  return os << ';';
}

#define CASE_WRITE(x) do { case x: return os << #x; } while(0)

std::ostream& operator<<(std::ostream& os, std::memory_order const& memory_order)
{
  switch (memory_order)
  {
    CASE_WRITE(std::memory_order_relaxed);
    CASE_WRITE(std::memory_order_consume);
    CASE_WRITE(std::memory_order_acquire);
    CASE_WRITE(std::memory_order_release);
    CASE_WRITE(std::memory_order_acq_rel);
    CASE_WRITE(std::memory_order_seq_cst);
  }
  return os << "<unknown memory order>";
}

std::ostream& operator<<(std::ostream& os, load_call const& load_call)
{
  os << load_call.m_memory_location_id << ".load(";
  if (load_call.m_memory_order != std::memory_order_seq_cst)
    os << load_call.m_memory_order;
  os << ")";
  if (load_call.m_readsvalue)
    os << ".readsvalue(" << load_call.m_readsvalue.get() << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, atomic_fetch_add_explicit const& atomic_fetch_add_explicit)
{
  os << "std::atomic_fetch_add_explicit(&" << atomic_fetch_add_explicit.m_memory_location_id << ", " << atomic_fetch_add_explicit.m_expression;
  if (atomic_fetch_add_explicit.m_memory_order != std::memory_order_seq_cst)
    os << ", " << atomic_fetch_add_explicit.m_memory_order;
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, atomic_fetch_sub_explicit const& atomic_fetch_sub_explicit)
{
  os << "std::atomic_fetch_sub_explicit(&" << atomic_fetch_sub_explicit.m_memory_location_id << ", " << atomic_fetch_sub_explicit.m_expression;
  if (atomic_fetch_sub_explicit.m_memory_order != std::memory_order_seq_cst)
    os << ", " << atomic_fetch_sub_explicit.m_memory_order;
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, atomic_compare_exchange_weak_explicit const& atomic_compare_exchange_weak_explicit)
{
  os << "std::atomic_compare_exchange_weak_explicit(&" <<
      atomic_compare_exchange_weak_explicit.m_memory_location_id << ", " <<
      atomic_compare_exchange_weak_explicit.m_expected << ", " <<
      atomic_compare_exchange_weak_explicit.m_desired;
  if (atomic_compare_exchange_weak_explicit.m_succeed != std::memory_order_seq_cst)
    os << ", " << atomic_compare_exchange_weak_explicit.m_succeed;
  if (atomic_compare_exchange_weak_explicit.m_fail != std::memory_order_seq_cst)
    os << ", " << atomic_compare_exchange_weak_explicit.m_fail;
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, store_call const& store_call)
{
  os << store_call.m_memory_location_id << ".store(" << store_call.m_val;
  if (store_call.m_memory_order != std::memory_order_seq_cst)
    os << ", " << store_call.m_memory_order;
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, register_assignment const& register_assignment)
{
  os << register_assignment.lhs << " = " << register_assignment.rhs;
  return os;
}

std::ostream& operator<<(std::ostream& os, assignment const& assignment)
{
  os << assignment.lhs << " = " << assignment.rhs;
  return os;
}

std::ostream& operator<<(std::ostream& os, function_call const& function_call)
{
  os << function_call.m_function << "()";
  return os;
}

std::ostream& operator<<(std::ostream& os, break_statement const& /*break_statement*/)
{
  os << "break;";
  return os;
}

std::ostream& operator<<(std::ostream& os, return_statement const& return_statement)
{
  os << "return " << return_statement.m_expression << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, jump_statement const& jump_statement)
{
  os << jump_statement.m_jump_statement_node;
  return os;
}

std::ostream& operator<<(std::ostream& os, wait_call const& wait_call)
{
  os << wait_call.m_condition_variable << ".wait(" << wait_call.m_unique_lock << ", [&]" << wait_call.m_compound_statement << ");";
  return os;
}

std::ostream& operator<<(std::ostream& os, notify_all_call const& notify_all_call)
{
  os << notify_all_call.m_condition_variable << ".notify_all();";
  return os;
}

std::ostream& operator<<(std::ostream& os, mutex_lock_call const& mutex_lock_call)
{
  os << mutex_lock_call.m_mutex << ".lock();";
  return os;
}

std::ostream& operator<<(std::ostream& os, mutex_unlock_call const& mutex_unlock_call)
{
  os << mutex_unlock_call.m_mutex << ".unlock();";
  return os;
}

std::ostream& operator<<(std::ostream& os, if_statement const& if_statement)
{
  os << "if (" << if_statement.m_condition << ") ";
  bool ambiguous = if_statement.m_then.m_statement_node.which() == SN_selection_statement; // selection_statement is currently always an if_statement.
  if (ambiguous) os << "{ ";
  os << if_statement.m_then;
  if (ambiguous) os << " }";
  if (if_statement.m_else)
    os << " else " << if_statement.m_else.get();
  return os;
}

std::ostream& operator<<(std::ostream& os, selection_statement const& selection_statement)
{
  os << selection_statement.m_if_statement;
  return os;
}

std::ostream& operator<<(std::ostream& os, while_statement const& while_statement)
{
  os << "while (" << while_statement.m_condition << ") " << while_statement.m_statement;
  return os;
}

std::ostream& operator<<(std::ostream& os, iteration_statement const& iteration_statement)
{
  os << iteration_statement.m_while_statement;
  return os;
}

std::ostream& operator<<(std::ostream& os, function_name const& function_name)
{
  os << function_name.name;
  return os;
}

std::ostream& operator<<(std::ostream& os, function const& function)
{
  if (function.m_function_name.name == "main")
    os << "int ";
  else
    os << "void ";
  os << function.m_function_name << "() " << function.m_compound_statement;
  return os;
}

std::ostream& operator<<(std::ostream& os, compound_statement const& compound_statement)
{
  os << "{ ";
  if (compound_statement.m_statement_seq)
    os << compound_statement.m_statement_seq.get() << ' ';
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, statement_seq const& statement_seq)
{
  bool first = true;
  for (auto&& statement : statement_seq.m_statements)
  {
    if (!first)
      os << ' ';
    os << statement;
    first = false;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, threads const& threads)
{
  os << "{{{ ";
  bool first = true;
  for (auto&& thread : threads.m_threads)
  {
    if (!first)
      os << " ||| ";
    first = false;
    os << thread;
  }
  os << " }}}";
  return os;
}

std::ostream& operator<<(std::ostream& os, cppmem const& cppmem)
{
  int last = -1;
  for (auto&& definition : cppmem)
  {
    int dn = definition.which();
    if (last == DN_declaration_statement ||
        last == DN_function)
      os << ' ';
    os << definition;
    last = dn;
  }
  return os;
}

assignment::assignment(register_assignment const& ra) : lhs(parser::Symbols::instance().set_register_id(ra.lhs)), rhs(ra.rhs)
{
}

char const* assignment_operators_str(assignment_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(ao_eq);
    AI_CASE_RETURN(ao_mul);
    AI_CASE_RETURN(ao_div);
    AI_CASE_RETURN(ao_mod);
    AI_CASE_RETURN(ao_add);
    AI_CASE_RETURN(ao_sub);
    AI_CASE_RETURN(ao_shr);
    AI_CASE_RETURN(ao_shl);
    AI_CASE_RETURN(ao_and);
    AI_CASE_RETURN(ao_xor);
    AI_CASE_RETURN(ao_or);
  }
  return "UNKNOWN assignment_operators";
}

char const* equality_operators_str(equality_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(eo_eq);
    AI_CASE_RETURN(eo_ne);
  }
  return "UNKNOWN equality_operators";
}

char const* relational_operators_str(relational_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(ro_lt);
    AI_CASE_RETURN(ro_gt);
    AI_CASE_RETURN(ro_ge);
    AI_CASE_RETURN(ro_le);
  }
  return "UNKNOWN relational_operators";
}

char const* shift_operators_str(shift_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(so_shl);
    AI_CASE_RETURN(so_shr);
  }
  return "UNKNOWN shift_operators";
}

char const* additive_operators_str(additive_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(ado_add);
    AI_CASE_RETURN(ado_sub);
  }
  return "UNKNOWN additive_operators";
}

char const* multiplicative_operators_str(multiplicative_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(mo_mul);
    AI_CASE_RETURN(mo_div);
  }
  return "UNKNOWN multiplicative_operators";
}

char const* unary_operators_str(unary_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(uo_inc);
    AI_CASE_RETURN(uo_dec);
    AI_CASE_RETURN(uo_dereference);
    AI_CASE_RETURN(uo_reference);
    AI_CASE_RETURN(uo_plus);
    AI_CASE_RETURN(uo_minus);
    AI_CASE_RETURN(uo_not);
    AI_CASE_RETURN(uo_invert );
  }
  return "UNKNOWN unary_operators";
}

char const* postfix_operators_str(postfix_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(po_inc);
    AI_CASE_RETURN(po_dec);
  }
  return "UNKNOWN postfix_operators";
}

std::ostream& operator<<(std::ostream& os, assignment_operators op)
{
  return os << assignment_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, equality_operators op)
{
  return os << equality_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, relational_operators op)
{
  return os << relational_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, shift_operators op)
{
  return os << shift_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, additive_operators op)
{
  return os << additive_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, multiplicative_operators op)
{
  return os << multiplicative_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, unary_operators op)
{
  return os << unary_operators_str(op);
}

std::ostream& operator<<(std::ostream& os, postfix_operators op)
{
  return os << postfix_operators_str(op);
}


} // namespace ast
