/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file KMPBambuCSGetTIDHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the OpenMP runtime helper that returns the thread identifier.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "KMPBambuCSGetTIDHDLGenerator.hpp"

#include "OMPCGExt.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_objects.hpp"

KMPBambuCSGetTIDHDLGenerator::KMPBambuCSGetTIDHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void KMPBambuCSGetTIDHDLGenerator::InternalExec(std::ostream& out, structural_objectRef _mod, unsigned int function_id,
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
   mod->get_out_port(0)->type_resize(ceil_log2(tid + omp_info->context_count - 1U));

   out << "reg [BITSIZE_" << _ports_out[0].name << "-1:0] tid [" << (omp_info->context_count - 1U) << ":0];\n";
   out << "initial\n"
       << "begin\n";
   for(auto i = 0U; i < omp_info->context_count; ++i, ++tid)
   {
      out << "tid[" << i << "] = 'd" << tid << ";\n";
   }
   out << "end\n\n";
   out << "assign out1 = tid[" << _ports_in[0].name << "];\n";
}
