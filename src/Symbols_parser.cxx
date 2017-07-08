#include "sys.h"
#include "debug.h"
#include "Symbols_parser.h"
#include "SymbolsImpl_parser.h"

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

void Symbols::mutex_decl(ast::mutex_decl const& mutex_decl)
{
  DoutEntering(dc::symbols, "Symbols::mutex_decl(\"" << mutex_decl << "\") with id " << mutex_decl.id);
  m_impl->mutexes.add(mutex_decl.m_name, mutex_decl);
  m_impl->mutexes_map[mutex_decl.id] = mutex_decl.m_name;
  m_impl->all_symbols[mutex_decl.id] = mutex_decl.m_name;
}

void Symbols::condition_variable_decl(ast::condition_variable_decl const& condition_variable_decl)
{
  DoutEntering(dc::symbols, "Symbols::condition_variable_decl(\"" << condition_variable_decl << "\") with id " << condition_variable_decl.id);
  m_impl->condition_variables.add(condition_variable_decl.m_name, condition_variable_decl);
  m_impl->condition_variables_map[condition_variable_decl.id] = condition_variable_decl.m_name;
  m_impl->all_symbols[condition_variable_decl.id] = condition_variable_decl.m_name;
}

void Symbols::unique_lock_decl(ast::unique_lock_decl const& unique_lock_decl)
{
  DoutEntering(dc::symbols, "Symbols::unique_lock_decl(\"" << unique_lock_decl << "\") with id " << unique_lock_decl.id);
  m_impl->unique_locks.add(unique_lock_decl.m_name, unique_lock_decl);
  m_impl->unique_locks_map[unique_lock_decl.id] = unique_lock_decl.m_name;
  m_impl->all_symbols[unique_lock_decl.id] = unique_lock_decl.m_name;
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

bool Symbols::is_register(ast::tag tag)
{
  std::string name = m_impl->all_symbols[tag.id];
  bool result = false;
  bool first = true;
  for (char c : name)
  {
    if (first)
    {
      if (c != 'r')
        return false;
      first = false;
      continue;
    }
    if (std::isdigit(c))
    {
      result = true;
      continue;
    }
    else
      return false;
  }
  return result;
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
    m_impl->mutexes_map = m_impl->m_mutex_stack.top();
    m_impl->condition_variables_map = m_impl->m_condition_variable_stack.top();
    m_impl->unique_locks_map = m_impl->m_unique_lock_stack.top();

    m_impl->na_memory_locations.clear();
    m_impl->atomic_memory_locations.clear();
    m_impl->register_locations.clear();
    m_impl->mutexes.clear();
    m_impl->condition_variables.clear();
    m_impl->unique_locks.clear();

    for (auto& naml : m_impl->na_memory_locations_map)
      m_impl->na_memory_locations.add(naml.second.c_str(), ast::tag(naml.first));
    for (auto& aml : m_impl->atomic_memory_locations_map)
      m_impl->atomic_memory_locations.add(aml.second.c_str(), ast::tag(aml.first));
    for (auto& rl : m_impl->register_locations_map)
      m_impl->register_locations.add(rl.second.c_str(), ast::tag(rl.first));
    for (auto& md : m_impl->mutexes_map)
      m_impl->mutexes.add(md.second.c_str(), ast::tag(md.first));
    for (auto& cv : m_impl->condition_variables_map)
      m_impl->condition_variables.add(cv.second.c_str(), ast::tag(cv.first));
    for (auto& ul : m_impl->unique_locks_map)
      m_impl->unique_locks.add(ul.second.c_str(), ast::tag(ul.first));

    m_impl->m_na_symbol_stack.pop();
    m_impl->m_symbol_stack.pop();
    m_impl->m_register_stack.pop();
    m_impl->m_mutex_stack.pop();
    m_impl->m_condition_variable_stack.pop();
    m_impl->m_unique_lock_stack.pop();

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
    m_impl->m_mutex_stack.push(m_impl->mutexes_map);
    m_impl->m_condition_variable_stack.push(m_impl->condition_variables_map);
    m_impl->m_unique_lock_stack.push(m_impl->unique_locks_map);
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
  m_impl->mutexes.clear();
  m_impl->mutexes_map.clear();
  m_impl->condition_variables.clear();
  m_impl->condition_variables_map.clear();
  m_impl->unique_locks.clear();
  m_impl->unique_locks_map.clear();
  while (!m_impl->m_na_symbol_stack.empty())
    m_impl->m_na_symbol_stack.pop();
  while (!m_impl->m_symbol_stack.empty())
    m_impl->m_symbol_stack.pop();
  while (!m_impl->m_register_stack.empty())
    m_impl->m_register_stack.pop();
  while (!m_impl->m_mutex_stack.empty())
    m_impl->m_mutex_stack.pop();
  while (!m_impl->m_condition_variable_stack.empty())
    m_impl->m_condition_variable_stack.pop();
  while (!m_impl->m_unique_lock_stack.empty())
    m_impl->m_unique_lock_stack.pop();
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
