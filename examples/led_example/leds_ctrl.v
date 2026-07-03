// This component is part of the PANDA/BAMBU IP LIBRARY
// Copyright (C) 2016-2026 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_MIT
`timescale 1ns / 1ps
module leds_ctrl(clock, reset, start_port, val, leds);
  // IN
  input clock;
  input reset;
  input start_port;
  input [15:0] val;
  // OUT
  output [15:0] leds;
  reg [15:0] leds;
  always @(posedge clock or negedge reset)
  begin
    if (!reset)
    begin
      leds<=0;
    end
    else
    begin
      if(start_port) leds<=val;
    end
  end
endmodule

