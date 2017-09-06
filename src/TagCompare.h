#pragma once

#include "ast_tag.h"
#include "debug.h"

struct TagCompare
{
  static bool less(ast::tag const& t1, ast::tag const& t2)
  {
    ASSERT(t1.id != -1 && t2.id != -1);
    return t1.id < t2.id;
  }
  bool operator()(ast::tag const& t1, ast::tag const& t2) const
  {
    return less(t1, t2);
  }
};
