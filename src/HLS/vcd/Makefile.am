#this file is src/vcd/Makefile.am
noinst_LTLIBRARIES = lib_vcd.la

lib_vcd_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = vcd_trace_head.hpp VcdSignalSelection.hpp \
   CallSitesCollectorVisitor.hpp HWCallPathCalculator.hpp \
	CallGraphUnfolder.hpp UnfoldedFunctionInfo.hpp UnfoldedCallInfo.hpp \
	UnfoldedCallGraph.hpp Discrepancy.hpp vcd_utility.hpp

lib_vcd_la_SOURCES = vcd_trace_head.cpp VcdSignalSelection.cpp \
   CallSitesCollectorVisitor.cpp CallGraphUnfolder.cpp \
	vcd_utility.cpp HWCallPathCalculator.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
