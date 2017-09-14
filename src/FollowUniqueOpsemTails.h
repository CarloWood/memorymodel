#pragma once

#include "Edge.h"

struct FollowUniqueOpsemTails
{
  bool operator()(EndPoint const& end_point) const
  {
    return end_point.edge()->is_opsem() && end_point.primary_tail(edge_mask_opsem);
  }
};
