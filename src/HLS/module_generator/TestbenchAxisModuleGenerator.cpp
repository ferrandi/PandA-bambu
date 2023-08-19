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
 * @file TestbenchAxisModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchAxisModuleGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

TestbenchAxisModuleGenerator::TestbenchAxisModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchAxisModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
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

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_axis_") - 1U, std::string::npos);

   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetMangledFunctionName();
   const auto top_fnode = HLSMgr->get_tree_manager()->CGetTreeReindex(function_id);
   const auto return_type = tree_helper::GetFunctionReturnType(top_fnode);
   THROW_ASSERT(HLSMgr->design_attributes.count(top_fname) && HLSMgr->design_attributes.at(top_fname).count(arg_name),
                "Parameter " + arg_name + " not found in function " + top_fname);
   const auto DesignAttributes = HLSMgr->design_attributes.at(top_fname).at(arg_name);
   const auto if_dir = port_o::to_port_direction(DesignAttributes.at(attr_interface_dir));
   const auto if_ndir = if_dir == port_o::IN ? port_o::OUT : port_o::IN;
   const auto if_alignment = DesignAttributes.find(attr_interface_alignment);
   const auto port_prefix = (if_dir == port_o::IN ? "s_axis_" : "m_axis_") + arg_name;
   std::string np_library = mod_cir->get_id() + " index";
   const auto add_port_parametric = [&](const std::string& suffix, port_o::port_direction dir, unsigned port_size) {
      const auto port_name = port_prefix + suffix;
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
   };
   add_port_parametric("_TDATA", if_ndir, 1U);
   add_port_parametric("_TVALID", if_ndir, 0U);
   add_port_parametric("_TREADY", if_dir, 0U);
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);

   out << R"(
function automatic integer log2;
  input integer value;
  `ifdef _SIM_HAVE_CLOG2
    log2 = $clog2(value);
  `else
    automatic integer temp_value = value-1;
    for (log2=0; temp_value > 0; log2=log2+1)
      temp_value = temp_value >> 1;
  `endif
endfunction

parameter BITSIZE_data=BITSIZE_)"
       << port_prefix << "_TDATA,\n";
   if(if_alignment == DesignAttributes.end())
   {
      out << "ALIGNMENT=log2(BITSIZE_data) > 3 ? (1<<(log2(BITSIZE_data)-3)) : 1;\n";
   }
   else
   {
      out << "ALIGNMENT=" << if_alignment->second << ";\n";
   }
   out << R"(
arg_utils a_utils();
mem_utils #(BITSIZE_data) m_utils();
)";

   if(if_dir == port_o::IN)
   {
      out << R"(
ptr_t addr, addr_next, addr_last, addr_last_next;
reg [BITSIZE_data-1:0] val, val_next;

initial
begin
  val = 0;
  val_next = 0;
  addr = 0;
  addr_next = 0;
  addr_last = 0;
  addr_last_next = 0;
end

always @(posedge clock)
begin
  if(setup_port)
  begin
    automatic ptr_t addr_val = a_utils.getptrarg(index);
    addr <= addr_val;
    addr_last <= addr_val + a_utils.getptrargsize(index)"
          << (return_type ? "-1" : "") << R"();
    val <= m_utils.read(addr_val);
  end
  else
  begin
    val <= val_next;
    addr <= addr_next;
    addr_last <= addr_last_next;
    if ()" << port_prefix
          << R"(_TREADY == 1'b1)
    begin
      automatic ptr_t val_next_addr = addr_next + ALIGNMENT;
      addr <= val_next_addr;
      if(val_next_addr < addr_last_next)
      begin
        val <= m_utils.read(val_next_addr);
      end
      if(addr_next >= addr_last_next)
      begin
        $display("Too many read requests for parameter )"
          << port_prefix << R"(");
        $finish;
      end
    end
  end
end

always @(*) 
begin
  addr_next = addr;
  addr_last_next = addr_last;
  val_next = val;
end
)";
      out << "assign " << port_prefix << "_TDATA = val;\n"
          << "assign " << port_prefix << "_TVALID = addr < addr_last;";
   }
   else if(if_dir == port_o::OUT)
   {
      out << R"(
ptr_t addr, addr_next, addr_last, addr_last_next;
reg enable, enable_next;

initial
begin
  addr = 0;
  addr_next = 0;
  addr_last = 0;
  addr_last_next = 0;
  enable = 0;
  enable_next = 0;
end

always @(posedge clock)
begin
  if(setup_port)
  begin
    automatic ptr_t addr_val = a_utils.getptrarg(index);
    addr <= addr_val;
    addr_last <= addr_val + a_utils.getptrargsize(index)"
          << (return_type ? "-1" : "") << R"();
    enable <= 1'b1;
  end
  else
  begin
    addr <= addr_next;
    addr_last <= addr_last_next;
    enable <= enable_next;
    if(enable == 1'b1 && )"
          << port_prefix << R"(_TVALID == 1'b1)
    begin
      if(addr_next >= addr_last_next)
      begin
        $display("Too many write requests for parameter )"
          << port_prefix << R"(");
        $finish;
      end
      addr <= addr_next + ALIGNMENT;
    end
  end
end

always @(*) 
begin
  addr_next = addr;
  addr_last_next = addr_last;
  enable_next = enable && !done_port;
end
always @(negedge clock)
begin
  if (enable == 1'b1 && )"
          << port_prefix << R"(_TVALID == 1'b1)
  begin
    if(addr < addr_last)
    begin
      m_utils.write(BITSIZE_data, )"
          << port_prefix << R"(_TDATA, addr);
    end
  end
end
)";
      out << "assign " << port_prefix << "_TREADY = addr < addr_last;";
   }
   else
   {
      THROW_UNREACHABLE("Unknown AXIS interface port direction: " + port_o::GetString(if_dir));
   }
}
