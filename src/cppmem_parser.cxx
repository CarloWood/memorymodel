#include "sys.h"
#include "debug.h"
#include "cppmem_parser.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/get.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

BOOST_FUSION_ADAPT_STRUCT(
    AST::memory_location,
    (std::string, m_name)
)

BOOST_FUSION_ADAPT_STRUCT(
    AST::vardecl,
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
  assert(m_body->m_body_nodes.front().which() == BN_statement);
  return boost::get<statement>(m_body->m_body_nodes.front()) == stmt;
}

} // namespace AST

namespace
{

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace repository = boost::spirit::repository;
namespace phoenix = boost::phoenix;

template<typename Iterator>
struct cpp_comment_skipper : public qi::grammar<Iterator>
{
  qi::rule<Iterator> cpp_comment;
  qi::rule<Iterator> c_comment;
  qi::rule<Iterator> skip;

  cpp_comment_skipper() : cpp_comment_skipper::base_type(skip, "C++ comment")
  {
    cpp_comment = repository::confix("//", qi::eol)[*(ascii::char_ - qi::eol)];
    c_comment = repository::confix("/*", "*/")[*(ascii::char_ - "*/")];
    skip = +ascii::space | c_comment | cpp_comment;
  }
};

template <typename Iterator, typename StartRule, typename Skipper = cpp_comment_skipper<Iterator>>
class grammar_base : public qi::grammar<Iterator, StartRule(), Skipper>
{
 protected:
  template<typename T> using rule = qi::rule<Iterator, T(), Skipper>;
  template<typename T> using rule_noskip = qi::rule<Iterator, T()>;     // Rules appearing inside lexeme or no_skip etc, must not have a Skipper.

  rule_noskip<qi::unused_type>  cpp_comment;
  rule_noskip<qi::unused_type>  c_comment;
  rule_noskip<qi::unused_type>  whitespace;
  rule_noskip<char>             identifier_begin_char;
  rule_noskip<char>             identifier_char;

  rule<std::string>             identifier;

  rule<AST::type>               type;
  rule<AST::register_location>  register_location;
  rule<AST::memory_location>    memory_location;
  rule<AST::vardecl>            vardecl;
  rule<AST::statement>          statement;
  rule<AST::body>               body;
  rule<AST::scope>              scope;
  rule<AST::function_name>      function_name;
  rule<AST::function>           function;

  rule<AST::function>           main;
  rule<AST::scope>              main_scope;
  rule<qi::unused_type>         return_statement;

  rule<AST::threads>            threads;
  rule<AST::cppmem>             cppmem_program;

 protected:
  grammar_base(rule<StartRule> const& start, char const* name) : qi::grammar<Iterator, StartRule(), Skipper>(start, name)
  {
    using ascii::char_;

    // No Skipper rules.
    cpp_comment                 = repository::confix("//", qi::eol)[*(ascii::char_ - qi::eol)];
    c_comment                   = repository::confix("/*", "*/")[*(ascii::char_ - "*/")];
    whitespace                  = +(+ascii::space | c_comment | cpp_comment);
    identifier_begin_char       = ascii::alpha | char_('_');
    identifier_char             = ascii::alnum | char_('_');

    identifier                  = qi::lexeme[ identifier_begin_char >> *identifier_char ];

    type                        = ( "int"        >> qi::attr(AST::type_int)
                                  | "atomic_int" >> qi::attr(AST::type_atomic_int)
                                  ) >> !qi::no_skip[ identifier_char ];

    register_location           = qi::lexeme[ 'r' >> qi::uint_ >> !identifier_char ];

    memory_location             = identifier - register_location;

    vardecl                     = type >> qi::no_skip[whitespace] >> memory_location >> -("=" > qi::int_) >> ";";

    //symbol = register_location | memory_location;

    statement                   = !(char_('{') | char_('|') | "return") >> +(char_ - (char_('}') | ';')) >> ';';

    body                        = +(vardecl | statement | scope | threads) >> qi::attr(false);

    scope                       = "{" >> -body >> "}";

    function_name               = identifier;

    function                    = "void" >> qi::no_skip[whitespace] >> function_name >> "()" >> scope;

    return_statement            = "return" >> qi::no_skip[whitespace] >> qi::int_ >> ';';

    main_scope                  = "{" >> -body >> -return_statement >> "}";

    main                        = "int" >> qi::no_skip[whitespace] >> qi::string("main") >> "()" >> main_scope;

    threads                     = "{{{" > body >> +("|||" > body) > "}}}" >> qi::attr(false);

    identifier_begin_char.name("identifier_begin_char");
    identifier_char.name("identifier_char");
    identifier.name("identifier");
    type.name("type");
    register_location.name("register_location");
    memory_location.name("memory_location");
    vardecl.name("vardecl");
    statement.name("statement");
    body.name("body");
    scope.name("scope");
    function_name.name("function_name");
    function.name("function");
    return_statement.name("return_statement");
    main_scope.name("main_scope");
    main.name("main");
    threads.name("threads");

    // Uncomment this to turn on debugging.
#if 1
    qi::debug(vardecl);
    qi::debug(type);
    qi::debug(register_location);
    qi::debug(memory_location);
    qi::debug(statement);
    qi::debug(body);
    qi::debug(scope);
    qi::debug(function_name);
    qi::debug(function);
    qi::debug(return_statement);
    qi::debug(main);
    qi::debug(main_scope);
    qi::debug(threads);
#endif
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
    unit_test = this->main | this->vardecl | this->function | this->scope | this->threads | this->statement | this->type | this->memory_location | this->register_location;
    unit_test.name("unit_test");
    //qi::debug(unit_test);
  }
};

template <typename Iterator>
class cppmem_grammar : public grammar_base<Iterator, AST::cppmem>
{
 private:
  qi::rule<Iterator, AST::cppmem(), cpp_comment_skipper<Iterator>> cppmem_program;
  char const* const m_filename;

 public:
  cppmem_grammar(char const* filename) : grammar_base<Iterator, AST::cppmem>(cppmem_program, "cppmem_grammar"), m_filename(filename)
  {
    cppmem_program = *(this->vardecl | this->function) > this->main;
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

bool parse(char const* filename, std::string const& text, AST::cppmem& out)
{
  std::string::const_iterator text_begin{text.begin()};
  std::string::const_iterator const text_end{text.end()};
  using iterator_type = boost::spirit::line_pos_iterator<std::string::const_iterator>;
  iterator_type begin(text_begin);
  iterator_type const end(text_end);
  cpp_comment_skipper<iterator_type> skipper;
  return qi::phrase_parse(begin, end, cppmem_grammar<iterator_type>(filename), skipper, out) && begin == end;
}

} // namespace cppmem
