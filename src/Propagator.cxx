#include "sys.h"
#include "Propagator.h"
#include "Action.h"

bool Propagator::rf_acq_but_not_rel() const
{
  ASSERT(!m_edge_is_rf || m_current_is_write);
  return m_edge_is_rf && m_child_action->is_acquire() && !m_current_action->is_release();
}

bool Propagator::rf_rel_acq() const
{
  ASSERT(!m_edge_is_rf || m_current_is_write);
  return m_edge_is_rf && m_child_action->is_acquire() && m_current_action->is_release();
}

bool Propagator::is_write_rel_to(RFLocation location) const
{
  return m_current_is_write && m_current_location == location && m_current_action->is_release();
}

bool Propagator::is_store_to(RFLocation location) const
{
  return m_current_is_write && m_current_location == location && m_current_action->kind() == Action::atomic_store;
}

std::ostream& operator<<(std::ostream& os, Propagator const& propagator)
{
  os << '{';
  os << propagator.m_child;
  if (propagator.m_edge_is_rf)
    os << "<-rf-";
  else
    os << "<----";
  if (propagator.m_current_is_write)
  {
    os << "(W";
    if (propagator.m_edge_is_rf)
      os << ' ' << propagator.m_current_location;
    os << ")";
  }
  os << propagator.m_current_node << ";" << propagator.m_condition;
  return os << '}';
}
