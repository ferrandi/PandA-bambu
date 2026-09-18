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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file hls_c_writer.hpp
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef HLS_C_WRITER_HPP
#define HLS_C_WRITER_HPP
#include "c_writer.hpp"

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(HLSCBackendInformation);

class HLSCWriter : public CWriter
{
 protected:
   /// Backend information
   const CBackendInformationConstRef c_backend_info;

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
    */
   void WriteParamInitialization(const BehavioralHelperConstRef behavioral_helper,
                                 const std::map<std::string, std::string>& curr_test_vector);

   /**
    * Writes a call to the top function to be tested, using its parameters.
    * The code for the declaration and initialization of such parameters
    * must be written before a call to obtain meaningful outcome.
    * @param behavioral_helper refers to the function to be tested
    */
   void WriteTestbenchFunctionCall(const BehavioralHelperConstRef behavioral_helper);

   /**
    * Write additional initialization code needed by subclasses
    */
   virtual void WriteExtraInitCode();

   virtual void WriteExtraCodeBeforeEveryMainCall();

   virtual void InternalInitialize() override;

   /**
    * Writes the global declarations
    */
   virtual void InternalWriteGlobalDeclarations() override;

   /**
    * Write function implementation
    * @param function_index is the index of the function to be written
    */
   virtual void WriteFunctionImplementation(unsigned int function_index) override;

   /**
    * Writes implementation of __builtin_wait_call
    */
   virtual void WriteBuiltinWaitCall() override;

   void InternalWriteHeader() override;

   /**
    * Writes the main() of the testbench C program
    */
   virtual void InternalWriteFile() override;

 public:
   /**
    * Constructor of the class
    * @param hls_c_backend_information is the information about backend
    * @param _HLSMgr is the hls manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the stream where code has to be printed
    */
   HLSCWriter(const CBackendInformationConstRef hls_c_backend_information, const HLS_managerConstRef _HLSMgr,
              const InstructionWriterRef instruction_writer, const IndentedOutputStreamRef indented_output_stream);

   virtual ~HLSCWriter() = default;
};
#endif
