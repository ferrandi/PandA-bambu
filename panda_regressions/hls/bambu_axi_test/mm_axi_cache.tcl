create_project -force mm

add_file $::env(OUT_DIR)/mmult.v
add_file bambu_axi_test/mm_axi_tb.sv

create_bd_design "VIP"
update_compile_order -fileset sources_1
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_vip:1.1 axi_vip_0
endgroup
create_bd_cell -type module -reference mmult mmult_0
set_property -dict [list CONFIG.INTERFACE_MODE {SLAVE}] [get_bd_cells axi_vip_0]
connect_bd_intf_net [get_bd_intf_pins mmult_0/m_axi_gmem0] [get_bd_intf_pins axi_vip_0/S_AXI]
create_bd_port -dir I -type clk -freq_hz 20000000 clk
connect_bd_net [get_bd_ports clk] [get_bd_pins mmult_0/clock]
connect_bd_net [get_bd_ports clk] [get_bd_pins axi_vip_0/aclk]
startgroup
create_bd_port -dir I -type rst rst
endgroup
connect_bd_net [get_bd_ports rst] [get_bd_pins mmult_0/reset]
connect_bd_net [get_bd_ports rst] [get_bd_pins axi_vip_0/aresetn]
create_bd_port -dir I start
connect_bd_net [get_bd_ports start] [get_bd_pins mmult_0/start_port]
create_bd_port -dir I -from 31 -to 0 -type data a
connect_bd_net [get_bd_ports a] [get_bd_pins mmult_0/a]
create_bd_port -dir I -from 31 -to 0 -type data b
connect_bd_net [get_bd_ports b] [get_bd_pins mmult_0/b]
create_bd_port -dir I -from 31 -to 0 -type data c
connect_bd_net [get_bd_ports c] [get_bd_pins mmult_0/_output_]
create_bd_port -dir O done
connect_bd_net [get_bd_ports done] [get_bd_pins mmult_0/done_port]
save_bd_design
close_bd_design [get_bd_designs VIP]

set_property top mmult_tb [get_filesets sim_1]
launch_simulation
set simError [get_value -radix unsigned /mmult_tb/error]
if { $simError == 0 } { run all }
set simError [get_value -radix unsigned /mmult_tb/error]
exit $simError
