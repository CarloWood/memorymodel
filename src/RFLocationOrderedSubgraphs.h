#pragma once

#include "utils/Vector.h"

class DirectedSubgraph;

namespace ordering_category {
struct RFLocation;        // All memory locations with at least one ReadFrom edge.
} // namespace ordering_category

using RFLocation = utils::VectorIndex<ordering_category::RFLocation>;
using RFLocationOrderedSubgraphs = utils::Vector<DirectedSubgraph const*, RFLocation>;

std::ostream& operator<<(std::ostream& os, RFLocation index);
