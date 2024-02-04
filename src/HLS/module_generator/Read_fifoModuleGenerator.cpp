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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
 * @file Read_fifoModuleGenerator.cpp
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

#include "Read_fifoModuleGenerator.hpp"

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
   i_dout,
   i_empty_n,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_read,
   o_last
};

Read_fifoModuleGenerator::Read_fifoModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Read_fifoModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                                            vertex /* op_v */, const HDLWriter_Language /* language */,
                                            const std::vector<ModuleGenerator::parameter>& /* _p */,
                                            const std::vector<ModuleGenerator::parameter>& _ports_in,
                                            const std::vector<ModuleGenerator::parameter>& _ports_out,
                                            const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager()->GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   if(iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end())
   {
      THROW_ERROR("Registered FIFO interface not yet implemented.");
   }

   out << "reg started, started_0;\n"
       << "reg done_0;\n\n";

   out << "always @(posedge clock 1RESET_EDGE)\n"
       << "begin\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    started <= 0;\n"
       << "  end\n"
       << "  else\n"
       << "  begin\n"
       << "    started <= started_0;\n"
       << "  end\n"
       << "end\n\n";

   out << "always @(*)\n"
       << "begin\n"
       << "  started_0 = (started | " << _ports_in[i_start].name << ") & ~" << _ports_in[i_empty_n].name << ";\n"
       << "  done_0 = ( started | " << _ports_in[i_start].name << ") & " << _ports_in[i_empty_n].name << ";\n"
       << "end\n\n";

   out << "assign " << _ports_out[o_out1].name << " = {" << _ports_in[i_empty_n].name << ", " << _ports_in[i_dout].name
       << "};\n";
   out << "assign " << _ports_out[o_done].name << " = done_0;\n";
   out << "assign " << _ports_out[o_read].name << " = done_0;\n";
}