# Usage: vivado -mode batch -source vector_mask_qor.tcl -tclargs RTL_DIR REPORT_DIR
# Measure both netlists with the same part, clock and synthesis options.
set_param general.maxThreads 2
set rtl_dir [lindex $argv 0]
set report_dir [lindex $argv 1]
file mkdir $report_dir
read_verilog [list $rtl_dir/eq16.v $rtl_dir/panda_libtech.v]
synth_design -top eq16 -part xc7z020clg484-1 -flatten_hierarchy rebuilt
create_clock -name clock -period 10 [get_ports clock]
report_utilization -file $report_dir/utilization.rpt
report_utilization -hierarchical -file $report_dir/hierarchy.rpt
report_timing_summary -file $report_dir/timing.rpt
set path [get_timing_paths -delay_type max -max_paths 1]
if {[llength $path] == 0} {
  error "No timing path found"
}
set output [open $report_dir/metrics.txt w]
puts $output "clock_period_ns=10"
puts $output "worst_slack_ns=[get_property SLACK $path]"
puts $output "data_path_delay_ns=[get_property DATAPATH_DELAY $path]"
close $output
exit
