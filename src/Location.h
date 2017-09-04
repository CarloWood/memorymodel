#pragma once

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
  int m_id;
  std::string m_name;
  Kind m_kind;

 public:
  Location(int id, std::string const& name, Kind kind) : m_id(id), m_name(name), m_kind(kind) { }

  // Accessors.
  int id() const { return m_id; }
  std::string const& name() const { return m_name; }
  Kind kind() const { return m_kind; }

  static std::string kind_str(Kind kind);
  friend std::ostream& operator<<(std::ostream& os, Location const& location);
};

inline std::ostream& operator<<(std::ostream& os, Location::Kind kind) { return os << Location::kind_str(kind); }

struct LocationCompare
{
  bool operator()(Location const& l1, Location const& l2) const
  {
    return l1.id() < l2.id();
  }
};

using locations_type = std::set<Location, LocationCompare>;
