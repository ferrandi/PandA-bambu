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
 * @file TestbenchNoneHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the testbench model of an interface without handshake signals.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "TestbenchNoneHDLGenerator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_manager.hpp"
#include "utility.hpp"

TestbenchNoneHDLGenerator::TestbenchNoneHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchNoneHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir, unsigned int function_id,
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

   const auto arg_name = boost::replace_first_copy(mod_cir->get_id(), "if_none_", "");

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
      ip_components.push_back("TestbenchFifoWrite");
      out << "TestbenchFifoWrite #(.index(index),\n"
          << "  .BITSIZE_din(BITSIZE_data)) fifo_write(.clock(clock),\n"
          << "  .setup_port(setup_port),\n"
          << "  .done_port(done_port),\n"
          << "  .write(done_port),\n"
          << "  .din(" << din_port << "));\n";
   }
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::IP_COMPONENT,
                                            container_to_string(ip_components, ","));
}
