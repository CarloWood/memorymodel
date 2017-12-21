#include "sys.h"
#include "Quirk.h"
#include "ReadFromGraph.h"

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
//         v    | rf  v
// Wrel y (3)------->(6) Racq y
//         |    |     |
//         v    '     v
//        (4)    `-->(7) Rrlx x
//
// has a hidden visual side effect because the write #2 is in between
// #1 and #7 that reads from #1 while following a Happens-Before
// path (1 -> 2 -> 3 -> 6 -> 7).

// The reads_from Quirk object changes internal state while propagating
// back from the Read node to corresponding Write node (the end_point).
// In the above example, the Quirk object is created at #7 and then
// propagated backwards through 7 -> 6 -> 3 -> 2 -> 1.
//
// The initial state is "read_from #1" (A node reads from the write
// of node #1). This state means that the memory location that node #1
// writes to will be read when following a Happens-Before path down
// the graph, without encountering a Write to that memory location.
//
// If a Write does happen along that path before reaching the Read,
// the then state should be "read_from_hidden #1". In the graph above
// that would be the state between #1 and #2, because the Write at
// #2 will happen before we reach the Read at #7.
//
// There are two more states: "possible_rs #1 y" with and without
// knowing the thread that the Wrel to y is in.
//
// While propagating back the state should change according to this
// diagram (where node #1 is a write to x):
//                                           Wx!#1                           Wx#1
//              ________"read_from #1"____ --------> "read_from_hidden #1" -------> Hidden vse detection.
//              |  ^                 ^
//              |  |                 |
//     rf/Racq y|  |Wrel y     Wrel y|(same thread)
//              |  |                 |
//              v  '     Wrlx y      '                                     <-- The Wrlx y may not be a RMWrlx (those do not cause a state transition).
//   "possible_rs #1 y" --------> "possible_rs #1 y thread"
//                                       |
//                                   W* y|(diff thread)                    <-- The W* y can be a write of any memory order,
//                                       |                                     but note that again a RMW does not cause a
//                                       v                                     state transition.
//                                "broken_rs #1 y"
//                                       |
//                                       | Wrel y
//                                       v
//                                    discard
//
// Reaching node #1 always discards the Quirk object; a "hidden vse" detection
// only occurs when the state of that Quirk object is "read_from_hidden #1" at
// that moment, otherwise nothing happens.
//
// The Quirk object is also discarded when we are in the state "possible_rs #1 y thread"
// and we encounter a 'Wrel y' on a thread that is different than the stored thread.
//
// For example, in an execution represented by the following graph,
//
// Wrlx x (1)
//       sb| \
//         v  \rf
// Wrlx x (2)  \
//         |    .
//         v    |
// Wrel y (3)   |
//         |    |
//         v    |
// Wrlx x (4)   |
//         |    |     |
//         v    |     v
// Wrlx y (5)--rf--->(6) Racq y
//         |    |     |
//         v    '     v
//               `-->(7) Rrlx x
//
//      Thread     Thread
//         1          2
//
// the state transitions would be:
//
// (7) Creation of the Quirk object in state "read_from #1".
// (6) Nothing happens because we didn't follow the rf yet. The state stays at "read_from #1".
// (6)-->(5) Backtracking the Read-From, ending in a Racq y, causes the state to become "possible_rs #1 y".
// (5) The Wrlx y in thread1 causes the state to change to "possible_rs #1 y thread1".
// (4) The Wrlx x has no effect because we're not in state "read_from #1".
// (3) Due to the 'Wrel y', also in thread1, we go back to state "read_from #1".
// (2) The (RM)W* x in a node different from #1 causes us to go to the state "read_from_hidden #1".
// (1) We reach #1, so a hidden vse is detected and the Quirk object is discarded.
//
//
// Now consider the following execution:
//
//             Wrlx x (1)
//                   sb| \
//                     v  \rf
//             Wrlx x (2)  \
//                     |    .
//                     v    |
//             Wrel y (3)   |
//          |          |    |
//          v          v    |
//  Racq z (8)<---rf--(B) Wrel z
//          |          |    |
//          |          |    |
//  Wrlx y (9)         |    |
//          |          |    |
//          v          v    |
//  Wrel z (A)--rf--->(C) Racq z
//          |          |    |
//          v          v    |
//             Wrlx x (4)   |
//                     |    |     |
//                     v    |     v
//             Wrlx y (5)--rf--->(6) Racq y
//                     |    |     |
//                     v    '     v
//                           `-->(7) Rrlx x
//            
//       Thread     Thread     Thread
//          3          1          2
// At first the Depth-First-Search algorithm will follow
// the path (1 -> 2 -> 3 -> B -> C -> 4 -> 5 -> 6 -> 7)
// and then backtrack to B, after which the state will
// be as above "possible_rs #1 y thread1", both on nodes
// #B as well as #C.
//
// Then the algorithm will follow the path (B -> 8 -> 9 -> A -> C)
// and copy the Quirk object and backtrack changes the state
// as follows:
//
// (C)-->(A) Backtracking the Read-From, ending in a Racq z, causes the state to become "possible_rs #1 z".
// (A) Due to the 'Wrel z' we go back to state "possible_rs #1 y thread1".
// (9) Because this is a W* y on a different thread, the state becomes "broken_rs #1 y".
// (8)-->(B) Backtracking the Read-From, ending in a Racq z, causes the state to become "possible_rs #1 z".
// (B) Due to the 'Wrel z' we go back to state "broken_rs #1 y" and then combine it with
//     the copy we already had in the state "possible_rs #1 y thread1", resulting in
//     the state "broken_rs #1 y".
// (3) The Quirk object is discarded because it is in state "broken_rs #1 y".
//
// The result is that no hidden vse is detected for the path containing the Wrlx y (9) (in the case
// that would be a conditional path, in this example no hidden vse is detected period).
//
bool Quirk::is_relevant(ReadFromGraph const& read_from_graph) const
{
  switch (m_quirk)
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

      // The reads_from quirk is relevant as long as we didn't return to the
      // parent node that it is reading from.
      return m_hidden || m_end_point != read_from_graph.current_node();
  }

  // Should never get here.
  ASSERT(false);
  return false;
}

std::ostream& operator<<(std::ostream& os, Quirk const& quirk)
{
  os << '{';
  if (quirk.m_quirk == causal_loop)
    os << "causal_loop";
  else
  {
    if (quirk.m_hidden)
      os << "reads_from_hidden";
    else
      os << "reads_from";
  }
  os << ';' << quirk.m_end_point;
  return os << '}';
}
