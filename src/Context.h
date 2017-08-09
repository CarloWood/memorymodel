#pragma once

#include "position_handler.h"
#include "Symbols.h"
#include "Locks.h"
#include "Loops.h"
#include "Graph.h"
#include "Branch.h"
#include "Conditional.h"
#include "EvaluationNodes.h"
#include <string>

using iterator_type = std::string::const_iterator;

struct Context
{
  using last_before_nodes_type = EvaluationNodes;
  using full_expression_evaluations_type = std::vector<std::unique_ptr<Evaluation>>;

  struct ConditionalBranch
  {
    conditionals_type::iterator m_conditional;
    ConditionalBranch(conditionals_type::iterator const& conditional) : m_conditional(conditional) { }
    Condition operator()(bool conditional_true) { return Condition(Branch(m_conditional, conditional_true)); }
    friend std::ostream& operator<<(std::ostream& os, ConditionalBranch const& conditional_branch);
  };

  position_handler<iterator_type>& m_position_handler;
  Symbols m_symbols;
  Locks m_locks;
  Loops m_loops;
  Graph& m_graph;

 private:
  Thread::id_type m_next_thread_id;                                     // The id to use for the next thread.
  ThreadPtr m_current_thread;                                           // The current thread.
  std::stack<bool> m_threads;                                           // Whether or not current scope is a thread.
  bool m_beginning_of_thread;                                           // Set to true when a new thread was just started.
  int m_full_expression_detector_depth;                                 // The current number of nested FullExpressionDetector objects.
  std::stack<std::unique_ptr<Evaluation>> m_last_full_expressions;      // The last full-expressions of parent threads.
  conditionals_type m_conditionals;                                     // Branch conditionals.
  full_expression_evaluations_type m_full_expression_evaluations;       // List of Evaluations of full expressions that need to be kept around.
  std::unique_ptr<Evaluation> m_previous_full_expression;               // The previous full expression, if not a condition
                                                                        //  (aka m_previous_full_expression_condition is 1),
                                                                        //  otherwise use m_last_condition.
  Condition m_previous_full_expression_condition;                       // The condition of the previous full expression (not 1), if that
                                                                        // is a full expression, otherwise this is 1.
  int m_last_condition;                                                 // The index into m_full_expression_evaluations of the previous full expression
                                                                        //  if the previous full expression is a condition
                                                                        //  (aka m_previous_full_expression_condition is not 1).
  int m_last_full_expression_of_previous_branch;                        // The index into m_full_expression_evaluations of the last full expression
                                                                        //  of the previous branch.
  int m_finalize_branch;                                                // Set when the next full expression comes after a selection statement;
                                                                        //  set to 1 when there was only a 'true' branch, set to 2 when there were
                                                                        //  two branches (aka, if (...) { ... } else { ... }  <next-full-expression>;
                                                                        //                          ^        ^               ^
                                                                        //                           \        \               \_ current position.
                                                                        //                            \        \_ m_last_full_expression_of_previous_branch.
                                                                        //                             \_ m_last_condition.

 public:
  Context(position_handler<iterator_type>& ph, Graph& g) :
      m_position_handler(ph),
      m_graph(g),
      m_next_thread_id{1},
      m_current_thread{Thread::create_main_thread()},
      m_beginning_of_thread(false),
      m_full_expression_detector_depth(0),
      m_finalize_branch(0) { }

  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();

  // Uninitialized declaration.
  Evaluation uninitialized(ast::tag decl);

  // Non-atomic read and writes.
  void read(ast::tag variable, Evaluation& evaluation);
  void write(ast::tag variable, Evaluation&& evaluation, bool side_effect_sb_value_computation);

  // Atomic read and writes.
  void read(ast::tag variable, std::memory_order mo, Evaluation& evaluation);
  NodePtr write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation);
  NodePtr RMW(ast::tag variable, std::memory_order mo, Evaluation&& evaluation);
  NodePtr compare_exchange_weak(ast::tag variable, ast::tag expected, int desired, std::memory_order success, std::memory_order fail, Evaluation&& evaluation);

  // Accessors.
  int number_of_threads() const { return m_next_thread_id; }
  conditionals_type const& conditionals() const { return  m_conditionals; }

  // Mutex declaration and (un)locking.
  Evaluation lockdecl(ast::tag mutex);
  Evaluation lock(ast::tag mutex);
  Evaluation unlock(ast::tag mutex);

  // Called at sequence-points.
  void detect_full_expression_start();
  void detect_full_expression_end(Evaluation& full_expression);

  // Generate node pairs for Sequenced-Before heads of before_nodes and Sequenced-Before tails of after_evaluation.
  Evaluation::node_pairs_type generate_node_pairs(
      EvaluationNodes const& before_nodes,
      Evaluation const& after_evaluation
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel));

  // Add edges of type edge_type for each node pair in node_pairs with condition.
  void add_edges(
      EdgeType edge_type,
      Evaluation::node_pairs_type node_pairs
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel),
      Condition const& condition = Condition());

  // Add unconditional edges of type edge_type between heads of before_evaluation and and tails of after_evaluation.
  void add_edges(
      EdgeType edge_type,
      Evaluation const& before_evaluation,
      Evaluation const& after_evaluation
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel));

  // Add edges of type edge_type between heads of before_evaluation and after_node.
  void add_edges(
      EdgeType edge_type,
      Evaluation const& before_evaluation,
      NodePtr const& after_node
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel));

  // Register a branch condition.
  ConditionalBranch add_condition(std::unique_ptr<Evaluation> const& condition)
  {
    DoutEntering(dc::notice, "Context::add_condition(" << *condition << ")");
    ASSERT(condition->is_valid());
    // The Evaluation object that the unique_ptr points at is stored in a std::set and will never move anymore.
    // Therefore we can use a normal pointer to "copy" the unique_ptr (as opposed to changing the unique_ptr
    // to shared_ptr everywhere). Moreover, we use this pointer as key for the std::map<Evaluation*, Conditional>
    // that the new Conditional is stored in.
    //
    // Is this a new condition?
    auto existing_entry = m_conditionals.find(condition.get());
    if (existing_entry != m_conditionals.end())
      return existing_entry;
    auto res = m_conditionals.emplace(condition.get(), Conditional());      // Calling Conditional() creates 'irreversible' a new boolean variable.
    // This created a new boolean variable, so the insertion has to be new.
    ASSERT(res.second);
    return res.first;
  }

  // The previous full-expression is a condition and we're about to execute
  // the branch that is followed when this condition is `condition_true`.
  ConditionalBranch begin_branch_with_condition(bool condition_true)
  {
    DoutEntering(dc::notice, "Context::begin_branch_with_condition(" << condition_true << ")");
    ASSERT(m_previous_full_expression);
    Dout(dc::notice|continued_cf, "Adding condition from m_previous_full_expression (" << *m_previous_full_expression << ") ");
    ConditionalBranch conditional_branch{add_condition(m_previous_full_expression)};        // The previous full-expression is a conditional.
    m_previous_full_expression_condition = conditional_branch(condition_true);              // We'll follow the true/false branch of this condition first.
    // We rely on this fact to know that m_previous_full_expression is a condition.
    ASSERT(m_previous_full_expression_condition.conditional());
    Dout(dc::finish, "with condition " << m_previous_full_expression_condition << '.');
    // Keep Evaluations that are conditionals alive.
    m_last_condition = m_full_expression_evaluations.size();
    Dout(dc::notice, "MOVING m_previous_full_expression (" << *m_previous_full_expression << ") to m_full_expression_evaluations!");
    m_full_expression_evaluations.push_back(std::move(m_previous_full_expression));
    return conditional_branch;
  }

  void begin_branch_with_condition(ConditionalBranch& conditional_branch, bool condition_true)
  {
    DoutEntering(dc::notice, "Context::begin_branch_with_condition(" << conditional_branch << ", " << condition_true << ")");
    m_last_full_expression_of_previous_branch = m_full_expression_evaluations.size();
    Dout(dc::notice, "MOVING m_previous_full_expression (" << *m_previous_full_expression << ") to m_full_expression_evaluations!");
    m_full_expression_evaluations.push_back(std::move(m_previous_full_expression));
    m_previous_full_expression_condition = conditional_branch(condition_true);
    // We rely on this fact to know that m_previous_full_expression is a condition.
    ASSERT(m_previous_full_expression_condition.conditional());
  }

  void end_branch_with_condition(ConditionalBranch& conditional_branch, int number_of_branches, bool last_branch_condition_true)
  {
    DoutEntering(dc::notice, "Context::end_branch_with_condition(" << conditional_branch << ", " << last_branch_condition_true << ")");
    ASSERT(number_of_branches == 1 || number_of_branches == 2);
    m_finalize_branch = number_of_branches;
    m_previous_full_expression_condition = conditional_branch(!last_branch_condition_true);
    Dout(dc::notice, "m_previous_full_expression_condition = " << m_previous_full_expression_condition);
  }

#ifdef CWDEBUG
  void print_previous_full_expression() const
  {
    Dout(dc::notice|continued_cf, "Previous full expression: " << *m_previous_full_expression);
  }
 private:
  static bool s_first_full_expression;
#endif
};

// Statements that are not expressions, but contain expressions, are
//
// new-placement
//      ( expression-list )
// noptr-new-declarator
//      [ expression ] attribute-specifier-seq_opt
//      noptr-new-declarator [ constant-expression ] attribute-specifier-seq_opt
// new-initializer
//      ( expression-list_opt )
// statement
//      attribute-specifier-seq_opt expression-statement
// labeled-statement
//      attribute-specifier-seqopt case constant-expression : statement
// condition
//      expression
// iteration-statement
//      do statement while ( expression ) ;
//      for ( for-init-statement condition_opt ; expression_opt ) statement
// for-init-statement
//      expression-statement
// for-range-initializer
//      expression braced-init-list
// jump-statement
//      return expression_opt ;
// static_assert-declaration
//      static_assert ( constant-expression , string-literal ) ;
// decltype-specifier
//      decltype ( expression )
// enumerator-definition
//      enumerator = constant-expression
// alignment-specifier
//      alignas ( alignment-expression ..._opt )
// noptr-declarator
//      noptr-declarator [ constant-expression_opt ] attribute-specifier-seq_opt
// declarator-id
//      ..._opt id-expression
// noptr-abstract-declarator
//      noptr-abstract-declarator_opt [ constant-expression ] attribute-specifier-seq_opt
// initializer
//      ( expression-list )
// initializer-clause
//      assignment-expression
// member-declarator
//      identifier_opt attribute-specifier-seq_opt virt-specifier-seq_opt : constant-expression
// mem-initializer
//      mem-initializer-id ( expression-list_opt )
// type-parameter
//      template < template-parameter-list > class identifieropt = id-expression
// template-argument
//      constant-expression
//      id-expression
// noexcept-specification
//      noexcept ( constant-expression )
// if-group
//      # if constant-expression new-line group_opt
// elif-group
//      # elif constant-expression new-line group_opt

// The only full-expressions (http://eel.is/c++draft/intro.execution#12) that we seem to be
// confronted with are http://eel.is/c++draft/intro.execution#12.5.
// - An expression that is not a subexpression of another expression and that is not otherwise part of a full-expression.
// Hence, we can detect full-expressions with a simple recursive counter around every expression,
// provided we mark the start of every full-expression right.
//
// From the above list that would be:
// - expression-statement (as part of statement and for-init-statement),
// - expression (as part of noptr-new-declarator, condition, iteration-statement, jump-statement and decltype-specifier),
// - expression-list (as part of new-placement, new-initializer, initializer and mem-initializer),
// - alignment-expression (as part of alignment-specifier),
// - id-expression (as part of declarator-id and type-parameter),
// - assignment-expression (as part of initializer-clause).
//
// Note that constant-expression is always a full-expression (as per http://eel.is/c++draft/intro.execution#12.2)
//

// Detect full expressions by creating a FullExpressionDetector object
// at the start of executing an expression that potentially is a full-
// expression.
//
// Note that an expression is a full-expression if it is not already
// part of another full-expression. Hence this detector works by
// counting the number of FullExpressionDetector objects and only
// processing an expression as a full expression when the last
// FullExpressionDetector is destructed.
//
struct FullExpressionDetector
{
  Evaluation& m_full_expression;        // Potential full-expression.
  Context& m_context;
  FullExpressionDetector(Evaluation& full_expression, Context& context) :
      m_full_expression(full_expression), m_context(context)
    { context.detect_full_expression_start(); }
  ~FullExpressionDetector()
    { m_context.detect_full_expression_end(m_full_expression); }
};
