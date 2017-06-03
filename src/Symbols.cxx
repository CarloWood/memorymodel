#include "sys.h"
#include "Symbols.h"

namespace parser {

void Symbols::function(ast::function const& function)
{
  function_names.add(function.m_function_name.name, function.id);
}

void Symbols::vardecl(ast::memory_location const& memory_location)
{
  memory_locations.add(memory_location.m_name, memory_location.id);
}

void Symbols::regdecl(ast::register_location const& register_location)
{
  register_locations.add(std::string("r") + std::to_string(register_location.m_id), register_location.id);
}

void Symbols::scope(bool begin)
{
  if (begin)
  {
    m_symbol_stack.push(memory_locations);
    m_register_stack.push(register_locations);
  }
  else
  {
    memory_locations = m_symbol_stack.top();
    m_symbol_stack.pop();
    register_locations = m_register_stack.top();
    m_register_stack.pop();
  }
}

struct SymbolSearcher
{
  SymbolSearcher(int id, std::string& result) : m_id(id), m_found(result) { }

  void operator() (std::string const& s, int id)
  {
    if (id == m_id)
    {
      m_found = s;
    }
  }

private:
  int m_id;
  std::string& m_found;
};

std::string Symbols::id_to_string(int id)
{
  DoutEntering(dc::notice, "Symbols::id_to_string(" << id << ")");
  std::string found;
  SymbolSearcher symbol_searcher(id, found);
  memory_locations.for_each(symbol_searcher);
  return found;
}

namespace {

SingletonInstance<parser::Symbols> dummy __attribute__ ((__unused__));

}

} // namespace parser
