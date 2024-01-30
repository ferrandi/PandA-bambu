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
 *              Copyright (C) 2023-2023 Politecnico di Milano
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
 * @file TestbenchAcknowledgeModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchAcknowledgeModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "utility.hpp"

TestbenchAcknowledgeModuleGenerator::TestbenchAcknowledgeModuleGenerator(const HLS_managerRef& _HLSMgr)
    : Registrar(_HLSMgr)
{
}

void TestbenchAcknowledgeModuleGenerator::InternalExec(
    std::ostream& out, structural_objectRef mod_cir, unsigned int function_id, vertex /* op_v */,
    const HDLWriter_Language language, const std::vector<ModuleGenerator::parameter>& /* _p */,
    const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
    const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
    const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_acknowledge_") - 1U, std::string::npos);

   const auto top_fname = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(arg_name);
   const auto if_dir = port_o::to_port_direction(iface_attrs.at(FunctionArchitecture::iface_direction));
   const std::string in_suffix = if_dir == port_o::IO ? "_i" : "";
   const std::string out_suffix = if_dir == port_o::IO ? "_o" : "";
   std::string np_library = mod_cir->get_id() + " index";
   std::vector<std::string> ip_components;
   const auto add_port_parametric = [&](const std::string& suffix, port_o::port_direction dir, unsigned port_size) {
      const auto port_name = arg_name + suffix;
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
   };
   out << "localparam BITSIZE_data=BITSIZE_" << arg_name << (in_suffix.size() ? in_suffix : out_suffix) << ";\n";
   if(if_dir == port_o::IN || if_dir == port_o::IO)
   {
      add_port_parametric(in_suffix, port_o::OUT, 1U);
      add_port_parametric(in_suffix + "_ack", port_o::IN, 0U);
      ip_components.push_back("TestbenchFifoRead");
      out << "TestbenchFifoRead #(.index(index),\n"
          << "  .CHECK_ACK(1),\n"
          << "  .BITSIZE_dout(BITSIZE_data)) fifo_read(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .read(" << arg_name << in_suffix << "_ack),\n"
          << "  .dout(" << arg_name << in_suffix << "));\n";
   }
   if(if_dir == port_o::OUT || if_dir == port_o::IO)
   {
      add_port_parametric(out_suffix, port_o::IN, 1U);
      add_port_parametric(out_suffix + "_ack", port_o::OUT, 0U);
      ip_components.push_back("TestbenchFifoWrite");
      out << "initial $display(\"BEAWARE: Output acknowledge interface will read output at each clock cycle\");\n\n"
          << "TestbenchFifoWrite #(.index(index),\n"
          << "  .BITSIZE_din(BITSIZE_data)) fifo_write(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .full_n(" << arg_name << out_suffix << "_ack),\n"
          << "  .write(1'b1),\n"
          << "  .din(" << arg_name << out_suffix << "));\n";
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::IP_COMPONENT,
                                            container_to_string(ip_components, ","));
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);
}
