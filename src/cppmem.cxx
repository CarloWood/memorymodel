#include "sys.h"
#include "debug.h"
#include "ast.h"
#include "position_handler.h"
#include "cppmem_parser.h"
#include "utils/AIAlert.h"
#include <boost/variant/get.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <stack>

struct TagCompare { bool operator()(ast::tag const& t1, ast::tag const& t2) const { return t1.id < t2.id; } };
std::map<ast::tag, ast::function, TagCompare> functions;
using iterator_type = std::string::const_iterator;

class Symbols {
 private:
  std::vector<std::pair<std::string, ast::vardecl>> m_symbols;
  std::stack<int> m_stack;
 public:
  void add(ast::vardecl const& vardecl);
  void scope_start(bool is_thread);
  void scope_end();
  ast::vardecl const& find(std::string var_name) const;
};

void Symbols::add(ast::vardecl const& vardecl)
{
  m_symbols.push_back(std::make_pair(vardecl.m_memory_location.m_name, vardecl));
}

Symbols symbols;

void execute_body(std::string name, ast::body const& body, position_handler<iterator_type>& handler);

void Symbols::scope_start(bool is_thread)
{
  if (is_thread)
    Dout(dc::notice, "New thread:");
  Dout(dc::notice, "{");
  Debug(libcw_do.inc_indent(2));
  m_stack.push(m_symbols.size());
}

void Symbols::scope_end()
{
  Debug(libcw_do.dec_indent(2));
  Dout(dc::notice, "}");
  m_symbols.resize(m_stack.top());
  m_stack.pop();
}

ast::vardecl const& Symbols::find(std::string var_name) const
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

void execute_expression(ast::expression const& expression, position_handler<iterator_type>& handler);

void execute_simple_expression(ast::simple_expression const& simple_expression, position_handler<iterator_type>& handler)
{
  auto const& node = simple_expression.m_simple_expression_node;
  switch (node.which())
  {
    case ast::SE_int:
      //auto const& literal(boost::get<int>(node));
      break;
    case ast::SE_tag:
    {
      auto const& tag(boost::get<ast::tag>(node));
      Dout(dc::notice, "[read from '" << tag << "' " << handler.location(handler.id_to_pos(tag)) << "]");
      // Check if that variable wasn't masked by an atomic int.
      std::string name = parser::Symbols::instance().tag_to_string(tag);
      ast::vardecl const& vardecl{symbols.find(name)};
      if (vardecl.m_memory_location != tag)
      {
        std::string position{handler.location(handler.id_to_pos(vardecl.m_memory_location))};
        THROW_ALERT("Please use [MEMORY_LOCATION].load() for atomic_int declared at [POSITION] instead of [MEMORY_LOCATION]",
                    AIArgs("[MEMORY_LOCATION]", vardecl.m_memory_location)("[POSITION]", position));
      }
      break;
    }
    case ast::SE_load_statement:
    {
      auto const& load_statement(boost::get<ast::load_statement>(node));
      Dout(dc::notice, "[load from: " << handler.location(handler.id_to_pos(load_statement.m_memory_location_id)) << "]");
      break;
    }
    case ast::SE_expression:
    {
      auto const& expression(boost::get<ast::expression>(node));
      execute_expression(expression, handler);
      break;
    }
  }
}

void execute_expression(ast::expression const& expression, position_handler<iterator_type>& handler)
{
#ifdef CWDEBUG
  debug::Mark marker('!');
#endif
  ast::unary_expression const& unary_expression(expression.m_operand);
  execute_simple_expression(unary_expression.m_simple_expression, handler);
  for (auto& chain : expression.m_chained)
  {
    execute_expression(chain.operand, handler);
  }
}

void execute_statement(ast::statement const& statement, position_handler<iterator_type>& handler)
{
  auto const& node = statement.m_statement;
  switch (node.which())
  {
    case ast::SN_assignment:
    {
      auto const& assignment(boost::get<ast::assignment>(node));
      if (!parser::Symbols::instance().is_register(assignment.lhs))
        Dout(dc::notice, assignment << "; [write to: " << handler.location(handler.id_to_pos(assignment.lhs)) << "]");
      else
        Dout(dc::notice, assignment << ";");
      execute_expression(assignment.rhs, handler);
      break;
    }
    case ast::SN_load_statement:
    {
      auto const& load_statement(boost::get<ast::load_statement>(node));
      Dout(dc::notice, load_statement << "; [load from: " << handler.location(handler.id_to_pos(load_statement.m_memory_location_id)) << "]");
      break;
    }
    case ast::SN_store_statement:
    {
      auto const& store_statement(boost::get<ast::store_statement>(node));
      Dout(dc::notice, store_statement << "; [write to: " << handler.location(handler.id_to_pos(store_statement.m_memory_location_id)) << "]");
      execute_expression(store_statement.m_val, handler);
      break;
    }
    case ast::SN_function_call:
    {
      auto const& function_call(boost::get<ast::function_call>(node));
#ifdef CWDEBUG
      Dout(dc::notice, function_call.m_function << "();");
      debug::Mark mark;
#endif
      auto const& function(functions[function_call.m_function]);
      if (function.m_scope.m_body)
        execute_body(function.m_function_name.name, *function.m_scope.m_body, handler);
      break;
    }
    case ast::SN_if_statement:
      break;
    case ast:: SN_while_statement:
      break;
  }
}

void execute_body(std::string name, ast::body const& body, position_handler<iterator_type>& handler)
{
#ifdef CWDEBUG
  if (name != "scope" && name != "thread")
  {
    if (name == "main")
      Dout(dc::notice, "int " << name << "()");
    else
      Dout(dc::notice, "void " << name << "()");
  }
#endif
  symbols.scope_start(name == "thread");
  for (auto& node : body.m_body_nodes)
  {
    switch (node.which())
    {
      case ast::BN_vardecl:
      {
        auto const& vardecl(boost::get<ast::vardecl>(node));
        Dout(dc::notice, vardecl << " [" << handler.location(handler.id_to_pos(vardecl.m_memory_location)) << "]");
        symbols.add(vardecl);
        break;
      }
      case ast::BN_statement:
      {
        auto const& statement(boost::get<ast::statement>(node));
        try
        {
          execute_statement(statement, handler);
        }
        catch (AIAlert::Error const& alert)
        {
          THROW_ALERT(alert, " in `[STATEMENT]`", AIArgs("[STATEMENT]", statement.m_statement));
        }
        break;
      }
      case ast::BN_scope:
      {
        auto const& b(boost::get<ast::scope>(node).m_body);
        if (b)
          execute_body("scope", *b, handler);
        break;
      }
      case ast::BN_threads:
      {
        auto const& t(boost::get<ast::threads>(node));
        for (auto& b : t.m_threads)
          execute_body("thread", b, handler);
        break;
      }
    }
  }
  symbols.scope_end();
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
  position_handler<iterator_type> handler(filename, begin, end);
  try
  {
    if (!cppmem::parse(begin, end, handler, ast))
      return 1;
  }
  catch (std::exception const& error)
  {
    Dout(dc::warning, "Parser threw exception: " << error.what());
  }

  std::cout << "Abstract Syntax Tree: " << ast << std::endl;

  // Collect all global variables and their initialization, if any.
  for (auto& node : ast)
    if (node.which() == ast::DN_vardecl)
    {
      ast::vardecl& vardecl{boost::get<ast::vardecl>(node)};
      Dout(dc::notice, vardecl << " [" << handler.location(handler.id_to_pos(vardecl.m_memory_location)) << "]");
      symbols.add(vardecl);
    }

  // Collect all function definitions.
  ast::function const* main_function;
  for (auto& node : ast)
    if (node.which() == ast::DN_function)
    {
      ast::function& function{boost::get<ast::function>(node)};
      functions[function] = function;
      //std::cout << "Function definition: " << function << std::endl;
      if (function.m_function_name.name == "main")
        main_function = &functions[function];
    }

  try
  {
    // Execute main()
    execute_body("main", *main_function->m_scope.m_body, handler);
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
}
