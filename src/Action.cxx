#include "sys.h"
#include "Action.h"
#include "Context.h"
#include "FollowOpsemTails.h"
#include "FilterAllActions.h"
#include "Graph.h"
#include "utils/macros.h"
#include "iomanip_dotfile.h"

//static
char const* Action::action_kind_str(Kind kind)
{
  switch (kind)
  {
    case lock:
      return "lock";
    case unlock:
      return "unlock";
    case atomic_load:
      return "atomic_load";
    case atomic_store:
      return "atomic_store";
    case atomic_rmw:
      return "atomic_rmw";
    case non_atomic_read:
      return "na_read";
    case non_atomic_write:
      return "na_write";
    case fence:
      return "fence";
  }
  return "UNKNOWN Action::Kind";
}

void Action::print_node_label_on(std::ostream& os) const
{
  os << name() << ':' << type() << ' ' << tag();
  print_code(os);
}

std::ostream& operator<<(std::ostream& os, Action const& action)
{
  if (IOManipDotFile::is_dot_file(os))
    action.print_node_label_on(os);
  else
    os << "{node:" << action.name() << ", thread:" << action.m_thread->id() << ", location:" << *action.m_location << ", kind:" << action.kind() << '}';
  return os;
}

Action::Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable) : m_id(next_node_id), m_thread(thread), m_exists(true), m_sequence_number(0)
{
#ifdef CWDEBUG
  locations_type const& locations{Context::instance().locations()};
  m_location = std::find_if(locations.begin(), locations.end(), [variable](Location const& loc) { return loc == variable; });
  ASSERT(m_location != locations.end());
#endif
}

void Action::add_end_point(Edge* edge, EndPointType type, Action* other_node, bool edge_owner)
{
  //DoutEntering(*dc::edge[edge->edge_type()], "Action::add_end_point(" << *edge << ", " << type << ", " << *other_node << ", " << edge_owner << ") [this = " << *this << "]");
  m_end_points.emplace_back(edge, type, other_node, edge_owner);
  // Only test this for opsem edges (and even then it appears never to happen).
  if (edge->is_opsem())
  {
    end_points_type::iterator begin = m_end_points.begin();
    end_points_type::iterator last = m_end_points.end();
    end_points_type::iterator iter = --last;      // Point to just added element.
    while (iter != begin)
      if (AI_UNLIKELY(*last == *--iter))          // Is added end point equal to already existing one?
      {
        // End point already existed.
        m_end_points.pop_back();
        ASSERT(false); // FIXME: does this ever happen?
      }
  }
  // End point did not already exist.
}

// A new incoming sequenced-before or additionally-synchronizes-with edge was added.
void Action::update_exists()
{
  DoutEntering(dc::sb_edge, "Action::update_exists() [this = " << *this << "]");
  boolean::Expression node_exists(0);  // When does this node exist?
  int node_heads = 0;
  for (auto&& end_point : m_end_points)
  {
    if ((end_point.edge_type() == edge_sb || end_point.edge_type() == edge_asw) && end_point.type() == head)
    {
      ++node_heads;
      node_exists += end_point.edge()->exists();
      Dout(dc::sb_edge, "Found on this node (" << *this << ") an edge (from " << *end_point.other_node() <<
          ") which exists when " << end_point.edge()->exists() <<
          "; node_exists of this node is now " << node_exists);
    }
  }
  Dout(dc::sb_edge, "Setting m_exists to " << node_exists);
  m_exists = std::move(node_exists);
  for (auto&& end_point : m_end_points)
    if (end_point.edge_type() == edge_sb && end_point.type() == tail)
      end_point.other_node()->update_exists();
}

void Action::add_edge_to(EdgeType edge_type, Action* head_node, boolean::Expression&& condition)
{
  DoutEntering(*dc::edge[edge_type], "Action::add_edge_to(" << edge_type << ", " << *head_node << ", " << condition << ") [this = " << *this << "]");
  Edge* new_edge = new Edge(edge_type, this, std::move(condition));
  bool directed = EdgeMaskType{edge_type}.is_directed();
  // Call tail first!
  add_end_point(new_edge, directed ? tail : undirected, head_node, false);
  // For the sake of memory management, this EndPoint owns the allocated new_edge; so pass 'true'.
  head_node->add_end_point(new_edge, directed ? head : undirected, this, true);
  Dout(*dc::edge[edge_type], "ADDED EDGE " << *new_edge);
  if (edge_type == edge_sb || edge_type == edge_asw)
    head_node->update_exists();
}

void Action::delete_edge_to(EdgeType edge_type, Action* head_node)
{
  ASSERT(!(edge_type == edge_sb || edge_type == edge_asw));
  m_end_points.erase(
      std::remove_if(
        m_end_points.begin(),
        m_end_points.end(),
        [edge_type, head_node](EndPoint const& end_point)
        {
          return end_point.other_node() == head_node && end_point.edge_type() == edge_type;
        }
      ),
      m_end_points.end());
  head_node->m_end_points.erase(
      std::remove_if(
        head_node->m_end_points.begin(),
        head_node->m_end_points.end(),
        [edge_type, this](EndPoint const& end_point)
        {
          return end_point.other_node() == this && end_point.edge_type() == edge_type;
        }
      ),
      head_node->m_end_points.end());
}

//static
boolean::Expression const Action::s_expression_one(true);
boolean::Product const Action::s_product_one(true);

boolean::Expression const& Action::provides_sequenced_before_value_computation() const
{
  if (provided_type().type() == NodeProvidedType::side_effect)
    return m_connected.provides_sequenced_before_value_computation();
  return s_expression_one;
}

boolean::Expression const& Action::provides_sequenced_before_side_effect() const
{
  if (provided_type().type() == NodeProvidedType::value_computation)
    return m_connected.provides_sequenced_before_side_effect();
  return s_expression_one;
}

// Called on the tail-node of a new (conditional) sb edge.
void Action::sequenced_before()
{
  using namespace boolean;
  DoutEntering(dc::sb_edge, "sequenced_before() [this = " << *this << "]");

  boolean::Expression sequenced_before_value_computation(0);
  boolean::Expression sequenced_before_side_effect(0);
  // Run over all outgoing (tail end_points) Sequenced-Before edges.
  for (auto&& end_point : m_end_points)
  {
    if (end_point.edge_type() == edge_sb && end_point.type() == tail)
    {
      Dout(dc::sb_edge, "Found tail EndPoint " << end_point << " with condition '" << end_point.edge()->condition() << "'.");
      // Get condition of this edge.
      boolean::Product edge_conditional(end_point.edge()->condition().as_product());
      // Get the provides boolean expressions from the other node and AND them with the condition of that edge.
      // OR everything.
      sequenced_before_value_computation += end_point.other_node()->provides_sequenced_before_value_computation() * edge_conditional;
      sequenced_before_side_effect += end_point.other_node()->provides_sequenced_before_side_effect() * edge_conditional;
    }
    else
      Dout(dc::sb_edge, "Skipping EndPoint " << end_point << '.');
  }
  Dout(dc::sb_edge, "Result:");
  DebugMarkDownRight;
  Dout(dc::sb_edge, "sequenced_before_value_computation = '" << sequenced_before_value_computation << "'.");
  Dout(dc::sb_edge, "sequenced_before_side_effect = '" << sequenced_before_side_effect << "'.");

  // We don't support volatile memory accesses... otherwise a node could be a side_effect and value_computation at the same time :/
  bool node_provides_side_effect_not_value_computation = provided_type().type() == NodeProvidedType::side_effect;
  bool sequenced_before_value_computation_changed =
      m_connected.update_sequenced_before_value_computation(!node_provides_side_effect_not_value_computation, std::move(sequenced_before_value_computation));
  bool sequenced_before_side_effect_changed =
      m_connected.update_sequenced_before_side_effect(node_provides_side_effect_not_value_computation, std::move(sequenced_before_side_effect));

  if (sequenced_before_value_computation_changed || sequenced_before_side_effect_changed)
  {
    // Propagate bits to nodes sequenced before us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == head)
        end_point.other_node()->sequenced_before();
  }
}

bool Action::provides_sequenced_after_something() const
{
  bool result = true;
  if (provided_type().type() == NodeProvidedType::side_effect)
    result = m_connected.provides_sequenced_after_something();
  return result;
}

// Called on the head-node of a new (conditional) sb edge.
void Action::sequenced_after()
{
  DoutEntering(dc::sb_edge, "sequenced_after() [this = " << *this << "]");
  m_connected.set_sequenced_after_something();
}

// This is a value-computation node that no longer should be marked as value-computation
// head, because it is sequenced before a side-effect that must be sequenced before
// it's value computation (which then becomes the new value-computation head).
void Action::sequenced_before_side_effect_sequenced_before_value_computation()
{
  Dout(dc::sb_edge, "Marking " << *this << " as sequenced before a (pseudo) value computation.");
  // Passing one() as boolean expression because we are unconditionally sequenced before
  // a related side-effect that is unconditionally sequenced before the value computation
  // of the expression that this node is currently a tail of.
  //
  // For example, the expression ++x has a Read node of x sequenced before
  // the Write node that writes to x:
  //
  //     Rna x=
  //       |
  //       v
  //     Wna x=
  //       |
  //       v
  //     Pseudo value computation Node
  //
  // Both edges have no condition of their own, and we are not linked to
  // any other node yet, so their condition is 1.
  //
  // Bottom line, we NEVER want to add a tail to the "Rna x=".
  m_connected.update_sequenced_before_value_computation(true, boolean::Expression{true});
#ifdef CWDEBUG
  for (auto&& end_point : m_end_points)
    if (end_point.edge_type() == edge_sb && end_point.type() == head)
    {
      Dout(dc::debug, "Node " << *this << " has head end_point " << end_point << '!');
      // If the found head isn't already marked as being sequenced before a value computation
      // then we need to update it! I don't expect this to ever happen.
      ASSERT(end_point.other_node()->m_connected.provides_sequenced_before_value_computation().is_one());
    }
#endif
}

// This is the write node of a prefix operator that read from read_node,
// or the write node of an assignment expression whose value computation
// is used.
void Action::sequenced_before_value_computation()
{
  Dout(dc::sb_edge, "Marking " << *this << " as sequenced before its value computation.");
  m_connected.set_sequenced_before_pseudo_value_computation();
}

bool Action::matches(NodeRequestedType const& requested_type, boolean::Expression& hiding) const
{
  if (requested_type.any())
    return true;

  DoutEntering(dc::sb_edge, "matches(" << requested_type << ") [this = " << *this << "]");
  Dout(dc::sb_edge, "provided_type() = " << provided_type() << "; m_connected = " << m_connected);

  // Is itself the requested_type type (value-computation or side-effect)?
  bool is_requested_type =
      m_connected.sequenced_before_pseudo_value_computation() ||        // The write node of a pre-increment/decrement expression
      requested_type.matches(provided_type());                          //   fakes being both, value-computation and side-effect.

  if (!is_requested_type)
  {
    Dout(dc::sb_edge, "rejected because we ourselves are not the requested_type evaluation type.");
    return false;
  }

  hiding = m_connected.hiding_behind_another(requested_type);
  if (hiding.is_one())
  {
    Dout(dc::sb_edge, "rejected because we are hiding behind another node that provides the requested_type type.");
    return false;
  }

  return true;
}

//static
void Action::initialize_post_opsem(Graph const& graph, std::vector<Action*>& topological_ordered_actions)
{
  DoutEntering(dc::notice, "Action::initialize_post_opsem(...)");
  // Number all actions in a smart way.
  int sequence_number = 0;
  FollowOpsemTails follow_opsem_tails;
  FilterAllActions all_actions;
  graph.for_actions_no_condition(
      follow_opsem_tails,
      all_actions,
      [&sequence_number, &topological_ordered_actions](Action* action)
      {
        DoutEntering(dc::for_action, "if_found(" << *action << ")");
        // This happens for if() statements without else; ie
        // if (A) x=1; y=1;
        // will lead to:
        //    a
        //    |\ A
        //  _ | \          where if the edge to b is followed first will cause
        //  A |  b Wna x   the sequence number on c to be set before the edge
        //    | /          from a to c is followed.
        //    |/
        //    v
        //    c Wna y
        if (action->m_sequence_number != 0)
          return true;
        for (auto&& end_point : action->m_end_points)
          if (end_point.type() == tail && end_point.edge()->is_opsem())
          {
            end_point.other_node()->m_prior_actions.add(action->m_prior_actions);
            end_point.other_node()->m_prior_actions.add(*action);
          }
        for (auto&& end_point : action->m_end_points)
          if (end_point.type() == head && end_point.edge()->is_opsem())
          {
            if (end_point.other_node()->m_sequence_number == 0)
            {
              Dout(dc::for_action, "Returning true because " << *end_point.other_node() << " still has sequence number of 0.");
              return true;
            }
          }
        Dout(dc::for_action, "Setting sequence number on " << *action);
        ASSERT(action->m_sequence_number == 0);
        action->m_sequence_number = ++sequence_number;
        topological_ordered_actions.push_back(action);
        Dout(dc::for_action, "Returning false");
        return false;
      }
  );
}

bool Action::is_fully_visited(int visited_generation, Action* read_node) const
{
  for (auto&& end_point : m_end_points)
    if (end_point.type() == tail && end_point.edge()->is_opsem())
    {
      if (end_point.other_node() != read_node && !end_point.other_node()->is_sequenced_before(*read_node))
      {
        Dout(dc::visited, "The edge to " << end_point.other_node()->name() <<
            " is skipped because that node is not sequenced before the Read node " << read_node->name() << '.');
        continue;
      }
      if (!end_point.edge()->is_visited(visited_generation))
      {
        Dout(dc::visited, "The edge to " << end_point.other_node()->name() << " wasn't visited yet.");
        return false;
      }
    }
  return true;
}

boolean::Expression Action::calculate_path_condition(int visited_generation, Action* read_node) const
{
  DoutEntering(dc::visited, "Action::calculate_path_condition('" << read_node->name() << "') [this = " << *this << "].");
  boolean::Expression branch_path_condition[2] = { true, true };
  boolean::Product branch_condition;
  bool have_branch = false;
  for (auto&& end_point : m_end_points)
    if (end_point.type() == tail && end_point.edge()->is_opsem() &&
        (end_point.other_node() == read_node || end_point.other_node()->is_sequenced_before(*read_node)))
    {
      // Opsem edges have edge conditions that are products.
      boolean::Product edge_condition{end_point.edge()->condition().as_product()};
#ifdef CWDEBUG
      if (!edge_condition.is_one())
        Dout(dc::visited, "Edge from " << name() << " to " << end_point.other_node()->name() << " has edge condition " << edge_condition << '.');
#endif
      if (!have_branch && !edge_condition.is_one())
      {
        have_branch = true;
        branch_condition = edge_condition;
        Dout(dc::visited, "Visited-condition of " << name() << " --> " << end_point.other_node()->name() << " is " <<
            end_point.edge()->visited_condition(visited_generation) << '.');
        branch_path_condition[0] = end_point.edge()->visited_condition(visited_generation).copy();
        Dout(dc::visited, "Initialized branch_path_condition[0] with " << branch_path_condition[0] << " and branch_path_condition[1] with 1.");
        continue;
      }
      int branch = (!have_branch || edge_condition == branch_condition) ? 0 : 1;
      branch_path_condition[branch] = branch_path_condition[branch].times(end_point.edge()->visited_condition(visited_generation));
      Dout(dc::visited, "Visited-condition of " << name() << " --> " << end_point.other_node()->name() << " is " <<
          end_point.edge()->visited_condition(visited_generation) << "; branch_path_condition[" << branch << "] is now " << branch_path_condition[branch] << '.');
    }
  return (have_branch && !branch_path_condition[1].is_one()) ? branch_path_condition[0] + branch_path_condition[1] : branch_path_condition[0].copy();
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct for_action("FORACTION");
channel_ct visited("VISITED");
NAMESPACE_DEBUG_CHANNELS_END
#endif
