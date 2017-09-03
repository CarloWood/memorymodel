#pragma once

#include "ast_tag.h"
#include <cassert>

struct TagCompare
{
  bool operator()(ast::tag const& t1, ast::tag const& t2) const
  {
    assert(t1.id != -1 && t2.id != -1);
    return t1.id < t2.id;
  }
};

