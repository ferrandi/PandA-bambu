cd [file dirname [info script]]
source ../../utils/xmlq_wrapper.tcl

set bambu_results [xmldump $env(BAMBU_HLS_RESULTS)]

set sdc_file "constraints.sdc"
file delete -force $sdc_file
set sdc_fd [open $sdc_file w]
if { [dict get $bambu_results /application/top_module@combinational] } {
   puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_outputs\]"
} else {
   puts $sdc_fd "create_clock -period [dict get $bambu_results /application/target@period] -name [dict get $bambu_results /application/top_module@clock_name] \[get_ports [dict get $bambu_results /application/top_module@clock_name]\]"
   puts $sdc_fd "derive_pll_clocks"
   puts $sdc_fd "derive_clock_uncertainty"
}
if { "$sdc_ext_file" ne "{}" } {
   puts $sdc_fd "source $sdc_ext_file"
}
close $sdc_fd

project_new [dict get $bambu_results /application/top_module@name] -overwrite

set OMP_NUM_THREADS [dict get $bambu_results /application/backend@parallel]
set_global_assignment -name NUM_PARALLEL_PROCESSORS $OMP_NUM_THREADS

set_global_assignment -name FAMILY [dict get $bambu_results /application/target@family]
set_global_assignment -name DEVICE [dict get $bambu_results /application/target@model]
set_global_assignment -name SDC_FILE $sdc_file
set_global_assignment -name TIMEQUEST_DO_REPORT_TIMING On
set_global_assignment -name ADV_NETLIST_OPT_SYNTH_WYSIWYG_REMAP ON
set_global_assignment -name AUTO_RAM_TO_LCELL_CONVERSION ON
set_global_assignment -name DSP_BLOCK_BALANCING Auto
set_global_assignment -name OPTIMIZATION_TECHNIQUE SPEED
set_global_assignment -name OPTIMIZE_MULTI_CORNER_TIMING ON
set_global_assignment -name PHYSICAL_SYNTHESIS_ASYNCHRONOUS_SIGNAL_PIPELINING ON
set_global_assignment -name PHYSICAL_SYNTHESIS_COMBO_LOGIC ON
set_global_assignment -name PHYSICAL_SYNTHESIS_COMBO_LOGIC_FOR_AREA ON
set_global_assignment -name PHYSICAL_SYNTHESIS_EFFORT EXTRA
set_global_assignment -name PHYSICAL_SYNTHESIS_MAP_LOGIC_TO_MEMORY_FOR_AREA ON
set_global_assignment -name PHYSICAL_SYNTHESIS_REGISTER_DUPLICATION ON
set_global_assignment -name PHYSICAL_SYNTHESIS_REGISTER_RETIMING ON
set_global_assignment -name PLACEMENT_EFFORT_MULTIPLIER 4
set_global_assignment -name PRE_MAPPING_RESYNTHESIS ON
set_global_assignment -name REMOVE_REDUNDANT_LOGIC_CELLS ON
set_global_assignment -name ROUTER_EFFORT_MULTIPLIER 4
set_global_assignment -name SYNCHRONIZATION_REGISTER_CHAIN_LENGTH 10
set_global_assignment -name SYNTH_TIMING_DRIVEN_SYNTHESIS ON
set_global_assignment -name ENABLE_DRC_SETTINGS ON
set_global_assignment -name OPTIMIZE_HOLD_TIMING "IO PATHS AND MINIMUM TPD PATHS"
set_global_assignment -name NUMBER_OF_PATHS_TO_REPORT 100000

set frequency [expr {1000.0 / [dict get $bambu_results /application/target@period]}]
create_base_clock -fmax $frequency -target [dict get $bambu_results /application/top_module@clock_name] [dict get $bambu_results /application/top_module@clock_name]

set vhdl_library ""
if { [dict exists $bambu_results /application/vhdl_library@sources] } { set vhdl_library "-library [dict get $bambu_results /application/vhdl_library@sources]" }
set sources [dict get $bambu_results /application/outputs/file]
foreach file $sources {
   set file "$env(BAMBU_HLS_OUTDIR)/$file"
   set_global_assignment -name SOURCE_FILE $src {*}$vhdl_library
}

project_close
