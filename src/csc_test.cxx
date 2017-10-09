#include "sys.h"
#include "debug.h"
#include "utils/MultiLoop.h"
#include <iostream>
#include <array>
#include <iomanip>
#include <bitset>
#include <stack>
#include <set>

int constexpr number_of_threads = 4;
int constexpr nodes_per_thread = 3;
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

mask_type at_or_sequenced_below(int node)
{
  mask_type const mask = to_mask(node);
  return same_thread(node) & (~mask + 1);
}

mask_type sequenced_after(int node)
{
  mask_type const mask = to_mask(node);
  return same_thread(node) & ~(mask - 1) & ~mask;
}

class Node
{
  static int constexpr unused = -1;
  friend class Graph;
  int m_index;                  // Index into the graph array.
  int m_linked;                 // The index of the Node that we're linked with (or unused).
  mask_type m_sequenced_after;
  mask_type m_sequenced_before;
  int m_old_loop_node_offset;
  static int s_connected_nodes;

 public:
  Node() : m_linked(unused), m_sequenced_after(0), m_sequenced_before(0) { }
  Node(unsigned int linked) : m_linked(linked) { }
  Node(int linked) : m_linked(linked) { }

 private:
  void init(int index)
  {
    m_index = index;
  }

  bool link_to(Node& linked_node)
  {
    ASSERT(m_linked == unused);
    Node* node = this;
    int index = m_index;
    int row = ::row(index);
    while (row > 0)
    {
      --index;
      --node;
      --row;
      m_sequenced_after |= to_mask(index);
      if (node->m_linked != unused && node->m_linked != index)
      {
        m_sequenced_after |= to_mask(node->m_linked);
        m_sequenced_after |= node->m_sequenced_after;
        break;
      }
    }
    node = this;
    index = m_index;
    row = ::row(index);
    while (row < nodes_per_thread - 1)
    {
      ++index;
      ++node;
      ++row;
      m_sequenced_before |= to_mask(index);
      if (node->m_linked != unused && node->m_linked != index)
      {
        m_sequenced_before |= to_mask(node->m_linked);
        m_sequenced_before |= node->m_sequenced_before;
        break;
      }
    }
    m_linked = linked_node.m_index;
    m_sequenced_after |= linked_node.m_sequenced_after;
    linked_node.m_sequenced_after |= m_sequenced_after;
    m_sequenced_before |= linked_node.m_sequenced_before;
    linked_node.m_sequenced_before |= m_sequenced_before;
    // Did this create a staircase loop?
    if ((m_sequenced_before & m_sequenced_after) != 0)
    {
      m_linked = unused;
      m_sequenced_after = linked_node.m_sequenced_after = 0;
      m_sequenced_before = linked_node.m_sequenced_before = 0;
      return false;
    }
    return true;
  }

 public:
  bool link(Node& node)
  {
    ASSERT(m_index < number_of_nodes - nodes_per_thread);
    if (this != &node)
    {
      if (!link_to(node))
        return false;
      // Do not count nodes in the last thread.
      if (node.m_index < number_of_nodes - nodes_per_thread)
        ++s_connected_nodes;
      if (!node.link_to(*this))
      {
        m_linked = unused;
        return false;
      }
    }
    else
    {
      m_linked = m_index;
    }
    ++s_connected_nodes;
    return true;
  }

  static int connected_nodes() { return s_connected_nodes; }
  bool is_unused() const { return m_linked == unused; }
  bool is_used() const { return m_linked != unused; }
  bool is_linked_to(int node) const { return m_linked == node; }
  void reset()
  {
    if (m_index < number_of_nodes - nodes_per_thread)
      --s_connected_nodes;
    m_linked = unused;
    m_sequenced_after = 0;
  }

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

  void print() const;
  bool is_looped(std::vector<bool>& ls, std::array<int, number_of_threads>& a, int index) const;
  bool sanity_check() const;

  void unlink(Node& node)
  {
    ASSERT(node.m_linked != Node::unused);
    if (node.m_linked != node.m_index)
      at(node.m_linked).reset();
    node.reset();
  }

  Node& linked_node(Node const& node)
  {
    ASSERT(node.is_used());
    return at(node.m_linked);
  }
};

Graph::Graph()
{
  for (int index = 0; index < number_of_nodes; ++index)
    at(index).init(index);
}

bool is_same_thread(int node1, int node2)
{
  return thread(node1) == thread(node2);
}

bool is_sequenced_before(int node1, int node2)
{
  return node1 < node2 && is_same_thread(node1, node2);
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

int last_thread(int node)
{
  return (thread(node) == number_of_threads - 1) ? number_of_threads - 2 : number_of_threads - 1;
}

void Graph::print() const
{
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
  std::cout << std::setw(indent + 2) << ' ';
  for (int t = 0; t < number_of_threads; ++t)
    std::cout << "      " << header1 << "  ";
  std::cout << '\n';
  std::cout << std::setw(indent + 2) << ' ';
  for (int t = 0; t < number_of_threads; ++t)
    std::cout << "    " << char('A' + t) << ' ' << header2 << "  ";
  std::cout << '\n';
  for (int f = 0; f < nodes_per_thread; ++f)
  {
    std::cout << std::setw(indent) << ' ' << "  ";
    for (int n = f; n < number_of_nodes; n += nodes_per_thread)
    {
      std::cout << n << ": ";
      int linked = graph[n].m_linked;
      char linked_thread = char('A' + thread(graph[n].m_linked));
      int linked_row = row(linked);
#if 2
      if (graph[n].is_unused())                 // Not linked?
        std::cout << " *";
      else if (graph[n].is_linked_to(n))        // Linked to itself?
        std::cout << " |";
      else
        std::cout << linked_thread << linked_row;
      std::bitset<number_of_nodes> bs(graph[n].m_sequenced_after);
#else
        std::cout << " |";
      std::bitset<number_of_nodes> bs(graph[n].sequenced_after_mask());
#endif
      std::cout << '-' << bs << "  ";
    }
    std::cout << '\n';
  }
  std::cout << std::endl;

  //std::getchar();
}

std::set<Graph> all_graphs;

bool Graph::is_looped(std::vector<bool>& ls, std::array<int, number_of_threads>& a, int index) const
{
  Graph const& graph(*this);

  if (ls[index])
    return false;
  int t = thread(index);
  if (index >= a[t])
    return true;
  int first_node = t * nodes_per_thread;
  for (int i = index - 1; i >= first_node; --i)
  {
    if (ls[i])
      return false;
    a[t] = i;
    if (graph[i].m_linked != -1 && graph[i].m_linked != i && is_looped(ls, a, graph[i].m_linked))
      return true;
  }
  ls[index] = true;
  return false;
}

bool Graph::sanity_check() const
{
  Graph const& graph(*this);
  // Make sure this is the first and only time this function is called for this graph.
  auto res = all_graphs.insert(graph);
  ASSERT(res.second);

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
  // Each bit in mask_type represents a node.
  ASSERT(number_of_nodes <= sizeof(mask_type) * 8);

  int constexpr number_of_loops = number_of_nodes;
  Graph graph;

  int loop_node_offset = 0;
  // First loop starts at value nodes_per_thread - 1, so it gets connected to
  // itself and subsequently to the first node of the next thread.
  for (MultiLoop ml(number_of_loops - nodes_per_thread, nodes_per_thread - 1); !ml.finished(); ml.next_loop())
  {
#if 0
    if (ml() >= number_of_nodes)
    {
      std::cout << "WARNING: not even entering while loop for loop " << *ml << " because its value " << ml() << " >= " << number_of_nodes << "!" << std::endl;
    }
#endif
    while (ml() < number_of_nodes)
    {
#if 0
      std::cout << "At top of loop " << *ml << " (with value " << ml() << ")." << std::endl;
#endif
      int old_loop_node_offset = loop_node_offset;
      int loop_node_index = *ml + loop_node_offset;
      while (loop_node_index < number_of_nodes && graph[loop_node_index].is_used())
      {
#if 0
        std::cout << "Skipping " << graph[loop_node_index].name() <<
          " because it is already connected (to " << graph.linked_node(graph[loop_node_index]).name() << ")." << std::endl;
#endif
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
#if 0
        std::cout << "  begin = " << begin << "." << std::endl;
#endif

        loop_node_offset = loop_node_index - *ml;
        loop_node = &graph[loop_node_index];
        loop_node->set_old_loop_node_offset(old_loop_node_offset);
        int to_node_index = (ml() == begin) ? loop_node_index : ml();
        Node& to_node{graph[to_node_index]};
#if 0
        std::cout << "Considering " << loop_node->name() << " <--> " << to_node.name() << '.' << std::endl;
#endif
        if (to_node.is_used())
        {
#if 0
          std::cout << "Skipping connection to " << to_node.name() << " because it is already connected (to " << graph.linked_node(to_node).name() << ")." << std::endl;
#endif
          loop_node_offset = loop_node->get_old_loop_node_offset();
          ml.breaks(0);                // Normal continue (of current loop).
          break;                       // Return control to MultiLoop.
        }
        if (!loop_node->link(to_node))
        {
#if 0
          std::cout << "Skipping connection to " << to_node.name() << " because it is hidden." << std::endl;
#endif
          loop_node_offset = loop_node->get_old_loop_node_offset();
          ml.breaks(0);                // Normal continue (of current loop).
          break;                       // Return control to MultiLoop.

        }
#if 0
        std::cout << "Made connection for loop " << *ml << " (" << loop_node->name() << " <--> " << to_node.name();
#endif
        fully_connected = Node::connected_nodes() == number_of_nodes - nodes_per_thread;
#if 0
        if (fully_connected)
          std::cout << "; we are now fully connected";
        std::cout << ")." << std::endl;
        graph.print();
#endif
      }
#if 0
      else
        std::cout << "This was the last node of the second-last thread." << std::endl;
#endif

      if (ml.inner_loop() || fully_connected || last_node)
      {
        // Here all possible permutations of connecting pairs occur.
        static int ok_count = 0, count = 0;
        ok_count += graph.sanity_check() ? 1 : 0;
        ++count;
        std::cout << "Graph #" << ok_count << '/' << count << std::endl;
        graph.print();

        if (loop_node)
        {
#if 0
          std::cout << "At the end of loop " << *ml << " (" << loop_node->name() << ")." << std::endl;
#endif
          graph.unlink(*loop_node);
#if 0
          std::cout << "Unlinked " << loop_node->name() << ":" << std::endl;
#endif
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
#if 0
      if (ml() >= number_of_nodes)
      {
        std::cout << "Leaving while loop because ml() reached " << ml() << std::endl;
      }
#endif
    }
    int loop = ml.end_of_loop();
    if (loop >= 0)
    {
      Node& loop_node{graph[loop + loop_node_offset]};
#if 0
      std::cout << "At the end of loop " << loop << " (" << loop_node.name() << ")." << std::endl;
#endif
      graph.unlink(loop_node);
#if 0
      std::cout << "Unlinked " << loop_node.name() << ":" << std::endl;
      graph.print();
#endif
      loop_node_offset = loop_node.get_old_loop_node_offset();
    }
  }
}
