#include "sys.h"
#include "Graph.h"
#include "Context.h"
#include "debug.h"
#include "utils/macros.h"
#include "iomanip_dotfile.h"
#include "iomanip_html.h"
#include <stdexcept>
#include <fstream>

void Graph::new_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Condition const& condition)
{
  tail_node->add_edge_to(edge_type, &*head_node, boolean::Expression{condition.boolean_product()});
#ifdef CWDEBUG
  Dout(dc::notice|continued_cf,
      "Graph::new_edge: added new edge " << *tail_node->get_end_points().back().edge() <<
      " from \"" << *tail_node << "\" to \"" << *head_node << "\"");
  if (condition.conditional())
    Dout(dc::continued, " with condition " << condition);
  Dout(dc::finish, ".");
#endif
  if (edge_type == edge_sb)
  {
    tail_node->sequenced_before();
    head_node->sequenced_after();
  }
}

void Graph::generate_dot_file(
    std::string const& filename,
    TopologicalOrderedActions const& topological_ordered_actions,
    boolean::Expression const& valid,
    boolean::Expression const& invalid) const
{
  DoutEntering(dc::notice, "Graph::generate_dot_file(\"" << filename << "\"");

  std::ofstream out;
  out.open(filename);
  if (!out)
  {
    std::cerr << "Failed to open dot file." << std::endl;
    return;
  }

  int max_count = 0;
  utils::Vector<int, SequenceNumber> vertical_position;                         // As function of sequence number.
  std::vector<int> pos_of_last_node(Context::instance().number_of_threads(), -1);               // Position of last node of a thread as function of thread id.
  std::vector<Action*> action_of_last_node(Context::instance().number_of_threads(), nullptr);   // Last node of a thread as function of thread id.
  for (Action* action : topological_ordered_actions)
  {
    // Action are ordered as follows:
    // pos: seq.nr:
    //  0   1
    //  1   2
    //  2   3
    //  3        4   7
    //  4        5   8   10
    //  5        6   9
    //  6            11
    //  7   12
    //  8   13
    // where the node of each thread is as high as possible,
    // but below (vertically) nodes that they are sequenced after.
    // Ie, here, 4 and 7 are sequenced after 3, 10 is sequenced
    // after 7 and 12 is sequenced after 11.
    Thread::id_type thread_id = action->thread()->id();
    int pos = ++pos_of_last_node[thread_id];
    for (Thread::id_type tid = 0; tid < Context::instance().number_of_threads(); ++tid)
      if (tid != thread_id && action_of_last_node[tid] && action_of_last_node[tid]->is_sequenced_before(*action))
        pos = std::max(pos, pos_of_last_node[tid] + 1);
    pos_of_last_node[thread_id] = pos;
    action_of_last_node[thread_id] = action;
    vertical_position.push_back(pos);
    max_count = std::max(max_count, pos);
  }

  int const fontsize = 14;
  int const edge_label_fontsize = 10;
  double const xscale = 2.0;
  double const yscale = 1.0;

  // Write a .dot file.
  out << dotfile;
  out << "digraph G {\n";
  out << " splines=true;\n";
  out << " overlap=false;\n";
  out << " ranksep = 0.2;\n";
  out << " nodesep = 0.25;\n";
  out << " fontsize=" << fontsize << " fontname=\"Helvetica\" label=\"\";\n";
  for (NodePtr node{m_nodes.begin()}; node != m_nodes.end(); ++node)
  {
    Dout(dc::notice, '"' << *node << '"');
    double posx, posy;
    int thread = node->thread()->id();
    posx = 1.0 + xscale * thread;
    posy = 1.0 + yscale * (max_count - vertical_position[node->sequence_number()]);
    out << "node" << node->name() <<
        " [shape=plaintext, fontname=\"Helvetica\", fontsize=" << fontsize << "]"
        " [label=\"";
#ifdef CWDEBUG
    // Print sequence number of node.
    out << node->sequence_number() << ':';
#endif
    out << *node << "\", pos=\"" << posx << ',' << posy << "!\"]"
        " [margin=\"0.0,0.0\"][fixedsize=\"true\"][height=\"" << (yscale * 0.25) << "\"][width=\"" << (xscale * 0.6) << "\"];\n";
  }
  for (NodePtr node{m_nodes.begin()}; node != m_nodes.end(); ++node)
  {
    //Dout(dc::notice, "LOOP: " << *node);
    for (auto&& end_point : node->get_end_points())
    {
      //Dout(dc::notice, "END-POINT: " << end_point);

      if (*end_point.other_node() < *node)
        continue;

      std::string color = edge_color(end_point.edge_type());
      Edge* edge = end_point.edge();
      Action const* tail_node = ((end_point.type() == tail) ? &*node : end_point.other_node());
      Action const* head_node = ((end_point.type() == tail) ? end_point.other_node() : &*node);
      out << "node" << tail_node->name() << " -> "
             "node" << head_node->name() <<
             " [label=<<font color=\"" << color << "\">" << edge->name();
#ifdef CWDEBUG
      // Print id of the edge to show in what order edges have been created.
      out << edge->id();
#endif
      if (edge->is_conditional())
        out << ':' << edge->condition().as_html_string();
      out << "</font>>, color=\"" << color << "\", fontname=\"Helvetica\", "
               "fontsize=" << edge_label_fontsize << ", penwidth=1., arrowsize=\"0.8\"];\n";
    }
  }

  conditionals_type const& conditionals{Context::instance().conditionals()};
  if (!conditionals.empty())
  {
    out <<
        "  { rank = sink;\n"
        "     Legend [shape=none, margin=0, pos=\"1.0," << (1.0 - 1.5 * yscale) << "!\", label=<\n"
        "     <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n"
        "      <TR>\n"
        "       <TD COLSPAN=\"2\"><FONT face=\"Helvetica\"><B>Conditions</B></FONT></TD>\n"
        "      </TR>\n";

    for (auto&& conditional : conditionals)
    {
      out <<
        "      <TR>\n"
        "      <TD>" << conditional.second.id_name() << "</TD>\n"
        "      <TD><FONT COLOR=\"black\">";
      out << html << *conditional.first;
      out <<
        "</FONT></TD>\n"
        "      </TR>\n";
    }
    if (!valid.is_one())
    {
      out <<
        "      <TR>\n"
        "      <TD>Valid</TD>\n"
        "      <TD><FONT COLOR=\"black\">";
      out << valid.as_html_string();
      out <<
        "</FONT></TD>\n"
        "      </TR>\n";
    }
    if (!invalid.is_zero())
    {
      out <<
        "      <TR>\n"
        "      <TD>Invalid</TD>\n"
        "      <TD><FONT COLOR=\"black\">";
      out << invalid.as_html_string();
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
