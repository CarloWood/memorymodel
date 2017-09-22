#include "sys.h"
#include "Node.h"
#include "debug_ostream_operators.h"
#include "Context.h"
#include "iomanip_dotfile.h"
#include "utils/is_power_of_two.h"
#include "utils/macros.h"
#include "boolean-expression/BooleanExpression.h"
#include <sstream>

char const* memory_order_str(std::memory_order memory_order)
{
  switch (memory_order)
  {
    case std::memory_order_relaxed:
      return "rlx";
    case std::memory_order_consume:
      return "con";
    case std::memory_order_acquire:
      return "acq";
    case std::memory_order_release:
      return "rel";
    case std::memory_order_acq_rel:   // acquire/release
      return "ar";
    case std::memory_order_seq_cst:   // sequentially consistent
      return "sc";
  }
  return "<UNKNOWN memory order>";
}

std::string NAReadNode::type() const
{
  return "Rna";
}

std::string AtomicReadNode::type() const
{
  std::string result = "R";
  result += memory_order_str(m_read_memory_order);
  return result;
}

std::string NAWriteNode::type() const
{
  return "Wna";
}

std::string AtomicWriteNode::type() const
{
  std::string result = "W";
  result += memory_order_str(m_write_memory_order);
  return result;
}

std::string MutexDeclNode::type() const
{
  return "Wna";
}

std::string MutexLockNode::type() const
{
  return "LK";
}

std::string MutexUnlockNode::type() const
{
  return "UL";
}

std::string RMWNode::type() const
{
  std::string result = "RMW";
  result += memory_order_str(m_memory_order);
  return result;
}

std::string CEWNode::type() const
{
  std::string result = "CEW";
  result += memory_order_str(m_write_memory_order);     // memory order of the RMW operation on success (m_location->tag() == expected).
  result += ',';
  result += memory_order_str(m_fail_memory_order);
  return result;
}

void NAReadNode::print_code(std::ostream& os) const
{
  //FIXME print what we read...
  //os << '<-';
}

void AtomicReadNode::print_code(std::ostream& os) const
{
  //FIXME print what we read...
  //os << '<-';
}

void WriteNode::print_code(std::ostream& os) const
{
  os << '=';
  if (!IOManipDotFile::is_dot_file(os))
    m_evaluation->print_on(os);
}

void MutexDeclNode::print_code(std::ostream& UNUSED_ARG(os)) const
{
}

void MutexReadNode::print_code(std::ostream& UNUSED_ARG(os)) const
{
}

void MutexLockNode::print_code(std::ostream& UNUSED_ARG(os)) const
{
}

void MutexUnlockNode::print_code(std::ostream& UNUSED_ARG(os)) const
{
}

void RMWNode::print_code(std::ostream& os) const
{
  ASSERT(m_evaluation->binary_operator() == additive_ado_add || m_evaluation->binary_operator() == additive_ado_sub);
  os << ((m_evaluation->binary_operator() == additive_ado_add) ? "+=": "-=");
  if (!IOManipDotFile::is_dot_file(os))
    m_evaluation->rhs()->print_on(os);
}

void CEWNode::print_code(std::ostream& os) const
{
  os << "=" << m_expected << '?';
  if (!IOManipDotFile::is_dot_file(os))
    os << m_desired;
}

char const* end_point_str(EndPointType end_point_type)
{
  switch (end_point_type)
  {
    AI_CASE_RETURN(undirected);
    AI_CASE_RETURN(tail);
    AI_CASE_RETURN(head);
  }
  return "UNKNOWN EndPointType";
}

std::ostream& operator<<(std::ostream& os, EndPointType end_point_type)
{
  return os << end_point_str(end_point_type);
}

std::ostream& operator<<(std::ostream& os, Edge const& edge)
{
  os << '{';
  Debug(os << edge.m_id << "; ");
  os << edge.m_edge_type << "; " << *edge.m_tail_node;
  if (!edge.m_condition.is_one())       // Conditional edge?
    os << "; " << edge.m_condition;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, EndPoint const& end_point)
{
  os << '{' << *end_point.m_edge << ", " << end_point.m_type << ", " << *end_point.m_other_node << '}';
  return os;
}

bool operator==(EndPoint const& end_point1, EndPoint const& end_point2)
{
  return *end_point1.m_edge == *end_point2.m_edge &&
         end_point1.m_type == end_point2.m_type &&
         *end_point1.m_other_node == *end_point2.m_other_node;
}
