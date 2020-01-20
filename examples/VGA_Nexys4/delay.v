// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module delay(clock, reset, start_port, value, done_port);
  // IN
  input clock;
  input reset;
  input start_port;
  input [31:0] value;
  // OUT
  output done_port;
  reg [31:0] counter = 0;
  assign done_port = value == counter;
  always @(posedge clock)
  begin
    if (!reset || start_port)
      counter <= 0;
    else
      counter <= counter + 1;
  end
endmodule

