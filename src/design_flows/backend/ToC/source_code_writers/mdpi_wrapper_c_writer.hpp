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
 *              Copyright (C) 2024 Politecnico di Milano
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
 * @file mdpi_wrapper_c_writer.hpp
 * @brief
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef MDPI_WRAPPER_CWRITER_HPP
#define MDPI_WRAPPER_CWRITER_HPP
#include "c_writer.hpp"

class MdpiWrapperCWriter : public CWriter
{
   void WriteSimulatorInitMemory(const unsigned int function_id);

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