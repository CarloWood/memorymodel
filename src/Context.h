#pragma once

#include "position_handler.h"
#include <string>

using iterator_type = std::string::const_iterator;

class Graph;

struct Context {
  position_handler<iterator_type>& m_position_handler;
  Graph& m_graph;

  Context(position_handler<iterator_type>& ph, Graph& g) : m_position_handler(ph), m_graph(g) { }

  // Uninitialized declaration.
  void uninitialized(ast::tag decl);

  // Non-atomic read and writes.
  void read(ast::tag variable);
  void write(ast::tag variable);

  // Atomic read and writes.
  void read(ast::tag variable, std::memory_order mo);
  void write(ast::tag variable, std::memory_order mo);

  // Mutex declaration and (un)locking.
  void lockdecl(ast::tag mutex);
  void lock(ast::tag mutex);
  void unlock(ast::tag mutex);
};
