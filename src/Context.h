#pragma once

#include "position_handler.h"
#include "Symbols.h"
#include "Locks.h"
#include "Loops.h"
#include "Graph.h"
#include <string>

using iterator_type = std::string::const_iterator;

struct Context {
  position_handler<iterator_type>& m_position_handler;
  Symbols m_symbols;
  Locks m_locks;
  Loops m_loops;
  Graph& m_graph;

 private:
  using sequence_point_id_type  = int;
  using node_ptr_type           = Graph::nodes_type::const_iterator;
  using value_computations_type = std::list<std::pair<sequence_point_id_type, node_ptr_type>>;
  using side_effects_type       = std::list<std::pair<sequence_point_id_type, node_ptr_type>>;

  sequence_point_id_type        m_current_sequence_point_id;
  value_computations_type       m_value_computations;
  side_effects_type             m_side_effects;

  int m_full_expression_detector_depth;

  void add_value_computation(node_ptr_type node) { m_value_computations.push_back(std::make_pair(m_current_sequence_point_id, node)); }
  void add_side_effect(node_ptr_type node) { m_side_effects.push_back(std::make_pair(m_current_sequence_point_id, node)); }

 public:
  Context(position_handler<iterator_type>& ph, Graph& g) : m_position_handler(ph), m_graph(g), m_current_sequence_point_id(0), m_full_expression_detector_depth(0) { }

  // Uninitialized declaration.
  void uninitialized(ast::tag decl);

  // Non-atomic read and writes.
  void read(ast::tag variable);
  void write(ast::tag variable, Evaluation&& evaluation);

  // Atomic read and writes.
  void read(ast::tag variable, std::memory_order mo);
  void write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation);

  // Mutex declaration and (un)locking.
  void lockdecl(ast::tag mutex);
  void lock(ast::tag mutex);
  void unlock(ast::tag mutex);

  // Called at sequence-points.
  void sequence_point();
  void detect_full_expression_start();
  void detect_full_expression_end();
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
struct FullExpressionDetector {
  Context& m_context;
  FullExpressionDetector(Context& context) : m_context(context) { context.detect_full_expression_start(); }
  ~FullExpressionDetector() { m_context.detect_full_expression_end(); }
};
