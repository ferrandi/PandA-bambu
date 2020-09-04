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
 *                 URL: http://trac.ws.dei.polimi.it/panda
 *                      Microarchitectures Laboratory
 *                       Politecnico di Milano - DEIB
 *             ***********************************************
 *              Copyright (c) 2018-2020 Politecnico di Milano
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
*/
/**
 * @file Read_axis.cpp
 * @brief Snippet for the Read_axis dynamic generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
*/

std::cout << "integer ii=0;\n";
std::cout << "reg [PORTSIZE_" << _ports_out[1].name << "-1:0] started 1INIT_ZERO_VALUE;\n";
std::cout << "reg [PORTSIZE_" << _ports_out[1].name << "-1:0] started0 1INIT_ZERO_VALUE;\n";
std::cout << "reg [(PORTSIZE_" << _ports_out[1].name << "*BITSIZE_" << _ports_out[1].name << ")-1:0] " << _ports_out[1].name << " ;\n";
std::cout << "reg [PORTSIZE_" << _ports_out[1].name << "-1:0] " << _ports_out[0].name << "0 1INIT_ZERO_VALUE;\n";

std::cout << "always @(*)\n";
std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_out[1].name << "; ii=ii+1)\n";
std::cout << "    started0[ii] <= (started[ii] | " << _ports_in[2].name << "[ii]) & !" << _ports_in[5].name << ";\n";
std::cout << "always @(posedge clock 1RESET_EDGE)\n";
std::cout << "  if (1RESET_VALUE)\n";
std::cout << "    started <= 0;\n";
std::cout << "  else\n";
std::cout << "    for(ii=0; ii<PORTSIZE_"<< _ports_out[1].name << "; ii=ii+1)\n";
std::cout << "      started[ii] <= started0[ii];\n";

std::cout << "always @(*)\n";
std::cout << "begin\n";
std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_out[1].name << "; ii=ii+1)\n";
std::cout << "    " << _ports_out[1].name << "[(BITSIZE_" << _ports_out[1].name <<")*ii+:BITSIZE_" << _ports_out[1].name<<"] = "<< _ports_in[4].name <<" >> (8*"<< _ports_in[3].name <<"[(BITSIZE_" << _ports_in[3].name <<")*ii+:BITSIZE_" << _ports_in[3].name<<"]);" <<std::endl;
std::cout << "end\n";

std::cout << "always @(*)\n";
std::cout << "begin\n";
std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_out[1].name << "; ii=ii+1)\n";
std::cout << "    " << _ports_out[0].name << "0[ii] = ("<< _ports_in[2].name <<"[ii] & " << _ports_in[5].name << ") | (started[ii] & " << _ports_in[5].name << ");" <<std::endl;
 std::cout << "end\n";

std::cout << "assign " << _ports_out[0].name << " = " << _ports_out[0].name << "0;\n";
std::cout << "assign " << _ports_out[2].name << " = (|" <<  _ports_in[2].name << ") | (|started);\n";

