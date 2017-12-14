#include "sys.h"
#include "LoopEvent.h"
#include "ReadFromGraph.h"

bool LoopEvent::is_actual(ReadFromGraph const& read_from_graph) const
{
  switch (m_loop_event)
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

    case hidden_vse:
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
      return false; // FIXME
  }

  // Should never get here.
  ASSERT(false);
  return false;
}

std::ostream& operator<<(std::ostream& os, LoopEvent const& loop_event)
{
  os << '{';
  if (loop_event.m_loop_event == causal_loop)
    os << "causal_loop";
  else
    os << "hidden_vse";
  os << ';' << loop_event.m_end_point;
  return os << '}';
}
