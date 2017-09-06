#pragma once

#include "debug.h"
#include "NodePtr.h"
#include "Condition.h"
#include "EdgeType.h"
#include "utils/is_power_of_two.h"
#include <iosfwd>

class Edge;

enum EndPointType {
  undirected,
  tail,                 // >>-- This end point is the tail of the arrow: the beginning of the directed edge.
  head                  // -->> This end point is the head of the arrow: the end of the directed edge.
};

char const* end_point_str(EndPointType end_point_type);
std::ostream& operator<<(std::ostream& os, EndPointType end_point_type);

// One side of a given edge.
class EndPoint
{
 private:
  Edge* m_edge;
  EndPointType m_type;
  NodePtr m_other_node;
  bool m_edge_owner;            // Owns the allocation of m_edge.

 public:
  EndPoint(Edge* edge, EndPointType type, NodePtr const& other_node, bool edge_owner) :
      m_edge(edge), m_type(type), m_other_node(other_node), m_edge_owner(edge_owner) { }
  // Move constructor; needed because memory of a std::vector might need reallocation.
  EndPoint(EndPoint&& end_point) :
    m_edge(end_point.m_edge),
    m_type(end_point.m_type),
    m_other_node(end_point.m_other_node),
    m_edge_owner(end_point.m_edge_owner)
  {
    end_point.m_edge_owner = false;
  }
  inline ~EndPoint();

  // Accessors.
  inline EdgeType edge_type() const;
  Edge* edge() const { return m_edge; }
  EndPointType type() const { return m_type; }
  NodePtr other_node() const { return m_other_node; }
  bool primary_tail(EdgeType edge_type) const;
  bool primary_head(EdgeType edge_type) const;

  friend bool operator==(EndPoint const& end_point1, EndPoint const& end_point2);
  friend std::ostream& operator<<(std::ostream& os, EndPoint const& end_point);
};

class Edge
{
 private:
  EdgeType m_edge_type;
  Condition m_condition;
  NodePtr m_tail_node;    // The Node from where the edge starts:  tail_node ---> head_node.
#ifdef CWDEBUG
  int m_id;             // For debugging purposes.
  static int s_id;
 public:
  int id() const { return m_id; }
#endif

 public:
  Edge(EdgeType edge_type, NodePtr const& tail_node, Condition const& condition) :
      m_edge_type(edge_type),
      m_condition(condition),
      m_tail_node(tail_node)
      COMMA_DEBUG_ONLY(m_id(s_id++))
      {
        ASSERT(utils::is_power_of_two(edge_type.mask));
        Dout(dc::sb_edge(edge_type == edge_sb)|
             dc::asw_edge(edge_type == edge_asw),
             "Creating " << edge_type << " Edge " << m_id << '.');
      }

  EdgeType edge_type() const { return m_edge_type; }
  Condition const& condition() const { return m_condition; }
  char const* name() const { return m_edge_type.name(); }

  inline bool is_conditional() const;
  inline boolean::Expression exists() const;

  friend std::ostream& operator<<(std::ostream& os, Edge const& edge);
  friend bool operator==(Edge const& edge1, Edge const& edge2) { return edge1.m_edge_type == edge2.m_edge_type; }
};

//inline -- now that Edge is defined we can define this.
EdgeType EndPoint::edge_type() const { return m_edge->edge_type(); }
EndPoint::~EndPoint() { if (m_edge_owner) delete m_edge; }
