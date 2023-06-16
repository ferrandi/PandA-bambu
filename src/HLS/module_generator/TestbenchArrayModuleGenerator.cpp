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
   THROW_ASSERT(HLSMgr->design_attributes.count(top_fname) && HLSMgr->design_attributes.at(top_fname).count(arg_name),
                "");
   const auto DesignAttributes = HLSMgr->design_attributes.at(top_fname).at(arg_name);
   const auto if_dir = port_o::to_port_direction(DesignAttributes.at(attr_interface_dir));
   const auto if_alignment = DesignAttributes.find(attr_interface_alignment);
   const auto n_channels = HLSMgr->get_parameter()->getOption<unsigned int>(OPT_channels_number);

   std::string np_library = mod_cir->get_id() + " index WRITE_DELAY READ_DELAY";
   const auto add_port_parametric = [&](unsigned idx, const std::string& name, port_o::port_direction dir,
                                        unsigned port_size) {
      const auto port_name = arg_name + "_" + name + STR(idx);
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
   };
   for(unsigned i = 0; i < n_channels; ++i)
   {
      add_port_parametric(i, "address", port_o::IN, 1U);
      add_port_parametric(i, "ce", port_o::IN, 0U);
      if(if_dir != port_o::IN)
      {
         add_port_parametric(i, "we", port_o::IN, 0U);
         add_port_parametric(i, "d", port_o::IN, 1U);
      }
      if(if_dir != port_o::OUT)
      {
         add_port_parametric(i, "q", port_o::OUT, 1U);
      }
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);

   out << "parameter CHANNELS_NUMBER=" << n_channels << ",\n"
       << "  BITSIZE_address=BITSIZE_" << arg_name << "_address0,\n"
       << "  BITSIZE_dq=BITSIZE_" << arg_name << (if_dir == port_o::IN ? "_q0" : "_d0") << ",\n"
       << "  BITSIZE_ce=1,\n"
       << "  BITSIZE_we=1;\n";
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

parameter )";

   if(if_alignment == DesignAttributes.end())
   {
      out << "ALIGNMENT=log2(BITSIZE_data) > 3 ? (1<<(log2(BITSIZE_data)-3)) : 1";
   }
   else
   {
      out << "ALIGNMENT=" << if_alignment->second;
   }
   out << R"(,
  MAX_DELAY= WRITE_DELAY > READ_DELAY ? WRITE_DELAY : READ_DELAY,
  BITSIZE_item=BITSIZE_dq+BITSIZE_address+BITSIZE_ce+BITSIZE_we,
  BITSIZE_chunk=BITSIZE_item*CHANNELS_NUMBER,
  BITSIZE_ce_offset=0,
  BITSIZE_we_offset=BITSIZE_ce_offset+BITSIZE_ce,
  BITSIZE_address_offset=BITSIZE_we_offset+BITSIZE_we,
  BITSIZE_data_offset=BITSIZE_address_offset+BITSIZE_address;
genvar i;

mem_utils #(BITSIZE_dq) m_utils();
arg_utils a_utils();

ptr_t base_addr, base_addr_next;
reg [BITSIZE_chunk*MAX_DELAY-1:0] queue, queue_next;
reg [CHANNELS_NUMBER*BITSIZE_dq-1:0] q, q_next;

)";

   for(unsigned i = 0; i < n_channels; ++i)
   {
      out << "assign queue[BITSIZE_item*" << (i + 1) << "-1:BITSIZE_item*" << i << "] = {";
      if(if_dir != port_o::IN)
      {
         out << arg_name << "_d" << i << ", ";
      }
      else
      {
         out << "{BITSIZE_dq{1'b0}}, ";
      }
      out << arg_name << "_address" << i << ", ";
      if(if_dir != port_o::IN)
      {
         out << arg_name << "_we" << i << ", ";
      }
      else
      {
         out << "{BITSIZE_we{1'b0}}, ";
      }
      out << arg_name << "_ce" << i << "};\n";
      if(if_dir != port_o::OUT)
      {
         out << "assign " << arg_name << "_q" << i << " = q[BITSIZE_dq*" << (i + 1) << "-1:BITSIZE_dq*" << i << "];\n";
      }
   }

   out << R"(
always @(posedge clock)
begin
  if(1RESET_VALUE)
  begin
    base_addr <= a_utils.getptrarg(index);
    queue[BITSIZE_chunk*MAX_DELAY-1:BITSIZE_chunk] <= 0;
  end
  else
  begin
    base_addr <= base_addr_next;
    queue[BITSIZE_chunk*MAX_DELAY-1:BITSIZE_chunk] <= queue_next[BITSIZE_chunk*MAX_DELAY-1:BITSIZE_chunk];
  end
end

always @(*)
begin
  base_addr_next = base_addr;
  queue_next[BITSIZE_chunk-1:0] = queue[BITSIZE_chunk-1:0];
end

generate
  if(MAX_DELAY > 1)
  begin
    always @(*)
    begin
      queue_next[BITSIZE_chunk*MAX_DELAY-1:BITSIZE_chunk] = queue[BITSIZE_chunk*(MAX_DELAY-1)-1:0];
    end
  end
endgenerate
)";

   if(if_dir != port_o::IN)
   {
      out << R"(
generate
  for(i = 0; i < CHANNELS_NUMBER; i = i + 1)
  begin : write_port
    if(WRITE_DELAY > 1)
    begin
      always @(negedge clock)
      begin
        if(!(1RESET_VALUE))
        begin
        if(queue_next[BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_ce_offset+BITSIZE_ce-1:
                      BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_ce_offset] == 1'b1
          && queue_next[BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_we_offset+BITSIZE_we-1:
                        BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_we_offset] == 1'b1)
        begin
          automatic ptr_t address = base_addr_next +
            queue_next[BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_address_offset+BITSIZE_address-1:
                      BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_address_offset] * ALIGNMENT;
          automatic reg [BITSIZE_dq-1:0] data =
            queue_next[BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_data_offset+BITSIZE_dq-1:
                      BITSIZE_chunk*(WRITE_DELAY-1)+BITSIZE_item*i+BITSIZE_data_offset];
          m_utils.write(BITSIZE_dq, data, address);
        end
        end
      end
    end
    else
    begin
      always @(negedge clock)
      begin
        if(queue[BITSIZE_item*i+BITSIZE_ce_offset+BITSIZE_ce-1:BITSIZE_item*i+BITSIZE_ce_offset] == 1'b1
          && queue[BITSIZE_item*i+BITSIZE_we_offset+BITSIZE_we-1:BITSIZE_item*i+BITSIZE_we_offset] == 1'b1)
        begin
          automatic ptr_t address = base_addr +
            queue[BITSIZE_item*i+BITSIZE_address_offset+BITSIZE_address-1:
                  BITSIZE_item*i+BITSIZE_address_offset] * ALIGNMENT;
          automatic reg [BITSIZE_dq-1:0] data =
            queue[BITSIZE_item*i+BITSIZE_data_offset+BITSIZE_dq-1:BITSIZE_item*i+BITSIZE_data_offset];
          m_utils.write(BITSIZE_dq, data, address);
        end
      end
    end
  end
endgenerate
)";
   }
   if(if_dir != port_o::OUT)
   {
      out << R"(
generate
  for(i = 0; i < CHANNELS_NUMBER; i = i + 1)
  begin : read_port
    if(READ_DELAY > 1)
    begin
      always @(posedge clock)
      begin
        if(!(1RESET_VALUE))
        begin
          if(queue_next[BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_ce_offset+BITSIZE_ce-1:
                        BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_ce_offset] == 1'b1
            && queue_next[BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_we_offset+BITSIZE_we-1:
                          BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_we_offset] == 1'b0)
          begin
            automatic ptr_t address = base_addr_next +
              queue_next[BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_address_offset+BITSIZE_address-1:
                         BITSIZE_chunk*(READ_DELAY-1)+BITSIZE_item*i+BITSIZE_address_offset] * ALIGNMENT;
            q[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] <= m_utils.read(address);
          end
          else
          begin
            q[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] <= q_next[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i];
          end
        end
      end
      always @(*)
      begin
        q_next[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] = q[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i];
      end
    end
    else
    begin
      always @(posedge clock)
      begin
        q[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] <= q_next[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i];
      end
      always @(*)
      begin
        if(queue[BITSIZE_item*i+BITSIZE_ce_offset+BITSIZE_ce-1:BITSIZE_item*i+BITSIZE_ce_offset] == 1'b1
            && queue[BITSIZE_item*i+BITSIZE_we_offset+BITSIZE_we-1:BITSIZE_item*i+BITSIZE_we_offset] == 1'b0)
        begin
          automatic ptr_t address = base_addr +
            queue[BITSIZE_item*i+BITSIZE_address_offset+BITSIZE_address-1:
                  BITSIZE_item*i+BITSIZE_address_offset] * ALIGNMENT;
          q_next[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] = m_utils.read(address);
        end
        else
        begin
          q_next[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i] = q[BITSIZE_dq*(i+1)-1:BITSIZE_dq*i];
        end
      end
    end
  end
endgenerate)";
   }
}
