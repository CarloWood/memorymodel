#pragma once

#include "debug.h"
#include "Symbols.h"
#include <boost/range/iterator_range.hpp>
#include <iostream>
#include <memory>

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct poshandler;
NAMESPACE_DEBUG_CHANNELS_END
#endif

template<typename Iterator>
struct position_handler
{
  template <typename, typename, typename>
  struct result { typedef void type; };

  position_handler(char const* filename, Iterator first, Iterator last) :
      m_filename(filename), first(first), last(last), m_iters(std::make_shared<std::vector<Iterator>>())
  {
    parser::Symbols::instance().reset();
  }

  boost::iterator_range<Iterator> get_line_and_range(Iterator pos, int& line, int& col) const
  {
    // Find start of current line, line- and column number;
    line = 1;
    col = 1;
    Iterator line_start = first;
    Iterator current = first;
    while (current != pos)
    {
      bool eol = false;
      if (*current == '\r')
      {
        eol = true;
        if (current++ == pos)
        {
          ++col;
          break;
        }
      }
      if (*current == '\n')
      {
        eol = true;
        if (current++ == pos)
        {
          ++col;
          break;
        }
      }
      if (eol)
      {
        ++line;
        line_start = current;
        col = 1;
      }
      else
      {
        ++current;
        ++col;
      }
    }
    // Find end of current line.
    Iterator line_end = pos;
    while (line_end != last && *line_end != '\r' && *line_end != '\n')
      ++line_end;
    return { line_start, line_end };
  }

  void show(std::ostream& os, boost::iterator_range<Iterator> const& range, int col) const
  {
    // col starts at 1.
    os << range << '\n' << std::string(col - 1, ' ') << '^';
  }

  template <typename Message, typename What>
  void operator()(Message const& message, What const& what, Iterator err_pos) const
  {
    int line, col;
    boost::iterator_range<Iterator> range{get_line_and_range(err_pos, line, col)};
    if (err_pos != last)
    {
      std::cout << message << what << " line " << line << ':' << std::endl;
      show(std::cout, range, col);
      std::cout << std::endl;
    }
    else
    {
      std::cout << "Unexpected end of file. ";
      std::cout << message << what << " line " << line << std::endl;
    }
  }

#ifdef CWDEBUG
  void show(libcwd::channel_ct const& debug_channel, Iterator pos) const
  {
    int line, col;
    boost::iterator_range<Iterator> range{get_line_and_range(pos, line, col)};
    Dout(debug_channel, range);
    Dout(debug_channel, std::string(col - 1, ' ') << '^');
  }
#endif

  std::string location(Iterator pos) const
  {
    int line, col;
    boost::iterator_range<Iterator> range{get_line_and_range(pos, line, col)};
    std::stringstream ss;
    ss << m_filename << ':' << line << ':' << col;
    return ss.str();
  }

  int pos_to_id(Iterator pos) const
  {
    int id;
    int const size = m_iters->size();
    for (id = 0; id != size; ++id)
      if ((*m_iters)[id] == pos)
        return id;
    m_iters->push_back(pos);
    return id;
  }

  Iterator id_to_pos(ast::tag const& tag) const
  {
    int id = tag.id;
    assert(id >= 0 && id < (int)m_iters->size());
    return (*m_iters)[id];
  }

  void operator()(ast::function& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator()(ast::function& {" << ast << "}, " << location(pos) << ")");
    ast.id = pos_to_id(pos);
    //m_functions[ast.id] = &ast;
    parser::Symbols::instance().function(ast);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(ast::vardecl& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator(ast::vardecl& {" << ast << "}, " << location(pos) << ")");
    ast.m_memory_location.id = pos_to_id(pos);
    ast.m_memory_location.m_type = ast.m_type;
    //m_memory_locations[ast.m_memory_location.id] = &ast.m_memory_location;
    parser::Symbols::instance().vardecl(ast.m_memory_location);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(ast::mutex_decl& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator(ast::mutex_decl& {" << ast << "}, " << location(pos) << ")");
    ast.id = pos_to_id(pos);
    parser::Symbols::instance().mutex_decl(ast);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(ast::condition_variable_decl& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator(ast::condition_variable_decl& {" << ast << "}, " << location(pos) << ")");
    ast.id = pos_to_id(pos);
    parser::Symbols::instance().condition_variable_decl(ast);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(ast::unique_lock_decl& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator(ast::unique_lock_decl& {" << ast << "}, " << location(pos) << ")");
    ast.id = pos_to_id(pos);
    parser::Symbols::instance().unique_lock_decl(ast);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(ast::register_assignment& ast, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator(ast::register_assignment& {" << ast << "}, " << location(pos) << ")");
    ast.lhs.id = pos_to_id(pos);
    parser::Symbols::instance().regdecl(ast.lhs);
    Debug(show(dc::poshandler, pos));
  }

  void operator()(int begin, Iterator pos) const
  {
    DoutEntering(dc::poshandler, "position_handler<Iterator>::operator()(" << ((begin == 1) ? "scope_begin" : (begin == 0) ? "scope_end" : "scope_end/scope_begin") << ", " << location(pos) << ")");
    parser::Symbols::instance().scope(begin);
    Debug(show(dc::poshandler, pos));
  }

  char const* const m_filename;
  Iterator const first;
  Iterator const last;
  std::shared_ptr<std::vector<Iterator>> m_iters;
};
