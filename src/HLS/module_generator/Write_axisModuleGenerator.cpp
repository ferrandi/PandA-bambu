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
 *              Copyright (C) 2022-2022 Politecnico di Milano
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
 * @file Write_axisModuleGenerator.cpp
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

#include "Write_axisModuleGenerator.hpp"

#include "language_writer.hpp"

Write_axisModuleGenerator::Write_axisModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Write_axisModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */, unsigned int /* function_id */,
                                             vertex /* op_v */, const HDLWriter_Language /* language */,
                                             const std::vector<ModuleGenerator::parameter>& /* _p */,
                                             const std::vector<ModuleGenerator::parameter>& _ports_in,
                                             const std::vector<ModuleGenerator::parameter>& _ports_out,
                                             const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   out << "integer ii=0;\n";
   out << "reg [PORTSIZE_" << _ports_in[3].name << "-1:0] started 1INIT_ZERO_VALUE;\n";
   out << "reg [PORTSIZE_" << _ports_in[3].name << "-1:0] started0 1INIT_ZERO_VALUE;\n";
   out << "reg [" << _ports_out[1].type_size << "-1:0] " << _ports_out[1].name << "_0;\n";
   out << "reg [PORTSIZE_" << _ports_in[3].name << "-1:0] " << _ports_out[0].name << "0 1INIT_ZERO_VALUE;\n";

   out << "always @(*)\n";
   out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
   out << "    started0[ii] = (started[ii] | " << _ports_in[2].name << "[ii]) & !" << _ports_in[6].name << ";\n";
   out << "always @(posedge clock 1RESET_EDGE)\n";
   out << "  if (1RESET_VALUE)\n";
   out << "    started <= 0;\n";
   out << "  else\n";
   out << "    for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
   out << "      started[ii] <= started0[ii];\n";

   out << "assign " << _ports_out[1].name << " = " << _ports_out[1].name << "_0;\n";
   out << "always @(*)\n";
   out << "begin\n";
   out << "  " << _ports_out[1].name << "_0 = 0;\n";
   out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
   out << "  begin\n";
   out << "    " << _ports_out[1].name << "_0 = (" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
       << ")*ii+:BITSIZE_" << _ports_in[3].name << "]>=" << _ports_out[1].type_size << ")?" << _ports_in[4].name
       << "[(BITSIZE_" << _ports_in[4].name << ")*ii+:BITSIZE_" << _ports_in[4].name << "]:(" << _ports_out[1].name
       << "_0^((((BITSIZE_" << _ports_in[4].name << ">=" << _ports_out[1].type_size << "?" << _ports_in[4].name
       << "[(BITSIZE_" << _ports_in[4].name << ")*ii+:BITSIZE_" << _ports_in[4].name << "]:{{("
       << _ports_out[1].type_size << "<BITSIZE_" << _ports_in[4].name << " ? 1 : " << _ports_out[1].type_size
       << "-BITSIZE_" << _ports_in[4].name << "){1'b0}}," << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
       << ")*ii+:BITSIZE_" << _ports_in[4].name << "]})<<" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name
       << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)^" << _ports_out[1].name << "_0) & (((" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]+" << _ports_in[5].name
       << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)>"
       << _ports_out[1].type_size << ") ? ((({(" << _ports_out[1].type_size << "){1'b1}})>>(" << _ports_in[5].name
       << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<(" << _ports_in[5].name
       << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)) : ((((({("
       << _ports_out[1].type_size << "){1'b1}})>>(" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name
       << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<(" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name
       << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<(" << _ports_out[1].type_size << "-" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]-" << _ports_in[5].name
       << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))>>("
       << _ports_out[1].type_size << "-" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_"
       << _ports_in[3].name << "]-" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_"
       << _ports_in[5].name << "]*8)))));\n";
   out << "  end\n";
   out << "end\n";

   out << "always @(*)\n";
   out << "begin\n";
   out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
   out << "    " << _ports_out[0].name << "0[ii] = (" << _ports_in[2].name << "[ii] & " << _ports_in[6].name
       << ") | (started[ii] & " << _ports_in[6].name << ") ;\n";
   out << "end\n";

   out << "assign " << _ports_out[0].name << " = " << _ports_out[0].name << "0;\n";

   out << "assign " << _ports_out[2].name << " = (|" << _ports_in[2].name << ") | (|started);\n";
}