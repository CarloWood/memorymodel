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

  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <test file>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios_base::binary | std::ios_base::ate);      // Open file with file position at the end.

  if (!file.is_open())
  {
    std::cerr << "Failed to open file \"" << argv[1] << "\".\n";
    return 1;
  }

  // Allocate space and read contents of file into memory.
  std::streampos size = file.tellg();
  std::string input(size, '\0');
  file.seekg(0, std::ios::beg);
  file.read(&input.at(0), size);
  file.close();

  AST::cppmem result;
  try {
    if (!cppmem::parse(argv[1], input, result))
    {
      std::cerr << "Parsing failed." << std::endl;
      return 1;
    }
  } catch (std::exception const& e) {
    Dout(dc::warning, "Parser threw exception: " << e.what());
  }

  std::cout << "Result: " << result << std::endl;
}
