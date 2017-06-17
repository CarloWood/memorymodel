#include "sys.h"
#include "cppmem_parser.h"
#include "grammar_cppmem.h"

namespace cppmem {

bool parse(iterator_type& begin, iterator_type const& end, position_handler<iterator_type>& handler, ast::cppmem& out)
{
  using namespace parser;

  skipper<iterator_type> skipper;
  bool r = qi::phrase_parse(begin, end, grammar_cppmem<iterator_type>(handler), skipper, out);
  if (!r)
  {
    std::cerr << "Parse error." << std::endl;
    return false;
  }
  if (begin != end)
  {
    std::cerr << "Remaining unparsed input: '" << std::string(begin, end) << "'.\n";
    throw std::domain_error("code after main()");
  }
  return true;
}

} // namespace cppmem
