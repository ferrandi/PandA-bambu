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
   THROW_ASSERT(HLSMgr->design_attributes.count(top_fname) && HLSMgr->design_attributes.at(top_fname).count(arg_name),
                "Parameter " + arg_name + " not found in function " + top_fname);
   const auto DesignAttributes = HLSMgr->design_attributes.at(top_fname).at(arg_name);
   const auto if_dir = port_o::to_port_direction(DesignAttributes.at(attr_interface_dir));
   const std::string in_suffix = if_dir == port_o::IO ? "_i" : "";
   const std::string out_suffix = if_dir == port_o::IO ? "_o" : "";
   std::string np_library = mod_cir->get_id() + " index";
   const auto add_port_parametric = [&](const std::string& suffix, port_o::port_direction dir, unsigned port_size) {
      const auto port_name = arg_name + suffix;
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
   };
   out << "localparam BITSIZE_data=BITSIZE_" << arg_name << (in_suffix.size() ? in_suffix : out_suffix) << ";\n"
       << R"(
arg_utils a_utils();
mem_utils #(BITSIZE_data) m_utils();
)";
   if(if_dir == port_o::IN || if_dir == port_o::IO)
   {
      add_port_parametric(in_suffix, port_o::OUT, 1U);
      add_port_parametric(in_suffix + "_ack", port_o::IN, 0U);
      out << R"(
reg ack_trigger; 
wire ack_trigger_next;
reg [BITSIZE_data-1:0] val;
wire [BITSIZE_data-1:0] val_next;

initial
begin
  val = 0;
  ack_trigger = 0;
end

always @(posedge clock)
begin
  if(setup_port)
  begin
    automatic ptr_t addr = a_utils.getptrarg(index);
    val <= m_utils.read(addr);
    ack_trigger <= 0;
  end
  else
  begin
    val <= val_next;
    ack_trigger <= ack_trigger_next;
    if(ack_trigger_next)
    begin
      val <= 0;
    end
    if(done_port && !ack_trigger_next)
    begin
      $display("Acknowledge signal never asserted for parameter )"
          << arg_name << in_suffix << R"(");
      $finish;
    end
  end
end

assign val_next = val;
assign ack_trigger_next = )"
          << arg_name << in_suffix << R"(_ack | ack_trigger;
)";
      out << "assign " << arg_name << in_suffix << " = val;\n";
   }
   if(if_dir == port_o::OUT || if_dir == port_o::IO)
   {
      add_port_parametric(out_suffix, port_o::IN, 1U);
      add_port_parametric(out_suffix + "_ack", port_o::OUT, 0U);
      out << R"(
ptr_t addr, addr_next;
reg enable, enable_next;

initial
begin
  enable = 0;
  $display("BEAWARE: Output acknowledge interface will read at each clock cycle");
end

always @(posedge clock)
begin
  if(setup_port)
  begin
    addr <= a_utils.getptrarg(index);
    enable <= 1'b1;
  end
  else
  begin
    addr <= addr_next;
    enable <= enable_next;
    if(enable_next)
    begin
      m_utils.write(BITSIZE_data, )"
          << arg_name << out_suffix << R"(, addr_next);
    end
  end
end

always @(*)
begin
  addr_next = addr;
  enable_next = enable && !done_port;
end
)";
      out << "assign " << arg_name << out_suffix << "_ack = 1'b1;\n";
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);
}
