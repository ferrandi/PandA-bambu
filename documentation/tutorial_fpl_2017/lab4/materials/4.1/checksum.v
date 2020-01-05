// 
// Politecnico di Milano
// Code created using PandA - Version: PandA 0.9.5 - Revision b82983176017785e69abc44ef7b3522d5ed9a7bc-non_restoring_sdiv - Date 2017-Sep-04 18:45:02
// /opt/panda/bin/bambu executed with: /opt/panda/bin/bambu -O3 --top-fname=checksum --do-not-expose-globals --std=c11 -I../ --simulate ../checksum.c 
// 
// Send any bug to: panda-info@polimi.it
// ************************************************************************
// The following text holds for all the components tagged with PANDA_LGPLv3.
// They are all part of the BAMBU/PANDA IP LIBRARY.
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with the PandA framework; see the files COPYING.LIB
// If not, see <http://www.gnu.org/licenses/>.
// ************************************************************************

`ifdef __ICARUS__
  `define _SIM_HAVE_CLOG2
`endif
`ifdef VERILATOR
  `define _SIM_HAVE_CLOG2
`endif
`ifdef MODEL_TECH
  `define _SIM_HAVE_CLOG2
`endif
`ifdef VCS
  `define _SIM_HAVE_CLOG2
`endif
`ifdef NCVERILOG
  `define _SIM_HAVE_CLOG2
`endif
`ifdef XILINX_SIMULATOR
  `define _SIM_HAVE_CLOG2
`endif
`ifdef XILINX_ISIM
  `define _SIM_HAVE_CLOG2
`endif

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>, Christian Pilato <christian.pilato@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module mangled_constant_value(out1);
  parameter BITSIZE_out1=1, value=1'b0;
  // OUT
  output [BITSIZE_out1-1:0] out1;
  assign out1 = value;
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module mangled_UUconvert_expr_FU(in1, out1);
  parameter BITSIZE_in1=1, BITSIZE_out1=1;
  // IN
  input [BITSIZE_in1-1:0] in1;
  // OUT
  output [BITSIZE_out1-1:0] out1;
  generate
  if (BITSIZE_out1 <= BITSIZE_in1)
  begin
    assign out1 = in1[BITSIZE_out1-1:0];
  end
  else
  begin
    assign out1 = {{(BITSIZE_out1-BITSIZE_in1){1'b0}},in1};
  end
  endgenerate
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module mangled_UIdata_converter_FU(in1, out1);
  parameter BITSIZE_in1=1, BITSIZE_out1=1;
  // IN
  input [BITSIZE_in1-1:0] in1;
  // OUT
  output signed [BITSIZE_out1-1:0] out1;
  generate
  if (BITSIZE_out1 <= BITSIZE_in1)
  begin
    assign out1 = in1[BITSIZE_out1-1:0];
  end
  else
  begin
    assign out1 = {{(BITSIZE_out1-BITSIZE_in1){1'b0}},in1};
  end
  endgenerate
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module IUdata_converter_FU(in1, out1);
  parameter BITSIZE_in1=1, BITSIZE_out1=1;
  // IN
  input signed [BITSIZE_in1-1:0] in1;
  // OUT
  output [BITSIZE_out1-1:0] out1;
  generate
  if (BITSIZE_out1 <= BITSIZE_in1)
  begin
    assign out1 = in1[BITSIZE_out1-1:0];
  end
  else
  begin
    assign out1 = {{(BITSIZE_out1-BITSIZE_in1){in1[BITSIZE_in1-1]}},in1};
  end
  endgenerate
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module plus_expr_FU(in1, in2, out1);
  parameter BITSIZE_in1=1, BITSIZE_in2=1, BITSIZE_out1=1;
  // IN
  input signed [BITSIZE_in1-1:0] in1;
  input signed [BITSIZE_in2-1:0] in2;
  // OUT
  output signed [BITSIZE_out1-1:0] out1;
  assign out1 = in1 + in2;
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module ternary_plus_expr_FU(in1, in2, in3, out1);
  parameter BITSIZE_in1=1, BITSIZE_in2=1, BITSIZE_in3=1, BITSIZE_out1=1;
  // IN
  input signed [BITSIZE_in1-1:0] in1;
  input signed [BITSIZE_in2-1:0] in2;
  input signed [BITSIZE_in3-1:0] in3;
  // OUT
  output signed [BITSIZE_out1-1:0] out1;
  assign out1 = in1 + in2 + in3;
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module ui_plus_expr_FU(in1, in2, out1);
  parameter BITSIZE_in1=1, BITSIZE_in2=1, BITSIZE_out1=1;
  // IN
  input [BITSIZE_in1-1:0] in1;
  input [BITSIZE_in2-1:0] in2;
  // OUT
  output [BITSIZE_out1-1:0] out1;
  assign out1 = in1 + in2;
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2004-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module ui_rshift_expr_FU(in1, in2, out1);
  parameter BITSIZE_in1=1, BITSIZE_in2=1, BITSIZE_out1=1, PRECISION=1;
  // IN
  input [BITSIZE_in1-1:0] in1;
  input [BITSIZE_in2-1:0] in2;
  // OUT
  output [BITSIZE_out1-1:0] out1;
  `ifndef _SIM_HAVE_CLOG2
    function integer log2;
       input integer value;
       integer temp_value;
      begin
        temp_value = value-1;
        for (log2=0; temp_value>0; log2=log2+1)
          temp_value = temp_value>>1;
      end
    endfunction
  `endif
  `ifdef _SIM_HAVE_CLOG2
    parameter arg2_bitsize = $clog2(PRECISION);
  `else
    parameter arg2_bitsize = log2(PRECISION);
  `endif
  generate
    if(BITSIZE_in2 > arg2_bitsize)
      assign out1 = in1 >> (in2[arg2_bitsize-1:0]);
    else
      assign out1 = in1 >> in2;
  endgenerate

endmodule

// Datapath RTL descrition for checksum
// This component has been derived from the input source code and so it does not fall under the copyright of PandA framework, but it follows the input source code copyright, and may be aggregated with components of the BAMBU/PANDA IP LIBRARY.
// Author(s): Component automatically generated by bambu
// License: THIS COMPONENT IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
`timescale 1ns / 1ps
module datapath_checksum(clock, reset, in_port_in1, in_port_in2, return_port);
  // IN
  input clock;
  input reset;
  input [63:0] in_port_in1;
  input [63:0] in_port_in2;
  // OUT
  output [7:0] return_port;
  // Component and signal declarations
  wire [7:0] out_IUdata_converter_FU_26_i0_fu_checksum_25432_25507;
  wire signed [7:0] out_UIdata_converter_FU_10_i0_fu_checksum_25432_25470;
  wire signed [7:0] out_UIdata_converter_FU_13_i0_fu_checksum_25432_25477;
  wire signed [7:0] out_UIdata_converter_FU_16_i0_fu_checksum_25432_25484;
  wire signed [7:0] out_UIdata_converter_FU_19_i0_fu_checksum_25432_25491;
  wire signed [7:0] out_UIdata_converter_FU_22_i0_fu_checksum_25432_25498;
  wire signed [7:0] out_UIdata_converter_FU_25_i0_fu_checksum_25432_25505;
  wire signed [7:0] out_UIdata_converter_FU_4_i0_fu_checksum_25432_25457;
  wire signed [7:0] out_UIdata_converter_FU_7_i0_fu_checksum_25432_25463;
  wire [7:0] out_UUconvert_expr_FU_11_i0_fu_checksum_25432_25473;
  wire [7:0] out_UUconvert_expr_FU_12_i0_fu_checksum_25432_25475;
  wire [7:0] out_UUconvert_expr_FU_14_i0_fu_checksum_25432_25480;
  wire [7:0] out_UUconvert_expr_FU_15_i0_fu_checksum_25432_25482;
  wire [7:0] out_UUconvert_expr_FU_17_i0_fu_checksum_25432_25487;
  wire [7:0] out_UUconvert_expr_FU_18_i0_fu_checksum_25432_25489;
  wire [7:0] out_UUconvert_expr_FU_20_i0_fu_checksum_25432_25494;
  wire [7:0] out_UUconvert_expr_FU_21_i0_fu_checksum_25432_25496;
  wire [7:0] out_UUconvert_expr_FU_23_i0_fu_checksum_25432_25501;
  wire [7:0] out_UUconvert_expr_FU_24_i0_fu_checksum_25432_25503;
  wire [7:0] out_UUconvert_expr_FU_2_i0_fu_checksum_25432_25454;
  wire [7:0] out_UUconvert_expr_FU_3_i0_fu_checksum_25432_25455;
  wire [7:0] out_UUconvert_expr_FU_5_i0_fu_checksum_25432_25459;
  wire [7:0] out_UUconvert_expr_FU_6_i0_fu_checksum_25432_25461;
  wire [7:0] out_UUconvert_expr_FU_8_i0_fu_checksum_25432_25466;
  wire [7:0] out_UUconvert_expr_FU_9_i0_fu_checksum_25432_25468;
  wire [4:0] out_const_0;
  wire [5:0] out_const_1;
  wire [6:0] out_const_2;
  wire [6:0] out_const_3;
  wire [5:0] out_const_4;
  wire [6:0] out_const_5;
  wire [6:0] out_const_6;
  wire signed [7:0] out_plus_expr_FU_8_8_8_28_i0_fu_checksum_25432_25506;
  wire signed [7:0] out_ternary_plus_expr_FU_8_8_8_8_29_i0_fu_checksum_25432_25471;
  wire signed [7:0] out_ternary_plus_expr_FU_8_8_8_8_29_i1_fu_checksum_25432_25485;
  wire signed [7:0] out_ternary_plus_expr_FU_8_8_8_8_29_i2_fu_checksum_25432_25499;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i0_fu_checksum_25432_25456;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i1_fu_checksum_25432_25462;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i2_fu_checksum_25432_25469;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i3_fu_checksum_25432_25476;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i4_fu_checksum_25432_25483;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i5_fu_checksum_25432_25490;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i6_fu_checksum_25432_25497;
  wire [7:0] out_ui_plus_expr_FU_8_8_8_30_i7_fu_checksum_25432_25504;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_31_i0_fu_checksum_25432_25458;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_31_i1_fu_checksum_25432_25460;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_32_i0_fu_checksum_25432_25465;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_32_i1_fu_checksum_25432_25467;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_33_i0_fu_checksum_25432_25472;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_33_i1_fu_checksum_25432_25474;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_34_i0_fu_checksum_25432_25479;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_34_i1_fu_checksum_25432_25481;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_35_i0_fu_checksum_25432_25486;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_35_i1_fu_checksum_25432_25488;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_36_i0_fu_checksum_25432_25493;
  wire [8:0] out_ui_rshift_expr_FU_64_0_64_36_i1_fu_checksum_25432_25495;
  wire [7:0] out_ui_rshift_expr_FU_64_0_64_37_i0_fu_checksum_25432_25500;
  wire [7:0] out_ui_rshift_expr_FU_64_0_64_37_i1_fu_checksum_25432_25502;
  
  mangled_constant_value #(.BITSIZE_out1(5), .value(5'b01000)) const_0 (.out1(out_const_0));
  mangled_constant_value #(.BITSIZE_out1(6), .value(6'b010000)) const_1 (.out1(out_const_1));
  mangled_constant_value #(.BITSIZE_out1(7), .value(7'b0100000)) const_2 (.out1(out_const_2));
  mangled_constant_value #(.BITSIZE_out1(7), .value(7'b0101000)) const_3 (.out1(out_const_3));
  mangled_constant_value #(.BITSIZE_out1(6), .value(6'b011000)) const_4 (.out1(out_const_4));
  mangled_constant_value #(.BITSIZE_out1(7), .value(7'b0110000)) const_5 (.out1(out_const_5));
  mangled_constant_value #(.BITSIZE_out1(7), .value(7'b0111000)) const_6 (.out1(out_const_6));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(64), .BITSIZE_out1(8)) fu_checksum_25432_25454 (.out1(out_UUconvert_expr_FU_2_i0_fu_checksum_25432_25454), .in1(in_port_in1));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(64), .BITSIZE_out1(8)) fu_checksum_25432_25455 (.out1(out_UUconvert_expr_FU_3_i0_fu_checksum_25432_25455), .in1(in_port_in2));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25456 (.out1(out_ui_plus_expr_FU_8_8_8_30_i0_fu_checksum_25432_25456), .in1(out_UUconvert_expr_FU_2_i0_fu_checksum_25432_25454), .in2(out_UUconvert_expr_FU_3_i0_fu_checksum_25432_25455));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25457 (.out1(out_UIdata_converter_FU_4_i0_fu_checksum_25432_25457), .in1(out_ui_plus_expr_FU_8_8_8_30_i0_fu_checksum_25432_25456));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(5), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25458 (.out1(out_ui_rshift_expr_FU_64_0_64_31_i0_fu_checksum_25432_25458), .in1(in_port_in1), .in2(out_const_0));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25459 (.out1(out_UUconvert_expr_FU_5_i0_fu_checksum_25432_25459), .in1(out_ui_rshift_expr_FU_64_0_64_31_i0_fu_checksum_25432_25458));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(5), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25460 (.out1(out_ui_rshift_expr_FU_64_0_64_31_i1_fu_checksum_25432_25460), .in1(in_port_in2), .in2(out_const_0));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25461 (.out1(out_UUconvert_expr_FU_6_i0_fu_checksum_25432_25461), .in1(out_ui_rshift_expr_FU_64_0_64_31_i1_fu_checksum_25432_25460));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25462 (.out1(out_ui_plus_expr_FU_8_8_8_30_i1_fu_checksum_25432_25462), .in1(out_UUconvert_expr_FU_5_i0_fu_checksum_25432_25459), .in2(out_UUconvert_expr_FU_6_i0_fu_checksum_25432_25461));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25463 (.out1(out_UIdata_converter_FU_7_i0_fu_checksum_25432_25463), .in1(out_ui_plus_expr_FU_8_8_8_30_i1_fu_checksum_25432_25462));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(6), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25465 (.out1(out_ui_rshift_expr_FU_64_0_64_32_i0_fu_checksum_25432_25465), .in1(in_port_in1), .in2(out_const_1));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25466 (.out1(out_UUconvert_expr_FU_8_i0_fu_checksum_25432_25466), .in1(out_ui_rshift_expr_FU_64_0_64_32_i0_fu_checksum_25432_25465));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(6), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25467 (.out1(out_ui_rshift_expr_FU_64_0_64_32_i1_fu_checksum_25432_25467), .in1(in_port_in2), .in2(out_const_1));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25468 (.out1(out_UUconvert_expr_FU_9_i0_fu_checksum_25432_25468), .in1(out_ui_rshift_expr_FU_64_0_64_32_i1_fu_checksum_25432_25467));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25469 (.out1(out_ui_plus_expr_FU_8_8_8_30_i2_fu_checksum_25432_25469), .in1(out_UUconvert_expr_FU_8_i0_fu_checksum_25432_25466), .in2(out_UUconvert_expr_FU_9_i0_fu_checksum_25432_25468));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25470 (.out1(out_UIdata_converter_FU_10_i0_fu_checksum_25432_25470), .in1(out_ui_plus_expr_FU_8_8_8_30_i2_fu_checksum_25432_25469));
  ternary_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_in3(8), .BITSIZE_out1(8)) fu_checksum_25432_25471 (.out1(out_ternary_plus_expr_FU_8_8_8_8_29_i0_fu_checksum_25432_25471), .in1(out_UIdata_converter_FU_4_i0_fu_checksum_25432_25457), .in2(out_UIdata_converter_FU_7_i0_fu_checksum_25432_25463), .in3(out_UIdata_converter_FU_10_i0_fu_checksum_25432_25470));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(6), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25472 (.out1(out_ui_rshift_expr_FU_64_0_64_33_i0_fu_checksum_25432_25472), .in1(in_port_in1), .in2(out_const_4));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25473 (.out1(out_UUconvert_expr_FU_11_i0_fu_checksum_25432_25473), .in1(out_ui_rshift_expr_FU_64_0_64_33_i0_fu_checksum_25432_25472));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(6), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25474 (.out1(out_ui_rshift_expr_FU_64_0_64_33_i1_fu_checksum_25432_25474), .in1(in_port_in2), .in2(out_const_4));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25475 (.out1(out_UUconvert_expr_FU_12_i0_fu_checksum_25432_25475), .in1(out_ui_rshift_expr_FU_64_0_64_33_i1_fu_checksum_25432_25474));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25476 (.out1(out_ui_plus_expr_FU_8_8_8_30_i3_fu_checksum_25432_25476), .in1(out_UUconvert_expr_FU_11_i0_fu_checksum_25432_25473), .in2(out_UUconvert_expr_FU_12_i0_fu_checksum_25432_25475));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25477 (.out1(out_UIdata_converter_FU_13_i0_fu_checksum_25432_25477), .in1(out_ui_plus_expr_FU_8_8_8_30_i3_fu_checksum_25432_25476));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25479 (.out1(out_ui_rshift_expr_FU_64_0_64_34_i0_fu_checksum_25432_25479), .in1(in_port_in1), .in2(out_const_2));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25480 (.out1(out_UUconvert_expr_FU_14_i0_fu_checksum_25432_25480), .in1(out_ui_rshift_expr_FU_64_0_64_34_i0_fu_checksum_25432_25479));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25481 (.out1(out_ui_rshift_expr_FU_64_0_64_34_i1_fu_checksum_25432_25481), .in1(in_port_in2), .in2(out_const_2));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25482 (.out1(out_UUconvert_expr_FU_15_i0_fu_checksum_25432_25482), .in1(out_ui_rshift_expr_FU_64_0_64_34_i1_fu_checksum_25432_25481));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25483 (.out1(out_ui_plus_expr_FU_8_8_8_30_i4_fu_checksum_25432_25483), .in1(out_UUconvert_expr_FU_15_i0_fu_checksum_25432_25482), .in2(out_UUconvert_expr_FU_14_i0_fu_checksum_25432_25480));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25484 (.out1(out_UIdata_converter_FU_16_i0_fu_checksum_25432_25484), .in1(out_ui_plus_expr_FU_8_8_8_30_i4_fu_checksum_25432_25483));
  ternary_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_in3(8), .BITSIZE_out1(8)) fu_checksum_25432_25485 (.out1(out_ternary_plus_expr_FU_8_8_8_8_29_i1_fu_checksum_25432_25485), .in1(out_ternary_plus_expr_FU_8_8_8_8_29_i0_fu_checksum_25432_25471), .in2(out_UIdata_converter_FU_13_i0_fu_checksum_25432_25477), .in3(out_UIdata_converter_FU_16_i0_fu_checksum_25432_25484));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25486 (.out1(out_ui_rshift_expr_FU_64_0_64_35_i0_fu_checksum_25432_25486), .in1(in_port_in1), .in2(out_const_3));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25487 (.out1(out_UUconvert_expr_FU_17_i0_fu_checksum_25432_25487), .in1(out_ui_rshift_expr_FU_64_0_64_35_i0_fu_checksum_25432_25486));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25488 (.out1(out_ui_rshift_expr_FU_64_0_64_35_i1_fu_checksum_25432_25488), .in1(in_port_in2), .in2(out_const_3));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25489 (.out1(out_UUconvert_expr_FU_18_i0_fu_checksum_25432_25489), .in1(out_ui_rshift_expr_FU_64_0_64_35_i1_fu_checksum_25432_25488));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25490 (.out1(out_ui_plus_expr_FU_8_8_8_30_i5_fu_checksum_25432_25490), .in1(out_UUconvert_expr_FU_18_i0_fu_checksum_25432_25489), .in2(out_UUconvert_expr_FU_17_i0_fu_checksum_25432_25487));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25491 (.out1(out_UIdata_converter_FU_19_i0_fu_checksum_25432_25491), .in1(out_ui_plus_expr_FU_8_8_8_30_i5_fu_checksum_25432_25490));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25493 (.out1(out_ui_rshift_expr_FU_64_0_64_36_i0_fu_checksum_25432_25493), .in1(in_port_in1), .in2(out_const_5));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25494 (.out1(out_UUconvert_expr_FU_20_i0_fu_checksum_25432_25494), .in1(out_ui_rshift_expr_FU_64_0_64_36_i0_fu_checksum_25432_25493));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(9), .PRECISION(64)) fu_checksum_25432_25495 (.out1(out_ui_rshift_expr_FU_64_0_64_36_i1_fu_checksum_25432_25495), .in1(in_port_in2), .in2(out_const_5));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(9), .BITSIZE_out1(8)) fu_checksum_25432_25496 (.out1(out_UUconvert_expr_FU_21_i0_fu_checksum_25432_25496), .in1(out_ui_rshift_expr_FU_64_0_64_36_i1_fu_checksum_25432_25495));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25497 (.out1(out_ui_plus_expr_FU_8_8_8_30_i6_fu_checksum_25432_25497), .in1(out_UUconvert_expr_FU_21_i0_fu_checksum_25432_25496), .in2(out_UUconvert_expr_FU_20_i0_fu_checksum_25432_25494));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25498 (.out1(out_UIdata_converter_FU_22_i0_fu_checksum_25432_25498), .in1(out_ui_plus_expr_FU_8_8_8_30_i6_fu_checksum_25432_25497));
  ternary_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_in3(8), .BITSIZE_out1(8)) fu_checksum_25432_25499 (.out1(out_ternary_plus_expr_FU_8_8_8_8_29_i2_fu_checksum_25432_25499), .in1(out_ternary_plus_expr_FU_8_8_8_8_29_i1_fu_checksum_25432_25485), .in2(out_UIdata_converter_FU_19_i0_fu_checksum_25432_25491), .in3(out_UIdata_converter_FU_22_i0_fu_checksum_25432_25498));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(8), .PRECISION(64)) fu_checksum_25432_25500 (.out1(out_ui_rshift_expr_FU_64_0_64_37_i0_fu_checksum_25432_25500), .in1(in_port_in1), .in2(out_const_6));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25501 (.out1(out_UUconvert_expr_FU_23_i0_fu_checksum_25432_25501), .in1(out_ui_rshift_expr_FU_64_0_64_37_i0_fu_checksum_25432_25500));
  ui_rshift_expr_FU #(.BITSIZE_in1(64), .BITSIZE_in2(7), .BITSIZE_out1(8), .PRECISION(64)) fu_checksum_25432_25502 (.out1(out_ui_rshift_expr_FU_64_0_64_37_i1_fu_checksum_25432_25502), .in1(in_port_in2), .in2(out_const_6));
  mangled_UUconvert_expr_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25503 (.out1(out_UUconvert_expr_FU_24_i0_fu_checksum_25432_25503), .in1(out_ui_rshift_expr_FU_64_0_64_37_i1_fu_checksum_25432_25502));
  ui_plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25504 (.out1(out_ui_plus_expr_FU_8_8_8_30_i7_fu_checksum_25432_25504), .in1(out_UUconvert_expr_FU_24_i0_fu_checksum_25432_25503), .in2(out_UUconvert_expr_FU_23_i0_fu_checksum_25432_25501));
  mangled_UIdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25505 (.out1(out_UIdata_converter_FU_25_i0_fu_checksum_25432_25505), .in1(out_ui_plus_expr_FU_8_8_8_30_i7_fu_checksum_25432_25504));
  plus_expr_FU #(.BITSIZE_in1(8), .BITSIZE_in2(8), .BITSIZE_out1(8)) fu_checksum_25432_25506 (.out1(out_plus_expr_FU_8_8_8_28_i0_fu_checksum_25432_25506), .in1(out_UIdata_converter_FU_25_i0_fu_checksum_25432_25505), .in2(out_ternary_plus_expr_FU_8_8_8_8_29_i2_fu_checksum_25432_25499));
  IUdata_converter_FU #(.BITSIZE_in1(8), .BITSIZE_out1(8)) fu_checksum_25432_25507 (.out1(out_IUdata_converter_FU_26_i0_fu_checksum_25432_25507), .in1(out_plus_expr_FU_8_8_8_28_i0_fu_checksum_25432_25506));
  // io-signal post fix
  assign return_port = out_IUdata_converter_FU_26_i0_fu_checksum_25432_25507;

endmodule

// FSM based controller descrition for checksum
// This component has been derived from the input source code and so it does not fall under the copyright of PandA framework, but it follows the input source code copyright, and may be aggregated with components of the BAMBU/PANDA IP LIBRARY.
// Author(s): Component automatically generated by bambu
// License: THIS COMPONENT IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
`timescale 1ns / 1ps
module controller_checksum(done_port, clock, reset, start_port);
  // IN
  input clock;
  input reset;
  input start_port;
  // OUT
  output done_port;
  parameter [0:0] S_0 = 1'b1;
  reg [0:0] _present_state, _next_state;
  reg done_port;
  
  always @(posedge clock)
    if (reset == 1'b0) _present_state <= S_0;
    else _present_state <= _next_state;
  
  always @(*)
  begin
    done_port = 1'b0;
    case (_present_state)
      S_0 :
        if(start_port == 1'b1)
        begin
          _next_state = S_0;
          done_port = 1'b1;
        end
        else
          _next_state = S_0;
      
      default :
        begin
          _next_state = S_0;
          done_port = 1'bX;
        end
    endcase
  end
endmodule

// Top component for checksum
// This component has been derived from the input source code and so it does not fall under the copyright of PandA framework, but it follows the input source code copyright, and may be aggregated with components of the BAMBU/PANDA IP LIBRARY.
// Author(s): Component automatically generated by bambu
// License: THIS COMPONENT IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
`timescale 1ns / 1ps
module checksum(clock, reset, start_port, done_port, in_val1, in_val2, out_val);
  // IN
  input clock;
  input reset;
  input start_port;
  input [63:0] in_val1;
  input [63:0] in_val2;
  // OUT
  output done_port;
  output [7:0] out_val;
  // Component and signal declarations
  
  controller_checksum Controller_i (.done_port(done_port), .clock(clock), .reset(reset), .start_port(start_port));
  datapath_checksum Datapath_i (.return_port(out_val), .clock(clock), .reset(reset), .in_port_in1(in1), .in_port_in2(in2));

endmodule
