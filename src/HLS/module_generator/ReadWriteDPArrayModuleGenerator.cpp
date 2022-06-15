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
 * @file ReadWriteDPArrayModuleGenerator.cpp
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

#include "ReadWriteDPArrayModuleGenerator.hpp"

#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "op_graph.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

ReadWriteDPArrayModuleGenerator::ReadWriteDPArrayModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWriteDPArrayModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */, unsigned int function_id,
                                                   vertex op_v, const HDLWriter_Language /* language */,
                                                   const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                   const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                   const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                   const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
   const auto cfg = FB->CGetOpGraph(FunctionBehavior::CFG);
   const auto op = cfg->CGetOpNodeInfo(op_v)->GetOperation();
   auto parameter_name = op.substr(0, op.find(STR_CST_interface_parameter_keyword));
   const auto TM = HLSMgr->get_tree_manager();
   const auto fnode = TM->CGetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(fnode);
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   const auto arraySize = HLSMgr->design_interface_arraysize.find(fname) != HLSMgr->design_interface_arraysize.end() &&
                                  HLSMgr->design_interface_arraysize.find(fname)->second.find(parameter_name) !=
                                      HLSMgr->design_interface_arraysize.find(fname)->second.end() ?
                              boost::lexical_cast<unsigned int>(
                                  HLSMgr->design_interface_arraysize.find(fname)->second.find(parameter_name)->second) :
                              1U;

   const auto isAlignedPowerOfTwo = _ports_out[1].alignment == round_to_power2(_ports_out[1].alignment);
   out << "//" << (isAlignedPowerOfTwo ? "T" : "F") << "\n";
   out << "integer ii=0;\n";
   out << "reg [" << _ports_out[1].type_size << "-1:0] " << _ports_out[1].name << ";\n";
   out << "reg [" << _ports_out[2].type_size << "-1:0] " << _ports_out[2].name << ";\n";
   if(_ports_out.size() == 9U)
   {
      out << "reg [" << _ports_out[7].type_size << "-1:0] " << _ports_out[7].name << ";\n";
      out << "reg [" << _ports_out[8].type_size << "-1:0] " << _ports_out[8].name << ";\n";
   }
   if(_ports_in.size() == 9U)
   {
      out << "reg [(PORTSIZE_" << _ports_out[0].name << "*BITSIZE_" << _ports_out[0].name << ")+(-1):0] "
          << _ports_out[0].name << ";\n";
   }

   const auto log2nbyte =
       _ports_out[1].alignment == 1U ? 0U : (32u - static_cast<unsigned>(__builtin_clz(_ports_out[1].alignment - 1U)));

   const auto addressMaxValue = _ports_out[1].alignment * arraySize - 1U;
   const auto nbitAddress = addressMaxValue == 1U ? 1U : (32u - static_cast<unsigned>(__builtin_clz(addressMaxValue)));

   if(log2nbyte > 0U)
   {
      out << "reg [(PORTSIZE_" << _ports_in[3].name << "*" << log2nbyte << ")+(-1):0] " << _ports_in[6].name << "_0;\n";
      out << "always @(*)\n";
      out << "begin\n";
      out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
      if(isAlignedPowerOfTwo)
      {
         out << "    " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte << "] = " << _ports_in[6].name
             << "[(BITSIZE_" << _ports_in[6].name << ")*ii+:" << log2nbyte << "];\n";
      }
      else
      {
         out << "    " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte << "] = " << _ports_in[6].name
             << "[2+(BITSIZE_" << _ports_in[6].name << ")*ii+:" << nbitAddress - 2U << "] % " << log2nbyte - 2U << "'d"
             << _ports_out[1].alignment / 4U << ";\n";
      }
      out << "end\n";
   }
   if(log2nbyte > 0U && _ports_in.size() == 9)
   {
      out << "reg [(PORTSIZE_" << _ports_in[3].name << "*" << log2nbyte << ")+(-1):0] " << _ports_in[6].name
          << "_reg;\n";
      out << "always @(posedge clock 1RESET_EDGE)\n";
      out << "  if (1RESET_VALUE)\n";
      out << "    " << _ports_in[6].name << "_reg <= 0;\n";
      out << "  else\n";
      out << "    for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
      out << "      " << _ports_in[6].name << "_reg[" << log2nbyte << "*ii+:" << log2nbyte
          << "] <= " << _ports_in[6].name << "_0[" << log2nbyte << "*ii+:" << log2nbyte << "];\n";
   }
   out << "always @(*)\n";
   out << "begin\n";
   out << "  " << _ports_out[1].name << " = {" << _ports_out[1].type_size << "{1'b1}};\n";
   out << "  " << _ports_out[2].name << " = {" << _ports_out[2].type_size << "{1'b1}};\n";
   out << "  if(" << _ports_in[2].name << "[0])\n";
   out << "  begin\n";
   if(isAlignedPowerOfTwo)
   {
      out << "    " << _ports_out[1].name << " = " << _ports_out[1].name << " & (" << _ports_in[6].name << "[(BITSIZE_"
          << _ports_in[6].name << ")*0+:" << nbitAddress << "] / " << _ports_out[1].alignment << ");\n";
   }
   else
   {
      out << "    " << _ports_out[1].name << " = " << _ports_out[1].name << " & (" << _ports_in[6].name
          << "[2+(BITSIZE_" << _ports_in[6].name << ")*0+:" << nbitAddress - 2U << "] / " << _ports_out[1].alignment / 4
          << ");\n";
   }
   out << "  end\n";
   out << "  if(" << _ports_in[2].name << "[1])\n";
   out << "  begin\n";
   if(isAlignedPowerOfTwo)
   {
      out << "    " << _ports_out[2].name << " = " << _ports_out[2].name << " & (" << _ports_in[6].name << "[(BITSIZE_"
          << _ports_in[6].name << ")*1+:" << nbitAddress << "] / " << _ports_out[2].alignment << ");\n";
   }
   else
   {
      out << "    " << _ports_out[2].name << " = " << _ports_out[2].name << " & (" << _ports_in[6].name
          << "[2+(BITSIZE_" << _ports_in[6].name << ")*1+:" << nbitAddress - 2U << "] / " << _ports_out[2].alignment / 4
          << ");\n";
   }
   out << "  end\n";
   out << "end\n";

   out << "assign " << _ports_out[3].name << " = " << _ports_in[2].name << "[0];\n";
   out << "assign " << _ports_out[4].name << " = " << _ports_in[2].name << "[1];\n";

   if(_ports_in.size() == 9U)
   {
      out << "always @(*)\n";
      out << "begin\n";
      if(log2nbyte > 0U)
      {
         out << "  " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name << ")*0+:BITSIZE_"
             << _ports_out[0].name << "] = " << _ports_in[7].name << " >> {" << _ports_in[6].name << "_reg["
             << log2nbyte << "*0+:" << log2nbyte << "],3'b0};\n";
      }
      else
      {
         out << "  " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name << ")*0+:BITSIZE_"
             << _ports_out[0].name << "] = " << _ports_in[7].name << ";\n";
      }
      if(log2nbyte > 0)
      {
         out << "  " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name << ")*1+:BITSIZE_"
             << _ports_out[0].name << "] = " << _ports_in[8].name << " >> {" << _ports_in[6].name << "_reg["
             << log2nbyte << "*1+:" << log2nbyte << "],3'b0};\n";
      }
      else
      {
         out << "  " << _ports_out[0].name << "[(BITSIZE_" << _ports_out[0].name << ")*1+:BITSIZE_"
             << _ports_out[0].name << "] = " << _ports_in[8].name << ";\n";
      }

      out << "end\n";
   }

   if(_ports_out.size() == 9U)
   {
      out << "assign " << _ports_out[5].name << " = (" << _ports_in[2].name << "[0]) & (|(" << _ports_in[3].name
          << "[(BITSIZE_" << _ports_in[3].name << ")*0+:BITSIZE_" << _ports_in[3].name << "]));\n";
      out << "assign " << _ports_out[6].name << " = (" << _ports_in[2].name << "[1]) & (|(" << _ports_in[3].name
          << "[(BITSIZE_" << _ports_in[3].name << ")*1+:BITSIZE_" << _ports_in[3].name << "]));\n";
      out << "always @(*)\n";
      out << "begin\n";
      out << "  " << _ports_out[7].name << " = 0;\n";
      out << "  " << _ports_out[8].name << " = 0;\n";
      out << "    if(" << _ports_in[2].name << "[0])\n";
      if(log2nbyte > 0U)
      {
         out << "      " << _ports_out[7].name << " = (" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*0+:BITSIZE_" << _ports_in[4].name << "]>=" << _ports_out[7].type_size << ")?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]:(" << _ports_out[7].name
             << "^((((BITSIZE_" << _ports_in[5].name << ">" << _ports_out[7].type_size << "?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]:{{("
             << _ports_out[7].type_size << "< BITSIZE_" << _ports_in[5].name << " ? 1 : " << _ports_out[7].type_size
             << "-BITSIZE_" << _ports_in[5].name << "){1'b0}}," << _ports_in[5].name << "[(BITSIZE_"
             << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]})<<{" << _ports_in[6].name << "_0["
             << log2nbyte << "*0+:" << log2nbyte << "],3'b0})^" << _ports_out[7].name << ") & (((" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*0+:BITSIZE_" << _ports_in[4].name << "]+{" << _ports_in[6].name
             << "_0[" << log2nbyte << "*0+:" << log2nbyte << "],3'b0})>" << _ports_out[7].type_size << ") ? ((({("
             << _ports_out[7].type_size << "){1'b1}})>>({" << _ports_in[6].name << "_0[" << log2nbyte
             << "*0+:" << log2nbyte << "],3'b0}))<<({" << _ports_in[6].name << "_0[" << log2nbyte << "*0+:" << log2nbyte
             << "],3'b0})) : ((((({(" << _ports_out[7].type_size << "){1'b1}})>>({" << _ports_in[6].name << "_0["
             << log2nbyte << "*0+:" << log2nbyte << "],3'b0}))<<({" << _ports_in[6].name << "_0[" << log2nbyte
             << "*0+:" << log2nbyte << "],3'b0}))<<(" << _ports_out[7].type_size << "-" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*0+:BITSIZE_" << _ports_in[4].name << "]-{" << _ports_in[6].name
             << "_0[" << log2nbyte << "*0+:" << log2nbyte << "],3'b0}))>>(" << _ports_out[7].type_size << "-"
             << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name << ")*0+:BITSIZE_" << _ports_in[4].name << "]-{"
             << _ports_in[6].name << "_0[" << log2nbyte << "*0+:" << log2nbyte << "],3'b0})))));\n";
      }
      else
      {
         out << "      " << _ports_out[7].name << " = (" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*0+:BITSIZE_" << _ports_in[4].name << "]>=" << _ports_out[7].type_size << ")?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]:(" << _ports_out[7].name
             << "^((((BITSIZE_" << _ports_in[5].name << ">" << _ports_out[7].type_size << "?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]:{{("
             << _ports_out[7].type_size << "< BITSIZE_" << _ports_in[5].name << " ? 1 : " << _ports_out[7].type_size
             << "-BITSIZE_" << _ports_in[5].name << "){1'b0}}," << _ports_in[5].name << "[(BITSIZE_"
             << _ports_in[5].name << ")*0+:BITSIZE_" << _ports_in[5].name << "]}))^" << _ports_out[7].name << ") & ((("
             << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name << ")*0+:BITSIZE_" << _ports_in[4].name << "])>"
             << _ports_out[7].type_size << ") ? ((({(" << _ports_out[7].type_size << "){1'b1}}))) : ((((({("
             << _ports_out[7].type_size << "){1'b1}})))<<(" << _ports_out[7].type_size << "-" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*0+:BITSIZE_" << _ports_in[4].name << "]))>>("
             << _ports_out[7].type_size << "-" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*0+:BITSIZE_" << _ports_in[4].name << "])))));\n";
      }
      out << "    if(" << _ports_in[2].name << "[1])\n";
      if(log2nbyte > 0U)
      {
         out << "      " << _ports_out[8].name << " = (" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*1+:BITSIZE_" << _ports_in[4].name << "]>=" << _ports_out[8].type_size << ")?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]:(" << _ports_out[8].name
             << "^((((BITSIZE_" << _ports_in[5].name << ">" << _ports_out[8].type_size << "?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]:{{("
             << _ports_out[8].type_size << "< BITSIZE_" << _ports_in[5].name << " ? 1 : " << _ports_out[8].type_size
             << "-BITSIZE_" << _ports_in[5].name << "){1'b0}}," << _ports_in[5].name << "[(BITSIZE_"
             << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]})<<{" << _ports_in[6].name << "_0["
             << log2nbyte << "*1+:" << log2nbyte << "],3'b0})^" << _ports_out[8].name << ") & (((" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*1+:BITSIZE_" << _ports_in[4].name << "]+{" << _ports_in[6].name
             << "_0[" << log2nbyte << "*1+:" << log2nbyte << "],3'b0})>" << _ports_out[8].type_size << ") ? ((({("
             << _ports_out[8].type_size << "){1'b1}})>>({" << _ports_in[6].name << "_0[" << log2nbyte
             << "*1+:" << log2nbyte << "],3'b0}))<<({" << _ports_in[6].name << "_0[" << log2nbyte << "*1+:" << log2nbyte
             << "],3'b0})) : ((((({(" << _ports_out[8].type_size << "){1'b1}})>>({" << _ports_in[6].name << "_0["
             << log2nbyte << "*1+:" << log2nbyte << "],3'b0}))<<({" << _ports_in[6].name << "_0[" << log2nbyte
             << "*1+:" << log2nbyte << "],3'b0}))<<(" << _ports_out[8].type_size << "-" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*1+:BITSIZE_" << _ports_in[4].name << "]-{" << _ports_in[6].name
             << "_0[" << log2nbyte << "*1+:" << log2nbyte << "],3'b0}))>>(" << _ports_out[8].type_size << "-"
             << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name << ")*1+:BITSIZE_" << _ports_in[4].name << "]-{"
             << _ports_in[6].name << "_0[" << log2nbyte << "*1+:" << log2nbyte << "],3'b0})))));\n";
      }
      else
      {
         out << "      " << _ports_out[8].name << " = (" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*1+:BITSIZE_" << _ports_in[4].name << "]>=" << _ports_out[8].type_size << ")?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]:(" << _ports_out[8].name
             << "^((((BITSIZE_" << _ports_in[5].name << ">" << _ports_out[8].type_size << "?" << _ports_in[5].name
             << "[(BITSIZE_" << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]:{{("
             << _ports_out[8].type_size << "< BITSIZE_" << _ports_in[5].name << " ? 1 : " << _ports_out[8].type_size
             << "-BITSIZE_" << _ports_in[5].name << "){1'b0}}," << _ports_in[5].name << "[(BITSIZE_"
             << _ports_in[5].name << ")*1+:BITSIZE_" << _ports_in[5].name << "]}))^" << _ports_out[8].name << ") & ((("
             << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name << ")*1+:BITSIZE_" << _ports_in[4].name << "])>"
             << _ports_out[8].type_size << ") ? ((({(" << _ports_out[8].type_size << "){1'b1}}))) : ((((({("
             << _ports_out[8].type_size << "){1'b1}})))<<(" << _ports_out[8].type_size << "-" << _ports_in[4].name
             << "[(BITSIZE_" << _ports_in[4].name << ")*1+:BITSIZE_" << _ports_in[4].name << "]))>>("
             << _ports_out[8].type_size << "-" << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
             << ")*1+:BITSIZE_" << _ports_in[4].name << "])))));\n";
      }
      out << "end\n";
   }
}