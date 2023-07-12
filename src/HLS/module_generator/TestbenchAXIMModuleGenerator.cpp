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
 * @file TestbenchAXIMModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchAXIMModuleGenerator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"

TestbenchAXIMModuleGenerator::TestbenchAXIMModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchAXIMModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
                                                unsigned int /* function_id */, vertex /* op_v */,
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

   const auto port_prefix = mod_cir->get_id().substr(sizeof("if_") - 1U, std::string::npos);
   std::string np_library = mod_cir->get_id() + " WRITE_DELAY READ_DELAY QUEUE_SIZE";
   std::string internal_port_assign;
   const auto add_port_parametric = [&](const std::string& name, port_o::port_direction dir, unsigned port_size)
   {
      const auto port_name = port_prefix + "_" + name;
      const auto port_obj = structural_manager::add_port(
          port_name, dir, mod_cir, structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      GetPointerS<port_o>(port_obj)->set_is_memory(true);
      if(port_size)
      {
         np_library += " " + port_name;
      }
      out << "reg ";
      if(port_size)
      {
         out << "[BITSIZE_" << port_name << "-1:0] ";
      }
      out << name << ";\n";
      if(dir == port_o::IN)
      {
         internal_port_assign += "assign " + name + "=" + port_name + ";\n";
      }
      else
      {
         internal_port_assign += "assign " + port_name + "=" + name + ";\n";
      }
   };

   add_port_parametric("awready", port_o::OUT, 0U);
   add_port_parametric("wready", port_o::OUT, 0U);
   add_port_parametric("bid", port_o::OUT, 1U);
   add_port_parametric("bresp", port_o::OUT, 2U);
   add_port_parametric("buser", port_o::OUT, 1U);
   add_port_parametric("bvalid", port_o::OUT, 0U);
   add_port_parametric("arready", port_o::OUT, 0U);
   add_port_parametric("rid", port_o::OUT, 1U);
   add_port_parametric("rdata", port_o::OUT, 1U);
   add_port_parametric("rresp", port_o::OUT, 2U);
   add_port_parametric("rlast", port_o::OUT, 0U);
   add_port_parametric("ruser", port_o::OUT, 1U);
   add_port_parametric("rvalid", port_o::OUT, 0U);

   add_port_parametric("awid", port_o::IN, 1U);
   add_port_parametric("awaddr", port_o::IN, 1U);
   add_port_parametric("awlen", port_o::IN, 1U);
   add_port_parametric("awsize", port_o::IN, 1U);
   add_port_parametric("awburst", port_o::IN, 2U);
   add_port_parametric("awlock", port_o::IN, 1U);
   add_port_parametric("awcache", port_o::IN, 1U);
   add_port_parametric("awprot", port_o::IN, 1U);
   add_port_parametric("awqos", port_o::IN, 1U);
   add_port_parametric("awregion", port_o::IN, 1U);
   add_port_parametric("awuser", port_o::IN, 1U);
   add_port_parametric("awvalid", port_o::IN, 0U);

   add_port_parametric("wid", port_o::IN, 1U); // AXI3 spec only
   add_port_parametric("wdata", port_o::IN, 1U);
   add_port_parametric("wstrb", port_o::IN, 1U);
   add_port_parametric("wlast", port_o::IN, 0U);
   add_port_parametric("wuser", port_o::IN, 1U);
   add_port_parametric("wvalid", port_o::IN, 0U);

   add_port_parametric("bready", port_o::IN, 0U);

   add_port_parametric("arid", port_o::IN, 1U);
   add_port_parametric("araddr", port_o::IN, 1U);
   add_port_parametric("arlen", port_o::IN, 1U);
   add_port_parametric("arsize", port_o::IN, 1U);
   add_port_parametric("arburst", port_o::IN, 2U);
   add_port_parametric("arlock", port_o::IN, 1U);
   add_port_parametric("arcache", port_o::IN, 1U);
   add_port_parametric("arprot", port_o::IN, 1U);
   add_port_parametric("arqos", port_o::IN, 1U);
   add_port_parametric("arregion", port_o::IN, 1U);
   add_port_parametric("aruser", port_o::IN, 1U);
   add_port_parametric("arvalid", port_o::IN, 0U);

   add_port_parametric("rready", port_o::IN, 0U);
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);

   out << internal_port_assign << "\n"
       << "parameter BITSIZE_data=BITSIZE_" << port_prefix << "_rdata,\n"
       << "  BITSIZE_counter=32,\n"
       << "  BITSIZE_burst=BITSIZE_" << port_prefix << "_arburst,\n"
       << "  BITSIZE_len=BITSIZE_" << port_prefix << "_arlen,\n"
       << "  BITSIZE_size=BITSIZE_" << port_prefix << "_arsize,\n"
       << "  BITSIZE_addr=BITSIZE_" << port_prefix << "_araddr,\n"
       << "  OFFSET_counter=0,\n"
       << "  OFFSET_burst=OFFSET_counter+BITSIZE_counter,\n"
       << "  OFFSET_len=OFFSET_burst+BITSIZE_burst,\n"
       << "  OFFSET_size=OFFSET_len+BITSIZE_len,\n"
       << "  OFFSET_addr=OFFSET_size+BITSIZE_size,\n"
       << "  BITSIZE_aritem=BITSIZE_addr+BITSIZE_size+BITSIZE_len+BITSIZE_burst+BITSIZE_counter,\n"
       << "  BITSIZE_awitem=BITSIZE_addr+BITSIZE_size+BITSIZE_len+BITSIZE_burst+BITSIZE_counter;\n"
       << R"(
reg [QUEUE_SIZE*BITSIZE_aritem-1:0] arqueue,next_arqueue;
reg [QUEUE_SIZE*BITSIZE_awitem-1:0] awqueue,next_awqueue;
integer arqueue_size, next_arqueue_size;
integer awqueue_size, next_awqueue_size;

mem_utils #(BITSIZE_data) m_utils();

initial
begin
  arqueue = 0;
  next_arqueue = 0;
  arqueue_size = 0;
  next_arqueue_size = 0;
  awqueue = 0;
  next_awqueue = 0;
  awqueue_size = 0;
  next_awqueue_size = 0;
  awready = 0;
  wready = 0;
  bid = 0;
  bresp = 0;
  buser = 0;
  bvalid = 0;
  arready = 0;
  rid = 0;
  rdata = 0;
  rresp = 0;
  rlast = 0;
  ruser = 0;
  rvalid = 0;
end

// Combinatorial logic for read transactions
always@(*)
begin: read_comb
  automatic integer unsigned i;
  next_arqueue = arqueue;
  next_arqueue_size = arqueue_size;
  if(arvalid)
  begin
    if(arqueue_size < QUEUE_SIZE)
    begin
      next_arqueue[arqueue_size*BITSIZE_aritem +:BITSIZE_aritem] = {araddr, arsize, arlen, arburst, {BITSIZE_counter{1'b0}} -READ_DELAY};
      next_arqueue_size = arqueue_size + 1;
    end
  end
  if(arqueue_size > 0 && next_arqueue[OFFSET_counter+:BITSIZE_counter] == next_arqueue[OFFSET_len+:BITSIZE_len] && rready)
  begin
    for(i = 1; i < QUEUE_SIZE; i = i + 1)
    begin
      next_arqueue[(i-1)*BITSIZE_aritem+:BITSIZE_aritem] = next_arqueue[i*BITSIZE_aritem+:BITSIZE_aritem];
    end
    next_arqueue_size = next_arqueue_size - 1;
  end
  if(next_arqueue_size > 0 && next_arqueue[OFFSET_counter+BITSIZE_counter-1] == 1'b0 && rready)
  begin
    next_arqueue[OFFSET_counter+:BITSIZE_counter] = next_arqueue[OFFSET_counter+:BITSIZE_counter] + 1;
  end
  for(i = 0; i < QUEUE_SIZE; i = i + 1)
  begin
    if(arqueue[i*BITSIZE_aritem+OFFSET_counter+BITSIZE_counter-1] == 1'b1)
    begin
      next_arqueue[i*BITSIZE_aritem+OFFSET_counter+:BITSIZE_counter] = arqueue[i*BITSIZE_aritem+OFFSET_counter+:BITSIZE_counter] + 1;
    end
  end
end

// Combinatorial logic for write transactions
always@(*) 
begin: write_comb
  automatic integer i;
  automatic reg [BITSIZE_counter-1:0] counter;
  next_awqueue = awqueue;
  next_awqueue_size = awqueue_size;
  if(awvalid)
  begin
    if(awqueue_size < QUEUE_SIZE)
    begin
      next_awqueue[awqueue_size*BITSIZE_awitem+:BITSIZE_awitem] = {awaddr, awsize, awlen, awburst, 32'd1};
      next_awqueue_size = awqueue_size + 1;
    end
  end
  if(next_awqueue_size > 0 && next_awqueue[OFFSET_counter+:BITSIZE_counter] == 0 && bready)
  begin
    for(i = 1; i < QUEUE_SIZE; i = i + 1)
    begin
      next_awqueue[(i-1)*BITSIZE_awitem+:BITSIZE_awitem] = next_awqueue[i*BITSIZE_awitem+:BITSIZE_awitem];
    end
    next_awqueue_size = next_awqueue_size - 1;
  end
  if(wvalid)
  begin
    i = 0;
    counter = next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter];
    while((counter == 0 || counter[BITSIZE_counter-1] == 1) && i < next_awqueue_size)
    begin
      i = i + 1;
      counter = next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter];
    end
    if(wlast)
    begin
      next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] = {{(BITSIZE_counter-1){1'b0}}, 1'b1} - WRITE_DELAY;
    end
    else
    begin
      next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] = next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] + 1;
    end
  end
  for(i = 0; i < QUEUE_SIZE; i = i + 1)
  begin
    if(next_awqueue[i*BITSIZE_awitem+OFFSET_counter+BITSIZE_counter-1] == 1'b1)
    begin
      next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] = next_awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] + 1;
    end
  end
end

// Sequential logic for read transactions
always@(posedge clock)
begin : read_seq
  automatic ptr_t currAddr;
  automatic ptr_t endAddr;
  automatic integer i;
  arready <= (arqueue_size < QUEUE_SIZE) & ~(1RESET_VALUE);
  rvalid <= 0;
  rresp <= 0;
  rlast <= 0;
  if(arqueue_size > 0 && arqueue[OFFSET_counter+BITSIZE_counter-1] == 1'b0)
  begin
    rvalid <= 1;
    if(arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b00)
    begin
      currAddr = arqueue[OFFSET_addr+:BITSIZE_addr];
    end
    else if(arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b01)
    begin
      currAddr = arqueue[OFFSET_addr+:BITSIZE_addr] + arqueue[OFFSET_counter+:BITSIZE_counter] * (1 << arqueue[OFFSET_size+:BITSIZE_size]);
    end
    else if(arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b10)
    begin
      endAddr = arqueue[OFFSET_addr+:BITSIZE_addr] - (arqueue[OFFSET_addr+:BITSIZE_addr] % ((arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << arqueue[OFFSET_size+:BITSIZE_size]))) + ((arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << arqueue[OFFSET_size+:BITSIZE_size]));
      currAddr = arqueue[OFFSET_addr+:BITSIZE_addr] + arqueue[OFFSET_counter+:BITSIZE_counter] * (1 << arqueue[OFFSET_size+:BITSIZE_size]);
      if(currAddr > endAddr)
      begin
        currAddr = currAddr - ((arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << arqueue[OFFSET_size+:BITSIZE_size]));
      end
    end
    currAddr = currAddr - (currAddr % (1 << arqueue[OFFSET_size+:BITSIZE_size]));
    rdata <= m_utils.read(currAddr); // {_bambu_testbench_mem_[currAddr + 1 - base_addr], _bambu_testbench_mem_[currAddr + 0 - base_addr]};
    rresp <= 0;
    if(arqueue[OFFSET_counter+:BITSIZE_counter] == arqueue[OFFSET_len+:BITSIZE_len])
    begin
      rlast <= 1;
    end
  end
  arqueue <= next_arqueue;
  arqueue_size <= next_arqueue_size;
end

// Sequential logic for write transactions
always@(posedge clock)
begin: write_seq
  automatic ptr_t currAddr;
  automatic ptr_t endAddr;
  automatic integer unsigned i;
  awready <= (awqueue_size < QUEUE_SIZE) & ~(1RESET_VALUE);
  wready <= ~(1RESET_VALUE);
  bvalid <= 0;
  if(awqueue_size > 0 && awqueue[OFFSET_counter+:BITSIZE_counter] == 0)
  begin 
    bresp <= 0;
    bvalid <= 1;
  end
  if(wvalid)
  begin
    if(awqueue_size > 0 || awvalid == 1'b1); else $error("Received data on write channel, but no transaction in queue");
    if(awqueue_size > 0)
    begin
      i = 0;
      while((awqueue[i*BITSIZE_awitem+OFFSET_counter+BITSIZE_counter-1] == 1 || awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] == 0) && i < awqueue_size)
      begin
        i = i + 1;
      end
      if(i < awqueue_size || awvalid == 1'b1); else $error("Received write data, but all transactions in queue are in delay phase");
      if(i < awqueue_size)
      begin 
        if(awqueue[i*BITSIZE_awitem+OFFSET_burst+:BITSIZE_burst] == 2'b00)
        begin
          currAddr = awqueue[i*BITSIZE_awitem+OFFSET_addr+:BITSIZE_addr];
        end
        else if(awqueue[i*BITSIZE_awitem+OFFSET_burst+:BITSIZE_burst] == 2'b01)
        begin
          currAddr = awqueue[i*BITSIZE_awitem+OFFSET_addr+:BITSIZE_addr] + (awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] - 1) * (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size]);
        end
        else if(awqueue[i*BITSIZE_awitem+OFFSET_burst+:BITSIZE_burst] == 2'b10)
        begin
          endAddr = awqueue[i*BITSIZE_awitem+OFFSET_addr+:BITSIZE_addr] 
            - (awqueue[i*BITSIZE_awitem+OFFSET_addr+:BITSIZE_addr] % ((awqueue[i*BITSIZE_awitem+OFFSET_len+:BITSIZE_len] + 1) * (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size])))
            + ((awqueue[i*BITSIZE_awitem+OFFSET_len+:BITSIZE_len] + 1) * (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size]));
          currAddr = awqueue[i*BITSIZE_awitem+OFFSET_addr+:BITSIZE_addr] 
            + (awqueue[i*BITSIZE_awitem+OFFSET_counter+:BITSIZE_counter] - 1) * (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size]);
          if(currAddr > endAddr)
          begin
            currAddr = currAddr - ((awqueue[i*BITSIZE_awitem+OFFSET_len+:BITSIZE_len] + 1) * (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size]));
          end
        end
        currAddr = currAddr - (currAddr % (1 << awqueue[i*BITSIZE_awitem+OFFSET_size+:BITSIZE_size]));
      end
      else
      begin
        currAddr = awaddr - (awaddr % (1 << awsize));
      end
    end
    else
    begin
      currAddr = awaddr - (awaddr % (1 << awsize));
    end
    if(wstrb != 0)
    begin
      m_utils.write_strobe(wstrb, wdata, currAddr);
    end
  end
  awqueue <= next_awqueue;
  awqueue_size <= next_awqueue_size;
end
)";
}
