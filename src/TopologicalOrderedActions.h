#pragma once

#include "utils/Vector.h"

class Action;

namespace ordering_category {
struct Topological;
} // namespace ordering_category

using SequenceNumber = utils::VectorIndex<ordering_category::Topological>;
using TopologicalOrderedActions = utils::Vector<Action*, SequenceNumber>;

std::ostream& operator<<(std::ostream& os, SequenceNumber index);
