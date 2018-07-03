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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Castellana <michele.castellana@mail.polimi.it>
 */

#ifndef DISCREPANCY_ANALYSIS_C_WRITER
#define DISCREPANCY_ANALYSIS_C_WRITER

#include "hls_c_writer.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(Discrepancy);

class DiscrepancyAnalysisCWriter : public HLSCWriter
{
   protected:

      const DiscrepancyRef Discrepancy;

      /**
       * Writes the global declarations
       */
      virtual void WriteGlobalDeclarations();

      /**
       * Write additional initialization code needed by subclasses
       */
      virtual void WriteExtraInitCode();

      virtual void WriteExtraCodeBeforeEveryMainCall();

      virtual void WriteBBHeader(unsigned int bb_number);

      /**
       * Write extra information on the given statement vertex, before the
       * statement itself
       */
      void writePreInstructionInfo
      (const FunctionBehaviorConstRef FB, const vertex v);

      /**
       * Write extra information on the given statement vertex, after the
       * statement itself
       */
      void writePostInstructionInfo
      (const FunctionBehaviorConstRef fun_behavior, const vertex);

      /**
       * Write function implementation
       * @param function_id is the index of the function to be written
       */
      virtual void WriteFunctionImplementation(unsigned int function_index);

   public:

      /**
       * Constructor
       */
      DiscrepancyAnalysisCWriter
      (
         const HLSCBackendInformationConstRef _hls_c_backend_information,
         const application_managerConstRef _AppM,
         const InstructionWriterRef _instruction_writer,
         const IndentedOutputStreamRef _indented_output_stream,
         const ParameterConstRef _parameters,
         bool _verbose
      );

      /**
       * Destructor
       */
      ~DiscrepancyAnalysisCWriter();

      /**
       * Declares the local variable; in case the variable used in the intialization of
       * curVar hasn't been declared yet it get declared
       * @param to_be_declared is the set of variables which have to be declared
       * @param already_decl_variables is the set of already declared variables
       * @param locally_declared_type is the set of already declared types
       * @param helper is the behavioral helper associated with the function
       * @param varFunc is the printer functor
       */
      void DeclareLocalVariables
      (
         const CustomSet<unsigned int> & to_be_declared,
         CustomSet<unsigned int> & already_declared_variables,
         CustomSet<std::string> & locally_declared_type,
         const BehavioralHelperConstRef BH,
         const var_pp_functorConstRef varFunc
      );

      virtual void WriteFunctionDeclaration(const unsigned int funId);

      /**
       * Writes implementation of __builtin_wait_call
       */
      virtual void WriteBuiltinWaitCall();
};
#endif
