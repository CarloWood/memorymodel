#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include "pstreams/pstream.h"

std::string write(bool top_thread_writes_x, bool top_thread_writes_sc_first, bool bottom_thread_reads_sc_first, bool sc_is_written_to_twice, bool sc_second_write_is_relaxed)
{
  int unique_label =
    (top_thread_writes_x ? 1 : 0) +
    (top_thread_writes_sc_first ? 2 : 0) +
    (bottom_thread_reads_sc_first ? 4 : 0) +
    (sc_is_written_to_twice ? 8 : 0) +
    (sc_second_write_is_relaxed ? 16 : 0);
  std::ostringstream ss;
  ss << "test30_" << unique_label << ".c";
  std::ofstream out;
  out.open(ss.str());
  // Write test header.
  out << "int main()\n"
         "{\n"
         "  atomic_int sc = 0;\n"
         "  atomic_int x = 0;\n"
         "  {{{\n"
         "    {\n";
  // Write top thread.
  for (int line = 0 ; line < 2; ++line)
  {
    if ((line == 0) == top_thread_writes_sc_first)
    {
      // Write to sc.
      out << "      sc.store(1);\n";
      if (sc_is_written_to_twice)
      {
        if (sc_second_write_is_relaxed)
          out << "      sc.store(2, mo_relaxed);\n";
        else
          out << "      sc.store(2);\n";
      }
    }
    else
    {
      if (top_thread_writes_x)
      {
        out << "      x.store(1, mo_release);\n";
      }
      else
      {
        out << "      x.load(mo_acquire).readsvalue(1);\n";
      }
    }
  }
  // Thread separator code.
  out << "    }\n"
         "  |||\n"
         "    {\n";
  // Write bottom thread.
  for (int line = 0 ; line < 2; ++line)
  {
    if ((line == 0) == bottom_thread_reads_sc_first)
    {
      // Read from sc.
      out << "      sc.load().readsvalue(1);\n";
    }
    else
    {
      if (top_thread_writes_x)
      {
        out << "      x.load(mo_acquire).readsvalue(1);\n";
      }
      else
      {
        out << "      x.store(1, mo_release);\n";
      }
    }
  }
  // Write test footer.
  out << "    }\n"
         "  }}}\n"
         "}\n";
  return ss.str();
}

char const* commands = "set fontsize = 10\n"
                       "set node_height = 0.2\n"
                       "set node_width = 0.9\n"
                       "set xscale = 1.5\n"
                       "set yscale = 0.7\n"
                       "set nodesep = 0.25\n"
                       "set ranksep = 0.2\n"
                       "set penwidth = 1.0\n"
                       "set layout = neato_par_init\n"
                       "suppress_edge ithb\n"
                       "suppress_edge cad\n"
                       "suppress_edge vse\n"
                       "suppress_edge vsses\n"
                       "suppress_edge hrs\n"
                       "suppress_edge rs\n"
                       "suppress_edge dd\n"
//                       "suppress_edge sc\n"
                       "suppress_edge dr\n"
                       "next_consistent\n"
                       ;

int main()
{
  // Two variables: sc and x.
  // Top thread always writes sc with mo_seq_cst,
  // bottom thread always reads sc with mo_seq_cst.
  for (int n = 0; n < 32; ++n)
  {
    bool top_thread_writes_x = n & 1;
    bool top_thread_writes_sc_first = n & 2;
    bool bottom_thread_reads_sc_first = n & 4;
    bool sc_is_written_to_twice = n & 8;          // The second write is not read.
    bool sc_second_write_is_relaxed = n & 16;
    if (!sc_is_written_to_twice && sc_second_write_is_relaxed)
      continue;
    std::string const source_file = write(top_thread_writes_x, top_thread_writes_sc_first, bottom_thread_reads_sc_first, sc_is_written_to_twice, sc_second_write_is_relaxed);
    std::string const dot_file = std::regex_replace(std::regex_replace(source_file, std::regex("\\+"), "_"), std::regex("\\.c$"), ".dot");
    std::string const png_file = std::regex_replace(source_file, std::regex("\\.c$"), ".png");
    redi::pstream cppmem_prog("/home/carlo/projects/memorymodel/cppmem/cppmem -i " + source_file + " -o " + dot_file + " -quiet -terminal plain -stopat on_solutions -model standard");
    std::cout << "Executed command: \"" << cppmem_prog.command() << "\"." << std::endl;
    cppmem_prog << commands << "generate dot " << dot_file << std::endl;
    cppmem_prog << "quit" << std::endl;
    std::cout << cppmem_prog.rdbuf();
    //redi::opstream neato_prog("neato -Tpng -o " + png_file + " " + dot_file);
    //std::cout << neato_prog.rdbuf();
    break;
  }
}
