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
 * @file Write_acknowledgeModuleGenerator.cpp
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

#include "Write_acknowledgeModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
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
   i_ack,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_last
};

Write_acknowledgeModuleGenerator::Write_acknowledgeModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Write_acknowledgeModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod,
                                                    unsigned int function_id, vertex /* op_v */,
                                                    const HDLWriter_Language language,
                                                    const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                    const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                    const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager()->GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);
   const auto registered = iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end();

   if(registered)
   {
      out << "reg acked;\n";
      out << "wire ack_next;\n";

      out << "always @(posedge clock 1RESET_EDGE)\n";
      out << "begin\n";
      out << "  if (1RESET_VALUE)\n";
      out << "  begin\n";
      out << "    acked <= 0;\n";
      out << "  end\n";
      out << "  else\n";
      out << "  begin\n";
      out << "    acked <= ack_next;\n";
      out << "  end\n";
      out << "end\n\n";

      out << "assign ack_next = " << _ports_in[i_ack].name << ";\n";
   }
   out << "reg started;\n";
   out << "wire started_next;\n\n";

   out << "always @(posedge clock 1RESET_EDGE)\n";
   out << "begin\n";
   out << "  if (1RESET_VALUE)\n";
   out << "  begin\n";
   out << "    started <= 0;\n";
   out << "  end\n";
   out << "  else\n";
   out << "  begin\n";
   out << "    started <= started_next;\n";
   out << "  end\n";
   out << "end\n\n";

   out << "assign " << _ports_out[o_out1].name << " = " << _ports_in[i_in2].name << ";\n";

   if(registered)
   {
      out << "assign started_next = (" << _ports_in[i_start].name << " | started) & ~(started & acked);\n";
      out << "assign " << _ports_out[o_done].name << " = started & acked;\n";
   }
   else
   {
      out << "assign started_next = (" << _ports_in[i_start].name << " | started) & ~" << _ports_in[i_ack].name
          << ";\n";
      out << "assign " << _ports_out[o_done].name << " = (" << _ports_in[i_start].name << " | started) & "
          << _ports_in[i_ack].name << ";\n";
   }
}