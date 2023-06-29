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
 * @file ReadWrite_arrayModuleGenerator.cpp
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

#include "ReadWrite_arrayModuleGenerator.hpp"

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

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
   i_q,
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_address,
   o_ce,
   o_we,
   o_d,
   o_last
};

ReadWrite_arrayModuleGenerator::ReadWrite_arrayModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWrite_arrayModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */,
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
   const auto arg_name = fname.substr(0, fname.find(STR_CST_interface_parameter_keyword));
   const auto top_id = *HLSMgr->CGetCallGraphManager()->GetRootFunctions().begin();
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetMangledFunctionName();
   THROW_ASSERT(HLSMgr->design_attributes.count(top_fname) && HLSMgr->design_attributes.at(top_fname).count(arg_name),
                "Parameter " + arg_name + " not found in function " + top_fname);
   const auto DesignAttributes = HLSMgr->design_attributes.at(top_fname).at(arg_name);
   THROW_ASSERT(DesignAttributes.count(attr_interface_factor), "");
   THROW_ASSERT(DesignAttributes.count(attr_size), "");
   const auto factor = boost::lexical_cast<unsigned>(DesignAttributes.at(attr_interface_factor));
   const auto arraySize = boost::lexical_cast<unsigned>(DesignAttributes.at(attr_size));

   const auto isAlignedPowerOfTwo = _ports_in[i_in4].alignment == ceil_pow2(_ports_in[i_in4].alignment);
   const auto addressMaxValue = factor * _ports_in[i_in4].alignment * arraySize - 1U;
   const auto nbitAddress =
       addressMaxValue <= 1U ? 1U : (64u - static_cast<unsigned>(__builtin_clzll(addressMaxValue)));

   out << "//" << (isAlignedPowerOfTwo ? "T" : "F") << "\n";
   out << "assign " << _ports_out[o_ce].name << " = " << _ports_in[i_start].name << "[0];\n";

   if(isAlignedPowerOfTwo)
   {
      out << "assign " << _ports_out[o_address].name << " = " << _ports_in[i_in4].name << "[BITSIZE_"
          << _ports_in[i_in4].name << "*0+:" << nbitAddress << "] / " << _ports_in[i_in4].alignment << ";\n";
   }
   else
   {
      out << "assign " << _ports_out[o_address].name << " = " << _ports_in[i_in4].name << "[2+BITSIZE_"
          << _ports_in[i_in4].name << "*0+:" << nbitAddress - 2U << "] / " << _ports_in[i_in4].alignment / 4 << ";\n";
   }

   if(_ports_in.size() > i_q)
   {
      out << "assign " << _ports_out[o_out1].name << "[BITSIZE_" << _ports_out[o_out1].name << "*0+:BITSIZE_"
          << _ports_out[o_out1].name << "] = " << _ports_in[i_q].name << ";\n";
   }

   if(_ports_out.size() > o_d)
   {
      out << "assign " << _ports_out[o_we].name << " = " << _ports_in[i_start].name << "[0] & (|"
          << _ports_in[i_in1].name << "[BITSIZE_" << _ports_in[i_in1].name << "*0+:BITSIZE_" << _ports_in[i_in1].name
          << "]);\n";
      out << "assign " << _ports_out[o_d].name << " = " << _ports_in[i_in3].name << "[BITSIZE_" << _ports_in[i_in3].name
          << "*0+:BITSIZE_" << _ports_in[i_in3].name << "];\n";
   }
}
