// This component is part of the PANDA/BAMBU IP LIBRARY
// Copyright (C) 2016-2026 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_MIT
`timescale 1ns / 1ps
module get_ticks(clock, reset, start_port, restart_value, out1);
  // IN
  input clock;
  input reset;
  input start_port;
  input [7:0] restart_value;
  // OUT
  output [31:0] out1;
  reg [31:0] out1;
  reg [31:0] counter = 0;

  always @(posedge clock)
  begin
    if (!reset || (start_port && restart_value[0]==1))
      out1 <= 0;
    else if(counter==(100000-1))
      out1 <= out1 + 1;
  end

  always @(posedge clock)
  begin
    if (!reset || counter==(100000-1))
      counter <= 0;
    else
      counter <= counter + 1;
  end

endmodule

