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
 * @file ReadWriteDP_arrayModuleGenerator.cpp
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

#include "ReadWriteDP_arrayModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "op_graph.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

ReadWriteDP_arrayModuleGenerator::ReadWriteDP_arrayModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWriteDP_arrayModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */,
                                                    unsigned int function_id, vertex op_v,
                                                    const HDLWriter_Language /* language */,
                                                    const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                    const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto fname = [&]() {
      const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
      if(op_v)
      {
         const auto cfg = FB->CGetOpGraph(FunctionBehavior::CFG);
         return cfg->CGetOpNodeInfo(op_v)->GetOperation();
      }
      return FB->CGetBehavioralHelper()->get_function_name();
   }();
   THROW_ASSERT(fname.find(STR_CST_interface_parameter_keyword) != std::string::npos,
                "Unexpected array interface module name");
   const auto parameter_name = fname.substr(0, fname.find(STR_CST_interface_parameter_keyword));
   auto foundParam = false;
   auto arraySize = 1U;
   const auto TM = HLSMgr->get_tree_manager();
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   for(const auto& f_props : HLSMgr->design_attributes)
   {
      const auto& name = f_props.first;
      const auto& props = f_props.second;
      const auto id = TM->function_index_mngl(name);
      if(top_functions.count(id) && props.find(parameter_name) != props.end() &&
         props.at(parameter_name).find(attr_size) != props.at(parameter_name).end() && !foundParam)
      {
         arraySize = boost::lexical_cast<decltype(arraySize)>(props.at(parameter_name).at(attr_size));
         foundParam = true;
      }
      else if(foundParam)
      {
         THROW_ERROR("At least two top functions have the same array parameter");
      }
   }

   const auto isAlignedPowerOfTwo = _ports_out[1].alignment == ceil_pow2(_ports_out[1].alignment);
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

   const auto log2nbyte = _ports_out[1].alignment == 1ULL ?
                              0U :
                              (64u - static_cast<unsigned>(__builtin_clzll(_ports_out[1].alignment - 1U)));

   const auto addressMaxValue = _ports_out[1].alignment * arraySize - 1U;
   const auto nbitAddress =
       addressMaxValue <= 1ULL ? 1U : (64u - static_cast<unsigned>(__builtin_clzll(addressMaxValue)));

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
