#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include "sys.h"
#include <boost/test/unit_test.hpp>
#include <boost/variant/get.hpp>
#include "cppmem.h"

using namespace AST;

BOOST_AUTO_TEST_CASE(type_type_int)
{
  std::string const text{"int"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_int);
}

BOOST_AUTO_TEST_CASE(type_type_atomic_int)
{
  std::string const text{"atomic_int"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}

BOOST_AUTO_TEST_CASE(memory_location_internal)
{
  std::string const text{"internal"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "internal");
}

BOOST_AUTO_TEST_CASE(register_r42)
{
  std::string const text{"r42"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(ARegisterLocation, value.which());
  BOOST_REQUIRE(boost::get<register_location>(value) == 42U);
}

BOOST_AUTO_TEST_CASE(memory_location_r2d2)
{
  std::string const text{"r2d2"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "r2d2");
}

BOOST_AUTO_TEST_CASE(global_simple)
{
  std::string const text{"atomic_int x;"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_atomic_int, "x"));
}

BOOST_AUTO_TEST_CASE(global_init)
{
  std::string const text{"int int3 = 123;"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_int, "int3", 123));
}

BOOST_AUTO_TEST_CASE(scope_anything)
{
  std::string const text{"{\n  int y = 4 ;\n}\n"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AScope, value.which());
  BOOST_REQUIRE(boost::get<scope>(value) == "int y = 4 ");
}

BOOST_AUTO_TEST_CASE(scope_recursive)
{
  std::string const text{
    "{\n"
    "  int y = 4;\n"
    "  atomic_int x;\n"
    "  {\n"
    "    { x.store(1); }\n"
    "    r0 = y.load(std::memory_order_relaxed);\n"
    "  }\n"
    "}\n"
  };

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AScope, value.which());
  //BOOST_REQUIRE(boost::get<scope>(value) == "int y = 4;");
}

BOOST_AUTO_TEST_CASE(function_wrlock)
{
  std::string const text{"void wrlock()\n{\n  int y = 4;\n}\n"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AFunction, value.which());
  AST::function const& f = boost::get<function>(value);
  std::stringstream ss;
  ss << f;
  BOOST_REQUIRE(ss.str() == "void wrlock() { int y = 4; }");
}
