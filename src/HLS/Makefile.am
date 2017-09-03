#this file is src/HLS/Makefile.am
METASOURCES = AUTO

DIST_SUBDIRS = \
   architecture_creation \
   backend \
   binding \
   chaining \
   evaluation \
   function_allocation\
   hls_flow \
   interface \
   liveness \
   memory \
   module_allocation \
   module_generator \
   scheduling \
   simulation \
   stg\
   virtual_components
SUBDIRS = \
   architecture_creation \
   backend \
   binding \
   chaining \
   evaluation \
   function_allocation\
   hls_flow \
   interface \
   liveness \
   memory \
   module_allocation \
   module_generator \
   scheduling \
   simulation \
   stg\
   virtual_components

noinst_LTLIBRARIES = lib_HLS.la

lib_HLS_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows/  \
   -I$(top_srcdir)/src/design_flows/backend  \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/backend \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/evaluation/exact \
   -I$(top_srcdir)/src/HLS/evaluation/linear \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/hls_flow  \
   -I$(top_srcdir)/src/HLS/hls_flow/synthesis  \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/interface/minimal \
   -I$(top_srcdir)/src/HLS/interface/WB4 \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation\
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR)  $(all_includes)

noinst_HEADERS = \
   hls.hpp \
   hls_constraints.hpp \
   hls_manager.hpp \
   hls_target.hpp \
   hls_flow_step_factory.hpp \
   hls_function_step.hpp \
   hls_manager.hpp \
   hls_step.hpp \
   hls_target.hpp

lib_HLS_la_SOURCES = \
   hls.cpp \
   hls_constraints.cpp \
   hls_flow_step_factory.cpp \
   hls_function_step.cpp \
   hls_manager.cpp \
   hls_step.cpp \
   hls_target.cpp

lib_HLS_la_LIBADD = \
           $(top_builddir)/src/HLS/architecture_creation/lib_architecture_creation.la \
           $(top_builddir)/src/HLS/backend/lib_hls_backend.la \
           $(top_builddir)/src/HLS/hls_flow/lib_hls_flow.la \
           $(top_builddir)/src/HLS/scheduling/lib_scheduling.la \
           $(top_builddir)/src/HLS/module_allocation/lib_allocation.la \
           $(top_builddir)/src/HLS/function_allocation/lib_function_allocation.la \
           $(top_builddir)/src/HLS/binding/lib_binding.la \
           $(top_builddir)/src/HLS/stg/lib_stg.la \
           $(top_builddir)/src/HLS/liveness/lib_liveness.la \
           $(top_builddir)/src/HLS/memory/lib_HLS_memory.la \
           $(top_builddir)/src/HLS/simulation/lib_simulation.la \
           $(top_builddir)/src/HLS/module_generator/lib_module_generator.la \
           $(top_builddir)/src/HLS/virtual_components/lib_virtual_components.la \
           $(top_builddir)/src/HLS/chaining/lib_hls_chaining.la \
           $(top_builddir)/src/HLS/evaluation/lib_evaluation.la \
           $(top_builddir)/src/HLS/interface/lib_interface.la 

if BUILD_LIB_EXPERIMENTAL
   SUBDIRS += export
   DIST_SUBDIRS += export

   lib_HLS_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/backend/ToDataFile \
      -I$(top_srcdir)/src/HLS/export  \
      -I$(top_srcdir)/src/HLS/hls_flow/design_space_exploration \
      -I$(top_srcdir)/src/HLS/interface/AXI \
      -I$(top_srcdir)/src/HLS/interface/FSL \
      -I$(top_srcdir)/src/HLS/interface/NPI

   lib_HLS_la_LIBADD += \
           $(top_builddir)/src/HLS/export/lib_hls_export.la

endif
if BUILD_LIB_VCD
   lib_HLS_la_CPPFLAGS += \
      -I$(top_srcdir)/src/HLS/vcd
endif


if BUILD_BEAGLE
  lib_HLS_la_CPPFLAGS += -I$(BEAGLE_DIR)/beagle/include/  \
                         -I$(top_builddir)/ext/beagle/beagle/include/ \
                         -I$(BEAGLE_DIR)/beagle/GA/include/ \
                         -I$(BEAGLE_DIR)/PACC
endif

if BUILD_LIB_ILP
  lib_HLS_la_CPPFLAGS += \
                      -I$(top_srcdir)/src/ilp
endif

if BUILD_LIB_VCD
   SUBDIRS += vcd
   DIST_SUBDIRS += vcd
   lib_HLS_la_LIBADD += \
                 $(top_builddir)/src/HLS/vcd/lib_vcd.la
endif

if BUILD_TASTE
   lib_HLS_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations/aadl_asn
endif

hls_constraints.cpp : constraints_STD.data
#The tab in this rule can't be replaced by blanks
constraints_STD.data: $(top_srcdir)/etc/lib/technology/constraints_STD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/constraints_STD.xml constraints_STD.data

#Documentation stuff
DOX_TAGFILES = src src/behavior src/circuit src/technology src/HLS/scheduling src/HLS/module_allocation src/HLS/binding src/HLS/binding/register src/HLS/binding/module src/HLS/binding/interconnection src/HLS/controller src/HLS/datapath src/HLS/hls_flow src/HLS/evaluation src/HLS/target /src/HLS/chaining src/HLS/function_allocation 

PRJ_DOC = HLS.doc HLS_constraints.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

