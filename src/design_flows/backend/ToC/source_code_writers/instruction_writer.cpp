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
 * @file instruction_writer.cpp
 * @brief Simple class to print single instruction
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "instruction_writer.hpp"

#include "Parameter.hpp"
#include "actor_graph_backend.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "indented_output_stream.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "var_pp_functor.hpp"

InstructionWriter::InstructionWriter(const application_managerConstRef _AppM,
                                     const IndentedOutputStreamRef _indented_output_stream,
                                     const ParameterConstRef _parameters)
    : AppM(_AppM), indented_output_stream(_indented_output_stream), parameters(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

InstructionWriterRef InstructionWriter::CreateInstructionWriter(const ActorGraphBackend_Type actor_graph_backend_type,
                                                                const application_managerConstRef AppM,
                                                                const IndentedOutputStreamRef indented_output_stream,
                                                                const ParameterConstRef parameters)
{
   switch(actor_graph_backend_type)
   {
      case(ActorGraphBackend_Type::BA_NONE):
      {
         return InstructionWriterRef(new InstructionWriter(AppM, indented_output_stream, parameters));
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return InstructionWriterRef();
}

InstructionWriter::~InstructionWriter() = default;

void InstructionWriter::Initialize()
{
}

void InstructionWriter::write(const FunctionBehaviorConstRef function_behavior, const vertex statement,
                              const var_pp_functorConstRef varFunctor)
{
   const auto statement_string = function_behavior->CGetBehavioralHelper()->print_vertex(
       function_behavior->CGetOpGraph(FunctionBehavior::CFG), statement, varFunctor);

   if(statement_string.size())
   {
      indented_output_stream->Append(statement_string);
   }
}

void InstructionWriter::declareFunction(const unsigned int function_id)
{
   const auto TM = AppM->get_tree_manager();
   const auto BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto fdecl = tree_helper::PrintType(TM, TM->GetTreeNode(function_id), false, true, false, nullptr,
                                             var_pp_functorConstRef(new std_var_pp_functor(BH)));
   indented_output_stream->Append(fdecl);
}

void InstructionWriter::write_declarations()
{
}

void InstructionWriter::WriteComment(const std::string& text)
{
   indented_output_stream->Append("//" + text + "\n");
}
