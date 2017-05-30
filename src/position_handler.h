#pragma once

#include <iostream>
#include "grammar_statement.h"
#include "debug.h"

template<typename Iterator>
struct position_handler
{
  template <typename, typename, typename>
  struct result { typedef void type; };

  position_handler(char const* filename, Iterator first, Iterator last) : m_filename(filename), first(first), last(last) {}

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

  int pos_to_id(Iterator /*pos*/) const
  {
    return 0;
  }

  void operator()(ast::function& ast, Iterator pos, parser::grammar_statement<Iterator>* stgp) const
  {
    DoutEntering(dc::notice, "position_handler<Iterator>::operator()(ast::function& {" << ast << "}, " << location(pos) << ", stgp)");
    ast.id = pos_to_id(pos);
    stgp->function(ast.m_function_name.name);
    Debug(show(dc::notice, pos));
  }

  void operator()(ast::vardecl& ast, Iterator pos, parser::grammar_statement<Iterator>* stgp) const
  {
    DoutEntering(dc::notice, "position_handler<Iterator>::operator(ast::vardecl& {" << ast << "}, " << location(pos) << ", stgp)");
    ast.m_memory_location.id = pos_to_id(pos);
    stgp->vardecl(ast.m_memory_location.m_name);
    Debug(show(dc::notice, pos));
  }

  void operator()(bool begin, Iterator pos, parser::grammar_statement<Iterator>* stgp) const
  {
    DoutEntering(dc::notice, "position_handler<Iterator>::operator()(" << (begin ? "scope_begin" : "scope_end") << ", " << location(pos) << ", stgp)");
    stgp->scope(begin);
    Debug(show(dc::notice, pos));
  }

  char const* const m_filename;
  Iterator first;
  Iterator last;
  std::vector<Iterator> iters;
};
