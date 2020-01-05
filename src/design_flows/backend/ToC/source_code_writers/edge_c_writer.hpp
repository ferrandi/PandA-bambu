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
 * @file edge_c_writer.hpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef EDGE_C_WRITER_HPP
#define EDGE_C_WRITER_HPP

/// Super class include
#include "c_writer.hpp"

/// Graph include
#include "basic_block.hpp"

/// STD include
#include <fstream>
#include <iosfwd>
#include <ostream>
#include <string>

/// STL include
#include <deque>
#include <list>
#include <set>
#include <vector>

/// Utility include
#include "graph.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);

/**
 * Class use to write the C code with instrumented edges
 */
class EdgeCWriter : public virtual CWriter
{
 protected:
   /**
    * Dump operation requested for instrument an edges
    * @param e is the edge
    * @param index is the index of the variable to be incremented
    */
   virtual void print_edge(EdgeDescriptor e, unsigned int index);

   /**
    * Print operations needed to store into symbol table information about last path
    * @param fun_id is the index of the function
    * @param loop_id is the index of the loop
    */
   virtual void print_end_path(unsigned int fun_id, unsigned int loop_id);

   /**
    * Print operation requested for record information about a path which exit from a loop and immediately enter in another
    * @param e is the edge
    */
   virtual void print_loop_switching(EdgeDescriptor e);

   /**
    * Write recursively instructions belonging to a basic block of task or of a function
    * @param function_behavior is the function to which instructions belong
    * @param current_vertex is the basic block which is being printed
    * @param instructions are the instructions which belongs to this task/function
    * @param bracket tells if bracket should be added before and after this basic block
    */
   void writeRoutineInstructions_rec(vertex current_vertex, bool bracket);

   /**
    * Writes the instructions of the current routine, being it a task or a function of the original program.
    * @param function_index is the index of the function
    * @param instructions is the instructions which have to be printed
    * @param var_pp_functor is the variable functor
    * @param bb_start is the first basic block to be printed
    * @param bb_end is the set of first basic block not to be printed
    */
   void writeRoutineInstructions(const unsigned int function_index, const OpVertexSet& instructions, const var_pp_functorConstRef variableFunctor, vertex bb_start = NULL_VERTEX, CustomOrderedSet<vertex> bb_end = CustomOrderedSet<vertex>()) override;

 protected:
   /// Increment which should be added before the label in a basic block
   std::map<vertex, EdgeDescriptor> local_inc;

   /// Set of already dumped edges
   std::set<EdgeDescriptor, ltedge<BBGraph>> dumped_edges;

   /// Map a pair function - loop to an unique index
   std::map<unsigned int, std::map<unsigned int, unsigned int>> fun_loop_to_index;

   /// The size of fun_loop_to_index
   unsigned int counter;

   /// Special control flow graphs
   BBGraphConstRef support_cfg;

   /// Index of the current function
   unsigned int fun_id;

   /**
    * Dump operations requested for record information about a loop path which ends
    * @param e is the feedback or outgoing edge
    */
   virtual void print_loop_ending(EdgeDescriptor e);

   /**
    * Dump operations requested for record information about a path which exit from a loop
    * @param e is the feedback or outgoing edge
    */
   virtual void print_loop_escaping(EdgeDescriptor e);

   /**
    * Dump initializations of variable for recording a loop path
    * @param e is the incoming edged
    */
   virtual void print_loop_starting(EdgeDescriptor e);

 public:
   /**
    * Constructor of the class
    * @param AppM is the application manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the output stream
    * @param Param is the set of parameters
    * @param verbose tells if annotations
    */
   EdgeCWriter(const application_managerConstRef _AppM, const InstructionWriterRef instruction_writer, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef Param, bool verbose = true);

   /**
    * Destructor
    */
   ~EdgeCWriter() override;

   /**
    * Returns the map which associates to each loop a unique id
    * @return the map which associates to each loop a unique id
    */
   const std::map<unsigned int, std::map<unsigned int, unsigned int>>& CGetFunctionLoopToId() const;

   /**
    * Initialize data structure
    */
   void Initialize() override;

   /**
    * Writes the header of the file
    */
   void WriteHeader() override;
};
#endif
