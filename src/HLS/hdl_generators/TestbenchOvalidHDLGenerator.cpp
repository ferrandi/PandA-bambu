/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2023-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file TestbenchOvalidHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the testbench model of an output-valid interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "TestbenchOvalidHDLGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "utility.hpp"

TestbenchOvalidHDLGenerator::TestbenchOvalidHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchOvalidHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
                                               unsigned int function_id, gc_vertex_descriptor /* op_v */,
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

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_ovalid_") - 1U, std::string::npos);

   const auto top_fname = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(arg_name);
   const auto if_dir = port_o::to_port_direction(iface_attrs.at(FunctionArchitecture::iface_direction));
   const std::string in_suffix = if_dir == port_o::IO ? "_i" : "";
   const std::string out_suffix = if_dir == port_o::IO ? "_o" : "";
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, mod_cir->get_id() + " index");
   out << "localparam BITSIZE_data=BITSIZE_" << arg_name << (in_suffix.size() ? in_suffix : out_suffix) << ";\n";
   std::vector<std::string> ip_components;
   if(if_dir == port_o::IN || if_dir == port_o::IO)
   {
      const auto dout_port = add_port(mod_cir, arg_name + in_suffix, port_o::OUT, 1U, true);
      ip_components.push_back("TestbenchFifoRead");
      out << "TestbenchFifoRead #(.index(index),\n"
          << "  .BITSIZE_dout(BITSIZE_data)) fifo_read(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .dout(" << dout_port << "));\n";
   }
   if(if_dir == port_o::OUT || if_dir == port_o::IO)
   {
      const auto din_port = add_port(mod_cir, arg_name + out_suffix, port_o::IN, 1U, true);
      const auto write_port = add_port(mod_cir, arg_name + out_suffix + "_vld", port_o::IN, 0U, true);
      ip_components.push_back("TestbenchFifoWrite");
      out << "wire _full_n;\n\n"
          << "TestbenchFifoWrite #(.index(index),\n"
          << "  .BITSIZE_din(BITSIZE_data)) fifo_write(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .full_n(_full_n),\n"
          << "  .write(" << write_port << "),\n"
          << "  .din(" << din_port << "));\n";
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::IP_COMPONENT,
                                            container_to_string(ip_components, ","));
}
