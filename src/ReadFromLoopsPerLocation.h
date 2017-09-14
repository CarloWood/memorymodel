#pragma once

#include "ReadFromLoop.h"
#include "Location.h"
#include "Action.h"
#include <vector>

class ReadFromLoopsPerLocation
{
 public:
  using read_from_loops_type = std::vector<ReadFromLoop>;

 private:
  Location const& m_location;
  read_from_loops_type m_read_from_loops;
  int m_previous_read_from_loop_index;

 public:
  ReadFromLoopsPerLocation(Location const& location) : m_location(location), m_previous_read_from_loop_index(-1) { }

  void add_read_action(Action* read_action)
  {
    read_action->set_read_from_loop_index(m_read_from_loops.size());
    m_read_from_loops.emplace_back(read_action);
  }

  ReadFromLoop const& get_read_from_loop_of(Action* read_action) const
  {
    return m_read_from_loops[read_action->get_read_from_loop_index()];
  }

  size_t number_of_read_actions() const { return m_read_from_loops.size(); }

  // Called when at the top of loop read_from_loop_index. Returns true when this loop was just entered.
  bool top_begin(int read_from_loop_index)
  {
    bool begin = m_previous_read_from_loop_index < read_from_loop_index;
    m_previous_read_from_loop_index = read_from_loop_index;
    return begin;
  }

  ReadFromLoop& operator[](int read_from_loop_index) { return m_read_from_loops[read_from_loop_index]; }
};


