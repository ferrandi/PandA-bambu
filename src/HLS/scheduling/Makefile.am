#this is src/HLS/scheduling/Makefile.am
# silp_scheduling
METASOURCES = AUTO
SUBDIRS =
DIST_SUBDIRS =

noinst_LTLIBRARIES = lib_scheduling.la

lib_scheduling_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_COIN_OR
lib_scheduling_la_CPPFLAGS += \
                             -DHAVE_CONFIG_H \
                             -I$(COIN_OR_DIR)/Cbc/src \
                             -I$(COIN_OR_DIR)/Cgl/src \
                             -I$(COIN_OR_DIR)/Cgl/src/CglProbing \
                             -I$(COIN_OR_DIR)/CoinUtils/src \
                             -I$(COIN_OR_DIR)/Osi/src/Osi \
                             -I$(top_builddir)/$(COIN_OR_OBJDIR)/CoinUtils/src \
                             -I$(top_srcdir)/src/HLS/scheduling/newilp_scheduling

endif

noinst_HEADERS = \
   ASLAP.hpp \
   fixed_scheduling.hpp \
   parametric_list_based.hpp \
   priority.hpp \
   rehashed_heap.hpp \
   schedule.hpp \
   scheduling.hpp

lib_scheduling_la_SOURCES = \
   ASLAP.cpp \
   fixed_scheduling.cpp \
   parametric_list_based.cpp \
   priority.cpp \
   schedule.cpp \
   scheduling.cpp

lib_scheduling_la_LIBADD =

if BUILD_LIB_ILP
   noinst_HEADERS += \
      sdc_scheduling.hpp
   lib_scheduling_la_SOURCES += \
      sdc_scheduling.cpp
   lib_scheduling_la_CPPFLAGS += \
      -I$(top_srcdir)/src/algorithms/loops_detection \
      -I$(top_srcdir)/src/frontend_analysis \
      -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
      -I$(top_srcdir)/src/ilp
if BUILD_LIB_EXPERIMENTAL
   lib_scheduling_la_CPPFLAGS += \
      -I$(top_srcdir)/src/frontend_analysis
   SUBDIRS += ilp_scheduling silp_scheduling newilp_scheduling
   DIST_SUBDIRS += ilp_scheduling silp_scheduling newilp_scheduling
   noinst_HEADERS += ilp_scheduling.hpp ilp_scheduling_new.hpp silp_scheduling.hpp
   lib_scheduling_la_SOURCES += ilp_scheduling.cpp ilp_scheduling_new.cpp silp_scheduling.cpp
   lib_scheduling_la_LIBADD += $(top_builddir)/src/HLS/scheduling/ilp_scheduling/lib_ilp_scheduling.la $(top_builddir)/src/HLS/scheduling/silp_scheduling/lib_silp_scheduling.la $(top_builddir)/src/HLS/scheduling/newilp_scheduling/lib_newilp_scheduling.la
   lib_scheduling_la_CPPFLAGS += \
      -I$(top_srcdir)/src/HLS/scheduling/ilp_scheduling \
      -I$(top_srcdir)/src/HLS/scheduling/newilp_scheduling \
      -I$(top_srcdir)/src/technology/physical_library \
      -I$(top_srcdir)/src/technology/physical_library/models \
      -I$(top_srcdir)/src/HLS/scheduling/silp_scheduling
endif
endif

PRJ_DOC = scheduling.doc
if BUILD_LIB_EXPERIMENTAL
  PRJ_DOC += \
    path_based.doc ilp_scheduling_new.doc
endif

EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
