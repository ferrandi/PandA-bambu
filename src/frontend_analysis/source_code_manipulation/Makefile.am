#Tthis file is src/frontend_analysis/source_code_manipulation/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_source_code_manipulation.la

lib_source_code_manipulation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/pragma \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS =
lib_source_code_manipulation_la_SOURCES =

if BUILD_LIB_FROM_PRAGMA
noinst_HEADERS += \
   pragma_substitution.hpp

lib_source_code_manipulation_la_SOURCES += \
   pragma_substitution.cpp
endif

if BUILD_TASTE
lib_source_code_manipulation_la_CPPFLAGS += \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/intermediate_representations/aadl_asn \
   -I$(top_srcdir)/src/tree
noinst_HEADERS += \
   create_address_translation.hpp
lib_source_code_manipulation_la_SOURCES += \
   create_address_translation.cpp
endif

#do not touch the following line

