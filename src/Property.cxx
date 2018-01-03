#include "sys.h"
#include "Property.h"
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

// The reads_from Property object changes internal state while propagating
// back from the Read node to corresponding Write node (the end_point).
// In the above example, the Property object is created at #7 and then
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
// Reaching node #1 always discards the Property object; a "hidden vse" detection
// only occurs when the state of that Property object is "read_from_hidden #1" at
// that moment, otherwise nothing happens.
//
// The Property object is also discarded when we are in the state "possible_rs #1 y thread"
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
// (7) Creation of the Property object in state "read_from #1".
// (6) Nothing happens because we didn't follow the rf yet. The state stays at "read_from #1".
// (6)-->(5) Backtracking the Read-From, ending in a Racq y, causes the state to become "possible_rs #1 y".
// (5) The Wrlx y in thread1 causes the state to change to "possible_rs #1 y thread1".
// (4) The Wrlx x has no effect because we're not in state "read_from #1".
// (3) Due to the 'Wrel y', also in thread1, we go back to state "read_from #1".
// (2) The (RM)W* x in a node different from #1 causes us to go to the state "read_from_hidden #1".
// (1) We reach #1, so a hidden vse is detected and the Property object is discarded.
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
// and copy the Property object and backtrack changes the state
// as follows:
//
// (C)-->(A) Backtracking the Read-From, ending in a Racq z, causes the state to become "possible_rs #1 z".
// (A) Due to the 'Wrel z' we go back to state "possible_rs #1 y thread1".
// (9) Because this is a W* y on a different thread, the state becomes "broken_rs #1 y".
// (8)-->(B) Backtracking the Read-From, ending in a Racq z, causes the state to become "possible_rs #1 z".
// (B) Due to the 'Wrel z' we go back to state "broken_rs #1 y" and then combine it with
//     the copy we already had in the state "possible_rs #1 y thread1", resulting in
//     the state "broken_rs #1 y".
// (3) The Property object is discarded because it is in state "broken_rs #1 y".
//
// The result is that no hidden vse is detected for the path containing the Wrlx y (9) (in the case
// that would be a conditional path, in this example no hidden vse is detected period).
//
bool Property::is_relevant(ReadFromGraph const& read_from_graph) const
{
  switch (m_type)
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
      return read_from_graph.is_followed(m_end_point) &&
             !(m_broken_release_sequence && !m_not_synced_yet); // State F is not relevant.

    case reads_from:
      // The reads_from property is relevant as long as we didn't return to the
      // parent node that it is reading from.
      return m_hidden || m_end_point != read_from_graph.current_node();
  }

  // Should never get here.
  ASSERT(false);
  return false;
}

// Return true if the current property invalidates the graph.
bool Property::invalidates_graph(ReadFromGraph const* read_from_graph) const
{
  return m_end_point == read_from_graph->current_node() &&
      (m_type != reads_from || m_hidden) &&
      (m_type != causal_loop || !m_not_synced_yet);
}

property_merge_type need_merging(Property const& p1, Property const& p2)
{
  return (p1.m_end_point == p2.m_end_point &&
          p1.m_type == p2.m_type &&
          p1.m_rs_end == p2.m_rs_end &&
          p1.m_hidden == p2.m_hidden) ? (p1.m_type == causal_loop && p1.m_rs_end == p2.m_rs_end) ? merge_causal_loop
                                                                                                 : merge_simple
                                      : merge_none;
}

// Convert property under operator 'propagator'.
// The path_condition was already updated when we get here.
// Return false when this property can be disregarded after this conversion.
//
// causal_loop:
//
// The causal_loop Property has the following states:                     m_not_synced_yet  m_broken_rs  m_thread       Attributes
// A) Creation from (ReadModify)Write release                           : false             false                       end_point
// B) Creation from ReadModifyWrite non-release to *location*           : true              false        -1             end_point, location, rs_end
// C) Creation from Write non-release to *location*                     : true              false        <value>        end_point, location, rs_end, thread
//    or
//    Inbetween Write release and its creation point (B or C) with
//    (mutual exclusive) conditions under which a non-release Write
//    happened in some *Thread*.
// E) Broken Release Sequence, where two or more different threads      : true              true         -1             end_point, location
//    wrote to its associated memory location.
// F) Broken Release Sequence that hit a Write release.                 : false             true         -1             end_point
//
// The Property object is always created in state A, after which convert
// is called to convert to state B or C.
//
bool Property::convert(Propagator const& propagator)
{
  if (m_type == causal_loop)
  {
    if (!m_not_synced_yet)
    {
      // State A.
      if (propagator.rf_acq_but_not_rel())
      {
        // A --> B
        ASSERT(!m_broken_release_sequence);
        ASSERT(m_not_synced_thread == -1);      // Should not come here for state F.
        m_not_synced_yet = true;
        m_not_synced_location = propagator.current_location();
      }
    }
    if (m_not_synced_yet)
    {
      if (m_broken_release_sequence)
      {
        // State E.
        ASSERT(m_not_synced_thread == -1);
        if (propagator.is_write_rel_to(m_not_synced_location))
        {
          // E -> F
          m_not_synced_yet = false;
        }
      }
      // State B or C.
      else if (propagator.is_write_rel_to(m_not_synced_location))       // Also RMW.
      {
        // B or C --> A
        m_not_synced_yet = false;
        m_not_synced_thread = -1;
      }
      else if (propagator.is_non_rel_write(m_not_synced_location))      // Not RMW.
      {
        if (m_not_synced_thread == -1)
        {
          // B --> C
          m_not_synced_thread = propagator.current_thread();
        }
        else if (m_not_synced_thread != propagator.current_thread())
        {
          // C --> E
          m_broken_release_sequence = true;
          m_not_synced_thread = -1;
        }
      }
    }
  }
  return true;
}

// A new Property has been discovered. It should be added to map.
//
// merge_simple:
//
// It is possible that the property already exists (possibly with a different
// path condition), in which case the condition must be OR-ed with the existing
// condition for that property because in that case we're dealing with an
// alternative path.
//
// For example,
//
//   0       3←.
//   |       | |
//   ↓  rf:B ↓ |
//   1←------4 |
//   |↖      | |
//   | \rf:A ↓ |
//   |  `----5 |rf
//   ↓         |
//   2---------'
//
// Let node 1 be a Read that reads the Write at 4 under condition B and from
// the Write at 5 under the condition A (which can only happen when AB = 0).
// Then the algorithm when doing a depth-first search starting at 0 could
// visit 0-1-2-3-4-5 and then discover the Property "causal loop ending at 1",
// which subsequently will end up in the Properties of node 4 (with condition A).
// At that point it would follow the edge from 4 to 1 and rediscover the
// same Property "causal loop ending at 1" under condition B. From the point
// of view of node 4, the Property "cause loop ending at 1" now has condition
// A + B.
//
// merge_causal_loop:
//
// However, in the case of a causal loop a complex merge is needed where
// the state of m_not_synced_yet and m_broken_release_sequence interfere
// with eachother.
//
// Let xy be one of {00, 01, 10, 11} where x = m_broken_release_sequence
// and y = m_not_synced_yet. When two paths are merged where one or more
// is broken then the result is broken, so the first bit needs to be
// OR-ed. While if two paths are merged where one or more are synced
// then the result is synced, so the second bit needs to be AND-ed.
//
// Hence *) the following table applies.
//
//    | Merging in under condition E                              |
//    |   10      |   00                    |  11                 | 01
//--------------------------------------------------------------------------------------
// 10 |  A + E    |  A + C * E              |  A + B * E          | A
// 00 |  B * !E   |  B + E * !(A + C)       |  B * !E             | B
// 11 |  C * !E   |  C * !E                 |  C + E * !(A + B)   | C
// 01 |  D * !E   |  D * !E                 |  D * !E             | D + E * !(A + B + C)
//
//  ^   ^
//  |   `-- Resulting conditions after merge.
//  `------ Original conditions respectively A, B, C and D.
//
// *) It took me four days to figure that out.
//
void Property::merge_into(std::vector<Property>& map)
{
  int internal_state = (m_broken_release_sequence ? 0 : 1) + (m_not_synced_yet ? 2 : 0);
  boolean::Expression inverse_E;
  if (internal_state != 3)    // We don't need !E for the case 01.
    inverse_E = m_path_condition.inverse();
  boolean::Expression sum{false};
  boolean::Expression XE;     // Either C * E or B * E for the case 00 and 11 respectively.
  Property* P10 = nullptr;
  Property* Psum = nullptr;
  int zero_count = 0;
  Property* Pzero[3];
  for (Property& property : map)
  {
    switch (need_merging(*this, property))
    {
      case merge_none:
        break;
      case merge_simple:
        property.m_path_condition += m_path_condition;
        return;
      case merge_causal_loop:
      {
        switch (internal_state)
        {
          case 0:     // this is 10
            if (property.m_broken_release_sequence && !property.m_not_synced_yet)
            {
              // property is 10 (with condition A).
              property.m_path_condition += m_path_condition;    // A + E.
              m_path_condition = false; // Prevent adding this object at the end of map.
            }
            else
            {
              property.m_path_condition = property.m_path_condition.times(inverse_E);   // B * !E, C * !E, D * !E.
              Pzero[zero_count++] = &property;
            }
            break;
          case 1:     // this is 00
            if (property.m_broken_release_sequence)
            {
              // property is 10 (with condition A) or 11 (with condition C).
              sum += property.path_condition();               // Collect A + C.
              if (property.m_not_synced_yet)                  // property is 11 (with condition C).
                XE = property.path_condition().times(m_path_condition); // C * E.
              else
                P10 = &property;
            }
            if (!property.m_not_synced_yet)
            {
              property.m_path_condition = property.m_path_condition.times(inverse_E);   // C * !E, D * !E.
              Pzero[zero_count++] = &property;
              if (!property.m_broken_release_sequence)
                Psum = &property;
            }
            break;
          case 2:     // this is 11
            if (!property.m_not_synced_yet)
            {
              // property is 10 (with condition A) or 00 (with condition B).
              sum += property.path_condition();               // Collect A + B
              if (!property.m_broken_release_sequence)        // property is 00 (with condition B).
                XE = property.path_condition().times(m_path_condition); // B * E.
              else
                P10 = &property;
            }
            if (!property.m_broken_release_sequence)
            {
              property.m_path_condition = property.m_path_condition.times(inverse_E);   // B * !E, D * !E.
              Pzero[zero_count++] = &property;
            }
            else if (property.m_not_synced_yet)
              Psum = &property;
            break;
          case 3:     // this is 01
            if (property.m_broken_release_sequence || !property.m_not_synced_yet)
              sum += property.path_condition();               // Collect A + B + C.
            else
              Psum = &property;
            break;
        }
        break;
      }
    }
  }
  auto Pend = map.end();
  auto Plast = Pend - 1;
  // m_map index: 0   1   2   3   4   5   6
  //              A   0   B   0   0   0   C           zero_count == 4
  //                                      ^
  //                                      |
  //                                     Plast
  int z_last = zero_count - 1;
  //                                  ^
  //                                  |
  //                                z_last
  for (int z_begin = 0; z_begin <= z_last; ++z_begin)
  {
    // First iteration:
    //            A   0   B   0   0   0   C
    //                ^
    //                |
    //             z_begin
    while (z_begin <= z_last && &*Plast == Pzero[z_last])
    {
      --z_last;
      --Plast;
    }
    if (z_begin <= z_last)
    {
      *Pzero[z_begin] = std::move(*Plast);
      --Plast;
      // First iteration:
      //          A   C   B   0   0   0   m
      //                              ^
      //                              |
      //                             Plast
    }
    // Second iteration:
    //            A   C   B   0   0   0   m
    //                        ^       ^
    //                        |       |
    //                     z_begin  z_last
    // And then the while loop gobbles up everything until z_last < z_begin:
    //            A   C   B   0   0   0   m
    //                    ^   ^
    //                    |   |
    //                    |z_begin=1
    //                    |
    //                  Plast
  }
  auto Pback = Plast + 1;
  if (internal_state > 0)
  {
    if (!Psum)
      m_path_condition = m_path_condition.times(sum.inverse());
    else
      Psum->m_path_condition += m_path_condition.times(sum.inverse());
    if (internal_state < 3)
    {
      if (P10)
        P10->m_path_condition += XE;
      else if (XE.is_initialized() && !XE.is_zero())
      {
        if (Pback != Pend)
          *Pback++ = Property(*this, std::move(XE));
        else
          map.emplace_back(*this, std::move(XE));
      }
    }
  }
  if (!Psum && !m_path_condition.is_zero())
  {
    if (Pback != Pend)
      *Pback++ = std::move(*this);
    else
      map.emplace_back(std::move(*this));
  }
  if (Pback != Pend)
    map.erase(Pback, Pend);
}

std::ostream& operator<<(std::ostream& os, Property const& property)
{
  os << '{';
  if (property.m_type == causal_loop)
  {
    os << "causal_loop";

    if (property.m_not_synced_yet || property.m_broken_release_sequence)
    {
      os << '(';
      if (property.m_not_synced_yet)
        os << "not_synced_yet;";
      if (property.m_broken_release_sequence)
        os << "broken";
      else if (property.m_not_synced_thread == -1)
        os << "T?";
      else
        os << 'T' << property.m_not_synced_thread;
      os << ")";
    }
  }
  else
  {
    os << "reads_from";

    if (property.m_hidden)
      os << "(hidden)";
  }

  os << ';' << property.m_end_point;
  os << ';' << property.m_path_condition;
  return os << '}';
}
