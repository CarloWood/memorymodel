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
  error_handler<std::string::const_iterator> error_h(start, text.end());
  if (!qi::phrase_parse(start, text.end(), grammar_unittest<std::string::const_iterator>(error_h), skipper, out) || start != text.end())
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
  error_handler<iterator_type> error_h(begin, end);
  bool r = qi::phrase_parse(begin, end, grammar_cppmem<iterator_type>(filename, error_h), skipper, out);
  if (!r)
    return false;
  if (begin != end)
    throw std::domain_error("code after main()");
  return true;
}

} // namespace cppmem
