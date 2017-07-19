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
#include "cppmem_parser.h"
#include "utils/AIAlert.h"
#include <libcwd/type_info.h>
#include <cstdlib>      // std::system
#include <boost/variant/get.hpp>
#include <iostream>
#include <fstream>

std::map<ast::tag, ast::function, TagCompare> functions;

void execute_body(std::string name, ast::statement_seq const& body, Context& context);
Evaluation execute_expression(ast::assignment_expression const& expression, Context& context);
Evaluation execute_expression(ast::expression const& expression, Context& context);

void execute_declaration(ast::declaration_statement const& declaration_statement, Context& context)
{
  DoutEntering(dc::notice, "execute_declaration(`" << declaration_statement << "`)");

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
  FullExpressionDetector detector(full_expression, context);

  switch (declaration_statement.m_declaration_statement_node.which())
  {
    case ast::DS_mutex_decl:
    {
      auto const& mutex_decl{boost::get<ast::mutex_decl>(declaration_statement.m_declaration_statement_node)};
      Dout(dc::notice, declaration_statement);
      DebugMarkUp;
      full_expression = context.lockdecl(mutex_decl);
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
      full_expression = context.lock(unique_lock_decl.m_mutex);
      context.m_locks.add(unique_lock_decl, context);
      break;
    }
    case ast::DS_vardecl:
    {
      auto const& vardecl{boost::get<ast::vardecl>(declaration_statement.m_declaration_statement_node)};
      if (vardecl.m_initial_value)
      {
        // This is a `initializer-clause` - but is this already part of an expression (the expression-statement that is the declaration)?
        full_expression = execute_expression(*vardecl.m_initial_value, context);
        Dout(dc::notice, declaration_statement);
        DebugMarkUp;
        full_expression.write(declaration_statement.tag(), context);
      }
      else
      {
        Dout(dc::notice, declaration_statement);
        DebugMarkUp;
        full_expression = context.uninitialized(declaration_statement.tag());
      }
      break;
    }
  }
  context.m_symbols.add(declaration_statement);
}

Evaluation execute_condition(ast::expression const& condition, Context& context)
{
  DoutEntering(dc::notice, "execute_condition(`" << condition << "`)");

  Evaluation result = execute_expression(condition, context);
  Dout(dc::continued, condition);
  return result;
}

Evaluation execute_primary_expression(ast::primary_expression const& primary_expression, Context& context)
{
  DoutEntering(dc::notice, "execute_primary_expression(`" << primary_expression << "`)");

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
      ast::declaration_statement const& declaration_statement{context.m_symbols.find(name)}; // Declaration of actual object.
      if (declaration_statement.tag() != tag)                                      // Did the parser make a mistake?
      {
        std::string position{context.m_position_handler.location(declaration_statement.tag())};
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
      result.read(tag, context);
      break;
    }
    case ast::PE_expression:
    {
      ast::expression const& expression{boost::get<ast::expression>(node)};
      result = execute_expression(expression, context);
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
Evaluation execute_operator_list_expression(T const& expr, Context& context)
{
  // Only print Entering.. when there is actually an operator.
  DoutEntering(dc::notice(!expr.m_chained.empty()),
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
  Evaluation result = execute_operator_list_expression(expr.m_other_expression, context);
  for (auto const& tail : expr.m_chained)
  {
    Evaluation rhs = execute_operator_list_expression(tail, context);
    if (std::is_same<T, ast::logical_or_expression>::value ||
        std::is_same<T, ast::logical_and_expression>::value)
    {
      Dout(dc::sb_edge, "Boolean expression (operator || or &&)");
      DebugMarkUp;
      context.add_edges(
          edge_sb,
          context.generate_node_pairs(result, rhs COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge))
          COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    }
    result.OP(get_operator<T>(tail), std::move(rhs));
  }
  return result;
}

// Specialization for boost::fusion::tuple<operators, prev_precedence_type>
template<typename OPERATORS, typename PREV_PRECEDENCE_TYPE>
Evaluation execute_operator_list_expression(boost::fusion::tuple<OPERATORS, PREV_PRECEDENCE_TYPE> const& tuple, Context& context)
{
  // Doesn't really make much sense to print this, as the function being called already prints basically the same info.
  //DoutEntering(dc::notice, "execute_operator_list_expression<" <<
  //    type_info_of<OPERATORS>().demangled_name() << ", " <<
  //    type_info_of<PREV_PRECEDENCE_TYPE>().demangled_name() << ">(" << tuple << ").");

  // Just unpack the second argument.
  return execute_operator_list_expression(boost::fusion::get<1>(tuple), context);
}

Evaluation execute_postfix_expression(ast::postfix_expression const& expr, Context& context)
{
  // Only print Entering... when we actually have a pre- increment, decrement or unary operator.
  DoutEntering(dc::notice(!expr.m_postfix_operators.empty()), "execute_postfix_expression(`" << expr << "`)");

  Evaluation result;

  auto const& node = expr.m_postfix_expression_node;
  ASSERT(node.which() == ast::PE_primary_expression);
  auto const& primary_expression{boost::get<ast::primary_expression>(node)};
  result = execute_primary_expression(primary_expression, context);

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
      result.write(tag, context);
    }
  }

  return result;
}

// Specialization for unary_expression.
template<>
Evaluation execute_operator_list_expression(ast::unary_expression const& expr, Context& context)
{
  // Only print Entering... when we actually have a pre- increment, decrement or unary operator.
  DoutEntering(dc::notice(!expr.m_unary_operators.empty()), "execute_operator_list_expression(`" << expr << "`) [for pre-increment, pre-decrement and unary expressions]");

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
      result = execute_postfix_expression(expr.m_postfix_expression, context);

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
          result.write(tag, context, true);
        }
        else
          result.unary_operator(unary_operator);
      }
      break;
    }
    case ast::PE_atomic_fetch_add_explicit:
    {
      auto const& atomic_fetch_add_explicit{boost::get<ast::atomic_fetch_add_explicit>(node)};
      // FIXME TODO
      //result = execute_expression(atomic_fetch_add_explicit.m_expression, context);
      //context.read(atomic_fetch_add_explicit.m_memory_location_id, atomic_fetch_add_explicit.m_memory_order);
      //context.write(atomic_fetch_add_explicit.m_memory_location_id, atomic_fetch_add_explicit.m_memory_order, std::move(result));
      result = atomic_fetch_add_explicit.m_memory_location_id;
      break;
    }
    case ast::PE_atomic_fetch_sub_explicit:
    {
      auto const& atomic_fetch_sub_explicit{boost::get<ast::atomic_fetch_sub_explicit>(node)};
      // FIXME TODO
      //result = execute_expression(atomic_fetch_sub_explicit.m_expression, context);
      //context.read(atomic_fetch_sub_explicit.m_memory_location_id, atomic_fetch_sub_explicit.m_memory_order);
      //context.write(atomic_fetch_sub_explicit.m_memory_location_id, atomic_fetch_sub_explicit.m_memory_order, std::move(result));
      result = atomic_fetch_sub_explicit.m_memory_location_id;
      break;
    }
    case ast::PE_atomic_compare_exchange_weak_explicit:
    {
      auto const& atomic_compare_exchange_weak_explicit{boost::get<ast::atomic_compare_exchange_weak_explicit>(node)};
      DoutTag(dc::notice, "TODO: [load/(store) of", atomic_compare_exchange_weak_explicit.m_memory_location_id);
      // FIXME TODO
      //result =
      break;
    }
    case ast::PE_load_call:
    {
      auto const& load_call{boost::get<ast::load_call>(node)};
      result = load_call.m_memory_location_id;
      result.read(load_call.m_memory_location_id, load_call.m_memory_order, context);
      break;
    }
  }
  return result;
}

Evaluation execute_expression(ast::assignment_expression const& expression, Context& context)
{
  DoutEntering(dc::notice, "execute_expression(`" << expression << "`).");
  DebugMarkDown;

  Evaluation result;
  // - expression-statement (as part of statement and for-init-statement)
  FullExpressionDetector detector(result, context);

  auto const& node = expression.m_assignment_expression_node;
  switch (node.which())
  {
    case ast::AE_conditional_expression:
    {
      auto const& conditional_expression{boost::get<ast::conditional_expression>(node)};
      result = execute_operator_list_expression(conditional_expression.m_logical_or_expression, context);
      if (conditional_expression.m_conditional_expression_tail)
      {
        ast::expression const& expression{boost::fusion::get<0>(conditional_expression.m_conditional_expression_tail.get())};
        ast::assignment_expression const& assignment_expression{boost::fusion::get<1>(conditional_expression.m_conditional_expression_tail.get())};
        Evaluation true_evaluation = execute_expression(expression, context);
        Evaluation false_evaluation = execute_expression(assignment_expression, context);
        result.conditional_operator(
            std::move(true_evaluation),
            context.generate_node_pairs(result, true_evaluation, DEBUGCHANNELS::dc::sb_edge),
            std::move(false_evaluation),
            context.generate_node_pairs(result, false_evaluation, DEBUGCHANNELS::dc::sb_edge),
            context);
      }
      break;
    }
    case ast::AE_register_assignment:
    {
      auto const& register_assignment{boost::get<ast::register_assignment>(node)};
      result = execute_expression(register_assignment.rhs, context);
      // Assignment to a register doesn't generate a side-effect (we're not really writing to memory),
      // so just leave the result what it is as it represents the full-expression of this assignment.
      break;
    }
    case ast::AE_assignment:
    {
      auto const& assignment{boost::get<ast::assignment>(node)};
      result = execute_expression(assignment.rhs, context);
      Dout(dc::valuecomp, "Assignment value computation results in " << result << "; assigned to `" << assignment.lhs << "`.");
      result.write(assignment.lhs, context);
      break;
    }
  }
  return result;
}

Evaluation execute_expression(ast::expression const& expression, Context& context)
{
  DoutEntering(dc::notice(expression.m_chained.size() > 1), "execute_expression(`" << expression << "`).");
  Evaluation result;
  // - expression (as part of noptr-new-declarator, condition, iteration-statement, jump-statement and decltype-specifier)
  FullExpressionDetector detector(result, context);
  result = execute_expression(expression.m_assignment_expression, context);
  for (auto const& assignment_expression : expression.m_chained)
  {
    Evaluation rhs = execute_expression(assignment_expression, context);
    context.add_edges(
        edge_sb,
        context.generate_node_pairs(result, rhs COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge))
        COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    result.comma_operator(std::move(rhs));
  }
  return result;
}

void execute_statement(ast::statement const& statement, Context& context)
{
  auto const& node = statement.m_statement_node;
  switch (node.which())
  {
    case ast::SN_expression_statement:
    {
      auto const& expression_statement{boost::get<ast::expression_statement>(node)};
      if (expression_statement.m_expression)
      {
        execute_expression(expression_statement.m_expression.get(), context);
        Dout(dc::notice, expression_statement);
      }
      break;
    }
    case ast::SN_store_call:
    {
      auto const& store_call{boost::get<ast::store_call>(node)};
      Evaluation value = execute_expression(store_call.m_val, context);
      Dout(dc::notice, store_call << ";");
      DebugMarkUp;
      value.write(store_call.m_memory_location_id, store_call.m_memory_order, context);
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
        execute_body(function.m_function_name.name, *function.m_compound_statement.m_statement_seq, context);
      break;
    }
    case ast::SN_wait_call:
    {
      auto const& wait_call{boost::get<ast::wait_call>(node)};
      Dout(dc::notice, "TODO: " << wait_call);
      break;
    }
    case ast::SN_notify_all_call:
    {
      auto const& notify_all_call{boost::get<ast::notify_all_call>(node)};
      Dout(dc::notice, "TODO: " << notify_all_call);
      break;
    }
    case ast::SN_mutex_lock_call:
    {
      auto const& mutex_lock_call{boost::get<ast::mutex_lock_call>(node)};
      Dout(dc::notice, mutex_lock_call);
      DebugMarkUp;
      context.lock(mutex_lock_call.m_mutex);
      break;
    }
    case ast::SN_mutex_unlock_call:
    {
      auto const& mutex_unlock_call{boost::get<ast::mutex_unlock_call>(node)};
      Dout(dc::notice, mutex_unlock_call);
      DebugMarkUp;
      context.unlock(mutex_unlock_call.m_mutex);
      break;
    }
    case ast::SN_threads:
    {
      auto const& threads{boost::get<ast::threads>(node)};
      for (auto& statement_seq : threads.m_threads)
        execute_body("thread", statement_seq, context);
      break;
    }
    case ast::SN_compound_statement:
    {
      auto const& compound_statement{boost::get<ast::compound_statement>(node)};
      if (compound_statement.m_statement_seq)
        execute_body("compound_statement", *compound_statement.m_statement_seq, context);
      break;
    }
    case ast::SN_selection_statement:
    {
      auto const& selection_statement{boost::get<ast::selection_statement>(node)};
      Dout(dc::notice|continued_cf, "if (");
      try { execute_condition(selection_statement.m_if_statement.m_condition, context); }
      catch (std::exception const&) { Dout(dc::finish, ""); throw; }
      Dout(dc::finish, ")");
      execute_statement(selection_statement.m_if_statement.m_then, context);
      if (selection_statement.m_if_statement.m_else)
        execute_statement(*selection_statement.m_if_statement.m_else, context);
      break;
    }
    case ast::SN_iteration_statement:
    {
      auto const& iteration_statement{boost::get<ast::iteration_statement>(node)};
      // We only support while () { } at the moment.
      Dout(dc::notice|continued_cf, "while (");
      try { execute_condition(iteration_statement.m_while_statement.m_condition, context); }
      catch (std::exception const&) { Dout(dc::finish, ""); throw; }
      Dout(dc::finish, ")");
      context.m_loops.enter(iteration_statement);
      execute_statement(iteration_statement.m_while_statement.m_statement, context);
      context.m_loops.leave(iteration_statement);
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
          context.m_loops.add_break(break_statement);
          break;
        }
        case ast::JS_return_statement:
        {
          auto const& return_statement{boost::get<ast::return_statement>(jump_statement.m_jump_statement_node)};
          Dout(dc::notice, return_statement);
          DebugMarkUp;
          execute_expression(return_statement.m_expression, context);
          break;
        }
      }
      break;
    }
    case ast::SN_declaration_statement:
    {
      auto const& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement, context);
      break;
    }
  }
}

void execute_body(std::string name, ast::statement_seq const& body, Context& context)
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
  context.m_symbols.scope_start(name == "thread", context);
  for (auto const& statement : body.m_statements)
  {
    try
    {
      execute_statement(statement, context);
    }
    catch (AIAlert::Error const& alert)
    {
      THROW_ALERT(alert, " in `[STATEMENT]`", AIArgs("[STATEMENT]", statement));
    }
  }
  context.m_symbols.scope_end(context);
}

int main(int argc, char* argv[])
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  char const* filename;
  if (argc == 2)
  {
    filename = argv[1];
  }
  else
  {
    std::cerr << "Usage: " << argv[0] << " <input file>\n";
    return 1;
  }

  std::ifstream in(filename);
  if (!in.is_open())
  {
    std::cerr << "Failed to open input file \"" << filename << "\".\n";
    return 1;
  }

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
  position_handler<iterator_type> position_handler(filename, begin, end);
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
  Context context(position_handler, graph);

  std::cout << "Abstract Syntax Tree: " << ast << std::endl;

  // Collect all global variables and their initialization, if any.
  for (auto& node : ast)
    if (node.which() == ast::DN_declaration_statement)
    {
      ast::declaration_statement& declaration_statement{boost::get<ast::declaration_statement>(node)};
      execute_declaration(declaration_statement, context);
    }

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

  try
  {
    // Execute main()
    if (main_function->m_compound_statement.m_statement_seq)
      execute_body("main", *main_function->m_compound_statement.m_statement_seq, context);
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

  std::string const source_filename = filename;
  std::string const basename = source_filename.substr(0, source_filename.find_last_of("."));
  std::string const dot_filename = basename + ".dot";
  std::string const png_filename = basename + ".png";
  graph.generate_dot_file(dot_filename);
  std::string command = "dot -Kdot -Tpng -o " + png_filename + " " + dot_filename;
  std::system(command.c_str());
}
