#include "sys.h"
#include "debug.h"
#include "ast.h"
#include "position_handler.h"
#include "Graph.h"
#include "Context.h"
#include "cppmem_parser.h"
#include "utils/AIAlert.h"
#include <boost/variant/get.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <stack>

struct TagCompare {
  bool operator()(ast::tag const& t1, ast::tag const& t2) const
  {
    assert(t1.id != -1 && t2.id != -1);
    return t1.id < t2.id;
  }
};

std::map<ast::tag, ast::function, TagCompare> functions;

template<typename AST>
class ScopeDetector {
 private:
  using m_asts_type = std::vector<std::pair<size_t, AST>>;
  m_asts_type m_asts;
 public:
  void add(AST const& unique_lock_decl);
  void reset(size_t stack_depth, Context& context);
  virtual void left_scope(AST const& ast, Context& context) = 0;
};

class Locks : public ScopeDetector<ast::unique_lock_decl> {
  void left_scope(ast::unique_lock_decl const& unique_lock_decl, Context& context) override;
};

Locks locks;

class Loops {
 private:
  std::stack<ast::iteration_statement> m_stack;
 public:
  void enter(ast::iteration_statement const& iteration_statement);
  void leave(ast::iteration_statement const& iteration_statement);
  void add_break(ast::break_statement const& break_statement);
};

void Loops::enter(ast::iteration_statement const& iteration_statement)
{
  m_stack.push(iteration_statement);
}

void Loops::leave(ast::iteration_statement const& iteration_statement)
{
  DebugMarkUp;
  // This is where a break jumps to.
  Dout(dc::notice, "TODO: left scope of: " << iteration_statement);
  m_stack.pop();
}

void Loops::add_break(ast::break_statement const& break_statement)
{
  Dout(dc::notice, "TODO: break;");
}

Loops loops;

class Symbols {
 private:
  std::vector<std::pair<std::string, ast::declaration_statement>> m_symbols;
  std::stack<int> m_stack;
 public:
  void add(ast::declaration_statement const& declaration_statement);
  void scope_start(bool is_thread);
  void scope_end(Context& context);
  int stack_depth() const { return m_stack.size(); }
  ast::declaration_statement const& find(std::string var_name) const;
};

Symbols symbols;

void Symbols::add(ast::declaration_statement const& declaration_statement)
{
  m_symbols.push_back(std::make_pair(declaration_statement.name(), declaration_statement));
}

template<typename AST>
void ScopeDetector<AST>::add(AST const& ast)
{
  m_asts.push_back(typename m_asts_type::value_type(symbols.stack_depth(), ast));
}

template<typename AST>
void ScopeDetector<AST>::reset(size_t stack_depth, Context& context)
{
  while (!m_asts.empty() && m_asts.back().first > stack_depth)
  {
    DebugMarkUp;
    left_scope(m_asts.back().second, context);
    m_asts.erase(m_asts.end());
  }
}

void Locks::left_scope(ast::unique_lock_decl const& unique_lock_decl, Context& context)
{
  context.m_graph.unlock(unique_lock_decl.m_mutex, context);
}

void execute_body(std::string name, ast::statement_seq const& body, Context& context);

void Symbols::scope_start(bool is_thread)
{
  if (is_thread)
    Dout(dc::notice, "New thread:");
  Dout(dc::notice, "{");
  Debug(libcw_do.inc_indent(2));
  m_stack.push(m_symbols.size());
}

void Symbols::scope_end(Context& context)
{
  Debug(libcw_do.dec_indent(2));
  Dout(dc::notice, "}");
  m_symbols.resize(m_stack.top());
  m_stack.pop();
  locks.reset(m_stack.size(), context);
}

ast::declaration_statement const& Symbols::find(std::string var_name) const
{
  auto iter = m_symbols.rbegin();
  while (iter != m_symbols.rend())
  {
    if (iter->first == var_name)
      break;
    ++iter;
  }
  assert(iter != m_symbols.rend());
  return iter->second;
}

void execute_expression(ast::expression const& expression, Context& context);

void execute_declaration(ast::declaration_statement const& declaration_statement, Context& context)
{
  if (declaration_statement.m_declaration_statement_node.which() == ast::DS_vardecl)
  {
    auto const& vardecl{boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node)};
    if (vardecl.m_initial_value)
    {
      execute_expression(*vardecl.m_initial_value, context);
      Dout(dc::notice, declaration_statement);
      DebugMarkUp;
      context.m_graph.write(declaration_statement.tag(), context);
    }
    else
    {
      Dout(dc::notice, declaration_statement);
      DebugMarkUp;
      context.m_graph.uninitialized(declaration_statement.tag(), context);
    }
  }
  else if (declaration_statement.m_declaration_statement_node.which() == ast::DS_unique_lock_decl)
  {
    auto const& unique_lock_decl{boost::get<ast::unique_lock_decl>(declaration_statement.m_declaration_statement_node)};
    DoutTag(dc::notice, declaration_statement << " [declaration of", declaration_statement.tag());
    DebugMarkUp;
    context.m_graph.lock(unique_lock_decl.m_mutex, context);
    locks.add(unique_lock_decl);
  }
  else if (declaration_statement.m_declaration_statement_node.which() == ast::DS_mutex_decl)
  {
    auto const& mutex_decl{boost::get<ast::mutex_decl>(declaration_statement.m_declaration_statement_node)};
    DoutTag(dc::notice, declaration_statement << " [declaration of", declaration_statement.tag());
    DebugMarkUp;
    context.m_graph.lockdecl(mutex_decl, context);
  }
  else
  {
    DoutTag(dc::notice, declaration_statement << " [declaration of", declaration_statement.tag());
    DebugMarkUp;
    context.m_graph.uninitialized(declaration_statement.tag(), context);
  }
  symbols.add(declaration_statement);
}

void execute_condition(ast::expression const& condition, Context& context)
{
  execute_expression(condition, context);
  Dout(dc::continued, condition);
}

void execute_simple_expression(ast::simple_expression const& simple_expression, Context& context)
{
  auto const& node = simple_expression.m_simple_expression_node;
  switch (node.which())
  {
    case ast::SE_int:
      //auto const& literal(boost::get<int>(node));
      break;
    case ast::SE_bool:
      //auto const& literal(boost::get<bool>(node));
      break;
    case ast::SE_tag:
    {
      auto const& tag(boost::get<ast::tag>(node));                                 // tag that the parser thinks we are using.
      // Check if that variable wasn't masked by another variable of different type.
      std::string name = parser::Symbols::instance().tag_to_string(tag);           // Actual C++ object of that name in current scope.
      ast::declaration_statement const& declaration_statement{symbols.find(name)}; // Declaration of actual object.
      if (declaration_statement.tag() != tag)                                      // Did the parser make a mistake?
      {
        std::string position{context.m_position_handler.location(declaration_statement.tag())};
        if (declaration_statement.m_declaration_statement_node.which() == ast::DS_vardecl)
        {
          auto const& vardecl(boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node));
          if (vardecl.m_memory_location.m_type == ast::type_atomic_int)
            THROW_ALERT("Please use [MEMORY_LOCATION].load() for atomic_int declared at [POSITION] instead of [MEMORY_LOCATION]",
                        AIArgs("[MEMORY_LOCATION]", vardecl.m_memory_location)("[POSITION]", position));
        }
        THROW_ALERT("[OBJECT] declared at [POSITION] has the wrong type for this operation,",
                    AIArgs("[OBJECT]", declaration_statement)("[POSITION]", position));
      }
      // This must be a non-atomic int or bool, or we wouldn't be reading from it.
      assert(declaration_statement.m_declaration_statement_node.which() == ast::DS_vardecl &&
          !boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node).m_type.is_atomic());
      context.m_graph.read(tag, context);
      break;
    }
    case ast::SE_load_call:
    {
      auto const& load_call{boost::get<ast::load_call>(node)};
      context.m_graph.read(load_call.m_memory_location_id, load_call.m_memory_order, context);
      break;
    }
    case ast::SE_register_assignment:
    {
      auto const& register_assignment{boost::get<ast::register_assignment>(node)};
      execute_expression(register_assignment.rhs, context);
      break;
    }
    case ast::SE_assignment:
    {
      auto const& assignment{boost::get<ast::assignment>(node)};
      execute_expression(assignment.rhs, context);
      context.m_graph.write(assignment.lhs, context);
      break;
    }
    case ast::SE_atomic_fetch_add_explicit:
    {
      auto const& atomic_fetch_add_explicit{boost::get<ast::atomic_fetch_add_explicit>(node)};
      execute_expression(atomic_fetch_add_explicit.m_expression, context);
      context.m_graph.read(atomic_fetch_add_explicit.m_memory_location_id, atomic_fetch_add_explicit.m_memory_order, context);
      context.m_graph.write(atomic_fetch_add_explicit.m_memory_location_id, atomic_fetch_add_explicit.m_memory_order, context);
      break;
    }
    case ast::SE_atomic_fetch_sub_explicit:
    {
      auto const& atomic_fetch_sub_explicit{boost::get<ast::atomic_fetch_sub_explicit>(node)};
      execute_expression(atomic_fetch_sub_explicit.m_expression, context);
      context.m_graph.read(atomic_fetch_sub_explicit.m_memory_location_id, atomic_fetch_sub_explicit.m_memory_order, context);
      context.m_graph.write(atomic_fetch_sub_explicit.m_memory_location_id, atomic_fetch_sub_explicit.m_memory_order, context);
      break;
    }
    case ast::SE_atomic_compare_exchange_weak_explicit:
    {
      auto const& atomic_compare_exchange_weak_explicit{boost::get<ast::atomic_compare_exchange_weak_explicit>(node)};
      DoutTag(dc::notice, "TODO: [load/(store) of", atomic_compare_exchange_weak_explicit.m_memory_location_id);
      break;
    }
    case ast::SE_expression:
    {
      auto const& expression{boost::get<ast::expression>(node)};
      execute_expression(expression, context);
      break;
    }
  }
}

void execute_expression(ast::expression const& expression, Context& context)
{
#ifdef CWDEBUG
  debug::Mark marker("\e[42m⇩\e[0m");
#endif
  ast::unary_expression const& unary_expression{expression.m_operand};
  execute_simple_expression(unary_expression.m_simple_expression, context);
  for (auto& chain : expression.m_chained)
  {
    execute_expression(chain.operand, context);
  }
}

void execute_statement(ast::statement const& statement, Context& context)
{
  auto const& node = statement.m_statement_node;
  switch (node.which())
  {
    case ast::SN_expression_statement:
    {
      auto const& expression_statement{boost::get<ast::expression_statement>(node)};
      execute_expression(expression_statement.m_expression, context);
      Dout(dc::notice, expression_statement);
      break;
    }
    case ast::SN_store_call:
    {
      auto const& store_call{boost::get<ast::store_call>(node)};
      execute_expression(store_call.m_val, context);
      Dout(dc::notice, store_call << ";");
      DebugMarkUp;
      context.m_graph.write(store_call.m_memory_location_id, store_call.m_memory_order, context);
      break;
    }
    case ast::SN_function_call:
    {
      auto const& function_call{boost::get<ast::function_call>(node)};
#ifdef CWDEBUG
      Dout(dc::notice, function_call.m_function << "();");
      debug::Mark mark;
#endif
      auto const& function{functions[function_call.m_function]};
      if (function.m_compound_statement.m_statement_seq)
        execute_body(function.m_function_name.name, *function.m_compound_statement.m_statement_seq, context);
      break;
    }
    case ast::SN_wait_call:
    {
      auto const& wait_call{boost::get<ast::wait_call>(node)};
      Dout(dc::notice, "TODO: " << wait_call);
      break;
    }
    case ast::SN_notify_all_call:
    {
      auto const& notify_all_call{boost::get<ast::notify_all_call>(node)};
      Dout(dc::notice, "TODO: " << notify_all_call);
      break;
    }
    case ast::SN_mutex_lock_call:
    {
      auto const& mutex_lock_call{boost::get<ast::mutex_lock_call>(node)};
      Dout(dc::notice, mutex_lock_call);
      DebugMarkUp;
      context.m_graph.lock(mutex_lock_call.m_mutex, context);
      break;
    }
    case ast::SN_mutex_unlock_call:
    {
      auto const& mutex_unlock_call{boost::get<ast::mutex_unlock_call>(node)};
      Dout(dc::notice, mutex_unlock_call);
      DebugMarkUp;
      context.m_graph.unlock(mutex_unlock_call.m_mutex, context);
      break;
    }
    case ast::SN_threads:
    {
      auto const& threads{boost::get<ast::threads>(node)};
      for (auto& statement_seq : threads.m_threads)
        execute_body("thread", statement_seq, context);
      break;
    }
    case ast::SN_compound_statement:
    {
      auto const& compound_statement{boost::get<ast::compound_statement>(node)};
      if (compound_statement.m_statement_seq)
        execute_body("compound_statement", *compound_statement.m_statement_seq, context);
      break;
    }
    case ast::SN_selection_statement:
    {
      auto const& selection_statement{boost::get<ast::selection_statement>(node)};
      Dout(dc::notice|continued_cf, "if (");
      execute_condition(selection_statement.m_if_statement.m_condition, context);
      Dout(dc::finish, ")");
      execute_statement(selection_statement.m_if_statement.m_then, context);
      if (selection_statement.m_if_statement.m_else)
        execute_statement(*selection_statement.m_if_statement.m_else, context);
      break;
    }
    case ast::SN_iteration_statement:
    {
      auto const& iteration_statement{boost::get<ast::iteration_statement>(node)};
      Dout(dc::notice|continued_cf, "while (");
      execute_condition(iteration_statement.m_while_statement.m_condition, context);
      Dout(dc::finish, ")");
      loops.enter(iteration_statement);
      execute_statement(iteration_statement.m_while_statement.m_statement, context);
      loops.leave(iteration_statement);
      break;
    }
    case ast::SN_jump_statement:
    {
      auto const& jump_statement{boost::get<ast::jump_statement>(node)};
      switch (jump_statement.m_jump_statement_node.which())
      {
        case ast::JS_break_statement:
        {
          auto const& break_statement{boost::get<ast::break_statement>(jump_statement.m_jump_statement_node)};
          loops.add_break(break_statement);
          break;
        }
        case ast::JS_return_statement:
        {
          auto const& return_statement{boost::get<ast::return_statement>(jump_statement.m_jump_statement_node)};
          Dout(dc::notice, "TODO: " << return_statement);
          break;
        }
      }
      break;
    }
    case ast::SN_declaration_statement:
    {
      auto const& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement, context);
      break;
    }
  }
}

void execute_body(std::string name, ast::statement_seq const& body, Context& context)
{
#ifdef CWDEBUG
  if (name != "compound_statement" && name != "thread")
  {
    if (name == "main")
      Dout(dc::notice, "int " << name << "()");
    else
      Dout(dc::notice, "void " << name << "()");
  }
#endif
  symbols.scope_start(name == "thread");
  for (auto const& statement : body.m_statements)
  {
    try
    {
      execute_statement(statement, context);
    }
    catch (AIAlert::Error const& alert)
    {
      THROW_ALERT(alert, " in `[STATEMENT]`", AIArgs("[STATEMENT]", statement));
    }
  }
  symbols.scope_end(context);
}

int main(int argc, char* argv[])
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  char const* filename;
  if (argc == 2)
  {
    filename = argv[1];
  }
  else
  {
    std::cerr << "Usage: " << argv[0] << " <input file>\n";
    return 1;
  }

  std::ifstream in(filename);
  if (!in.is_open())
  {
    std::cerr << "Failed to open input file \"" << filename << "\".\n";
    return 1;
  }

  std::string source_code;              // We will read the contents here.
  in.unsetf(std::ios::skipws);          // No white space skipping!
  std::copy(
      std::istream_iterator<char>(in),
      std::istream_iterator<char>(),
      std::back_inserter(source_code));
  in.close();

  ast::cppmem ast;
  iterator_type begin(source_code.begin());
  iterator_type const end(source_code.end());
  position_handler<iterator_type> position_handler(filename, begin, end);
  try
  {
    if (!cppmem::parse(begin, end, position_handler, ast))
      return 1;
  }
  catch (std::exception const& error)
  {
    Dout(dc::warning, "Parser threw exception: " << error.what());
  }

  Graph graph;
  Context context(position_handler, graph);

  std::cout << "Abstract Syntax Tree: " << ast << std::endl;

  // Collect all global variables and their initialization, if any.
  for (auto& node : ast)
    if (node.which() == ast::DN_declaration_statement)
    {
      ast::declaration_statement& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement, context);
    }

  // Collect all function definitions.
  ast::function const* main_function;
  for (auto& node : ast)
    if (node.which() == ast::DN_function)
    {
      ast::function& function{boost::get<ast::function>(node)};
      if (function.id == -1)
      {
        Dout(dc::warning, "function " << function << " has id -1 !?!");
      }
      functions[function] = function;
      //std::cout << "Function definition: " << function << std::endl;
      if (function.m_function_name.name == "main")
        main_function = &functions[function];
    }

  try
  {
    // Execute main()
    if (main_function->m_compound_statement.m_statement_seq)
      execute_body("main", *main_function->m_compound_statement.m_statement_seq, context);
  }
  catch (AIAlert::Error const& error)
  {
    for (auto&& line : error.lines())
    {
      if (line.prepend_newline()) std::cerr << std::endl;
      std::cerr << translate::getString(line.getXmlDesc(), line.args());
    }
    std::cerr << '.' << std::endl;
  }

  graph.print_nodes();
}
