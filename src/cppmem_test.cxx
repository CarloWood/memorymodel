#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_NO_MAIN

#include "sys.h"
#include "debug.h"
#include "cppmem_parser.h"
#include <boost/test/unit_test.hpp>
#include <boost/variant/get.hpp>
#include <iostream>

using namespace ast;

#define MIN_TEST 0
#define MAX_TEST 17

#define type_type_int_nr                0
#define type_type_atomic_int_nr         1
#define type_comment1_nr                2
#define type_comment2_nr                3
#define memory_location_internal_nr     4
#define register_r42_nr                 5
#define memory_location_r2d2_nr         6
#define memory_location_r_comment1_nr   7
#define memory_location_r_comment2_nr   8
#define vardecl_simple_nr               9
#define vardecl_init_nr                10
#define type_comment3_nr               11
#define scope_vardecl_nr               12
#define scope_assignment_nr            13
#define scope_recursive_nr             14
#define function_wrlock_nr             15
#define threads_simple_nr              16
#define function_main_nr               17

#if MAX_TEST < MIN_TEST
#undef MAX_TEST
#define MAX_TEST MIN_TEST
#endif

void find_and_replace(std::string& str, std::string const& old_str, std::string const& new_str)
{
  std::string::size_type pos = 0u;
  while ((pos = str.find(old_str, pos)) != std::string::npos)
  {
    str.replace(pos, old_str.length(), new_str);
    pos += new_str.length();
  }
}

#define DO_TEST(x) (x##_nr >= MIN_TEST && x##_nr <= MAX_TEST)

#if DO_TEST(type_type_int)
BOOST_AUTO_TEST_CASE(type_type_int)
{
  std::string const text{"int"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_type, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_int);
}
#endif

#if DO_TEST(type_type_atomic_int)
BOOST_AUTO_TEST_CASE(type_type_atomic_int)
{
  std::string const text{"atomic_int"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_type, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}
#endif

#if DO_TEST(type_comment1)
BOOST_AUTO_TEST_CASE(type_comment1)
{
  std::string const text{"/* */atomic_int/* */"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_type, value.which());
  BOOST_REQUIRE(boost::get<type>(value) == type_atomic_int);
}
#endif

#if DO_TEST(type_comment2)
BOOST_AUTO_TEST_CASE(type_comment2)
{
  std::string const text{"atomic_/**/int"};

  ast::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
}
#endif

#if DO_TEST(memory_location_internal)
BOOST_AUTO_TEST_CASE(memory_location_internal)
{
  std::string const text{"internal"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_memory_location, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == "internal");
}
#endif

#if DO_TEST(register_r42)
BOOST_AUTO_TEST_CASE(register_r42)
{
  std::string const text{"r42"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_register_location, value.which());
  BOOST_REQUIRE(boost::get<register_location>(value) == 42U);
}
#endif

#if DO_TEST(memory_location_r2d2)
BOOST_AUTO_TEST_CASE(memory_location_r2d2)
{
  std::string const text{"r2d2"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_memory_location, value.which());
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

  ast::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
  BOOST_CHECK_THROW(cppmem::parse(right_text, value), std::domain_error);

  // The behavior when text would be interpreted as wrong_text is different:
  BOOST_CHECK_NO_THROW(cppmem::parse(wrong_text, value));
  BOOST_REQUIRE_EQUAL(NT_register_location, value.which());
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

  ast::nonterminal value;
  BOOST_CHECK_THROW(cppmem::parse(text, value), std::domain_error);
  BOOST_CHECK_THROW(cppmem::parse(right_text, value), std::domain_error);

  // The behavior when text would be interpreted as wrong_text is different:
  BOOST_CHECK_NO_THROW(cppmem::parse(wrong_text, value));
  BOOST_REQUIRE_EQUAL(NT_memory_location, value.which());
  BOOST_REQUIRE(boost::get<memory_location>(value) == wrong_text);
}
#endif

#if DO_TEST(vardecl_simple)
BOOST_AUTO_TEST_CASE(vardecl_simple)
{
  std::string const text{"atomic_int x;"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_vardecl, value.which());
  BOOST_REQUIRE(boost::get<vardecl>(value) == vardecl(type_atomic_int, "x"));
}
#endif

#if DO_TEST(vardecl_init)
BOOST_AUTO_TEST_CASE(vardecl_init)
{
  std::string const text{"int int3 = 123;"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_vardecl, value.which());
  BOOST_REQUIRE(boost::get<vardecl>(value) == vardecl(type_int, "int3", 123));
}
#endif

#if DO_TEST(type_comment3)
BOOST_AUTO_TEST_CASE(type_comment3)
{
  std::string const text{"atomic_int/**/x = 3;"};

  ast::nonterminal value;
  BOOST_CHECK_NO_THROW(cppmem::parse(text, value));

  BOOST_REQUIRE_EQUAL(NT_vardecl, value.which());
  BOOST_REQUIRE(boost::get<vardecl>(value) == vardecl(type_atomic_int, "x", 3));
}
#endif

#if DO_TEST(scope_vardecl)
BOOST_AUTO_TEST_CASE(scope_vardecl)
{
  std::string const text{"{\n int  y = 4;\n}\n"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_scope, value.which());
  ast::scope sc(boost::get<scope>(value));
  //std::cout << "Result: \"" << sc << "\"." << std::endl;
  BOOST_REQUIRE(sc.m_body);
  BOOST_REQUIRE(sc.m_body->m_body_nodes.size() == 1);
  BOOST_REQUIRE(sc.m_body->m_body_nodes.front().which() == BN_vardecl);
  BOOST_REQUIRE(boost::get<vardecl>(sc.m_body->m_body_nodes.front()) == vardecl(type_int, "y", 4U));
}
#endif

#if DO_TEST(scope_assignment)
BOOST_AUTO_TEST_CASE(scope_assignment)
{
  std::string const text{"{ int y=0;   y = 4 ;\n}\n"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_scope, value.which());
  ast::scope sc(boost::get<scope>(value));
  //std::cout << "Result: \"" << sc << "\"." << std::endl;
  BOOST_REQUIRE(sc.m_body);
  BOOST_REQUIRE(sc.m_body->m_body_nodes.size() == 2);
  BOOST_REQUIRE(sc.m_body->m_body_nodes.front().which() == BN_vardecl);
  BOOST_REQUIRE(sc.m_body->m_body_nodes.back().which() == BN_statement);
  ast::statement const& s(boost::get<statement>(sc.m_body->m_body_nodes.back()));
  BOOST_REQUIRE(s.which() == SN_assignment);
  std::stringstream ss;
  ss << s;
  std::string out = ss.str();
  std::cout << "s = \"" << out << "\"." << std::endl;
  BOOST_REQUIRE(out == "y = 4;");
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

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_scope, value.which());
  ast::scope const& s = boost::get<scope>(value);
  std::stringstream ss;
  ss << s;
  std::string out = ss.str();
  //std::cout << "s = \"" << out << "\"." << std::endl;
  find_and_replace(out, "\e[31m", "");
  find_and_replace(out, "\e[0m", "");
  BOOST_REQUIRE(out == "{ int y = 4; atomic_int x; { { x.store(1); }r0=y.load(std::memory_order_relaxed); } }");
}
#endif

#if DO_TEST(function_wrlock)
BOOST_AUTO_TEST_CASE(function_wrlock)
{
  std::string const text{"void wrlock()\n{\n  int y = 4;\n}\n"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_function, value.which());
  ast::function const& f = boost::get<function>(value);
  std::stringstream ss;
  ss << f;
  //std::cout << "f = \"" << ss.str() << "\"." << std::endl;
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

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_threads, value.which());
  ast::threads const& t = boost::get<threads>(value);
  std::stringstream ss;
  ss << t;
  //std::cout << "s = \"" << ss.str() << "\"." << std::endl;
  //BOOST_REQUIRE(ss.str() == "void wrlock() { int y = 4; }");
}
#endif

#if DO_TEST(function_main)
BOOST_AUTO_TEST_CASE(function_main)
{
  std::string const text{"int main()\n{\n  return 0;\n}\n"};

  ast::nonterminal value;
  cppmem::parse(text, value);

  BOOST_REQUIRE_EQUAL(NT_function, value.which());
  ast::function const& f = boost::get<function>(value);
  std::stringstream ss;
  ss << f;
  //std::cout << "s = \"" << ss.str() << "\"." << std::endl;
  BOOST_REQUIRE(ss.str() == "int main() { }");
}
#endif

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
  Debug(NAMESPACE_DEBUG::init());

  return ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}

namespace ast {

bool scope::operator==(std::string const& stmt) const
{
  assert(m_body);
  assert(m_body->m_body_nodes.size() == 1);
  assert(m_body->m_body_nodes.front().which() == BN_statement);
  ast::statement const& s(boost::get<statement>(m_body->m_body_nodes.front()));
  std::stringstream ss;
  ss << s;
  //std::cout << "s = \"" << out << "\"." << std::endl;
  std::string out = ss.str();
  return out == stmt;
}

} // namespace ast
