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
 * @file hls_c_writer.hpp
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */

#ifndef HLS_C_WRITER_HPP
#define HLS_C_WRITER_HPP

/// Superclass include
#include "c_writer.hpp"

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(HLSCBackendInformation);

class HLSCWriter : public CWriter
{
 protected:
   /// Backend information
   const HLSCBackendInformationConstRef hls_c_backend_information;

   /**
    * Print the binary representation of a number
    */
   std::string convert_in_binary(const BehavioralHelperConstRef behavioral_helper, unsigned int base_type, const std::string& C_value, unsigned int precision);

   /**
    * Check if a binary string is a sequence of zeros with
    * length multiple of 8
    */
   bool is_all_8zeros(const std::string& str);

   /**
    * Write printf statements to write n_bytes bytes initialized to 0 for the
    * memory initialization of the HDL simulator
    * @param [in] n_bytes the number of zeroed bytes to be printed
    */
   inline void WriteZeroedBytes(const size_t n_bytes);

   /**
    * Takes a binary string and writes in the C file a series of statements
    * used to print the equivalent memory initialization data for the HDL
    * simulator
    */
   size_t WriteBinaryMemoryInit(const std::string& binary_string, const size_t data_size, std::string& bits_offset);

   /**
    * Write global variables needed by the tesbench
    */
   void WriteTestbenchGlobalVars();

   /**
    * Write helper functions used by the testbench, like an __exit() helper
    * and some binary conversion functions used to print memory
    * initialization information
    */
   void WriteTestbenchHelperFunctions();

   /**
    * Write declaration of the top function parameters.
    * Declaration and initialization are separate statements.
    */
   void WriteParamDecl(const BehavioralHelperConstRef behavioral_helper);

   /**
    * Initialize the parameters of the function.
    * Declaration and initialization are separate statements and the
    * declaration code must be printed before the initialization code.
    * @param behavioral_helper is the function to which the parameters are referred
    * @param curr_test_vector is the test vector used to initialize the parameters
    * @param vector_index is the index of the test_vector (this should not be
    * needed here, since it's used to get memory information, it will be
    * removed when tesbench memory allocation and c testbench creation will
    * be executed as separate steps)
    */
   void WriteParamInitialization(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector, const unsigned int vector_index);

   /**
    * Writes a call to the top function to be tested, using its parameters.
    * The code for the declaration and initialization of such parameters
    * must be written before a call to obtain meaningful outcome.
    * @param behavioral_helper refers to the function to be tested
    */
   void WriteTestbenchFunctionCall(const BehavioralHelperConstRef behavioral_helper);

   /**
    * Write some print statements used to dump the results of each call to
    * the tested function. The output is in a format that is recognized by
    * the HDL testbench generation
    */
   void WriteExpectedResults(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector);

   /**
    * Write some print statements used to dump the values used by the HDL to
    * initialize the memory before the simulation.
    * (This function actually also reserves memory out of the top function to
    * be tested, if needed. This should not be done here, but in a separate
    * memory allocation step, and will be moved soon).
    */
   void WriteSimulatorInitMemory(const unsigned int function_id);

   /**
    * Write additional initialization code needed by subclasses
    */
   virtual void WriteExtraInitCode();

   virtual void WriteExtraCodeBeforeEveryMainCall();

   /**
    * Writes the main() of the testbench C program
    */
   void WriteMainTestbench();

   /**
    * Writes the global declarations
    */
   void WriteGlobalDeclarations() override;

   /**
    * Write function implementation
    * @param function_id is the index of the function to be written
    */
   void WriteFunctionImplementation(unsigned int function_index) override;

   /**
    * Writes implementation of __builtin_wait_call
    */
   void WriteBuiltinWaitCall() override;

 public:
   /**
    * Constructor of the class
    * @param hls_c_backend_information is the information about backend
    * @param AppM is the manager of the application
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the stream where code has to be printed
    * @param Param is the set of parameters
    * @param verbose tells if annotations
    */
   HLSCWriter(const HLSCBackendInformationConstRef hls_c_backend_information, const application_managerConstRef _AppM, const InstructionWriterRef instruction_writer, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef Param,
              bool verbose = true);

   /**
    * Destructor
    */
   ~HLSCWriter() override;

   /**
    * Writes the final C file
    * @param file_name is the name of the file to be generated
    */
   void WriteFile(const std::string& file_name) override;

   /**
    * Writes the header of the file
    */
   void WriteHeader() override;
};
#endif
