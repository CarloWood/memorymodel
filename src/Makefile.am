AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

noinst_LIBRARIES = libcppmem.a
bin_PROGRAMS = cppmem_test cppmem

libcppmem_a_SOURCES = \
		 grammar_whitespace.cxx \
		 grammar_whitespace.h \
		 grammar_cppmem.cxx \
		 grammar_cppmem.h \
		 grammar_unittest.cxx \
		 grammar_unittest.h \
		 cppmem_parser.cxx \
		 cppmem_parser.h \
		 Context.cxx \
		 Context.h \
		 ScopeDetector.h \
		 ScopeDetector.cxx \
		 Symbols_parser.cxx \
		 Symbols_parser.h \
		 Symbols.cxx \
		 Symbols.h \
		 Loops.cxx \
		 Loops.h \
		 Locks.h \
		 Locks.cxx \
		 Graph.cxx \
		 Graph.h \
		 Node.cxx \
		 Node.h \
		 ast.cxx \
		 ast.h \
		 Evaluation.cxx \
		 Evaluation.h \
		 position_handler.cxx \
		 position_handler.h \
		 grammar_general.h \
		 debug.cxx \
		 debug.h

cppmem_test_SOURCES = cppmem_test.cxx

cppmem_SOURCES = cppmem.cxx

libcppmem_a_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@ #-DBOOST_SPIRIT_QI_DEBUG

cppmem_test_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@
cppmem_test_LDADD = libcppmem.a ../utils/libutils.la $(top_builddir)/cwds/libcwds.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@

cppmem_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@
cppmem_LDADD = libcppmem.a ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
