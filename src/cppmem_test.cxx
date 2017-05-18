#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include "sys.h"
#include <boost/test/unit_test.hpp>
#include <boost/variant/get.hpp>
#include "cppmem.h"

using namespace AST;

#define MIN_TEST 0
#define MAX_TEST 9

#define type_type_int_nr                0
#define type_type_atomic_int_nr         1
#define memory_location_internal_nr     2
#define register_r42_nr                 3
#define memory_location_r2d2_nr         4
#define global_simple_nr                5
#define global_init_nr                  6
#define scope_anything_nr               7
#define scope_recursive_nr              8
#define function_wrlock_nr              9

#if MAX_TEST < MIN_TEST
#undef MAX_TEST
#define MAX_TEST MIN_TEST
#endif

#define DO_TEST(x) (x##_nr >= MIN_TEST && x##_nr <= MAX_TEST)

#if DO_TEST(type_type_int)
BOOST_AUTO_TEST_CASE(type_type_int)
{
  std::string const text{"int"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_int);
}
#endif

#if DO_TEST(type_type_atomic_int)
BOOST_AUTO_TEST_CASE(type_type_atomic_int)
{
  std::string const text{"atomic_int"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}
#endif

#if DO_TEST(memory_location_internal)
BOOST_AUTO_TEST_CASE(memory_location_internal)
{
  std::string const text{"internal"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "internal");
}
#endif

#if DO_TEST(register_r42)
BOOST_AUTO_TEST_CASE(register_r42)
{
  std::string const text{"r42"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(ARegisterLocation, value.which());
  BOOST_REQUIRE(boost::get<register_location>(value) == 42U);
}
#endif

#if DO_TEST(memory_location_r2d2)
BOOST_AUTO_TEST_CASE(memory_location_r2d2)
{
  std::string const text{"r2d2"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "r2d2");
}
#endif

#if DO_TEST(global_simple)
BOOST_AUTO_TEST_CASE(global_simple)
{
  std::string const text{"atomic_int x;"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_atomic_int, "x"));
}
#endif

#if DO_TEST(global_init)
BOOST_AUTO_TEST_CASE(global_init)
{
  std::string const text{"int int3 = 123;"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_int, "int3", 123));
}
#endif

#if DO_TEST(scope_anything)
BOOST_AUTO_TEST_CASE(scope_anything)
{
  std::string const text{"{\n  int y = 4 ;\n}\n"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AScope, value.which());
  BOOST_REQUIRE(boost::get<scope>(value) == "int y = 4 ");
}
#endif

#if DO_TEST(scope_recursive)
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
#endif

#if DO_TEST(function_wrlock)
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
#endif
