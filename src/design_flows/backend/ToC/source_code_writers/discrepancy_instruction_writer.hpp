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
 *   Copyright (C) 2019-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file discrepancy_instruction_writer.hpp
 * @brief specialization of the instruction writer for the discrepancy analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef DISCREPANCY_INSTRUCTION_WRITER_HPP
#define DISCREPANCY_INSTRUCTION_WRITER_HPP

#include "hls_instruction_writer.hpp"

class discrepancy_instruction_writer : public HLSInstructionWriter
{
 public:
   /**
    * Constructor
    * @param app_man is the application manager
    * @param indented_output_stream is the output stream on which source code has to be written
    * @param parameters is the set of input parameters
    */
   discrepancy_instruction_writer(const application_managerConstRef app_man,
                                  const IndentedOutputStreamRef indented_output_stream,
                                  const ParameterConstRef parameters);

   void declareFunction(const unsigned int function_id) final;
};
#endif
