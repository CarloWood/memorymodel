AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = cppmem_test cppmem

cppmem_test_SOURCES = \
		 cppmem.cxx \
		 cppmem.h \
		 cppmem_test.cxx

cppmem_SOURCES = \
		 cppmem.cxx \
		 cppmem.h \
		 spirit.cxx

cppmem_test_CXXFLAGS = -fmax-errors=3 @LIBCWD_FLAGS@
cppmem_test_LDADD = ../utils/libutils.la $(top_builddir)/cwds/libcwds.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@

cppmem_CXXFLAGS = -fmax-errors=3 @LIBCWD_FLAGS@
cppmem_LDADD = ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
