#pragma once
#include "Edge.h"

struct FollowOpsemHeads
{
  bool operator()(EndPoint const& end_point) const
  {
    return end_point.edge()->is_opsem() && end_point.type() == head;
  }
};
