#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include <list>
#include <atomic>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/variant/variant.hpp>
#include <boost/optional.hpp>

namespace ast {

struct tag
{
  int id;
  tag() { }
  explicit tag(int id_) : id(id_) { }

  friend std::ostream& operator<<(std::ostream& os, tag const& tag);
  friend bool operator!=(tag const& tag1, tag const& tag2) { return tag1.id != tag2.id; }
};

enum Type { type_bool, type_int, type_atomic_int };

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
struct register_location : tag
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
struct memory_location : tag
{
  type m_type;
  std::string m_name;

  memory_location() { }
  memory_location(std::string const& name) : m_name(name) { }
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

struct identifier : tag
{
  identifier(std::string const& name = "") : name(name) { }
  std::string name;
};

struct load_statement
{
  tag m_memory_location_id;
  std::memory_order m_memory_order;
  boost::optional<int> m_readsvalue;

  friend std::ostream& operator<<(std::ostream& os, load_statement const& load_statement);
};

struct expression;
struct atomic_fetch_add_explicit;
enum operators { op_eq, op_ne, op_lt };

enum SimpleExpression                    { SE_int, SE_bool, SE_tag,                       SE_atomic_fetch_add_explicit,  SE_load_statement,                       SE_expression };
using simple_expression_node = boost::variant<int,    bool,    tag, boost::recursive_wrapper<atomic_fetch_add_explicit>,    load_statement, boost::recursive_wrapper<expression>>;

struct simple_expression
{
  simple_expression_node m_simple_expression_node;

  friend std::ostream& operator<<(std::ostream& os, simple_expression const& simple_expression);
};

struct unary_expression
{
  bool m_negated = false;
  simple_expression m_simple_expression;

  friend std::ostream& operator<<(std::ostream& os, unary_expression const& unary_expression);
};

struct chain;
struct expression
{
  unary_expression m_operand;
  std::vector<chain> m_chained;

  friend std::ostream& operator<<(std::ostream& os, expression const& expression);
};

struct chain
{
  operators op;
  expression operand;
};

struct atomic_fetch_add_explicit
{
  tag m_memory_location_id;
  expression m_expression;
  std::memory_order m_memory_order;

  friend std::ostream& operator<<(std::ostream& os, atomic_fetch_add_explicit const& atomic_fetch_add_explicit);
};

struct store_statement
{
  tag m_memory_location_id;
  std::memory_order m_memory_order;
  expression m_val;

  friend std::ostream& operator<<(std::ostream& os, store_statement const& store_statement);
};

struct register_assignment
{
  register_location lhs;
  expression rhs;

  friend std::ostream& operator<<(std::ostream& os, register_assignment const& register_assignment);
};

struct assignment
{
  tag lhs;
  expression rhs;

  assignment() = default;

  // Convert a register_assignment to an assignment.
  assignment(register_assignment const& ra);

  friend std::ostream& operator<<(std::ostream& os, assignment const& assignment);
};

struct function_call
{
  tag m_function;

  friend std::ostream& operator<<(std::ostream& os, function_call const& function_call);
};

struct break_statement
{
  bool m_dummy;

  friend std::ostream& operator<<(std::ostream& os, break_statement const& break_statement);
};

struct unique_lock_decl : tag
{
  tag m_lock;
  std::string m_name;
  tag m_mutex;

  friend std::ostream& operator<<(std::ostream& os, unique_lock_decl const& unique_lock_decl);
};

struct if_statement;
struct while_statement;
enum Statement                   { SN_break_statement, SN_assignment, SN_load_statement, SN_store_statement, SN_function_call,                       SN_if_statement,                        SN_while_statement };
using statement_node = boost::variant<break_statement,    assignment,    load_statement,    store_statement,    function_call, boost::recursive_wrapper<if_statement>, boost::recursive_wrapper<while_statement>>;

struct statement
{
  statement_node m_statement;

  friend std::ostream& operator<<(std::ostream& os, statement const& statement);
};

// TYPE MEMORY_LOCATION [= INT_];
struct vardecl {
  type m_type;
  memory_location m_memory_location;
  boost::optional<expression> m_initial_value;

  vardecl() { }
  vardecl(type type, memory_location memory_location) : m_type(type), m_memory_location(memory_location) { }
  vardecl(type type, memory_location memory_location, expression initial_value) : m_type(type), m_memory_location(memory_location), m_initial_value(initial_value) { }
#ifdef BOOST_FUSION_HAS_VARIADIC_VECTOR
  vardecl(boost::fusion::vector<ast::type, ast::memory_location, boost::optional<expression>> const& attr) :
#else
  vardecl(boost::fusion::vector3<ast::type, ast::memory_location, boost::optional<expression>> const& attr) :
#endif
      m_type(boost::fusion::at_c<0>(attr)), m_memory_location(boost::fusion::at_c<1>(attr))
      {
        if (boost::fusion::at_c<2>(attr))
          m_initial_value = boost::fusion::at_c<2>(attr).get();
      }

  friend std::ostream& operator<<(std::ostream& os, vardecl const& vardecl);
};

struct scope;
struct threads;
enum BodyNode               { BN_vardecl, BN_statement,                       BN_scope,                        BN_threads };
using body_node = boost::variant<vardecl,    statement, boost::recursive_wrapper<scope>, boost::recursive_wrapper<threads>>;

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

enum StatementOrScopeNode                 { SS_statement, SS_scope };
using statement_or_scope_node = boost::variant<statement,    scope>;

struct statement_or_scope
{
  statement_or_scope_node m_body;

  friend std::ostream& operator<<(std::ostream& os, statement_or_scope const& statement_or_scope);
};

struct if_statement
{
  expression m_condition;
  statement_or_scope m_then;
  //boost::optional<statement_or_scope> m_else;

  friend std::ostream& operator<<(std::ostream& os, if_statement const& if_statement);
};

struct while_statement
{
  expression m_condition;
  statement_or_scope m_body;

  friend std::ostream& operator<<(std::ostream& os, while_statement const& while_statement);
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
  std::string name;

  function_name(std::string const& name = "") : name(name) { }

  // Ugly stuff necessary to make this work with boost::spirit (wtf!?!)
  using value_type = std::string::value_type;
  bool empty() const { return name.empty(); }
  void insert(std::string::const_iterator at, value_type c) { name.insert(at, c); }
  std::string::const_iterator end() const { return name.end(); }
  template<typename Iterator> function_name(Iterator const& begin, Iterator const& end) : name(begin, end) { }

  friend std::ostream& operator<<(std::ostream& os, function_name const& function_name);
};

// void FUNCTION_NAME()
// SCOPE
struct function : tag
{
  function_name m_function_name;
  scope m_scope;

  function() { }
  function(function_name function_name) : m_function_name(function_name) { }
#ifdef BOOST_FUSION_HAS_VARIADIC_VECTOR
  function(boost::fusion::vector<ast::function_name, ast::scope> const& attr) :
#else
  function(boost::fusion::vector2<ast::function_name, ast::scope> const& attr) :
#endif
      m_function_name(boost::fusion::at_c<0>(attr)), m_scope (boost::fusion::at_c<1>(attr)) { }

  friend std::ostream& operator<<(std::ostream& os, function const& function);
};

enum DefinitionNode               { DN_vardecl, DN_function };
using definition_node = boost::variant<vardecl,    function>;

// *DEFINITION
struct cppmem : public std::vector<definition_node>
{
  friend std::ostream& operator<<(std::ostream& os, cppmem const& cppmem);
};

enum Nonterminals             { NT_type, NT_register_location, NT_memory_location, NT_vardecl, NT_statement, NT_scope, NT_function_name, NT_function, NT_cppmem, NT_threads };
using nonterminal = boost::variant<type,    register_location,    memory_location,    vardecl,    statement,    scope,    function_name,    function,    cppmem,    threads>;

} // namespace ast
