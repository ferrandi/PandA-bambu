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

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_objects.hpp"

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
   i_q0,
   i_q1,
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_address0,
   o_address1,
   o_ce0,
   o_ce1,
   o_we0,
   o_we1,
   o_d0,
   o_d1,
   o_last
};

ReadWriteDP_arrayModuleGenerator::ReadWriteDP_arrayModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWriteDP_arrayModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod,
                                                    unsigned int function_id, vertex /* op_v */,
                                                    const HDLWriter_Language /* language */,
                                                    const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                    const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager()->GetRootFunctionFrom(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(top_fname);
   THROW_ASSERT(func_arch, "Expected function architecture for function " + top_fname);
   const auto arraySize =
       std::accumulate(func_arch->parms.begin(), func_arch->parms.end(), 0ULL, [&](auto& a, auto& it) {
          const auto& parm_attrs = it.second;
          if(parm_attrs.at(FunctionArchitecture::parm_bundle) == bundle_name)
          {
             THROW_ASSERT(parm_attrs.find(FunctionArchitecture::parm_elem_count) != parm_attrs.end(), "");
             return a + std::stoull(parm_attrs.at(FunctionArchitecture::parm_elem_count));
          }
          return a;
       });

   const auto isAlignedPowerOfTwo = _ports_in[i_in4].alignment == ceil_pow2(_ports_in[i_in4].alignment);
   const auto addressMaxValue = _ports_in[i_in4].alignment * arraySize - 1U;
   const auto nbitAddress =
       addressMaxValue <= 1ULL ? 1U : (64u - static_cast<unsigned>(__builtin_clzll(addressMaxValue)));

   out << "//" << (isAlignedPowerOfTwo ? "T" : "F") << "\n";
   out << "assign " << _ports_out[o_ce0].name << " = " << _ports_in[i_start].name << "[0];\n";
   out << "assign " << _ports_out[o_ce1].name << " = " << _ports_in[i_start].name << "[1];\n";

   if(isAlignedPowerOfTwo)
   {
      out << "assign " << _ports_out[o_address0].name << " = " << _ports_in[i_in4].name << "[BITSIZE_"
          << _ports_in[i_in4].name << "*0+:" << nbitAddress << "] / " << _ports_in[i_in4].alignment << ";\n";
      out << "assign " << _ports_out[o_address1].name << " = " << _ports_in[i_in4].name << "[BITSIZE_"
          << _ports_in[i_in4].name << "*1+:" << nbitAddress << "] / " << _ports_in[i_in4].alignment << ";\n";
   }
   else
   {
      out << "assign " << _ports_out[o_address0].name << " = " << _ports_in[i_in4].name << "[2+(BITSIZE_"
          << _ports_in[i_in4].name << ")*0+:" << nbitAddress - 2U << "] / " << _ports_in[i_in4].alignment / 4 << ";\n";
      out << "assign " << _ports_out[o_address1].name << " = " << _ports_in[i_in4].name << "[2+BITSIZE_"
          << _ports_in[i_in4].name << "*1+:" << nbitAddress - 2U << "] / " << _ports_in[i_in4].alignment / 4 << ";\n";
   }

   if(_ports_in.size() > i_q1)
   {
      out << "assign " << _ports_out[o_out1].name << "[BITSIZE_" << _ports_out[o_out1].name << "*0+:BITSIZE_"
          << _ports_out[o_out1].name << "] = " << _ports_in[i_q0].name << ";\n";
      out << "assign " << _ports_out[o_out1].name << "[BITSIZE_" << _ports_out[o_out1].name << "*1+:BITSIZE_"
          << _ports_out[o_out1].name << "] = " << _ports_in[i_q1].name << ";\n";
   }

   if(_ports_out.size() > o_d1)
   {
      out << "assign " << _ports_out[o_we0].name << " = " << _ports_in[i_start].name << "[0] & (|"
          << _ports_in[i_in1].name << "[BITSIZE_" << _ports_in[i_in1].name << "*0+:BITSIZE_" << _ports_in[i_in1].name
          << "]);\n";
      out << "assign " << _ports_out[o_d0].name << " = " << _ports_in[i_in3].name << "[BITSIZE_"
          << _ports_in[i_in3].name << "*0+:BITSIZE_" << _ports_in[i_in3].name << "];\n";
      out << "assign " << _ports_out[o_we1].name << " = " << _ports_in[i_start].name << "[1] & (|"
          << _ports_in[i_in1].name << "[BITSIZE_" << _ports_in[i_in1].name << "*1+:BITSIZE_" << _ports_in[i_in1].name
          << "]);\n";
      out << "assign " << _ports_out[o_d1].name << " = " << _ports_in[i_in3].name << "[BITSIZE_"
          << _ports_in[i_in3].name << "*1+:BITSIZE_" << _ports_in[i_in3].name << "];\n";
   }
}
