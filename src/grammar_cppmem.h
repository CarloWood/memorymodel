#pragma once

#include "position_handler.h"
#include "grammar_general.h"
#include "grammar_whitespace.h"

namespace parser {

template <typename Iterator>
class grammar_unittest;

template <typename Iterator>
class grammar_cppmem : public qi::grammar<Iterator, ast::cppmem(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;
  using rule_noskip = qi::rule<Iterator, qi::unused_type()>;     // Rules appearing inside lexeme or no_skip etc, must not have a skipper.
  using rule_identifier_noskip = qi::rule<Iterator, char()>;

  // Needs access to the rules.
  friend class grammar_unittest<Iterator>;

  grammar_whitespace<Iterator>                     whitespace;
  rule_noskip                                      scope_begin;
  rule_noskip                                      scope_end;
  rule_noskip                                      threads_begin;
  rule_noskip                                      threads_next;
  rule_noskip                                      threads_end;

  rule_identifier_noskip                           identifier_begin_char;
  rule_identifier_noskip                           identifier_char;
  rule<std::string>                                identifier;
  rule<ast::type>                                  type;
  rule<ast::register_location>                     register_location;
  rule<ast::memory_location>                       memory_location;
  rule<ast::declaration_statement>                 declaration_statement;
  rule<ast::vardecl>                               vardecl;
  rule<ast::mutex_decl>                            mutex_decl;
  rule<ast::condition_variable_decl>               condition_variable_decl;
  rule<ast::unique_lock_decl>                      unique_lock_decl;

  qi::symbols<char, ast::operators>                operators;
  rule<ast::simple_expression>                     simple_expression;
  rule<ast::unary_expression>                      unary_expression;
  rule<ast::expression>                            expression;
  rule<ast::statement>                             statement;
  rule<ast::register_assignment>                   register_assignment;
  rule<ast::assignment>                            assignment;
  qi::symbols<char, std::memory_order>             memory_order;
  rule<ast::function_call>                         function_call;
  rule<ast::atomic_fetch_add_explicit>             atomic_fetch_add_explicit;
  rule<ast::atomic_fetch_sub_explicit>             atomic_fetch_sub_explicit;
  rule<ast::atomic_compare_exchange_weak_explicit> atomic_compare_exchange_weak_explicit;
  rule<ast::load_call>                        load_call;
  rule<ast::store_call>                            store_call;
  rule<ast::jump_statement>                        jump_statement;
  rule<ast::break_statement>                       break_statement;
  rule<ast::return_statement>                      return_statement;
  rule<ast::wait_call>                             wait_call;
  rule<ast::notify_all_call>                       notify_all_call;
  rule<ast::mutex_lock_call>                       mutex_lock_call;
  rule<ast::mutex_unlock_call>                     mutex_unlock_call;
  rule<ast::expression_statement>                  expression_statement;

  rule<ast::selection_statement>                   selection_statement;
  rule<ast::statement>                             else_statement;
  rule<ast::if_statement>                          if_statement;
  rule<ast::iteration_statement>                   iteration_statement;
  rule<ast::while_statement>                       while_statement;
  rule<ast::statement_seq>                         statement_seq;
  rule<ast::compound_statement>                    compound_statement;
  rule<ast::function_name>                         function_name;
  rule<ast::function>                              function;

  rule<ast::function>                              main;
  rule<qi::unused_type>                            return_statement_main;

  rule<ast::threads>                               threads;
  rule<ast::cppmem>                                cppmem;

 public:
  grammar_cppmem(position_handler<Iterator>& position_handler);
};

} // namespace parser
