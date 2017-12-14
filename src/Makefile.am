AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

noinst_LIBRARIES = libcppmem.a
bin_PROGRAMS = cppmem_test cppmem csc_test matchings test_generate_test30

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
		 Location.cxx \
		 Location.h \
		 Graph.cxx \
		 Graph.h \
		 NodePtr.cxx \
		 NodePtr.h \
		 FilterAllActions.h \
		 FilterWriteLocation.h \
		 FollowOpsemTails.h \
		 FollowUniqueOpsemTails.h \
		 FollowVisitedOpsemHeads.cxx \
		 FollowVisitedOpsemHeads.h \
		 ReadFromLoopsPerLocation.h \
		 ReadFromLoop.cxx \
		 ReadFromLoop.h \
		 DirectedEdge.cxx \
		 DirectedEdge.h \
		 DirectedEdgeTails.cxx \
		 DirectedEdgeTails.h \
		 DirectedSubgraph.cxx \
		 DirectedSubgraph.h \
		 ReadFromGraph.h \
		 ReadFromGraph.cxx \
		 ReadFromLocationSubgraphs.cxx \
		 ReadFromLocationSubgraphs.h \
		 Action.cxx \
		 Action.h \
		 Action.inl \
		 ActionSet.h \
		 ActionSet.inl \
		 Node.cxx \
		 Node.h \
		 EdgeType.cxx \
		 EdgeType.h \
		 Edge.cxx \
		 Edge.h \
		 EvaluationNodePtrs.cxx \
		 EvaluationNodePtrs.h \
		 Conditional.cxx \
		 Conditional.h \
		 Branch.cxx \
		 Branch.h \
		 Thread.cxx \
		 Thread.h \
		 BranchInfo.cxx \
		 BranchInfo.h \
		 NodePtrConditionPair.cxx \
		 NodePtrConditionPair.h \
		 Condition.cxx \
		 Condition.h \
		 ConditionalBranch.cxx \
		 ConditionalBranch.h \
		 SBNodePresence.cxx \
		 SBNodePresence.h \
		 LoopEvent.cxx \
		 LoopEvent.h \
		 PathConditionPerLoopEvent.cxx \
		 PathConditionPerLoopEvent.h \
		 TopologicalOrderedActions.cxx \
		 TopologicalOrderedActions.h \
		 debug_ostream_operators.cxx \
		 debug_ostream_operators.h \
		 ast.cxx \
		 ast.h \
		 iomanip_html.cxx \
		 iomanip_html.h \
		 iomanip_dotfile.cxx \
		 iomanip_dotfile.h \
		 Evaluation.cxx \
		 Evaluation.h \
		 position_handler.cxx \
		 position_handler.h \
		 grammar_general.h \
		 debug.cxx \
		 debug.h

cppmem_test_SOURCES = cppmem_test.cxx

cppmem_SOURCES = cppmem.cxx

csc_test_SOURCES = csc_test.cxx

matchings_SOURCES = matchings.cxx

test_generate_test30_SOURCES = test_generate_test30.cxx

libcppmem_a_CXXFLAGS = @LIBCWD_FLAGS@ #-DBOOST_SPIRIT_QI_DEBUG

cppmem_test_CXXFLAGS = @LIBCWD_FLAGS@
cppmem_test_LDADD = libcppmem.a ../boolean-expression/libboolean_expression.la ../utils/libutils.la $(top_builddir)/cwds/libcwds.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@

cppmem_CXXFLAGS = @LIBCWD_FLAGS@
cppmem_LDADD = libcppmem.a ../boolean-expression/libboolean_expression.la ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

csc_test_CXXFLAGS = @LIBCWD_FLAGS@
csc_test_LDADD = ../utils/libutils.la $(top_builddir)/cwds/libcwds.la

matchings_CXXFLAGS =

test_generate_test30_CXXFLAGS = -DREDI_EVISCERATE_PSTREAMS=0

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
