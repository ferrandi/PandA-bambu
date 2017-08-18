#this is file src/HLS/virtual_components/Makefile.am
noinst_LTLIBRARIES =  lib_virtual_components.la

lib_virtual_components_la_CPPFLAGS =  -I$(top_srcdir)/src \
                             -I$(top_srcdir)/src/HLS \
                             -I$(top_srcdir)/src/behavior \
                             -I$(top_srcdir)/src/graph \
                             -I$(top_srcdir)/src/circuit \
                             -I$(top_srcdir)/src/utility \
                             -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = generic_obj.hpp \
                 funit_obj.hpp \
                 register_obj.hpp \
                 mux_obj.hpp adder_conn_obj.hpp multiplier_conn_obj.hpp bitfield_obj.hpp \
                 commandport_obj.hpp dataport_obj.hpp conv_conn_obj.hpp

lib_virtual_components_la_SOURCES = \
   generic_obj.cpp \
   mux_obj.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
