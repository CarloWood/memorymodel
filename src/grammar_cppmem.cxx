#include "sys.h"
#include "grammar_cppmem.h"
#include "position_handler.h"
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

BOOST_FUSION_ADAPT_STRUCT(ast::function, m_function_name, m_scope)
BOOST_FUSION_ADAPT_STRUCT(ast::body, m_body_nodes, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::scope, m_body)
BOOST_FUSION_ADAPT_STRUCT(ast::threads, m_threads, m_dummy)

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// Cppmem program grammar
//=====================================

template<typename Iterator>
grammar_cppmem<Iterator>::grammar_cppmem(position_handler<Iterator>& handler) :
    grammar_cppmem::base_type(cppmem, "grammar_cppmem"),
    vardecl(handler), statement_g(handler)
{
  using namespace qi;
  attr_type dummy;

  // Unused_type rules with on_success actions.
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
  body                        = +(vardecl | statement_g | scope | threads) >> dummy(false);
  threads                     = threads_begin > body >> +(threads_next > body) > threads_end >> dummy(false);

  cppmem                      = *(vardecl | function) > main;

  // Debugging and error handling and reporting support.
  using qi::debug;
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
      (cppmem)
  );

  using handler_function = phoenix::function<position_handler<Iterator>>;

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      cppmem
    , handler_function(handler)("Error! Expecting ", _4, _3)
  );

  // Annotation: on success in function, call position_handler.
  on_success(
      function
    , handler_function(handler)(_val, _1)
  );

  on_success(
      scope_begin
    , handler_function(handler)(1, _1)
  );

  on_success(
      scope_end
    , handler_function(handler)(0, _1)
  );

  on_success(
      threads_begin
    , handler_function(handler)(1, _1)
  );

  on_success(
      threads_next
    , handler_function(handler)(2, _1)
  );

  on_success(
      threads_end
    , handler_function(handler)(0, _1)
  );
}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_cppmem<std::string::const_iterator>;
