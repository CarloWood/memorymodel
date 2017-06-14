#include "sys.h"
#include "debug.h"
#include "cppmem_parser.h"
#include <boost/variant/get.hpp>
#include <iostream>
#include <string>
#include <fstream>

std::map<std::string, ast::function> functions;

void execute_body(std::string name, ast::body const& body)
{
  DoutEntering(dc::notice, "execute_body(\"" << name << "\")");
  for (auto& node : body.m_body_nodes)
  {
    switch (node.which())
    {
      case ast::BN_vardecl:
      {
        auto const& vd(boost::get<ast::vardecl>(node));
        Dout(dc::notice, "Found: " << vd);
        break;
      }
      case ast::BN_statement:
        break;
      case ast::BN_scope:
      {
        auto const& b(boost::get<ast::scope>(node).m_body);
        if (b)
          execute_body("scope", *b);
        break;
      }
      case ast::BN_threads:
      {
        auto const& t(boost::get<ast::threads>(node));
        for (auto& b : t.m_threads)
          execute_body("thread", b);
        break;
      }
    }
  }
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
  try
  {
    if (!cppmem::parse(filename, source_code, ast))
    {
      std::cerr << "Parse failure." << std::endl;
      return 1;
    }
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
      std::cout << "Global definition: " << vardecl << std::endl;
    }

  // Collect all function definitions.
  for (auto& node : ast)
    if (node.which() == ast::DN_function)
    {
      ast::function& function{boost::get<ast::function>(node)};
      //std::cout << "Function definition: " << function << std::endl;
      std::string name = function.m_function_name.name;
      functions[name] = function;
    }

  std::string const name = "test1";
  ast::function const& main_function = functions[name];

  // Execute main()
  execute_body(name, *main_function.m_scope.m_body);
}
