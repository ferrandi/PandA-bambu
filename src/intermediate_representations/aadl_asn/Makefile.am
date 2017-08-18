#this file is src/intermediate_representations/aadl_asn/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_aadl_asn.la

lib_aadl_asn_la_CPPFLAGS = \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   aadl_information.hpp \
   asn_type.hpp

lib_aadl_asn_la_SOURCES = \
   aadl_information.cpp \
   asn_type.cpp

#do not touch the following line
include $(top_srcdir)/panda.am


