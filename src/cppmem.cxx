#include "sys.h"
#include "debug.h"
#include "cppmem.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/get.hpp>

#include <iostream>

namespace AST
{

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
  os << static_cast<std::string const&>(statement) << ';';
  return os;
}

std::ostream& operator<<(std::ostream& os, function const& function)
{
  os << "void " << function.m_function_name << "() " << function.m_scope;
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
  for (auto&& node : body.m_body_nodes)
    os << node;
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
  for (auto&& definition : cppmem)
    os << definition << "; ";
  return os;
}

} // namespace AST
