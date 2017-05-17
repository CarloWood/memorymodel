#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/variant/get.hpp>
#include "cppmem.h"

using namespace AST;

#if 0
BOOST_AUTO_TEST_CASE(type_type_int)
{
  std::string const text{"int"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_int);
}
#endif

BOOST_AUTO_TEST_CASE(type_type_atomic_int)
{
  std::string const text{"atomic_int"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}

#if 0
BOOST_AUTO_TEST_CASE(memory_location_internal)
{
  std::string const text{"internal"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "internal");
}

BOOST_AUTO_TEST_CASE(register_r42)
{
  std::string const text{"r42"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(ARegisterLocation, value.which());
  BOOST_REQUIRE(boost::get<register_location>(value) == 42U);
}

BOOST_AUTO_TEST_CASE(memory_location_r2d2)
{
  std::string const text{"r2d2"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "r2d2");
}
#endif

BOOST_AUTO_TEST_CASE(global_simple)
{
  std::string const text{"atomic_int x;"};

  auto const value = cppmem::parse(text);

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_atomic_int, "x"));
}
