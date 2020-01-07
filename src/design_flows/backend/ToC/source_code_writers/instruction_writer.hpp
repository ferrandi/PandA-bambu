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
 * @file instruction_writer.hpp
 * @brief Simple class to print single instruction
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef INSTRUCTION_WRITER_HPP
#define INSTRUCTION_WRITER_HPP

/// Graph include
#include "graph.hpp"

/// STD include
#include <fstream>
#include <iosfwd>
#include <ostream>

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(IndentedOutputStream);
REF_FORWARD_DECL(InstructionWriter);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(var_pp_functor);
class simple_indent;
enum class ActorGraphBackend_Type;

class InstructionWriter
{
 protected:
   /// The application manager
   const application_managerConstRef AppM;

   /// The indented output stream
   const IndentedOutputStreamRef indented_output_stream;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

   /**
    * Constructor; it is protected since factory method should be used
    * @param AppM is the application manager
    * @param indented_output_stream is the output stream for source code
    * @param parameters is the set of input parameters
    */
   InstructionWriter(const application_managerConstRef AppM, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters);

 public:
   /**
    * Factory method
    * @param actor_graph_backend_type is the type of thread model to be considered in backend
    * @param AppM is the application manager
    * @param indented_output_stream is the output stream
    * @param parameters is the set of input parameters
    */
   static InstructionWriterRef CreateInstructionWriter(const ActorGraphBackend_Type actor_graph_backend_type, const application_managerConstRef AppM, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~InstructionWriter();

   /**
    * Initialize data structure
    */
   virtual void Initialize();

   /**
    * Write a statement
    * @param function_behavior is the function to which the statement belongs
    * @param statement is the statement to be printed
    * @param varFunctor is the variable functor
    */
   virtual void write(const FunctionBehaviorConstRef function_behavior, const vertex statement, const var_pp_functorConstRef varFunctor);

   /**
    * Write the declaration of a function
    * @param function_id is the index of the function
    */
   virtual void declareFunction(const unsigned int function_id);

   /**
    * Write code needed for declaration/initialization of auxiliary variables
    */
   virtual void write_declarations();

   /**
    * Writes a comment
    * @param comment is the string to be printed
    */
   void WriteComment(const std::string& comment);
};
typedef refcount<InstructionWriter> InstructionWriterRef;
#endif
