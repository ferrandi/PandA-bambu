/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                 URL: http://trac.ws.dei.polimi.it/panda
 *                      Microarchitectures Laboratory
 *                       Politecnico di Milano - DEIB
 *             ***********************************************
 *              Copyright (c) 2015-2020 Politecnico di Milano
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the 
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
*/
/**
 * @file printf_verilog_generator_p1n.cpp
 * @brief Snippet for the printf dynamimc generator when a parallel bus architecture is considered.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
std::string fsm;
std::string case_statement;

std::stringstream _npString;
_npString<<((_np < 2) ? 2 : _np);
std::string selector_dimension=_npString.str();

std::stringstream _np1String;
_np1String<<(((_np-1) < 1) ? 1 : (_np-1));
std::string selector_left=_np1String.str();

int selector=1;
for(int i=0;i<_np;i++){
   std::stringstream _selector_string;
   _selector_string<<selector;
   std::stringstream _index_string;
   _index_string<<(i+1);
   case_statement+="            "+selector_dimension+"'d"+_selector_string.str()+":\n             begin\n               data1="+_p[i].name+";\n               data1_size=BITSIZE_in"+_index_string.str()+";\n             end\n";
   selector*=2;
}
case_statement+="            default:\n\
             begin\n\
               data1 = 64'b0;\n\
               data1_size = 8'b0;\n\
             end\n";

std::string sensitivity;
for(int i=0;i<_np;i++){
  sensitivity += " or " + _p[i].name;
}
if(_np>1){
  case_statement="case (_present_selector)\n"+case_statement+"          endcase";
}
else{
  case_statement="";
}


fsm="\
// synthesis translate_off\n\
function real bits32_to_real64;\n\
  input [31:0] fin1;\n\
  reg [7:0] exponent1;\n\
  reg is_exp_zero;\n\
  reg is_all_ones;\n\
  reg [10:0] exp_tmp;\n\
  reg [63:0] fout1;\n\
begin\n\
  exponent1 = fin1[30:23];\n\
  is_exp_zero = exponent1 == 8'd0;\n\
  is_all_ones = exponent1 == {8{1'b1}};\n\
  exp_tmp = {3'd0, exponent1} + 11'd896;\n\
  fout1[63] = fin1[31];\n\
  fout1[62:52] = is_exp_zero ? 11'd0 : (is_all_ones ? {11{1'b1}} : exp_tmp);\n\
  fout1[51:29] = fin1[22:0];\n\
  fout1[28:0] = 29'd0;\n\
  bits32_to_real64 = $bitstoreal(fout1);\n\
end\n\
endfunction\n\
// synthesis translate_on\n\
reg [BITSIZE_Mout_addr_ram-1:0] _present_pointer 1INIT_ZERO_VALUE;\n\
reg [BITSIZE_Mout_addr_ram-1:0] _next_pointer;\n\
reg [BITSIZE_Mout_addr_ram-1:0] _present_pointer1 1INIT_ZERO_VALUE;\n\
reg [BITSIZE_Mout_addr_ram-1:0] _next_pointer1;\n\
reg mem_sel_LOAD;\n\
wire mem_done_port;\n\
reg done_port;\n\
reg mem_start_port;\n\
wire [" + data_bus_bitsize + "-1:0] mem_out1;\n\
reg [" + addr_bus_bitsize + "-1:0] mem_in2;\n\
reg [" + size_bus_bitsize + "-1:0] mem_in3;\n\
  MEMORY_CTRL_P1N #(.BITSIZE_in1(" + data_bus_bitsize + "), .BITSIZE_in2(" + addr_bus_bitsize + "), .BITSIZE_in3(" + size_bus_bitsize + "), .BITSIZE_in4(1), .BITSIZE_out1(" + data_bus_bitsize + "), .BITSIZE_Min_oe_ram(BITSIZE_Min_oe_ram), .PORTSIZE_Min_oe_ram(PORTSIZE_Min_oe_ram), .BITSIZE_Min_we_ram(BITSIZE_Min_we_ram), .PORTSIZE_Min_we_ram(PORTSIZE_Min_we_ram), .BITSIZE_Mout_oe_ram(BITSIZE_Mout_oe_ram), .PORTSIZE_Mout_oe_ram(PORTSIZE_Mout_oe_ram), .BITSIZE_Mout_we_ram(BITSIZE_Mout_we_ram), .PORTSIZE_Mout_we_ram(PORTSIZE_Mout_we_ram), .BITSIZE_M_DataRdy(BITSIZE_M_DataRdy), .PORTSIZE_M_DataRdy(PORTSIZE_M_DataRdy), .BITSIZE_Min_addr_ram(BITSIZE_Min_addr_ram), .PORTSIZE_Min_addr_ram(PORTSIZE_Min_addr_ram), .BITSIZE_Mout_addr_ram(BITSIZE_Mout_addr_ram), .PORTSIZE_Mout_addr_ram(PORTSIZE_Mout_addr_ram), .BITSIZE_M_Rdata_ram(BITSIZE_M_Rdata_ram), .PORTSIZE_M_Rdata_ram(PORTSIZE_M_Rdata_ram), .BITSIZE_Min_Wdata_ram(BITSIZE_Min_Wdata_ram), .PORTSIZE_Min_Wdata_ram(PORTSIZE_Min_Wdata_ram), .BITSIZE_Mout_Wdata_ram(BITSIZE_Mout_Wdata_ram), .PORTSIZE_Mout_Wdata_ram(PORTSIZE_Mout_Wdata_ram), .BITSIZE_Min_data_ram_size(BITSIZE_Min_data_ram_size), .PORTSIZE_Min_data_ram_size(PORTSIZE_Min_data_ram_size), .BITSIZE_Mout_data_ram_size(BITSIZE_Mout_data_ram_size), .PORTSIZE_Mout_data_ram_size(PORTSIZE_Mout_data_ram_size), .BITSIZE_access_allowed(BITSIZE_access_allowed), .PORTSIZE_access_allowed(PORTSIZE_access_allowed), .BITSIZE_access_request(BITSIZE_access_request), .PORTSIZE_access_request(PORTSIZE_access_request)) MEMORY_CTRL_P1N_instance (.done_port(mem_done_port), .out1(mem_out1), .Mout_oe_ram(Mout_oe_ram), .Mout_we_ram(Mout_we_ram), .Mout_addr_ram(Mout_addr_ram), .Mout_Wdata_ram(Mout_Wdata_ram), .Mout_data_ram_size(Mout_data_ram_size), .access_request(access_request), .clock(clock), .start_port(mem_start_port), .in1(0), .in2(mem_in2), .in3(mem_in3), .in4(1), .sel_LOAD(mem_sel_LOAD), .sel_STORE(1'b0), .Min_oe_ram(Min_oe_ram), .Min_we_ram(Min_we_ram), .Min_addr_ram(Min_addr_ram), .M_Rdata_ram(M_Rdata_ram), .Min_Wdata_ram(Min_Wdata_ram), .Min_data_ram_size(Min_data_ram_size), .M_DataRdy(M_DataRdy), .access_allowed(access_allowed));\n\
\n\
parameter [2:0] S_0 = 3'd0,\n\
  S_1 = 3'd1,\n\
  S_2 = 3'd2,\n\
  S_3 = 3'd3,\n\
  S_4 = 3'd4,\n\
  S_5 = 3'd5,\n\
  S_6 = 3'd6,\n\
  S_7 = 3'd7;\n\
reg [2:0] _present_state 1INIT_ZERO_VALUE;\n\
reg [2:0] _next_state;\n\
reg ["+selector_left+":0] _present_selector 1INIT_ZERO_VALUE;\n\
reg ["+selector_left+":0] _next_selector;\n\
reg [63:0] data1;\n\
reg [7:0] _present_data2 1INIT_ZERO_VALUE;\n\
reg [7:0] _next_data2;\n\
reg [7:0] data1_size;\n\
reg write_done;\n\
\n\
  always @(posedge clock 1RESET_EDGE)\n\
    if (1RESET_VALUE)\n\
      begin\n\
        _present_state <= S_0;\n\
        _present_pointer <= {BITSIZE_Mout_addr_ram{1'b0}};\n\
        _present_pointer1 <= {BITSIZE_Mout_addr_ram{1'b0}};\n\
        _present_selector <="+selector_dimension+"'d0;\n\
        _present_data2 <= 8'b0;\n\
      end\n\
    else\n\
      begin\n\
        _present_state <= _next_state;\n\
        _present_pointer <= _next_pointer;\n\
        _present_pointer1 <= _next_pointer1;\n\
        _present_selector <= _next_selector;\n\
        _present_data2 <= _next_data2;\n\
      end\n\
\n\
  always @(_present_state or _present_pointer or _present_pointer1 or _present_selector or start_port or M_DataRdy[0] or Min_we_ram or Min_oe_ram or Min_Wdata_ram or Min_addr_ram or Min_data_ram_size" + sensitivity + " or _present_data2 or mem_done_port or M_Rdata_ram[7:0])\n\
      begin\n\
        _next_state = _present_state;\n\
        _next_pointer = _present_pointer;\n\
        _next_pointer1 = _present_pointer1;\n\
        _next_selector = _present_selector;\n\
        _next_data2 = _present_data2;\n\
        done_port = 1'b0;\n\
        mem_sel_LOAD = 1'b0;\n\
        mem_start_port = 1'b0;\n\
        mem_in2=" + addr_bus_bitsize + "'d0;\n\
        mem_in3=" + size_bus_bitsize + "'d0;\n\
        "+ case_statement + "\n\
        case (_present_state)\n\
          S_0:\n\
            if(start_port)\n\
              begin\n\
                _next_pointer=0;\n\
                _next_pointer1=0;\n\
                _next_state=S_1;  \n\
                _next_selector="+selector_dimension+"'d2;\n \
              end\n\
            \n\
         S_1:\n\
           begin\n\
             mem_in2 = in1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer;\n\
             mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n\
             mem_sel_LOAD=1'b1;\n\
             mem_start_port=1'b1;\n\
             if(mem_done_port)\n\
             begin\n\
                _next_data2 = mem_out1[7:0];\n\
                _next_state=S_2;\n\
// synthesis translate_off\n\
                write_done=1'b0;\n\
// synthesis translate_on\n\
             end\n\
           end\n\
         S_2:\n\
           begin\n\
             _next_pointer=_present_pointer+1'd1;\n\
             if((_present_data2!=8'd0)&&(_present_data2!=8'd37))\n\
             begin\n\
// synthesis translate_off\n\
               if(!write_done)\n\
               begin\n\
                 $write(\"%c\",_present_data2);\n\
               write_done=1'b1;\n\
               end\n\
// synthesis translate_on\n\
               _next_state=S_1;\n\
             end\n\
             else if(_present_data2==8'd37)\n\
               _next_state=S_3;\n\
             else if(_present_data2==8'd0)\n\
             begin\n\
               done_port = 1'b1;\n\
               _next_state=S_0;\n\
             end\n\
           end\n\
         S_3:\n\
           begin\n\
             mem_in2 = in1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer;\n\
             mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n\
             mem_sel_LOAD=1'b1;\n\
             if(mem_done_port)\n\
             begin\n\
                _next_data2 = mem_out1[7:0];\n\
                _next_state=S_5;\n\
// synthesis translate_off\n\
                write_done=1'b0;\n\
// synthesis translate_on\n\
             end\n\
           end\n\
         S_5 :\n\
           begin\n\
             _next_state=S_6;\n\
             _next_pointer=_present_pointer+1'd1;\n\
             case(_present_data2)\n\
               8'd37: //%%\n\
               begin\n\
                 _next_state=S_1;\n\
// synthesis translate_off\n\
                 if(!write_done)\n\
                 begin\n\
                   $write(\"%c\",8'd37);\n\
                   write_done=1'b1;\n\
                 end\n\
// synthesis translate_on\n\
               end\n\
               8'd99: //Char\n\
               begin\n\
// synthesis translate_off\n\
                 if(!write_done)\n\
                 begin\n\
                   $write(\"%c\",data1[7:0]);\n\
                   write_done=1'b1;\n\
                 end\n\
// synthesis translate_on\n\
               end\n\
               8'd100: //Decimal %d\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[15:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[7:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Decimal precision not supported (d) %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               8'd105: //Decimal %i\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[15:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$signed(data1[7:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Decimal precision not supported (i) %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               8'd101: //Exponential %e\n\
               begin\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%e\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%e\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd69: //Exponential %E\n\
               begin\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%e\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%e\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd102: //Float %f\n\
               begin\n\
                 if(data1_size==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20f\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20f\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd70: //Float %F\n\
               begin\n\
                 if(data1_size==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20f\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20f\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd103: //Float %g\n\
               begin\n\
                 if(data1_size==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20g\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20g\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd71: //Float %G\n\
               begin\n\
                 if(data1_size==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20g\",$bitstoreal(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%20.20g\",bits32_to_real64(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Floating point precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               end\n\
               8'd111: //Octal\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0o\",data1);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0o\",data1[31:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0o\",data1[15:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0o\",data1[7:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Octal precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               8'd112: //%p\n\
                 begin\n\
// synthesis translate_off\n\
                 if(!write_done)\n\
                 begin\n\
                   $write(\"0x%0h\",data1);\n\
                   write_done=1'b1;\n\
                 end\n\
// synthesis translate_on\n\
               end\n\
               8'd115: //String\n\
               begin\n\
                 _next_state=S_7;\n\
                 _next_pointer1=0;\n\
               end\n\
               8'd117: //unsigned int %u TO BE FIXED\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$unsigned(data1));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$unsigned(data1[31:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$unsigned(data1[15:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0d\",$unsigned(data1[7:0]));\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Unsigned precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               8'd120: //Hex %x\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[31:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[15:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[7:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Hex precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               8'd88: //Hex %X\n\
                 if(data1_size ==8'd64)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd32)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[31:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd16)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[15:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else if(data1_size ==8'd8)\n\
                 begin\n\
// synthesis translate_off\n\
                   if(!write_done)\n\
                   begin\n\
                     $write(\"%0h\",data1[7:0]);\n\
                     write_done=1'b1;\n\
                   end\n\
// synthesis translate_on\n\
                 end\n\
                 else\n\
                 begin\n\
// synthesis translate_off\n\
                   $display(\"ERROR - Hex precision not supported %d\", data1_size);\n\
                   $finish;\n\
// synthesis translate_on\n\
                 end\n\
               default:\n\
                 _next_state=S_3;\n\
             endcase\n\
           end\n\
         S_6:\n\
           begin\n\
             _next_selector=_present_selector<<1;\n\
             _next_state=S_1;\n\
           end\n\
         S_7:\n\
           begin\n\
             mem_in2 = data1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer1;\n\
             mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n\
             mem_sel_LOAD=1'b1;\n\
             if(mem_done_port)\n\
             begin\n\
               _next_data2 = mem_out1[7:0];\n\
               _next_state=S_4;\n\
// synthesis translate_off\n\
               write_done=1'b0;\n\
// synthesis translate_on\n\
             end\n\
           end\n\
         S_4:\n\
           begin\n\
             _next_pointer1=_present_pointer1+1'd1;\n\
             if(_present_data2!=8'd0)\n\
             begin\n\
// synthesis translate_off\n\
               if(!write_done)\n\
               begin\n\
                 $write(\"%c\",_present_data2);\n\
                 write_done=1'b1;\n\
               end\n\
// synthesis translate_on\n\
               _next_state=S_7;\n\
             end\n\
             else\n\
               _next_state=S_6;\n\
           end\n\
      endcase\n \
  end\n";

std::cout << fsm;

