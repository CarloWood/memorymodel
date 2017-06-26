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

  bool is_atomic() const { return m_type == type_atomic_int; }

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

struct load_call
{
  tag m_memory_location_id;
  std::memory_order m_memory_order;
  boost::optional<int> m_readsvalue;

  friend std::ostream& operator<<(std::ostream& os, load_call const& load_call);
};

struct register_assignment;
struct assignment;
struct atomic_fetch_add_explicit;
struct atomic_fetch_sub_explicit;
struct atomic_compare_exchange_weak_explicit;
struct expression;

enum SimpleExpression {
    SE_int,
    SE_bool,
    SE_tag,
    SE_load_call,
    SE_register_assignment,
    SE_assignment,
    SE_atomic_fetch_add_explicit,
    SE_atomic_fetch_sub_explicit,
    SE_atomic_compare_exchange_weak_explicit,
    SE_expression
};

using simple_expression_node = boost::variant<
    int,
    bool,
    tag,
    load_call,
    boost::recursive_wrapper<register_assignment>,
    boost::recursive_wrapper<assignment>,
    boost::recursive_wrapper<atomic_fetch_add_explicit>,
    boost::recursive_wrapper<atomic_fetch_sub_explicit>,
    boost::recursive_wrapper<atomic_compare_exchange_weak_explicit>,
    boost::recursive_wrapper<expression>
>;

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

enum operators { op_eq, op_ne, op_lt, op_gt, op_le, op_ge, op_bo, op_ba };
struct chain
{
  operators op;
  expression operand;
};

struct expression_statement
{
  expression m_expression;

  friend std::ostream& operator<<(std::ostream& os, expression_statement const& expression_statement);
};

struct atomic_fetch_add_explicit
{
  tag m_memory_location_id;
  expression m_expression;
  std::memory_order m_memory_order;

  friend std::ostream& operator<<(std::ostream& os, atomic_fetch_add_explicit const& atomic_fetch_add_explicit);
};

struct atomic_fetch_sub_explicit
{
  tag m_memory_location_id;
  expression m_expression;
  std::memory_order m_memory_order;

  friend std::ostream& operator<<(std::ostream& os, atomic_fetch_sub_explicit const& atomic_fetch_sub_explicit);
};

struct atomic_compare_exchange_weak_explicit
{
  tag m_memory_location_id;
  int m_expected;
  int m_desired;
  std::memory_order m_succeed;
  std::memory_order m_fail;

  friend std::ostream& operator<<(std::ostream& os, atomic_compare_exchange_weak_explicit const& atomic_compare_exchange_weak_explicit);
};

struct store_call
{
  tag m_memory_location_id;
  std::memory_order m_memory_order;
  expression m_val;

  friend std::ostream& operator<<(std::ostream& os, store_call const& store_call);
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

  // Workaround for the fact that automatic attribute propagation rules have some trouble with Fusion sequences that consist of a single element.
  // See https://stackoverflow.com/questions/44730979/boost-spirit-compile-error-for-trivial-grammar
  bool m_dummy;
};

struct break_statement
{
  bool m_dummy;

  friend std::ostream& operator<<(std::ostream& os, break_statement const& break_statement);
};

struct return_statement
{
  expression m_expression;

  friend std::ostream& operator<<(std::ostream& os, return_statement const& return_statement);
};

enum JumpStatementNode                { JS_break_statement, JS_return_statement };
using jump_statement_node = boost::variant<break_statement,    return_statement>;

struct jump_statement
{
  jump_statement_node m_jump_statement_node;

  friend std::ostream& operator<<(std::ostream& os, jump_statement const& jump_statement);
};

struct mutex_decl : tag
{
  std::string m_name;

  friend std::ostream& operator<<(std::ostream& os, mutex_decl const& mutex_decl);
};

struct condition_variable_decl : tag
{
  std::string m_name;

  friend std::ostream& operator<<(std::ostream& os, condition_variable_decl const& condition_variable_decl);
};

struct unique_lock_decl : tag
{
  tag m_lock;
  std::string m_name;
  tag m_mutex;

  friend std::ostream& operator<<(std::ostream& os, unique_lock_decl const& unique_lock_decl);
};

struct vardecl
{
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

enum DeclarationStatementNode {                DS_mutex_decl, DS_condition_variable_decl, DS_unique_lock_decl, DS_vardecl };
using declaration_statement_node = boost::variant<mutex_decl,    condition_variable_decl,    unique_lock_decl,    vardecl>;

struct declaration_statement
{
  declaration_statement_node m_declaration_statement_node;

  ast::tag tag() const;
  std::string name() const;

  friend std::ostream& operator<<(std::ostream& os, declaration_statement const& declaration_statement);
};

//-----------------------------------------------------------------------------

struct wait_call;
struct notify_all_call;
struct threads;
struct compound_statement;
struct selection_statement;
struct iteration_statement;

enum Statement {
    SN_expression_statement,
    SN_store_call,
    SN_function_call,
    SN_wait_call,
    SN_notify_all_call,
    SN_threads,
    SN_compound_statement,
    SN_selection_statement,
    SN_iteration_statement,
    SN_jump_statement,
    SN_declaration_statement
};

using statement_node = boost::variant<
    expression_statement,
    store_call,
    function_call,
    boost::recursive_wrapper<wait_call>,
    boost::recursive_wrapper<notify_all_call>,
    boost::recursive_wrapper<threads>,
    boost::recursive_wrapper<compound_statement>,
    boost::recursive_wrapper<selection_statement>,
    boost::recursive_wrapper<iteration_statement>,
    jump_statement,
    declaration_statement
>;

struct statement
{
  statement_node m_statement_node;

  friend std::ostream& operator<<(std::ostream& os, statement const& statement);

  // Workaround for the fact that automatic attribute propagation rules have some trouble with Fusion sequences that consist of a single element.
  // See https://stackoverflow.com/questions/44730979/boost-spirit-compile-error-for-trivial-grammar
  bool m_dummy;
};

struct statement_seq
{
  typedef std::vector<statement> container_type;
  container_type m_statements;

  friend std::ostream& operator<<(std::ostream& os, statement_seq const& statement_seq);

  // Workaround for the fact that spirit/fusion doesn't work correctly for structs containing a single STL container.
  bool m_dummy;
};

struct compound_statement
{
  boost::optional<statement_seq> m_statement_seq;

  friend std::ostream& operator<<(std::ostream& os, compound_statement const& compound_statement);

  // For the test suite.
  bool operator==(std::string const& statement) const;
};

struct if_statement
{
  expression m_condition;
  statement m_then;
  boost::optional<statement> m_else;

  friend std::ostream& operator<<(std::ostream& os, if_statement const& if_statement);
};

struct selection_statement
{
  if_statement m_if_statement;

  friend std::ostream& operator<<(std::ostream& os, selection_statement const& selection_statement);
};

struct while_statement
{
  expression m_condition;
  statement m_statement;

  friend std::ostream& operator<<(std::ostream& os, while_statement const& while_statement);
};

struct iteration_statement
{
  while_statement m_while_statement;

  friend std::ostream& operator<<(std::ostream& os, iteration_statement const& iteration_statement);
};

struct threads
{
  typedef std::vector<statement_seq> container_type;
  container_type m_threads;

  friend std::ostream& operator<<(std::ostream& os, threads const& threads);

  // Workaround for the fact that spirit/fusion doesn't work correctly for structs containing a single STL container.
  bool m_dummy;
};

struct wait_call
{
  tag m_condition_variable;
  tag m_unique_lock;
  compound_statement m_compound_statement;

  friend std::ostream& operator<<(std::ostream& os, wait_call const& wait_call);
};

struct notify_all_call
{
  tag m_condition_variable;

  friend std::ostream& operator<<(std::ostream& os, notify_all_call const& notify_all_call);

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
  compound_statement m_compound_statement;

  function() : tag(-1) { }
  function(function_name function_name) : m_function_name(function_name) { }
#ifdef BOOST_FUSION_HAS_VARIADIC_VECTOR
  function(boost::fusion::vector<ast::function_name, ast::compound_statement> const& attr) :
#else
  function(boost::fusion::vector2<ast::function_name, ast::compound_statement> const& attr) :
#endif
      m_function_name(boost::fusion::at_c<0>(attr)), m_compound_statement(boost::fusion::at_c<1>(attr)) { }

  friend std::ostream& operator<<(std::ostream& os, function const& function);
};

enum DefinitionNode               { DN_declaration_statement, DN_function };
using definition_node = boost::variant<declaration_statement,    function>;

// *DEFINITION
struct cppmem : public std::vector<definition_node>
{
  friend std::ostream& operator<<(std::ostream& os, cppmem const& cppmem);
};

enum Nonterminals             { NT_type, NT_register_location, NT_memory_location, NT_vardecl, NT_statement, NT_compound_statement, NT_function_name, NT_function, NT_cppmem, NT_threads };
using nonterminal = boost::variant<type,    register_location,    memory_location,    vardecl,    statement,    compound_statement,    function_name,    function,    cppmem,    threads>;

} // namespace ast
