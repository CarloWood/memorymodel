#pragma once

#include "position_handler.h"
#include "Symbols.h"
#include "Locks.h"
#include "Loops.h"
#include "BranchInfo.h"
#include "EvaluationNodePtrs.h"
#include "Thread.h"
#include "Node.h"
#include <string>

class Graph;
using iterator_type = std::string::const_iterator;

struct Context
{
  using threads_final_full_expression_type = std::map<int, std::vector<std::unique_ptr<Evaluation>>>;

  position_handler<iterator_type>& m_position_handler;
  Symbols m_symbols;
  Locks m_locks;
  Loops m_loops;
  Graph& m_graph;

 private:
  //BranchInfo::full_expression_evaluations_type m_full_expression_evaluations;       // List of Evaluations of full expressions that need to be kept around.
  Thread::id_type m_next_thread_id;                                     // The id to use for the next thread.
  ThreadPtr m_current_thread;                                           // The current thread.
  std::stack<bool> m_threads;                                           // Whether or not current scope is a thread.
  conditionals_type m_conditionals;                                     // Branch conditionals.
  static std::vector<std::unique_ptr<Evaluation>> s_condition_evaluations;

 public:
  Context(position_handler<iterator_type>& ph, Graph& g) :
      m_position_handler(ph),
      m_graph(g),
      m_next_thread_id{1},
      m_current_thread{Thread::create_main_thread()} { }

  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();
  void start_threads();
  void join_all_threads();

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
  ThreadPtr const& current_thread() const { return m_current_thread; }
  conditionals_type const& conditionals() const { return  m_conditionals; }

  // Mutex declaration and (un)locking.
  Evaluation lockdecl(ast::tag mutex);
  Evaluation lock(ast::tag mutex);
  Evaluation unlock(ast::tag mutex);

  // Add edges of type edge_type between before_node_ptrs and after_node_ptrs with (optional) condition.
  void add_edges(
      EdgeType edge_type,
      EvaluationNodePtrs const& before_node_ptrs,
      EvaluationNodePtrs const& after_node_ptrs
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel),
      Condition const& condition = Condition());

  // Add edges of type sb or asw between before_node_ptr_condition_pairs and after_node_ptrs.
  void add_sb_or_asw_edges(
      EvaluationNodePtrConditionPairs& before_node_ptr_condition_pairs,
      EvaluationNodePtrs const& after_node_ptrs);

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
    DoutEntering(dc::branch, "Context::add_condition(" << *condition << ")");
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
    auto res = m_conditionals.emplace(condition.get(), Conditional()); // Calling Conditional() creates 'irreversible' a new boolean variable.
    // This created a new boolean variable, so the insertion has to be new.
    ASSERT(res.second);
    return res.first;
  }

  ConditionalBranch add_condition(std::unique_ptr<Evaluation>&& condition)
  {
    ConditionalBranch result{add_condition(condition)};
    // Keep a std::unique_ptr around to stop this Evaluation from being deleted.
    s_condition_evaluations.emplace_back(std::move(condition));
    return result;
  }
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
    { context.current_thread()->detect_full_expression_start(); }
  ~FullExpressionDetector()
    { m_context.current_thread()->detect_full_expression_end(m_full_expression, m_context); }
};
