proc dump_statistics { report_file } {
   set util_rpt [report_utilization -return_string]
   set SliceRegisters 0
   set Slice 0
   set SliceLUTs 0
   set BRAMFIFO36 0
   set BRAMFIFO18 0
   set BRAMFIFO36_star 0
   set BRAMFIFO18_star 0
   set BRAM18 0
   set BRAMFIFO 0
   set DRAM 0
   set BIOB 0
   set DSPs 0
   set TotPower 0
   set design_datapath_delay 0
   set mpc 0
   set design_slack 0
   set design_req 0
   set design_delay 0
   regexp --  {\s*Slice Registers\s*\|\s*([^[:blank:]]+)} $util_rpt ignore SliceRegisters
   regexp --  {\s*Slice\s*\|\s*([^[:blank:]]+)} $util_rpt ignore Slice
   regexp --  {\s*LUT as Logic\s*\|\s*([^[:blank:]]+)} $util_rpt ignore SliceLUTs
   regexp --  {\s*RAMB36/FIFO36\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO36
   regexp --  {\s*RAMB18/FIFO18\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO18
   regexp --  {\s*RAMB36/FIFO\*\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO36_star
   regexp --  {\s*RAMB18/FIFO\*\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO18_star
   regexp --  {\s*RAMB18\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAM18
   set BRAMFIFO [expr {(2 *$BRAMFIFO36) + $BRAMFIFO18 + (2*$BRAMFIFO36_star) + $BRAMFIFO18_star + $BRAM18}]
   regexp --  {\s*LUT as Memory\s*\|\s*([^[:blank:]]+)} $util_rpt ignore DRAM
   regexp --  {\s*Bonded IOB\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BIOB
   regexp --  {\s*DSPs\s*\|\s*([^[:blank:]]+)} $util_rpt ignore DSPs
   set power_rpt [report_power -return_string]
   regexp --  {\s*Total On-Chip Power \(W\)\s*\|\s*([^[:blank:]]+)} $power_rpt ignore TotPower
   set Timing_Paths [get_timing_paths -max_paths 1 -nworst 1 -setup]
   if { [expr {$Timing_Paths == ""}] } {
      set design_datapath_delay 0
      set design_slack 0
      set design_req 0
      set mpc 0
   } else {
      set design_datapath_delay [get_property DATAPATH_DELAY $Timing_Paths]
      set design_slack [get_property SLACK $Timing_Paths]
      set design_req [get_property REQUIREMENT  $Timing_Paths]
      puts $design_datapath_delay
      set min_period_check [report_pulse_width -min_period -return_string]
      regexp --  {Min Period\s*([^[:blank:]]+)\s*([^[:blank:]]+)\s*([^[:blank:]]+)\s*([^[:blank:]]+)} $min_period_check ignore ignore ignore ignore mpc
      # puts "MPC: $mpc"
   }
   if { [expr {$design_datapath_delay == ""}] } { set design_datapath_delay 0 }
   if { [expr {$design_slack == ""}] } { set design_slack 0 }
   if { [expr {$design_req == ""}] } { set design_req 0 }
   if { [expr {$mpc == ""}] } { set mpc 0 }
   set design_delay [expr {$design_datapath_delay}]
   # puts "Computed delay: $design_delay"
   if { [expr {$DSPs != 0}] } {
      if { [expr {$mpc > $design_delay}] } {
         set design_delay [expr {$mpc}]
      }
      if {[expr {$design_req - $design_slack}] > $design_delay } {
         set design_delay [expr {$design_req - $design_slack}]
      }
   }
   # puts "Corrected delay: $design_delay"
   set frequency [expr {1000.0 / $design_delay}]
   file delete -force $report_file
   set report_fd [open $report_file w]
   puts $report_fd "<?xml version=\"1.0\"?>"
   puts $report_fd "<application>"
   puts $report_fd "  <resources"
   puts $report_fd "    SLICES=\"$Slice\""
   puts $report_fd "    REGISTERS=\"$SliceRegisters\""
   puts $report_fd "    LUTS=\"$SliceLUTs\""
   puts $report_fd "    BRAMS=\"$BRAMFIFO\""
   puts $report_fd "    DRAMS=\"$DRAM\""
   puts $report_fd "    IOPINS=\"$BIOB\""
   puts $report_fd "    DSPS=\"$DSPs\""
   puts $report_fd "    POWER=\"$TotPower\""
   puts $report_fd "    FREQUENCY=\"$frequency\""
   puts $report_fd "    SLACK=\"$design_slack\""
   puts $report_fd "    DELAY=\"$design_delay\" />"
   puts $report_fd "</application>"
   close $report_fd
}
