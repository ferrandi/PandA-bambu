// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module sevensegments_ctrl(clock, reset, start_port, val, mask, sseg_ca, sseg_an);
  // IN
  input clock;
  input reset;
  input start_port;
  input [63:0] val;
  input [63:0] mask;
  // OUT
  output [7:0] sseg_ca;
  output [7:0] sseg_an;
  reg [63:0] mem;
  reg [7:0] index;
  reg [16:0] counter=0;
  wire int_clock;
  assign sseg_ca = (mem[7:0] & {8{index[0]}})|(mem[15:8] & {8{index[1]}})|(mem[23:16] & {8{index[2]}})|(mem[31:24] & {8{index[3]}})|
                   (mem[39:32] & {8{index[4]}})|(mem[47:40] & {8{index[5]}})|(mem[55:48] & {8{index[6]}})|(mem[63:56] & {8{index[7]}});
  assign sseg_an = ~index;
  always @(posedge clock)
  begin
    if (!reset)
    begin
      mem<={64{1'b1}};
    end
    else
    begin
      if(start_port) mem <= (mem&(~mask)) | ((~val)&mask);
    end
  end
  always @(posedge clock)
  begin
    if (!reset)
    begin
      counter <= 0;
    end
    else
    begin
      counter <= counter + 1;
    end
  end
  assign int_clock = counter[16];
  always @(posedge int_clock)
  begin
    if (!reset)
    begin
      index<=8'd1;
    end
    else
    begin
      index <= {index[6:0],index[7]};
    end
  end

endmodule

