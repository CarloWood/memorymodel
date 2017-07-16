#include "sys.h"
#include "Graph.h"
#include "Context.h"
#include "debug.h"
#include "utils/macros.h"
#include <stdexcept>
#include <fstream>

std::ostream& operator<<(std::ostream& os, Thread const& thread)
{
  os << "{Thread: m_id = " << thread.m_id << "}";
  return os;
}

char const* access_str(Access access)
{
  switch (access)
  {
    AI_CASE_RETURN(ReadAccess);
    AI_CASE_RETURN(WriteAccess);
    AI_CASE_RETURN(MutexDeclaration);
    AI_CASE_RETURN(MutexLock1);
    AI_CASE_RETURN(MutexLock2);
    AI_CASE_RETURN(MutexUnlock1);
    AI_CASE_RETURN(MutexUnlock2);
  }
  throw std::runtime_error("Unknown Access type");
}

std::ostream& operator<<(std::ostream& os, Access access)
{
  return os << access_str(access);
}

void Graph::scope_start(bool is_thread)
{
  DoutEntering(dc::notice, "Graph::scope_start('is_thread' = " << is_thread << ")");
  m_threads.push(is_thread);
  if (is_thread)
  {
    m_beginning_of_thread = true;
    m_current_thread = Thread::create_new_thread(m_next_thread_id, m_current_thread);
    DebugMarkDown;
    Dout(dc::notice, "Created " << m_current_thread << '.');
  }
}

void Graph::scope_end()
{
  bool is_thread = m_threads.top();
  DoutEntering(dc::notice, "Graph::scope_end() [is_thread = " << is_thread << "].");
  m_threads.pop();
  if (is_thread)
  {
    DebugMarkUp;
    Dout(dc::notice, "Joined thread " << m_current_thread << '.');
    m_current_thread = m_current_thread->parent_thread();
  }
}

void Graph::new_edge(EdgeType edge_type, node_iterator const& tail_node, node_iterator const& head_node)
{
  DoutEntering(dc::notice, "new_edge(" << edge_type << ", " << *tail_node << ", " <<  *head_node << ").");
  DebugMarkUp;
  Node::add_edge(edge_type, tail_node, head_node);
  if (1)        // Successfully added a new edge.
  {
    if (edge_type == edge_sb || 1) // FIXME: remove the '1' when we don't abuse edge_asw for red anymore
    {
      tail_node->sequenced_before(*head_node);
      head_node->sequenced_after(*tail_node);
    }
  }
}

void Graph::generate_dot_file(std::string const& filename) const
{
  std::ofstream out;
  out.open(filename);
  if (!out)
  {
    std::cerr << "Failed to open dot file." << std::endl;
    return;
  }

  // Count number of nodes per thread.
  int max_count = 0;
  int main_thread_nodes = 0;
  std::vector<int> number_of_nodes(number_of_threads(), 0);
  for (auto&& node : m_nodes)
  {
    int n = ++number_of_nodes[node.thread()->id()];
    if (node.thread()->is_main_thread())
      ++main_thread_nodes;
    else if (n > max_count)
      max_count = n;
  }
  std::vector<int> depth(number_of_threads(), 0);
  depth[0] = main_thread_nodes;

  // Write a .dot file.
  out << "digraph G {\n";
  out << " splines=true;\n";
  out << " overlap=false;\n";
  out << " ranksep = 0.2;\n";
  out << " nodesep = 0.25;\n";
  out << " fontsize=10 fontname=\"Helvetica\" label=\"\";\n";
  for (auto&& node : m_nodes)
  {
    Dout(dc::notice, '"' << node << '"');
    double posx, posy;
    int thread = node.thread()->id();
    posx = 1.0 + 1.5 * thread;
    posy = 1.0 + 0.7 * (max_count + --depth[thread]);
    out << "node" << node.name() <<
        " [shape=plaintext, fontname=\"Helvetica\", fontsize=10]"
        " [label=\"" << node.label(true) << "\", pos=\"" << posx << ',' << posy << "!\"]"
        " [margin=\"0.0,0.0\"][fixedsize=\"true\"][height=\"0.200000\"][width=\"0.900000\"];\n";
  }
  for (node_iterator node = m_nodes.begin(); node != m_nodes.end(); ++node)
  {
    Dout(dc::notice, "LOOP: " << *node);
    for (auto&& end_point : node->get_end_points())
    {
      Dout(dc::notice, "END-POINT: " << end_point);

      if (*end_point.other_node() < *node)
        continue;

      std::string color;
      switch (end_point.edge_type())
      {
        case edge_sb:
          color = "black";
          break;
        default:
          color = "red";
          break;
      }
      node_iterator tail_node = ((end_point.type() == tail) ? node : end_point.other_node());
      node_iterator head_node = ((end_point.type() == tail) ? end_point.other_node() : node);
      out << "node" << tail_node->name() << " -> "
             "node" << head_node->name() <<
             " [label=<<font color=\"" << color << "\">sb</font>>, "
               "color=\"" << color << "\", fontname=\"Helvetica\", "
               "fontsize=10, penwidth=1., arrowsize=\"0.8\"];\n";
    }
  }
  out << "}\n";
}
