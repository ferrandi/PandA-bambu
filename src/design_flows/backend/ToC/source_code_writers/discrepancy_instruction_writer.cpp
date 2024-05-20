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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file discrepancy_instruction_writer.cpp
 * @brief specialization of the instrunction writer for the discrepancy analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "discrepancy_instruction_writer.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

discrepancy_instruction_writer::discrepancy_instruction_writer(const application_managerConstRef _app_man,
                                                               const IndentedOutputStreamRef _indented_output_stream,
                                                               const ParameterConstRef _parameters)
    : HLSInstructionWriter(_app_man, _indented_output_stream, _parameters)
{
}

void discrepancy_instruction_writer::declareFunction(const unsigned int function_id)
{
   const auto FB = AppM->CGetFunctionBehavior(function_id);
   const auto BH = FB->CGetBehavioralHelper();
   const auto funName = BH->get_function_name();
   const auto TM = AppM->get_tree_manager();
   const auto node_fun = TM->GetTreeNode(function_id);
   THROW_ASSERT(GetPointer<function_decl>(node_fun), "expected a function decl");
   const auto prepend_static =
       !tree_helper::IsStaticDeclaration(node_fun) && !tree_helper::IsExternDeclaration(node_fun) && funName != "main";
   if(prepend_static)
   {
      GetPointerS<function_decl>(node_fun)->static_flag = true;
   }
   HLSInstructionWriter::declareFunction(function_id);
   if(prepend_static)
   {
      GetPointerS<function_decl>(node_fun)->static_flag = false;
   }
}
