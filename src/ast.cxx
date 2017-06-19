#include "sys.h"
#include "ast.h"
#include "Symbols.h"
#include <iostream>
#include <boost/variant/get.hpp>

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
      os << "atomic_int";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, unique_lock_decl const& unique_lock_decl)
{
  os << "UNIQUE_LOCK";
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
  os << statement.m_statement;
  if (statement.m_statement.which() <= SN_function_call)
    os << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, operators op)
{
  switch (op)
  {
    case op_eq:
      return os << "==";
    case op_ne:
      return os << "!=";
    case op_lt:
      return os << "<";
  }
  return os << "<UNKNOWN OP>";
}

std::ostream& operator<<(std::ostream& os, simple_expression const& simple_expression)
{
  if (simple_expression.m_simple_expression_node.which() == SE_expression)
    return os << '(' << simple_expression.m_simple_expression_node << ')';
  if (simple_expression.m_simple_expression_node.which() == SE_bool)
  {
    bool val = boost::get<bool>(simple_expression.m_simple_expression_node);
    return os << (val ? "true" : "false");
  }
  return os << simple_expression.m_simple_expression_node;
}

std::ostream& operator<<(std::ostream& os, unary_expression const& unary_expression)
{
  if (unary_expression.m_negated)
    os << '!';
  return os << unary_expression.m_simple_expression;
}

std::ostream& operator<<(std::ostream& os, expression const& expression)
{
  os << expression.m_operand;
  for (auto& chain : expression.m_chained)
    os << ' ' << chain.op << ' ' << chain.operand;
  return os;
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

std::ostream& operator<<(std::ostream& os, load_statement const& load_statement)
{
  os << load_statement.m_memory_location_id << ".load(";
  if (load_statement.m_memory_order != std::memory_order_seq_cst)
    os << load_statement.m_memory_order;
  os << ")";
  if (load_statement.m_readsvalue)
    os << ".readsvalue(" << load_statement.m_readsvalue.get() << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, atomic_fetch_add_explicit const& atomic_fetch_add_explicit)
{
  os << "atomic_fetch_add_explicit(&" << atomic_fetch_add_explicit.m_memory_location_id << ", " << atomic_fetch_add_explicit.m_expression;
  if (atomic_fetch_add_explicit.m_memory_order != std::memory_order_seq_cst)
    os << ", " << atomic_fetch_add_explicit.m_memory_order;
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, store_statement const& store_statement)
{
  os << store_statement.m_memory_location_id << ".store(" << store_statement.m_val;
  if (store_statement.m_memory_order != std::memory_order_seq_cst)
    os << ", " << store_statement.m_memory_order;
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
  os << "break";
  return os;
}

std::ostream& operator<<(std::ostream& os, statement_or_scope const& statement_or_scope)
{
  os << statement_or_scope.m_body;
  return os;
}

std::ostream& operator<<(std::ostream& os, if_statement const& if_statement)
{
  os << "if (" << if_statement.m_condition << ") " << if_statement.m_then;
  return os;
}

std::ostream& operator<<(std::ostream& os, while_statement const& while_statement)
{
  os << "while (" << while_statement.m_condition << ") " << while_statement.m_body;
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
  os << function.m_function_name << "() " << function.m_scope;
  return os;
}

std::ostream& operator<<(std::ostream& os, scope const& scope)
{
  os << "{ ";
  if (scope.m_body)
    os << scope.m_body.get() << ' ';
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, body const& body)
{
  int last = -1;
  for (auto&& node : body.m_body_nodes)
  {
    int bn = node.which();
    if (last == BN_vardecl || last == BN_statement)
      os << ' ';
    os << node;
    last = bn;
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
    if (last == DN_vardecl || last == DN_function)
      os << ' ';
    os << definition;
    last = dn;
  }
  return os;
}

assignment::assignment(register_assignment const& ra) : lhs(parser::Symbols::instance().set_register_id(ra.lhs)), rhs(ra.rhs)
{
}

} // namespace ast
