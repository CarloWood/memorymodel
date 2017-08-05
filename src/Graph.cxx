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

void Graph::new_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Branches const& branches)
{
  DebugMarkUp;
  bool success = NodeBase::add_edge(edge_type, tail_node, head_node, branches);
  if (success)  // Successfully added a new edge.
  {
    Dout(dc::notice, "Added new edge " << *tail_node->get_end_points().back().edge() << " from \"" << *tail_node << "\" to \"" << *head_node << "\".");
    if (edge_type == edge_sb)
    {
      tail_node->sequenced_before();
      head_node->sequenced_after();
    }
  }
}

void Graph::generate_dot_file(std::string const& filename, Context& context) const
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
  std::vector<int> number_of_nodes(context.number_of_threads(), 0);
  for (NodePtr node = m_nodes.begin(); node != m_nodes.end(); ++node)
  {
    int n = ++number_of_nodes[node->thread()->id()];
    if (node->thread()->is_main_thread())
      ++main_thread_nodes;
    else if (n > max_count)
      max_count = n;
  }
  std::vector<int> depth(context.number_of_threads(), 0);
  depth[0] = main_thread_nodes;

  int const fontsize = 14;
  int const edge_label_fontsize = 10;
  double const xscale = 2.0;
  double const yscale = 1.0;

  // Write a .dot file.
  out << "digraph G {\n";
  out << " splines=true;\n";
  out << " overlap=false;\n";
  out << " ranksep = 0.2;\n";
  out << " nodesep = 0.25;\n";
  out << " fontsize=" << fontsize << " fontname=\"Helvetica\" label=\"\";\n";
  for (NodePtr node = m_nodes.begin(); node != m_nodes.end(); ++node)
  {
    Dout(dc::notice, '"' << *node << '"');
    double posx, posy;
    int thread = node->thread()->id();
    posx = 1.0 + xscale * thread;
    posy = 1.0 + yscale * (max_count + --depth[thread]);
    out << "node" << node->name() <<
        " [shape=plaintext, fontname=\"Helvetica\", fontsize=" << fontsize << "]"
        " [label=\"" << node->label(true) << "\", pos=\"" << posx << ',' << posy << "!\"]"
        " [margin=\"0.0,0.0\"][fixedsize=\"true\"][height=\"" << (yscale * 0.25) << "\"][width=\"" << (xscale * 0.6) << "\"];\n";
  }
  for (NodePtr node = m_nodes.begin(); node != m_nodes.end(); ++node)
  {
    //Dout(dc::notice, "LOOP: " << *node);
    for (auto&& end_point : node->get_end_points())
    {
      //Dout(dc::notice, "END-POINT: " << end_point);

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
      Edge* edge = end_point.edge();
      NodePtr tail_node = ((end_point.type() == tail) ? node : end_point.other_node());
      NodePtr head_node = ((end_point.type() == tail) ? end_point.other_node() : node);
      out << "node" << tail_node->name() << " -> "
             "node" << head_node->name() <<
             " [label=<<font color=\"" << color << "\">sb";
#ifdef CWDEBUG
      out << edge->id();
#endif
      if (edge->is_conditional())
        out << ':' << edge->exists().as_html_string();
      out << "</font>>, color=\"" << color << "\", fontname=\"Helvetica\", "
               "fontsize=" << edge_label_fontsize << ", penwidth=1., arrowsize=\"0.8\"];\n";
    }
  }

  conditions_type const& conditions{context.conditions()};
  if (!conditions.empty())
  {
    out <<
        "  { rank = sink;\n"
        "     Legend [shape=none, margin=0, pos=\"1.0," << (1.0 - 1.5 * yscale) << "!\", label=<\n"
        "     <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n"
        "      <TR>\n"
        "       <TD COLSPAN=\"2\"><FONT face=\"Helvetica\"><B>Legend</B></FONT></TD>\n"
        "      </TR>\n";

    for (auto&& condition : conditions)
    {
      out <<
        "      <TR>\n"
        "      <TD>" << condition.second.id_name() << "</TD>\n"
        "      <TD><FONT COLOR=\"black\">";
      condition.first->print_on(out, true);
      out <<
        "</FONT></TD>\n"
        "      </TR>\n";
    }

    out <<
        "     </TABLE>\n"
        "    >];\n"
        "  }\n";
  }

  out << "}\n";
}

#ifdef CWDEBUG
int Edge::s_id = 1;
#endif
