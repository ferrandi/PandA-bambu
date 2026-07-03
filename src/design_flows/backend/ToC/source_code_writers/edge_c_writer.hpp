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
 * @file edge_c_writer.hpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef EDGE_C_WRITER_HPP
#define EDGE_C_WRITER_HPP

#include "basic_block.hpp"
#include "c_writer.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <fstream>
#include <iosfwd>
#include <map>
#include <ostream>
#include <set>

struct BBGraphsCollection;

/**
 * Class use to write the C code with instrumented edges
 */
class EdgeCWriter : public CWriter
{
 protected:
   /// Increment which should be added before the label in a basic block
   std::map<gc_vertex_descriptor, gc_edge_descriptor> local_inc;

   /// Set of already dumped edges
   std::set<gc_edge_descriptor, ltedge<BBGraphsCollection>> dumped_edges;

   /// Map a pair function - loop to an unique index
   std::map<unsigned int, std::map<unsigned int, unsigned int>> fun_loop_to_index;

   /// The size of fun_loop_to_index
   unsigned int counter;

   /**
    * Dump operations requested for record information about a loop path which ends
    * @param fid is the identifier of the function containing the loop edge
    * @param e is the feedback or outgoing edge
    */
   virtual void print_loop_ending(unsigned fid, gc_edge_descriptor e);

   /**
    * Dump operations requested for record information about a path which exit from a loop
    * @param fid is the identifier of the function containing the loop edge
    * @param e is the feedback or outgoing edge
    */
   virtual void print_loop_escaping(unsigned fid, gc_edge_descriptor e);

   /**
    * Dump initializations of variable for recording a loop path
    * @param fid is the identifier of the function containing the loop edge
    * @param e is the incoming edged
    */
   virtual void print_loop_starting(unsigned fid, gc_edge_descriptor e);

   /**
    * Dump operation requested for instrument an edges
    * @param fid is the identifier of the function containing the edge
    * @param e is the edge
    * @param index is the index of the variable to be incremented
    */
   virtual void print_edge(unsigned fid, gc_edge_descriptor e, unsigned int index);

   /**
    * Print operations needed to store into symbol table information about last path
    * @param fun_id is the index of the function
    * @param loop_id is the index of the loop
    */
   virtual void print_end_path(unsigned int fun_id, unsigned int loop_id);

   /**
    * Print operation requested for record information about a path which exit from a loop and immediately enter in
    * another
    * @param fid is the identifier of the function containing the loop edge
    * @param e is the edge
    */
   virtual void print_loop_switching(unsigned fid, gc_edge_descriptor e);

   /**
    * Write recursively instructions belonging to a basic block of task or of a function
    * @param fid is the identifier of the function to which instructions belong
    * @param current_vertex is the basic block which is being printed
    * @param bracket tells if bracket should be added before and after this basic block
    * @param variableFunctor is the functor used to print variables inside the generated code
    */
   void writeRoutineInstructions_rec(unsigned fid, gc_vertex_descriptor current_vertex, bool bracket,
                                     const std::unique_ptr<var_pp_functor>& variableFunctor);

   /**
    * Writes the instructions of the current routine, being it a task or a function of the original program.
    * @param function_index is the index of the function
    * @param instructions is the instructions which have to be printed
    * @param variableFunctor is the variable functor
    * @param bb_start is the first basic block to be printed
    * @param bb_end is the set of first basic block not to be printed
    */
   void writeRoutineInstructions(
       const unsigned int function_index, const OpVertexSet& instructions,
       const std::unique_ptr<var_pp_functor>& variableFunctor, gc_vertex_descriptor bb_start = gc_null_vertex(),
       CustomOrderedSet<gc_vertex_descriptor> bb_end = CustomOrderedSet<gc_vertex_descriptor>()) override;

   virtual void Initialize() override;

   virtual void InternalWriteHeader() override;

 public:
   /**
    * Constructor of the class
    * @param _HLSMgr is the HLS manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the output stream
    */
   EdgeCWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef instruction_writer,
               const IndentedOutputStreamRef indented_output_stream);

   virtual ~EdgeCWriter() override = default;

   /**
    * Returns the map which associates to each loop a unique id
    * @return the map which associates to each loop a unique id
    */
   const std::map<unsigned int, std::map<unsigned int, unsigned int>>& CGetFunctionLoopToId() const;
};
#endif
