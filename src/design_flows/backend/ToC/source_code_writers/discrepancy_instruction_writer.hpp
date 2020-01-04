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
 *              Copyright (C) 2019-2020 Politecnico di Milano
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
 * @file discrepancy_instruction_writer.hpp
 * @brief specialization of the instrunction writer for the discrepancy analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DISCREPANCY_INSTRUCTION_WRITER_HPP
#define DISCREPANCY_INSTRUCTION_WRITER_HPP

/// Superclass include
#include "hls_instruction_writer.hpp"

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

class discrepancy_instruction_writer : public HLSInstructionWriter
{
 public:
   /**
    * Constructor
    * @param app_man is the application manager
    * @param indented_output_stream is the output stream on which source code has to be written
    * @param parameters is the set of input parameters
    */
   discrepancy_instruction_writer(const application_managerConstRef app_man, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~discrepancy_instruction_writer() override;

   void declareFunction(const unsigned int function_id) override;
};
#endif
