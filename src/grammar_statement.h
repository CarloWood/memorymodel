#pragma once

#include "grammar_whitespace.h"
#include "ast.h"

template<typename Iterator>
struct position_handler;

namespace parser {

template<typename Iterator>
class grammar_statement : public qi::grammar<Iterator, ast::statement(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;

  qi::symbols<char, ast::function_name> function_names;
  qi::symbols<char, ast::memory_location> memory_locations;
  qi::symbols<char, ast::register_location> register_locations;

  grammar_whitespace<Iterator> whitespace;
  rule<ast::statement>          catchall;
  rule<ast::statement>         statement;

 public:
  grammar_statement(position_handler<Iterator>& handler);

  void function(std::string const& name);
  void vardecl(std::string const& name);
  void scope(bool begin);
};

template<typename Iterator>
void grammar_statement<Iterator>::function(std::string const& name)
{
  function_names.add(name);
}

template<typename Iterator>
void grammar_statement<Iterator>::vardecl(std::string const& name)
{
  memory_locations.add(name);
}

template<typename Iterator>
void grammar_statement<Iterator>::scope(bool /*begin*/)
{
}

} // namespace parser
