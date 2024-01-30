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
 * @file Write_noneModuleGenerator.cpp
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

#include "Write_noneModuleGenerator.hpp"

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
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_last
};

Write_noneModuleGenerator::Write_noneModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Write_noneModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir, unsigned int function_id,
                                             vertex /* op_v */, const HDLWriter_Language language,
                                             const std::vector<ModuleGenerator::parameter>& /* _p */,
                                             const std::vector<ModuleGenerator::parameter>& _ports_in,
                                             const std::vector<ModuleGenerator::parameter>& _ports_out,
                                             const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod_cir->get_id().substr(0, mod_cir->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   if(iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end())
   {
      if(language == HDLWriter_Language::VHDL)
      {
         out << "constant ones : std_logic_vector(\\" << _ports_out[o_out1].name << "\\'range) := (others => '1');\n";
         out << "constant threezeros : std_logic_vector(2 downto 0) := (others => '0');\n";
         out << "begin\n";
         out << "process(clock,reset)\n";
         out << "  variable \\" << _ports_out[o_out1].name << "_0\\ : std_logic_vector("
             << (_ports_out[o_out1].type_size - 1) << " downto 0);\n";
         out << "begin\n";
         out << "  if (1RESET_VALUE) then\n";
         out << "    \\" << _ports_out[o_out1].name << "\\ <= (others => '0');\n";
         out << "  elsif (clock'event and clock='1') then\n";
         out << "    if(unsigned(" << _ports_in[i_start].name << ") /= 0 ) then\n";
         out << "      \\" << _ports_out[o_out1].name << "\\ <= std_logic_vector(resize(unsigned("
             << _ports_in[i_in2].name << "), " << _ports_out[o_out1].type_size << "));\n";
         out << "    end if;\n";
         out << "  end if;\n";
         out << "end process;\n";
      }
      else
      {
         out << "reg [" << (_ports_out[o_out1].type_size - 1) << ":0] " << _ports_out[o_out1].name << ";\n";

         out << "always @(posedge clock 1RESET_EDGE)\n";
         out << "begin\n";
         out << "  if (1RESET_VALUE)\n";
         out << "    " << _ports_out[o_out1].name << " <= 0;\n";
         out << "  else if(" << _ports_in[i_start].name << ")\n";
         out << "    " << _ports_out[o_out1].name << " <= " << _ports_in[i_in2].name << ";\n";
         out << "end\n";
      }
   }
   else
   {
      if(language == HDLWriter_Language::VHDL)
      {
         out << "begin\n  \\" << _ports_out[o_out1].name << "\\ <= std_logic_vector(resize(unsigned("
             << _ports_in[i_in2].name << "), " << _ports_out[o_out1].type_size << "));\n";
      }
      else
      {
         out << "assign " << _ports_out[o_out1].name << " = " << _ports_in[i_in2].name << ";\n";
      }
   }
}