// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module tb_arbiter #(parameter DEPTH = 2);
import "DPI-C" function int unsigned m_next(input int unsigned state);
reg clock = 0;
always #5 clock = !clock;
reg reset = 1, setup_port = 0, done_port = 0;
reg hold_responses = 1;
reg [1:0] oe = 0;
reg [127:0] addr = 0;
reg [5:0] request_tag = 0;
reg [1:0] oe_next = 0;
reg [127:0] addr_next = 0;
reg [5:0] request_tag_next = 0;
always @(negedge clock)
begin
 oe <= oe_next;
 addr <= addr_next;
 request_tag <= request_tag_next;
end
wire [1:0] memory_ready, request_bp, response_bp;
wire [1:0] inactive_response_bp;
wire [127:0] memory_data;
wire [5:0] memory_tag;
wire [3:0] ready, tag;
wire [255:0] data;
reg pending [0:7];
reg [63:0] expected [0:7];
integer completed = 0, collisions = 0, parallel_responses = 0;
TestbenchMEMRamulatorIntegrated #(.index(0), .QUEUE_SIZE(DEPTH),
 .BITSIZE_Mout_oe_ram(2), .BITSIZE_Mout_we_ram(2), .BITSIZE_Mout_addr_ram(128),
 .BITSIZE_Mout_Wdata_ram(128), .BITSIZE_Mout_data_ram_size(14), .BITSIZE_Mout_tag(6),
 .BITSIZE_M_tag(6), .BITSIZE_M_DataRdy(2), .BITSIZE_M_Rdata_ram(128),
 .BITSIZE_M_back_pressure(2), .BITSIZE_Mout_back_pressure(2)) memory(
 .clock(clock), .reset(reset), .setup_port(setup_port), .done_port(done_port),
 .Mout_oe_ram(oe), .Mout_we_ram(2'b0), .Mout_addr_ram(addr), .Mout_Wdata_ram(128'b0),
 .Mout_data_ram_size({7'd64,7'd64}), .Mout_tag(request_tag), .Mout_back_pressure(request_bp),
 .M_DataRdy(memory_ready), .M_Rdata_ram(memory_data), .M_tag(memory_tag),
 .M_back_pressure(hold_responses ? 2'b11 : response_bp | inactive_response_bp));
Arbiter_connection_interface #(.ARBITER_TYPE(1),
    .INNER_TAG(1),
    .SELECTOR_SIZE(2),
    .IS_PIPELINED(1),
    .BITSIZE_Mout_oe_ram(1),
    .PORTSIZE_Mout_oe_ram(4),
    .BITSIZE_Mout_we_ram(1),
    .PORTSIZE_Mout_we_ram(4),
    .BITSIZE_Mout_addr_ram(64),
    .PORTSIZE_Mout_addr_ram(4),
    .BITSIZE_Mout_Wdata_ram(64),
    .PORTSIZE_Mout_Wdata_ram(4),
    .BITSIZE_Mout_data_ram_size(7),
    .PORTSIZE_Mout_data_ram_size(4),
    .BITSIZE_Mout_tag(1),
    .PORTSIZE_Mout_tag(4),
    .BITSIZE_M_DataRdy(1),
    .PORTSIZE_M_DataRdy(2),
    .BITSIZE_M_Rdata_ram(64),
    .PORTSIZE_M_Rdata_ram(2),
    .BITSIZE_M_tag(3),
    .PORTSIZE_M_tag(2),
    .BITSIZE_Mout_oe_ram_out(1),
    .PORTSIZE_Mout_oe_ram_out(2),
    .BITSIZE_Mout_we_ram_out(1),
    .PORTSIZE_Mout_we_ram_out(2),
    .BITSIZE_Mout_addr_ram_out(64),
    .PORTSIZE_Mout_addr_ram_out(2),
    .BITSIZE_Mout_Wdata_ram_out(64),
    .PORTSIZE_Mout_Wdata_ram_out(2),
    .BITSIZE_Mout_data_ram_size_out(7),
    .PORTSIZE_Mout_data_ram_size_out(2),
    .BITSIZE_Mout_tag_out(3),
    .PORTSIZE_Mout_tag_out(2),
    .BITSIZE_M_DataRdy_out(1),
    .PORTSIZE_M_DataRdy_out(4),
    .BITSIZE_M_Rdata_ram_out(64),
    .PORTSIZE_M_Rdata_ram_out(4),
    .BITSIZE_M_tag_out(1),
    .PORTSIZE_M_tag_out(4),
    .BITSIZE_M_back_pressure(1),
    .PORTSIZE_M_back_pressure(2),
    .BITSIZE_Mout_back_pressure(1),
    .PORTSIZE_Mout_back_pressure(4),
    .BITSIZE_Mout_back_pressure_out(1),
    .PORTSIZE_Mout_back_pressure_out(2)) dut(
 .clock(clock),.reset(reset),
 .Mout_oe_ram(4'b0),.Mout_we_ram(4'b0),.Mout_addr_ram(256'b0),.Mout_Wdata_ram(256'b0),
 .Mout_data_ram_size(28'b0),.Mout_tag(4'b0),.Mout_back_pressure_out(2'b0),
 .M_DataRdy(memory_ready & {2{!hold_responses}}),.M_tag(memory_tag),.M_Rdata_ram(memory_data),
 .M_back_pressure(response_bp), .M_DataRdy_out(ready),.M_tag_out(tag),.M_Rdata_ram_out(data));
// Inactive fork-call arbiters share the bus response inputs and merge their
// backpressure outputs. Their stale selectors must not stall another fork.
Arbiter_connection_interface #(.ARBITER_TYPE(1), .INNER_TAG(0), .IS_PIPELINED(0),
 .SELECTOR_SIZE(2), .PORTSIZE_Mout_oe_ram(4), .PORTSIZE_Mout_we_ram(4),
 .PORTSIZE_Mout_addr_ram(4), .PORTSIZE_Mout_Wdata_ram(4), .PORTSIZE_Mout_data_ram_size(4),
 .PORTSIZE_M_DataRdy(2), .BITSIZE_M_Rdata_ram(64), .PORTSIZE_M_Rdata_ram(2),
 .BITSIZE_M_tag(3), .PORTSIZE_M_tag(2), .PORTSIZE_Mout_oe_ram_out(2),
 .PORTSIZE_Mout_we_ram_out(2), .PORTSIZE_Mout_addr_ram_out(2),
 .PORTSIZE_Mout_Wdata_ram_out(2), .PORTSIZE_Mout_data_ram_size_out(2),
 .PORTSIZE_M_DataRdy_out(4), .BITSIZE_M_Rdata_ram_out(64), .PORTSIZE_M_Rdata_ram_out(4),
 .PORTSIZE_M_back_pressure(2), .PORTSIZE_Mout_back_pressure(4),
 .PORTSIZE_Mout_back_pressure_out(2)) inactive_dut(
 .clock(clock), .reset(reset), .Mout_oe_ram(4'b0), .Mout_we_ram(4'b0),
 .Mout_addr_ram(4'b0), .Mout_Wdata_ram(4'b0), .Mout_data_ram_size(4'b0),
 .Mout_tag(1'b0), .Mout_back_pressure_out(2'b0),
 .M_DataRdy(memory_ready & {2{!hold_responses}}), .M_tag(memory_tag), .M_Rdata_ram(memory_data),
 .M_back_pressure(inactive_response_bp));
always @(posedge clock)
begin : check
 integer port_index, t, delivered;
 if(!reset && !hold_responses)
 begin
  if(inactive_response_bp !== 0) $fatal(1,"Inactive arbiter stalled another fork's response");
  delivered = 0;
  if(response_bp != 0)
  begin
   if(response_bp !== (collisions%2==0 ? 2'b10 : 2'b01)) $fatal(1,"Round-robin fairness");
   collisions = collisions + 1;
  end
  for(port_index=0; port_index<4; port_index=port_index+1)
  begin
   if(ready[port_index])
   begin
    t = port_index*2 + tag[port_index];
    if(!pending[t]) $fatal(1,"Unexpected/duplicate response tag=%0d",t);
    if(data[port_index*64+:64] !== expected[t]) $fatal(1,"Corrupted response tag=%0d",t);
    pending[t] = 0;
    completed = completed + 1;
    delivered = delivered + 1;
   end
  end
  if(delivered==2) parallel_responses = parallel_responses + 1;
 end
end
initial
begin : run
 integer round_index, channel, t, index;
 if((m_next(1) & 255) != 2) $fatal(1,"MDPI setup");
 for(t=0;t<8;t=t+1) pending[t]=0;
 repeat(3) @(negedge clock);
 reset=0; setup_port=1;
 repeat(2) @(negedge clock);
 setup_port=0;
 #1;
 for(round_index=0;round_index<8;round_index=round_index+1)
 begin
  hold_responses=1;
  for(channel=0;channel<2;channel=channel+1)
  begin
   t = channel==0 ? 6 : (round_index%2==0 ? 7 : 3);
   request_tag_next[channel*3+:3] = t;
   index = round_index*32 + channel*16;
   addr_next[channel*64+:64] = 64'h40000000 + index*4;
   expected[t] = {((index+1) ^ 32'h13579bdf), (index ^ 32'h13579bdf)};
   pending[t]=1;
  end
  oe_next=2'b11;
  @(negedge clock);
  @(posedge clock);
  if(request_bp!=0) $fatal(1,"Unexpected request stall");
  oe_next=0;
  @(negedge clock);
  wait(memory_ready==2'b11);
  @(negedge clock); hold_responses=0;
  wait(completed==(round_index+1)*2);
  @(negedge clock);
  #1;
 end
 if(collisions!=4 || parallel_responses!=4) $fatal(1,"Missing collision/parallel coverage");
 done_port=1;
 $display("PASS real Ramulator response arbitration: collisions=%0d parallel=%0d",collisions,parallel_responses);
 $finish;
end
initial begin #100000; $fatal(1,"Response arbiter timeout"); end
endmodule
