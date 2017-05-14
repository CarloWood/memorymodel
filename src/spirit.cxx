#define BOOST_SPIRIT_DEBUG
#include <boost/config/warning_disable.hpp>
//#include <boost/spirit/include/qi_grammar.hpp>
//#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <fstream>

namespace client
{

namespace qi = boost::spirit::qi;

struct cppmem
{
  int m_i;
  friend std::ostream& operator<<(std::ostream& os, cppmem const& cppmem)
  {
    os << cppmem.m_i;
    return os;
  }
};

} // namespace client

BOOST_FUSION_ADAPT_STRUCT(
  client::cppmem,
  (int, m_i)
)

namespace client
{

template <typename Iterator>
class cppmem_grammar : public qi::grammar<Iterator, cppmem(), qi::space_type>
{
 private:
  qi::rule<Iterator, cppmem(), qi::space_type> cppmem_program;

 public:
  cppmem_grammar() : cppmem_grammar::base_type(cppmem_program, "cppmem_program")
  {
    cppmem_program = qi::int_;
  }
};

} // namespace client

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <test file>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios_base::binary | std::ios_base::ate);      // Open file with file position at the end.

  if (!file.is_open())
  {
    std::cerr << "Failed to open file \"" << argv[1] << "\".\n";
    return 1;
  }

  // Allocate space and read contents of file into memory.
  std::streampos size = file.tellg();
  std::string input(size, '\0');
  file.seekg(0, std::ios::beg);
  file.read(&input.at(0), size);
  file.close();

  using iterator_type = std::string::iterator;
  using cppmem_grammar = client::cppmem_grammar<iterator_type>;
  namespace qi = boost::spirit::qi;

  cppmem_grammar program;
  client::cppmem result;
  iterator_type iter{input.begin()};
  iterator_type const end{input.end()};
  bool r = qi::phrase_parse(iter, end, program, qi::space, result);

  if (!r || iter != end)
  {
    std::cerr << "Parsing failed." << std::endl;
    return 1;
  }

  std::cout << "Result: " << result << std::endl;
}
