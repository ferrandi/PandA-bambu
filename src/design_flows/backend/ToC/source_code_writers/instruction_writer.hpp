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
 * @file instruction_writer.hpp
 * @brief Simple class to print single instruction
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef INSTRUCTION_WRITER_HPP
#define INSTRUCTION_WRITER_HPP

#include "graph.hpp"
#include "refcount.hpp"

#include <fstream>
#include <iosfwd>
#include <ostream>

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(IndentedOutputStream);
REF_FORWARD_DECL(InstructionWriter);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(var_pp_functor);
class simple_indent;

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

 public:
   /**
    * Constructor; it is protected since factory method should be used
    * @param AppM is the application manager
    * @param indented_output_stream is the output stream for source code
    * @param parameters is the set of input parameters
    */
   InstructionWriter(const application_managerConstRef AppM, const IndentedOutputStreamRef indented_output_stream,
                     const ParameterConstRef parameters);

   virtual ~InstructionWriter() = default;

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
   virtual void write(const FunctionBehaviorConstRef function_behavior, gc_vertex_descriptor statement,
                      const std::unique_ptr<var_pp_functor>& varFunctor);

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
    * @param text is the string to be printed
    */
   void WriteComment(const std::string& text);
};
using InstructionWriterRef = refcount<InstructionWriter>;
#endif
