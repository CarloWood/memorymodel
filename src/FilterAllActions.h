#pragma once
#include "Action.h"

struct FilterAllActions
{
  bool operator()(Action const&) { return true; }
};
