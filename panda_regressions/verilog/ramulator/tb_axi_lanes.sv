// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module tb_axi_lanes #(parameter DEPTH = 4);
  import "DPI-C" function int unsigned m_next(input int unsigned state);
  import "DPI-C" function void m_memory_tick
      (input longint unsigned cycle);
  reg clock = 0;
  always
  begin
    #5 clock = !clock;
  end
  reg reset = 1;
  reg arvalid = 0, awvalid = 0, wvalid = 0, rready = 0, bready = 0;
  wire arready, awready, wready, rvalid, bvalid, rlast;
  reg [63 : 0] araddr = 0, awaddr = 0;
  reg [7 : 0] arid = 0, awid = 0, arlen = 0, awlen = 0;
  reg [2 : 0] arsize = 2, awsize = 2;
  reg [1 : 0] arburst = 1, awburst = 1;
  reg [31 : 0] wdata = 0;
  reg [3 : 0] wstrb = 0;
  reg wlast = 0;
  wire [7 : 0] rid, bid;
  wire [31 : 0] rdata;
  wire [1 : 0] rresp, bresp;
  integer cycle = 0;

`ifdef CLASSIC_AXI
  TestbenchMEMAXI #(.READ_DELAY(2),
                    .WRITE_DELAY(2),
`else
  TestbenchMEMAXIRamulatorIntegrated #(.AXI_PORT_ID(0),
`endif
                    .index(0),
                    .QUEUE_SIZE(DEPTH + 1),
                    .BITSIZE_araddr(64),
                    .BITSIZE_awaddr(64),
                    .BITSIZE_arid(8),
                    .BITSIZE_awid(8),
                    .BITSIZE_rid(8),
                    .BITSIZE_bid(8),
                    .BITSIZE_arlen(8),
                    .BITSIZE_awlen(8),
                    .BITSIZE_arsize(3),
                    .BITSIZE_awsize(3),
                    .BITSIZE_arburst(2),
                    .BITSIZE_awburst(2),
                    .BITSIZE_rdata(32),
                    .BITSIZE_wdata(32),
                    .BITSIZE_wstrb(4),
                    .BITSIZE_rresp(2),
                    .BITSIZE_bresp(2))
      memory(.clock(clock),
             .reset(reset),
             .arvalid(arvalid),
             .arready(arready),
             .araddr(araddr),
             .arid(arid),
             .arlen(arlen),
             .arsize(arsize),
             .arburst(arburst),
             .arlock(1'b0),
             .arcache(1'b0),
             .arprot(1'b0),
             .arqos(1'b0),
             .arregion(1'b0),
             .aruser(1'b0),
             .rvalid(rvalid),
             .rready(rready),
             .rid(rid),
             .rdata(rdata),
             .rresp(rresp),
             .rlast(rlast),
             .ruser(),
             .awvalid(awvalid),
             .awready(awready),
             .awaddr(awaddr),
             .awid(awid),
             .awlen(awlen),
             .awsize(awsize),
             .awburst(awburst),
             .awlock(1'b0),
             .awcache(1'b0),
             .awprot(1'b0),
             .awqos(1'b0),
             .awregion(1'b0),
             .awuser(1'b0),
             .wvalid(wvalid),
             .wready(wready),
             .wdata(wdata),
             .wstrb(wstrb),
             .wlast(wlast),
             .wuser(1'b0),
             .bvalid(bvalid),
             .bready(bready),
             .bid(bid),
             .bresp(bresp),
             .buser());

  reg [7 : 0] golden_bytes[0 : 1023];
  reg held_r = 0, held_b = 0;
  reg [44 : 0] held_read;
  reg [10 : 0] held_write;
  always @(negedge clock)
  begin
    if(!reset)
    begin
      cycle = cycle + 1;
      m_memory_tick(cycle);
    end
  end
  always @(posedge clock)
  begin
    if(!reset)
    begin
      if(held_r && {rvalid, rlast, rid, rresp, rdata} !== held_read)
      begin
        $fatal(1, "AXI narrow R changed during stall");
      end
      if(held_b && {bvalid, bid, bresp} !== held_write)
      begin
        $fatal(1, "AXI narrow B changed during stall");
      end
      held_r = rvalid && !rready;
      held_b = bvalid && !bready;
      held_read = {rvalid, rlast, rid, rresp, rdata};
      held_write = {bvalid, bid, bresp};
    end
  end

  function automatic integer beat_address
      (input integer start, size, beat,
       input bit fixed_address);
    beat_address = fixed_address || beat == 0 ? start : (start / (1 << size) + beat) * (1 << size);
  endfunction

  task automatic write_address
      (input integer start, size, beats,
       input bit fixed_address);
    @(negedge clock);
    #1;
    awvalid = 1;
    awaddr = 64'h40060000 + start;
    awsize = size;
    awlen = beats - 1;
    awburst = fixed_address ? 0 : 1;
    awid = 8'h31;
    do
      @(posedge clock);
    while(!awready);
    @(negedge clock);
    #1;
    awvalid = 0;
  endtask

  task automatic write_bytes
      (input integer start, size, beats,
       input bit fixed_address);
    integer beat, address, word_base, last_byte, lane;
    for(beat = 0; beat < beats; beat = beat + 1)
    begin
      address = beat_address(start, size, beat, fixed_address);
      word_base = address / 4 * 4;
      last_byte = (address / (1 << size) + 1) * (1 << size);
      @(negedge clock);
      #1;
      wvalid = 1;
      wlast = beat == beats - 1;
      wstrb = 0;
      wdata = 0;
      for(lane = 0; lane < 4; lane = lane + 1)
      begin
        wdata[lane * 8 +: 8] = 8'ha0 + beat * 7 + lane;
        if(word_base + lane >= address && word_base + lane < last_byte)
        begin
          wstrb[lane] = 1;
        end
      end
      do
        @(posedge clock);
      while(!wready);
      for(lane = 0; lane < 4; lane = lane + 1)
      begin
        if(wstrb[lane])
        begin
          golden_bytes[word_base + lane] = wdata[lane * 8 +: 8];
        end
      end
      @(negedge clock);
      #1;
      wvalid = 0;
    end
  endtask

  task automatic read_bytes
      (input integer start, size, beats,
       input bit fixed_address);
    integer beat, address, word_base, last_byte, lane;
    @(negedge clock);
    #1;
    arvalid = 1;
    araddr = 64'h40060000 + start;
    arsize = size;
    arlen = beats - 1;
    arburst = fixed_address ? 0 : 1;
    arid = 8'h52;
    do
      @(posedge clock);
    while(!arready);
    @(negedge clock);
    #1;
    arvalid = 0;
    for(beat = 0; beat < beats; beat = beat + 1)
    begin
      // Hold a valid response before accepting it, including descriptor reuse.
      wait(rvalid);
      repeat(3)
      begin
        @(negedge clock);
      end
      #1;
      rready = 1;
      do
        @(posedge clock);
      while(!rvalid);
      address = beat_address(start, size, beat, fixed_address);
      word_base = address / 4 * 4;
      last_byte = (address / (1 << size) + 1) * (1 << size);
      if(rid != 8'h52 || rresp != 0 || rlast != (beat == beats - 1))
      begin
        $fatal(1, "AXI narrow response metadata");
      end
      for(lane = 0; lane < 4; lane = lane + 1)
      begin
        if(word_base + lane >= address && word_base + lane < last_byte &&
           rdata[lane * 8 +: 8] !== golden_bytes[word_base + lane])
        begin
          $fatal(1, "AXI lane start=%0d size=%0d beat=%0d lane=%0d got=%h expected=%h", start, size,
                 beat, lane, rdata[lane * 8 +: 8], golden_bytes[word_base + lane]);
        end
      end
      @(negedge clock);
      #1;
      rready = 0;
    end
  endtask

  task automatic exercise
      (input integer start, size, beats,
       input bit fixed_address);
    fork
      write_address(start, size, beats, fixed_address);
      write_bytes(start, size, beats, fixed_address);
    join
    wait(bvalid);
    repeat(4)
    begin
      @(negedge clock);
    end
    #1;
    bready = 1;
    @(posedge clock);
    if(bid != 8'h31 || bresp != 0)
    begin
      $fatal(1, "AXI narrow write response");
    end
    @(negedge clock);
    #1;
    bready = 0;
    read_bytes(start, size, beats, fixed_address);
    // Full-width reads detect corruption of neighbouring, unstrobed bytes.
    read_bytes(start / 4 * 4, 2, (beats * (1 << size) + 3) / 4 + 1, 0);
  endtask

  initial
  begin : run
    integer i, word, lane;
    if((m_next(1) & 255) != 2)
    begin
      $fatal(1, "MDPI setup");
    end
    for(i = 0; i < 256; i = i + 1)
    begin
      word = (32'h60000 / 4 + i) ^ 32'h13579bdf;
      for(lane = 0; lane < 4; lane = lane + 1)
      begin
        golden_bytes[i * 4 + lane] = word >> (lane * 8);
      end
    end
    repeat(3)
    begin
      @(negedge clock);
    end
    #1;
    reset = 0;
    // Check reads before writes can hide an incorrect lane mapping.
    for(i = 0; i < 4; i = i + 1)
    begin
      read_bytes(i, 0, 8, 0);
    end
    read_bytes(1, 1, 8, 0);
    read_bytes(3, 2, 8, 0);
    for(i = 0; i < 4; i = i + 1)
    begin
      exercise(64 + i, 0, 8, 0);
    end
    exercise(128, 1, 8, 0);
    exercise(146, 1, 8, 0);
    exercise(161, 1, 8, 0);
    exercise(193, 2, 8, 0);
    exercise(256 + 3, 0, 4, 1);
    exercise(272 + 2, 1, 4, 1);
    repeat(4)
    begin
      @(negedge clock);
    end
    $display("PASS AXI byte lanes, narrow and unaligned FIXED/INCR bursts with real MDPI");
    $finish;
  end
  initial
  begin
    #1000000;
    $fatal(1, "AXI narrow timeout");
  end
endmodule
