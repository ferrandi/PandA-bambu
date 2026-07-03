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
 * @file KMPBambuCSGetGTIDHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the OpenMP runtime helper that returns the global thread identifier.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "KMPBambuCSGetGTIDHDLGenerator.hpp"

#include "OMPCGExt.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_objects.hpp"

KMPBambuCSGetGTIDHDLGenerator::KMPBambuCSGetGTIDHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void KMPBambuCSGetGTIDHDLGenerator::InternalExec(std::ostream& out, structural_objectRef _mod, unsigned int function_id,
                                                 gc_vertex_descriptor /* op_v */,
                                                 const HDLWriter_Language /* language */,
                                                 const std::vector<HDLGenerator::parameter>& /* _p */,
                                                 const std::vector<HDLGenerator::parameter>& _ports_in,
                                                 const std::vector<HDLGenerator::parameter>& _ports_out,
                                                 const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   const auto mod = GetPointer<module_o>(_mod);
   THROW_ASSERT(_ports_in.size() == 1, "Expected one input ports");
   THROW_ASSERT(_ports_out.size() == 1, "Expected one output port");
   const auto omp_info = HLSMgr->CGetFunctionBehavior(function_id)->GetOMPInfo();
   THROW_ASSERT(omp_info, "");
   const auto parent_info = HLSMgr->CGetFunctionBehavior(omp_info->fork_call_id)->GetOMPInfo();
   const auto fork_info = OMPCGExt::GetOMPForkInfo(omp_info->fork_call_id);
   auto tid = 0U;
   for(auto i = 0U; i < fork_info.size(); ++i)
   {
      if(fork_info.at(i)->core_id < omp_info->core_id)
      {
         tid += fork_info.at(i)->context_count;
      }
   }
   mod->get_in_port(0)->type_resize(ceil_log2(omp_info->context_count));
   mod->get_out_port(0)->type_resize(
       ceil_log2(OMPInfo::make_global(tid + omp_info->context_count - 1U, omp_info->ncore, parent_info)));

   out << "reg [BITSIZE_" << _ports_out[0].name << "-1:0] gtid [" << (omp_info->context_count - 1U) << ":0];\n";
   out << "initial\n"
       << "begin\n";

   for(auto i = 0U; i < omp_info->context_count; ++i, ++tid)
   {
      out << "gtid[" << i << "] = 'd" << OMPInfo::make_global(tid, omp_info->ncore, parent_info) << ";\n";
   }
   out << "end\n\n";
   out << "assign out1 = gtid[" << _ports_in[0].name << "];\n";
}
