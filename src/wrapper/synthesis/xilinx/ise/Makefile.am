#this file is src/wrapper/synthesis/xilinx/ise/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_xilinx_ise.la

lib_xilinx_ise_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/parser/polixml \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/design_flows/backend/ToHDL \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/technology/physical_library/models \
    -I$(top_srcdir)/src/technology/physical_library/models/area \
    -I$(top_srcdir)/src/technology/physical_library/models/time \
    -I$(top_srcdir)/src/wrapper \
    -I$(top_srcdir)/src/wrapper/simulation \
    -I$(top_srcdir)/src/wrapper/synthesis \
    -I$(top_srcdir)/src/wrapper/synthesis/xilinx \
    -I$(top_srcdir)/src/wrapper/simulation/modelsim \
    -I$(BOOST_DIR) $(all_includes)

lib_xilinx_ise_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

noinst_HEADERS = xst_wrapper.hpp \
                 ngdbuild_wrapper.hpp \
                 map_wrapper.hpp \
                 trce_wrapper.hpp \
                 par_wrapper.hpp

lib_xilinx_ise_la_SOURCES = \
                           xst_wrapper.cpp \
                           ngdbuild_wrapper.cpp \
                           map_wrapper.cpp \
                           trce_wrapper.cpp \
                           par_wrapper.cpp

#do not touch the following line

