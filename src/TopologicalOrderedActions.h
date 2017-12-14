#pragma once

#include "utils/Vector.h"

class Action;

struct TopologicalOrderedActionsCategory;
using TopologicalOrderedActions = utils::Vector<Action*, utils::VectorIndex<TopologicalOrderedActionsCategory>>;
using TopologicalOrderedActionsIndex = TopologicalOrderedActions::index_type;

std::ostream& operator<<(std::ostream& os, TopologicalOrderedActionsIndex index);
