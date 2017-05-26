#include "sys.h"
#include "grammar_cppmem.h"
//#include "annotation.h"
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

BOOST_FUSION_ADAPT_STRUCT(
    ast::function,
    (ast::function_name, m_function_name),
    (ast::scope, m_scope)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::body,
    (ast::body::container_type, m_body_nodes),
    (bool, m_dummy)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::scope,
    (boost::optional<ast::body>, m_body)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::threads,
    (ast::threads::container_type, m_threads),
    (bool, m_dummy)
)

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// Cppmem program grammar
//=====================================

template<typename Iterator>
grammar_cppmem<Iterator>::grammar_cppmem(char const* filename, error_handler<Iterator>& error_h) :
    grammar_cppmem::base_type(cppmem, "grammar_cppmem"),
    m_filename(filename)
{
  ascii::char_type char_;
  qi::lit_type lit;
  qi::int_type int_;
  qi::string_type string;
  qi::no_skip_type no_skip;
  qi::attr_type dummy;

  // Unused_type rules with semantic actions.
  scope_begin                 = (lit('{') - "{{{");
  scope_end                   = lit('}');
  threads_begin               = lit("{{{");
  threads_next                = lit("|||");
  threads_end                 = lit("}}}");

  // int main() { ... [return 0;] }   // the optional return statement is ignored.
  main                        = "int" > no_skip[whitespace] > string("main") > "()" > main_scope;
  main_scope                  = scope_begin > -body > -return_statement > scope_end;
  return_statement            = "return" > no_skip[whitespace] > int_ > ';';

  // void function_name() { ... }
  function                    = "void" > no_skip[whitespace] > function_name > "()" > scope;
  function_name               = vardecl.identifier;
  scope                       = scope_begin > -body > scope_end;

  // The body of a function.
  body                        = +(vardecl | statement | scope | threads) >> dummy(false);
  threads                     = threads_begin > body >> +(threads_next > body) > threads_end >> dummy(false);

  // Statements.
  catchall                    = !(char_('{') | char_('|') | "return") >> +(char_ - (char_('}') | ';')) >> ';';
  statement                   = /*assignment |*/ catchall;
  //assignment                  = m_symbols >> '=' >> int_ >> ';';

  cppmem                      = *(vardecl | function) > main;

  // Names of grammar rules.
  scope_begin.name("scope_begin");
  scope_end.name("scope_end");
  threads_begin.name("threads_begin");
  threads_next.name("threads_next");
  threads_end.name("threads_end");
  main.name("main");
  main_scope.name("main_scope");
  return_statement.name("return_statement");
  function.name("function");
  function_name.name("function_name");
  scope.name("scope");
  body.name("body");
  threads.name("threads");
  statement.name("statement");
  assignment.name("assignment");
  catchall.name("catchall");
  cppmem.name("cppmem");

  // Debugging and error handling and reporting support.
  BOOST_SPIRIT_DEBUG_NODES(
      (scope_begin)
      (scope_end)
      (threads_begin)
      (threads_next)
      (threads_end)
      (main)
      (main_scope)
      (return_statement)
      (function)
      (function_name)
      (scope)
      (body)
      (threads)
      (statement)
      (assignment)
      (catchall)
      (cppmem)
  );

  using qi::on_error;
  using qi::on_success;
  using qi::fail;
  using phoenix::construct;
  using phoenix::val;
  using error_handler_function = phoenix::function<error_handler<Iterator>>;
//  using annotation_function = function<annotation<Iterator>>;

  qi::_1_type _1;
  qi::_2_type _2;
  qi::_3_type _3;
  qi::_4_type _4;
  qi::_val_type _val;

  // Error handling: on error in start, call error_h.
  on_error<fail>
  (
      cppmem
    , error_handler_function(error_h)("Error! Expecting ", _4, _3)
  );

  // Annotation: on success in start, call annotation.
//  on_success(function, annotation_function(error_h.iters)(_val, _1));
}

} // namespace parser

#include <boost/spirit/include/support_line_pos_iterator.hpp>

// Instantiate grammar templates.
template struct parser::grammar_cppmem<boost::spirit::line_pos_iterator<std::string::const_iterator>>;
template struct parser::grammar_cppmem<std::string::const_iterator>;
