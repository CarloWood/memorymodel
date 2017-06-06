#include "sys.h"
#include "debug.h"
#include "Symbols.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct symbols("SYMBOLS");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace parser {

void Symbols::function(ast::function const& function)
{
  DoutEntering(dc::symbols, "Symbols::function(\"" << function << "\") with id " << function.id);
  function_names.add(function.m_function_name.name, function);
  function_names_map[function.id] = function.m_function_name.name;
  all_symbols[function.id] = function.m_function_name.name;
}

void Symbols::vardecl(ast::memory_location const& memory_location)
{
  DoutEntering(dc::symbols, "Symbols::vardecl(\"" << memory_location << "\"" <<
      (memory_location.m_type == ast::type_atomic_int ? "[atomic]" : "[non-atomic]") << ") with id " << memory_location.id);
  if (memory_location.m_type == ast::type_atomic_int)
  {
    atomic_memory_locations.add(memory_location.m_name, memory_location);
    atomic_memory_locations_map[memory_location.id] = memory_location.m_name;
  }
  else
  {
    na_memory_locations.add(memory_location.m_name, memory_location);
    na_memory_locations_map[memory_location.id] = memory_location.m_name;
  }
  all_symbols[memory_location.id] = memory_location.m_name;
}

void Symbols::regdecl(ast::register_location const& register_location)
{
  DoutEntering(dc::symbols, "Symbols::regdecl(\"" << register_location << "\") with id " << register_location.id);
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  register_locations.add(name, register_location);
  register_locations_map[register_location.id] = name;
  all_symbols[register_location.id] = name;
}

ast::tag Symbols::set_register_id(ast::register_location const& register_location)
{
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  ast::tag* p = register_locations.find(name);
  // regdecl should already have been called.
  assert(p);
  assert(register_location.id == p->id);
  return *p;
}

void Symbols::scope(int begin)
{
  DoutEntering(dc::symbols, "Symbols::scope(" << begin << ")");
  if (begin != 1)
  {
    DoutEntering(dc::symbols, "POPPING FROM STACK (begin = " << begin << "); register_locations is now");
    print(register_locations);

    na_memory_locations_map = m_na_symbol_stack.top();
    atomic_memory_locations_map = m_symbol_stack.top();
    register_locations_map = m_register_stack.top();

    na_memory_locations.clear();
    atomic_memory_locations.clear();
    register_locations.clear();

    for (auto& naml : na_memory_locations_map)
      na_memory_locations.add(naml.second.c_str(), ast::tag(naml.first));
    for (auto& aml : atomic_memory_locations_map)
      atomic_memory_locations.add(aml.second.c_str(), ast::tag(aml.first));
    for (auto& rl : register_locations_map)
      register_locations.add(rl.second.c_str(), ast::tag(rl.first));

    m_na_symbol_stack.pop();
    m_symbol_stack.pop();
    m_register_stack.pop();

    Dout(dc::symbols, "Popped from stack:");
    print(register_locations);
  }
  if (begin != 0)
  {
    DoutEntering(dc::symbols, "PUSHING TO STACK (begin = " << begin << "); register_locations is now");
    print(register_locations);
    m_na_symbol_stack.push(na_memory_locations_map);
    m_symbol_stack.push(atomic_memory_locations_map);
    m_register_stack.push(register_locations_map);
  }
}

void Symbols::reset()
{
  function_names.clear();
  function_names_map.clear();
  na_memory_locations.clear();
  na_memory_locations_map.clear();
  atomic_memory_locations.clear();
  atomic_memory_locations_map.clear();
  register_locations.clear();
  register_locations_map.clear();
  while (!m_na_symbol_stack.empty())
    m_na_symbol_stack.pop();
  while (!m_symbol_stack.empty())
    m_symbol_stack.pop();
  while (!m_register_stack.empty())
    m_register_stack.pop();
}

std::string Symbols::tag_to_string(ast::tag id)
{
  return all_symbols[id.id];
}

struct SymbolPrinter
{
  void operator() (std::string const& s, ast::tag id)
  {
    Dout(dc::symbols, "\"" << s << "\" : " << id.id);
  }
};

void Symbols::print(boost::spirit::qi::symbols<char, ast::tag>& table) const
{
  SymbolPrinter symbol_printer;
  table.for_each(symbol_printer);
}

namespace {

SingletonInstance<parser::Symbols> dummy __attribute__ ((__unused__));

}

} // namespace parser
