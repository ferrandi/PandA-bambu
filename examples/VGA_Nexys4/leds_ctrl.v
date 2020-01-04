// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
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
  always @(posedge clock)
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

