include $(srcdir)/cwm4/root_makefile_top.am

SUBDIRS = @CW_SUBDIRS@ . src

include $(srcdir)/cwm4/root_makefile_bottom.am

# Rules for compiling third-party library boolexpr:

FORCE:

boolexpr/Makefile: $(srcdir)/boolexpr/CMakeLists.txt $(srcdir)/boolexpr/cmake/*.cmake
	(mkdir -p boolexpr && cd boolexpr && cmake -G "Unix Makefiles" $(srcdir)/boolexpr)

boolexpr/libboolexpr.a: boolexpr/Makefile FORCE
	@$(MAKE) MAKEFLAGS=--quiet -C boolexpr boolexpr

clean-local:
	rm -rf boolexpr

all-local: boolexpr/libboolexpr.a
