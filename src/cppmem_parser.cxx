#include "sys.h"
#include "cppmem_parser.h"
#include "grammar_unittest.h"
#include "grammar_cppmem.h"

namespace cppmem {

using iterator_type = std::string::const_iterator;

void parse(std::string const& text, ast::nonterminal& out)
{
  using namespace parser;

  iterator_type begin(text.begin());
  iterator_type const end(text.end());
  skipper<iterator_type> skipper;
  position_handler<iterator_type> handler("<unit_test>", begin, end);
  bool r = qi::phrase_parse(begin, end, grammar_unittest<iterator_type>(handler), skipper, out);
  if (!r || begin != end)
    throw std::domain_error("invalid cppmem input");
}

bool parse(char const* filename, std::string const& text, ast::cppmem& out)
{
  using namespace parser;

  iterator_type begin(text.begin());
  iterator_type const end(text.end());
  skipper<iterator_type> skipper;
  position_handler<iterator_type> handler(filename, begin, end);
  bool r = qi::phrase_parse(begin, end, grammar_cppmem<iterator_type>(handler), skipper, out);
  if (!r)
    return false;
  if (begin != end)
    throw std::domain_error("code after main()");
  return true;
}

} // namespace cppmem
