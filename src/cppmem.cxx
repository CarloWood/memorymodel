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

std::ostream& operator<<(std::ostream& os, function_name const& function_name)
{
  os << function_name.m_name;
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

} // namespace AST
