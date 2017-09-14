#pragma once
#include "Edge.h"

struct FollowSBHeads
{
  bool operator()(EndPoint const& end_point) const
  {
    return end_point.edge()->edge_type() == edge_sb && end_point.type() == head;
  }
};
