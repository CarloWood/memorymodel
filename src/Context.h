#pragma once

#include "position_handler.h"
#include <string>

using iterator_type = std::string::const_iterator;

class Graph;

struct Context {
  position_handler<iterator_type>& m_position_handler;
  Graph& m_graph;

  Context(position_handler<iterator_type>& ph, Graph& g) : m_position_handler(ph), m_graph(g) { }
};

