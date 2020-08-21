#this file is src/HLS/Makefile.am
METASOURCES = AUTO
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)
noinst_LTLIBRARIES =
noinst_HEADERS =
PRJ_DOC =

noinst_LTLIBRARIES +=  lib_controller_creation.la
lib_controller_creation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += \
   architecture_creation/controller_creation/controller_creator_base_step.hpp \
   architecture_creation/controller_creation/control_flow_checker.hpp \
   architecture_creation/controller_creation/controller_cs.hpp \
   architecture_creation/controller_creation/fsm_controller.hpp \
   architecture_creation/controller_creation/pipeline_controller.hpp
lib_controller_creation_la_SOURCES = \
   architecture_creation/controller_creation/controller_creator_base_step.cpp \
   architecture_creation/controller_creation/control_flow_checker.cpp \
   architecture_creation/controller_creation/controller_cs.cpp \
   architecture_creation/controller_creation/fsm_controller.cpp \
   architecture_creation/controller_creation/pipeline_controller.cpp

#DOCUMENTATION STUFF
PRJ_DOC += architecture_creation/controller_creation/controller.doc architecture_creation/controller_creation/fsm.doc

noinst_LTLIBRARIES += lib_datapath_creation.la
lib_datapath_creation_la_CPPFLAGS =  \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/HLS/function_allocation \
   $(AM_CPPFLAGS)

noinst_HEADERS += architecture_creation/datapath_creation/datapath_creator.hpp architecture_creation/datapath_creation/classic_datapath.hpp architecture_creation/datapath_creation/datapath_cs.hpp architecture_creation/datapath_creation/datapath_parallel_cs.hpp
lib_datapath_creation_la_SOURCES = architecture_creation/datapath_creation/datapath_creator.cpp architecture_creation/datapath_creation/classic_datapath.cpp architecture_creation/datapath_creation/datapath_cs.cpp architecture_creation/datapath_creation/datapath_parallel_cs.cpp

PRJ_DOC += architecture_creation/datapath_creation/datapath.doc

noinst_LTLIBRARIES += lib_architecture_creation.la
noinst_HEADERS += architecture_creation/top_entity.hpp architecture_creation/TopEntityMemoryMapped.hpp architecture_creation/top_entity_cs.hpp architecture_creation/top_entity_parallel_cs.hpp
lib_architecture_creation_la_SOURCES = architecture_creation/top_entity.cpp architecture_creation/TopEntityMemoryMapped.cpp architecture_creation/top_entity_cs.cpp architecture_creation/top_entity_parallel_cs.cpp
lib_architecture_creation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows\
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

lib_architecture_creation_la_LIBADD = lib_datapath_creation.la lib_controller_creation.la

noinst_LTLIBRARIES += lib_hls_backend.la
lib_hls_backend_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/wrapper/simulation \
   $(AM_CPPFLAGS)

noinst_HEADERS += backend/generate_hdl.hpp
lib_hls_backend_la_SOURCES = backend/generate_hdl.cpp
if BUILD_SIMULATION
  noinst_HEADERS += backend/generate_simulation_scripts.hpp
  lib_hls_backend_la_SOURCES += backend/generate_simulation_scripts.cpp
endif
if BUILD_SYNTHESIS
  noinst_HEADERS += backend/generate_synthesis_scripts.hpp
  lib_hls_backend_la_SOURCES += backend/generate_synthesis_scripts.cpp
endif
if BUILD_TASTE
  lib_hls_backend_la_CPPFLAGS += \
     -I$(top_srcdir)/src/intermediate_representations/aadl_asn
  noinst_HEADERS += \
     backend/generate_taste_hdl_architecture.hpp \
     backend/generate_taste_synthesis_script.hpp
  lib_hls_backend_la_SOURCES += \
     backend/generate_taste_hdl_architecture.cpp \
     backend/generate_taste_synthesis_script.cpp
endif

noinst_LTLIBRARIES += lib_binding.la

noinst_LTLIBRARIES += lib_register_algorithms.la
lib_register_algorithms_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
    $(AM_CPPFLAGS)
noinst_HEADERS += binding/register/algorithms/conflict_based_register.hpp binding/register/algorithms/vertex_coloring_register.hpp \
                 binding/register/algorithms/compatibility_based_register.hpp \
                 binding/register/algorithms/chordal_coloring_register.hpp binding/register/algorithms/network_flow.hpp binding/register/algorithms/unique_binding_register.hpp binding/register/algorithms/weighted_clique_register.hpp
lib_register_algorithms_la_SOURCES = binding/register/algorithms/conflict_based_register.cpp binding/register/algorithms/vertex_coloring_register.cpp \
                                     binding/register/algorithms/compatibility_based_register.cpp \
                                     binding/register/algorithms/chordal_coloring_register.cpp binding/register/algorithms/network_flow.cpp binding/register/algorithms/unique_binding_register.cpp binding/register/algorithms/weighted_clique_register.cpp


noinst_LTLIBRARIES += lib_registerbinding.la
lib_registerbinding_la_CPPFLAGS = \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/src/algorithms/clique_covering \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/design_flows\
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/HLS/virtual_components \
    -I$(top_srcdir)/src/HLS/binding \
    -I$(top_srcdir)/src/HLS/binding/interconnection \
    -I$(top_srcdir)/src/HLS/binding/module \
    -I$(top_srcdir)/src/HLS/binding/register/algorithms \
    -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
    -I$(top_srcdir)/src/HLS/liveness \
    -I$(top_srcdir)/src/HLS/stg \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/technology/target_device/FPGA \
    -I$(top_srcdir)/src/technology \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/HLS/function_allocation \
    $(AM_CPPFLAGS)
noinst_HEADERS += binding/register/reg_binding.hpp binding/register/reg_binding_creator.hpp binding/register/Variable.hpp binding/register/reg_binding_cs.hpp
lib_registerbinding_la_SOURCES = binding/register/reg_binding.cpp binding/register/reg_binding_creator.cpp binding/register/reg_binding_cs.cpp
lib_registerbinding_la_LIBADD = lib_register_algorithms.la
PRJ_DOC += binding/register/reg_binding_creator.doc

noinst_LTLIBRARIES += lib_interconnectionbinding.la
lib_interconnectionbinding_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/parser/treegcc\
   -I$(top_srcdir)/src/HLS/binding/interconnection/ \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += binding/interconnection/conn_binding_creator.hpp binding/interconnection/connection_obj.hpp \
                  binding/interconnection/conn_binding.hpp binding/interconnection/port_swapping.hpp binding/interconnection/mux_connection_binding.hpp\
                  binding/interconnection/direct_conn.hpp binding/interconnection/mux_conn.hpp binding/interconnection/conn_binding_cs.hpp \
                  binding/interconnection/parallel_memory_conn_binding.hpp
lib_interconnectionbinding_la_SOURCES = binding/interconnection/conn_binding_creator.cpp binding/interconnection/conn_binding.cpp binding/interconnection/port_swapping.cpp\
                                        binding/interconnection/mux_connection_binding.cpp binding/interconnection/conn_binding_cs.cpp binding/interconnection/parallel_memory_conn_binding.cpp

noinst_LTLIBRARIES += lib_modulebinding.la
lib_modulebinding_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants  \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/flopoco \
   $(AM_CPPFLAGS)
noinst_HEADERS += binding/module/fu_binding.hpp binding/module/fu_binding_creator.hpp binding/module/easy_module_binding.hpp binding/module/unique_binding.hpp \
                  binding/module/cdfc_module_binding.hpp binding/module/module_binding_check.hpp binding/module/fu_binding_cs.hpp binding/module/parallel_memory_fu_binding.hpp
lib_modulebinding_la_SOURCES = binding/module/fu_binding.cpp binding/module/fu_binding_creator.cpp binding/module/easy_module_binding.cpp binding/module/unique_binding.cpp \
                               binding/module/cdfc_module_binding.cpp binding/module/fu_binding_cs.cpp binding/module/parallel_memory_fu_binding.cpp
PRJ_DOC += binding/module/fu_binding_creator.doc

noinst_LTLIBRARIES += lib_storage_value_insertion.la
lib_storage_value_insertion_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += binding/storage_value_insertion/storage_value_information.hpp binding/storage_value_insertion/storage_value_information_fsm.hpp binding/storage_value_insertion/storage_value_information_pipeline.hpp binding/storage_value_insertion/storage_value_insertion.hpp binding/storage_value_insertion/values_scheme.hpp
lib_storage_value_insertion_la_SOURCES = binding/storage_value_insertion/storage_value_information.cpp binding/storage_value_insertion/storage_value_information_fsm.cpp binding/storage_value_insertion/storage_value_information_pipeline.cpp binding/storage_value_insertion/storage_value_insertion.cpp binding/storage_value_insertion/values_scheme.cpp

lib_binding_la_SOURCES =
lib_binding_la_LIBADD = lib_registerbinding.la lib_interconnectionbinding.la lib_storage_value_insertion.la lib_modulebinding.la

PRJ_DOC += binding/binding.doc

noinst_LTLIBRARIES += lib_hls_chaining.la
lib_hls_chaining_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

noinst_HEADERS +=  chaining/chaining.hpp chaining/sched_based_chaining_computation.hpp chaining/chaining_information.hpp
lib_hls_chaining_la_SOURCES = chaining/chaining.cpp chaining/sched_based_chaining_computation.cpp chaining/chaining_information.cpp

noinst_LTLIBRARIES += lib_exact_evaluation.la
lib_exact_evaluation_la_CPPFLAGS = \
   -I$(top_srcdir)/src  \
   -I$(top_srcdir)/src/algorithms/loops_detection  \
   -I$(top_srcdir)/src/design_flows/  \
   -I$(top_srcdir)/src/design_flows/backend  \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL  \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(top_srcdir)/src/wrapper/simulation/modelsim \
   -I$(top_srcdir)/src/wrapper/synthesis/ \
   -I$(top_srcdir)/src/wrapper/treegcc \
   $(AM_CPPFLAGS)
lib_exact_evaluation_la_SOURCES=
if BUILD_LIB_RTL
   lib_exact_evaluation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/rtl
endif
if BUILD_SIMULATION
   noinst_HEADERS += evaluation/exact/simulation_evaluation.hpp
   lib_exact_evaluation_la_SOURCES += evaluation/exact/simulation_evaluation.cpp
endif
if BUILD_LIB_RTL_CHARACTERIZATION
   noinst_HEADERS += evaluation/exact/synthesis_evaluation.hpp
   lib_exact_evaluation_la_SOURCES += evaluation/exact/synthesis_evaluation.cpp
endif

noinst_LTLIBRARIES += lib_evaluation.la
lib_evaluation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/evaluation/exact \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/algorithms/loops_detection  \
   -I$(top_srcdir)/src/design_flows/\
   -I$(top_srcdir)/src/design_flows/backend  \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/constants  \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/wrapper/synthesis/ \
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(top_srcdir)/src/wrapper/simulation/modelsim \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += evaluation/evaluation.hpp evaluation/evaluation_base_step.hpp evaluation/dry_run_evaluation.hpp
lib_evaluation_la_SOURCES = evaluation/evaluation.cpp evaluation/evaluation_base_step.cpp evaluation/dry_run_evaluation.cpp
lib_evaluation_la_LIBADD = lib_exact_evaluation.la
PRJ_DOC += evaluation/evaluation.doc

noinst_LTLIBRARIES += lib_function_allocation.la
lib_function_allocation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/technology \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility  \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology  \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   $(AM_CPPFLAGS)
noinst_HEADERS += function_allocation/function_allocation.hpp function_allocation/fun_dominator_allocation.hpp function_allocation/functions.hpp
lib_function_allocation_la_SOURCES = \
   function_allocation/function_allocation.cpp function_allocation/fun_dominator_allocation.cpp function_allocation/functions.cpp

if BUILD_LIB_FROM_PRAGMA
   lib_function_allocation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign 
   noinst_HEADERS += \
      function_allocation/omp_function_allocation_CS.hpp \
      function_allocation/omp_function_allocation.hpp \
      function_allocation/omp_functions.hpp

   lib_function_allocation_la_SOURCES += \
      function_allocation/omp_function_allocation_CS.cpp \
      function_allocation/omp_function_allocation.cpp \
      function_allocation/omp_functions.cpp
endif

noinst_LTLIBRARIES += lib_hls_synthesis.la
lib_hls_synthesis_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows/ \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/hls_flow \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += hls_flow/synthesis/standard_hls.hpp hls_flow/synthesis/virtual_hls.hpp hls_flow/synthesis/hls_synthesis_flow.hpp
lib_hls_synthesis_la_SOURCES = hls_flow/synthesis/standard_hls.cpp hls_flow/synthesis/virtual_hls.cpp hls_flow/synthesis/hls_synthesis_flow.cpp
if BUILD_LIB_FROM_PRAGMA
   noinst_HEADERS += hls_flow/synthesis/omp_for_wrapper_cs_synthesis_flow.hpp hls_flow/synthesis/omp_body_loop_synthesis_flow.hpp
   lib_hls_synthesis_la_SOURCES += hls_flow/synthesis/omp_for_wrapper_cs_synthesis_flow.cpp hls_flow/synthesis/omp_body_loop_synthesis_flow.cpp
endif

noinst_LTLIBRARIES += lib_hls_flow.la
lib_hls_flow_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching\
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/design_flows/backend/ToC\
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/backend\
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/hls_flow/synthesis \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
   $(AM_CPPFLAGS)
noinst_HEADERS += \
   hls_flow/initialize_hls.hpp \
   hls_flow/classical_synthesis_flow.hpp \
   hls_flow/write_hls_summary.hpp
lib_hls_flow_la_SOURCES = \
   hls_flow/initialize_hls.cpp \
   hls_flow/classical_synthesis_flow.cpp \
   hls_flow/write_hls_summary.cpp
lib_hls_flow_la_LIBADD = lib_hls_synthesis.la
if BUILD_LIB_VCD
   lib_hls_flow_la_CPPFLAGS += \
      -I$(top_srcdir)/src/HLS/vcd \
      -I$(top_srcdir)/src/HLS/virtual_components \
      -I$(top_srcdir)/src/parser/vcd
endif
if BUILD_TRNG
   lib_hls_flow_la_CPPFLAGS += \
      -I$(top_builddir)/ext/trng-4.17
endif

noinst_LTLIBRARIES += lib_interface.la
lib_interface_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
    -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/HLS/binding \
    -I$(top_srcdir)/src/HLS/binding/interconnection \
    -I$(top_srcdir)/src/HLS/binding/module \
    -I$(top_srcdir)/src/HLS/binding/register \
    -I$(top_srcdir)/src/HLS/function_allocation \
    -I$(top_srcdir)/src/HLS/interface \
    -I$(top_srcdir)/src/HLS/interface/WB4 \
    -I$(top_srcdir)/src/HLS/interface/minimal \
    -I$(top_srcdir)/src/HLS/memory \
    -I$(top_srcdir)/src/HLS/module_allocation \
    -I$(top_srcdir)/src/HLS/scheduling \
    -I$(top_srcdir)/src/HLS/simulation \
    -I$(top_srcdir)/src/HLS/stg \
    -I$(top_srcdir)/src/HLS/virtual_components \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/design_flows \
    -I$(top_srcdir)/src/design_flows/backend/ToHDL \
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    $(AM_CPPFLAGS)
noinst_HEADERS += interface/module_interface.hpp \
                 interface/minimal/minimal_interface.hpp \
                 interface/WB4/WB4_interface.hpp \
                 interface/WB4/WB4Intercon_interface.hpp \
                 interface/cs_interface.hpp
lib_interface_la_SOURCES = interface/module_interface.cpp \
                           interface/minimal/minimal_interface.cpp \
                           interface/WB4/WB4_interface.cpp \
                           interface/WB4/WB4Intercon_interface.cpp \
                           interface/cs_interface.cpp
if BUILD_TASTE
noinst_HEADERS += interface/taste_interface_generation.hpp
lib_interface_la_SOURCES += interface/taste_interface_generation.cpp
lib_interface_la_CPPFLAGS += \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations/aadl_asn \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device
endif

noinst_LTLIBRARIES += lib_liveness.la
lib_liveness_la_CPPFLAGS =  \
                          -I$(top_srcdir)/src \
                          -I$(top_srcdir)/src/algorithms/loops_detection \
                          -I$(top_srcdir)/src/behavior \
                          -I$(top_srcdir)/src/design_flows \
                          -I$(top_srcdir)/src/graph \
                          -I$(top_srcdir)/src/HLS \
                          -I$(top_srcdir)/src/HLS/chaining \
                          -I$(top_srcdir)/src/HLS/memory \
                          -I$(top_srcdir)/src/HLS/scheduling \
                          -I$(top_srcdir)/src/HLS/stg \
                          -I$(top_srcdir)/src/polixml \
                          -I$(top_srcdir)/src/tree \
                          -I$(top_srcdir)/src/technology \
                          -I$(top_srcdir)/src/utility \
                          $(AM_CPPFLAGS)
noinst_HEADERS += liveness/liveness.hpp liveness/liveness_computer.hpp liveness/FSM_NI_SSA_liveness.hpp
lib_liveness_la_SOURCES = liveness/liveness.cpp liveness/liveness_computer.cpp liveness/FSM_NI_SSA_liveness.cpp

noinst_LTLIBRARIES += lib_HLS_memory.la
lib_HLS_memory_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/technology  \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device  \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += \
   memory/memory_allocation.hpp \
   memory/memory_symbol.hpp \
   memory/memory.hpp \
   memory/mem_dominator_allocation.hpp \
   memory/mem_dominator_allocation_cs.hpp \
   memory/memory_cs.hpp
lib_HLS_memory_la_SOURCES = \
   memory/memory_allocation.cpp \
   memory/memory.cpp \
   memory/mem_dominator_allocation.cpp \
   memory/mem_dominator_allocation_cs.cpp

noinst_LTLIBRARIES += lib_allocation.la
lib_allocation_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL  \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_generator \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device/ \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility  \
   -I$(top_srcdir)/src/wrapper/flopoco \
   $(AM_CPPFLAGS)

noinst_HEADERS += \
   module_allocation/add_library.hpp \
   module_allocation/allocation.hpp \
   module_allocation/allocation_information.hpp \
   module_allocation/hls_bit_value.hpp \
   module_allocation/hls_function_bit_value.hpp \
   module_allocation/omp_allocation.hpp

lib_allocation_la_SOURCES = \
   module_allocation/add_library.cpp \
   module_allocation/allocation.cpp \
   module_allocation/allocation_information.cpp \
   module_allocation/hls_bit_value.cpp \
   module_allocation/hls_function_bit_value.cpp \
   module_allocation/omp_allocation.cpp

#DOCUMENTATION STUFF
PRJ_DOC += module_allocation/main_allocation.doc module_allocation/allocation.doc module_allocation/binding_constraints.doc

noinst_LTLIBRARIES += lib_module_generator.la
lib_module_generator_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/graph_helpers \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows  \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels  \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers  \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL  \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(top_srcdir)/src/wrapper/treegcc \
   $(AM_CPPFLAGS)
noinst_HEADERS += module_generator/moduleGenerator.hpp
lib_module_generator_la_SOURCES = module_generator/moduleGenerator.cpp

noinst_LTLIBRARIES += lib_scheduling.la
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
   -I$(top_srcdir)/src/HLS/scheduling \
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
   $(AM_CPPFLAGS)
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
noinst_HEADERS += \
   scheduling/ASLAP.hpp \
   scheduling/fixed_scheduling.hpp \
   scheduling/parametric_list_based.hpp \
   scheduling/priority.hpp \
   scheduling/rehashed_heap.hpp \
   scheduling/schedule.hpp \
   scheduling/scheduling.hpp
lib_scheduling_la_SOURCES = \
   scheduling/ASLAP.cpp \
   scheduling/fixed_scheduling.cpp \
   scheduling/parametric_list_based.cpp \
   scheduling/priority.cpp \
   scheduling/schedule.cpp \
   scheduling/scheduling.cpp
if BUILD_LIB_ILP
   noinst_HEADERS += \
      scheduling/sdc_scheduling.hpp
   lib_scheduling_la_SOURCES += \
      scheduling/sdc_scheduling.cpp
   lib_scheduling_la_CPPFLAGS += \
      -I$(top_srcdir)/src/algorithms/loops_detection \
      -I$(top_srcdir)/src/frontend_analysis \
      -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
      -I$(top_srcdir)/src/ilp
endif
PRJ_DOC += scheduling/scheduling.doc

noinst_LTLIBRARIES += lib_stg.la
lib_stg_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/graph_helpers \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)
noinst_HEADERS += stg/state_transition_graph.hpp stg/state_transition_graph_manager.hpp stg/StateTransitionGraph_constructor.hpp stg/STG_creator.hpp stg/BB_based_stg.hpp
lib_stg_la_SOURCES = stg/state_transition_graph.cpp stg/state_transition_graph_manager.cpp stg/StateTransitionGraph_constructor.cpp stg/STG_creator.cpp stg/BB_based_stg.cpp

noinst_LTLIBRARIES += lib_virtual_components.la
lib_virtual_components_la_CPPFLAGS =  -I$(top_srcdir)/src \
                             -I$(top_srcdir)/src/HLS \
                             -I$(top_srcdir)/src/behavior \
                             -I$(top_srcdir)/src/graph \
                             -I$(top_srcdir)/src/circuit \
                             -I$(top_srcdir)/src/utility \
                             $(AM_CPPFLAGS)
noinst_HEADERS += virtual_components/generic_obj.hpp \
                 virtual_components/funit_obj.hpp \
                 virtual_components/register_obj.hpp \
                 virtual_components/mux_obj.hpp \
                 virtual_components/adder_conn_obj.hpp \
                 virtual_components/multiplier_conn_obj.hpp \
                 virtual_components/bitfield_obj.hpp \
                 virtual_components/commandport_obj.hpp \
                 virtual_components/dataport_obj.hpp \
                 virtual_components/conv_conn_obj.hpp \
                 virtual_components/multi_unbounded_obj.hpp
lib_virtual_components_la_SOURCES = \
   virtual_components/generic_obj.cpp \
   virtual_components/mux_obj.cpp

DIST_SUBDIRS = simulation
SUBDIRS = simulation

noinst_LTLIBRARIES += lib_HLS.la

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
   $(AM_CPPFLAGS)

noinst_HEADERS += \
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
           lib_architecture_creation.la lib_hls_backend.la lib_binding.la \
           lib_hls_flow.la lib_scheduling.la lib_allocation.la lib_function_allocation.la \
           lib_stg.la lib_liveness.la lib_HLS_memory.la \
           $(top_builddir)/src/HLS/simulation/lib_simulation.la \
           lib_module_generator.la lib_virtual_components.la lib_hls_chaining.la lib_evaluation.la lib_interface.la 

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
noinst_LTLIBRARIES += lib_vcd.la
lib_vcd_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants  \
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
   $(AM_CPPFLAGS)
noinst_HEADERS += \
   vcd/CallGraphUnfolding.hpp \
   vcd/CallSitesCollectorVisitor.hpp \
   vcd/Discrepancy.hpp \
   vcd/HWPathComputation.hpp \
   vcd/HWDiscrepancyAnalysis.hpp \
   vcd/UnfoldedFunctionInfo.hpp \
   vcd/UnfoldedCallInfo.hpp \
   vcd/UnfoldedCallGraph.hpp \
   vcd/VcdSignalSelection.hpp \
   vcd/vcd_trace_head.hpp \
   vcd/vcd_utility.hpp
lib_vcd_la_SOURCES = \
   vcd/CallGraphUnfolding.cpp \
   vcd/CallSitesCollectorVisitor.cpp \
   vcd/HWPathComputation.cpp \
   vcd/HWDiscrepancyAnalysis.cpp \
   vcd/vcd_trace_head.cpp \
   vcd/VcdSignalSelection.cpp \
   vcd/vcd_utility.cpp
   lib_HLS_la_LIBADD += lib_vcd.la
endif

if BUILD_TASTE
   lib_HLS_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations/aadl_asn
endif

hls_constraints.cpp : constraints_STD.data constraints_STD_ALUs.data
#The tab in this rule can't be replaced by blanks
constraints_STD.data: $(top_srcdir)/etc/lib/technology/constraints_STD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/constraints_STD.xml constraints_STD.data
constraints_STD_ALUs.data: $(top_srcdir)/etc/lib/technology/constraints_STD_ALUs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/constraints_STD_ALUs.xml constraints_STD_ALUs.data

#Documentation stuff
DOX_TAGFILES = src src/graph src/HLS src/behavior src/circuit src/technology src/parser/HLS_constraintsIO src/HLS/controller src/HLS/scheduling src/HLS/module_allocation src/HLS/binding src/HLS/binding/register src/HLS/binding/storage_value_insertion src/HLS/binding/module src/HLS/binding/interconnection src/HLS/controller src/HLS/datapath src/HLS/hls_flow src/HLS/evaluation src/HLS/target /src/HLS/chaining src/HLS/function_allocation 

PRJ_DOC += HLS.doc HLS_constraints.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

