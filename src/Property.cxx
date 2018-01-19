#include "sys.h"
#include "Property.h"
#include "ReadFromGraph.h"
#include "ReleaseSequence.h"
#include "Context.h"

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
      return read_from_graph.is_followed(m_end_point);

    case release_sequence:
      return !m_broken_release_sequence || m_not_synced_yet;

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
      !((m_type == reads_from && !m_hidden) ||
        m_type == release_sequence);
}

bool need_merging(Property const& p1, Property const& p2)
{
  return p1.m_end_point == p2.m_end_point && p1.m_type == p2.m_type && p1.m_hidden == p2.m_hidden;
}

void Property::wrap(Property const& property)
{
  ASSERT(m_type == release_sequence);
  m_pending.emplace_back(property, property.m_path_condition.copy());
}

// Convert property under operator 'propagator'.
// The path_condition was already updated when we get here.
// Return false when this property can be disregarded after this conversion.
bool Property::convert(Propagator const& propagator)
{
  DoutEntering(dc::property, "Property::convert(" << propagator << ") for *this = " << *this);
  if (m_type == causal_loop)
  {
    // Causal loop Property objects should be copied normally (return true from this function)
    // when they are propagated over a Racq<---Wrel edge.
    if (propagator.edge_is_rf() && !propagator.rf_rel_acq())
    {
      // A non-rel-acq Read-From edge is detected. Remember the memory location that
      // this happened for. If this happened before for a different memory location
      // then the causal loop is broken and this Property can be discarded (return false).
      if (!m_location.undefined() && m_location != propagator.current_location())
      {
        Dout(dc::property, "Returning false because causal loop follows non-rel-acq rf for second memory location.");
        return false;
      }
      Dout(dc::property, "Read-From edge is not rel-acq: setting m_location of causal_loop Property to " << propagator.current_location() << '.');
      m_location = propagator.current_location();
    }
  }
  else if (m_type == reads_from)
  {
    if (propagator.rf_acq_but_not_rel())        // Should we wrap all Property objects instead of copying them?
    {
      Dout(dc::property, "Returning false because reads_from should not be propagated over a non-rel-acq rf (but wrapped).");
      return false;
    }
    if (propagator.is_write() && m_location == propagator.current_location() && m_end_point != propagator.current_node())
    {
      Dout(dc::property, "The reads_from " << m_end_point << " Property fails because it is overwritten by node " << propagator.current_node() << '.');
      m_hidden = true;
    }
  }
  else if (m_type == release_sequence)
  {
    // Do not copy non-causal-loop Property objects in the case that the propagator is a Racq<---Wrlx because
    // in that case all Property objects are being copied by wrapping them in a release_sequence. However
    // make a exception for the release_sequence Property that is the Property that is doing the wrapping.
    if (m_location.undefined())         // Is this the release_sequence Proporty that is doing the wrapping?
    {
      // This should be the first time we get here, directly after construction of the Property.
      ASSERT(m_end_point == propagator.child());
      Dout(dc::property, "This is the release_sequence Property that is doing the wrapping. Set location (" << propagator.current_location() << ").");
      m_location = propagator.current_location();
      // We always get here directly after creating a release_sequence Property.
      // This records the thread of the possible to-be release sequence.
      Dout(dc::property, "Recording the current thread (" << propagator.current_thread() << ") as the thread that writes.");
      ASSERT(m_release_sequence_thread == -1);
      m_release_sequence_thread = propagator.current_thread();
      return true;
    }
    else if (propagator.rf_acq_but_not_rel())   // Should we wrap all Property objects instead of copying them?
    {
      Dout(dc::property, "Returning false because release_sequence should not be propagated over a non-rel-acq rf (but wrapped).");
      return false;
    }
    // m_location should always be defined when we get here.
    ASSERT(!m_location.undefined());
    // Once synced, no conversion is possible anymore (can we even get here for a synced release sequence Property?)
    if (m_not_synced_yet)
    {
      ASSERT(m_release_sequence_thread != -1);
      bool correct_thread = propagator.current_thread() == m_release_sequence_thread;
      // Synchronize when a write release is encountered (in the correct thread).
      // We also do this when the release_sequence was already broken (that flag isn't touched here).
      if (correct_thread &&
          propagator.is_write_rel_to(m_location))       // Also RMW.
      {
        Dout(dc::property, "Resetting m_not_synced_yet because " << propagator.current_action() << " is a Wrel in the correct thread to the correct memory location.");
        m_not_synced_yet = false;
      }
      else if (!m_broken_release_sequence &&            // No need to clear m_pending again.
               !correct_thread &&
               propagator.is_store_to(m_location))      // Not RMWs.
      {
        Dout(dc::property, "Setting m_broken_release_sequence because " << propagator.current_action() <<
            " is store to the correct memory location but in the wrong thread (" <<
            propagator.current_thread() << " != " << m_release_sequence_thread << ").");
        m_broken_release_sequence = true;
        m_pending.clear();    // Not needed anymore.
      }
    }
  }
  return true;
}

void Property::unwrap_to(Properties& properties, SequenceNumber rs_begin)
{
  DoutEntering(dc::property, "Property::unwrap_to(" << properties << ")");
  ReleaseSequence const& release_sequence{Context::instance().m_release_sequences.insert(rs_begin, m_rs_end)};
  Dout(dc::notice, "Adding properties from ReleaseSequence " << release_sequence);
  boolean::Expression path_condition(m_path_condition * release_sequence.boolexpr_variable());
  for (Property const& property : m_pending)
    properties.add(Property(property, property.m_path_condition.times(path_condition)));
  m_pending.clear();
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
void Property::merge_into(std::vector<Property>& map)
{
  DoutEntering(dc::property, "Property::merge_into(map) with *this = " << *this);

  if (m_type == release_sequence)
  {
    bool needs_release_sequence_merging = false;
    for (Property& property : map)
      if (need_merging(*this, property))
      {
        needs_release_sequence_merging = true;
        break;
      }
      else
        Dout(dc::property, "No merge required with " << property);
    if (!needs_release_sequence_merging)
    {
      Dout(dc::property, "No merging required; just adding " << *this);
      map.emplace_back(std::move(*this));
      return;
    }
  }
  else
  {
#ifdef CWDEBUG
    bool needs_merging = false;
#endif
    for (Property& property : map)
      if (need_merging(*this, property))
      {
        needs_merging = true;
        Dout(dc::property|continued_cf, "Merging " << *this << " into " << property << " ---> ");
        if (m_type == causal_loop && m_location.undefined() != property.m_location.undefined())
        {
          if (m_location.undefined())
            property.m_path_condition = property.m_path_condition.times(!m_path_condition.as_product());
          else
            m_path_condition = m_path_condition.times(!property.m_path_condition.as_product());
          Dout(dc::continued, property);
          break;
        }
        else
          property.m_path_condition += m_path_condition;
        Dout(dc::finish, property);
        return;
      }
#ifdef CWDEBUG
    if (!needs_merging)
      Dout(dc::property, "No merging required; just adding " << *this);
    else
      Dout(dc::finish, " + " << *this);
#endif
    map.emplace_back(std::move(*this));
    return;
  }

//PROPERTY  : Entering Property::merge_into(map) with *this = {rel_seq(not_synced_yet;broken;L6)[];#7;B}
//PROPERTY  :   Merging {rel_seq(not_synced_yet;broken;L6)[];#7;B} into
//              [ {rel_seq(not_synced_yet;T2;L6)[{causal_loop;L?;#7;1}];#7;1} ] --->
//              [ {rel_seq(not_synced_yet;broken;L6)[];#7;B} ].

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

  // merge_causal_loop:
  //
  // In the case of a causal loop a complex merge is needed where the state of m_not_synced_yet
  // and m_broken_release_sequence interfere with eachother.
  //
  // Let xy be one of {00, 01, 10, 11} where x = m_broken_release_sequence and y = m_not_synced_yet.
  // When two paths are merged where one or more is broken then the result is broken, so the first
  // bit needs to be OR-ed. While if two paths are merged where one or more are synced then the
  // result is synced, so the second bit needs to be AND-ed.
  //
  // The following table applies (don't worry if you don't understand why, this is not trival).
  //
  //    | Merging in under condition E                              |
  //    |    0      |    1                    |   2                 |  3   <-- internal_state
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

  Dout(dc::property|continued_cf, "Merging " << *this << " into [");
  for (Property& property : map)
  {
    if (need_merging(*this, property))
    {
      Dout(dc::continued, ' ' << property);
      switch (internal_state)
      {
        case 0:     // this is 10
          // 10 |  A + E
          // 00 |  B * !E
          // 11 |  C * !E
          // 01 |  D * !E
          if (property.m_broken_release_sequence && !property.m_not_synced_yet)
          {
            // property is 10 (with condition A).
            property.m_path_condition += m_path_condition;    // A + E.
            m_path_condition = false; // Prevent adding this object at the end of map.
          }
          else
          {
            // property is 00 (with condition B), 11 (with condition C) or 01 (with condition D).
            property.m_path_condition = property.m_path_condition.times(inverse_E);   // B * !E, C * !E, D * !E.
            if (property.m_path_condition.is_zero())
              Pzero[zero_count++] = &property;                          // Remember properties that need to be removed.
          }
          break;
        case 1:     // this is 00
          // 10 |  A + C * E
          // 00 |  B + E * !(A + C)
          // 11 |  C * !E
          // 01 |  D * !E
          if (property.m_broken_release_sequence)
          {
            // property is 10 (with condition A) or 11 (with condition C).
            sum += property.m_path_condition;                           // Collect A + C.
            if (property.m_not_synced_yet)
            {
              // property is 11 (with condition C).
              XE = property.m_path_condition.times(m_path_condition);   // Collect C * E.
            }
            else
            {
              // property is 10 (with condition A).
              P10 = &property;                                          // Remember the property that needs XE to be added (to become A + C * E).
            }
          }
          if (property.m_not_synced_yet)
          {
            // property is 11 (with condition C) or 01 (with condition D).
            property.m_path_condition = property.m_path_condition.times(inverse_E);   // C * !E, D * !E.
            if (property.m_path_condition.is_zero())
              Pzero[zero_count++] = &property;                          // Remember properties that need to be removed.
          }
          else if (!property.m_broken_release_sequence)
          {
            // property is 00 (with condition B).
            Psum = &property;                                           // Remember the property that needs E * !sum to be added (to become B + E * !(A + C)).
          }
          break;
        case 2:     // this is 11
          // 10 |  A + B * E
          // 00 |  B * !E
          // 11 |  C + E * !(A + B)
          // 01 |  D * !E
          if (!property.m_not_synced_yet)
          {
            // property is 10 (with condition A) or 00 (with condition B).
            sum += property.m_path_condition;                           // Collect A + B.
            if (!property.m_broken_release_sequence)
            {
              // property is 00 (with condition B).
              XE = property.m_path_condition.times(m_path_condition);   // Collect B * E.
            }
            else
            {
              // property 10 (with condition A).
              P10 = &property;                                          // Remember the property that needs XE to be added (to become A + B * E).
            }
          }
          if (!property.m_broken_release_sequence)
          {
            // property is 00 (with condition B) or 01 (with condition D).
            property.m_path_condition = property.m_path_condition.times(inverse_E);   // B * !E, D * !E.
            if (property.m_path_condition.is_zero())
              Pzero[zero_count++] = &property;                          // Remember properties that need to be removed.
          }
          else if (property.m_not_synced_yet)
          {
            // property is 11 (with condition C).
            Psum = &property;                                           // Remember the property that needs E * !sum to be added (to become C + E * !(A + B)).
          }
          break;
        case 3:     // this is 01
          // 10 | A
          // 00 | B
          // 11 | C
          // 01 | D + E * !(A + B + C)
          if (property.m_broken_release_sequence || !property.m_not_synced_yet)
          {
            // property is 10 (with condition A) or 00 (with condition B) or 11 (with condition C).
            sum += property.m_path_condition;               // Collect A + B + C.
          }
          else
          {
            // property is 01 (with condition D).
            Psum = &property;                                           // Remember the property that needs E * !sum to be added (to become D + E * !(A + B + C)).
          }
          break;
      }
    }
  }
  Dout(dc::continued, "] ---> [");

  if (Psum)
    Psum->m_path_condition += m_path_condition.times(sum.inverse());
  else if (internal_state > 0)  // Should there be a Psum?
    m_path_condition = m_path_condition.times(sum.inverse());           // We need to add a new property to the map with path condition E * !sum.
  if (P10 && XE.is_initialized())
    P10->m_path_condition += XE;

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

  // Add this object to the map if needed.
  if (!Psum && !m_path_condition.is_zero())
  {
    if (Pback != Pend)
      *Pback++ = std::move(*this);
    else
      map.emplace_back(std::move(*this));
  }

  // If there was no P10 found but there should have been then a 10 property with path condition XE needs to be added.
  if (!P10 && 0 < internal_state && internal_state < 3 && XE.is_initialized() && !XE.is_zero())
  {
    m_broken_release_sequence =true;
    m_not_synced_yet = false;
    if (Pback != Pend)
      *Pback++ = Property(*this, std::move(XE));
    else
      map.emplace_back(*this, std::move(XE));
  }

  // Erase deleted properties from the map.
  if (Pback != Pend)
    map.erase(Pback, Pend);

#ifdef CWDEBUG
  for (Property& property : map)
    if (need_merging(*this, property))
      Dout(dc::continued, ' ' << property);
  Dout(dc::finish, " ].");
#endif
}

#if 0
bool broken(int i)
{
  return !(i & 1);
}

bool not_synced(int i)
{
  return (i & 2);
}

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  boolean::Product const A(boolean::Context::instance().create_variable("A"));
  boolean::Product const B(boolean::Context::instance().create_variable("B"));
  boolean::Product const C(boolean::Context::instance().create_variable("C"));
  boolean::Product const D(boolean::Context::instance().create_variable("D"));
  boolean::Product const E(boolean::Context::instance().create_variable("E"));

  SequenceNumber const s1((size_t)1);
  SequenceNumber const s2((size_t)2);

  for (int j = 0; j < 5; ++j)
    for (int i1 = 0; i1 <= 3; ++i1)
    {
      Property pE(not_synced(i1), broken(i1), s1, s2, boolean::Expression(E));
      std::vector<Property> v;
      Property pA(not_synced(0), broken(0), s1, s2, boolean::Expression(A));
      Property pB(not_synced(1), broken(1), s1, s2, boolean::Expression(B));
      Property pC(not_synced(2), broken(2), s1, s2, boolean::Expression(C));
      Property pD(not_synced(3), broken(3), s1, s2, boolean::Expression(D));
      if (j > 0)
      {
        Dout(dc::property, "Leaving out " << char('A' - 1 + j));
      }
      if (j != 1)
        v.emplace_back(std::move(pA));
      if (j != 2)
        v.emplace_back(std::move(pB));
      if (j != 3)
        v.emplace_back(std::move(pC));
      if (j != 4)
        v.emplace_back(std::move(pD));
      pE.merge_into(v);
    }
}
#endif

std::ostream& operator<<(std::ostream& os, Property const& property)
{
  os << '{';
  if (property.m_type == causal_loop)
  {
    os << "causal_loop;" << property.m_location;
  }
  else if (property.m_type == release_sequence)
  {
    os << "rel_seq";
    if (property.m_not_synced_yet || property.m_broken_release_sequence)
    {
      os << '(';
      if (property.m_not_synced_yet)
        os << "not_synced_yet;";
      if (property.m_broken_release_sequence)
        os << "broken";
      else if (property.m_release_sequence_thread == -1)
        os << "T?";
      else
        os << 'T' << property.m_release_sequence_thread;
      if (!property.m_location.undefined())
        os << ";" << property.m_location;
      os << ")[";
      for (auto&& prop : property.m_pending)
        os << prop;
      os << ']';
    }
    else
      ASSERT(property.m_pending.empty());
  }
  else
  {
    os << "reads_from;" << property.m_location;
    if (property.m_hidden)
      os << "(hidden)";
  }

  os << ';' << property.m_end_point;
  os << ';' << property.m_path_condition;
  return os << '}';
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct property("PROPERTY");
NAMESPACE_DEBUG_CHANNELS_END
#endif
