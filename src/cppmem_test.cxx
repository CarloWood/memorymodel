#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include "sys.h"
#include <boost/test/unit_test.hpp>
#include <boost/variant/get.hpp>
#include "cppmem_parser.h"

using namespace AST;

#define MIN_TEST 0
#define MAX_TEST 15

#define type_type_int_nr                0
#define type_type_atomic_int_nr         1
#define type_comment1_nr                2
#define type_comment2_nr                3
#define memory_location_internal_nr     4
#define register_r42_nr                 5
#define memory_location_r2d2_nr         6
#define memory_location_r_comment1_nr   7
#define memory_location_r_comment2_nr   8
#define global_simple_nr                9
#define global_init_nr                 10
#define type_comment3_nr               11
#define scope_anything_nr              12
#define scope_recursive_nr             13
#define function_wrlock_nr             14
#define threads_simple_nr              15

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

#if DO_TEST(type_comment1)
BOOST_AUTO_TEST_CASE(type_comment1)
{
  std::string const text{"/* */atomic_int/* */"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AType, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}
#endif

#if DO_TEST(type_comment2)
BOOST_AUTO_TEST_CASE(type_comment2)
{
  std::string const text{"atomic_/**/int"};

  AST::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
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

#if DO_TEST(memory_location_r_comment1)
BOOST_AUTO_TEST_CASE(memory_location_r_comment1)
{
  std::string const text{"r/*1*/0"};
  // Check that the C comment is interpreted in the same way as whitespace would be.
  std::string const wrong_text{"r0"};
  std::string const right_text{"r 0"};

  AST::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
  BOOST_CHECK_THROW(cppmem::parse(right_text, value), std::domain_error);

  // The behavior when text would be interpreted as wrong_text is different:
  BOOST_CHECK_NO_THROW(cppmem::parse(wrong_text, value));
  BOOST_REQUIRE_EQUAL(ARegisterLocation, value.which());
  BOOST_REQUIRE(boost::get<register_location>(value) == 0U);
}
#endif

#if DO_TEST(memory_location_r_comment2)
BOOST_AUTO_TEST_CASE(memory_location_r_comment2)
{
  std::string const text{"r4/*2*/_"};
  // Check that the C comment is interpreted in the same way as whitespace would be.
  std::string const wrong_text{"r4_"};
  std::string const right_text{"r4 _"};

  AST::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
  BOOST_CHECK_THROW(cppmem::parse(right_text, value), std::domain_error);

  // The behavior when text would be interpreted as wrong_text is different:
  BOOST_CHECK_NO_THROW(cppmem::parse(wrong_text, value));
  BOOST_REQUIRE_EQUAL(AMemoryLocation, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == wrong_text);
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

#if DO_TEST(type_comment3)
BOOST_AUTO_TEST_CASE(type_comment3)
{
  std::string const text{"atomic_int/**/x = 3;"};

  AST::nonterminal value;
  BOOST_CHECK_NO_THROW(cppmem::parse(text, value));

  BOOST_REQUIRE_EQUAL(AGlobal, value.which());
  BOOST_REQUIRE(boost::get<global>(value) == global(type_atomic_int, "x", 3));
}
#endif

#if DO_TEST(scope_anything)
BOOST_AUTO_TEST_CASE(scope_anything)
{
  std::string const text{"{\n  int y = 4 ;\n}\n"};

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AScope, value.which());
  //std::cout << "Result: \"" << boost::get<scope>(value) << "\"." << std::endl;
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
  AST::scope const& s = boost::get<scope>(value);
  std::stringstream ss;
  ss << s;
  //std::cout << "s = \"" << ss.str() << "\"." << std::endl;
  BOOST_REQUIRE(ss.str() == "{ int y = 4;atomic_int x;{ { x.store(1); }r0 = y.load(std::memory_order_relaxed); } }");
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
  //std::cout << "s = \"" << ss.str() << "\"." << std::endl;
  BOOST_REQUIRE(ss.str() == "void wrlock() { int y = 4; }");
}
#endif

#if DO_TEST(threads_simple)
BOOST_AUTO_TEST_CASE(threads_simple)
{
  std::string const text{
      "{{{\n"
    "    x.store(1, mo_release);\n"
    "  |||\n"
    "    {\n"
    "      r1 = x.load(mo_acquire).readsvalue(1);\n"
    "      y.store(1, mo_release);\n"
    "    }\n"
    "  |||\n"
    "      r2 = y.load(mo_acquire).readsvalue(1);\n"
    "      r3 = x.load(mo_relaxed);\n"
    "  }}}\n"
  };

  AST::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(AThreads, value.which());
  AST::threads const& t = boost::get<threads>(value);
  std::stringstream ss;
  ss << t;
  //std::cout << "s = \"" << ss.str() << "\"." << std::endl;
  //BOOST_REQUIRE(ss.str() == "void wrlock() { int y = 4; }");
}
#endif
