#pragma once

#include "position_handler.h"
#include "Symbols.h"
#include "Locks.h"
#include "Loops.h"
#include "Graph.h"
#include "Branch.h"
#include "Condition.h"
#include <string>

using iterator_type = std::string::const_iterator;

struct Context
{
  struct ConditionalBranch
  {
    conditions_type::iterator m_condition;
    ConditionalBranch(conditions_type::iterator const& condition) : m_condition(condition) { }
    Branches operator()(bool condition_true) { return Branches(Branch(m_condition, condition_true)); }
  };

  position_handler<iterator_type>& m_position_handler;
  Symbols m_symbols;
  Locks m_locks;
  Loops m_loops;
  Graph& m_graph;

 private:
  int m_full_expression_detector_depth;
  Evaluation m_last_full_expression;
  Thread::id_type m_next_thread_id;                     // The id to use for the next thread.
  ThreadPtr m_current_thread;                           // The current thread.
  std::stack<bool> m_threads;                           // Whether or not current scope is a thread.
  std::stack<Evaluation> m_last_full_expressions;       // Last full-expressions of parent threads.
  bool m_beginning_of_thread;                           // Set to true when a new thread was just started.
  conditions_type m_conditions;                         // Branch conditions.

 public:
  Context(position_handler<iterator_type>& ph, Graph& g) :
      m_position_handler(ph),
      m_graph(g),
      m_full_expression_detector_depth(0),
      m_last_full_expression(Evaluation::not_used),
      m_next_thread_id{1},
      m_current_thread{Thread::create_main_thread()},
      m_beginning_of_thread(false) { }

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
  Evaluation::node_iterator write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation);

  // Accessors.
  int number_of_threads() const { return m_next_thread_id; }
  conditions_type const& conditions() const { return  m_conditions; }

  // Mutex declaration and (un)locking.
  Evaluation lockdecl(ast::tag mutex);
  Evaluation lock(ast::tag mutex);
  Evaluation unlock(ast::tag mutex);

  // Called at sequence-points.
  void detect_full_expression_start();
  void detect_full_expression_end(Evaluation& full_expression);

  // Add edges between heads of before_evaluation and after_node.
  void add_edges(
      EdgeType edge_type,
      Evaluation const& before_evaluation,
      Evaluation::node_iterator const& after_node
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel));
  // Generate node pairs for edge_type edges between heads of before_evaluation and tails of after_evaluation.
  // Pass the result to the add_edges below.
  Evaluation::node_pairs_type generate_node_pairs(
      Evaluation const& before_evaluation,
      Evaluation const& after_evaluation
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel)) const;
  // Add edges for each node pair in node_pairs.
  void add_edges(
      EdgeType edge_type,
      Evaluation::node_pairs_type node_pairs
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel),
      Branches const& branches = Branches());
  // Short circuit for the combination of the above two member functions.
  void add_edges(
      EdgeType edge_type,
      Evaluation const& before_evaluation,
      Evaluation const& after_evaluation
      COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel));
  // Register a branch condition.
  ConditionalBranch add_condition(std::unique_ptr<Evaluation> const& condition)
  {
    // The Evaluation object that the unique_ptr points at is stored in a std::set and will never move anymore.
    // Therefore we can use a normal pointer to "copy" the unique_ptr (as opposed to changing the unique_ptr
    // to shared_ptr everywhere). Moreover, we use this pointer as key for the std::map<Evaluation*, Condition>
    // that the new Condition is stored in.
    //
    // Is this a new condition?
    auto existing_entry = m_conditions.find(condition.get());
    if (existing_entry != m_conditions.end())
      return existing_entry;
    auto res = m_conditions.emplace(condition.get(), Condition());      // Calling Condition() creates 'irreversible' a new boolean variable.
    // This created a new boolean variable, so the insertion has to be new.
    ASSERT(res.second);
    return res.first;
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
struct FullExpressionDetector
{
  Evaluation& m_full_expression;        // Potential full-expression.
  Context& m_context;
  FullExpressionDetector(Evaluation& full_expression, Context& context) :
      m_full_expression(full_expression), m_context(context)
    { context.detect_full_expression_start(); }
  ~FullExpressionDetector() { m_context.detect_full_expression_end(m_full_expression); }
};
