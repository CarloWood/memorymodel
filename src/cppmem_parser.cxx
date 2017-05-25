#include "sys.h"
#include "cppmem_parser.h"
#include "grammar_unittest.h"
#include "grammar_cppmem.h"
#include <boost/spirit/include/support_line_pos_iterator.hpp>

namespace cppmem {

void parse(std::string const& text, ast::nonterminal& out)
{
  using namespace parser;

  std::string::const_iterator start{text.begin()};
  skipper<std::string::const_iterator> skipper;
  if (!qi::phrase_parse(start, text.end(), grammar_unittest<std::string::const_iterator>(), skipper, out) || start != text.end())
  {
    throw std::domain_error("invalid cppmem input");
  }
}

bool parse(char const* filename, std::string const& text, ast::cppmem& out)
{
  using namespace parser;

  std::string::const_iterator text_begin{text.begin()};
  std::string::const_iterator const text_end{text.end()};
  using iterator_type = boost::spirit::line_pos_iterator<std::string::const_iterator>;
  iterator_type begin(text_begin);
  iterator_type const end(text_end);
  skipper<iterator_type> skipper;
  bool r = qi::phrase_parse(begin, end, grammar_cppmem<iterator_type>(filename), skipper, out);
  if (!r)
    return false;
  if (begin != end)
    throw std::domain_error("code after main()");
  return true;
}

} // namespace cppmem
