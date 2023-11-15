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
   std::string np_library = mod_cir->get_id();
   std::string internal_port_assign;
   const auto add_port_parametric_wire = [&](const std::string& name, port_o::port_direction dir, unsigned port_size) {
      const auto port_name = port_prefix + "_" + name;
      const auto port_obj = structural_manager::add_port(
          port_name, dir, mod_cir, structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      GetPointerS<port_o>(port_obj)->set_is_memory(true);
      if(port_size)
      {
         np_library += " " + port_name;
      }
      out << "wire ";
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

   const auto add_port_parametric_reg = [&](const std::string& name, port_o::port_direction dir, unsigned port_size) {
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

   add_port_parametric_reg("awready", port_o::OUT, 0U);
   add_port_parametric_reg("wready", port_o::OUT, 0U);
   add_port_parametric_reg("bid", port_o::OUT, 1U);
   add_port_parametric_reg("bresp", port_o::OUT, 2U);
   add_port_parametric_reg("buser", port_o::OUT, 1U);
   add_port_parametric_reg("bvalid", port_o::OUT, 0U);
   add_port_parametric_reg("arready", port_o::OUT, 0U);
   add_port_parametric_reg("rid", port_o::OUT, 1U);
   add_port_parametric_reg("rdata", port_o::OUT, 1U);
   add_port_parametric_reg("rresp", port_o::OUT, 2U);
   add_port_parametric_reg("rlast", port_o::OUT, 0U);
   add_port_parametric_reg("ruser", port_o::OUT, 1U);
   add_port_parametric_reg("rvalid", port_o::OUT, 0U);

   add_port_parametric_reg("awid", port_o::IN, 1U);
   add_port_parametric_reg("awaddr", port_o::IN, 1U);
   add_port_parametric_reg("awlen", port_o::IN, 1U);
   add_port_parametric_reg("awsize", port_o::IN, 1U);
   add_port_parametric_reg("awburst", port_o::IN, 2U);
   add_port_parametric_reg("awlock", port_o::IN, 1U);
   add_port_parametric_reg("awcache", port_o::IN, 1U);
   add_port_parametric_reg("awprot", port_o::IN, 1U);
   add_port_parametric_reg("awqos", port_o::IN, 1U);
   add_port_parametric_reg("awregion", port_o::IN, 1U);
   add_port_parametric_reg("awuser", port_o::IN, 1U);
   add_port_parametric_reg("awvalid", port_o::IN, 0U);

   add_port_parametric_reg("wid", port_o::IN, 1U); // AXI3 spec only
   add_port_parametric_reg("wdata", port_o::IN, 1U);
   add_port_parametric_reg("wstrb", port_o::IN, 1U);
   add_port_parametric_reg("wlast", port_o::IN, 0U);
   add_port_parametric_reg("wuser", port_o::IN, 1U);
   add_port_parametric_reg("wvalid", port_o::IN, 0U);

   add_port_parametric_wire("bready", port_o::IN, 0U);

   add_port_parametric_reg("arid", port_o::IN, 1U);
   add_port_parametric_reg("araddr", port_o::IN, 1U);
   add_port_parametric_reg("arlen", port_o::IN, 1U);
   add_port_parametric_reg("arsize", port_o::IN, 1U);
   add_port_parametric_reg("arburst", port_o::IN, 2U);
   add_port_parametric_reg("arlock", port_o::IN, 1U);
   add_port_parametric_reg("arcache", port_o::IN, 1U);
   add_port_parametric_reg("arprot", port_o::IN, 1U);
   add_port_parametric_reg("arqos", port_o::IN, 1U);
   add_port_parametric_reg("arregion", port_o::IN, 1U);
   add_port_parametric_reg("aruser", port_o::IN, 1U);
   add_port_parametric_reg("arvalid", port_o::IN, 0U);
   add_port_parametric_wire("rready", port_o::IN, 0U);

   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);

   if(HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_write) == 1)
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 4,
                    "Warning: AXI does not support mem-delay-write==1, as it requires at least a cycle of latence -> "
                    "mem-delay-write changed to 2.\n");
   }
   out << internal_port_assign << "\n"
       << "localparam WRITE_DELAY="
       << std::max(1U, (HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_write) - 1)) << ",\n"
       << "  READ_DELAY=" << (HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_read) - 1) << ",\n"
       << "  QUEUE_SIZE=2,\n"
       << "  BITSIZE_data=BITSIZE_" << port_prefix << "_rdata,\n"
       << "  BITSIZE_counter=32,\n"
       << "  BITSIZE_burst=BITSIZE_" << port_prefix << "_arburst,\n"
       << "  BITSIZE_len=BITSIZE_" << port_prefix << "_arlen,\n"
       << "  BITSIZE_delay=32,\n"
       << "  BITSIZE_size=BITSIZE_" << port_prefix << "_arsize,\n"
       << "  BITSIZE_addr=BITSIZE_" << port_prefix << "_araddr,\n"
       << "  BITSIZE_wdata=BITSIZE_" << port_prefix << "_wdata,\n"
       << "  BITSIZE_wstrb=BITSIZE_" << port_prefix << "_wstrb,\n"
       << "  OFFSET_delay=0,\n"
       << "  OFFSET_counter=OFFSET_delay+BITSIZE_delay,\n"
       << "  OFFSET_burst=OFFSET_counter+BITSIZE_counter,\n"
       << "  OFFSET_len=OFFSET_burst+BITSIZE_burst,\n"
       << "  OFFSET_size=OFFSET_len+BITSIZE_len,\n"
       << "  OFFSET_addr=OFFSET_size+BITSIZE_size,\n"
       << "  OFFSET_wdata=OFFSET_addr+BITSIZE_addr,\n"
       << "  OFFSET_wstrb=OFFSET_wdata+BITSIZE_wdata,\n"
       << "  BITSIZE_aritem=BITSIZE_addr+BITSIZE_size+BITSIZE_len+BITSIZE_burst+BITSIZE_counter+BITSIZE_delay,\n"
       << "  "
          "BITSIZE_awitem=BITSIZE_wstrb+BITSIZE_wdata+BITSIZE_addr+BITSIZE_size+BITSIZE_len+BITSIZE_burst+BITSIZE_"
          "counter+BITSIZE_"
          "delay;\n"
       << R"(
reg [QUEUE_SIZE*BITSIZE_aritem-1:0] arqueue;
reg [QUEUE_SIZE*BITSIZE_aritem-1:0] next_arqueue;
reg [QUEUE_SIZE*BITSIZE_awitem-1:0] awqueue; 
reg [QUEUE_SIZE*BITSIZE_awitem-1:0] next_awqueue;
reg [31:0] test_addr;
reg [31:0] test_strb;
reg [31:0] test_data;
integer arqueue_size, next_arqueue_size;
integer awqueue_size, next_awqueue_size;

mem_utils #(BITSIZE_data) m_utils();

initial
begin
  arqueue = 0;
  arqueue_size = 0;
  next_arqueue = 0;
  next_arqueue_size = 0;
  awqueue = 0;
  awqueue_size = 0;
  next_awqueue = 0;
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
  if(arvalid && arready)  // Valid and ready -> accept new transaction
  begin
    next_arqueue[arqueue_size*BITSIZE_aritem +:BITSIZE_aritem] = {araddr, arsize, (arlen == 0 ? {{(BITSIZE_len - 1){1'b0}},{1'b1}} : arlen), arburst, {BITSIZE_counter{1'b0}}, ({BITSIZE_delay{1'b0}} + READ_DELAY)}; // size of parameter is implementation dependent
    next_arqueue_size = arqueue_size + 1;
  end
  if(next_arqueue_size > 0 && rvalid && rready)
  begin
    next_arqueue[OFFSET_counter+:BITSIZE_counter] = next_arqueue[OFFSET_counter+:BITSIZE_counter] + 1;
  end
  if(arqueue_size > 0 && next_arqueue[OFFSET_counter+:BITSIZE_counter] == next_arqueue[OFFSET_len+:BITSIZE_len] && rready && rvalid)
  begin
    for(i = 1; i < QUEUE_SIZE; i = i + 1)
    begin
      next_arqueue[(i-1)*BITSIZE_aritem+:BITSIZE_aritem] = next_arqueue[i*BITSIZE_aritem+:BITSIZE_aritem];
    end
    next_arqueue_size = next_arqueue_size - 1;
  end
  for(i = 0; i < QUEUE_SIZE; i = i + 1)
  begin
    if(arqueue[i*BITSIZE_aritem+OFFSET_delay+:BITSIZE_delay] > 1)
    begin
      next_arqueue[i*BITSIZE_aritem+OFFSET_delay+:BITSIZE_delay] = arqueue[i*BITSIZE_aritem+OFFSET_delay+:BITSIZE_delay] - 1;
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
  if(awvalid && awready && wvalid && wready) // Valid and ready -> accept new transaction
  begin
    next_awqueue[awqueue_size*BITSIZE_awitem+:BITSIZE_awitem] = {wstrb, wdata, awaddr, awsize, (awlen == 0 ? {{(BITSIZE_len - 1){1'b0}},{1'b1}} : awlen), awburst, {BITSIZE_counter{1'b0}}, ({BITSIZE_delay{1'b0}} + WRITE_DELAY)};
    next_awqueue_size = awqueue_size + 1;
  end
  if(next_awqueue_size > 0 && next_awqueue[OFFSET_counter+:BITSIZE_counter] == next_awqueue[OFFSET_len+:BITSIZE_len] && bready && bvalid)
  begin
    for(i = 1; i < QUEUE_SIZE; i = i + 1)
    begin
      next_awqueue[(i-1)*BITSIZE_awitem+:BITSIZE_awitem] = next_awqueue[i*BITSIZE_awitem+:BITSIZE_awitem];
    end
    next_awqueue_size = next_awqueue_size - 1;
  end
  if(next_awqueue_size > 0)
  begin
    next_awqueue[OFFSET_counter+:BITSIZE_counter] = next_awqueue[OFFSET_counter+:BITSIZE_counter] + 1;
  end
  for(i = 0; i < QUEUE_SIZE; i = i + 1)
  begin
    if(next_awqueue[i*BITSIZE_awitem+OFFSET_delay+:BITSIZE_delay] > 1)
    begin
      next_awqueue[i*BITSIZE_awitem+OFFSET_delay+:BITSIZE_delay] = next_awqueue[i*BITSIZE_awitem+OFFSET_delay+:BITSIZE_delay] - 1;
    end
  end
end

always@(posedge clock)
begin
   if(1RESET_VALUE)
   begin
    arready <= 0;
    awready <= 0;
  ``wready <= 0;
   end
   else
   begin
    arready <= (next_arqueue_size - (next_arqueue[OFFSET_counter+:BITSIZE_counter] == (next_arqueue[OFFSET_len+:BITSIZE_len] - 1)) < QUEUE_SIZE);  //Ready if next_queue_size - rlast < QUEUE_SIZE
    awready <= (next_awqueue_size - (next_awqueue_size > 0 && next_awqueue[OFFSET_delay+:BITSIZE_delay] == 1 && (next_awqueue[OFFSET_counter+:BITSIZE_counter] == next_awqueue[OFFSET_len+:BITSIZE_len])) < QUEUE_SIZE); // ready if next_queue_size - bvalid < QUEUE_SIZE
  ``wready <= (next_awqueue_size - (next_awqueue_size > 0 && next_awqueue[OFFSET_delay+:BITSIZE_delay] == 1 && (next_awqueue[OFFSET_counter+:BITSIZE_counter] == next_awqueue[OFFSET_len+:BITSIZE_len])) < QUEUE_SIZE); // ready if next_queue_size - bvalid < QUEUE_SIZE
   end
end

always@(posedge clock)
begin
   if(1RESET_VALUE)
   begin
    rvalid <= 0;
    bvalid <= 0;
   end
   else
   begin
    rvalid <= (next_arqueue_size > 0 && next_arqueue[OFFSET_delay+:BITSIZE_delay] == 1); // if at posedge_clock delay is 1 I have to perfrom the operation in this cycle
    bvalid <= (next_awqueue_size > 0 && next_awqueue[OFFSET_delay+:BITSIZE_delay] == 1 && (next_awqueue[OFFSET_counter+:BITSIZE_counter] == next_awqueue[OFFSET_len+:BITSIZE_len])); // if at posedge_clock delay is 1 I have to perfrom the operation in this cycle
   end
end

always@(posedge clock)
begin
   if(1RESET_VALUE)
   begin
    arqueue <= 0;
    arqueue_size <= 0;
    awqueue <= 0;
    awqueue_size <= 0;
   end
   else
   begin
    arqueue <= next_arqueue;
    arqueue_size <= next_arqueue_size;
    awqueue <= next_awqueue;
    awqueue_size <= next_awqueue_size;
   end
end

// Sequential logic for read transactions
always@(posedge clock)
begin : read_seq
  automatic ptr_t currAddr;
  automatic ptr_t endAddr;
  rlast <= 0;
  if(next_arqueue_size > 0 && next_arqueue[OFFSET_delay+:BITSIZE_delay] == 1)
  begin
    if(next_arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b00)
    begin
      currAddr = next_arqueue[OFFSET_addr+:BITSIZE_addr];
    end
    else if(next_arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b01)
    begin
      currAddr = next_arqueue[OFFSET_addr+:BITSIZE_addr] + next_arqueue[OFFSET_counter+:BITSIZE_counter] * (1 << next_arqueue[OFFSET_size+:BITSIZE_size]);
    end
    else if(next_arqueue[OFFSET_burst+:BITSIZE_burst] == 2'b10)
    begin
      endAddr = next_arqueue[OFFSET_addr+:BITSIZE_addr] - (next_arqueue[OFFSET_addr+:BITSIZE_addr] % ((next_arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_arqueue[OFFSET_size+:BITSIZE_size]))) + ((next_arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_arqueue[OFFSET_size+:BITSIZE_size]));
      currAddr = next_arqueue[OFFSET_addr+:BITSIZE_addr] + next_arqueue[OFFSET_counter+:BITSIZE_counter] * (1 << next_arqueue[OFFSET_size+:BITSIZE_size]);
      if(currAddr > endAddr)
      begin
        currAddr = currAddr - ((next_arqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_arqueue[OFFSET_size+:BITSIZE_size]));
      end
    end
    currAddr = currAddr - (currAddr % (1 << next_arqueue[OFFSET_size+:BITSIZE_size]));
    rdata <= m_utils.read(currAddr); // {_bambu_testbench_mem_[currAddr + 1 - base_addr], _bambu_testbench_mem_[currAddr + 0 - base_addr]};
    if(next_arqueue[OFFSET_counter+:BITSIZE_counter] == (next_arqueue[OFFSET_len+:BITSIZE_len] - 1))
    begin
      rlast <= 1;
    end
  end
end

// Sequential logic for write transactions
always@(posedge clock)
begin: write_seq
  automatic ptr_t currAddr;
  automatic ptr_t endAddr;
  test_strb <=0;
  test_addr <=0;
  test_data <=0;
  if(next_awqueue_size > 0 && next_awqueue[OFFSET_delay+:BITSIZE_delay] == 1) // Performs the first write of the queue
  begin
    if(next_awqueue[OFFSET_burst+:BITSIZE_burst] == 2'b00)
    begin
      currAddr = next_awqueue[OFFSET_addr+:BITSIZE_addr];
    end
    else if(next_awqueue[OFFSET_burst+:BITSIZE_burst] == 2'b01)
    begin
      currAddr = next_awqueue[OFFSET_addr+:BITSIZE_addr] + (next_awqueue[OFFSET_counter+:BITSIZE_counter] - 1) * (1 << next_awqueue[OFFSET_size+:BITSIZE_size]);
    end
    else if(next_awqueue[OFFSET_burst+:BITSIZE_burst] == 2'b10)
    begin
      endAddr = next_awqueue[OFFSET_addr+:BITSIZE_addr] 
        - (next_awqueue[OFFSET_addr+:BITSIZE_addr] % ((next_awqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_awqueue[OFFSET_size+:BITSIZE_size])))
        + ((next_awqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_awqueue[OFFSET_size+:BITSIZE_size]));
      currAddr = next_awqueue[OFFSET_addr+:BITSIZE_addr] 
        + (next_awqueue[OFFSET_counter+:BITSIZE_counter] - 1) * (1 << next_awqueue[OFFSET_size+:BITSIZE_size]);
      if(currAddr > endAddr)
      begin
        currAddr = currAddr - ((next_awqueue[OFFSET_len+:BITSIZE_len] + 1) * (1 << next_awqueue[OFFSET_size+:BITSIZE_size]));
      end
    end
  end
  if(next_awqueue[OFFSET_wstrb+:BITSIZE_wstrb] != 0)
  begin
    test_strb <= next_awqueue[OFFSET_wstrb+:BITSIZE_wstrb];
    test_addr <= currAddr;
    test_data <= next_awqueue[OFFSET_wdata+:BITSIZE_wdata];
    m_utils.write_strobe(next_awqueue[OFFSET_wstrb+:BITSIZE_wstrb], next_awqueue[OFFSET_wdata+:BITSIZE_wdata], currAddr);
  end
end
)";
}
