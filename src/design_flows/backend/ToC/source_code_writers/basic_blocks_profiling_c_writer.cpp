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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file basic_blocks_profiling_c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges for profiling of executions of single basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "basic_blocks_profiling_c_writer.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// constants include
#include "host_profiling_constants.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp"

BasicBlocksProfilingCWriter::BasicBlocksProfilingCWriter(const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer, const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _Param, bool _verbose)
    : CWriter(_AppM, _instruction_writer, _indented_output_stream, _Param, _verbose), EdgeCWriter(_AppM, _instruction_writer, _indented_output_stream, _Param, _verbose)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
}

BasicBlocksProfilingCWriter::~BasicBlocksProfilingCWriter() = default;

void BasicBlocksProfilingCWriter::print_loop_ending(EdgeDescriptor e)
{
   print_edge(e, 0);
}

void BasicBlocksProfilingCWriter::print_loop_escaping(EdgeDescriptor e)
{
   print_edge(e, 0);
}

void BasicBlocksProfilingCWriter::print_loop_starting(EdgeDescriptor e)
{
   print_edge(e, 0);
}

void BasicBlocksProfilingCWriter::print_edge(EdgeDescriptor e, unsigned int)
{
   const auto target_id = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->block->number;
   const auto function_name = AppM->CGetFunctionBehavior(fun_id)->CGetBehavioralHelper()->get_function_name();
   indented_output_stream->Append(function_name + "_counter[" + STR(target_id) + "]++;\n");
   dumped_edges.insert(e);
}

void BasicBlocksProfilingCWriter::print_loop_switching(EdgeDescriptor e)
{
   print_edge(e, 0);
}

void BasicBlocksProfilingCWriter::WriteGlobalDeclarations()
{
   CWriter::WriteGlobalDeclarations();
   indented_output_stream->Append("#include <stdlib.h>\n");
   indented_output_stream->Append("#include <stdio.h>\n");
   indented_output_stream->Append("\n");
   CustomOrderedSet<unsigned int> functions = AppM->get_functions_with_body();
   for(const auto function : functions)
   {
      const auto function_behavior = AppM->CGetFunctionBehavior(function);
      const auto function_name = function_behavior->CGetBehavioralHelper()->get_function_name();
      const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function));
      const auto sl = GetPointer<statement_list>(GET_CONST_NODE(fd->body));
      const auto biggest_bb_number = sl->list_of_bloc.rbegin()->first;
      indented_output_stream->Append("int " + function_name + "_counter[" + STR(biggest_bb_number + 1) + "];\n");
   }
   indented_output_stream->Append("void _init_tp() __attribute__ ((no_instrument_function, constructor));\n");
   indented_output_stream->Append("void _init_tp()\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("int i = 0;\n");
   for(const auto function : functions)
   {
      const auto function_behavior = AppM->CGetFunctionBehavior(function);
      const auto function_name = function_behavior->CGetBehavioralHelper()->get_function_name();
      const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function));
      const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
      const auto biggest_bb_number = sl->list_of_bloc.rbegin()->first;
      indented_output_stream->Append("for(i = 0; i < " + STR(biggest_bb_number + 1) + "; i++)\n");
      indented_output_stream->Append("   " + function_name + "_counter[i] = 0;\n");
   }
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("void _end_tp() __attribute__ ((no_instrument_function, destructor));\n");
   indented_output_stream->Append("void _end_tp()\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("FILE* h_file = fopen(\"" + Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_host_profiling_data + "\", \"w\");\n");
   indented_output_stream->Append("int i = 0;\n");
   for(const auto function : functions)
   {
      const auto function_behavior = AppM->CGetFunctionBehavior(function);
      const auto function_name = function_behavior->CGetBehavioralHelper()->get_function_name();
      const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function));
      const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
      const auto biggest_bb_number = sl->list_of_bloc.rbegin()->first;
      indented_output_stream->Append("fprintf(h_file, \"Function %d\\n\", " + STR(function) + ");\n");
      indented_output_stream->Append("for(i = 0; i < " + STR(biggest_bb_number + 1) + "; i++)\n");
      indented_output_stream->Append("   fprintf(h_file, \"%d %d\\n\", i, " + function_name + "_counter[i]);\n");
   }
   indented_output_stream->Append("fclose(h_file);\n");

   indented_output_stream->Append("}\n");
}
