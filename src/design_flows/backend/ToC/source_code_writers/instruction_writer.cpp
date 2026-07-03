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
 * @file instruction_writer.cpp
 * @brief Simple class to print single instruction
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "instruction_writer.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "indented_output_stream.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "var_pp_functor.hpp"

InstructionWriter::InstructionWriter(const application_managerConstRef _AppM,
                                     const IndentedOutputStreamRef _indented_output_stream,
                                     const ParameterConstRef _parameters)
    : AppM(_AppM), indented_output_stream(_indented_output_stream), parameters(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void InstructionWriter::Initialize()
{
}

void InstructionWriter::write(const FunctionBehaviorConstRef function_behavior, OpGraph::vertex_descriptor statement,
                              const std::unique_ptr<var_pp_functor>& varFunctor)
{
   const auto statement_string = function_behavior->CGetBehavioralHelper()->print_vertex(
       function_behavior->GetOpGraph(FunctionBehavior::CFG), statement, varFunctor);

   if(statement_string.size())
   {
      indented_output_stream->Append(statement_string);
   }
}

void InstructionWriter::declareFunction(const unsigned int function_id)
{
   const auto TM = AppM->get_ir_manager();
   const auto BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto fdecl =
       ir_helper::PrintType(TM->GetIRNode(function_id), true, false, nullptr, std::make_unique<std_var_pp_functor>(BH));
   indented_output_stream->Append(fdecl);
}

void InstructionWriter::write_declarations()
{
}

void InstructionWriter::WriteComment(const std::string& text)
{
   indented_output_stream->Append("//" + text + "\n");
}
