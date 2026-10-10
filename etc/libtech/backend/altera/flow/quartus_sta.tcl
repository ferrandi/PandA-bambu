cd [file dirname [info script]]
source ../../utils/xmlq_wrapper.tcl

set bambu_results [xmldump $env(BAMBU_HLS_RESULTS)]

set report_file [dict get $bambu_results /application/backend@bambu_results]
set sdc_file "constraints.sdc"

load_package report
project_open [dict get $bambu_results /application/top_module@name]
load_report
create_timing_netlist
read_sdc $sdc_file
update_timing_netlist

set le_usage [get_fitter_resource_usage -le -used]
set lab_usage [get_fitter_resource_usage -lab -used]
set reg_usage [get_fitter_resource_usage -reg -used]
set mem_usage [get_fitter_resource_usage -mem_bit -used]
set iopin_usage [get_fitter_resource_usage -io_pin -used]
set alut_usage [get_fitter_resource_usage -alut -used]
set dsp_usage_string [get_fitter_resource_usage -resource "Total DSP*"]
set dsp_usage_array [split $dsp_usage_string /]
set dsp_usage [string trim [lindex $dsp_usage_array 0]]
set alm_usage [get_fitter_resource_usage -alm -used]
set slack [lindex [report_timing] 1]
set delay [expr {[dict get $bambu_results /application/target@period] - $slack}]
set frequency [expr {1000.0 / $delay}]
file delete -force $report_file
set report_fd [open $report_file w]
puts $report_fd "<?xml version=\"1.0\"?>"
puts $report_fd "<application>"
puts $report_fd "  <resources"
puts $report_fd "    LE=\"$le_usage\""
puts $report_fd "    LAB=\"$lab_usage\""
puts $report_fd "    AREA=\"$alm_usage\""
puts $report_fd "    REGISTERS=\"$reg_usage\""
puts $report_fd "    MEM=\"$mem_usage\""
puts $report_fd "    BRAMS=\"0\""
puts $report_fd "    DRAMS=\"0\""
puts $report_fd "    IOPINS=\"$iopin_usage\""
puts $report_fd "    ALUT=\"$alut_usage\""
puts $report_fd "    DSPS=\"$dsp_usage\""
puts $report_fd "    ALM=\"$alm_usage\""
puts $report_fd "    CLOCK_SLACK=\"$slack\""
puts $report_fd "    FREQUENCY=\"$frequency\""
puts $report_fd "    SLACK=\"$slack\""
puts $report_fd "    PERIOD=\"$delay\""
puts $report_fd "    DELAY=\"$delay\" />"
puts $report_fd "</application>"
close $report_fd

unload_report
delete_timing_netlist
project_close
