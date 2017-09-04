#pragma once

#include <iosfwd>

namespace ast {

struct tag
{
  int id;
  tag() { }
  explicit tag(int id_) : id(id_) { }

  friend std::ostream& operator<<(std::ostream& os, tag const& tag);
  friend bool operator!=(tag const& tag1, tag const& tag2) { return tag1.id != tag2.id; }
};

} // namespace ast
