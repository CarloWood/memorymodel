#include "sys.h"
#include "debug.h"
#include "Symbols.h"
#include "SymbolsImpl.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct symbols("SYMBOLS");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace parser {

Symbols::Symbols() : m_impl(new SymbolsImpl)
{
}

Symbols::~Symbols()
{
  delete m_impl;
}

void Symbols::function(ast::function const& function)
{
  DoutEntering(dc::symbols, "Symbols::function(\"" << function << "\") with id " << function.id);
  m_impl->function_names.add(function.m_function_name.name, function);
  m_impl->function_names_map[function.id] = function.m_function_name.name;
  m_impl->all_symbols[function.id] = function.m_function_name.name;
}

void Symbols::vardecl(ast::memory_location const& memory_location)
{
  DoutEntering(dc::symbols, "Symbols::vardecl(\"" << memory_location << "\"" <<
      (memory_location.m_type == ast::type_atomic_int ? "[atomic]" : "[non-atomic]") << ") with id " << memory_location.id);
  if (memory_location.m_type == ast::type_atomic_int)
  {
    m_impl->atomic_memory_locations.add(memory_location.m_name, memory_location);
    m_impl->atomic_memory_locations_map[memory_location.id] = memory_location.m_name;
  }
  else
  {
    m_impl->na_memory_locations.add(memory_location.m_name, memory_location);
    m_impl->na_memory_locations_map[memory_location.id] = memory_location.m_name;
  }
  m_impl->all_symbols[memory_location.id] = memory_location.m_name;
}

void Symbols::regdecl(ast::register_location const& register_location)
{
  DoutEntering(dc::symbols, "Symbols::regdecl(\"" << register_location << "\") with id " << register_location.id);
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  m_impl->register_locations.add(name, register_location);
  m_impl->register_locations_map[register_location.id] = name;
  m_impl->all_symbols[register_location.id] = name;
}

ast::tag Symbols::set_register_id(ast::register_location const& register_location)
{
  std::string name = "r";
  name += std::to_string(register_location.m_id);
  ast::tag* p = m_impl->register_locations.find(name);
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
    DoutEntering(dc::symbols, "POPPING FROM STACK (begin = " << begin << "); m_impl->register_locations is now");
    m_impl->print(m_impl->register_locations);

    m_impl->na_memory_locations_map = m_impl->m_na_symbol_stack.top();
    m_impl->atomic_memory_locations_map = m_impl->m_symbol_stack.top();
    m_impl->register_locations_map = m_impl->m_register_stack.top();

    m_impl->na_memory_locations.clear();
    m_impl->atomic_memory_locations.clear();
    m_impl->register_locations.clear();

    for (auto& naml : m_impl->na_memory_locations_map)
      m_impl->na_memory_locations.add(naml.second.c_str(), ast::tag(naml.first));
    for (auto& aml : m_impl->atomic_memory_locations_map)
      m_impl->atomic_memory_locations.add(aml.second.c_str(), ast::tag(aml.first));
    for (auto& rl : m_impl->register_locations_map)
      m_impl->register_locations.add(rl.second.c_str(), ast::tag(rl.first));

    m_impl->m_na_symbol_stack.pop();
    m_impl->m_symbol_stack.pop();
    m_impl->m_register_stack.pop();

    Dout(dc::symbols, "Popped from stack:");
    m_impl->print(m_impl->register_locations);
  }
  if (begin != 0)
  {
    DoutEntering(dc::symbols, "PUSHING TO STACK (begin = " << begin << "); m_impl->register_locations is now");
    m_impl->print(m_impl->register_locations);
    m_impl->m_na_symbol_stack.push(m_impl->na_memory_locations_map);
    m_impl->m_symbol_stack.push(m_impl->atomic_memory_locations_map);
    m_impl->m_register_stack.push(m_impl->register_locations_map);
  }
}

void Symbols::reset()
{
  m_impl->function_names.clear();
  m_impl->function_names_map.clear();
  m_impl->na_memory_locations.clear();
  m_impl->na_memory_locations_map.clear();
  m_impl->atomic_memory_locations.clear();
  m_impl->atomic_memory_locations_map.clear();
  m_impl->register_locations.clear();
  m_impl->register_locations_map.clear();
  while (!m_impl->m_na_symbol_stack.empty())
    m_impl->m_na_symbol_stack.pop();
  while (!m_impl->m_symbol_stack.empty())
    m_impl->m_symbol_stack.pop();
  while (!m_impl->m_register_stack.empty())
    m_impl->m_register_stack.pop();
}

std::string Symbols::tag_to_string(ast::tag id)
{
  return m_impl->all_symbols[id.id];
}

struct SymbolPrinter
{
  void operator() (std::string const& s, ast::tag id)
  {
    Dout(dc::symbols, "\"" << s << "\" : " << id.id);
  }
};

void SymbolsImpl::print(boost::spirit::qi::symbols<char, ast::tag>& table) const
{
  SymbolPrinter symbol_printer;
  table.for_each(symbol_printer);
}

namespace {

SingletonInstance<parser::Symbols> dummy __attribute__ ((__unused__));

}

} // namespace parser
