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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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
#include "tree_reindex.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

HLSInstructionWriter::HLSInstructionWriter(const application_managerConstRef _app_man,
                                           const IndentedOutputStreamRef _indented_output_stream,
                                           const ParameterConstRef _parameters)
    : InstructionWriter(_app_man, _indented_output_stream, _parameters)
{
}

HLSInstructionWriter::~HLSInstructionWriter() = default;

void HLSInstructionWriter::declareFunction(const unsigned int function_id)
{
   const auto flag_pp = parameters->isOption(OPT_pretty_print) ||
                        (parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy));
   // All I have to do is to change main in _main
   const auto TM = AppM->get_tree_manager();
   const auto FB = AppM->CGetFunctionBehavior(function_id);
   const auto BH = FB->CGetBehavioralHelper();
   auto fdecl = tree_helper::PrintType(TM, TM->CGetTreeReindex(function_id), false, true, false, nullptr,
                                       var_pp_functorConstRef(new std_var_pp_functor(BH)));
   const auto name = BH->get_function_name();

   if(!flag_pp)
   {
      const auto fd_node = AppM->get_tree_manager()->CGetTreeNode(function_id);
      const auto fd = GetPointer<const function_decl>(fd_node);
      const auto fname = tree_helper::GetMangledFunctionName(fd);
      const auto HLSMgr = GetPointerS<const HLS_manager>(AppM);
      if(HLSMgr && HLSMgr->design_interface_typename_orig_signature.find(fname) !=
                       HLSMgr->design_interface_typename_orig_signature.end())
      {
         const auto searchString = " " + name + "(";
         fdecl = fdecl.substr(0, fdecl.find(searchString) + searchString.size());
         THROW_ASSERT(HLSMgr->design_interface_typename_orig_signature.count(fname), "");
         const auto& typenameArgs = HLSMgr->design_interface_typename_orig_signature.at(fname);
         bool firstPar = true;
         for(auto i = 0U; i < typenameArgs.size(); ++i)
         {
            const auto& arg_typename = typenameArgs.at(i);
            if(firstPar)
            {
               fdecl += arg_typename;
               firstPar = false;
            }
            else
            {
               fdecl += ", " + arg_typename;
            }
         }
         fdecl += ")";
      }
   }
   if(name == "main")
   {
      boost::replace_all(fdecl, " main(", " _main("); /// the assumption is strong but the code that prints the
                                                      /// name of the function is under our control ;-)
   }

   indented_output_stream->Append(fdecl);
}
