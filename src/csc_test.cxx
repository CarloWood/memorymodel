#include "sys.h"
#include "debug.h"
#include "utils/MultiLoop.h"
#include "utils/is_power_of_two.h"
#include <iostream>
#include <array>
#include <iomanip>
#include <bitset>
#include <stack>
#include <set>

int constexpr number_of_threads = 3;
int constexpr nodes_per_thread = 2;
int constexpr number_of_nodes = nodes_per_thread * number_of_threads;

int thread(int node)
{
  return node / nodes_per_thread;
}

int row(int node)
{
  return node % nodes_per_thread;
}

using mask_type = uint64_t;

mask_type constexpr one = 1;
mask_type constexpr thread_mask0 = ((one << nodes_per_thread) - 1) << ((number_of_threads - 1) * nodes_per_thread);

mask_type to_mask(int node)
{
  return one << (number_of_nodes - 1 - node);
}

mask_type same_thread(int node)
{
  return thread_mask0 >> (nodes_per_thread * thread(node));
}

// Suppose that 'node' is B2, then to_mask(node) would return
// the following mask:
//
//        thread: A    B    C
//
// row: 0    msb->0    0    0  (msb: most significant bit).
//      1         0    0    0
//      2         0    1    0
//      3         0    0    0
//      4         0    0    0
//                          ^
//                          |__ least significant bit.
//
// mask - 1 is equal to:
//
// row: 0    msb->0    0    1
//      1         0    0    1
//      2         0    0    1
//      3         0    1    1
//      4         0    1    1
//
// And same_thread(node) would return:
//
// row: 0    msb->0    1    0
//      1         0    1    0
//      2         0    1    0
//      3         0    1    0
//      4         0    1    0
//
mask_type sequenced_after(int node)
{
  mask_type const mask = to_mask(node);
  return same_thread(node) & (mask - 1);
}

mask_type sequenced_before(int node)
{
  mask_type const mask = to_mask(node);
  return same_thread(node) & ~(mask - 1) & ~mask;
}

bool is_same_thread(int node1, int node2)
{
  return thread(node1) == thread(node2);
}

bool is_sequenced_before(int node1, int node2)
{
  mask_type const mask1 = to_mask(node1);
  mask_type const before2 = sequenced_before(node2);
  return (mask1 & before2);
}

int first_node(int thread)
{
  return thread * nodes_per_thread;
}

int last_node(int thread)
{
  return first_node(thread) + nodes_per_thread - 1;
}

int next_thread(int node)
{
  return thread(node) + 1;
}

// Return the last thread that isn't the thread of node.
int last_thread(int node)
{
  return (thread(node) == number_of_threads - 1) ? number_of_threads - 2 : number_of_threads - 1;
}

class Graph;

class Node
{
  static int constexpr unused = -1;
  friend class Graph;
  int m_index;                  // Index into the graph array.
  int m_linked;                 // The index of the Node that we're linked with (or unused).
  mask_type m_lowest_endpoints; // The lowest end point of staircases from this node to each respective thread.
  mask_type m_beginpoints;      // The begin points of staircases leading to m_lowest_endpoints.
  int m_old_loop_node_offset;
  static int s_connected_nodes;

 public:
  Node() : m_linked(unused), m_lowest_endpoints(0) { }
  Node(unsigned int linked) : m_linked(linked) { }
  Node(int linked) : m_linked(linked) { }

 private:
  void init(int index)
  {
    m_index = index;
  }

 public:
  bool link(Graph& graph, Node& node);
  static int connected_nodes() { return s_connected_nodes; }
  bool is_unused() const { return m_linked == unused; }
  bool is_used() const { return m_linked != unused; }
  bool is_linked_to(int node) const { return m_linked == node; }
  bool is_connected() const { return m_linked != unused && m_linked != m_index; }
  bool detect_loop(int index);
  bool can_set(Graph& graph, mask_type mask) const;
  void reset()
  {
    if (m_index < number_of_nodes - nodes_per_thread)
      --s_connected_nodes;
    m_linked = unused;
    m_lowest_endpoints = 0;
    m_beginpoints = 0;
  }
  void add_csc_to(Graph& graph, Node& node);

  void set_old_loop_node_offset(int old_loop_node_offset) { m_old_loop_node_offset = old_loop_node_offset; }
  int get_old_loop_node_offset() const { return m_old_loop_node_offset; }

  mask_type mask() const { return to_mask(m_index); }
  mask_type thread_mask() const { return same_thread(m_index); }
  mask_type sequenced_after_mask() const { return sequenced_after(m_index); }

  std::string name() const
  {
    std::string result;
    result += char('A' + thread(m_index));
    result += char('0' + row(m_index));
    return result;
  }

  friend std::ostream& operator<<(std::ostream& os, Node const& node)
  {
    return os << node.m_linked;
  }

  friend bool operator<(Node const& node1, Node const& node2)
  {
    ASSERT(node1.m_index == node2.m_index);
    return node1.m_linked < node2.m_linked;
  }
};

//static
int Node::s_connected_nodes;

using array_type = std::array<Node, number_of_nodes>;

class Graph : public array_type
{
 public:
  Graph();

  void print_on(std::ostream& os) const;
  bool calculate_staircases(bool fix = false);
  bool follow(int index, std::array<int, number_of_threads>& lowest_per_thread, std::array<int, number_of_threads> visited);
  bool is_looped(std::vector<bool>& ls, std::array<int, number_of_threads> a, int index) const;
  Node& mask_to_node(mask_type mask);
  bool sanity_check(bool output) const;

  void unlink(Node& node)
  {
    DoutEntering(dc::notice, "unlink(" << node.name() << ")");
    ASSERT(node.m_linked != Node::unused);
    if (node.m_linked != node.m_index)
      at(node.m_linked).reset();
    node.reset();
    calculate_staircases(true);
  }

  Node& linked_node(Node const& node)
  {
    ASSERT(node.is_used());
    return at(node.m_linked);
  }

  friend std::ostream& operator<<(std::ostream& os, Graph const& graph)
  {
    graph.print_on(os);
    return os;
  }
};

Graph::Graph()
{
  for (int index = 0; index < number_of_nodes; ++index)
    at(index).init(index);
}

// Starting at node index, follow staircases upwards and update lowest_per_thread.
// If a thread is entered at a point lower then were we were before (not as a branch,
// but as a loop) then return true.
bool Graph::follow(int index, std::array<int, number_of_threads>& lowest_per_thread, std::array<int, number_of_threads> visited)
{
  DoutEntering(dc::notice|continued_cf, "Graph::follow(" << index << ",");
  for (int e : lowest_per_thread) Dout(dc::continued, ' ' << e);
  Dout(dc::continued, ", ");
  for (int e : visited) Dout(dc::continued, ' ' << e);
  Dout(dc::finish, ")");
  Graph const& graph = *this;
  Node const* node = &graph[index];
  int row = ::row(index);
  if (index >= visited[thread(index)])        // Loop?
  {
    Dout(dc::notice, "Returning true because visited[" << thread(index) << "] = " << visited[thread(index)] <<
        ", which is less than or equal the current index (" << index << ").");
    return true;
  }
  int t = thread(index);
  visited[t] = index;
  if (index > lowest_per_thread[t] || lowest_per_thread[t] == number_of_nodes)
  {
    lowest_per_thread[t] = index;
    Dout(dc::notice, "Setting lowest_per_thread[" << t << "] to " << index);
  }
  while (row > 0)
  {
    --node;
    --index;
    --row;
    if (node->is_connected())
    {
      Dout(dc::notice, "Following rf of node " << node->name());
      if (follow(node->m_linked, lowest_per_thread, visited))
      {
        Dout(dc::notice, "Returning 'loop' from Graph::follow.");
        return true;
      }
    }
  }
  Dout(dc::notice|continued_cf, "No loop detected; returning false. lowest_per_thread is now: ");
  for (int e : lowest_per_thread)
  {
    if (e == number_of_nodes)
      Dout(dc::continued, " -");
    else
    {
      Node n;
      n.init(e);
      Dout(dc::continued, ' ' << n.name());
    }
  }
  Dout(dc::finish, ".");
  return false;
}

// Recalculate all staircases brute-force.
// Return true if a loop is detected.
bool Graph::calculate_staircases(bool fix)
{
  DoutEntering(dc::notice, "Graph::calculate_staircases()");
  Graph& graph(*this);
  // Run over all connected nodes.
  for (Node& start_node : graph)
  {
    if (!start_node.is_connected())
      continue;
    Dout(dc::notice, "for node " << start_node.name() << "...");
    // Fill an array with sufficiently large values, so every node index is less.
    std::array<int, number_of_threads> lowest_per_thread;
    for (int& e : lowest_per_thread) e = number_of_nodes;
    int t2 = thread(start_node.m_index);
    lowest_per_thread[t2] = start_node.m_index;
    Dout(dc::notice, "set lowest_per_thread[" << t2 << "] to " << start_node.m_index);
    Node* node = &start_node;
    int row = ::row(start_node.m_index);
    do
    {
      if (node->is_connected())
      {
        std::array<int, number_of_threads> visited;
        for (int& e : visited) e = number_of_nodes;
        visited[t2] = node->m_index;
        if (follow(node->m_linked, lowest_per_thread, visited))
        {
          Dout(dc::notice, "Returning 'loop' from Graph::calculate_staircases.");
          return true;
        }
      }
      --row;
      --node;
    }
    while (row >= 0);
    // Calculate the correct lowest_endpoints.
    mask_type lowest_endpoints = 0;
    for (int t = 0; t < number_of_threads; ++t)
      if (lowest_per_thread[t] < number_of_nodes)
        lowest_endpoints |= to_mask(lowest_per_thread[t]);
    if (start_node.m_lowest_endpoints != lowest_endpoints)
    {
      if (!fix)
        DoutFatal(dc::core, "m_lowest_endpoints of node " << start_node.name() << " should be " << std::bitset<number_of_nodes>(lowest_endpoints) <<
            ", but is " << std::bitset<number_of_nodes>(start_node.m_lowest_endpoints));
      Dout(dc::notice, "Fixing m_lowest_endpoints of node " << start_node.name());
      start_node.m_lowest_endpoints = lowest_endpoints;
    }
  }
  Dout(dc::notice, "Return 'no loop' from Graph::calculate_staircases.");
  return false;
}

// Return true if above us there is a (canonical) staircase that ends below index.
bool Node::detect_loop(int index)
{
  int t = thread(index);
  int row = ::row(m_index);
  Node* node = this;
  while (row > 0)
  {
    --row;
    --node;
    if (node->is_connected())
    {
      for (int n = first_node(t); n <= last_node(t); ++n)
      {
        if ((node->m_lowest_endpoints & to_mask(n)))
        {
          if (n > index)
            return true;
          break;
        }
      }
      break;
    }
  }
  return false;
}

mask_type thread_to_mask(int t)
{
  mask_type m1 = first_node(t) - 1;
  mask_type m2 = first_node(t + 1) - 1;
  return m2 ^ m1;
}

bool Node::can_set(Graph& graph, mask_type mask) const
{
  Node& n{graph.mask_to_node(mask)};
  int t = thread(n.m_index);
  mask_type tm = same_thread(t);
  mask_type r = (m_lowest_endpoints | mask) & tm;
  return r == 0 || utils::is_power_of_two(r);
}

void Node::add_csc_to(Graph& graph, Node& node2)
{
  DoutEntering(dc::notice, "'" << name() << "'->add_csc_to(graph, '" << node2.name() << "')");
  mask_type const bp = to_mask(m_index);        // The begin point is on this node.
  mask_type const ep = to_mask(node2.m_index);  // The end point is at the other node.
  node2.m_beginpoints |= bp;                    // Add the new csc link.
  ASSERT(can_set(graph, ep));
  m_lowest_endpoints |= ep;
  ASSERT(can_set(graph, bp));
  m_lowest_endpoints |= bp;                     // Also mark ourselves as reachable endpoint from here.
  int t1 = thread(m_index);
  int last_node1 = last_node(t1);
  int t2 = thread(node2.m_index);
  mask_type const sa1 = sequenced_after(m_index);
  mask_type const sa2 = sequenced_after(node2.m_index);
  mask_type const sb2 = sequenced_before(node2.m_index);
  // Run over all nodes sequenced after the current node.
  for (int i = m_index + 1; i <= last_node1; ++i)
  {
    Node& node_sequenced_after{graph[i]};
    // Skip unused nodes.
    if (!node_sequenced_after.is_connected())
      continue;

    // Cross rule:
    //
    //                |      |
    // our beginpoint o      |
    //                |╲1    |
    //                | ╲  2 |
    //              a o──╲──>o e
    //              b o───╲─>o f
    //                | 3  ╲ |
    //                |     >o our endpoint
    //                |      |
    //              c o─────>o
    //                |      |
    //
    // If a new canonical staircase (csc) edge '1' is added
    // and crosses one or more existing staircases (here '2'
    // and '3' with respective begin points 'a' and 'b'),
    // such that their begin points ('a' and 'b') are sequenced
    // after our beginpoint and their endpoint is sequenced
    // before our endpoint then those endpoints need to
    // be changed become our endpoint.
    //
    // FIXME: this documentation is incomplete
    // Also every other existing csc that ends in each of
    // the respective endpoints ('e' and 'f') and begins at
    // the node linked to their beginpoint, or begins in a
    // beginpoint of a csc that ends in that linked node,
    // should have their end point adjusted.
    //
    // For example, each csc in this graph (2, 4, 5) need to
    // have it's endpoint also adjusted from 'e' to our endpoint.
    //
    //                |                |
    // our beginpoint o                |
    //                |                |
    //                |        2       |
    //              a o───────────────>o e
    //                |                |
    //                |  rf       4    |
    //              a o------o────────>o e
    //                |                |
    //                |  rf         5  |
    //              a o------o<───o───>o e
    //                |                |
    //                |                |
    //                |                o our endpoint
    //                |                |

    // As soon as we find a point (c) that ends after our
    // end point, we can stop searching.
    if ((sa2 & node_sequenced_after.m_lowest_endpoints))
      break;
    if ((sb2 & node_sequenced_after.m_lowest_endpoints))
    {
      // Fix me.
      ASSERT(false);
    }
  }

  for (Node& node : graph)                              // Run over all nodes ◉ not in t1 or t2.
  {
    int t = thread(node.m_index);
    if (t == t1 || t == t2)
      continue;

    mask_type ep1 = node.m_lowest_endpoints & sa1;      // csc's ending after bp.
    mask_type ep2 = node.m_lowest_endpoints & sa2;      // csc's ending after ep.

    // Staircase rule:              ________
    //         t1      t2          ╱    3   ╲
    // |       |sb1 sb2|       |  ╱    |     ╲ |
    // |    bp o───2──>o ep    | ╱     o──────>o
    // |       |⌉     ⌈|  ==>  |╱      |       |
    // ◉───1──>⊗sa1 sa2|       ◉──────>⊗       |
    // |       |⌋     ⌊|       |       |       |
    //
    // If the canonical staircase edge '1' already exists and
    // we are creating '2', then we need to add a csc 3 to
    // our end point as that is now reachable following 1 and
    // then 2; however if there is already a csc from ◉ to sa2
    // then we should not add a csc to ep as that is not lower
    // on the t2 thread. If ◉ already has a csc to sb2 then its
    // endpoint needs to be updated.
    if (ep1 && !ep2)                                    // ◉ has a csc that ends in sa1 but not in sa2.
    {
      // Then add or update a csc link (3) to our endpoint.
      node.m_lowest_endpoints &= ~sb2;
      ASSERT(node.can_set(graph, ep));
      node.m_lowest_endpoints |= ep;
      ASSERT(node2.can_set(graph, to_mask(node.m_index)));
      node2.m_beginpoints |= to_mask(node.m_index);

      // Also from the point in sa1.
      Node& nx{graph.mask_to_node(ep1)};                // ⊗
      ASSERT(!(nx.m_lowest_endpoints & sb2));
      ASSERT(nx.can_set(graph, to_mask(nx.m_index)));
      nx.m_lowest_endpoints |= ep;
      node2.m_beginpoints |= to_mask(nx.m_index);
    }
  }
  for (Node& node : graph)                              // Run over all nodes ◉ not in t1 or t2.
  {
    int t = thread(node.m_index);
    if (t == t1 || t == t2)
      continue;

    // On the other hand, when staircase edge '2' already exists
    // we are creating '1', then we have the following case:
    //                                 ________
    //    t1                          ╱    3   ╲
    //    |⌉     ⌈|       |       |  ╱    |     ╲ |
    //    |sb1 sb2⊗───2──>◉       | ╱     ⊗──────>◉
    //    |⌋     ⌊|       |  ==>  |╱      |       |
    // bp o───1──>o ep    |       o──────>o       |
    //    |sa1 sa2|       |       |       |       |
    if ((node.m_beginpoints & sb2))
    {
      add_csc_to(graph, node);      
      node2.add_csc_to(graph, node);
    }
  }
}

bool Node::link(Graph& graph, Node& node)
{
  DoutEntering(dc::notice, name() << "->Node::link(graph, " << node.name() << ")");
  Dout(dc::notice, "graph is now:");
  Dout(dc::notice, graph);
  ASSERT(m_index < number_of_nodes - nodes_per_thread);
  if (this != &node)
  {
    ASSERT(m_linked == unused && node.m_linked == unused);
    //Debug(libcw_do.off());
    bool is_looping = graph.calculate_staircases();
    //Debug(libcw_do.on());
    ASSERT(!is_looping);
    is_looping = detect_loop(node.m_index) || node.detect_loop(m_index);
    if (is_looping)
      return false;
    m_linked = node.m_index;
    node.m_linked = m_index;
    // Do not count nodes in the last thread.
    if (node.m_index < number_of_nodes - nodes_per_thread)
      ++s_connected_nodes;
    // Update m_lowest_endpoints.
    add_csc_to(graph, node);
    node.add_csc_to(graph, *this);
    Dout(dc::notice, "graph is now:");
    Dout(dc::notice, graph);
    // Check if this worked...
    is_looping = graph.calculate_staircases();
    ASSERT(!is_looping);
  }
  else
  {
    ASSERT(m_linked == unused);
    m_linked = m_index;
  }
  ++s_connected_nodes;
  return true;
}

void Graph::print_on(std::ostream&) const
{
  std::ostream& os{std::cout};
  Graph const& graph(*this);
  int const indent = 4;
  std::string header1;
  std::string header2;
  for (int t = 0; t < number_of_threads; ++t)
    for (int j = 0; j < nodes_per_thread; ++j)
    {
      header1 += char('A' + t);
      header2 += char('0' + j);
    }
  os << std::setw(indent + 1) << ' ';
  for (int t = 0; t < number_of_threads; ++t)
    os << "       " << header1 << "  ";
  os << '\n';
  os << std::setw(indent + 1) << ' ';
  for (int t = 0; t < number_of_threads; ++t)
    os << "     " << char('A' + t) << ' ' << header2 << "  ";
  os << '\n';
  for (int f = 0; f < nodes_per_thread; ++f)
  {
    for (int ba = 0; ba < 1; ++ba)
    {
      os << std::setw(indent) << ' ' << ' ';
      for (int n = f; n < number_of_nodes; n += nodes_per_thread)
      {
        if (ba == 0)
          os << std::setw(2) << n << ": ";
        else
          os << "    ";
        int linked = graph[n].m_linked;
        char linked_thread = char('A' + thread(graph[n].m_linked));
        int linked_row = row(linked);
        if (ba == 1)
          os << "   ";
        else if (graph[n].is_unused())            // Not linked?
          os << " *-";
        else if (graph[n].is_linked_to(n))        // Linked to itself?
          os << " |-";
        else
          os << linked_thread << linked_row << '-';
        std::bitset<number_of_nodes> bs(graph[n].m_lowest_endpoints);
        os << bs << "  ";
      }
      os << '\n';
    }
  }
  os << std::endl;

  //std::getchar();
}

std::set<Graph> all_graphs;

Node& Graph::mask_to_node(mask_type mask)
{
  ASSERT(utils::is_power_of_two(mask));
  Graph& graph{*this};
  int s = __builtin_clzll(mask) - (8 * sizeof(mask_type) - number_of_nodes);
  Node& result{graph[s]};
  ASSERT(to_mask(result.m_index) == mask);
  return result;
}

bool Graph::is_looped(std::vector<bool>& ls, std::array<int, number_of_threads> a, int index) const
{
  DoutEntering(dc::notice, "Graph::is_looped(.., " << index << ")");
#ifdef CWDEBUG
  Dout(dc::notice|continued_cf, "Array a[] =");
  for (int e : a)
    Dout(dc::continued, ' ' << e);
  Dout(dc::finish, "");
#endif
  Graph const& graph(*this);

  if (ls[index])
  {
    Dout(dc::notice, "Returning 'no loop' because index " << index << " was already marked as not a loop.");
    return false;
  }
  int t = thread(index);
  if (index >= a[t])
  {
    Dout(dc::notice, "Returning 'loop!' because index " << index << " is larger or equal to a[" << t << "] = " << a[t]);
    return true;
  }
  int first_node = t * nodes_per_thread;
  for (int i = index; i >= first_node; --i)
  {
    if (ls[i])
    {
      Dout(dc::notice, "Returning 'no loop' because index " << i << " was already marked as not a loop.");
      return false;
    }
    a[t] = i;
    Dout(dc::notice, "Set a[" << t << "] to " << i);
    if (graph[i].m_linked != -1 && graph[i].m_linked != i && row(graph[i].m_linked) > 0 && is_looped(ls, a, graph[i].m_linked - 1))
    {
      Dout(dc::notice, "Returning 'loop!' because is_looped() returned 'loop!' for index " << (graph[i].m_linked - 1));
      return true;
    }
  }
  ls[index] = true;
  Dout(dc::notice, "Marked index " << index << " as NOT a loop.");
  return false;
}

bool Graph::sanity_check(bool output) const
{
  Graph const& graph(*this);
  if (output)
  {
    // Make sure this is the first and only time this function is called for this graph.
    auto res = all_graphs.insert(graph);
    ASSERT(res.second);
  }

  // Set to true when node is checked to NOT be part of a looped staircase.
  std::vector<bool> ls(number_of_nodes, false);

  // Make sure that none of the nodes are connected to twice and they are only connected to nodes of other threads.
  std::vector<bool> v(number_of_nodes, false);
  for (auto&& node : graph)
  {
    int linked = node.m_linked;
    if (linked == -1)           // Unused
      linked = node.m_index;    // is the same as self-connected.
    ASSERT(0 <= linked && linked < number_of_nodes);
    ASSERT(!v[linked]);
    v[linked] = true;
    ASSERT(linked == node.m_index || (thread(linked) != thread(node.m_index) && graph[linked].m_linked == node.m_index));
    // Check for each node that it isn't part of a looped staircase.
    std::array<int, number_of_threads> a;
    for (auto&& d : a)
      d = number_of_nodes;
    if (is_looped(ls, a, node.m_index))
      return false;
  }
  return true;
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  //Debug(libcw_do.off());
  // Each bit in mask_type represents a node.
  ASSERT(number_of_nodes <= sizeof(mask_type) * 8);

  int constexpr number_of_loops = number_of_nodes;
  Graph graph;

  int loop_node_offset = 0;
  // First loop starts at value nodes_per_thread - 1, so it gets connected to
  // itself and subsequently to the first node of the next thread.
  for (MultiLoop ml(number_of_loops - nodes_per_thread, nodes_per_thread - 1); !ml.finished(); ml.next_loop())
  {
#ifdef CWDEBUG
    if (ml() >= number_of_nodes)
    {
      Dout(dc::notice, "Not even entering while loop for loop " << *ml << " because its value " << ml() << " >= " << number_of_nodes << "!");
    }
#endif
    while (ml() < number_of_nodes)
    {
      Dout(dc::notice, "At top of loop " << *ml << " (with value " << ml() << ").");

      int old_loop_node_offset = loop_node_offset;
      int loop_node_index = *ml + loop_node_offset;
      while (loop_node_index < number_of_nodes && graph[loop_node_index].is_used())
      {
        Dout(dc::notice, "Skipping " << graph[loop_node_index].name() <<
          " because it is already connected (to " << graph.linked_node(graph[loop_node_index]).name() << ").");
        ++loop_node_index;
      }
      bool fully_connected = false;
      bool last_node = loop_node_index >= number_of_nodes - nodes_per_thread;
      Node* loop_node = nullptr;
      if (!last_node)
      {
        // The next loop must start at the node number of the last node of the current thread,
        // which indicates it needs to be connected to itself; and the next iteration we'll
        // connect it then to the first node of the next thread.
        int begin = (loop_node_index / nodes_per_thread + 1) * nodes_per_thread - 1;
        Dout(dc::notice, "  begin = " << begin << ".");

        loop_node_offset = loop_node_index - *ml;
        loop_node = &graph[loop_node_index];
        loop_node->set_old_loop_node_offset(old_loop_node_offset);
        int to_node_index = (ml() == begin) ? loop_node_index : ml();
        Node& to_node{graph[to_node_index]};
        Dout(dc::notice, "Considering " << loop_node->name() << " <--> " << to_node.name() << '.');
        if (to_node.is_used())
        {
          Dout(dc::notice, "Skipping connection to " << to_node.name() << " because it is already connected (to " << graph.linked_node(to_node).name() << ").");
          loop_node_offset = loop_node->get_old_loop_node_offset();
          ml.breaks(0);                // Normal continue (of current loop).
          break;                       // Return control to MultiLoop.
        }
        if (!loop_node->link(graph, to_node))
        {
          Dout(dc::notice, "Skipping connection to " << to_node.name() << " because it is hidden.");
          loop_node_offset = loop_node->get_old_loop_node_offset();
          ml.breaks(0);                // Normal continue (of current loop).
          break;                       // Return control to MultiLoop.

        }
        Dout(dc::notice|continued_cf, "Made connection for loop " << *ml << " (" << loop_node->name() << " <--> " << to_node.name());
        fully_connected = Node::connected_nodes() == number_of_nodes - nodes_per_thread;
        if (fully_connected)
          Dout(dc::continued, "; we are now fully connected");
        Dout(dc::finish, ").");
        Dout(dc::notice, graph);
      }
      else
        Dout(dc::notice, "This was the last node of the second-last thread.");

      if (ml.inner_loop() || fully_connected || last_node)
      {
        // Here all possible permutations of connecting pairs occur.
        static int ok_count = 0, count = 0;
        ok_count += graph.sanity_check(true) ? 1 : 0;
        ++count;
        std::cout << "Graph #" << ok_count << '/' << count << std::endl;
        graph.print_on(std::cout);

        if (loop_node)
        {
          Dout(dc::notice, "At the end of loop " << *ml << " (" << loop_node->name() << ").");
          graph.unlink(*loop_node);
          Dout(dc::notice, "Unlinked " << loop_node->name() << ":");
          Dout(dc::notice, graph);
          loop_node_offset = loop_node->get_old_loop_node_offset();
        }

        if (!ml.inner_loop())
        {
          // Continue the current loop instead of starting a new nested loop.
          ml.breaks(0);
          break;
        }
      }
      // This concerns the begin of the NEXT loop, so add 1 to loop_node_index,
      // or more if the next node is already used.
      int next_loop_node_index = loop_node_index;
      while (++next_loop_node_index < number_of_nodes && graph[next_loop_node_index].is_used())
        ;
      int begin = (next_loop_node_index / nodes_per_thread + 1) * nodes_per_thread - 1;
      ASSERT(next_loop_node_index == number_of_nodes || begin < number_of_nodes);
      ml.start_next_loop_at(begin);
#ifdef CWDEBUG
      if (ml() >= number_of_nodes)
      {
        Dout(dc::notice, "Leaving while loop because ml() reached " << ml());
      }
#endif
    }
    int loop = ml.end_of_loop();
    if (loop >= 0)
    {
      Node& loop_node{graph[loop + loop_node_offset]};
      Dout(dc::notice, "At the end of loop " << loop << " (" << loop_node.name() << ").");
      graph.unlink(loop_node);
#ifdef CWDEBUG
      Dout(dc::notice, "Unlinked " << loop_node.name() << ":");
      Dout(dc::notice, graph);
#endif
      loop_node_offset = loop_node.get_old_loop_node_offset();
    }
  }
}
