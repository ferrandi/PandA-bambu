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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file TestbenchDUTHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the DUT wrapper used in generated testbenches.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "TestbenchDUTHDLGenerator.hpp"

#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "VHDL_writer.hpp"
#include "behavioral_helper.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "verilog_writer.hpp"

#include <set>
#include <string>
#include <vector>

TestbenchDUTHDLGenerator::TestbenchDUTHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchDUTHDLGenerator::InternalExec(std::ostream& out, structural_objectRef dut_cir, unsigned int top_id,
                                            gc_vertex_descriptor /* op_v */, const HDLWriter_Language language,
                                            const std::vector<HDLGenerator::parameter>& /* _p */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_in */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_out */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   const auto top = HLSMgr->get_HLS(top_id)->top;
   const auto top_cir = top->get_circ();
   const auto top_mod = GetPointer<module_o>(top_cir);
   THROW_ASSERT(top_mod, "");
   const auto parameters = HLSMgr->get_parameter();
   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto memory_mapped_top = parameters->getOption<bool>(OPT_memory_mapped_top);

   std::string signals, modules;
   std::set<std::string> internal_ports;

   if(memory_mapped_top)
   {
      if(interface_type == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
      {
         structural_manager::add_port(DONE_PORT_NAME, port_o::port_direction::OUT, dut_cir,
                                      structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
         signals += "wire irq;\n";
         modules += "assign done_port = irq;\n\n";
         internal_ports.insert("irq");
      }
      else if(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION)
      {
      }
      else
      {
         THROW_ERROR("Interface type not supported for memory mapped top simulation.");
      }

      // Dummy start port
      structural_manager::add_port(START_PORT_NAME, port_o::port_direction::IN, dut_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
   }

   const auto mod_id = HDL_manager::convert_to_identifier(top_mod->get_id());
   std::string dut_body = mod_id + " top(";
   const auto port_count = top_mod->get_num_ports();
   for(auto i = 0U; i < port_count; ++i)
   {
      const auto top_port = GetPointerS<port_o>(top_mod->get_positional_port(i));
      const auto port_id = top_port->get_id();
      if(!internal_ports.count(port_id))
      {
         const auto port_bitsize = GET_TYPE_SIZE(top_port);
         const auto port_size = [&]() {
            if(top_port->get_id() == CLOCK_PORT_NAME || top_port->get_id() == RESET_PORT_NAME ||
               top_port->get_id() == START_PORT_NAME || top_port->get_id() == DONE_PORT_NAME)
            {
               return 0ULL;
            }
            return top_port->get_kind() == port_vector_o_K ? (port_bitsize * top_port->get_ports_size()) : port_bitsize;
         }();
         structural_manager::add_port(port_id, top_port->get_port_direction(), dut_cir,
                                      structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      }
      auto escaped_port_id = HDL_manager::convert_to_identifier(port_id);
      dut_body += "\n  ." + escaped_port_id + "(" + escaped_port_id + "),";
   }
   dut_body.pop_back();
   dut_body += ");\n";

   out << signals << "\n" << modules << "\n" << dut_body;
}
