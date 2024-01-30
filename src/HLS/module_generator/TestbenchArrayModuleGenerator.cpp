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
 * @file TestbenchArrayModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchArrayModuleGenerator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"
#include "utility.hpp"

TestbenchArrayModuleGenerator::TestbenchArrayModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchArrayModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
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

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_array_") - 1U, std::string::npos);

   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(arg_name);
   const auto if_dir = port_o::to_port_direction(iface_attrs.at(FunctionArchitecture::iface_direction));
   const auto n_channels = HLSMgr->get_parameter()->getOption<unsigned int>(OPT_channels_number);

   std::string np_library = mod_cir->get_id() + " index WRITE_DELAY READ_DELAY";
   const auto add_port_parametric = [&](unsigned idx, const std::string& name, port_o::port_direction dir,
                                        unsigned port_size) {
      const auto port_name = arg_name + "_" + name + STR(idx);
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
      return port_name;
   };
   std::vector<std::string> ce, we, address, d, q;
   for(unsigned i = 0; i < n_channels; ++i)
   {
      address.push_back(add_port_parametric(i, "address", port_o::IN, 1U));
      ce.push_back(add_port_parametric(i, "ce", port_o::IN, 0U));
      if(if_dir != port_o::IN)
      {
         we.push_back(add_port_parametric(i, "we", port_o::IN, 0U));
         d.push_back(add_port_parametric(i, "d", port_o::IN, 1U));
      }
      else
      {
         we.push_back("1'b0");
         d.push_back("{BITSIZE_dq{1'b0}}");
      }
      if(if_dir != port_o::OUT)
      {
         q.push_back(add_port_parametric(i, "q", port_o::OUT, 1U));
      }
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);

   out << "localparam CHANNELS_NUMBER=" << n_channels << ",\n"
       << "  BITSIZE_address=BITSIZE_" << arg_name << "_address0,\n"
       << "  BITSIZE_dq=BITSIZE_" << arg_name << (if_dir == port_o::IN ? "_q0" : "_d0") << ";\n\n";

   out << "TestbenchArrayImpl #(.index(index),\n"
       << "  .WRITE_DELAY(WRITE_DELAY),\n"
       << "  .READ_DELAY(READ_DELAY),\n"
       << "  .PORTSIZE_address(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_address(BITSIZE_address),\n"
       << "  .PORTSIZE_ce(CHANNELS_NUMBER),\n"
       << "  .PORTSIZE_we(CHANNELS_NUMBER),\n"
       << "  .PORTSIZE_d(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_d(BITSIZE_dq),\n"
       << "  .PORTSIZE_q(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_q(BITSIZE_dq)) array_impl(.clock(clock),\n"
       << "  .setup_port(setup_port),\n"
       << "  .ce({" << container_to_string(ce, ",") << "}),\n"
       << "  .we({" << container_to_string(we, ",") << "}),\n"
       << "  .address({" << container_to_string(address, ",") << "}),\n"
       << "  .d({" << container_to_string(d, ",") << "})";
   if(q.size())
   {
      out << ",\n  .q({" << container_to_string(q, ",") << "})";
   }
   out << ");\n";
}
