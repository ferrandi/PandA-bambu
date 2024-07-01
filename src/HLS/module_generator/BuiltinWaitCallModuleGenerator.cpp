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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
 * @file BuiltinWaitCallModuleGenerator.cpp
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

#include "BuiltinWaitCallModuleGenerator.hpp"

#include "application_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "op_graph.hpp"
#include "structural_objects.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

BuiltinWaitCallModuleGenerator::BuiltinWaitCallModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void BuiltinWaitCallModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */,
                                                  unsigned int function_id, vertex op_v,
                                                  const HDLWriter_Language /* language */,
                                                  const std::vector<ModuleGenerator::parameter>& _p,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto retval_size = [&]() {
      THROW_ASSERT(function_id && op_v, "");
      const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
      const auto TM = HLSMgr->get_tree_manager();
      const auto call_stmt = TM->GetTreeNode(FB->CGetOpGraph(FunctionBehavior::CFG)->CGetOpNodeInfo(op_v)->GetNodeId());
      THROW_ASSERT(call_stmt && call_stmt->get_kind() == gimple_call_K, "Expected gimple call statement.");
      const auto gc = GetPointerS<const gimple_call>(call_stmt);
      THROW_ASSERT(gc->args.size() >= 2, "Expected at least two arguments for the builtin wait call.");
      const auto called_addr = gc->args.at(0);
      const auto called_hasreturn = gc->args.at(1);
      THROW_ASSERT(called_hasreturn->get_kind() == integer_cst_K, "");
      if(tree_helper::GetConstValue(called_hasreturn))
      {
         const auto fpointer_type = tree_helper::CGetType(called_addr);
         const auto called_ftype = tree_helper::CGetPointedType(fpointer_type);
         const auto return_type = tree_helper::GetFunctionReturnType(called_ftype);
         if(return_type)
         {
            return tree_helper::SizeAlloc(return_type);
         }
      }
      return 0ULL;
   }();

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
       << "reg Sout_DataRdy;\n"
       << "reg Mout_oe_ram;\n"
       << "reg Mout_we_ram;\n"
       << "reg [BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram;\n"
       << "reg [BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram;\n"
       << "reg [BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size;\n"
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

   const auto n_iterations = retval_size ? (_p.size() + 3U) : _p.size();

   out << "parameter [31:0] ";
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
   out << "assign Sout_Rdata_ram = Sin_Rdata_ram;\n";

   out << "always @ (posedge clock 1RESET_EDGE)\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    active_request <= 0;\n"
       << "  end\n"
       << "  else\n"
       << "  begin\n"
       << "    active_request <= active_request_next;\n"
       << "  end\n";

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
   if(_p.size() > 3U)
   {
      for(idx = 1U; idx <= _p.size() - 3U; ++idx)
      {
         out << "     else if (step == S_" << idx << ") begin\n"
             << "       index = " << idx - 1U << ";\n"
             << "     end\n";
      }
   }

   if(_p.size() > 2U)
   {
      out << "    else if (step == S_" << idx << ") begin\n"
          << "      index = " << idx - 1U << ";\n"
          << "    end\n";
      idx++;
   }

   idx++;

   idx++;

   if(_p.size() > 2U && retval_size)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "    index = " << idx - 4U << ";\n"
          << "  end\n";
      idx++;
   }
   if(_p.size() > 2U)
   {
      out << "end\n";
   }

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
       << "  active_request_next = 0;\n"
       << "  if (step == S_0) begin\n"
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

   if(_p.size() > 3U)
   {
      for(idx = 1U; idx <= _p.size() - 3U; ++idx)
      {
         if(idx != _p.size() - 3U)
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram = active_request;\n"
                << "    Mout_addr_ram = (in1 + paramAddressRead) & {BITSIZE_Mout_addr_ram{active_request}};\n"
                << "    Mout_Wdata_ram = " << _p[idx + 1].name << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
                << "    Mout_data_ram_size = " << _p[idx + 1].type_size
                << " & {BITSIZE_Mout_data_ram_size{active_request}};\n"
                << "    if (M_DataRdy == 1'b1) begin\n"
                << "      next_step = S_" << idx + 1U << ";\n"
                << "      active_request_next = 1;\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                << "  end\n";
         }
         else
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram = active_request;\n"
                << "    Mout_addr_ram = (in1 + paramAddressRead) & {BITSIZE_Mout_addr_ram{active_request}};\n"
                << "    Mout_Wdata_ram = " << _p[idx + 1].name << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
                << "    Mout_data_ram_size = " << _p[idx + 1].type_size
                << " & {BITSIZE_Mout_data_ram_size{active_request}};\n"
                << "    if (M_DataRdy == 1'b1) begin\n"
                << "      next_step = in2[0] ? S_" << idx + 2U << " : S_" << idx + 1U << ";\n"
                << "      active_request_next = 1;\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                << "  end\n";
         }
      }
   }
   if(_p.size() > 2U)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "     Mout_we_ram = active_request;\n"
          << "     Mout_addr_ram = (in1 + paramAddressRead) & {BITSIZE_Mout_addr_ram{active_request}};\n"
          << "     Mout_Wdata_ram = " << _p[idx + 1].name << " & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
          << "     Mout_data_ram_size = " << _p[idx + 1].type_size
          << " & {BITSIZE_Mout_data_ram_size{active_request}};\n"
          << "   if (M_DataRdy == 1'b1) begin\n"
          << "     next_step = S_" << idx + 1U << ";\n"
          << "     active_request_next = 1;\n"
          << "   end else begin\n"
          << "     next_step = S_" << idx << ";\n"
          << "   end\n"
          << "  end\n";
      idx++;
   }

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    Mout_we_ram = active_request;\n"
       << "    Mout_addr_ram = in1 & {BITSIZE_Mout_addr_ram{active_request}};\n"
       << "    Mout_Wdata_ram = unlock_address & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
       << "    Mout_data_ram_size = BITSIZE_Mout_Wdata_ram & {BITSIZE_Mout_data_ram_size{active_request}};\n"
       << "    if (M_DataRdy == 1'b1) begin\n"
       << "      next_step = S_" << idx + 1U << ";\n"
       << "      active_request_next = 1;\n"
       << "    end else begin\n"
       << "      next_step = S_" << idx << ";\n"
       << "    end"
       << "  end\n";
   idx++;

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    if (S_we_ram == 1 && S_addr_ram == unlock_address) begin\n"
       << "      Sout_DataRdy = 1'b1;\n"
       << "      next_step = in2[0] ? S_" << (retval_size ? idx + 1U : 0U) << " : S_0;\n"
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
          << "      Mout_oe_ram = active_request;\n"
          << "      Mout_addr_ram = (in1 + paramAddressRead) & {BITSIZE_Mout_addr_ram{active_request}};\n"
          << "      Mout_data_ram_size = " << retval_size << " & {BITSIZE_Mout_data_ram_size{active_request}};\n"
          << "    if (M_DataRdy == 1'b1) begin\n"
          << "      next_step = S_" << idx + 1U << ";\n"
          << "      active_request_next = 1;\n"
          << "      next_readValue = M_Rdata_ram;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
      idx++;

      out << "  else if (step == S_" << idx << ") begin\n"
          << "    Mout_we_ram = active_request;\n"
          << "    Mout_addr_ram = " << _p[_p.size() - 1U].name << " & {BITSIZE_Mout_addr_ram{active_request}};\n"
          << "    Mout_Wdata_ram = readValue & {BITSIZE_Mout_Wdata_ram{active_request}};\n"
          << "    Mout_data_ram_size = " << retval_size << " & {BITSIZE_Mout_data_ram_size{active_request}};\n"
          << "    if (M_DataRdy == 1'b1) begin\n"
          << "      next_step = S_0;\n"
          << "      active_request_next = 1;\n"
          << "      done_port = 1'b1;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
   }
   out << "end\n";
}
