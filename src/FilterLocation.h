#pragma once

#include "Location.h"
#include "Action.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct for_action;
NAMESPACE_DEBUG_CHANNELS_END
#endif

struct FilterLocation
{
  Location const& m_location;
  FilterLocation(Location const& location) : m_location(location) { }
  bool operator()(Action const& action) const
  {
    bool result = action.location() == m_location;
    if (!result)
      Dout(dc::for_action, "Skipping node " << action.name() << " because it concerns location " << action.location().name() << " instead of " << m_location.name() << ".");
    return result;
  }
};


