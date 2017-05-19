#include "sys.h"
#include "debug.h"
#include "cppmem.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/get.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

BOOST_FUSION_ADAPT_STRUCT(
    AST::memory_location,
    (std::string, m_name)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::global,
    (AST::type, m_type),
    (AST::memory_location, m_memory_location),
    (boost::optional<int>, m_initial_value)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::function,
    (AST::function_name, m_function_name),
    (AST::scope, m_scope)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::body,
    (AST::body::container_type, m_body_nodes),
    (bool, m_dummy)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::scope,
    (boost::optional<AST::body>, m_body)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::threads,
    (AST::threads::container_type, m_threads),
    (bool, m_dummy)
)

namespace AST
{

bool scope::operator==(std::string const& stmt) const
{
  assert(m_body);
  assert(m_body->m_body_nodes.size() == 1);
  assert(m_body->m_body_nodes.front().which() == 0);  // statement
  return boost::get<statement>(m_body->m_body_nodes.front()) == stmt;
}

std::ostream& operator<<(std::ostream& os, scope const& scope)
{
  os << "{ ";
  if (scope.m_body)
    os << scope.m_body.get() << ' ';
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, body const& body)
{
  for (auto&& node : body.m_body_nodes)
    os << node;
  return os;
}

std::ostream& operator<<(std::ostream& os, threads const& threads)
{
  os << "{{{ ";
  bool first = true;
  for (auto&& thread : threads.m_threads)
  {
    if (!first)
      os << " ||| ";
    first = false;
    os << thread;
  }
  os << " }}}";
  return os;
}

} // namespace AST

namespace
{

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace repository = boost::spirit::repository;
namespace phoenix = boost::phoenix;

template<typename Iterator>
struct cpp_comment_skipper : public qi::grammar<Iterator> {

    cpp_comment_skipper() : cpp_comment_skipper::base_type(skip, "C++ comment") {
        skip = repository::confix("//", qi::eol)[*(ascii::char_ - qi::eol)];
    }
    qi::rule<Iterator> skip;
};

template <typename Iterator, typename StartRule, typename Skipper = cpp_comment_skipper<Iterator>>
class grammar_base : public qi::grammar<Iterator, StartRule(), Skipper>
{
 protected:
  template<typename T> using rule = qi::rule<Iterator, T(), Skipper>;

  rule<qi::unused_type>         whitespace;
  rule<char>                    identifier_begin_char;
  rule<char>                    identifier_char;
  rule<std::string>             identifier;
  rule<AST::type>               type;
  rule<AST::register_location>  register_location;
  rule<AST::memory_location>    memory_location;
  rule<AST::global>             global;
  rule<AST::statement>          statement;
  rule<AST::body>               body;
  rule<AST::scope>              scope;
  rule<AST::function_name>      function_name;
  rule<AST::function>           function;
  //rule<AST::threads>            threads;
  rule<AST::cppmem>             cppmem_program;

 protected:
  grammar_base(rule<StartRule> const& start, char const* name) : qi::grammar<Iterator, StartRule(), Skipper>(start, name)
  {
    whitespace                  = +ascii::space;
    identifier_begin_char       = ascii::alpha | ascii::char_('_');
    identifier_char             = ascii::alnum | ascii::char_('_');

    identifier                  = identifier_begin_char >> *identifier_char;

    type                        = ( qi::lit("int")        >> qi::attr(AST::type_int)
                                  | qi::lit("atomic_int") >> qi::attr(AST::type_atomic_int)
                                  ) >> !identifier_char;

    register_location           = 'r' >> qi::uint_ >> !identifier_char;

    memory_location             = identifier - register_location;

    global                      = type >> whitespace >> memory_location >>
                                         -whitespace >> -("=" >> -whitespace > qi::int_) >>
                                         -whitespace > ";" >>
                                         -whitespace;

    //symbol = register_location | memory_location;

    statement                   = -whitespace >> !(ascii::char_('{') | ascii::char_('|')) >> +(ascii::char_ - (ascii::char_('}') | ';')) >> ';' >> -whitespace;

    body                        = +(statement | scope /*| threads*/)            /* m_dummy workaround: */ >> qi::attr(false);

    scope                       = "{" >> -whitespace >> -body >>
                                         -whitespace > "}" >>
                                         -whitespace;

    function_name               = identifier;

    function                    = "void" >> whitespace >> function_name >>
                                           -whitespace >> "()" >>
                                           -whitespace >> scope;

#if 0
    threads                     =   "{{{" >> -whitespace > body >>
                                  +("|||" >> -whitespace > body) >
                                    "}}}" >> -whitespace;
#endif

    identifier_begin_char.name("identifier_begin_char");
    identifier_char.name("identifier_char");
    identifier.name("identifier");
    type.name("type");
    register_location.name("register_location");
    memory_location.name("memory_location");
    global.name("global");
    statement.name("statement");
    body.name("body");
    scope.name("scope");
    function_name.name("function_name");
    function.name("function");
    //threads.name("threads");

    // Uncomment this to turn on debugging.
    //qi::debug(global);
    //qi::debug(type);
    //qi::debug(register_location);
    //qi::debug(memory_location);
    //qi::debug(function_name);
    //qi::debug(function);
    //qi::debug(scope);
    //qi::debug(statement);
    //qi::debug(body);
    //qi::debug(threads);
  }
};

template <typename Iterator>
class unit_test_grammar : public grammar_base<Iterator, AST::nonterminal>
{
 private:
  qi::rule<Iterator, AST::nonterminal(), cpp_comment_skipper<Iterator>> unit_test;

 public:
  unit_test_grammar() : grammar_base<Iterator, AST::nonterminal>(unit_test, "unit_test_grammar")
  {
    unit_test = this->global | this->function | this->scope | this->statement | this->type | this->memory_location | this->register_location;
    unit_test.name("unit_test");
    //qi::debug(unit_test);
  }
};

template <typename Iterator>
class cppmem_grammar : public grammar_base<Iterator, AST::cppmem>
{
 private:
  qi::rule<Iterator, AST::cppmem(), cpp_comment_skipper<Iterator>> cppmem_program;

 public:
  cppmem_grammar() : grammar_base<Iterator, AST::cppmem>(cppmem_program, "cppmem_grammar")
  {
    cppmem_program = -(this->whitespace) >> *(this->global | this->function);
    cppmem_program.name("cppmem_program");
    qi::debug(cppmem_program);
  }
};

} // namespace

namespace cppmem
{

void parse(std::string const& text, AST::nonterminal& out)
{
  std::string::const_iterator start{text.begin()};
  cpp_comment_skipper<std::string::const_iterator> skipper;
  if (!qi::phrase_parse(start, text.end(), unit_test_grammar<std::string::const_iterator>(), skipper, out) || start != text.end())
  {
    throw std::domain_error("invalid cppmem input");
  }
}

bool parse(std::string const& text, AST::cppmem& out)
{
  std::string::const_iterator start{text.begin()};
  cpp_comment_skipper<std::string::const_iterator> skipper;
  return qi::phrase_parse(start, text.end(), cppmem_grammar<std::string::const_iterator>(), skipper, out) && start == text.end();
}

} // namespace cppmem
