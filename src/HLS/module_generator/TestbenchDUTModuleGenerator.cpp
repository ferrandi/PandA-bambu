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

#include "hls.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "verilog_writer.hpp"

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
   const auto top_mod = GetPointer<module>(top->get_circ());
   THROW_ASSERT(top_mod, "");

   std::string signals;
   std::string modules;
   const auto escape_keyword = [&](const std::string& str) -> std::string {
      if(verilog_writer::check_keyword_verilog(str))
      {
         return "\\" + str + " ";
      }
      return str;
   };
   const auto mod_id = escape_keyword(top_mod->get_id());
   std::string dut_body = mod_id + " top(";
   const auto port_count = top_mod->get_num_ports();
   for(auto i = 0U; i < port_count; ++i)
   {
      const auto top_port = GetPointerS<port_o>(top_mod->get_positional_port(i));
      const auto port_bitsize = GET_TYPE_SIZE(top_port);
      const auto port_id = top_port->get_id();
      // if(top_port->get_kind() == port_vector_o_K)
      // {
      //    const auto sig_id = "sig_" + port_id;
      //    const auto port_portsize = top_port->get_ports_size();
      //    const auto port_size = port_portsize * port_bitsize;
      //    signals += "wire " + (port_bitsize > 1U ? "[" + STR(port_bitsize - 1U) + ":0] " : "") + sig_id + " [" +
      //               STR(port_portsize - 1U) + ":0];\n";
      //    if(top_port->get_port_direction() == port_o::port_direction::IN)
      //    {
      //       modules += "split_signal #(.BITSIZE_in1(" + STR(port_size) + "),\n  .BITSIZE_out1(" + STR(port_bitsize) +
      //                  "),\n  .PORTSIZE_out1(" + STR(port_portsize) + ")) split_" + port_id + " (.in1(" + port_id +
      //                  "),\n  .out1(" + sig_id + "));\n";
      //    }
      //    else
      //    {
      //       modules += "join_signal #(.BITSIZE_in1(" + STR(port_bitsize) + "),\n  .PORTSIZE_in1(" +
      //       STR(port_portsize) +
      //                  "),\n  .BITSIZE_out1(" + STR(port_size) + ")) join_" + port_id + " (.in1(" + sig_id +
      //                  "),\n  .out1(" + port_id + "));\n";
      //    }
      //    const auto wrapper_port = structural_manager::add_port(
      //        port_id, top_port->get_port_direction(), dut_cir,
      //        structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      //    dut_body += "\n  ." + port_id + "(" + sig_id + "),";
      // }
      // else
      // {
      const auto port_size =
          top_port->get_kind() == port_vector_o_K ? (port_bitsize * top_port->get_ports_size()) : port_bitsize;
      structural_manager::add_port(port_id, top_port->get_port_direction(), dut_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      dut_body += "\n  ." + escape_keyword(port_id) + "(" + escape_keyword(port_id) + "),";
      // }
   }
   dut_body.pop_back();
   dut_body += ");\n";
   structural_manager::add_NP_functionality(dut_cir, NP_functionality::IP_COMPONENT, "join_signal,split_signal");

   out << signals << "\n" << modules << "\n" << dut_body;
}
