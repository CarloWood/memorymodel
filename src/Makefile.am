AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = cppmem

cppmem_SOURCES = \
		 cppmem.cxx \
		 cppmem.h \
		 cppmem_test.cxx

cppmem_CXXFLAGS = -fmax-errors=3 @LIBCWD_FLAGS@
cppmem_LDADD = ../utils/libutils.la $(top_builddir)/cwds/libcwds.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
