create_project -force axi_vip

set top_fname $::env(TOP_FNAME)
set vip_name ${top_fname}_VIP
# Add top-level design and bambu simulation design
add_file -norecurse $::env(OUT_DIR)/${top_fname}.v
add_file -fileset sim_1 -norecurse $::env(OUT_DIR)/HLS_output/simulation/bambu_testbench.v
set_property file_type SystemVerilog [get_files $::env(OUT_DIR)/HLS_output/simulation/bambu_testbench.v]
# Create Vivado IP Integrator design
create_bd_design $vip_name
update_compile_order -fileset sources_1
create_bd_cell -type module -reference $top_fname top
# Add AXI Verification IP
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_vip:1.1 axi_vip_0
# Connect IP AXI4 slave port to bambu top-level instance AXI4 master port
connect_bd_intf_net [get_bd_intf_pins top/m_axi_gmem0] [get_bd_intf_pins axi_vip_0/S_AXI]
# Set AXI VIP AXI4 master port as external port
make_bd_intf_pins_external  [get_bd_intf_pins axi_vip_0/M_AXI]
set_property name m_axi_gmem0 [get_bd_intf_ports M_AXI_0]
# Set AXI4 interface address mapping and address space range
assign_bd_address -target_address_space /top/m_axi_gmem0 [get_bd_addr_segs m_axi_gmem0/Reg] -force
set_property offset 0x0000000040000000 [get_bd_addr_segs {top/m_axi_gmem0/SEG_m_axi_gmem0_Reg}]
set_property range 1M [get_bd_addr_segs {top/m_axi_gmem0/SEG_m_axi_gmem0_Reg}]
# Connect clock and reset ports to external port
create_bd_port -dir I -type clk -freq_hz 100000000 clock
connect_bd_net [get_bd_ports clock] [get_bd_pins top/clock] [get_bd_pins axi_vip_0/aclk]
create_bd_port -dir I -type rst reset
connect_bd_net [get_bd_ports reset] [get_bd_pins top/reset] [get_bd_pins axi_vip_0/aresetn]
# Propagate top-level interface ports as external ports
make_bd_pins_external [get_bd_cells top]
foreach port [get_bd_ports ] { 
   set old_name [get_property name $port]
   if {[string match *_0 $old_name]} { 
      set new_name [string range $old_name 0 [expr {[string length $old_name] - 3}]]
      set_property name $new_name $port 
   }
}
save_bd_design
close_bd_design [get_bd_designs $vip_name]
# Set compilation and simulation options
set_property top clocked_bambu_testbench [get_filesets sim_1]
set_property -name {xsim.compile.xvlog.relax} -value {false} -objects [get_filesets sim_1]
set_property -name {xsim.compile.xvhdl.relax} -value {false} -objects [get_filesets sim_1]
set_property -name {xsim.compile.xvlog.more_options} -value {-define M64} -objects [get_filesets sim_1]
set_property -name {xsim.simulate.runtime} -value {400000000ns} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.snapshot} -value {axi_vip_tb} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.load_glbl} -value {false} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.debug_level} -value {off} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.relax} -value {false} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.mt_level} -value {off} -objects [get_filesets sim_1]
set_property -name {xsim.elaborate.xelab.more_options} -value "-sv_root $::env(OUT_DIR)/HLS_output/xsim_beh -sv_lib libmdpi -define M64" -objects [get_filesets sim_1]
file delete $::env(OUT_DIR)/HLS_output/simulation/panda_ipc_mmap
set ::env(M_IPC_SIM_CMD) "bash -c \"exit 0\""
set __testbench_pid [exec bash -c "$::env(OUT_DIR)/HLS_output/simulation/testbench |& tee $::env(OUT_DIR)/HLS_output/simulation/testbench.log" &]
after 5000 set stop_wait &
vwait stop_wait
unset stop_wait
launch_simulation
set finished [catch {exec ps -p ${__testbench_pid} >/dev/null}]
close_sim
exit
