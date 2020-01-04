// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module sw_ctrl(clock, reset, start_port, sw, out1);
  // IN
  input clock;
  input reset;
  input start_port;
  input [15:0] sw;
  // OUT
  output [15:0] out1;
  reg [15:0] out1;
  always @(posedge clock or negedge reset)
  begin
    if (!reset)
    begin
      out1<=0;
    end
    else
    begin
      if(start_port) out1<=sw;
    end
  end
endmodule

