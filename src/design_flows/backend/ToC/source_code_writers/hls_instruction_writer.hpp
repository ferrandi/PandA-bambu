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
 * @file hls_instruction_writer.hpp
 * @brief Simple class to print single instruction
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef HLS_INSTRUCTION_WRITER_HPP
#define HLS_INSTRUCTION_WRITER_HPP
#include "instruction_writer.hpp"

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

class HLSInstructionWriter : public InstructionWriter
{
 public:
   /**
    * Constructor
    * @param app_man is the application manager
    * @param indented_output_stream is the output stream on which source code has to be written
    * @param parameters is the set of input parameters
    */
   HLSInstructionWriter(const application_managerConstRef app_man, const IndentedOutputStreamRef indented_output_stream,
                        const ParameterConstRef parameters);

   void declareFunction(const unsigned int function_id) override;
};
#endif
