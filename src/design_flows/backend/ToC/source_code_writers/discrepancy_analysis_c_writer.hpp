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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#ifndef DISCREPANCY_ANALYSIS_C_WRITER
#define DISCREPANCY_ANALYSIS_C_WRITER

#include "hls_c_writer.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(Discrepancy);

class DiscrepancyAnalysisCWriter : public HLSCWriter
{
   const DiscrepancyRef Discrepancy;

   void InternalInitialize() override;

   /**
    * Writes the global declarations
    */
   void InternalWriteGlobalDeclarations() override;

   /**
    * Write additional initialization code needed by subclasses
    */
   void WriteExtraInitCode() override;

   void WriteExtraCodeBeforeEveryMainCall() override;

   void WriteBBHeader(const unsigned int bb_number, const unsigned int function_index) override;

   /**
    * Write extra information on the given statement vertex, before the
    * statement itself
    */
   void writePreInstructionInfo(const FunctionBehaviorConstRef FB, gc_vertex_descriptor statement) override;

   /**
    * Write extra information on the given statement vertex, after the
    * statement itself
    */
   void writePostInstructionInfo(const FunctionBehaviorConstRef fun_behavior, gc_vertex_descriptor) override;

   /**
    * Write function implementation
    * @param function_index is the index of the function to be written
    */
   void WriteFunctionImplementation(unsigned int function_index) override;

   void WriteBuiltinWaitCall() override;

   /**
    * Declares the local variable; in case the variable used in the initialization of
    * curVar hasn't been declared yet it get declared
    * @param to_be_declared is the set of variables which have to be declared
    * @param already_declared_variables is the set of already declared variables
    * @param locally_declared_type is the set of already declared types
    * @param BH is the behavioral helper associated with the function
    * @param varFunc is the printer functor
    */
   void DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared,
                              CustomSet<unsigned int>& already_declared_variables,
                              CustomSet<std::string>& locally_declared_type, const BehavioralHelperConstRef BH,
                              const std::unique_ptr<var_pp_functor>& varFunc) override;

   void WriteFunctionDeclaration(const unsigned int funId) override;

   void InternalWriteFile() override;

 public:
   DiscrepancyAnalysisCWriter(const CBackendInformationConstRef _c_backend_information,
                              const HLS_managerConstRef _HLSMgr, const InstructionWriterRef _instruction_writer,
                              const IndentedOutputStreamRef _indented_output_stream);
};
#endif
