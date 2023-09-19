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
 * @file OpenNModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "OpenNModuleGenerator.hpp"

#include "language_writer.hpp"
#include <fcntl.h>

OpenNModuleGenerator::OpenNModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void OpenNModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */,
                                        unsigned int /* function_id */, vertex /* op_v */,
                                        const HDLWriter_Language /* language */,
                                        const std::vector<ModuleGenerator::parameter>& /* _p */,
                                        const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                        const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                        const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   out << " // verilator lint_off LITENDIAN\n";
   out << "parameter MAX_BUFF_SIZE = 256;\n";
   out << "reg [0:8*MAX_BUFF_SIZE-1] buffer_name;\n";
   out << "reg [0:8*MAX_BUFF_SIZE-1] buffer_name_old;\n";
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

   std::string modes = "in2";

   std::string flags_string = "(" + modes + " & " + STR(O_RDWR) + ") != 0 && (" + modes + " & " + STR(O_APPEND) +
                              ") ? \"a+b\" : ((" + modes + " & " + STR(O_RDWR) + ") != 0 ? \"r+b\" : ((" + modes +
                              " & " + STR(O_WRONLY) + ") != 0 && (" + modes + " & " + STR(O_APPEND) + ") ? \"ab\" : (" +
                              modes + " & " + STR(O_WRONLY) + ") != 0 ? \"wb\" : \"rb\"" + "))";

   const auto fsm =
       "reg [nbits_buffer-1:0] _present_index;\n"
       "  reg [nbits_buffer-1:0] _next_index;\n"
       "  reg [BITSIZE_Mout_addr_ram-1:0] _present_pointer;\n"
       "  reg [BITSIZE_Mout_addr_ram-1:0] _next_pointer;\n"
       "  reg done_port;\n"
       "  reg signed [BITSIZE_out1-1:0] temp_out1;\n"
       "  reg [PORTSIZE_Mout_oe_ram-1:0] Mout_oe_ram;\n"
       "  reg [PORTSIZE_Mout_we_ram-1:0] Mout_we_ram;\n"
       "  reg [(PORTSIZE_Mout_addr_ram*BITSIZE_Mout_addr_ram)+(-1):0] Mout_addr_ram;\n"
       "  reg [(PORTSIZE_Mout_Wdata_ram*BITSIZE_Mout_Wdata_ram)+(-1):0] Mout_Wdata_ram;\n"
       "  reg [(PORTSIZE_Mout_data_ram_size*BITSIZE_Mout_data_ram_size)+(-1):0] Mout_data_ram_size;\n"
       "  reg active_request;\n"
       "  reg active_request_next;\n"
       "  reg active_request_now;\n"
       "  reg M_DataRdy_reg;\n"
       "  reg [BITSIZE_M_Rdata_ram-1:0]M_Rdata_ram_reg;\n"
       "  \n"
       "  parameter [1:0] S_0 = 2'd0,\n"
       "                  S_1 = 2'd1,\n"
       "                  S_2 = 2'd2;\n"
       "  reg [1:0] _present_state 1INIT_ZERO_VALUE;\n"
       "  reg [1:0] _next_state;\n"
       "  reg [63:0] data1;\n"
       "  reg [7:0] data1_size;\n"
       "  \n"
       "  always @(posedge clock 1RESET_EDGE)\n"
       "    if (1RESET_VALUE)\n"
       "      begin\n"
       "        _present_state <= S_0;\n"
       "        _present_pointer <= {BITSIZE_Mout_addr_ram{1'b0}};\n"
       "        _present_index <= {nbits_buffer{1'b0}};\n"
       "        buffer_name_old <= {8*MAX_BUFF_SIZE{1'b0}};\n"
       "        M_DataRdy_reg <= 0;\n"
       "        M_Rdata_ram_reg <= 0;\n"
       "      end\n"
       "    else\n"
       "      begin\n"
       "        _present_state <= _next_state;\n"
       "        _present_pointer <= _next_pointer;\n"
       "        _present_index <= _next_index;\n"
       "        buffer_name_old <= buffer_name;\n"
       "        M_DataRdy_reg <= M_DataRdy;\n"
       "        M_Rdata_ram_reg <= M_Rdata_ram;\n"
       "      end\n"
       "  \n"
       "  assign out1 = {1'b0,temp_out1[30:0]};\n"
       "  always @(posedge clock 1RESET_EDGE)\n"
       "  begin\n"
       "    if (1RESET_VALUE)\n"
       "    begin\n"
       "      active_request <= 0;\n"
       "    end\n"
       "    else\n"
       "    begin\n"
       "      active_request <= active_request_next;\n"
       "    end\n"
       "  end\n"
       "  always @(*)\n"
       "      begin\n"
       "        Mout_we_ram = Min_we_ram;\n"
       "        Mout_Wdata_ram = Min_Wdata_ram;\n"
       "        Mout_oe_ram = Min_oe_ram;\n"
       "        Mout_addr_ram = Min_addr_ram;\n"
       "        Mout_data_ram_size = Min_data_ram_size;\n"
       "        Mout_oe_ram = Min_oe_ram;\n"
       "        Mout_addr_ram = Min_addr_ram;\n"
       "        Mout_data_ram_size = Min_data_ram_size;\n"
       "        done_port = 1'b0;\n"
       "        _next_state = _present_state;\n"
       "        _next_pointer = _present_pointer;\n"
       "        _next_index = _present_index;\n"
       "        active_request_next = 1'b0;\n"
       "        active_request_now = 1'b0;\n"
       "        buffer_name = buffer_name_old;\n"
       "        case (_present_state)\n"
       "          S_0:\n"
       "            if(start_port)\n"
       "              begin\n"
       "                _next_pointer=0;\n"
       "                _next_index={nbits_buffer{1'b0}};\n"
       "                _next_state=S_1;  \n"
       "                active_request_next = 1'b1;\n"
       "                active_request_next = 1'b1;\n"
       "                buffer_name=0;  \n"
       "              end\n"
       "         S_1:\n"
       "           begin\n"
       "             if(M_DataRdy_reg)\n"
       "             begin\n"
       "                buffer_name[_present_index*8 +:8] = M_Rdata_ram_reg[7:0];\n"
       "                if(M_Rdata_ram_reg[7:0] == 8'd0)\n"
       "                  _next_state=S_2;\n"
       "                else\n"
       "                begin\n"
       "                  _next_state=S_1;\n"
       "                  _next_pointer = _present_pointer + 1'd1;\n"
       "                  _next_index = _present_index + 1'd1;\n"
       "                  active_request_now = 1'b1;\n"
       "                end\n"
       "             end\n"
       "             Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = (in1[BITSIZE_Mout_addr_ram-1:0]+_next_pointer) & "
       "{BITSIZE_Mout_addr_ram{active_request || active_request_now}};\n"
       "             Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = {{BITSIZE_Mout_data_ram_size-4{1'b0}}, 4'd8} "
       "& "
       "{BITSIZE_Mout_data_ram_size{active_request || active_request_now}};\n"
       "             Mout_oe_ram[0] = active_request || active_request_now;\n"
       "           end\n"
       "         S_2:\n"
       "           begin\n"
       "// synthesis translate_off\n"
       "             temp_out1 = $fopen(buffer_name, " +
       flags_string +
       ");\n"
       "// synthesis translate_on\n"
       "             done_port = 1'b1;\n"
       "             _next_state = S_0;\n"
       "           end\n"
       "      endcase\n"
       "  end\n";

   out << fsm;
   out << " // verilator lint_on LITENDIAN\n";
}
