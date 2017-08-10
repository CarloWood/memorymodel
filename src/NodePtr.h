#pragma once

#include <set>
#include <memory>
#include <iosfwd>

class NodeBase;

struct NodeOrder
{
  bool operator()(std::unique_ptr<NodeBase> const& ptr1, std::unique_ptr<NodeBase> const& ptr2) const;
};

class NodePtr
{
 public:
  using container_type = std::set<std::unique_ptr<NodeBase>, NodeOrder>;

 private:
  using iterator_type = container_type::iterator;
  iterator_type m_iterator;

 public:
  NodePtr(iterator_type const& iterator) : m_iterator(iterator) { }
  NodeBase const* operator->() const { return m_iterator->get(); }
  NodeBase const& operator*() const { return *m_iterator->get(); }
  iterator_type get_iterator() const { return m_iterator; }
  bool operator==(iterator_type const& iter) const { return m_iterator == iter; }
  bool operator!=(iterator_type const& iter) const { return m_iterator != iter; }
  NodePtr& operator++() { ++m_iterator; return *this; }

  template<typename NODE>
    NODE const* get() const;
};
