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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file discrepancy_instruction_writer.cpp
 * @brief specialization of the instruction writer for the discrepancy analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "discrepancy_instruction_writer.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"

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
   const auto funName = BH->GetFunctionName();
   const auto TM = AppM->get_ir_manager();
   const auto node_fun = TM->GetIRNode(function_id);
   THROW_ASSERT(GetPointer<function_val_node>(node_fun), "expected a function decl");
   const auto prepend_static =
       !ir_helper::IsStaticDeclaration(node_fun) && !ir_helper::IsExternDeclaration(node_fun) && funName != "main";
   if(prepend_static)
   {
      GetPointerS<function_val_node>(node_fun)->static_flag = true;
   }
   HLSInstructionWriter::declareFunction(function_id);
   if(prepend_static)
   {
      GetPointerS<function_val_node>(node_fun)->static_flag = false;
   }
}
