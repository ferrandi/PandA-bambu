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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file TestbenchFifoModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchFifoModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

TestbenchFifoModuleGenerator::TestbenchFifoModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchFifoModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
                                                unsigned int function_id, vertex /* op_v */,
                                                const HDLWriter_Language language,
                                                const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_fifo_") - 1U, std::string::npos);

   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(arg_name);
   const auto if_dir = port_o::to_port_direction(iface_attrs.at(FunctionArchitecture::iface_direction));
   const auto if_alignment = iface_attrs.at(FunctionArchitecture::iface_alignment);
   const auto if_ndir = if_dir == port_o::IN ? port_o::OUT : port_o::IN;
   std::string np_library = mod_cir->get_id() + " index";
   std::vector<std::string> ip_components;
   const auto addPort = [&](const std::string& port_name, port_o::port_direction dir, unsigned port_size,
                            bool isParametric = false) {
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      if(isParametric)
      {
         np_library += " " + port_name;
      }
   };
   out << "localparam BITSIZE_data=BITSIZE_" << arg_name << ((if_dir == port_o::IN) ? "_dout" : "_din") << ";\n";

   if(if_dir == port_o::IN)
   {
      addPort(arg_name + "_dout", if_ndir, 1U, true);
      addPort(arg_name + "_empty_n", if_ndir, 0U);
      addPort(arg_name + "_read", if_dir, 0U);
      ip_components.push_back("TestbenchFifoRead");
      out << "assign tb_done_port = 1'b1;\n\n"
          << "TestbenchFifoRead #(.index(index),\n"
          << "  .CHECK_ACK(1),\n"
          << "  .BITSIZE_dout(BITSIZE_data)) fifo_read(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .empty_n(" << arg_name << "_empty_n),\n"
          << "  .read(" << arg_name << "_read),\n"
          << "  .dout(" << arg_name << "_dout));\n";
   }
   else if(if_dir == port_o::OUT)
   {
      addPort(arg_name + "_din", if_ndir, 1U, true);
      addPort(arg_name + "_full_n", if_dir, 0U);
      addPort(arg_name + "_write", if_ndir, 0U);
      ip_components.push_back("TestbenchFifoWrite");
      out << "wire _full_n;\n\n"
          << "assign " << arg_name << "_full_n = _full_n;\n"
          << "assign tb_done_port = ~_full_n;\n\n"
          << "TestbenchFifoWrite #(.index(index),\n"
          << "  .BITSIZE_din(BITSIZE_data)) fifo_write(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .full_n(_full_n),\n"
          << "  .write(" << arg_name << "_write),\n"
          << "  .din(" << arg_name << "_din));\n";
   }
   else
   {
      THROW_UNREACHABLE("Unknown FIFO interface port direction: " + port_o::GetString(if_dir));
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::IP_COMPONENT,
                                            container_to_string(ip_components, ","));
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);
}
