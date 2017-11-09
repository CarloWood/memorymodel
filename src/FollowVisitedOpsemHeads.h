#pragma once
#include "Edge.h"
#include "Action.h"
#include <set>

// This class follows sb and asw edges 'upstream' (bottom to top) while keeping track
// of the condition under which the Read node that we started from will see the
// side-effect of writing to its corresponding memory location if we would write
// to it and followed the path downwards (top to bottom).
//
// Consider the following cases where 'R' is the starting node where we read
// some memory location and W1, W2, etc are nodes that write to that same location.
// The question here is: what is the condition under which R will see W1?
//
//      O W1              O W1                  O W1                            O W1
//      |                 |                     |                               |
//      |                 |                     |                               |
//      ↓                 ↓                     ↓                               ↓
//      O-----.--...      O-----.               O-----.---------.               O-----.---------.---------.
//      |      \   _      |      \   _          |      \   _     \   _          |      \         \   _     \   _
//   1:A|       \2:A   1:A|       \2:A       1:A|       \2:A      \3:A       1:A|       \2:A      \3:A      \4:A
//      ↓        \        ↓        \            ↓        \         \            ↓        \         \         \        .
//      O-. _    |        O-. _     O-. _       O-. _     O-. _     O-. _       O-. _     O-. _     O-. _     O-. _
//      |  \B    |        |  \B     |  \C       |  \B     |  \C     |  \D       |  \B     |  \C     |  \D     |  \E
//      |B  O W2 |        |B  O W2  |C  O W3    |B  O W2  |C  O W3  |D  O W4    |B  O W2  |C  O W3  |D  O W4  |E  O W5
//      |  /     |        |  /      |  /        |  /      |  /      |  /        |  /      |  /      |  /      |  /
//      ↘ ↙      /        ↘ ↙       ↘ ↙         ↘ ↙       ↘ ↙       ↘ ↙         ↘ ↙       ↘ ↙       ↘ ↙       ↘ ↙
//     R O      /        R O←--------O         R O←--------'---------'         R O←--------'---------'---------'
//       |  ___/           |                     |                               |
//       | /               |                     |                               |
//       ↓↙                ↓                     ↓                               ↓
//       O                 O                     O                               O
//
//   case I            case II               case III                        case IV
//
// Case I:
//
// Here edge 1 is reached (bottom to top) only when condition B is true
// (because the W2 write stops following edges further upstream: that
// write would be the visible side-effect that R would see). This condition
// is called (below) the "path condition". Hence edge 1 is marked with
// path_condition 'B'.
//
// Condition 'A' is the edge-condition, the condition under which we
// will follow edge 1 downwards (instead of edge 2).
//
// In this case the branch of edge 2 ends after node R (the node
// at the other end of 2 is not 'sequenced_before' R). Therefore,
// if A is false then R doesn't exist and thus will not read from
// W1. The condition under which R reads from W1 is therefore: AB.
//
// Case II:
//
// In this case the branch of not-A (A') *does* end at R. We now have
// two possible paths: one that exists when B and is taken when A and
// one that exists when C and is taken when not A. Hence the
// condition under which R reads from W1 is: AB + A'C.
//
// Case III:
//
// Here edges 2 and 3 exist at the same time (when A is false); they
// are thread creations (asw edges). The condition under which R now
// will read from W1 is: AB + A'CD, because when either C or D is
// false then W1 would be overwritten by W2 and/or W3 (if A is false).
//
// Case IV:
//
// Likewise, in this case the condition under which R reads from W1
// is ABC + A'DE.
//
// The algorithm below calculates these formulas by multiplying the
// path conditions of all edges that have the same edge condition,
// as well as with that edge condition and adding the results together.
// However, only those edges are taken into account that lead to a node
// that is sequenced before the R node.
//

struct FollowVisitedOpsemHeads
{
  struct SequenceNumberCompare
  {
    bool operator()(Action* action1, Action* action2)
    {
      return action1->sequence_number() > action2->sequence_number();
    }
  };
  using queued_type = std::set<Action*, SequenceNumberCompare>;

 private:
  int m_visited_generation;
  queued_type m_queued;
  Action* m_read_node;

 public:
  FollowVisitedOpsemHeads(Action* read_node, int visited_generation) :
      m_visited_generation(visited_generation), m_queued{read_node}, m_read_node(read_node) { }

  void process_queued(std::function<bool(Action*, boolean::Expression&&)> const& if_found);

  // Should we follow the edge of this end_point, that is only reached when path_condition?
  // Returns true and adjusts path_condition if so.
  bool operator()(EndPoint const& end_point, boolean::Expression& path_condition);
};
