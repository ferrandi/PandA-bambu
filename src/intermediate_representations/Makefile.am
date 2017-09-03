#this file is src/intermediate_representations/Makefile.am
SUBDIRS =
DIST_SUBDIRS =

METASOURCES = AUTO
noinst_HEADERS = \
   intermediate_representation.hpp
noinst_LTLIBRARIES = lib_intermediate_representations.la
lib_intermediate_representations_la_SOURCES = \
   intermediate_representation.cpp
lib_intermediate_representations_la_LIBADD =

lib_intermediate_representations_la_CPPFLAGS = \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_ACTOR_GRAPHS
   SUBDIRS += actor_graphs
   DIST_SUBDIRS += actor_graphs
   lib_intermediate_representations_la_LIBADD += $(top_builddir)/src/intermediate_representations/actor_graphs/lib_actor_graphs.la
endif

if BUILD_LIB_FROM_AADL_ASN
   SUBDIRS += aadl_asn
   DIST_SUBDIRS += aadl_asn
   lib_intermediate_representations_la_LIBADD += \
      $(top_builddir)/src/intermediate_representations/aadl_asn/lib_aadl_asn.la
endif

if BUILD_LIB_HLS
   SUBDIRS +=  hls
   DIST_SUBDIRS += hls
   lib_intermediate_representations_la_LIBADD += \
      $(top_builddir)/src/intermediate_representations/hls/lib_hls.la
endif

if BUILD_LIB_MAPPING
   SUBDIRS += mapping_annotations
   DIST_SUBDIRS += mapping_annotations
   lib_intermediate_representations_la_LIBADD += \
      $(top_builddir)/src/intermediate_representations/mapping_annotations/lib_mapping_annotations.la
endif

#do not touch the following line

