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
#include <boost/variant/variant.hpp>
#include <boost/optional.hpp>

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

namespace
{

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using skipper = ascii::space_type;

template <typename Iterator>
class cppmem_grammar : public qi::grammar<Iterator, AST::nonterminal(), skipper>
{
  template<typename T> using rule = qi::rule<Iterator, T(), skipper>;

 private:
  rule<char>                    identifier_begin_char;
  rule<char>                    identifier_char;
  rule<std::string>             identifier;
  rule<AST::type>               type;
  rule<AST::register_location>  register_location;
  rule<AST::memory_location>    memory_location;
  rule<AST::global>             global;
  //qi::rule<Iterator, AST::definition_node(), skipper> definition;
  //qi::rule<Iterator, AST::cppmem(), skipper> cppmem_program;
  rule<AST::nonterminal> start;

 public:
  cppmem_grammar() : cppmem_grammar::base_type(start, "cppmem_program")
  {
    identifier_begin_char       = qi::alpha | ascii::char_('_');
    identifier_char             = qi::alnum | ascii::char_('_');

    identifier                  = identifier_begin_char >> *identifier_char;

    type                        = ( qi::lit("int") >> qi::attr(AST::type_int)
                                  | qi::lit("atomic_int") >> qi::attr(AST::type_atomic_int)
                                  ) >> !identifier_char;

    register_location = ('r' > qi::uint_) >> !identifier_char;

    memory_location = identifier - register_location;

    global = type >> memory_location >> -("=" >  qi::int_) > ";";

    //symbol %= register_location | memory_location;

    //definition = type | register_location;
    //cppmem_program = *definition;

    start = global | type | memory_location | register_location;

    identifier_begin_char.name("identifier_begin_char");
    identifier_char.name("identifier_char");
    identifier.name("identifier");
    type.name("type");
    register_location.name("register_location");
    memory_location.name("memory_location");
    global.name("global");

    qi::debug(global);
    qi::debug(type);
    qi::debug(memory_location);
  }
};

} // namespace

namespace cppmem
{

AST::nonterminal parse(std::string const& text)
{
  std::string::const_iterator start{text.begin()};
  AST::nonterminal result;
  if (qi::phrase_parse(start, text.end(), cppmem_grammar<std::string::const_iterator>(), ascii::space, result) &&
      start == text.end())
  {
    return result;
  }

  throw std::domain_error("invalid cppmem input");
}

} // namespace cppmem
