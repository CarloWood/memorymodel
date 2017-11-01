#include "sys.h"
#include "ReadFromLocationSubgraphs.h"

void ReadFromLocationSubgraphs::add(DirectedSubgraph&& read_from_subgraph)
{
  m_subgraphs.emplace_back(std::move(read_from_subgraph));
}
