#include "sys.h"
#include "Location.h"
#include "utils/macros.h"
#include <iostream>

//static
std::string Location::kind_str(Kind kind)
{
  switch (kind)
  {
    AI_CASE_RETURN(non_atomic);
    AI_CASE_RETURN(atomic);
    AI_CASE_RETURN(mutex);
  }
  return "UNKNOWN kind";
}

std::ostream& operator<<(std::ostream& os, Location const& location)
{
  os << "{id:" << location.m_id << ", \"" << location.m_name << "\", " << location.m_kind << '}';
  return os;
}
