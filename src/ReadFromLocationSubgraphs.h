#pragma once

#include "DirectedSubgraph.h"
#include <vector>

class Location;

class ReadFromLocationSubgraphs
{
 public:
  using subgraphs_type = std::vector<DirectedSubgraph>;
  using iterator = subgraphs_type::iterator;
  using const_iterator = subgraphs_type::const_iterator;

 private:
  Location const& m_location;   // The memory location that this object contains Read-From edge subgraphs for.
  subgraphs_type m_subgraphs;   // Possible Read-From edge subgraphs for this location.

 public:
  ReadFromLocationSubgraphs(Location const& location) : m_location(location) { }

  void add(DirectedSubgraph&& read_from_subgraph);
  size_t size() const { return m_subgraphs.size(); }

  // Accessor.
  Location const& location() const { return m_location; }
  DirectedSubgraph const& operator[](int index) const { return m_subgraphs[index]; }

  iterator begin() { return m_subgraphs.begin(); }
  const_iterator begin() const { return m_subgraphs.begin(); }
  iterator end() { return m_subgraphs.end(); }
  const_iterator end() const { return m_subgraphs.end(); }
};
