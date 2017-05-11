#pragma once

#include "MemoryLocation.h"
#include "utils/Singleton.h"
#include <set>
#include <string>

class Program : public Singleton<Program> {
  friend_Instance;
 private:
  Program() { }
  ~Program() { }
  Program(Program const&) = delete;

 public:
  using locations_type = std::set<MemoryLocation>;

  void inject(MemoryLocation const& memory_location);

 private:
  locations_type m_locations;   // The set of all memory locations in the program, by name.
};
