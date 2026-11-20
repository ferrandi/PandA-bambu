// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module tb_memory #(parameter DEPTH = 2);
  import "DPI-C" function int unsigned m_next(input int unsigned state);
  reg clock = 0;
  always #5 clock = !clock;
  reg reset = 1, setup_port = 0, done_port = 0;
  reg [1:0] oe = 0, we = 0, bp = 0;
  reg [127:0] addr = 0;
  reg [63:0] wdata = 0;
  reg [11:0] size = 0;
  reg [15:0] tag = 0;
  wire [1:0] ready, request_bp;
  wire [63:0] rdata;
  wire [15:0] rtag;
  reg [1:0] held_valid = 0;
  reg [31:0] held_data [0:1];
  reg [7:0] held_tag [0:1];
  integer stalled_requests = 0, stalled_responses = 0, out_of_order = 0;
  integer largest_tag [0:1];
  integer cycle = 0;
  integer completed [0:1];
  reg pending [0:1][0:255];
  reg [31:0] expected [0:1][0:255];
  reg reading [0:1][0:255];

  TestbenchMEMRamulatorIntegrated #(.index(0), .QUEUE_SIZE(DEPTH),
    .BITSIZE_Mout_oe_ram(2), .BITSIZE_Mout_we_ram(2),
    .BITSIZE_Mout_addr_ram(128), .BITSIZE_Mout_Wdata_ram(64), .BITSIZE_Mout_data_ram_size(12),
    .BITSIZE_Mout_tag(16), .BITSIZE_M_tag(16), .BITSIZE_M_DataRdy(2), .BITSIZE_M_Rdata_ram(64),
    .BITSIZE_M_back_pressure(2), .BITSIZE_Mout_back_pressure(2)) memory(
    .clock(clock), .reset(reset), .setup_port(setup_port), .done_port(done_port),
    .Mout_oe_ram(oe), .Mout_we_ram(we), .Mout_addr_ram(addr), .Mout_Wdata_ram(wdata),
    .Mout_data_ram_size(size), .Mout_tag(tag), .Mout_back_pressure(request_bp),
    .M_DataRdy(ready), .M_Rdata_ram(rdata), .M_tag(rtag), .M_back_pressure(bp));

  always @(posedge clock)
  begin : scoreboard
    integer channel, t;
    if(!reset)
    begin
      cycle = cycle + 1;
      for(channel = 0; channel < 2; channel = channel + 1)
      begin
        if(held_valid[channel] && (!ready[channel] || rdata[channel*32+:32] !== held_data[channel] ||
                                  rtag[channel*8+:8] !== held_tag[channel]))
          $fatal(1, "minimal response changed during stall");
        held_valid[channel] = ready[channel] && bp[channel];
        held_data[channel] = rdata[channel*32+:32];
        held_tag[channel] = rtag[channel*8+:8];
        if(request_bp[channel]) stalled_requests = stalled_requests + 1;
        if(held_valid[channel]) stalled_responses = stalled_responses + 1;
        if((oe[channel] || we[channel]) && !request_bp[channel])
        begin
          t = tag[channel*8+:8];
          if(pending[channel][t]) $fatal(1, "duplicate request tag");
          pending[channel][t] = 1;
          reading[channel][t] = oe[channel];
          expected[channel][t] = ((addr[channel*64+:64] - 64'h40000000) / 4) ^ 32'h13579bdf;
        end
        if(ready[channel] && !bp[channel])
        begin
          t = rtag[channel*8+:8];
          if(!pending[channel][t]) $fatal(1, "unknown/duplicate response ch=%0d tag=%0d", channel, t);
          if(reading[channel][t] && rdata[channel*32+:32] !== expected[channel][t])
            $fatal(1, "minimal data ch=%0d tag=%0d got=%h expected=%h", channel, t, rdata[channel*32+:32], expected[channel][t]);
          if(!reading[channel][t] && rdata[channel*32+:32] != 0) $fatal(1, "write response contaminated data");
          if(t < largest_tag[channel]) out_of_order = out_of_order + 1;
          if(t > largest_tag[channel]) largest_tag[channel] = t;
          pending[channel][t] = 0;
          completed[channel] = completed[channel] + 1;
        end
      end
    end
  end

  reg traffic_running = 0;
  integer sent [0:1];
  reg [1:0] accepted = 0;
  always @(posedge clock)
  begin
    accepted = (oe | we) & ~request_bp;
  end
  // A synchronous fake accelerator holds each request until the TB accepts it.
  always @(negedge clock)
  begin : fake_accelerator
    integer channel;
    if(!reset)
    begin
      bp[0] = cycle < 250 || cycle % 13 < 5;
      bp[1] = cycle < 350 || cycle % 17 < 8;
    end
    oe = 0;
    we = 0;
    for(channel = 0; channel < 2; channel = channel + 1)
    begin
      if(accepted[channel]) sent[channel] = sent[channel] + 1;
      if(traffic_running && sent[channel] <= 200)
      begin
        oe[channel] = sent[channel] % 3 != 0;
        we[channel] = sent[channel] % 3 == 0;
        addr[channel*64+:64] = 64'h40000000 + channel*65536 + sent[channel]*64;
        wdata[channel*32+:32] = 32'hf00d0000 + sent[channel];
        size[channel*6+:6] = 32;
        tag[channel*8+:8] = sent[channel];
      end
    end
  end

  initial
  begin : run
    integer channel, t;
    if((m_next(1) & 255) != 2) $fatal(1, "MDPI setup");
    for(channel = 0; channel < 2; channel = channel + 1)
    begin
      completed[channel] = 0;
      sent[channel] = 1;
      largest_tag[channel] = 0;
      for(t = 0; t < 256; t = t + 1) pending[channel][t] = 0;
    end
    repeat(3) @(negedge clock);
    reset = 0; setup_port = 1;
    repeat(2) @(negedge clock);
    setup_port = 0;
    #1; traffic_running = 1;
    wait(completed[0] == 200 && completed[1] == 200);
    repeat(5) @(negedge clock);
    done_port = 1;
    if(stalled_requests == 0 || stalled_responses == 0) $fatal(1, "missing minimal saturation coverage");
    if(DEPTH == 64 && out_of_order == 0) $fatal(1, "missing out-of-order response coverage");
    $display("PASS real Ramulator minimal multichannel stalled responses");
    $finish;
  end
  initial begin #1000000; $fatal(1, "minimal timeout ch0=%0d ch1=%0d", completed[0], completed[1]); end
endmodule
