#This file is src/frontend_analysis/IR_analysis/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_IR_analysis.la

lib_IR_analysis_la_CPPFLAGS = \
   -I$(top_srcdir)/src\
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/algorithms/graph_helpers \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/frontend_analysis/behavior_analysis \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml\
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   basic_blocks_cfg_computation.hpp \
   block_fix.hpp \
   call_expr_fix.hpp \
   call_graph_computation.hpp \
   check_system_type.hpp \
   operations_cfg_computation.hpp \
   string_cst_fix.hpp \
   switch_fix.hpp \
   use_counting.hpp \
   var_computation.hpp \
   var_decl_fix.hpp

lib_IR_analysis_la_SOURCES = \
   basic_blocks_cfg_computation.cpp \
   block_fix.cpp \
   call_expr_fix.cpp \
   call_graph_computation.cpp \
   check_system_type.cpp \
   operations_cfg_computation.cpp \
   switch_fix.cpp \
   string_cst_fix.cpp \
   use_counting.cpp \
   var_computation.cpp \
   var_decl_fix.cpp

BUILT_SOURCES =

if BUILD_BAMBU
  noinst_HEADERS += \
     bit_lattice.hpp \
     Bit_Value.hpp \
     Bit_Value_opt.hpp \
     BitValueIPA.hpp \
     call_graph_builtin_call.hpp \
     compute_implicit_calls.hpp \
     CSE.hpp \
     dead_code_elimination.hpp \
     determine_memory_accesses.hpp \
     extract_patterns.hpp \
     FixStructsPassedByValue.hpp \
     FunctionCallTypeCleanup.hpp \
     hdl_function_decl_fix.hpp \
     hdl_var_decl_fix.hpp \
     hls_div_cg_ext.hpp \
     HWCallInjection.hpp \
     ipa_point_to_analysis.hpp\
     IR_lowering.hpp \
     loops_analysis_bambu.hpp \
     lut_transformation.hpp \
     multi_way_if.hpp \
     multiple_entry_if_reduction.hpp \
     mem_cg_ext.hpp \
     NI_SSA_liveness.hpp \
     phi_opt.hpp \
     parm_decl_taken_address_fix.hpp \
     rebuild_initializations.hpp \
     remove_clobber_ga.hpp \
     remove_ending_if.hpp \
     split_return.hpp \
     short_circuit_taf.hpp \
     simple_code_motion.hpp \
     soft_float_cg_ext.hpp \
     vectorize.hpp \
     virtual_phi_nodes_split.hpp\
     constant_flop_wrapper.hpp
  lib_IR_analysis_la_SOURCES += \
     bit_lattice.cpp \
     Bit_Value.cpp \
     Bit_Value_backward.cpp \
     Bit_Value_forward.cpp \
     Bit_Value_opt.cpp \
     BitValueIPA.cpp \
     call_graph_builtin_call.cpp \
     compute_implicit_calls.cpp \
     CSE.cpp \
     dead_code_elimination.cpp \
     determine_memory_accesses.cpp \
     extract_patterns.cpp \
     FixStructsPassedByValue.cpp \
     FunctionCallTypeCleanup.cpp \
     hdl_function_decl_fix.cpp \
     hdl_var_decl_fix.cpp \
     hls_div_cg_ext.cpp \
     HWCallInjection.cpp \
     ipa_point_to_analysis.cpp\
     IR_lowering.cpp \
     IR_lowering_exec.cpp \
     loops_analysis_bambu.cpp \
     lut_transformation.cpp \
     multi_way_if.cpp \
     multiple_entry_if_reduction.cpp \
     mem_cg_ext.cpp \
     NI_SSA_liveness.cpp \
     phi_opt.cpp \
     parm_decl_taken_address_fix.cpp \
     rebuild_initializations.cpp \
     remove_clobber_ga.cpp \
     remove_ending_if.cpp \
     split_return.cpp \
     short_circuit_taf.cpp \
     simple_code_motion.cpp \
     soft_float_cg_ext.cpp \
     vectorize.cpp \
     virtual_phi_nodes_split.cpp \
     constant_flop_wrapper.cpp
   lib_IR_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/backend/ToHDL \
      -I$(top_srcdir)/src/design_flows/technology \
      -I$(top_srcdir)/src/HLS \
      -I$(top_srcdir)/src/HLS/scheduling \
      -I$(top_srcdir)/src/intermediate_representations \
      -I$(top_srcdir)/src/intermediate_representations/hls \
      -I$(top_srcdir)/src/technology \
      -I$(top_srcdir)/src/technology/physical_library/models

if BUILD_LIB_ILP
   noinst_HEADERS += \
      update_schedule.hpp
   lib_IR_analysis_la_SOURCES += \
      update_schedule.cpp
   lib_IR_analysis_la_CPPFLAGS += \
         -I$(top_srcdir)/src/HLS/module_allocation
endif

if BUILD_LIB_EXPERIMENTAL
      lib_IR_analysis_la_SOURCES += \
         simple_code_motion_pipelining.cpp
if BUILD_LIB_FROM_PRAGMA
      noinst_HEADERS += \
         check_critical_session.hpp \
         extract_omp_atomic.hpp \
         extract_omp_for.hpp
      lib_IR_analysis_la_SOURCES += \
         check_critical_session.cpp \
         extract_omp_atomic.cpp \
         extract_omp_for.cpp
endif
if BUILD_LIB_ILP
  lib_IR_analysis_la_CPPFLAGS += \
     -I$(top_srcdir)/src/technology \
     -I$(top_srcdir)/src/technology/physical_library/models
endif

endif
endif

if BUILD_LIB_CODE_ESTIMATION
   BUILT_SOURCES += tree_weights_symbolic.data tree_weights_constant.data
   noinst_HEADERS += instruction_sequences_computation.hpp update_tree_weight.hpp
   lib_IR_analysis_la_SOURCES += instruction_sequences_computation.cpp update_tree_weight.cpp
   lib_IR_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/Metrics/Static \
      -I$(top_srcdir)/src/machine \
      -I$(top_srcdir)/src/utility/probability_distribution
if BUILD_LIB_RTL
   BUILT_SOURCES += rtl_weights_constant.data rtl_weights_symbolic.data
   lib_IR_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/parser/rtlgcc \
      -I$(top_srcdir)/src/rtl
   noinst_HEADERS += update_rtl_weight.hpp
   lib_IR_analysis_la_SOURCES += update_rtl_weight.cpp
endif
endif

update_rtl_weights.cpp : rtl_weights_constant.data rtl_weights_symbolic.data

#The tab in this rule can't be replaced by blanks
rtl_weights_constant.data: $(top_srcdir)/etc/weight-models/rtl_weights_constant.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/weight-models/rtl_weights_constant.xml rtl_weights_constant.data

rtl_weights_symbolic.data: $(top_srcdir)/etc/weight-models/rtl_weights_symbolic.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/weight-models/rtl_weights_symbolic.xml rtl_weights_symbolic.data

if BUILD_LIB_FROM_PRAGMA
  noinst_HEADERS += \
       pragma_analysis.hpp
  lib_IR_analysis_la_SOURCES += \
       pragma_analysis.cpp
  lib_IR_analysis_la_CPPFLAGS += \
     -I$(top_srcdir)/src/parser/pragma \
     -I$(top_srcdir)/src/pragma
endif


if BUILD_ZEBU
   noinst_HEADERS += \
       array_ref_fix.hpp \
       call_args_structuring.hpp \
       dead_code_elimination.hpp \
       FunctionPointerCallGraphComputation.hpp \
       global_variables_analysis.hpp \
       header_structuring.hpp \
       loops_analysis_zebu.hpp \
       loop_regions_computation.hpp \
       pointed_data_computation.hpp \
       pointed_data_evaluation.hpp \
       predictability_analysis.hpp \
       reverse_restrict_computation.hpp \
       short_circuit_structuring.hpp \
       source_code_statistics.hpp \
       split_phi_nodes.hpp
   lib_IR_analysis_la_SOURCES += \
       array_ref_fix.cpp \
       call_args_structuring.cpp \
       dead_code_elimination.cpp \
       FunctionPointerCallGraphComputation.cpp \
       global_variables_analysis.cpp \
       header_structuring.cpp \
       loop_regions_computation.cpp \
       loops_analysis_zebu.cpp \
       pointed_data_computation.cpp \
       pointed_data_evaluation.cpp \
       predictability_analysis.cpp \
       reverse_restrict_computation.cpp \
       short_circuit_structuring.cpp \
       source_code_statistics.cpp \
       split_phi_nodes.cpp
   lib_IR_analysis_la_CPPFLAGS += \
     -I$(top_srcdir)/src/design_flows/backend/ToC \
     -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
     -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
     -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning

if BUILD_LIB_EXPERIMENTAL
      noinst_HEADERS += \
         parallel_loops_analysis.hpp \
         parallel_loop_swap.hpp \
         refined_var_computation.hpp
      lib_IR_analysis_la_SOURCES += \
         parallel_loops_analysis.cpp \
         parallel_loop_swap.cpp \
         refined_var_computation.cpp
if BUILD_LIB_HOST_PROFILING
      noinst_HEADERS += \
         dynamic_aggregate_data_flow_analysis.hpp \
         dynamic_var_computation.hpp
      lib_IR_analysis_la_SOURCES += \
         dynamic_aggregate_data_flow_analysis.cpp \
         dynamic_var_computation.cpp
endif
endif
if BUILD_MPPB
      noinst_HEADERS += global_variables_analysis.hpp
      lib_IR_analysis_la_SOURCES += global_variables_analysis.cpp
endif
endif

update_tree_weight.cpp : tree_weights_constant.data tree_weights_symbolic.data

#The tab in this rule can't be replaced by blanks
tree_weights_constant.data: $(top_srcdir)/etc/weight-models/tree_weights_constant.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/weight-models/tree_weights_constant.xml tree_weights_constant.data

tree_weights_symbolic.data: $(top_srcdir)/etc/weight-models/tree_weights_symbolic.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/weight-models/tree_weights_symbolic.xml tree_weights_symbolic.data

CLEANFILES = $(BUILT_SOURCES)

#do not touch the following line

