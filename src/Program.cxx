#include "sys.h"
#include "Program.h"

static SingletonInstance<Program> dummy __attribute__ ((__unused__));

void Program::inject(MemoryLocation const& memory_location)
{
  auto res = m_locations.insert(memory_location);
  if (!res.second)
  {
    // Each MemoryLocation in the program must have an unique name.
    throw std::logic_error("MemoryLocation with duplicated name.");
  }
}
