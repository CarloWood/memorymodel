#include "sys.h"
#include "NodePtr.h"
#include "Node.h"

bool NodeOrder::operator()(std::unique_ptr<NodeBase> const& ptr1, std::unique_ptr<NodeBase> const& ptr2) const
{
  return *ptr1 < *ptr2;
}

template<typename NODE>
NODE const* NodePtr::get() const
{
  return dynamic_cast<NODE const*>(m_iterator->get());
}

template WriteNode const* NodePtr::get() const;
template RMWNode const* NodePtr::get() const;
template CEWNode const* NodePtr::get() const;
