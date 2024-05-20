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
 * @file TestbenchDUTModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchDUTModuleGenerator.hpp"

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

TestbenchDUTModuleGenerator::TestbenchDUTModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchDUTModuleGenerator::InternalExec(std::ostream& out, structural_objectRef dut_cir, unsigned int top_id,
                                               vertex /* op_v */, const HDLWriter_Language language,
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

   const auto top = HLSMgr->get_HLS(top_id)->top;
   const auto top_cir = top->get_circ();
   const auto top_mod = GetPointer<module>(top_cir);
   THROW_ASSERT(top_mod, "");
   const auto parameters = HLSMgr->get_parameter();
   const auto top_language = parameters->getOption<HDLWriter_Language>(OPT_writer_language);
   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto memory_mapped_top = parameters->getOption<bool>(OPT_memory_mapped_top);

   std::string signals, modules;
   std::set<std::string> internal_ports;
   const auto escape_keyword = [&](const std::string& str,
                                   const HDLWriter_Language w = HDLWriter_Language::VERILOG) -> std::string {
      if(w == HDLWriter_Language::VERILOG && verilog_writer::check_keyword_verilog(str))
      {
         return "\\" + str + " ";
      }
      else if(w == HDLWriter_Language::VHDL && (str.find("__") != std::string::npos || str.front() == '_' ||
                                                str.back() == '_' || VHDL_writer::check_keyword_vhdl(str)))
      {
         return "\\" + str + "\\";
      }
      return str;
   };

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

   const auto mod_id = escape_keyword(top_mod->get_id(), top_language);
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
      dut_body += "\n  ." + escape_keyword(port_id) + "(" + escape_keyword(port_id) + "),";
   }
   dut_body.pop_back();
   dut_body += ");\n";

   out << signals << "\n" << modules << "\n" << dut_body;
}
