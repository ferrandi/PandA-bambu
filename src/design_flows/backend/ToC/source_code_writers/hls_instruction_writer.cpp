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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file hls_instruction_writer.cpp
 * @brief Simple class to print single instruction
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "hls_instruction_writer.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "c_writer.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <regex>

HLSInstructionWriter::HLSInstructionWriter(const application_managerConstRef _app_man,
                                           const IndentedOutputStreamRef _indented_output_stream,
                                           const ParameterConstRef _parameters)
    : InstructionWriter(_app_man, _indented_output_stream, _parameters)
{
}

void HLSInstructionWriter::declareFunction(const unsigned int function_id)
{
   // All I have to do is to change main in _main
   const auto TM = AppM->get_ir_manager();
   const auto FB = AppM->CGetFunctionBehavior(function_id);
   const auto BH = FB->CGetBehavioralHelper();
   const auto fname = BH->GetFunctionName();
   auto fdecl =
       ir_helper::PrintType(TM->GetIRNode(function_id), true, false, nullptr, std::make_unique<std_var_pp_functor>(BH));

   const auto HLSMgr = GetPointer<const HLS_manager>(AppM);
   if(HLSMgr)
   {
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
      if(func_arch)
      {
         THROW_ASSERT(func_arch, "Expected interface architecture for function " + fname);
         const auto parm_original_typename = [&]() -> std::vector<std::string> {
            std::vector<std::string> parm_ot(func_arch->parms.size(), "");
            for(auto& [parm, attrs] : func_arch->parms)
            {
               const auto idx = std::strtoul(attrs.at(FunctionArchitecture::parm_index).c_str(), nullptr, 10);
               parm_ot[idx] = attrs.at(FunctionArchitecture::parm_original_typename);
            }
            return parm_ot;
         }();
         const std::regex param_match("[^,(]+\\s(\\w+)\\s*([,)]\\s?)");
         auto param_idx = 0U;
         auto it = fdecl.cbegin();
         std::string if_fdecl;
         std::smatch match;
         while(std::regex_search(it, fdecl.cend(), match, param_match))
         {
            THROW_ASSERT(param_idx < parm_original_typename.size(), "Too many parameters matched.");
            it += match.position() + match.length();
            if_fdecl += match.prefix();
            if_fdecl += parm_original_typename.at(param_idx++) + " ";
            if_fdecl += match[1];
            if_fdecl += match[2];
         }
         THROW_ASSERT(param_idx == parm_original_typename.size(), "Expected to match all parameter declarations.");
         if(param_idx)
         {
            fdecl = if_fdecl;
         }
      }
   }
   if(fname == "main")
   {
      boost::replace_all(fdecl, " main(", " _main("); /// the assumption is strong but the code that prints the
                                                      /// name of the function is under our control ;-)
   }

   indented_output_stream->Append(fdecl);
}
