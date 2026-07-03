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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
   bool enable_instrumentation{false};

   void print_loop_ending(unsigned fid, gc_edge_descriptor e) final;

   void print_loop_escaping(unsigned fid, gc_edge_descriptor e) final;

   void print_loop_starting(unsigned fid, gc_edge_descriptor e) final;

   void print_edge(unsigned fid, gc_edge_descriptor e, unsigned int index) final;

   void print_loop_switching(unsigned fid, gc_edge_descriptor e) final;

   void InternalWriteHeader() final;

   void InternalWriteGlobalDeclarations() final;

   void StartFunctionBody(const unsigned int function_id) final;

   void EndFunctionBody(unsigned int funId) final;

 public:
   /**
    * Constructor of the class
    * @param _HLSMgr is the hls manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the output stream
    */
   BasicBlocksProfilingCWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef instruction_writer,
                               const IndentedOutputStreamRef indented_output_stream);
};
#endif
