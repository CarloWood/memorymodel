#include "sys.h"
#include "grammar_vardecl.h"
#include "position_handler.h"

BOOST_FUSION_ADAPT_STRUCT(ast::memory_location, m_name)
BOOST_FUSION_ADAPT_STRUCT(ast::vardecl, m_type, m_memory_location, m_initial_value)

namespace parser {

//=====================================
// The vardecl grammar
//=====================================

template<typename Iterator>
grammar_vardecl<Iterator>::grammar_vardecl(position_handler<Iterator>& handler) :
    grammar_vardecl::base_type(vardecl, "grammar_vardecl")
{
  ascii::alpha_type alpha;
  ascii::alnum_type alnum;
  ascii::char_type char_;
  qi::uint_type uint_;
  qi::int_type int_;
  qi::no_skip_type no_skip;
  qi::lexeme_type lexeme;

  // No-skipper rules.
  identifier_begin_char = alpha | char_('_');
  identifier_char       = alnum | char_('_');

  // A type.
  type =
      ( "int"        >> qi::attr(ast::type_int)
      | "atomic_int" >> qi::attr(ast::type_atomic_int)
      ) >> !no_skip[ identifier_char ];

  // The name of a variable in memory, atomic or non-atomic.
  memory_location =
      identifier - register_location;

  // The name of a register variable name (not in memory).
  register_location =
      lexeme[ 'r' >> uint_ >> !identifier_char ];

  // Allowable variable and function names.
  identifier =
      lexeme[ identifier_begin_char >> *identifier_char ];

  // Variable declaration (global or inside a scope).
  vardecl =
      type >> no_skip[whitespace] >> memory_location >> -("=" > int_) >> ";";

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
      (identifier_begin_char)
      (identifier_char)
      (type)
      (memory_location)
      (register_location)
      (identifier)
      (vardecl)
  );

  using handler_function = boost::phoenix::function<position_handler<Iterator>>;

  qi::_1_type _1;
  qi::_val_type _val;

  // Annotation: on success in vardecl, call position_handler.
  on_success(
      vardecl
    , handler_function(handler)(_val, _1)
  );
}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_vardecl<std::string::const_iterator>;
