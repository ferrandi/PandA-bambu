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
 * @file WriteFifoHDLGenerator.cpp
 * @brief Implementation of the HDL generator for a FIFO-based write interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "WriteFifoHDLGenerator.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_objects.hpp"

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_async,
   i_full_n,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_din,
   o_write,
   o_last
};

WriteFifoHDLGenerator::WriteFifoHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void WriteFifoHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                                         gc_vertex_descriptor /* op_v */, const HDLWriter_Language /* language */,
                                         const std::vector<HDLGenerator::parameter>& /* _p */,
                                         const std::vector<HDLGenerator::parameter>& _ports_in,
                                         const std::vector<HDLGenerator::parameter>& _ports_out,
                                         const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager().GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   if(iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end())
   {
      THROW_ERROR("Registered FIFO interface not yet implemented.");
   }
   out << "reg started;\n"
       << "wire started_0, active;\n\n";

   out << "always @(posedge clock 1RESET_EDGE)\n"
       << "begin\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    started <= 0;\n"
       << "  end\n"
       << "  else\n"
       << "  begin\n"
       << "    started <= started_0;\n"
       << "  end\n"
       << "end\n\n";

   out << "assign active = (started | " << _ports_in[i_start].name << ");\n"
       << "assign started_0 = active & ~" << _ports_in[i_full_n].name << ";\n\n";

   out << "assign " << _ports_out[o_done].name << " = active & (" << _ports_in[i_full_n].name << "|(1&"
       << _ports_in[i_async].name << "));\n";
   out << "assign " << _ports_out[o_write].name << " = active & " << _ports_in[i_full_n].name << ";\n";
   out << "assign " << _ports_out[o_out1].name << " = " << _ports_in[i_full_n].name << ";\n";
   out << "assign " << _ports_out[o_din].name << " = " << _ports_in[i_in3].name << ";\n";
}
