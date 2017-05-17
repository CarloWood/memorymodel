#include "sys.h"
#include "debug.h"
#include "cppmem.h"
#include <boost/config/warning_disable.hpp>
//#include <boost/spirit/include/qi_grammar.hpp>
//#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi.hpp>
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

namespace AST
{

bool scope::operator==(std::string const& stmt) const
{
  assert(m_list.size() == 1);
  assert(m_list.front().which() == 0);  // statement
  return boost::get<statement>(m_list.front()) == stmt;
}

} // namespace AST

namespace
{

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

template <typename Iterator, typename StartRule>
class grammar_base : public qi::grammar<Iterator, StartRule()>
{
 protected:
  template<typename T> using rule = qi::rule<Iterator, T()>;

  rule<qi::unused_type>         whitespace;
  rule<char>                    identifier_begin_char;
  rule<char>                    identifier_char;
  rule<std::string>             identifier;
  rule<AST::type>               type;
  rule<AST::register_location>  register_location;
  rule<AST::memory_location>    memory_location;
  rule<AST::global>             global;
  rule<AST::statement>          statement;
  rule<AST::scope>              scope;
  rule<AST::function_name>      function_name;
  rule<AST::function>           function;
  rule<AST::cppmem>             cppmem_program;

 protected:
  grammar_base(rule<StartRule> const& start, char const* name) : qi::grammar<Iterator, StartRule()>(start, name)
  {
    whitespace                  = +qi::space;
    identifier_begin_char       = qi::alpha | ascii::char_('_');
    identifier_char             = qi::alnum | ascii::char_('_');

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

    statement                   = -whitespace >> !ascii::char_('{') >> +(ascii::char_ - (ascii::char_('}') | ';')) >> ';' >> -whitespace;

    scope                       = "{" >> -whitespace >> *(statement | scope) >>
                                         -whitespace > "}" >>
                                         -whitespace;

    function_name               = identifier;

    function                    = "void" >> whitespace >> function_name >>
                                           -whitespace >> "()" >>
                                           -whitespace >> scope;

    //cppmem_program = *definition;

    identifier_begin_char.name("identifier_begin_char");
    identifier_char.name("identifier_char");
    identifier.name("identifier");
    type.name("type");
    register_location.name("register_location");
    memory_location.name("memory_location");
    global.name("global");
    statement.name("statement");
    scope.name("scope");
    function_name.name("function_name");
    function.name("function");

    // Uncomment this to turn on debugging.
    //qi::debug(global);
    //qi::debug(type);
    //qi::debug(register_location);
    //qi::debug(memory_location);
    //qi::debug(function_name);
    //qi::debug(function);
    //qi::debug(scope);
    //qi::debug(statement);
  }
};

template <typename Iterator>
class unit_test_grammar : public grammar_base<Iterator, AST::nonterminal>
{
 private:
  qi::rule<Iterator, AST::nonterminal()> unit_test;

 public:
  unit_test_grammar() : grammar_base<Iterator, AST::nonterminal>(unit_test, "unit_test")
  {
    unit_test = this->global | this->function | this->scope | this->statement | this->type | this->memory_location | this->register_location;
    //qi::debug(unit_test);
  }
};

template <typename Iterator>
class cppmem_grammar : public grammar_base<Iterator, AST::cppmem>
{
 private:
  qi::rule<Iterator, AST::cppmem> cppmem_program;

 public:
  cppmem_grammar() : grammar_base<Iterator, AST::cppmem>(cppmem_program, "cppmem_program")
  {
    cppmem_program = -(this->whitespace) >> *(this->global | this->function);
    //qi::debug(cppmem_program);
  }
};

} // namespace

namespace cppmem
{

void parse(std::string const& text, AST::nonterminal& out)
{
  std::string::const_iterator start{text.begin()};
  if (!qi::parse(start, text.end(), unit_test_grammar<std::string::const_iterator>(), out) || start != text.end())
  {
    throw std::domain_error("invalid cppmem input");
  }
}

bool parse(std::string const& text, AST::cppmem& out)
{
  std::string::const_iterator start{text.begin()};
  return qi::parse(start, text.end(), cppmem_grammar<std::string::const_iterator>(), out) && start == text.end();
}

} // namespace cppmem
