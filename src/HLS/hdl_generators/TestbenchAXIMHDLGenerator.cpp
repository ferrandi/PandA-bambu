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
 * @file TestbenchAXIMHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the testbench model of an AXI master interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "TestbenchAXIMHDLGenerator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"

TestbenchAXIMHDLGenerator::TestbenchAXIMHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchAXIMHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
                                             unsigned int /* function_id */, gc_vertex_descriptor /* op_v */,
                                             const HDLWriter_Language language,
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

   const auto port_prefix = mod_cir->get_id().substr(sizeof("if_") - 1U, std::string::npos);
   std::string np_library = mod_cir->get_id() + " index";
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
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::IP_COMPONENT, "TestbenchMEMAXI");

   if(HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_write) == 1)
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 4,
                    "Warning: AXI does not support mem-delay-write==1, as it requires at least a cycle of latency -> "
                    "mem-delay-write changed to 2.\n");
   }
   out << internal_port_assign << "\n"
       << "TestbenchMEMAXI #(.index(index),\n"
       << ".WRITE_DELAY(" << std::max(1U, (HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_write) - 1))
       << "),\n"
       << ".READ_DELAY(" << (HLSMgr->get_parameter()->getOption<unsigned int>(OPT_mem_delay_read) - 1) << "),\n"
       << ".QUEUE_SIZE(" << (HLSMgr->get_parameter()->getOption<unsigned int>(OPT_tb_queue_size) + 1)
       << "),\n" // + 1 needed to create a buffer in case of a valid response not accepted by the accelerator and a
                 // valid input.
       << ".BITSIZE_bid(BITSIZE_" << port_prefix << "_bid),\n"
       << ".BITSIZE_bresp(BITSIZE_" << port_prefix << "_bresp),\n"
       << ".BITSIZE_buser(BITSIZE_" << port_prefix << "_buser),\n"
       << ".BITSIZE_rid(BITSIZE_" << port_prefix << "_rid),\n"
       << ".BITSIZE_rdata(BITSIZE_" << port_prefix << "_rdata),\n"
       << ".BITSIZE_rresp(BITSIZE_" << port_prefix << "_rresp),\n"
       << ".BITSIZE_ruser(BITSIZE_" << port_prefix << "_ruser),\n"
       << ".BITSIZE_awid(BITSIZE_" << port_prefix << "_awid),\n"
       << ".BITSIZE_awaddr(BITSIZE_" << port_prefix << "_awaddr),\n"
       << ".BITSIZE_awlen(BITSIZE_" << port_prefix << "_awlen),\n"
       << ".BITSIZE_awsize(BITSIZE_" << port_prefix << "_awsize),\n"
       << ".BITSIZE_awburst(BITSIZE_" << port_prefix << "_awburst),\n"
       << ".BITSIZE_awlock(BITSIZE_" << port_prefix << "_awlock),\n"
       << ".BITSIZE_awcache(BITSIZE_" << port_prefix << "_awcache),\n"
       << ".BITSIZE_awprot(BITSIZE_" << port_prefix << "_awprot),\n"
       << ".BITSIZE_awqos(BITSIZE_" << port_prefix << "_awqos),\n"
       << ".BITSIZE_awregion(BITSIZE_" << port_prefix << "_awregion),\n"
       << ".BITSIZE_awuser(BITSIZE_" << port_prefix << "_awuser),\n"
       << ".BITSIZE_wdata(BITSIZE_" << port_prefix << "_wdata),\n"
       << ".BITSIZE_wstrb(BITSIZE_" << port_prefix << "_wstrb),\n"
       << ".BITSIZE_wuser(BITSIZE_" << port_prefix << "_wuser),\n"
       << ".BITSIZE_arid(BITSIZE_" << port_prefix << "_arid),\n"
       << ".BITSIZE_araddr(BITSIZE_" << port_prefix << "_araddr),\n"
       << ".BITSIZE_arlen(BITSIZE_" << port_prefix << "_arlen),\n"
       << ".BITSIZE_arsize(BITSIZE_" << port_prefix << "_arsize),\n"
       << ".BITSIZE_arburst(BITSIZE_" << port_prefix << "_arburst),\n"
       << ".BITSIZE_arlock(BITSIZE_" << port_prefix << "_arlock),\n"
       << ".BITSIZE_arcache(BITSIZE_" << port_prefix << "_arcache),\n"
       << ".BITSIZE_arprot(BITSIZE_" << port_prefix << "_arprot),\n"
       << ".BITSIZE_arqos(BITSIZE_" << port_prefix << "_arqos),\n"
       << ".BITSIZE_arregion(BITSIZE_" << port_prefix << "_arregion),\n"
       << ".BITSIZE_aruser(BITSIZE_" << port_prefix << "_aruser))"
       << R"(
axi4_tb (.awready(awready),
.wready(wready),
.bid(bid),
.bresp(bresp),
.buser(buser),
.bvalid(bvalid),
.arready(arready),
.rid(rid),
.rdata(rdata),
.rresp(rresp),
.rlast(rlast),
.ruser(ruser),
.rvalid(rvalid),
.clock(clock),
.reset(reset),
.awid(awid),
.awaddr(awaddr),
.awlen(awlen),
.awsize(awsize),
.awburst(awburst),
.awlock(awlock),
.awcache(awcache),
.awprot(awprot),
.awqos(awqos),
.awregion(awregion),
.awuser(awuser),
.awvalid(awvalid),
.wdata(wdata),
.wstrb(wstrb),
.wlast(wlast),
.wuser(wuser),
.wvalid(wvalid),
.bready(bready),
.arid(arid),
.araddr(araddr),
.arlen(arlen),
.arsize(arsize),
.arburst(arburst),
.arlock(arlock),
.arcache(arcache),
.arprot(arprot),
.arqos(arqos),
.arregion(arregion),
.aruser(aruser),
.arvalid(arvalid),
.rready(rready));)";
}
