AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = cppmem

cppmem_SOURCES = \
		 main.cxx \
		 Program.cxx \
		 Program.h \
		 MemoryLocation.cxx \
		 MemoryLocation.h

cppmem_CXXFLAGS = @LIBCWD_FLAGS@
cppmem_LDADD = ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
