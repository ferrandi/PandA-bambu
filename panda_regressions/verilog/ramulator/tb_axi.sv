// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module tb_axi #(parameter DEPTH = 4);
  import "DPI-C" function int unsigned m_next(input int unsigned state);
  reg clock = 0;
  always #5 clock = !clock;
  reg reset = 1;
  wire ramulator_phase;
  reg arvalid = 0, awvalid = 0, wvalid = 0, rready = 0, bready = 0;
  wire arready, awready, wready, rvalid, bvalid, rlast;
  reg [63:0] araddr = 0, awaddr = 0;
  reg [7:0] arid = 0, awid = 0, arlen = 0, awlen = 0;
  reg [2:0] arsize = 2, awsize = 2;
  reg [1:0] arburst = 1, awburst = 1;
  reg [31:0] wdata = 0;
  reg [3:0] wstrb = 0;
  reg wlast = 0;
  wire [7:0] rid, bid;
  wire [31:0] rdata;
  wire [1:0] rresp, bresp;
  integer cycle = 0, read_count = 0, write_count = 0;
  integer expected_beat [0:255];
  integer expected_length [0:255];
  integer expected_address [0:255];
  reg expected_fixed [0:255];
  reg read_pending [0:255], write_pending [0:255];
  reg [31:0] golden [0:255];

  TestbenchMEMAXIRamulatorIntegrated #(.index(0), .QUEUE_SIZE(DEPTH+1), .AXI_PORT_ID(0),
    .BITSIZE_araddr(64), .BITSIZE_awaddr(64), .BITSIZE_arid(8), .BITSIZE_awid(8),
    .BITSIZE_rid(8), .BITSIZE_bid(8), .BITSIZE_arlen(8), .BITSIZE_awlen(8),
    .BITSIZE_arsize(3), .BITSIZE_awsize(3), .BITSIZE_arburst(2), .BITSIZE_awburst(2),
    .BITSIZE_rdata(32), .BITSIZE_wdata(32), .BITSIZE_wstrb(4), .BITSIZE_rresp(2), .BITSIZE_bresp(2)) memory(
    .clock(clock), .ramulator_phase(ramulator_phase), .reset(reset), .arvalid(arvalid), .arready(arready), .araddr(araddr),
    .arid(arid), .arlen(arlen), .arsize(arsize), .arburst(arburst),
    .arlock(1'b0), .arcache(1'b0), .arprot(1'b0), .arqos(1'b0), .arregion(1'b0), .aruser(1'b0),
    .rvalid(rvalid), .rready(rready), .rid(rid), .rdata(rdata), .rresp(rresp), .rlast(rlast), .ruser(),
    .awvalid(awvalid), .awready(awready), .awaddr(awaddr), .awid(awid), .awlen(awlen),
    .awsize(awsize), .awburst(awburst), .awlock(1'b0), .awcache(1'b0), .awprot(1'b0),
    .awqos(1'b0), .awregion(1'b0), .awuser(1'b0),
    .wvalid(wvalid), .wready(wready), .wdata(wdata), .wstrb(wstrb), .wlast(wlast), .wuser(1'b0),
    .bvalid(bvalid), .bready(bready), .bid(bid), .bresp(bresp), .buser());

  ramulator_test_clock tick_source(.clock(clock), .reset(reset), .ramulator_phase(ramulator_phase));
  always @(negedge clock)
  begin
    if(!reset)
    begin
      rready = cycle > 1000 && cycle % 11 > 4;
      bready = cycle > 250 && cycle % 7 > 2;
    end
  end

  reg held_r = 0, held_b = 0;
  reg [50:0] held_read;
  reg [10:0] held_write;
  always @(posedge clock)
  begin : scoreboard
    integer word_index;
    if(!reset)
    begin
      cycle = cycle + 1;
      if(held_r && {rvalid, rlast, rid, rresp, rdata} !== held_read) $fatal(1, "unstable AXI R during stall");
      if(held_b && {bvalid, bid, bresp} !== held_write) $fatal(1, "unstable AXI B during stall");
      held_r = rvalid && !rready;
      held_b = bvalid && !bready;
      held_read = {rvalid, rlast, rid, rresp, rdata};
      held_write = {bvalid, bid, bresp};
      if(rvalid && rready)
      begin
        if(!read_pending[rid]) $fatal(1, "unknown AXI RID %0d", rid);
        word_index = expected_address[rid] + (expected_fixed[rid] ? 0 : expected_beat[rid]);
        if(rdata !== golden[word_index] || rresp != 0) $fatal(1, "AXI data id=%0d beat=%0d got=%h expected=%h", rid, expected_beat[rid], rdata, golden[word_index]);
        if(rlast != (expected_beat[rid] == expected_length[rid] - 1)) $fatal(1, "AXI RLAST");
        expected_beat[rid] = expected_beat[rid] + 1;
        if(rlast) begin read_pending[rid] = 0; read_count = read_count + 1; end
      end
      if(bvalid && bready)
      begin
        if(!write_pending[bid] || bresp != 0) $fatal(1, "unknown/duplicate AXI BID %0d", bid);
        write_pending[bid] = 0;
        write_count = write_count + 1;
      end
    end
  end

  task automatic send_aw(input integer id, input integer word_index, input integer beats, input bit fixed_address);
    @(negedge clock); #1;
    awvalid = 1; awid = id; awaddr = 64'h40060000 + word_index*4; awlen = beats - 1; awburst = fixed_address ? 0 : 1;
    write_pending[id] = 1;
    do begin @(posedge clock); end while(!awready);
    @(negedge clock); #1; awvalid = 0;
  endtask

  task automatic send_w(input integer word_index, input integer beats, input bit fixed_address, input reg [3:0] mask);
    integer beat, lane, word;
    for(beat = 0; beat < beats; beat = beat + 1)
    begin
      @(negedge clock); #1;
      wvalid = 1; wdata = 32'habcd0000 + beat; wstrb = mask; wlast = beat == beats-1;
      do begin @(posedge clock); end while(!wready);
      word = word_index + (fixed_address ? 0 : beat);
      for(lane = 0; lane < 4; lane = lane + 1)
        if(mask[lane]) golden[word][lane*8+:8] = wdata[lane*8+:8];
      @(negedge clock); #1; wvalid = 0;
    end
  endtask

  task automatic send_ar(input integer id, input integer word_index, input integer beats, input bit fixed_address);
    @(negedge clock); #1;
    arvalid = 1; arid = id; araddr = 64'h40060000 + word_index*4; arlen = beats - 1; arburst = fixed_address ? 0 : 1;
    read_pending[id] = 1; expected_beat[id] = 0; expected_length[id] = beats;
    expected_address[id] = word_index; expected_fixed[id] = fixed_address;
    do begin @(posedge clock); end while(!arready);
    @(negedge clock); #1; arvalid = 0;
  endtask

  initial
  begin : run
    integer i;
    if((m_next(1) & 255) != 2) $fatal(1, "MDPI setup");
    for(i = 0; i < 256; i = i + 1)
    begin
      golden[i] = (32'h60000/4+i) ^ 32'h13579bdf;
      read_pending[i] = 0; write_pending[i] = 0;
    end
    repeat(3) @(negedge clock);
    #1;
    reset = 0;
    #1;
    reset = 1;
    repeat(2) @(negedge clock);
    #1;
    reset = 0;
    // Data before address, with a long independent AW stall.
    fork
      begin repeat(20) @(negedge clock); send_aw(1, 0, 16, 0); end
      send_w(0, 16, 0, 4'b1111);
    join
    // Address before data, sparse strobes, and a fixed-address burst.
    send_aw(2, 32, 16, 0);
    repeat(10) @(negedge clock);
    send_w(32, 16, 0, 4'b0101);
    send_aw(3, 64, 4, 1); send_w(64, 4, 1, 4'b1111);
    send_aw(4, 80, 1, 0); send_w(80, 1, 0, 4'b0000);
    send_aw(5, 0, 256, 0); send_w(0, 256, 0, 4'b1111);
    wait(write_count == 5);
    // Two 16-beat bursts occupy two transaction slots, not 32 slots.
    send_ar(11, 0, 16, 0);
    send_ar(12, 32, 16, 0);
    send_ar(13, 64, 4, 1);
    send_ar(14, 80, 1, 0);
    send_ar(15, 96, 16, 0);
    send_ar(16, 0, 256, 0);
    wait(read_count == 6);
    repeat(5) @(negedge clock);
    $display("PASS real Ramulator AXI bursts, sparse strobes, independent AW/W, held R/B");
    $finish;
  end
  initial begin #1000000; $fatal(1, "AXI timeout reads=%0d writes=%0d", read_count, write_count); end
endmodule
