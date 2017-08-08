#pragma once

#include "NodePtr.h"
#include <vector>
#include <iosfwd>

class EvaluationNodes
{
 public:
  using container_type = std::vector<NodePtr>;
  using iterator = container_type::iterator;
  using const_iterator = container_type::const_iterator;

 private:
  container_type m_node_ptrs;

 public:
  EvaluationNodes() = default;
  EvaluationNodes(EvaluationNodes const& evaluation_nodes) : m_node_ptrs(evaluation_nodes.m_node_ptrs) { }
  EvaluationNodes(EvaluationNodes&& evaluation_nodes) : m_node_ptrs(std::move(evaluation_nodes.m_node_ptrs)) { }
  EvaluationNodes& operator=(EvaluationNodes const& evaluation_nodes) { m_node_ptrs = evaluation_nodes.m_node_ptrs; return *this; }
  void operator=(EvaluationNodes&& evaluation_nodes) { m_node_ptrs = std::move(evaluation_nodes.m_node_ptrs); }

  void swap(EvaluationNodes& evaluation_nodes) { m_node_ptrs.swap(evaluation_nodes.m_node_ptrs); }
  iterator begin() { return m_node_ptrs.begin(); }
  const_iterator begin() const { return m_node_ptrs.begin(); }
  iterator end() { return m_node_ptrs.end(); }
  const_iterator end() const { return m_node_ptrs.end(); }
  void clear() { m_node_ptrs.clear(); }
  void push_back(container_type::value_type const& node_ptr) { m_node_ptrs.push_back(node_ptr); }
  void push_back(container_type::value_type&& node_ptr) { m_node_ptrs.push_back(node_ptr); }

  friend std::ostream& operator<<(std::ostream& os, EvaluationNodes const& evaluation_nodes);
};
