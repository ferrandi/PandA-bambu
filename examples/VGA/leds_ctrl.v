// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2015-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module leds_ctrl(clock, reset, start_port, id, val, done_port, HEX0, HEX1, HEX2, HEX3, LEDR, LEDG);
  // IN
  input clock;
  input reset;
  input start_port;
  input [2:0] id;
  input [9:0] val;
  // OUT
  output done_port;
  output [6:0] HEX0;
  output [6:0] HEX1;
  output [6:0] HEX2;
  output [6:0] HEX3;
  output [9:0] LEDR;
  output [7:0] LEDG;
  reg [6:0] HEX0;
  reg [6:0] HEX1;
  reg [6:0] HEX2;
  reg [6:0] HEX3;
  reg [9:0] LEDR;
  reg [7:0] LEDG;
  always @(posedge clock or negedge reset)
  begin
    if (!reset)
    begin
      HEX0=0;
      HEX1=0;
      HEX2=0;
      HEX3=0;
      LEDR=0;
      LEDG=0;
    end
    else
    begin
      if(id==0) HEX0=val;
      else if(id==1) HEX1=val;
      else if(id==2) HEX2=val;
      else if(id==3) HEX3=val;
      else if(id==4) LEDR=val;
      else if(id==5) LEDG=val;
    end
  end
endmodule

