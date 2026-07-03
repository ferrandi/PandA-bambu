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
 *              Copyright (C) 2024-2026 Politecnico di Milano
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
 * @file mdpi_wrapper_c_writer.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef MDPI_WRAPPER_CWRITER_HPP
#define MDPI_WRAPPER_CWRITER_HPP
#include "c_writer.hpp"

class MdpiWrapperCWriter : public CWriter
{
   void WriteSimulatorInitMemory(const unsigned int function_id, bool global_use_banked);

   /**
    * Subfunction that writes the inizialitation values for the external variables
    */
   unsigned long long WriteMemmap_initalization(unsigned long long base_addr, const unsigned int function_id);

   /**
    * Allocates the memory on the banks used by the simulation
    */
   void WriteSimulatorBankedMemory(const unsigned int function_id, const unsigned int banked_args_decl_size);

   /**
    * Initialize the data structure used by the banked memory
    */
   void InitilizedBMDataStructure(const unsigned int bundle_number, const unsigned int bank_number,
                                  const bool use_space_required);

   /**
    * Copies the results of the simulation from the banked memory to the original one
    */
   void WriteBankedMemoryWritebackValue(const unsigned int function_id);

   void WriteFunctionImplementation(unsigned int) override;

   void WriteBuiltinWaitCall() override;

   void InternalInitialize() override;

   void InternalWriteHeader() override;

   void InternalWriteGlobalDeclarations() override;

   void InternalWriteFile() override;

 public:
   MdpiWrapperCWriter(const HLS_managerConstRef HLSMgr, const InstructionWriterRef instruction_writer,
                      const IndentedOutputStreamRef indented_output_stream);
};

#endif // MDPI_WRAPPER_CWRITER_HPP