proc dump_statistics { report_file } {
   set util_rpt [report_utilization -return_string]
   set ram_rpt [report_ram_utilization -return_string]
   set CLBLUTs 0
   set CLBRegisters 0
   set CLBs 0
   set BRAMFIFO36 0
   set BRAMFIFO18 0
   set BRAMFIFO36_star 0
   set BRAMFIFO18_star 0
   set BRAM18 0
   set BRAMFIFO 0
   set DRAM 0
   set URAM 0
   set BIOB 0
   set DSPs 0
   set TotPower 0
   set design_slack 0
   set design_req 0
   set design_delay 0
   regexp --  {\s*LUT as Logic\s*\|\s*([^[:blank:]]+)} $util_rpt ignore CLBLUTs
   regexp --  {\s*CLB Registers\s*\|\s*([^[:blank:]]+)} $util_rpt ignore CLBRegisters
   regexp --  {\s*CLB\s*\|\s*([^[:blank:]]+)} $util_rpt ignore CLBs
   regexp --  {\s*RAMB36/FIFO36\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO36
   regexp --  {\s*RAMB18/FIFO18\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO18
   regexp --  {\s*RAMB36/FIFO\*\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO36_star
   regexp --  {\s*RAMB18/FIFO\*\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAMFIFO18_star
   regexp --  {\s*RAMB18\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BRAM18
   set BRAMFIFO [expr {(2 *$BRAMFIFO36) + $BRAMFIFO18 + (2*$BRAMFIFO36_star) + $BRAMFIFO18_star + $BRAM18}]
   regexp --  {\s*LUT as Memory\s*\|\s*([^[:blank:]]+)} $util_rpt ignore DRAM
   regexp --  {\s*URAM\s*\|\s*([^[:blank:]]+)} $ram_rpt ignore URAM
   regexp --  {\s*Bonded IOB\s*\|\s*([^[:blank:]]+)} $util_rpt ignore BIOB
   regexp --  {\s*DSPs\s*\|\s*([^[:blank:]]+)} $util_rpt ignore DSPs
   set power_rpt [report_power -return_string]
   regexp --  {\s*Total On-Chip Power \(W\)\s*\|\s*([^[:blank:]]+)} $power_rpt ignore TotPower
   set Timing_Paths [get_timing_paths -max_paths 1 -nworst 1 -setup]
   if { [expr {$Timing_Paths == ""}] } {
      set design_slack 0
      set design_req 0
   } else {
      set design_slack [get_property SLACK $Timing_Paths]
      set design_req [get_property REQUIREMENT  $Timing_Paths]
   }
   if { [expr {$design_slack == ""}] } { set design_slack 0 }
   if { [expr {$design_req == ""}] } { set design_req 0 }
   set design_delay [expr {$design_req - $design_slack}]
   set frequency [expr {1000.0 / $design_delay}]
   file delete -force $report_file
   set report_fd [open $report_file w]
   puts $report_fd "<?xml version=\"1.0\"?>"
   puts $report_fd "<application>"
   puts $report_fd "  <resources"
   puts $report_fd "    SLICES=\"$CLBs\""
   puts $report_fd "    REGISTERS=\"$CLBRegisters\""
   puts $report_fd "    LUTS=\"$CLBLUTs\""
   puts $report_fd "    BRAMS=\"$BRAMFIFO\""
   puts $report_fd "    DRAMS=\"$DRAM\""
   puts $report_fd "    URAMS=\"$URAM\""
   puts $report_fd "    IOPINS=\"$BIOB\""
   puts $report_fd "    DSPS=\"$DSPs\""
   puts $report_fd "    POWER=\"$TotPower\""
   puts $report_fd "    FREQUENCY=\"$frequency\""
   puts $report_fd "    SLACK=\"$design_slack\""
   puts $report_fd "    DELAY=\"$design_delay\" />"
   puts $report_fd "</application>"
   close $report_fd
}
