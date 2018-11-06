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
 *                       Politecnico di Milano - DEI
 *             ***********************************************
 *              Copyright (c) 2018 Politecnico di Milano
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
 * @file Write_noneDS.cpp
 * @brief Snippet for the ReadWrite_noneDS dynamic generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
*/

std::cout << "integer ii=0;\n";
std::cout << "reg [" << _ports_out[0].type_size << "-1:0] reg_" << _ports_out[0].name << "_frst 1INIT_ZERO_VALUE;\n";
std::cout << "reg [" << _ports_out[0].type_size << "-1:0] reg_" << _ports_out[0].name << "_snd;\n";
std::cout << "assign " << _ports_out[0].name << " = reg_" << _ports_out[0].name << "_frst;\n";
std::cout << "always @(*)\n";
std::cout << "begin\n";
std::cout << "  reg_" << _ports_out[0].name << "_snd = reg_" << _ports_out[0].name << "_frst;\n";
std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_in[3].name << "; ii=ii+1)\n";
std::cout << "  begin\n";
std::cout << "    if (start_port[ii])\n";
std::cout << "      reg_" << _ports_out[0].name << "_snd = (" << _ports_in[3].name <<"[(BITSIZE_" << _ports_in[3].name <<")*ii+:BITSIZE_" << _ports_in[3].name<<"]>=" << _ports_out[0].type_size << ")?" << _ports_in[4].name <<"[(BITSIZE_" << _ports_in[4].name <<")*ii+:BITSIZE_" << _ports_in[4].name<<"]:(reg_" << _ports_out[0].name << "_snd^((((BITSIZE_" << _ports_in[4].name <<">=" << _ports_out[0].type_size << "?" << _ports_in[4].name <<"[(BITSIZE_" << _ports_in[4].name <<")*ii+:BITSIZE_" << _ports_in[4].name<<"]:{{(" << _ports_out[0].type_size << "-BITSIZE_" << _ports_in[4].name <<"){1'b0}}," << _ports_in[4].name <<"[(BITSIZE_" << _ports_in[4].name <<")*ii+:BITSIZE_" << _ports_in[4].name<<"]})<<" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8)^reg_" << _ports_out[0].name << "_snd) & (((" << _ports_in[3].name <<"[(BITSIZE_" << _ports_in[3].name <<")*ii+:BITSIZE_" << _ports_in[3].name<<"]+" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8)>" << _ports_out[0].type_size << ") ? ((({(" << _ports_out[0].type_size << "){1'b1}})>>(" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8))<<(" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8)) : ((((({(" << _ports_out[0].type_size << "){1'b1}})>>(" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8))<<(" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8))<<(" << _ports_out[0].type_size << "-" << _ports_in[3].name <<"[(BITSIZE_" << _ports_in[3].name <<")*ii+:BITSIZE_" << _ports_in[3].name<<"]-" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8))>>(" << _ports_out[0].type_size << "-" << _ports_in[3].name <<"[(BITSIZE_" << _ports_in[3].name <<")*ii+:BITSIZE_" << _ports_in[3].name<<"]-" << _ports_in[5].name <<"[(BITSIZE_" << _ports_in[5].name <<")*ii+:BITSIZE_" << _ports_in[5].name<<"]*8)))));\n";
std::cout << "  end\n";
std::cout << "end\n";

std::cout << "always @(posedge clock 1RESET_EDGE)\n";
std::cout << "  if (1RESET_VALUE)\n";
std::cout << "    reg_" << _ports_out[0].name << "_frst <= 0;\n";
std::cout << "  else\n";
std::cout << "    reg_" << _ports_out[0].name << "_frst <= reg_" << _ports_out[0].name << "_snd;\n";


