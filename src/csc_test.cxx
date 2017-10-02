#include "sys.h"
#include "debug.h"
#include "utils/MultiLoop.h"
#include <iostream>
#include <array>
#include <iomanip>
#include <bitset>

int constexpr number_of_threads = 4;
int constexpr nodes_per_thread = 4;
int constexpr number_of_nodes = nodes_per_thread * number_of_threads;

int thread(int node)
{
  return node / nodes_per_thread;
}

int column(int node)
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
  int m_index;          // Index into the graph array.
  int m_linked;         // The index of the Node that we're linked with (or unused).
  mask_type m_sequenced_after;
  static int s_connected_nodes;

 public:
  Node() : m_linked(unused), m_sequenced_after(0) { }
  Node(unsigned int linked) : m_linked(linked) { }
  Node(int linked) : m_linked(linked) { }

 private:
  void init(int index)
  {
    m_index = index;
  }

  void link_to(Node const& linked_node)
  {
    ASSERT(m_linked == unused);
    m_linked = linked_node.m_index;
    int col = column(m_index);
    if (col > 0)
      m_sequenced_after = (this - 1)->m_sequenced_after | to_mask(m_index - 1);
    //else
    //  ASSERT(m_sequenced_after == 0);
    m_sequenced_after |= linked_node.m_sequenced_after;
  }

 public:
  int link(Node& node)
  {
    link_to(node);
    ++s_connected_nodes;
    if (this != &node)
    {
      node.link_to(*this);
      ++s_connected_nodes;
    }
    return s_connected_nodes;
  }

  bool is_unused() const { return m_linked == unused; }
  bool is_used() const { return m_linked != unused; }
  bool is_linked_to(int node) const { return m_linked == node; }
  void reset()
  {
    --s_connected_nodes;
    m_linked = unused;
    m_sequenced_after = sequenced_after(m_index);
  }

  mask_type mask() const { return to_mask(m_index); }
  mask_type thread_mask() const { return same_thread(m_index); }
  mask_type sequenced_after_mask() const { return sequenced_after(m_index); }

  std::string name() const
  {
    std::string result;
    result += char('A' + thread(m_index));
    result += char('0' + column(m_index));
    return result;
  }

  friend std::ostream& operator<<(std::ostream& os, Node const& node)
  {
    return os << node.m_linked;
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
      std::cout << f << ": ";
      int linked = graph[n].m_linked;
      char linked_thread = char('A' + thread(graph[n].m_linked));
      int linked_column = column(linked);
#if 1
      if (graph[n].is_unused())                 // Not linked?
        std::cout << " *";
      else if (graph[n].is_linked_to(n))        // Linked to itself?
        std::cout << " |";
      else
        std::cout << linked_thread << linked_column;
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

  std::getchar();
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  // Each bit in mask_type represents a node.
  ASSERT(number_of_nodes <= sizeof(mask_type) * 8);

  int constexpr number_of_loops = number_of_nodes;
  Graph graph;

  bool did_break;
  for (MultiLoop ml(number_of_loops); !ml.finished(); ml.next_loop())
  {
    for (; ml() < number_of_nodes; ++ml)
    {
      did_break = false;
      Node& from_node{graph[*ml]};
      Node& to_node{graph[ml()]};
      std::cout << "At top of loop " << *ml << " (with value " << ml() << "); considering " << from_node.name() << " <--> " << to_node.name() << '.' << std::endl;
      if (from_node.is_used() || to_node.is_used())
      {
        if (from_node.is_used())
          std::cout << "Skipping " << from_node.name() << " because it is already connected (to " << graph.linked_node(from_node).name() << ")." << std::endl;
        else
          std::cout << "Skipping connection to " << to_node.name() << " because it is already connected (to " << graph.linked_node(to_node).name() << ")." << std::endl;
        ml.breaks(0);                // Normal continue (of current loop).
        did_break = true;
        break;                       // Return control to MultiLoop.
      }
      int connected_nodes = from_node.link(to_node);
      bool fully_connected = connected_nodes == number_of_nodes;

      std::cout << "Made connection for loop " << *ml << " (" << from_node.name() << " <--> " << to_node.name();
      if (fully_connected)
        std::cout << "; we are now fully connected";
      std::cout << ")." << std::endl;
      graph.print();

      if (fully_connected)
      {
        // Here all possible permutations of connecting pairs occur.
        //graph.print();

        if (ml.inner_loop())
        {
          std::cout << "At the end of loop " << *ml << " (" << from_node.name() << ")." << std::endl;
          graph.unlink(from_node);
          std::cout << "Unlinked " << from_node.name() << ":" << std::endl;
          graph.print();
        }
        else
        {
          ml.breaks(1);
          did_break = true;
          break;
        }
      }
    }
    if (*ml > 0 && !did_break)
    {
      Node& from_node{graph[*ml - 1]};
      std::cout << "At the end of loop " << (*ml - 1) << " (" << from_node.name() << ")." << std::endl;
      graph.unlink(from_node);
      std::cout << "Unlinked " << from_node.name() << ":" << std::endl;
      graph.print();
    }
    did_break = false;
  }
}
