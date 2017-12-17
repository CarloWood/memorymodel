#include "sys.h"
#include "Event.h"
#include "ReadFromGraph.h"

bool Event::is_relevant(ReadFromGraph const& read_from_graph) const
{
  switch (m_event)
  {
    case causal_loop:
      // A causal loop is impossible because everything in the loop either needs to be
      // Happens-Before or Read-From edges that are not Happens-Before but only for a
      // single memory location.
      // 
      // For example,
      //
      // Rrlx x (1)        (4)
      //       sb| ^        |sb
      //         v  \rf     v
      // Wrel y (2)------->(5) Racq y
      //         |    \     |
      //       sb|     \    |sb
      //         v      \   v
      //        (3)      `-(6) Wrlx x
      //
      // has a causal loop because the Read of x has to happen
      // before the Write to x (because of the rel/acq rf of y).
      //
      // m_end_point here is the vertex that we ran into while doing a depth-first search ((1) in the above case).
      return read_from_graph.is_followed(m_end_point);

    case reads_from:
      // A hidden visual side effect means that there is a Read-From that reads from
      // a write whose side effect is hidden by another write to the same memory
      // location (that happens after the first write and before the read).
      //
      // For example,
      //
      // Wrlx x (1)
      //       sb| \        |
      //         v  \rf     v
      // Wrlx x (2)  \     (5)
      //         |    .     |
      //         v    |     v
      // Wrel y (3)------->(6) Racq y
      //         |    |     |
      //         v    '     v
      //        (4)    `-->(7) Rrlx x
      //
      // has a hidden visual side effect because the write (2) is in between
      // (1) and (7) that reads from (1) while following a Happens-Before
      // path (1 -> 2 -> 3 -> 6 -> 7).

      // The reads_from event is relevant as long as we didn't return to the
      // parent node that it is reading from.
      return m_hidden || m_end_point != read_from_graph.current_node();
  }

  // Should never get here.
  ASSERT(false);
  return false;
}

std::ostream& operator<<(std::ostream& os, Event const& event)
{
  os << '{';
  if (event.m_event == causal_loop)
    os << "causal_loop";
  else
  {
    if (event.m_hidden)
      os << "reads_from_hidden";
    else
      os << "reads_from";
  }
  os << ';' << event.m_end_point;
  return os << '}';
}
