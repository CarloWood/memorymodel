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
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  register_locations.add(name, register_location.id);
}

int Symbols::set_register_id(ast::register_location const& register_location)
{
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  int* p = register_locations.find(name);
  // regdecl should already have been called.
  assert(p);
  register_location.id = *p;
  return *p;
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
  std::string found;
  SymbolSearcher symbol_searcher(id, found);
  memory_locations.for_each(symbol_searcher);
  if (!found.empty())
    return found;
  register_locations.for_each(symbol_searcher);
  if (!found.empty())
    return found;
  function_names.for_each(symbol_searcher);
  assert(!found.empty());
  return found;
}

namespace {

SingletonInstance<parser::Symbols> dummy __attribute__ ((__unused__));

}

} // namespace parser
