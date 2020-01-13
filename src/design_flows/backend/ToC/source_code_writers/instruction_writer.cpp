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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

/// Autoheader include
#include "config_HAVE_GRAPH_PARTITIONING_BUILT.hpp"
#include "config_HAVE_MPPB.hpp"

/// Header include
#include "instruction_writer.hpp"

/// behavior include
#include "application_manager.hpp"

/// design_flows/backend/ToC/source_code_writers
#if HAVE_GRAPH_PARTITIONING_BUILT
#include "pthread_instruction_writer.hpp"
#endif

/// design_flows/codesign/partitioning/graph_partitioningh
#if HAVE_GRAPH_PARTITIONING_BUILT
#include "partitioning_manager.hpp"
#endif

/// Backend include
#include "c_writer.hpp"
#if HAVE_MPPB
#include "mppb_instruction_writer.hpp"
#endif
#include "prettyPrintVertex.hpp"
#include "simple_indent.hpp"

/// Behavior include
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"

/// design_flows/backend/ToC/progmodels include
#include "actor_graph_backend.hpp"

/// Graph include
#include "graph.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <cmath>
#include <fstream>
#include <iosfwd>
#include <ostream>

/// tree includes
#include "var_pp_functor.hpp"

/// utility include
#include "indented_output_stream.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

InstructionWriter::InstructionWriter(const application_managerConstRef _AppM, const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _parameters)
    : AppM(_AppM), indented_output_stream(_indented_output_stream), parameters(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

InstructionWriterRef InstructionWriter::CreateInstructionWriter(const ActorGraphBackend_Type actor_graph_backend_type, const application_managerConstRef AppM, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters)
{
   switch(actor_graph_backend_type)
   {
#if HAVE_MPPB
      case(ActorGraphBackend_Type::BA_MPPB):
      {
         return InstructionWriterRef(new MppbInstructionWriter(AppM, indented_output_stream, true, parameters));
      }
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
      case(ActorGraphBackend_Type::BA_OPENMP):
#endif
      case(ActorGraphBackend_Type::BA_NONE):
      {
         return InstructionWriterRef(new InstructionWriter(AppM, indented_output_stream, parameters));
      }
#if HAVE_GRAPH_PARTITIONING_BUILT
      case(ActorGraphBackend_Type::BA_PTHREAD):
      {
         return InstructionWriterRef(new PThreadInstructionWriter(RefcountCast<const PartitioningManager>(AppM), indented_output_stream, parameters));
      }
#endif
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

void InstructionWriter::write(const FunctionBehaviorConstRef function_behavior, const vertex statement, const var_pp_functorConstRef varFunctor)
{
   const std::string statement_string = function_behavior->CGetBehavioralHelper()->print_vertex(function_behavior->CGetOpGraph(FunctionBehavior::CFG), statement, varFunctor);

   if(statement_string.size())
      indented_output_stream->Append(statement_string);
}

void InstructionWriter::declareFunction(const unsigned int function_id)
{
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   indented_output_stream->Append(behavioral_helper->print_type(function_id, false, true, false, 0, var_pp_functorConstRef(new std_var_pp_functor(behavioral_helper))));
}

void InstructionWriter::write_declarations()
{
}

void InstructionWriter::WriteComment(const std::string& text)
{
   indented_output_stream->Append("//" + text + "\n");
}
