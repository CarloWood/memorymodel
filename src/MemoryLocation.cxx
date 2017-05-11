#include "sys.h"
#include "MemoryLocation.h"
#include "Program.h"

MemoryLocation::MemoryLocation(std::string name) : m_name(name)
{
  Program::instance().inject(*this);    // Register this memory location with the program.
}

bool operator<(MemoryLocation const& ml1, MemoryLocation const& ml2)
{
  return ml1.m_name < ml2.m_name;
}
