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
 *              Copyright (C) 2022-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file BuiltinWaitCallNHDLGenerator.cpp
 * @brief Implementation of the built-in wait call module with an explicit wait count.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#include "BuiltinWaitCallNHDLGenerator.hpp"

#include "BambuParameter.hpp"
#include "application_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "memory_allocation.hpp"
#include "op_graph.hpp"
#include "structural_objects.hpp"

BuiltinWaitCallNHDLGenerator::BuiltinWaitCallNHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void BuiltinWaitCallNHDLGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */,
                                                unsigned int function_id, OpGraph::vertex_descriptor op_v,
                                                const HDLWriter_Language /* language */,
                                                const std::vector<HDLGenerator::parameter>& _p,
                                                const std::vector<HDLGenerator::parameter>& /* _ports_in */,
                                                const std::vector<HDLGenerator::parameter>& /* _ports_out */,
                                                const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   const auto retval_size = [&]() {
      THROW_ASSERT(function_id && op_v, "");
      const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
      const auto TM = HLSMgr->get_ir_manager();
      const auto call_tn = TM->GetIRNode(FB->GetOpGraph(FunctionBehavior::CFG).CGetNodeInfo(op_v).GetNodeId());
      THROW_ASSERT(call_tn && call_tn->get_kind() == call_stmt_K, "Expected call_stmt.");
      const auto gc = GetPointerS<const call_stmt>(call_tn);
      THROW_ASSERT(gc->args.size() >= 2, "Expected at least two arguments for the builtin wait call.");
      const auto called_addr = gc->args.at(0);
      const auto called_hasreturn = gc->args.at(1);
      THROW_ASSERT(called_hasreturn->get_kind() == constant_int_val_node_K, "");
      if(ir_helper::GetConstValue(called_hasreturn))
      {
         const auto fpointer_type = ir_helper::CGetType(called_addr);
         const auto called_ftype = ir_helper::CGetPointedType(fpointer_type);
         const auto return_type = ir_helper::GetFunctionReturnType(called_ftype);
         if(return_type)
         {
            return ir_helper::SizeAlloc(return_type);
         }
      }
      return 0ULL;
   }();

   const auto Param = HLSMgr->get_parameter();
   const auto emulate_bram =
       Param->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
       Param->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
           MemoryAllocation_Policy::EXT_PIPELINED_BRAM;

   // Signals declarations
   if(_p.size() == 3U)
   {
      out << "reg [0:0] index;\n\n";
   }
   else if(_p.size() > 3U)
   {
      out << "reg [" << ceil_log2(_p.size() - 2U) << "-1:0] index;\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "wire [BITSIZE_Mout_addr_ram-1:0] paramAddressRead;\n\n";
   }

   out << "reg [31:0] step;\n"
       << "reg [31:0] next_step;\n"
       << "reg done_port;\n"
       << "reg [PORTSIZE_Sout_DataRdy-1:0] Sout_DataRdy;\n"
       << "reg [PORTSIZE_Mout_oe_ram-1:0] Mout_oe_ram;\n"
       << "reg [PORTSIZE_Mout_we_ram-1:0] Mout_we_ram;\n"
       << "reg [PORTSIZE_Mout_addr_ram*BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram;\n"
       << "reg [PORTSIZE_Mout_Wdata_ram*BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram;\n"
       << "reg [PORTSIZE_Mout_data_ram_size*BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size;\n"
       << "reg active_request;\n"
       << "reg active_request_next;\n\n";

   if(retval_size)
   {
      out << "reg [" << retval_size << "-1:0] readValue 1INIT_ZERO_VALUE;\n"
          << "reg [" << retval_size << "-1:0] next_readValue;\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "reg [BITSIZE_Mout_addr_ram-1:0] paramAddress [" << (_p.size() - 2U) << "-1:0];\n\n";
   }

   out << "function [PORTSIZE_S_addr_ram-1:0] check_condition;\n"
       << "  input [PORTSIZE_S_addr_ram*BITSIZE_S_addr_ram-1:0] m;\n"
       << "  integer i1;\n"
       << "  begin\n"
       << "    for(i1 = 0; i1 < PORTSIZE_S_addr_ram; i1 = i1 + 1)\n"
       << "    begin\n"
       << "      check_condition[i1] = m[i1*BITSIZE_S_addr_ram +:BITSIZE_S_addr_ram] == unlock_address;\n"
       << "    end\n"
       << "  end\n"
       << "endfunction\n";

   out << "wire [PORTSIZE_S_addr_ram-1:0] internal;\n";

   out << "parameter [31:0] ";
   const auto n_iterations = retval_size ? (_p.size() + 3U) : _p.size();
   for(auto idx = 0U; idx <= n_iterations; ++idx)
   {
      if(idx != n_iterations)
      {
         out << "S_" << idx << " = 32'd" << idx << ",\n";
      }
      else
      {
         out << "S_" << idx << " = 32'd" << idx << ";\n";
      }
   }

   if(_p.size() > 2U)
   {
      out << "initial\n"
          << "   begin\n"
          << "     $readmemb(MEMORY_INIT_file, paramAddress, 0, " << (_p.size() - 2U) << "-1);\n"
          << "   end\n\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "assign paramAddressRead = paramAddress[index];\n";
   }
   out << "assign Sout_Rdata_ram = Sin_Rdata_ram;\n"
       << "assign internal = check_condition(S_addr_ram);\n";

   // State machine
   out << "always @ (posedge clock 1RESET_EDGE)\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    step <= 0;\n";

   if(retval_size)
   {
      if(retval_size == 1U)
      {
         out << "    readValue <= {1'b0};\n";
      }
      else
      {
         out << "    readValue <= {" << retval_size << " {1'b0}};\n";
      }
      out << "  end else begin\n"
          << "    step <= next_step;\n"
          << "    readValue <= next_readValue;\n"
          << "  end\n\n";
   }
   else
   {
      out << "  end else begin\n"
          << "    step <= next_step;\n"
          << "  end\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "always @(*)\n"
          << "  begin\n"
          << "    index = 0;\n"
          << "    if (step == S_0) begin\n"
          << "        index = 0;\n"
          << "    end\n";
   }

   auto idx = 1U;
   if(_p.size() > 3)
   {
      for(idx = 1U; idx <= (_p.size() - 3U); ++idx)
      {
         out << "     else if (step == S_" << idx << ") begin\n"
             << "       index = " << (idx - 1U) << ";\n"
             << "     end\n";
      }
   }
   if(_p.size() > 2U)
   {
      out << "    else if (step == S_" << idx << ") begin\n"
          << "      index = " << (idx - 1U) << ";\n"
          << "    end\n";
      idx++;
   }

   idx++;

   idx++;

   if(_p.size() > 2U && retval_size)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "    index = " << (idx - 4U) << ";\n"
          << "  end\n";
      idx++;
   }
   if(_p.size() > 2U)
   {
      out << "end\n";
   }

   if(emulate_bram)
   {
      out << "reg Mout_oe_ram_reg;\n"
          << "always @ (posedge clock 1RESET_EDGE)\n"
          << "  if (1RESET_VALUE)\n"
          << "  begin\n"
          << "    Mout_oe_ram_reg <= 0;\n"
          << "  end else begin\n"
          << "    Mout_oe_ram_reg <= Mout_oe_ram;\n"
          << "  end\n";
   }

   out << "always @ (posedge clock 1RESET_EDGE)\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    active_request <= 0;\n"
       << "  end\n"
       << "  else\n"
       << "  begin\n"
       << "    active_request <= active_request_next;\n"
       << "  end\n\n";

   out << "always @(*)\n"
       << "  begin\n"
       << "  Sout_DataRdy = Sin_DataRdy;\n"
       << "  done_port = 1'b0;\n"
       << "  next_step = S_0;\n"
       << (retval_size ? "  next_readValue = readValue;\n" : "") << "  Mout_we_ram = Min_we_ram;\n"
       << "  Mout_Wdata_ram = Min_Wdata_ram;\n"
       << "  Mout_oe_ram = Min_oe_ram;\n"
       << "  Mout_addr_ram = Min_addr_ram;\n"
       << "  Mout_data_ram_size = Min_data_ram_size;\n"
       << "  active_request_next = 0;\n";

   out << "  if (step == S_0) begin\n"
       << "    if (start_port == 1'b1) begin\n"
       << "      active_request_next = 1;\n";
   if(_p.size() == 3U)
   {
      out << "      next_step = in2[0] ? S_2 : S_1;\n";
   }
   else
   {
      out << "      next_step = S_1;\n";
   }
   out << "    end else begin\n"
       << "      next_step = S_0;\n"
       << "    end\n"
       << "  end\n";
   idx = 1U;
   if(_p.size() > 3)
   {
      for(idx = 1U; idx <= (_p.size() - 3U); ++idx)
      {
         if(idx != (_p.size() - 3U))
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = active_request;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = (in1 + paramAddressRead) & "
                   "{BITSIZE_Mout_addr_ram{active_request}};\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name
                << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1].type_size
                << " & {BITSIZE_Mout_data_ram_size{active_request}};\n";
            if(emulate_bram)
            {
               out << "      next_step = S_" << (idx + 1U) << ";\n"
                   << "      active_request_next = 1;\n";
            }
            else
            {
               out << "    if (M_DataRdy[0] == 1'b1) begin\n"
                   << "      next_step = S_" << (idx + 1U) << ";\n"
                   << "      active_request_next = 1;\n"
                   << "    end else begin\n"
                   << "      next_step = S_" << idx << ";\n"
                   << "    end\n";
            }
            out << "  end\n";
         }
         else
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = active_request;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = (in1 + paramAddressRead) & "
                   "{BITSIZE_Mout_addr_ram{active_request}};\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name
                << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1].type_size
                << " & {BITSIZE_Mout_data_ram_size{active_request}};\n";
            if(emulate_bram)
            {
               out << "      next_step = in2[0] ? S_" << (idx + 2U) << " : S_" << (idx + 1U) << ";\n"
                   << "      active_request_next = 1;\n";
            }
            else
            {
               out << "    if (M_DataRdy[0] == 1'b1) begin\n"
                   << "      next_step = in2[0] ? S_" << (idx + 2U) << " : S_" << (idx + 1U) << ";\n"
                   << "      active_request_next = 1;\n"
                   << "    end else begin\n"
                   << "      next_step = S_" << idx << ";\n"
                   << "    end\n";
            }
            out << "  end\n";
         }
      }
   }
   if(_p.size() > 2U)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "     Mout_we_ram[0] = active_request;\n"
          << "     Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = (in1 + paramAddressRead) & "
             "{BITSIZE_Mout_addr_ram{active_request}};\n"
          << "     Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name
          << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
          << "     Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1].type_size
          << " & {BITSIZE_Mout_data_ram_size{active_request}};\n";
      if(emulate_bram)
      {
         out << "     next_step = S_" << (idx + 1U) << ";\n"
             << "     active_request_next = 1;\n";
      }
      else
      {
         out << "   if (M_DataRdy[0] == 1'b1) begin\n"
             << "     next_step = S_" << (idx + 1U) << ";\n"
             << "     active_request_next = 1;\n"
             << "   end else begin\n"
             << "     next_step = S_" << idx << ";\n"
             << "   end\n";
      }
      out << "  end\n";
      idx++;
   }

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    Mout_we_ram[0] = active_request;\n"
       << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 & {BITSIZE_Mout_addr_ram{active_request}};\n"
       << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = unlock_address & "
          "{BITSIZE_Mout_Wdata_ram{active_request}};\n"
       << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = BITSIZE_Mout_Wdata_ram & "
          "{BITSIZE_Mout_data_ram_size{active_request}};\n";
   if(emulate_bram)
   {
      out << "      next_step = S_" << (idx + 1U) << ";\n"
          << "      active_request_next = 1;\n";
   }
   else
   {
      out << "    if (M_DataRdy[0] == 1'b1) begin\n"
          << "      next_step = S_" << (idx + 1U) << ";\n"
          << "      active_request_next = 1;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end";
   }
   out << "  end\n";
   idx++;

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    if (|(S_we_ram & internal)) begin\n"
       << "      Sout_DataRdy = (S_we_ram & internal) | Sin_DataRdy;\n"
       << "      next_step = in2[0] ? S_" << (retval_size ? (idx + 1U) : 0U) << " : S_0;\n"
       << "      active_request_next = 1;\n"
       << "      done_port = in2[0] ? 1'b0 : 1'b1;\n"
       << "    end else begin\n"
       << "      next_step = S_" << idx << ";\n"
       << "    end\n"
       << "  end\n";
   idx++;

   if(_p.size() > 2U && retval_size)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "      Mout_oe_ram[0] = active_request;\n"
          << "      Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = (in1 + paramAddressRead) & "
             "{BITSIZE_Mout_addr_ram{active_request}};\n"
          << "      Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << retval_size
          << " & {BITSIZE_Mout_data_ram_size{active_request}};\n";
      if(emulate_bram)
      {
         out << "    if (Mout_oe_ram_reg) begin\n";
      }
      else
      {
         out << "    if (M_DataRdy[0] == 1'b1) begin\n";
      }
      out << "      next_step = S_" << (idx + 1U) << ";\n"
          << "      active_request_next = 1;\n"
          << "      next_readValue = M_Rdata_ram;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
      idx++;

      out << "  else if (step == S_" << idx << ") begin\n"
          << "    Mout_we_ram[0] = active_request;\n"
          << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = " << _p[_p.size() - 1U].name
          << " & {BITSIZE_Mout_addr_ram{active_request}};\n"
          << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = readValue & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
          << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << retval_size
          << " & {BITSIZE_Mout_data_ram_size{active_request}};\n";
      if(emulate_bram)
      {
         out << "      next_step = S_0;\n"
             << "      active_request_next = 1;\n"
             << "      done_port = 1'b1;\n";
      }
      else
      {
         out << "    if (M_DataRdy[0] == 1'b1) begin\n"
             << "      next_step = S_0;\n"
             << "      active_request_next = 1;\n"
             << "      done_port = 1'b1;\n"
             << "    end else begin\n"
             << "      next_step = S_" << idx << ";\n"
             << "    end";
      }

      out << "  end\n";
   }
   out << "end\n";
}
