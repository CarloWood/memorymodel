#pragma once

#include "debug.h"

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

  friend std::ostream& operator<<(std::ostream& os, type const& type);
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

  friend std::ostream& operator<<(std::ostream& os, register_location const& register_location);
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

  friend std::ostream& operator<<(std::ostream& os, memory_location const& memory_location);
};

// TYPE MEMORY_LOCATION [= INT_];
struct global {
  type m_type;
  memory_location m_memory_location;
  boost::optional<int> m_initial_value;

  global() { }
  global(type type, memory_location memory_location) : m_type(type), m_memory_location(memory_location) { }
  global(type type, memory_location memory_location, int initial_value) : m_type(type), m_memory_location(memory_location), m_initial_value(initial_value) { }

  friend bool operator==(global const& g1, global const& g2) {
    return g1.m_type == g2.m_type && g1.m_memory_location == g2.m_memory_location && g1.m_initial_value == g2.m_initial_value;
  }

  friend std::ostream& operator<<(std::ostream& os, global const& global);
};

struct statement : std::string {
  friend std::ostream& operator<<(std::ostream& os, statement const& statement);
};


struct scope;
struct threads;
using body_node = boost::variant<statement, boost::recursive_wrapper<scope>, boost::recursive_wrapper<threads>>;

struct body
{
  using container_type = std::vector<body_node>;
  container_type m_body_nodes;

  friend std::ostream& operator<<(std::ostream& os, body const& body);

  // Workaround for the fact that spirit/fusion doesn't work correctly for structs containing a single STL container.
  bool m_dummy;
};

struct scope
{
  boost::optional<body> m_body;

  friend std::ostream& operator<<(std::ostream& os, scope const& scope);

  // For the test suite.
  bool operator==(std::string const& statement) const;
};

struct threads
{
  using container_type = std::vector<body>;
  container_type m_threads;

  friend std::ostream& operator<<(std::ostream& os, threads const& threads);

  // Workaround for the fact that spirit/fusion doesn't work correctly for structs containing a single STL container.
  bool m_dummy;
};

// IDENTIFIER
struct function_name
{
  std::string m_name;

  function_name() { }
  function_name(std::string const& name) : m_name(name) { }

  friend std::ostream& operator<<(std::ostream& os, function_name const& function_name);
};

// void FUNCTION_NAME()
// SCOPE
struct function {
  function_name m_function_name;
  scope m_scope;

  function() { }
  function(function_name function_name) : m_function_name(function_name) { }

  friend std::ostream& operator<<(std::ostream& os, function const& function);
};

using definition_node = boost::variant<global, function>;

// *DEFINITION
struct cppmem : public std::vector<definition_node>
{
  friend std::ostream& operator<<(std::ostream& os, cppmem const& cppmem);
};

enum Nonterminals               { AType, ARegisterLocation, AMemoryLocation, AGlobal, AStatement, AScope, AFunctionName, AFunction, ACppMem, AThreads };
using nonterminal = boost::variant<type,  register_location, memory_location, global,  statement,  scope,  function_name, function,  cppmem,  threads>;

} // namespace AST
