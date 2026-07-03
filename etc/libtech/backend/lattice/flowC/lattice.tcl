proc dump_statistics_map { report_file top_name target_period } {
   set LSLICES 0
   set LUTs 0
   set has_LUTs 0
   set Registers 0
   set BRAMFIFO 0
   set BIOB 0
   set DSPs 0
   set design_delay 0
   set lse_design_delay 0
   set map_design_delay 0
   set map_file [open ./impl/top_impl.mrp r]
   set map_log [read $map_file]
   regexp --  {\s*Number of SLICEs:\s*([0-9]+)} $map_log ignore LSLICES
   set has_LUTs [regexp --  {\s*Number of LUT4s:\s*([0-9]+)} $map_log ignore LUTs]
   regexp --  {\s*Number of Used DSP MULT Sites:\s*([0-9]+)} $map_log ignore DSPs
   regexp --  {\s*Number of registers:\s*([0-9]+)} $map_log ignore Registers
   regexp --  {\s*Number of block RAMs:\s*([0-9]+)} $map_log ignore BRAMFIFO
   puts $LSLICES
   regexp --  {\s*Number of PIO sites used:\s*([[0-9]+)} $map_log ignore BIOB
   file delete -force $report_file
   set twr_file [open ./impl/${top_name}_lse.twr r]
   set twr_log [read $twr_file]
   regexp --  {\s*Report:\s*([[0-9.]+)} $twr_log ignore lse_design_delay
   if { [expr {$lse_design_delay == 0}] } {
      regexp --  {\s*Warning:\s*([[0-9.]+)} $twr_log ignore lse_design_delay
   }
   set map_twr_file [open ./impl/top_impl.tw1 r]
   set map_twr_log [read $map_twr_file]
   regexp --  {\s*Report:\s*([[0-9.]+)} $map_twr_log ignore map_design_delay
   if { [expr {$map_design_delay == 0}] } {
      regexp -- {\s*Warning:\s*([[0-9.]+MHz)} $map_twr_log ignore map_design_delay
      if { [expr {$map_design_delay == 0}] } {
         regexp --  {\s*Warning:\s*([[0-9.]+)} $map_twr_log ignore map_design_delay
      } else {
         regexp -- {\s*Warning:\s*([[0-9.]+)} $map_twr_log ignore map_design_delay
         set map_design_delay [expr {1000 / $map_design_delay}]
      }
   } else {
      set design_delay 0
      regexp -- {\s*Report:\s*([[0-9.]+MHz)} $map_twr_log ignore map_design_delay
      if { [expr {$map_design_delay == 0}] } {
         regexp --  {\s*Report:\s*([[0-9.]+)} $map_twr_log ignore map_design_delay
      } else {
         regexp -- {\s*Report:\s*([[0-9.]+)} $map_twr_log ignore map_design_delay
         set map_design_delay [expr {1000 / $map_design_delay}]
      }
   }
   if { [expr {$lse_design_delay < $map_design_delay && $lse_design_delay != 0}] } {
      set design_delay [expr {$lse_design_delay}]
   } else {
      set design_delay [expr {$map_design_delay}]
   }
   set frequency [expr {$design_delay != 0 ? 1000.0 / $design_delay : 0}]
   set clock_slack [expr {$target_period - $design_delay}]
   set report_fd [open $report_file w]
   puts $report_fd "<?xml version=\"1.0\"?>"
   puts $report_fd "<application>"
   puts $report_fd "  <resources"
   puts $report_fd "    AREA=\"$LSLICES\""
   puts $report_fd "    SLICES=\"$LSLICES\""
   if {$has_LUTs} {
      puts $report_fd "    LUTS=\"$LUTs\""
   }
   puts $report_fd "    REGISTERS=\"$Registers\""
   puts $report_fd "    BRAMS=\"$BRAMFIFO\""
   puts $report_fd "    DRAMS=\"0\""
   puts $report_fd "    IOPINS=\"$BIOB\""
   puts $report_fd "    DSPS=\"$DSPs\""
   puts $report_fd "    CLOCK_SLACK=\"$clock_slack\""
   puts $report_fd "    FREQUENCY=\"$frequency\""
   puts $report_fd "    PERIOD=\"$design_delay\""
   puts $report_fd "    DELAY=\"$design_delay\" />"
   puts $report_fd "</application>"
   close $report_fd
}

set SWD [file dirname [info script]]
source $SWD/../../utils/xmlq_wrapper.tcl

set bambu_results [xmldump $env(BAMBU_HLS_RESULTS)]

set frequency [expr {1000.0 / [dict get $bambu_results /application/target@period]}]
set target_period [dict get $bambu_results /application/target@period]
set report_file $SWD/[dict get $bambu_results /application/backend@bambu_results]
set sdc_ext_file $SWD/[dict get $bambu_results /application/backend@sdc_ext_file]

set sdc_file "$SWD/constraints.ldc"
file delete -force $sdc_file
set sdc_fd [open $sdc_file w]
if { [dict get $bambu_results /application/top_module@combinational] } {
   puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_outputs\]"
} else {
   puts $sdc_fd "create_clock -period [dict get $bambu_results /application/target@period] -name [dict get $bambu_results /application/top_module@clock_name] \[get_ports [dict get $bambu_results /application/top_module@clock_name]\]"
   if { [dict get $bambu_results /application/target@connect_iob] && "$sdc_ext_file" eq "{}" } {
      puts $sdc_fd "set_max_delay [dict get $bambu_results /application/target@period] -from \[all_inputs\] -to \[all_outputs\]"
   }
}
if { "$sdc_ext_file" ne "{}" } {
   puts $sdc_fd "source $sdc_ext_file"
}
close $sdc_fd

prj_project new -name top -impl impl -dev [dict get $bambu_results /application/target@model]
prj_strgy set_value -strategy Strategy1 lse_frequency=$frequency
prj_strgy set_value -strategy Strategy1 "lse_cmdline_args=-top [dict get $bambu_results /application/top_module@name]"
prj_strgy set_value -strategy Strategy1 map_overmap_device=True
prj_strgy set_value -strategy Strategy1 map_timing_driven=True
prj_strgy set_value -strategy Strategy1 map_timing_driven_node_replication=True
prj_strgy set_value -strategy Strategy1 map_timing_driven_pack=True
prj_strgy set_value -strategy Strategy1 map_reg_retiming=True
prj_src add $sdc_file

# Add sources
set vhdl_library ""
if { [dict exists $bambu_results /application/vhdl_library@sources] } { set vhdl_library "-work [dict get $bambu_results /application/vhdl_library@sources]" }
set sources [dict get $bambu_results /application/outputs/file]
set pmi_def "$env(FOUNDRY)/../cae_library/synthesis/verilog/pmi_def.v"
if {[file exists $pmi_def]} {
   prj_src add -format VERILOG $pmi_def
}
foreach file $sources {
   set ext [string tolower [file extension $file]]
   set file "$env(BAMBU_HLS_OUTDIR)/$file"
   if {$ext eq ".vhd" || $ext eq ".vhdl"} {
      prj_src add -format VHDL {*}$vhdl_library $file
   } elseif {$ext eq ".v" || $ext eq ".sv"} {
      prj_src add -format VERILOG $file
   } else {
      puts "Skipping unsupported file: $file"
   }
}

prj_syn set lse
prj_run Synthesis -impl impl -forceOne
prj_run Map -impl impl -forceOne
prj_run Map -impl impl -task MapTrace -forceOne
dump_statistics_map $report_file [dict get $bambu_results /application/top_module@name] $target_period
prj_project close
