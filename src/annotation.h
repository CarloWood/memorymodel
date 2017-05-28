#pragma once

#include "ast.h"
#include "error_handler.h"
#include "debug.h"

template <typename Iterator>
struct annotation
{
  template <typename, typename>
  struct result { using type = void; };

  error_handler<Iterator>& m_error_handler;

  annotation(error_handler<Iterator>& error_h) : m_error_handler(error_h) {}

#if 0
  struct set_id
  {
    using result_type = void;

    int id;
    set_id(int id) : id(id) {}

    void operator()(ast::function& x) const
    {
      x.id = id;
    }

    void operator()(ast::memory_location& x) const
    {
      x.id = id;
    }

    template <typename T>
    void operator()(T& x) const
    {
      // no-op
    }
  };

  void operator()(ast::operand& ast, Iterator pos) const
  {
    int id = m_error_handler.pos_to_id(pos);
    boost::apply_visitor(set_id(id), ast);
  }

  void operator()(ast::assignment& ast, Iterator pos) const
  {
    Dout(dc::notice, "ast::assignment");
    ast.lhs.id = m_error_handler.pos_to_id(pos);
  }
#endif

  void operator()(ast::function& ast, Iterator pos) const
  {
    DoutEntering(dc::notice, "annotation<Iterator>::operator()(ast::function& {" << ast << "}, " << m_error_handler.location(pos) << ")");
    ast.id = m_error_handler.pos_to_id(pos);
    Debug(m_error_handler.show(dc::notice, pos));
  }

  void operator()(ast::vardecl& ast, Iterator pos) const
  {
    DoutEntering(dc::notice, "annotation<Iterator>::operator(ast::vardecl& {" << ast << "}, " << m_error_handler.location(pos) << ")");
    ast.m_memory_location.id = m_error_handler.pos_to_id(pos);
    Debug(m_error_handler.show(dc::notice, pos));
  }

  void operator()(bool begin, Iterator pos) const
  {
    DoutEntering(dc::notice, "annotation<Iterator>::operator()(" << (begin ? "scope_begin" : "scope_end") << ", " << m_error_handler.location(pos) << ")");
    Debug(m_error_handler.show(dc::notice, pos));
  }
};
