#pragma once

#include "Location.h"
#include "Action.h"

struct FilterLocation
{
  Location const& m_location;
  FilterLocation(Location const& location) : m_location(location) { }
  bool operator()(Action const& action) { return action.location() == m_location; }
};


