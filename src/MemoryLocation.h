#pragma once

#include <string>

class MemoryLocation {
 private:
  std::string m_name;  // Unique, human readable, identifier for this memory location. Ie. "x", "y", etc...

 public:
  // Construct a new memory location with name `name'.
  MemoryLocation(std::string name);

  // Accessor.
  std::string const& name() const { return m_name; }

  // Less than comparable.
  friend bool operator<(MemoryLocation const& ml1, MemoryLocation const& ml2);
};
