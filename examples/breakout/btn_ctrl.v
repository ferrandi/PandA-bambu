// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module btn_ctrl(clock, reset, start_port, btn, out1);
parameter number_of_clocks = 4096;
  // IN
  input clock;
  input reset;
  input start_port;
  input [3:0] btn;
  // OUT
  output [3:0] out1;
  reg [3:0] out1;
  
  reg [15:0] cnt=0;
  reg [3:0] sigTmp;
  reg [3:0] stble;

  always @(posedge clock)
  begin
    if (!reset)
    begin
      sigTmp<=0;
      cnt<=0;
      stble<=0;
    end
    else
    begin
      if (btn == sigTmp)
      begin
        if (cnt == number_of_clocks-1)
        begin
          stble <= btn;
          cnt <= 0;
        end
        else
          cnt <= cnt + 1;
      end
      else
      begin
          cnt <= 0;
          sigTmp <= btn;
      end
    end
  end

  always @(posedge clock)
  begin
    if (!reset)
    begin
      out1<=0;
    end
    else
    begin
      if(start_port) out1<=stble;
    end
  end
endmodule

