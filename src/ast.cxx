#include "sys.h"
#include "ast.h"
#include "Symbols.h"
#include <iostream>

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
    case type_int:
      os << "int";
      break;
    case type_atomic_int:
      os << "atomic_int";
      break;
  }
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
  return os << statement.m_statement << ';';
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
  os << load_statement.m_memory_location_id << ".load(" << load_statement.m_memory_order << ")";
  if (load_statement.m_readsvalue)
    os << ".readsvalue(" << load_statement.m_readsvalue.get() << ')';
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

std::ostream& operator<<(std::ostream& os, if_statement const& if_statement)
{
  os << "IF_STATEMENT";
  return os;
}

std::ostream& operator<<(std::ostream& os, while_statement const& while_statement)
{
  os << "WHILE_STATEMENT";
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
