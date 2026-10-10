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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file csim_testbench_c_writer.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef CSIM_TESTBENCH_CWRITER_HPP
#define CSIM_TESTBENCH_CWRITER_HPP
#include "c_writer.hpp"

#include <filesystem>

class CSimTestbenchCWriter : public CWriter
{
   const std::filesystem::path csim_output;

   const std::filesystem::path bbp_output;

   void InternalInitialize() final;

   void InternalWriteHeader() final;

   void InternalWriteGlobalDeclarations() final;

   void InternalWriteFile() final;

 public:
   CSimTestbenchCWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef instruction_writer,
                        const IndentedOutputStreamRef indented_output_stream, const std::filesystem::path& csim_output,
                        const std::filesystem::path& bbp_output = "");

   ~CSimTestbenchCWriter() override = default;
};

#endif // CSIM_TESTBENCH_CWRITER_HPP
