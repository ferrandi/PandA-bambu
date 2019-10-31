#create the bitstream
open_checkpoint VGA_Nexys4_synth/HLS_output/Synthesis/vivado_flow/post_route.dcp
write_bitstream -force final_bistream.bit
#program the NEXYS4
open_hw
connect_hw_server -url localhost:3121
current_hw_target [lindex [get_hw_targets] 0]
open_hw_target
  current_hw_device [lindex [get_hw_devices] 0]
  set_property PROGRAM.FILE  {./final_bistream.bit}  [lindex [get_hw_devices] 0]
  program_hw_devices [lindex [get_hw_devices] 0]
close_hw_target
close_hw
