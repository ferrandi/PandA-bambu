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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file basic_blocks_profiling_c_writer.hpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges for
 * profiling of executions of single basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef BASIC_BLOCKS_PROFILING_C_WRITER_HPP
#define BASIC_BLOCKS_PROFILING_C_WRITER_HPP

#include "edge_c_writer.hpp"

/**
 * Class use to write the C code with instruented edges for basic blocks profiling
 */
class BasicBlocksProfilingCWriter final : public EdgeCWriter
{
   /**
    * Dump operations requested for record information about a loop path which ends
    * @param e is the feedback or outgoing edge
    */
   void print_loop_ending(EdgeDescriptor e) final;

   /**
    * Dump operations requested for record information about a path which exit from a loop
    * @param e is the feedback or outgoing edge
    */
   void print_loop_escaping(EdgeDescriptor e) final;

   /**
    * Dump initializations of variable for recording a loop path
    * @param e is the incoming edged
    */
   void print_loop_starting(EdgeDescriptor e) final;

   /**
    * Dump operation requested for instrument an edges
    * @param e is the edge
    * @param index is the index of the variable to be incremented
    */
   void print_edge(EdgeDescriptor e, unsigned int index) final;

   /**
    * Print operation requested for record information about a path which exit from a loop and immediately enter in
    * another
    * @param e is the edge
    */
   void print_loop_switching(EdgeDescriptor e) final;

   void InternalWriteGlobalDeclarations() final;

 public:
   /**
    * Constructor of the class
    * @param HLSMgr is the hls manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the output stream
    */
   BasicBlocksProfilingCWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef instruction_writer,
                               const IndentedOutputStreamRef indented_output_stream);

   ~BasicBlocksProfilingCWriter() final;
};
#endif
