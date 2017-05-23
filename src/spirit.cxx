#include "sys.h"
#include "debug.h"
#include "cppmem_parser.h"
#include <iostream>
#include <string>
#include <fstream>

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

  std::cout << "Result: " << ast << std::endl;
}
