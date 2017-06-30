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
    case op_gt:
      return os << ">";
    case op_le:
      return os << "<=";
    case op_ge:
      return os << ">=";
    case op_bo:
      return os << "||";
    case op_ba:
      return os << "&&";
    case op_add:
      return os << "+";
    case op_sub:
      return os << "-";
    case op_mul:
      return os << "*";
    case op_div:
      return os << "/";
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

std::ostream& operator<<(std::ostream& os, expression_statement const& expression_statement)
{
  os << expression_statement.m_expression << ';';
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

} // namespace ast
