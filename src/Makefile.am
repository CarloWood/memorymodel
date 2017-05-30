AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

noinst_LIBRARIES = libcppmem.a
bin_PROGRAMS = cppmem_test cppmem

libcppmem_a_SOURCES = \
		 grammar_general.h \
		 grammar_whitespace.cxx \
		 grammar_whitespace.h \
		 grammar_vardecl.cxx \
		 grammar_vardecl.h \
		 grammar_statement.cxx \
		 grammar_statement.h \
		 grammar_cppmem.cxx \
		 grammar_cppmem.h \
		 grammar_unittest.cxx \
		 grammar_unittest.h \
		 ast.cxx \
		 ast.h \
		 cppmem_parser.cxx \
		 cppmem_parser.h

cppmem_test_SOURCES = cppmem_test.cxx

cppmem_SOURCES = cppmem.cxx

libcppmem_a_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@ #-DBOOST_SPIRIT_QI_DEBUG

cppmem_test_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@
cppmem_test_LDADD = libcppmem.a ../utils/libutils.la $(top_builddir)/cwds/libcwds.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@

cppmem_CXXFLAGS = -fmax-errors=1 @LIBCWD_FLAGS@
cppmem_LDADD = libcppmem.a ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
