#pragma once

#include "TagCompare.h"
#include <string>
#include <iosfwd>
#include <set>

class Location
{
 public:
  enum Kind
  {
    non_atomic,
    atomic,
    mutex
  };

 private:
  ast::tag m_tag;
  std::string m_name;
  Kind m_kind;

 public:
  Location(int id, std::string const& name, Kind kind) : m_tag(id), m_name(name), m_kind(kind) { }

  // Accessors.
  ast::tag tag() const { return m_tag; }
  std::string const& name() const { return m_name; }
  Kind kind() const { return m_kind; }
  bool operator==(Location const& location) const { return m_tag == location.m_tag; }
  bool operator==(ast::tag variable) const { return m_tag == variable; }

  static std::string kind_str(Kind kind);
  friend std::ostream& operator<<(std::ostream& os, Location const& location);
};

inline std::ostream& operator<<(std::ostream& os, Location::Kind kind) { return os << Location::kind_str(kind); }

struct LocationCompare
{
  bool operator()(Location const& l1, Location const& l2) const
  {
    return TagCompare::less(l1.tag(), l2.tag());
  }
};

using locations_type = std::set<Location, LocationCompare>;
