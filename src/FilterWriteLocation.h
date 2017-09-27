#pragma once

#include "Location.h"
#include "Action.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct for_action;
NAMESPACE_DEBUG_CHANNELS_END
#endif

struct FilterWriteLocation
{
  Location const& m_location;
  FilterWriteLocation(Location const& location) : m_location(location) { }
  bool operator()(Action const& action) const
  {
    bool wanted_location = action.location() == m_location;
    bool is_write = action.is_write();
    if (!wanted_location)
      Dout(dc::for_action, "Skipping node " << action.name() << " because it concerns location " << action.location().name() << " instead of " << m_location.name() << ".");
    else if (!is_write)
      Dout(dc::for_action, "Skipping node " << action.name() << " because it isn't a write.");
    return wanted_location && is_write;
  }
};


