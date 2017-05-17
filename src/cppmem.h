#pragma once

#include <boost/variant/recursive_wrapper.hpp>
#include <boost/variant/variant.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

namespace AST
{

enum Type { type_int, type_atomic_int };

// "int" | "atomic_int"
struct type
{
  Type m_type;

  type() { }
  type(Type t) : m_type(t) { }

  friend bool operator!=(type const& t1, Type t2) { return t1.m_type != t2; }
  friend bool operator!=(Type t1, type const& t2) { return t1 != t2.m_type; }
  friend bool operator!=(type const& t1, type const& t2) { return t1.m_type != t2.m_type; }
  friend bool operator==(type const& t1, Type t2) { return t1.m_type == t2; }
  friend bool operator==(Type t1, type const& t2) { return t1 == t2.m_type; }
  friend bool operator==(type const& t1, type const& t2) { return t1.m_type == t2.m_type; }

  friend std::ostream& operator<<(std::ostream& os, type const& type)
  {
    switch (type.m_type)
    {
      case type_int:
        os << "*int";
        break;
      case type_atomic_int:
        os << "*atomic_int";
        break;
    }
    return os;
  }
};

// register_location = 'r' > uint_;
struct register_location
{
  unsigned int m_id;

  register_location() : m_id(0) { }
  register_location(unsigned int id) : m_id(id) { }

  friend bool operator!=(register_location const& rl1, unsigned int id2) { return rl1.m_id != id2; }
  friend bool operator!=(unsigned int id1, register_location const& rl2) { return id1 != rl2.m_id; }
  friend bool operator!=(register_location const& rl1, register_location const& rl2) { return rl1.m_id != rl2.m_id; }
  friend bool operator==(register_location const& rl1, unsigned int id2) { return rl1.m_id == id2; }
  friend bool operator==(unsigned int id1, register_location const& rl2) { return id1 == rl2.m_id; }
  friend bool operator==(register_location const& rl1, register_location const& rl2) { return rl1.m_id == rl2.m_id; }

  friend std::ostream& operator<<(std::ostream& os, register_location const& register_location)
  {
    os << 'r' << register_location.m_id;
    return os;
  }
};

// memory_location = identifier - register_location;
struct memory_location
{
  std::string m_name;

  memory_location() { }
  memory_location(char const* name) : m_name(name) { }

  friend bool operator!=(memory_location const& ml1, std::string const& ml2) { return ml1.m_name != ml2; }
  friend bool operator!=(std::string const& ml1, memory_location const& ml2) { return ml1 != ml2.m_name; }
  friend bool operator!=(memory_location const& ml1, memory_location const& ml2) { return ml1.m_name != ml2.m_name; }
  friend bool operator==(memory_location const& ml1, std::string const& ml2) { return ml1.m_name == ml2; }
  friend bool operator==(std::string const& ml1, memory_location const& ml2) { return ml1 == ml2.m_name; }
  friend bool operator==(memory_location const& ml1, memory_location const& ml2) { return ml1.m_name == ml2.m_name; }

  friend bool operator==(memory_location const& ml1, char const* ml2) { return ml1.m_name == ml2; }

  friend std::ostream& operator<<(std::ostream& os, memory_location const& memory_location)
  {
    os << memory_location.m_name;
    return os;
  }
};

// TYPE MEMORY_LOCATION [= INT_];
struct global {
  type m_type;
  memory_location m_memory_location;
  boost::optional<int> m_initial_value;

  global() { }
  global(type type, memory_location memory_location) : m_type(type), m_memory_location(memory_location) { }

  friend bool operator==(global const& g1, global const& g2) {
    return g1.m_type == g2.m_type && g1.m_memory_location == g2.m_memory_location && g1.m_initial_value == g2.m_initial_value;
  }

  friend std::ostream& operator<<(std::ostream& os, global const& global)
  {
    os << global.m_type << ' ' << global.m_memory_location;
    if (global.m_initial_value)
      os << " = " << global.m_initial_value;
    os << ';';
    return os;
  }
};

// IDENTIFIER
struct function_name
{
  std::string m_function_name;

  friend std::ostream& operator<<(std::ostream& os, function_name const& function_name)
  {
    os << function_name.m_function_name;
    return os;
  }
};

// {
//   *(SCOPE | +STATEMENT;)
// }
struct scope
{
  friend std::ostream& operator<<(std::ostream& os, scope const& /*scope*/)
  {
    os << "{ }";
    return os;
  }
};

// void FUNCTION_NAME()
// SCOPE
struct function {
  function_name m_function_name;
  scope m_scope;

  friend std::ostream& operator<<(std::ostream& os, function const& function)
  {
    os << "void " << function.m_function_name << "() " << function.m_scope;
    return os;
  }
};

using definition_node = boost::variant<global, function>;

// *DEFINITION
struct cppmem : public std::vector<definition_node>
{
  friend std::ostream& operator<<(std::ostream& os, cppmem const& cppmem)
  {
    for (auto&& definition : cppmem)
      os << definition << "; ";
    return os;
  }
};

enum Nonterminals               { AType, ARegisterLocation, AMemoryLocation, AGlobal, AFunctionName, AScope, AFunction, ACppMem };
using nonterminal = boost::variant<type,  register_location, memory_location, global,  function_name, scope,  function,  cppmem>;

} // namespace AST

namespace cppmem
{

extern AST::nonterminal parse(std::string const& text);

} // namespace cppmem
