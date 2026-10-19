/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
