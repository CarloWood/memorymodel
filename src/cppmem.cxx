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
  std::vector<std::pair<std::string, ast::declaration_statement>> m_symbols;
  std::stack<int> m_stack;
 public:
  void add(ast::declaration_statement const& declaration_statement);
  void scope_start(bool is_thread);
  void scope_end();
  ast::declaration_statement const& find(std::string var_name) const;
};

void Symbols::add(ast::declaration_statement const& declaration_statement)
{
  m_symbols.push_back(std::make_pair(declaration_statement.name(), declaration_statement));
}

Symbols symbols;

void execute_body(std::string name, ast::statement_seq const& body, position_handler<iterator_type>& handler);

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
      auto const& tag(boost::get<ast::tag>(node));                                 // tag that the parser thinks we are using.
      Dout(dc::notice, "[read from '" << tag << "' " << handler.location(handler.id_to_pos(tag)) << "]");
      // Check if that variable wasn't masked by another variable of different type.
      std::string name = parser::Symbols::instance().tag_to_string(tag);           // Actual C++ object of that name in current scope.
      ast::declaration_statement const& declaration_statement{symbols.find(name)}; // Declaration of actual object.
      if (declaration_statement.tag() != tag)                                      // Did the parser make a mistake?
      {
        std::string position{handler.location(handler.id_to_pos(declaration_statement.tag()))};
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
      break;
    }
    case ast::SE_load_call:
    {
      auto const& load_call(boost::get<ast::load_call>(node));
      Dout(dc::notice, "[load from: " << handler.location(handler.id_to_pos(load_call.m_memory_location_id)) << "]");
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
#if 0
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
    case ast::SN_load_call:
    {
      auto const& load_call(boost::get<ast::load_call>(node));
      Dout(dc::notice, load_call << "; [load from: " << handler.location(handler.id_to_pos(load_call.m_memory_location_id)) << "]");
      break;
    }
    case ast::SN_store_call:
    {
      auto const& store_call(boost::get<ast::store_call>(node));
      Dout(dc::notice, store_call << "; [write to: " << handler.location(handler.id_to_pos(store_call.m_memory_location_id)) << "]");
      execute_expression(store_call.m_val, handler);
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
      if (function.m_compound_statement.m_body)
        execute_body(function.m_function_name.name, *function.m_compound_statement.m_body, handler);
      break;
    }
    case ast::SN_if_statement:
      break;
    case ast:: SN_while_statement:
      break;
  }
#endif
}

void execute_body(std::string name, ast::statement_seq const& body, position_handler<iterator_type>& handler)
{
#if 0
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
      case ast::BN_declaration_statement:
      {
        auto const& declaration_statement(boost::get<ast::declaration_statement>(node));
        Dout(dc::notice, declaration_statement << " [" << handler.location(handler.id_to_pos(declaration_statement.tag())) << "]");
        symbols.add(declaration_statement);
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
      case ast::BN_compound_statement:
      {
        auto const& body(boost::get<ast::compound_statement>(node).m_body);
        if (body)
          execute_body("compound_statement", *body, handler);
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
#endif
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
    if (node.which() == ast::DN_declaration_statement)
    {
      ast::declaration_statement& declaration_statement{boost::get<ast::declaration_statement>(node)};
      Dout(dc::notice, declaration_statement << " [" << handler.location(handler.id_to_pos(declaration_statement.tag())) << "]");
      symbols.add(declaration_statement);
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
    if (main_function->m_compound_statement.m_statement_seq)
      execute_body("main", *main_function->m_compound_statement.m_statement_seq, handler);
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
