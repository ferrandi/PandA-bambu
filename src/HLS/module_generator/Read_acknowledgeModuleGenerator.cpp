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
 *              Copyright (C) 2022-2023 Politecnico di Milano
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
 * @file Read_acknowledgeModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Read_acknowledgeModuleGenerator.hpp"

#include "language_writer.hpp"

enum in_port
{
   i_start = 0,
   i_in1,
   i_in3,
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_ack,
   o_last
};

Read_acknowledgeModuleGenerator::Read_acknowledgeModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Read_acknowledgeModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */,
                                                   unsigned int /* function_id */, vertex /* op_v */,
                                                   const HDLWriter_Language /* language */,
                                                   const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                   const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                   const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                   const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");
   out << "reg [BITSIZE_" << _ports_out[o_out1].name << "-1:0] " << _ports_out[o_out1].name << ";\n\n";

   out << "always @(*)\n";
   out << "begin\n";
   out << "  " << _ports_out[o_out1].name << " = " << _ports_in[i_in3].name << " >> (8*" << _ports_in[i_in1].name
       << ");\n";
   out << "end\n\n";

   out << "assign " << _ports_out[o_ack].name << " = " << _ports_in[i_start].name << ";\n";
}