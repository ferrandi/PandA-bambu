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
 *              Copyright (c) 2018-2019 Politecnico di Milano
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
 * @file ReadWrite_array.cpp
 * @brief Snippet for the ReadWrite_array dynamic generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
*/

bool isAlignedPowerOfTwo=_ports_out[1].alignment==RUPNP2(_ports_out[1].alignment);
std::cout << "//"<< (isAlignedPowerOfTwo?"T":"F")<<"\n";
std::cout << "integer ii=0;\n";
std::cout << "reg [" << _ports_out[1].type_size << "-1:0] " << _ports_out[1].name << ";\n";
if(_np_out==5)
  std::cout << "reg [" << _ports_out[4].type_size << "-1:0] " << _ports_out[4].name << ";\n";
if(_np_in==8)
  std::cout << "reg [(PORTSIZE_" << _ports_out[0].name << "*BITSIZE_" << _ports_out[0].name << ")+(-1):0] " << _ports_out[0].name << ";\n";

unsigned int log2nbyte= _ports_out[1].alignment==1 ? 0 : (32u-static_cast<unsigned>(__builtin_clz(_ports_out[1].alignment-1)));

auto addressMaxValue=_ports_out[1].alignment*boost::lexical_cast<unsigned>(_specializing_string)-1;
unsigned int nbitAddress= addressMaxValue == 1 ? 1 : (32u-static_cast<unsigned>(__builtin_clz(addressMaxValue)));

if(log2nbyte>0)
{
  std::cout << "reg [(PORTSIZE_" << _ports_in[3].name << "*" << log2nbyte << ")+(-1):0] " << _ports_in[6].name << "_0;\n";
  std::cout << "always @(*)\n";
  std::cout << "begin\n";
  std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_in[3].name << "; ii=ii+1)\n";
  if(isAlignedPowerOfTwo)
    std::cout << "    " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "] = " << _ports_in[6].name << "[(BITSIZE_" << _ports_in[6].name <<")*ii+:" << log2nbyte <<"];\n";
  else
    std::cout << "    " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "] = " << _ports_in[6].name << "[2+(BITSIZE_" << _ports_in[6].name <<")*ii+:" << nbitAddress-2 <<"] % " << log2nbyte-2 << "'d"<< _ports_out[1].alignment/4 << ";\n";
  std::cout << "end\n";
}
if(log2nbyte>0 && _np_in == 8)
{
  std::cout << "reg [(PORTSIZE_" << _ports_in[3].name << "*" << log2nbyte << ")+(-1):0] " << _ports_in[6].name << "_reg;\n";
  std::cout << "always @(posedge clock 1RESET_EDGE)\n";
  std::cout << "  if (1RESET_VALUE)\n";
  std::cout << "    " << _ports_in[6].name << "_reg <= 0;\n";
  std::cout << "  else\n";
  std::cout << "    for(ii=0; ii<PORTSIZE_"<< _ports_in[3].name << "; ii=ii+1)\n";
  std::cout << "      " << _ports_in[6].name << "_reg[" << log2nbyte << "*ii+:" << log2nbyte << "] <= " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "];\n";
}
std::cout << "always @(*)\n";
std::cout << "begin\n";
std::cout << "  " << _ports_out[1].name << " = {" << _ports_out[1].type_size << "{1'b1}};\n";
std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_in[3].name << "; ii=ii+1)\n";
std::cout << "  begin\n";
std::cout << "    if(" << _ports_in[2].name << "[ii])\n";
std::cout << "    begin\n";
if(isAlignedPowerOfTwo)
  std::cout << "      " << _ports_out[1].name << " = " << _ports_out[1].name << " & ("<< _ports_in[6].name << "[(BITSIZE_" << _ports_in[6].name <<")*ii+:" << nbitAddress <<"] / " << _ports_out[1].alignment << ");\n";
else
  std::cout << "      " << _ports_out[1].name << " = " << _ports_out[1].name << " & ("<< _ports_in[6].name << "[2+(BITSIZE_" << _ports_in[6].name <<")*ii+:" << nbitAddress-2 <<"] / " << _ports_out[1].alignment/4 << ");\n";
std::cout << "    end\n";
std::cout << "  end\n";
std::cout << "end\n";

std::cout << "assign " << _ports_out[2].name << " = |" << _ports_in[2].name << ";\n";

if(_np_in==8)
{
  std::cout << "always @(*)\n";
  std::cout << "begin\n";
  std::cout << "  for(ii=0; ii<PORTSIZE_"<< _ports_out[0].name << "; ii=ii+1)\n";
  if(log2nbyte>0)
    std::cout << "    " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name <<")*ii+:BITSIZE_" << _ports_out[0].name<<"] = "<< _ports_in[7].name <<" >> {"<< _ports_in[6].name << "_reg[" << log2nbyte << "*ii+:" << log2nbyte << "],3'b0};" <<std::endl;
  else
    std::cout << "    " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name <<")*ii+:BITSIZE_" << _ports_out[0].name<<"] = "<< _ports_in[7].name <<";" <<std::endl;

  std::cout << "end\n";
}


if(_np_out==5)
  std::cout << "assign " << _ports_out[3].name << " = (|" << _ports_in[2].name << ") & (|" << _ports_in[3].name << ");\n";


if(_np_out==5)
{
  std::cout << "always @(*)\n";
  std::cout << "begin\n";
  std::cout << "  " << _ports_out[4].name << " = 0;\n";
  std::cout << "  for(ii=0; ii<PORTSIZE_"<<  _ports_in[3].name << "; ii=ii+1)\n";
  std::cout << "  begin\n";
  std::cout << "    if(ii==0 || " << _ports_in[2].name << "[ii])\n";
  if(log2nbyte>0)
    std::cout << "      " << _ports_out[4].name << " = (" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]>=" << _ports_out[4].type_size << ")?" <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]:(" << _ports_out[4].name << "^((((BITSIZE_" <<  _ports_in[5].name <<">" << _ports_out[4].type_size << "?" <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]:{{(" << _ports_out[4].type_size << "< BITSIZE_" <<  _ports_in[5].name <<" ? 1 : " << _ports_out[4].type_size << "-BITSIZE_" <<  _ports_in[5].name <<"){1'b0}}," <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]})<<{" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0})^" << _ports_out[4].name << ") & (((" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]+{" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0})>" << _ports_out[4].type_size << ") ? ((({(" << _ports_out[4].type_size << "){1'b1}})>>({" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0}))<<({" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0})) : ((((({(" << _ports_out[4].type_size << "){1'b1}})>>({" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0}))<<({" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0}))<<(" << _ports_out[4].type_size << "-" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]-{" <<  _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0}))>>(" << _ports_out[4].type_size << "-" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]-{" << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte<< "],3'b0})))));\n";
  else
    std::cout << "      " << _ports_out[4].name << " = (" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]>=" << _ports_out[4].type_size << ")?" <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]:(" << _ports_out[4].name << "^((((BITSIZE_" <<  _ports_in[5].name <<">" << _ports_out[4].type_size << "?" <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]:{{(" << _ports_out[4].type_size << "< BITSIZE_" <<  _ports_in[5].name <<" ? 1 : " << _ports_out[4].type_size << "-BITSIZE_" <<  _ports_in[5].name <<"){1'b0}}," <<  _ports_in[5].name <<"[(BITSIZE_" <<  _ports_in[5].name <<")*ii+:BITSIZE_" <<  _ports_in[5].name<<"]}))^" << _ports_out[4].name << ") & (((" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"])>" << _ports_out[4].type_size << ") ? ((({(" << _ports_out[4].type_size << "){1'b1}}))) : ((((({(" << _ports_out[4].type_size << "){1'b1}})))<<(" << _ports_out[4].type_size << "-" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"]))>>(" << _ports_out[4].type_size << "-" <<  _ports_in[4].name <<"[(BITSIZE_" <<  _ports_in[4].name <<")*ii+:BITSIZE_" <<  _ports_in[4].name<<"])))));\n";
  std::cout << "  end\n";
  std::cout << "end\n";
}
