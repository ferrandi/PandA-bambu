cd [file dirname [info script]]
source ../../utils/xmlq_wrapper.tcl

set bambu_results [xmldump $env(BAMBU_HLS_RESULTS)]

set target_family [dict get $bambu_results /application/target@family]
if { [string first "Zynq" $target_family] || [string first "Virtex" $target_family] || [string first "Artix" $target_family] } {
   source ./stats.v1.tcl
} else {
   source ./stats.v2.tcl
}

set report_file [dict get $bambu_results /application/backend@bambu_results]
set target_device "[dict get $bambu_results /application/target@model][dict get $bambu_results /application/target@package][dict get $bambu_results /application/target@speed_grade]"

set_param general.maxThreads [dict get $bambu_results /application/backend@parallel]

create_project [dict get $bambu_results /application/top_module@name] -part $target_device -force

# Add sources
set vhdl_library ""
if { [dict exists $bambu_results /application/vhdl_library@sources] } { set vhdl_library "-library [dict get $bambu_results /application/vhdl_library@sources]" }
set sources [dict get $bambu_results /application/outputs/file]
foreach file $sources {
   set ext [string tolower [file extension $file]]
   set file "$env(BAMBU_HLS_OUTDIR)/$file"
   if {$ext eq ".vhd" || $ext eq ".vhdl"} {
      read_vhdl {*}$vhdl_library $file
   } elseif {$ext eq ".v"} {
      read_verilog $file
   } elseif {$ext eq ".sv"} {
      read_verilog -sv $file
   } else {
      puts "Skipping unsupported file: $file"
   }
}

# Read SDC files
set sdc_ext_file [dict get $bambu_results /application/backend@sdc_ext_file]
if { "$sdc_ext_file" ne "{}" } { read_xdc $sdc_ext_file }
set sdc_file "constraints.sdc"
file delete -force $sdc_file
set sdc_fd [open $sdc_file w]
if { [dict get $bambu_results /application/top_module@combinational] } {
   puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_outputs\]"
} else {
   puts $sdc_fd "create_clock -period [dict get $bambu_results /application/target@period] -name [dict get $bambu_results /application/top_module@clock_name] \[get_ports [dict get $bambu_results /application/top_module@clock_name]\]"
   if { [dict get $bambu_results /application/target@connect_iob] } {
      puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_outputs\]"
      puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_registers\]"
      puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_registers\] -to \[all_outputs\]"
   }
}
puts $sdc_fd "set_property HD.CLK_SRC BUFGCTRL_X0Y0 \[get_ports clock\]"
close $sdc_fd

# Perform synthesis
set synth_flags "-top [dict get $bambu_results /application/top_module@name] -part $target_device"
if { ! [dict get $bambu_results /application/target@connect_iob] } {
   set synth_flags "$synth_flags -mode out_of_context -no_iobuf"
}

read_xdc $sdc_file
synth_design {*}$synth_flags
write_checkpoint -force post_synth.dcp
report_timing_summary -file post_synth_timing_summary.rpt
report_utilization -file post_synth_util.rpt
report_utilization -hierarchical -file post_synth_util_hier.rpt
dump_statistics $report_file

# Optimize design
opt_design
report_utilization -file post_opt_design_util.rpt
report_utilization -hierarchical -file post_opt_design_util_hier.rpt
dump_statistics $report_file

# Place design
place_design -directive Explore
report_clock_utilization -file clock_util.rpt
# Optionally run optimization if there are timing violations after placement
if {[get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]] < 0.5} {
   puts "Found setup timing violations => running physical optimization"
   phys_opt_design
}
write_checkpoint -force post_place.dcp
report_utilization -file post_place_util.rpt
report_utilization -hierarchical -file post_place_util_hier.rpt
report_timing_summary -file post_place_timing_summary.rpt
dump_statistics $report_file

# Route design
route_design -directive Explore
write_checkpoint -force post_route.dcp
report_route_status -file post_route_status.rpt
report_timing_summary -file post_route_timing_summary.rpt
report_power -file post_route_power.rpt
report_drc -file post_imp_drc.rpt
report_utilization -file post_route_util.rpt
report_utilization -hierarchical -file post_route_util_hier.rpt
dump_statistics $report_file

close_design
close_project
