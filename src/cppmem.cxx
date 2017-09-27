#include "sys.h"
#include "debug.h"
#include "ast.h"
#include "position_handler.h"
#include "Graph.h"
#include "Context.h"
#include "Evaluation.h"
#include "TagCompare.h"
#include "ScopeDetector.h"
#include "Locks.h"
#include "Loops.h"
#include "Symbols.h"
#include "Location.h"
#include "cppmem_parser.h"
#include "FollowOpsemTails.h"
#include "FollowUniqueOpsemTails.h"
#include "ReadFromLoopsPerLocation.h"
#include "FilterAllActions.h"
#include "boolean-expression/TruthProduct.h"
#include "utils/AIAlert.h"
#include "utils/MultiLoop.h"
#include <cstdlib>      // std::system
#include <boost/variant/get.hpp>
#include <ostream>
#include <fstream>
#include <iomanip>
#ifdef CWDEBUG
#include <libcwd/type_info.h>
#endif

std::map<ast::tag, ast::function, TagCompare> functions;

void execute_body(std::string name, ast::statement_seq const& body);
Evaluation execute_expression(ast::assignment_expression const& expression);
Evaluation execute_expression(ast::expression const& expression);

void execute_declaration(ast::declaration_statement const& declaration_statement)
{
  DoutEntering(dc::execute, "execute_declaration(`" << declaration_statement << "`)");

  // http://eel.is/c++draft/intro.execution#12.5 states
  //
  //   12.5 An expression that is not a subexpression of another expression and
  //   that is not otherwise part of a full-expression. If a language construct
  //   is defined to produce an implicit call of a function, a use of the language
  //   construct is considered to be an expression for the purposes of this definition.
  //   Conversions applied to the result of an expression in order to satisfy the
  //   requirements of the language construct in which the expression appears are
  //   also considered to be part of the full-expression.
  //
  //   For an initializer, performing the initialization of the entity (including
  //   evaluating default member initializers of an aggregate) is also considered
  //   part of the full-expression.
  //
  // Therefore, although a declaration isn't an (assignment-)expression, the side-effect
  // of initialization of the declared variable is to be considered part of the full-
  // expression (see also examples below 12.5).

  Evaluation full_expression;   // Evaluation (value computation and side-effect) of
                                // this full-expression.
  FullExpressionDetector detector(full_expression);

  switch (declaration_statement.m_declaration_statement_node.which())
  {
    case ast::DS_mutex_decl:
    {
      auto const& mutex_decl{boost::get<ast::mutex_decl>(declaration_statement.m_declaration_statement_node)};
      Dout(dc::notice, declaration_statement);
      DebugMarkUp;
      full_expression = Context::instance().lockdecl(mutex_decl);
      break;
    }
    case ast::DS_condition_variable_decl:
    {
      DoutTag(dc::notice, declaration_statement << " [declaration of", declaration_statement.tag());
      // TODO:  full_expression = ?
      break;
    }
    case ast::DS_unique_lock_decl:
    {
      auto const& unique_lock_decl{boost::get<ast::unique_lock_decl>(declaration_statement.m_declaration_statement_node)};
      DoutTag(dc::notice, declaration_statement << " [declaration of", declaration_statement.tag());
      DebugMarkUp;
      full_expression = Context::instance().lock(unique_lock_decl.m_mutex);
      Context::instance().m_locks.add(unique_lock_decl);
      break;
    }
    case ast::DS_vardecl:
    {
      auto const& vardecl{boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node)};
      if (vardecl.m_initial_value)
      {
        // This is a `initializer-clause` - but is this already part of an expression (the expression-statement that is the declaration)?
        full_expression = execute_expression(*vardecl.m_initial_value);
        Dout(dc::notice, declaration_statement);
        DebugMarkUp;
        full_expression.write(declaration_statement.tag());
      }
      else
      {
        Dout(dc::notice, declaration_statement);
        DebugMarkUp;
        full_expression = Context::instance().uninitialized(declaration_statement.tag());
      }
      break;
    }
  }
  Context::instance().m_symbols.add(declaration_statement);
}

Evaluation execute_condition(ast::expression const& condition)
{
  DoutEntering(dc::execute, "execute_condition(`" << condition << "`)");
  Evaluation result = execute_expression(condition);
  Dout(dc::continued, condition);
  return result;
}

Evaluation execute_primary_expression(ast::primary_expression const& primary_expression)
{
  DoutEntering(dc::execute, "execute_primary_expression(`" << primary_expression << "`)");

  Evaluation result;
  auto const& node = primary_expression.m_primary_expression_node;
  switch (node.which())
  {
    case ast::PE_int:
    {
      auto const& literal(boost::get<int>(node));
      result = literal;
      break;
    }
    case ast::PE_bool:
    {
      auto const& literal(boost::get<bool>(node));
      result = literal;
      break;
    }
    case ast::PE_tag:
    {
      auto const& tag(boost::get<ast::tag>(node));                                 // tag that the parser thinks we are using.
      result = tag;
      // We shouldn't really read from registers... but I'll leave this in for now.
      if (parser::Symbols::instance().is_register(tag))                            // Don't add nodes for reading from a register.
        break;
      // Check if that variable wasn't masked by another variable of different type.
      std::string name = parser::Symbols::instance().tag_to_string(tag);           // Actual C++ object of that name in current scope.
      ast::declaration_statement const& declaration_statement{Context::instance().m_symbols.find(name)}; // Declaration of actual object.
      if (declaration_statement.tag() != tag)                                      // Did the parser make a mistake?
      {
        std::string position{Context::instance().get_position_handler().location(declaration_statement.tag())};
        if (declaration_statement.m_declaration_statement_node.which() == ast::DS_vardecl)
        {
          auto const& vardecl(boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node));
          if (vardecl.m_memory_location.m_type == ast::type_atomic_int)
            THROW_ALERT("Please use [MEMORY_LOCATION].load() for atomic_int declared at [POSITION] instead of [MEMORY_LOCATION]",
                        AIArgs("[MEMORY_LOCATION]", vardecl.m_memory_location)("[POSITION]", position));
        }
        THROW_ALERT("[OBJECT] declared at [POSITION] has the wrong type for this operation,",
                    AIArgs("[OBJECT]", declaration_statement)("[POSITION]", position));
      }
      // This must be a non-atomic int or bool, or we wouldn't be reading from it.
      assert(declaration_statement.m_declaration_statement_node.which() == ast::DS_vardecl &&
          !boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node).m_type.is_atomic());
      result.read(tag);
      break;
    }
    case ast::PE_expression:
    {
      ast::expression const& expression{boost::get<ast::expression>(node)};
      result = execute_expression(expression);
      break;
    }
  }
  return result;
}

template<typename T>
binary_operators get_operator(typename T::tail_type const& chain);

// Specializations.
template<> binary_operators get_operator<ast::logical_or_expression>(ast::logical_or_expression::tail_type const&) { return logical_or; }
template<> binary_operators get_operator<ast::logical_and_expression>(ast::logical_and_expression::tail_type const&) { return logical_and; }
template<> binary_operators get_operator<ast::inclusive_or_expression>(ast::inclusive_or_expression::tail_type const&) { return bitwise_inclusive_or; }
template<> binary_operators get_operator<ast::exclusive_or_expression>(ast::exclusive_or_expression::tail_type const&) { return bitwise_exclusive_or; }
template<> binary_operators get_operator<ast::and_expression>(ast::and_expression::tail_type const&) { return bitwise_and; }
template<> binary_operators get_operator<ast::equality_expression>(ast::equality_expression::tail_type const& tail)
    { return static_cast<binary_operators>(equality_eo_eq + boost::fusion::get<0>(tail)); }
template<> binary_operators get_operator<ast::relational_expression>(ast::relational_expression::tail_type const& tail)
    { return static_cast<binary_operators>(relational_ro_lt + boost::fusion::get<0>(tail)); }
template<> binary_operators get_operator<ast::shift_expression>(ast::shift_expression::tail_type const& tail)
    { return static_cast<binary_operators>(shift_so_shl + boost::fusion::get<0>(tail)); }
template<> binary_operators get_operator<ast::additive_expression>(ast::additive_expression::tail_type const& tail)
    { return static_cast<binary_operators>(additive_ado_add + boost::fusion::get<0>(tail)); }
template<> binary_operators get_operator<ast::multiplicative_expression>(ast::multiplicative_expression::tail_type const& tail)
    { return static_cast<binary_operators>(multiplicative_mo_mul + boost::fusion::get<0>(tail)); }

template<typename T>
Evaluation execute_operator_list_expression(T const& expr);

template<typename OPERATORS, typename PREV_PRECEDENCE_TYPE>
Evaluation execute_operator_list_expression(boost::fusion::tuple<OPERATORS, PREV_PRECEDENCE_TYPE> const& tuple);

template<typename T>
Evaluation execute_operator_list_expression(T const& expr)
{
  // Only print Entering.. when there is actually an operator.
  DoutEntering(dc::execute(!expr.m_chained.empty()),
      "execute_operator_list_expression<" << type_info_of<T>().demangled_name() << ">(`" << expr << "`).");

  // T is one of:
  // <logical_or_expression>
  // <logical_and_expression>
  // <inclusive_or_expression>
  // <exclusive_or_expression>
  // <and_expression>
  // <equality_expression>
  // <relational_expression>
  // <shift_expression>
  // <additive_expression>
  // <multiplicative_expression>

  // A chain, ie x + y + z really is (x + y) + z.
  Evaluation result = execute_operator_list_expression(expr.m_other_expression);
  for (auto const& tail : expr.m_chained)
  {
    Evaluation rhs = execute_operator_list_expression(tail);
    if (std::is_same<T, ast::logical_or_expression>::value ||
        std::is_same<T, ast::logical_and_expression>::value ||
        std::is_same<T, ast::shift_expression>::value)                  // Only as of C++17! Rule 19 of http://en.cppreference.com/w/cpp/language/eval_order
    {
      Dout(dc::sb_edge, "Boolean expression (operator || or &&), or shift expression (operator << || >>)");
      DebugMarkUp;
      Context::instance().add_edges(edge_sb, result, rhs);
    }
    result.OP(get_operator<T>(tail), std::move(rhs));
  }
  return result;
}

// Specialization for boost::fusion::tuple<operators, prev_precedence_type>
template<typename OPERATORS, typename PREV_PRECEDENCE_TYPE>
Evaluation execute_operator_list_expression(boost::fusion::tuple<OPERATORS, PREV_PRECEDENCE_TYPE> const& tuple)
{
  // Doesn't really make much sense to print this, as the function being called already prints basically the same info.
  //DoutEntering(dc::execute, "execute_operator_list_expression<" <<
  //    type_info_of<OPERATORS>().demangled_name() << ", " <<
  //    type_info_of<PREV_PRECEDENCE_TYPE>().demangled_name() << ">(" << tuple << ").");

  // Just unpack the second argument.
  return execute_operator_list_expression(boost::fusion::get<1>(tuple));
}

Evaluation execute_postfix_expression(ast::postfix_expression const& expr)
{
  // Only print Entering... when we actually have a pre- increment, decrement or unary operator.
  DoutEntering(dc::execute(!expr.m_postfix_operators.empty()), "execute_postfix_expression(`" << expr << "`)");

  Evaluation result;

  auto const& node = expr.m_postfix_expression_node;
  ASSERT(node.which() == ast::PE_primary_expression);
  auto const& primary_expression{boost::get<ast::primary_expression>(node)};
  result = execute_primary_expression(primary_expression);

  if (!expr.m_postfix_operators.empty())
  {
    auto const& primary_expression_node{primary_expression.m_primary_expression_node};
    if (primary_expression_node.which() != ast::PE_tag)
    {
      THROW_ALERT("Can't use a postfix operator after `[EXPRESSION]`", AIArgs("[EXPRESSION]", primary_expression_node));
    }
    ast::tag const& tag{boost::get<ast::tag>(primary_expression_node)};

    // Postfix operators.
    for (auto const& postfix_operator : expr.m_postfix_operators)
    {
      result.postfix_operator(postfix_operator);
      result.write(tag);
    }
  }

  return result;
}

// Specialization for unary_expression.
template<>
Evaluation execute_operator_list_expression(ast::unary_expression const& expr)
{
  // Only print Entering... when we actually have a pre- increment, decrement or unary operator.
  DoutEntering(dc::execute(!expr.m_unary_operators.empty()),
      "execute_operator_list_expression(`" << expr << "`) [for pre-increment, pre-decrement and unary expressions]");

  Evaluation result;
  auto const& node = expr.m_postfix_expression.m_postfix_expression_node;
  if (node.which() != ast::PE_primary_expression && !expr.m_postfix_expression.m_postfix_operators.empty())
  {
    THROW_ALERT("Can't use a postfix expression after `[EXPRESSION]`", AIArgs("[EXPRESSION]", node));
  }
  switch (node.which())
  {
    case ast::PE_primary_expression:
    {
      auto const& primary_expression{boost::get<ast::primary_expression>(node)};
      // Postfix expressions can only be applied to primary expressions (and PE_tag types to that),
      // but we still have to process the postfix expression ;).
      result = execute_postfix_expression(expr.m_postfix_expression);

      // Prefix and unary operators.
      for (auto const& unary_operator : expr.m_unary_operators)
      {
        // Prefix operator?
        if (unary_operator == ast::uo_inc || unary_operator == ast::uo_dec)
        {
          auto const& primary_expression_node{primary_expression.m_primary_expression_node};
          if (primary_expression_node.which() != ast::PE_tag)
            THROW_ALERT("Can't use a prefix operator before `[EXPRESSION]`", AIArgs("[EXPRESSION]", primary_expression_node));
          ast::tag const& tag{boost::get<ast::tag>(primary_expression_node)};
          result.prefix_operator(unary_operator);
          result.write(tag, true);
        }
        else
          result.unary_operator(unary_operator);
      }
      break;
    }
    case ast::PE_atomic_fetch_add_explicit:
    {
      auto const& atomic_fetch_add_explicit{boost::get<ast::atomic_fetch_add_explicit>(node)};
      result = atomic_fetch_add_explicit.m_memory_location_id;
      result.OP(additive_ado_add, execute_expression(atomic_fetch_add_explicit.m_expression));
      NodePtr rmw_node = result.RMW(atomic_fetch_add_explicit.m_memory_location_id, atomic_fetch_add_explicit.m_memory_order);
      Context::instance().add_edges(edge_sb, *rmw_node.get<RMWNode>()->get_evaluation(), rmw_node);
      break;
    }
    case ast::PE_atomic_fetch_sub_explicit:
    {
      auto const& atomic_fetch_sub_explicit{boost::get<ast::atomic_fetch_sub_explicit>(node)};
      result = atomic_fetch_sub_explicit.m_memory_location_id;
      result.OP(additive_ado_sub, execute_expression(atomic_fetch_sub_explicit.m_expression));
      NodePtr rmw_node = result.RMW(atomic_fetch_sub_explicit.m_memory_location_id, atomic_fetch_sub_explicit.m_memory_order);
      Context::instance().add_edges(edge_sb, *rmw_node.get<RMWNode>()->get_evaluation(), rmw_node);
      break;
    }
    case ast::PE_atomic_compare_exchange_weak_explicit:
    {
      auto const& atomic_compare_exchange_weak_explicit{boost::get<ast::atomic_compare_exchange_weak_explicit>(node)};
      result = atomic_compare_exchange_weak_explicit.m_memory_location_id;
      NodePtr cew_node =
          result.compare_exchange_weak(
              atomic_compare_exchange_weak_explicit.m_memory_location_id,
              atomic_compare_exchange_weak_explicit.m_expected,
              atomic_compare_exchange_weak_explicit.m_desired,
              atomic_compare_exchange_weak_explicit.m_succeed,
              atomic_compare_exchange_weak_explicit.m_fail);
      Context::instance().add_edges(edge_sb, *cew_node.get<CEWNode>()->get_evaluation(), cew_node);
      break;
    }
    case ast::PE_load_call:
    {
      auto const& load_call{boost::get<ast::load_call>(node)};
      result = load_call.m_memory_location_id;
      result.read(load_call.m_memory_location_id, load_call.m_memory_order);
      break;
    }
  }
  return result;
}

Evaluation execute_expression(ast::assignment_expression const& expression)
{
  DoutEntering(dc::execute, "execute_expression(`" << expression << "`).");
  DebugMarkDown;

  Evaluation result;
  // - expression-statement (as part of statement and for-init-statement)
  FullExpressionDetector detector(result);

  auto const& node = expression.m_assignment_expression_node;
  switch (node.which())
  {
    case ast::AE_conditional_expression:
    {
      auto const& conditional_expression{boost::get<ast::conditional_expression>(node)};
      result = execute_operator_list_expression(conditional_expression.m_logical_or_expression);
      if (conditional_expression.m_conditional_expression_tail)
      {
        ast::expression const& expression{boost::fusion::get<0>(conditional_expression.m_conditional_expression_tail.get())};
        ast::assignment_expression const& assignment_expression{boost::fusion::get<1>(conditional_expression.m_conditional_expression_tail.get())};
        Evaluation true_evaluation = execute_expression(expression);
        Evaluation false_evaluation = execute_expression(assignment_expression);
        EvaluationNodePtrs before_node_ptrs = result.get_nodes(NodeRequestedType::heads COMMA_DEBUG_ONLY(edge_sb));
        result.conditional_operator(before_node_ptrs, std::move(true_evaluation), std::move(false_evaluation));
      }
      break;
    }
    case ast::AE_register_assignment:
    {
      auto const& register_assignment{boost::get<ast::register_assignment>(node)};
      result = execute_expression(register_assignment.rhs);
      // Assignment to a register doesn't generate a side-effect (we're not really writing to memory),
      // so just leave the result what it is as it represents the full-expression of this assignment.
      break;
    }
    case ast::AE_assignment:
    {
      auto const& assignment{boost::get<ast::assignment>(node)};
      result = execute_expression(assignment.rhs);
      Dout(dc::valuecomp, "Assignment value computation results in " << result << "; assigned to `" << assignment.lhs << "`.");
      result.write(assignment.lhs, true);
      break;
    }
  }
  return result;
}

Evaluation execute_expression(ast::expression const& expression)
{
  DoutEntering(dc::execute(expression.m_chained.size() > 1), "execute_expression(`" << expression << "`).");
  Evaluation result;
  // - expression (as part of noptr-new-declarator, condition, iteration-statement, jump-statement and decltype-specifier)
  FullExpressionDetector detector(result);
  result = execute_expression(expression.m_assignment_expression);
  for (auto const& assignment_expression : expression.m_chained)
  {
    Evaluation rhs = execute_expression(assignment_expression);
    Context::instance().add_edges(edge_sb, result, rhs);
    result.comma_operator(std::move(rhs));
  }
  return result;
}

void execute_statement(ast::statement const& statement)
{
  auto const& node = statement.m_statement_node;
  switch (node.which())
  {
    case ast::SN_expression_statement:
    {
      auto const& expression_statement{boost::get<ast::expression_statement>(node)};
      if (expression_statement.m_expression)
      {
        execute_expression(expression_statement.m_expression.get());
        Dout(dc::notice, expression_statement);
      }
      break;
    }
    case ast::SN_store_call:
    {
      auto const& store_call{boost::get<ast::store_call>(node)};
      Evaluation value;
      FullExpressionDetector detector(value);          // A call to store like: y.store(expr, mo); is a expression statement.
      value = execute_expression(store_call.m_val);
      Dout(dc::notice, store_call << ";");
      DebugMarkUp;
      NodePtr write_node = value.write(store_call.m_memory_location_id, store_call.m_memory_order);
      // Side-effects and value-computations of function arguments are sequenced before the side-effects and value-computations of the function body.
      // The evaluation of the write_node is the function argument passed here (the previous value of 'value' as returned by execute_expression()).
      Context::instance().add_edges(edge_sb, *write_node.get<WriteNode>()->get_evaluation(), write_node);
      break;
    }
    case ast::SN_function_call:
    {
      auto const& function_call{boost::get<ast::function_call>(node)};
#ifdef CWDEBUG
      Dout(dc::notice, function_call.m_function << "();");
      debug::Mark mark;
#endif
      auto const& function{functions[function_call.m_function]};
      if (function.m_compound_statement.m_statement_seq)
        execute_body(function.m_function_name.name, *function.m_compound_statement.m_statement_seq);
      break;
    }
    case ast::SN_wait_call:
    {
      auto const& wait_call{boost::get<ast::wait_call>(node)};
      Dout(dc::notice, "TODO: " << wait_call);
      //FullExpressionDetector detector(value);
      break;
    }
    case ast::SN_notify_all_call:
    {
      auto const& notify_all_call{boost::get<ast::notify_all_call>(node)};
      Dout(dc::notice, "TODO: " << notify_all_call);
      //FullExpressionDetector detector(value);
      break;
    }
    case ast::SN_mutex_lock_call:
    {
      auto const& mutex_lock_call{boost::get<ast::mutex_lock_call>(node)};
      Evaluation value;
      FullExpressionDetector detector(value);
      Dout(dc::notice, mutex_lock_call);
      DebugMarkUp;
      value = Context::instance().lock(mutex_lock_call.m_mutex);
      break;
    }
    case ast::SN_mutex_unlock_call:
    {
      auto const& mutex_unlock_call{boost::get<ast::mutex_unlock_call>(node)};
      Evaluation value;
      FullExpressionDetector detector(value);
      Dout(dc::notice, mutex_unlock_call);
      DebugMarkUp;
      value = Context::instance().unlock(mutex_unlock_call.m_mutex);
      break;
    }
    case ast::SN_threads:
    {
      auto const& threads{boost::get<ast::threads>(node)};
      // The notation '{{{' begins starting a batch of threads.
      Context::instance().start_threads();
      for (auto& statement_seq : threads.m_threads)
        execute_body("thread", statement_seq);
      // The notation '}}}' implies that all threads are joined.
      Context::instance().join_all_threads();
      break;
    }
    case ast::SN_compound_statement:
    {
      auto const& compound_statement{boost::get<ast::compound_statement>(node)};
      if (compound_statement.m_statement_seq)
        execute_body("compound_statement", *compound_statement.m_statement_seq);
      break;
    }
    case ast::SN_selection_statement:
    {
      auto const& selection_statement{boost::get<ast::selection_statement>(node)};
      Dout(dc::notice|continued_cf, "if (");
      Evaluation condition;
      try { condition = execute_condition(selection_statement.m_if_statement.m_condition); }
      catch (std::exception const&) { Dout(dc::finish, ""); throw; }
      Dout(dc::finish, ")");
      if (condition.is_literal())
      {
        Dout(dc::branch, "Selection statement conditional is a literal! Not doing a branch.");
        if (condition.literal_value())
          execute_statement(selection_statement.m_if_statement.m_then);
        else if (selection_statement.m_if_statement.m_else)
          execute_statement(*selection_statement.m_if_statement.m_else);
      }
      else
      {
        EvaluationNodePtrConditionPairs unconnected_heads;
        Context::instance().current_thread()->begin_branch_true(unconnected_heads, Evaluation::make_unique(std::move(condition)));
        execute_statement(selection_statement.m_if_statement.m_then);
        if (selection_statement.m_if_statement.m_else)
        {
          Context::instance().current_thread()->begin_branch_false(unconnected_heads);
          execute_statement(*selection_statement.m_if_statement.m_else);
        }
        Context::instance().current_thread()->end_branch(unconnected_heads);
      }
      break;
    }
    case ast::SN_iteration_statement:
    {
      auto const& iteration_statement{boost::get<ast::iteration_statement>(node)};
      // We only support while () { } at the moment.
      Dout(dc::notice|continued_cf, "while (");
      try { execute_condition(iteration_statement.m_while_statement.m_condition); }
      catch (std::exception const&) { Dout(dc::finish, ""); throw; }
      Dout(dc::finish, ")");
      Context::instance().m_loops.enter(iteration_statement);
      execute_statement(iteration_statement.m_while_statement.m_statement);
      Context::instance().m_loops.leave(iteration_statement);
      break;
    }
    case ast::SN_jump_statement:
    {
      auto const& jump_statement{boost::get<ast::jump_statement>(node)};
      switch (jump_statement.m_jump_statement_node.which())
      {
        case ast::JS_break_statement:
        {
          auto const& break_statement{boost::get<ast::break_statement>(jump_statement.m_jump_statement_node)};
          Context::instance().m_loops.add_break(break_statement);
          break;
        }
        case ast::JS_return_statement:
        {
          auto const& return_statement{boost::get<ast::return_statement>(jump_statement.m_jump_statement_node)};
          Dout(dc::notice, return_statement);
          DebugMarkUp;
          execute_expression(return_statement.m_expression);
          break;
        }
      }
      break;
    }
    case ast::SN_declaration_statement:
    {
      auto const& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement);
      break;
    }
  }
}

void execute_body(std::string name, ast::statement_seq const& body)
{
  if (name != "compound_statement" && name != "thread")
  {
    if (name == "main")
      Dout(dc::notice, "int " << name << "()");
    else
    {
      Dout(dc::notice, "void " << name << "()");
    }
  }
  Context::instance().m_symbols.scope_start(name == "thread");
  for (auto const& statement : body.m_statements)
  {
    try
    {
      execute_statement(statement);
    }
    catch (AIAlert::Error const& alert)
    {
      THROW_ALERT(alert, " in `[STATEMENT]`", AIArgs("[STATEMENT]", statement));
    }
  }
  Context::instance().m_symbols.scope_end();
}

void Graph::write_png_file(std::string basename, std::vector<Action*> const& topological_ordered_actions, boolean::Expression const& valid, int appendix) const
{
  if (appendix >= 0)
  {
    std::ostringstream oss;
    oss << '_' << std::setfill('0') << std::setw(3) << appendix;
    basename += oss.str();
  }
  std::string const dot_filename = basename + ".dot";
  std::string const png_filename = basename + ".png";
  generate_dot_file(dot_filename, topological_ordered_actions, valid);
  std::string command = "dot ";
  if (Context::instance().number_of_threads() > 1)
    command += "-Kneato ";
  command += "-Tpng -o " + png_filename + " " + dot_filename;
  std::system(command.c_str());
}

int main(int argc, char* argv[])
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  //==========================================================================
  // Open the input file.

  char const* filepath;
  if (argc == 2)
  {
    filepath = argv[1];
  }
  else
  {
    std::cerr << "Usage: " << argv[0] << " <input file>\n";
    return 1;
  }

  std::ifstream in(filepath);
  if (!in.is_open())
  {
    std::cerr << "Failed to open input file \"" << filepath << "\".\n";
    return 1;
  }

  //==========================================================================
  // Parse the input file.

  std::string source_code;              // We will read the contents here.
  in.unsetf(std::ios::skipws);          // No white space skipping!
  std::copy(
      std::istream_iterator<char>(in),
      std::istream_iterator<char>(),
      std::back_inserter(source_code));
  in.close();

  ast::cppmem ast;
  iterator_type begin(source_code.begin());
  iterator_type const end(source_code.end());
  position_handler<iterator_type> position_handler(filepath, begin, end);
  try
  {
    if (!cppmem::parse(begin, end, position_handler, ast))
      return 1;
  }
  catch (std::exception const& error)
  {
    Dout(dc::warning, "Parser threw exception: " << error.what());
  }

  Graph graph;
  Context::instance().initialize(position_handler, graph);

  std::cout << "Abstract Syntax Tree: " << ast << std::endl;

  //==========================================================================
  // Collect all global variables and their initialization, if any.

  for (auto& node : ast)
    if (node.which() == ast::DN_declaration_statement)
    {
      ast::declaration_statement& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement);
    }

  //==========================================================================
  // Collect all function definitions.

  ast::function const* main_function;
  for (auto& node : ast)
    if (node.which() == ast::DN_function)
    {
      ast::function& function{boost::get<ast::function>(node)};
      if (function.id == -1)
      {
        Dout(dc::warning, "function " << function << " has id -1 !?!");
      }
      functions[function] = function;
      //std::cout << "Function definition: " << function << std::endl;
      if (function.m_function_name.name == "main")
        main_function = &functions[function];
    }

  //==========================================================================
  // Generate Xopsem: all actions/nodes with all Sequenced-Before and
  // Additionally-Synchronizes-With edges (we don't do Data-Dependencies).

  try
  {
    // Execute main()
    if (main_function->m_compound_statement.m_statement_seq)
      execute_body("main", *main_function->m_compound_statement.m_statement_seq);
  }
  catch (AIAlert::Error const& error)
  {
    for (auto&& line : error.lines())
    {
      if (line.prepend_newline()) std::cerr << std::endl;
      std::cerr << translate::getString(line.getXmlDesc(), line.args());
    }
    std::cerr << '.' << std::endl;
    return 1;
  }

  //==========================================================================
  // Brute force approach.

#ifdef CWDEBUG
  // Check that every node has a boolean product as exists condition.
  FilterAllActions all_actions;
  FollowUniqueOpsemTails follow_unique_opsem_tails;
  graph.for_actions_no_condition(
      follow_unique_opsem_tails,
      all_actions,
      [](Action* action)
      {
        Dout(dc::notice, "Testing " << *action);
        ASSERT(action->exists().is_product());
        return false;
      }
  );
#endif

  // Initialize m_sequence_number and m_sequenced_before of all Action nodes.
  std::vector<Action*> topological_ordered_actions;
  Action::initialize_post_opsem(graph, topological_ordered_actions);

  // Generate the *_opsem.dot file.
  std::string const path = filepath;
  std::string const source_filename = path.substr(path.find_last_of("/") + 1);
  std::string const basename = source_filename.substr(0, source_filename.find_last_of("."));
  graph.write_png_file(basename + "_opsem", topological_ordered_actions, true);

#if 0//def CWDEBUG
  // Print out all sequenced-before results.
  Debug(dc::for_action.off());
  graph.for_actions_no_condition(
      follow_unique_opsem_tails,
      all_actions,
      [&](Action* action1)
      {
        Dout(dc::notice, action1->name() << " -->");
        graph.for_actions_no_condition(
            follow_unique_opsem_tails,
            all_actions,
            [&](Action* action2)
            {
              if (action1->is_sequenced_before(*action2))
                Dout(dc::notice|nolabel_cf|nonewline_cf, ' ' << action2->name());
              return false;
            }
        );
        Dout(dc::notice|nolabel_cf, "");
        return false;
      }
  );
  Debug(dc::for_action.on());
#endif

  // A vector of objects representing all read nodes per memory location.
  std::vector<ReadFromLoopsPerLocation> read_from_loops_per_location_vector;

  // Run over all memory locations.
  for (auto&& location : Context::instance().locations())
  {
    Dout(dc::notice, "Considering location " << location);
    read_from_loops_per_location_vector.emplace_back();
    ReadFromLoopsPerLocation& read_from_loops_per_location(read_from_loops_per_location_vector.back());

    // Find all read actions for this location in topological order (their m_sequence_number).
    // Also reset the visited flags.
    for (Action* action : topological_ordered_actions)
    {
      if (action->location() == location && action->is_read())
      {
        Dout(dc::notice, "Found read " << *action);
        read_from_loops_per_location.add_read_action(action);
      }
    }

    int visited_generation = 0;
    bool new_writes_found = false;
    int rf_candidate = 0;
    size_t number_of_read_actions = read_from_loops_per_location.number_of_read_actions();
    Dout(dc::notice, "Number of read actions: " << number_of_read_actions);
    for (MultiLoop ml(number_of_read_actions); !ml.finished(); ml.next_loop())
      for (;;)
      {
        ReadFromLoop& read_from_loop{read_from_loops_per_location[*ml]};

        // Each read_from_loop basically has the following structure:
        //
        // begin();
        // while (find_next_write_action())
        // {
        //   add_edge();
        //   ... <next for loop or inner loop body>...
        //   delete_edge();
        // }
        //
        // Hence find_next_write_action() finds the first write action that we read from
        // when begin() was just called, otherwise advances to the next iteration, and
        // returns false when there are no write action (left).

        if (ml() == 0)                          // If we just entered this loop, call begin().
          read_from_loop.begin();
        else
          read_from_loop.delete_edge();         // Otherwise remove the edge that was added the previous loop.

        if (!read_from_loop.find_next_write_action(read_from_loops_per_location, ++visited_generation))
          break;

        if (read_from_loop.add_edge())
          new_writes_found = true;

        if (ml.inner_loop())
        {
          if (new_writes_found)
          {
            // Calculate under which condition this graph is valid.
            // The graph is valid when every read has a read-from edge when it exists.
            boolean::Expression valid{true};
            for (unsigned int read_loop = 0; read_loop < number_of_read_actions; ++read_loop)
            {
              ReadFromLoop& read_from_loop{read_from_loops_per_location[read_loop]};
              valid = valid.times(read_from_loop.have_write()(boolean::TruthProduct{read_from_loop.have_read().as_product()}));
            }
            graph.write_png_file(basename + "_rf", topological_ordered_actions, valid, rf_candidate++);
            new_writes_found = false;
          }
        }

        ++ml;
      }
    // FIXME: for now break so we only do one variable.
    break;
  }

  // Run over all possible flow-control paths.
  conditionals_type const& conditionals{Context::instance().conditionals()};
  int number_of_boolean_expressions = conditionals.size();
  using mask_type = boolean::Expression::mask_type;
  mask_type permutations_end = (mask_type)1 << number_of_boolean_expressions;
  Dout(dc::notice, "There " << (permutations_end == 1 ? "is " : "are ") <<
      permutations_end << " flow-control path permutation" << (permutations_end == 1 ? "" : "s") << ".");
  for (mask_type permutation = 0; permutation < permutations_end; ++permutation)
  {

  }
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct execute("EXECUTE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
