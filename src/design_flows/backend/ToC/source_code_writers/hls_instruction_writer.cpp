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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file hls_instruction_writer.cpp
 * @brief Simple class to print single instruction
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <regex>

HLSInstructionWriter::HLSInstructionWriter(const application_managerConstRef _app_man,
                                           const IndentedOutputStreamRef _indented_output_stream,
                                           const ParameterConstRef _parameters)
    : InstructionWriter(_app_man, _indented_output_stream, _parameters)
{
}

HLSInstructionWriter::~HLSInstructionWriter() = default;

void HLSInstructionWriter::declareFunction(const unsigned int function_id)
{
   // All I have to do is to change main in _main
   const auto TM = AppM->get_tree_manager();
   const auto FB = AppM->CGetFunctionBehavior(function_id);
   const auto BH = FB->CGetBehavioralHelper();
   const auto fname = BH->GetMangledFunctionName();
   auto fdecl = tree_helper::PrintType(TM, TM->GetTreeNode(function_id), false, true, false, nullptr,
                                       var_pp_functorConstRef(new std_var_pp_functor(BH)));

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
