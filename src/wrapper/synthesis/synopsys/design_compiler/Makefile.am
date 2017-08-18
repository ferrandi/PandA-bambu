#this file is src/wrapper/synthesis/synopsys/design_compiler/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_designcompiler.la

lib_designcompiler_la_CPPFLAGS = \
                 -I$(top_srcdir)/src \
                 -I$(top_srcdir)/src/circuit \
                 -I$(top_srcdir)/src/constants \
                 -I$(top_srcdir)/src/design_flows/ \
                 -I$(top_srcdir)/src/design_flows/backend/ToHDL \
                 -I$(top_srcdir)/src/graph \
                 -I$(top_srcdir)/src/HLS/scheduling \
                 -I$(top_srcdir)/src/polixml \
                 -I$(top_srcdir)/src/parser/polixml \
                 -I$(top_srcdir)/src/technology \
                 -I$(top_srcdir)/src/technology/physical_library \
                 -I$(top_srcdir)/src/technology/physical_library/models \
                 -I$(top_srcdir)/src/technology/physical_library/models/area \
                 -I$(top_srcdir)/src/technology/target_device \
                 -I$(top_srcdir)/src/wrapper \
                 -I$(top_srcdir)/src/wrapper/synthesis \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys \
                 -I$(top_srcdir)/src/utility \
                 -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = DesignCompilerWrapper.hpp
lib_designcompiler_la_SOURCES = DesignCompilerWrapper.cpp

if BUILD_LIB_EXPERIMENTAL
lib_designcompiler_la_CPPFLAGS += -I$(top_srcdir)/src/circuit/aig\
                 -I$(top_srcdir)/src/logic_synthesis
endif

#do not touch the following line
include $(top_srcdir)/panda.am
