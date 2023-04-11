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
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2022-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file OpenCSModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "OpenCSModuleGenerator.hpp"

#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "memory.hpp"
#include <fcntl.h>

OpenCSModuleGenerator::OpenCSModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void OpenCSModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */, unsigned int /* function_id */,
                                         vertex /* op_v */, const HDLWriter_Language /* language */,
                                         const std::vector<ModuleGenerator::parameter>& _p,
                                         const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                         const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                         const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto data_bus_bitsize = STR(HLSMgr->Rmem->get_bus_data_bitsize());
   const auto addr_bus_bitsize = STR(HLSMgr->get_address_bitsize());
   const auto size_bus_bitsize = STR(HLSMgr->Rmem->get_bus_size_bitsize());

   out << " // verilator lint_off LITENDIAN\n";
   out << "parameter MAX_BUFF_SIZE = 256;\n";
   out << "reg [0:8*MAX_BUFF_SIZE-1] buffer_name;\n";
   out << "\n";
   out << "  `ifndef _SIM_HAVE_CLOG2\n";
   out << "    function integer log2;\n";
   out << "       input integer value;\n";
   out << "       integer temp_value;\n";
   out << "      begin\n";
   out << "        temp_value = value-1;\n";
   out << "        for (log2=0; temp_value>0; log2=log2+1)\n";
   out << "          temp_value = temp_value>>1;\n";
   out << "      end\n";
   out << "    endfunction\n";
   out << "  `endif\n";
   out << "\n";
   out << "  `ifdef _SIM_HAVE_CLOG2\n";
   out << "    parameter nbits_buffer = $clog2(MAX_BUFF_SIZE);\n";
   out << "  `else\n";
   out << "    parameter nbits_buffer = log2(MAX_BUFF_SIZE);\n";
   out << "  `endif\n";

   std::string sensitivity;
   for(auto i = 0U; i < _p.size(); i++)
   {
      sensitivity += " or " + _p[i].name;
   }

   std::string modes = "in2";

   std::string flags_string = "(" + modes + " & " + STR(O_RDWR) + ") != 0 && (" + modes + " & " + STR(O_APPEND) +
                              ") ? \"a+b\" : ((" + modes + " & " + STR(O_RDWR) + ") != 0 ? \"r+b\" : ((" + modes +
                              " & " + STR(O_WRONLY) + ") != 0 && (" + modes + " & " + STR(O_APPEND) + ") ? \"ab\" : (" +
                              modes + " & " + STR(O_WRONLY) + ") != 0 ? \"wb\" : \"rb\"" + "))";

   const auto fsm =
       "  reg [nbits_buffer-1:0] _present_index 1INIT_ZERO_VALUE;\n"
       "  reg [nbits_buffer-1:0] _next_index;\n"
       "  reg [BITSIZE_Mout_addr_ram-1:0] _present_pointer 1INIT_ZERO_VALUE;\n"
       "  reg [BITSIZE_Mout_addr_ram-1:0] _next_pointer;\n"
       "  reg done_port;\n"
       "  wire mem_done_port;\n"
       "  reg signed [BITSIZE_out1-1:0] temp_out1;\n"
       "  \n"
       "  parameter [2:0] S_0 = 3'd0,\n"
       "                  S_1 = 3'd1,\n"
       "                  S_2 = 3'd2,\n"
       "                  S_3 = 3'd3,\n"
       "                  S_4 = 3'd4;\n"
       "  reg [2:0] _present_state 1INIT_ZERO_VALUE;\n"
       "  reg [2:0] _next_state;\n"
       "  reg [63:0] data1;\n"
       "  reg [7:0] data1_size;\n"
       "  wire [" +
       data_bus_bitsize +
       "-1:0] mem_out1;\n"
       "  reg [" +
       addr_bus_bitsize +
       "-1:0] mem_in2;\n"
       "  reg [" +
       size_bus_bitsize +
       "-1:0] mem_in3;\n"
       "  reg mem_sel_LOAD;\n"
       "  reg start_memory_op;\n"
       "  mem_ctrl_kernel #(.TAG_MEM_REQ(0), .BITSIZE_in1(" +
       data_bus_bitsize + "), .BITSIZE_in2(" + addr_bus_bitsize + "), .BITSIZE_in3(" + size_bus_bitsize +
       "), .BITSIZE_out1(" + data_bus_bitsize +
       "), .BITSIZE_Mout_oe_ram(BITSIZE_Mout_oe_ram), .BITSIZE_Mout_we_ram(BITSIZE_Mout_we_ram), "
       ".BITSIZE_Mout_addr_ram(BITSIZE_Mout_addr_ram), .BITSIZE_Mout_data_ram_size(BITSIZE_Mout_data_ram_size), "
       ".BITSIZE_Mout_Wdata_ram(BITSIZE_Mout_Wdata_ram), .BITSIZE_Mout_tag_ram(BITSIZE_Mout_tag_ram), "
       ".BITSIZE_M_Rdata_ram(BITSIZE_M_Rdata_ram), .BITSIZE_M_DataRdy(BITSIZE_M_DataRdy), "
       ".BITSIZE_Min_tag(BITSIZE_Min_tag), .BITSIZE_request_accepted(BITSIZE_request_accepted)) "
       "mem_ctrl_kernel_instance (.clock(clock), .in1(0), .in2(mem_in2), .in3(mem_in3), .sel_LOAD(mem_sel_LOAD), "
       ".sel_STORE(1'b0), .done_port(mem_done_port), .out1(mem_out1), .Mout_oe_ram(Mout_oe_ram), "
       ".Mout_we_ram(Mout_we_ram), .Mout_addr_ram(Mout_addr_ram), .Mout_data_ram_size(Mout_data_ram_size), "
       ".Mout_Wdata_ram(Mout_Wdata_ram), .Mout_tag_ram(Mout_tag_ram), .M_Rdata_ram(M_Rdata_ram), "
       ".M_DataRdy(M_DataRdy), .Min_tag(Min_tag), .request_accepted(request_accepted), .start_port(start_memory_op));\n"
       "  \n"
       "  \n"
       "  always @(posedge clock 1RESET_EDGE)\n"
       "    if (1RESET_VALUE)\n"
       "      begin\n"
       "        _present_state <= S_0;\n"
       "        _present_pointer <= {BITSIZE_Mout_addr_ram{1'b0}};\n"
       "        _present_index <= {nbits_buffer{1'b0}};\n"
       "      end\n"
       "    else\n"
       "      begin\n"
       "        _present_state <= _next_state;\n"
       "        _present_pointer <= _next_pointer;\n"
       "        _present_index <= _next_index;\n"
       "      end\n"
       "  \n"
       "  assign out1 = {1'b0,temp_out1[30:0]};"
       "  always @(_present_state or _present_pointer or _present_index or start_port or mem_done_port" +
       sensitivity +
       " or mem_out1 or M_DataRdy)\n"
       "      begin\n"
       "        done_port = 1'b0;\n"
       "        _next_state = _present_state;\n"
       "        _next_pointer = _present_pointer;\n"
       "        _next_index = _present_index;\n"
       "        mem_sel_LOAD = 1'b0;\n"
       "        start_memory_op= 1'b0;\n"
       "        mem_in2=" +
       addr_bus_bitsize +
       "'d0;\n"
       "        mem_in3=" +
       size_bus_bitsize +
       "'d0;\n"
       "        case (_present_state)\n"
       "          S_0:\n"
       "            if(start_port)\n"
       "              begin\n"
       "                _next_pointer=0;\n"
       "                _next_index={nbits_buffer{1'b0}};\n"
       "                _next_state=S_1;  \n"
       "                buffer_name=0;  \n"
       "              end\n"
       "            \n"
       "         S_1:\n"
       "           begin\n"
       "             mem_in2 = in1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer;\n"
       "             mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n"
       "             mem_sel_LOAD=1'b1;\n"
       "             start_memory_op= 1'b1;\n"
       "             if(mem_done_port)\n"
       "             begin\n"
       "                buffer_name[_present_index*8 +:8] = mem_out1[7:0];\n"
       "                if(mem_out1[7:0] == 8'd0)\n"
       "                  _next_state=S_2;\n"
       "                else\n"
       "                  _next_state=S_3;\n"
       "             end\n"
       "             else begin\n"
       "               _next_state=S_4;\n"
       "             end\n"
       "           end\n"
       "         S_4:\n"
       "         begin\n"
       "           mem_in2 = in1[BITSIZE_Mout_addr_ram-1:0]+_present_pointer;\n"
       "           mem_in3 = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8};\n"
       "           mem_sel_LOAD=1'b1;\n"
       "           if(mem_done_port)\n"
       "           begin\n"
       "              buffer_name[_present_index*8 +:8] = mem_out1[7:0];\n"
       "              if(mem_out1[7:0] == 8'd0)\n"
       "                _next_state=S_2;\n"
       "              else\n"
       "                _next_state=S_3;\n"
       "           end\n"
       "         end\n"
       "         S_2:\n"
       "           begin\n"
       "// synthesis translate_off\n"
       "             temp_out1 = $fopen(buffer_name, " +
       flags_string +
       ");\n"
       "// synthesis translate_on\n"
       "             done_port = 1'b1;\n"
       "             _next_state=S_0;\n"
       "           end\n"
       "         S_3:\n"
       "           begin\n"
       "             if(!mem_done_port)\n"
       "             begin\n"
       "              _next_pointer=_present_pointer+1'd1;\n"
       "              _next_index=_present_index+1'd1;\n"
       "              _next_state=S_1;\n"
       "             end\n"
       "           end\n"
       "      endcase\n "
       "  end\n";

   out << fsm;
   out << " // verilator lint_on LITENDIAN\n";
}