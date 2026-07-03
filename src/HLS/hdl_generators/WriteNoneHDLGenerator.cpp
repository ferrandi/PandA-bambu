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
 *              Copyright (C) 2022-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file WriteNoneHDLGenerator.cpp
 * @brief Implementation of the HDL generator for a write interface without handshake signals.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "WriteNoneHDLGenerator.hpp"

#include "behavioral_helper.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
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
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_last
};

WriteNoneHDLGenerator::WriteNoneHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void WriteNoneHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir, unsigned int function_id,
                                         gc_vertex_descriptor /* op_v */, const HDLWriter_Language language,
                                         const std::vector<HDLGenerator::parameter>& /* _p */,
                                         const std::vector<HDLGenerator::parameter>& _ports_in,
                                         const std::vector<HDLGenerator::parameter>& _ports_out,
                                         const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod_cir->get_id().substr(0, mod_cir->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   if(iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end())
   {
      if(language == HDLWriter_Language::VHDL)
      {
         out << "constant OUT_SIZE : integer = " << _ports_out[o_out1].name << "'length;\n"
             << "process(clock,reset)\n"
             << "begin\n"
             << "  if (1RESET_VALUE) then\n"
             << "    " << _ports_out[o_out1].name << " <= (OUT_SIZE - 1 downto 0 => '0');\n"
             << "  elsif (clock'event and clock='1') then\n"
             << "    if(unsigned(" << _ports_in[i_start].name << ") /= 0 ) then\n"
             << "      " << _ports_out[o_out1].name << " <= std_logic_vector(resize(unsigned(" << _ports_in[i_in3].name
             << "), OUT_SIZE));\n"
             << "    end if;\n"
             << "  end if;\n"
             << "end process;\n";
      }
      else
      {
         out << "reg [" << (_ports_out[o_out1].type_size - 1) << ":0] " << _ports_out[o_out1].name << ";\n";

         out << "always @(posedge clock 1RESET_EDGE)\n";
         out << "begin\n";
         out << "  if (1RESET_VALUE)\n";
         out << "    " << _ports_out[o_out1].name << " <= 0;\n";
         out << "  else if(" << _ports_in[i_start].name << ")\n";
         out << "    " << _ports_out[o_out1].name << " <= " << _ports_in[i_in3].name << ";\n";
         out << "end\n";
      }
   }
   else
   {
      if(language == HDLWriter_Language::VHDL)
      {
         out << "constant OUT_SIZE : integer = " << _ports_out[o_out1].name << "'length;\n"
             << "begin\n  " << _ports_out[o_out1].name << " <= std_logic_vector(resize(unsigned("
             << _ports_in[i_in3].name << "), OUT_SIZE));\n";
      }
      else
      {
         out << "assign " << _ports_out[o_out1].name << " = " << _ports_in[i_in3].name << ";\n";
      }
   }
}
