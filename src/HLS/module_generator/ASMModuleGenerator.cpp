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
 * @file ASMModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "ASMModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "op_graph.hpp"

#include <boost/algorithm/string/replace.hpp>

ASMModuleGenerator::ASMModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ASMModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */, unsigned int function_id,
                                      vertex op_v, const HDLWriter_Language language,
                                      const std::vector<ModuleGenerator::parameter>& /* _p */,
                                      const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                      const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                      const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
   const auto cfg = FB->CGetOpGraph(FunctionBehavior::CFG);
   std::string asm_string = FB->CGetBehavioralHelper()->get_asm_string(cfg->CGetOpNodeInfo(op_v)->GetNodeId());
   boost::replace_all(asm_string, "%%", "&percent;");
   boost::replace_all(asm_string, "&percent;", "%");
   boost::replace_all(asm_string, "\\n", "\n");
   boost::replace_all(asm_string, "\\\"", "\"");
   boost::replace_all(asm_string, "\\\'", "\'");

   if(asm_string.empty())
   {
      out << "assign done_port = start_port;\n";
   }
   else
   {
      if(language == HDLWriter_Language::VHDL)
      {
         out << "begin\n";
      }
      /// remove possible dialects
      auto open_curl = false;
      auto count_pipes = 0U;
      const auto skip_count = language == HDLWriter_Language::VHDL ? 3U : 2U;
      char prec_char = '\0';
      for(auto i = 0U; i < asm_string.size(); ++i)
      {
         const auto& current_char = asm_string[i];
         if(current_char == '{' && prec_char != '%')
         {
            open_curl = true;
         }
         else if(current_char == '}' && prec_char != '%')
         {
            open_curl = false;
            count_pipes = 0U;
         }
         else if(open_curl && current_char == '|' && prec_char != '%')
         {
            ++count_pipes;
         }
         else if(open_curl && count_pipes != skip_count)
         {
         }
         else if(open_curl && count_pipes == skip_count)
         {
            out << current_char;
         }
         else
         {
            out << current_char;
         }
         prec_char = current_char;
      }
      out << "\n";
   }
}