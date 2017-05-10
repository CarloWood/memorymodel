AM_CPPFLAGS = -iquote $(top_srcdir)/cwds

bin_PROGRAMS = cppmem

cppmem_SOURCES = main.cxx

cppmem_CXXFLAGS = @LIBCWD_FLAGS@
cppmem_LDADD = $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
