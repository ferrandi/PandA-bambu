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
 * @file TestbenchArrayHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the testbench model of an array-based interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "TestbenchArrayHDLGenerator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"
#include "utility.hpp"

TestbenchArrayHDLGenerator::TestbenchArrayHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchArrayHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir, unsigned int function_id,
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

   const auto arg_name = mod_cir->get_id().substr(sizeof("if_array_") - 1U, std::string::npos);

   const auto top_bh = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(arg_name);
   const auto if_dir = port_o::to_port_direction(iface_attrs.at(FunctionArchitecture::iface_direction));
   const auto n_channels = HLSMgr->get_parameter()->getOption<unsigned int>(OPT_channels_number);

   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY,
                                            mod_cir->get_id() + " index WRITE_DELAY READ_DELAY");
   const auto add_port_parametric = [&](unsigned idx, const std::string& name, port_o::port_direction dir,
                                        unsigned port_size) {
      return add_port(mod_cir, arg_name + "_" + name + std::to_string(idx), dir, port_size, true);
   };
   std::vector<std::string> ce, we, address, d, q;
   for(unsigned i = 0; i < n_channels; ++i)
   {
      address.push_back(add_port_parametric(i, "address", port_o::IN, 1U));
      ce.push_back(add_port_parametric(i, "ce", port_o::IN, 0U));
      if(if_dir != port_o::IN)
      {
         we.push_back(add_port_parametric(i, "we", port_o::IN, 0U));
         d.push_back(add_port_parametric(i, "d", port_o::IN, 1U));
      }
      else
      {
         we.push_back("1'b0");
         d.push_back("{BITSIZE_dq{1'b0}}");
      }
      if(if_dir != port_o::OUT)
      {
         q.push_back(add_port_parametric(i, "q", port_o::OUT, 1U));
      }
   }

   out << "localparam CHANNELS_NUMBER=" << n_channels << ",\n"
       << "  BITSIZE_address=BITSIZE_" << arg_name << "_address0,\n"
       << "  BITSIZE_dq=BITSIZE_" << arg_name << (if_dir == port_o::IN ? "_q0" : "_d0") << ";\n\n";

   out << "TestbenchArrayImpl #(.index(index),\n"
       << "  .WRITE_DELAY(WRITE_DELAY),\n"
       << "  .READ_DELAY(READ_DELAY),\n"
       << "  .PORTSIZE_address(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_address(BITSIZE_address),\n"
       << "  .PORTSIZE_ce(CHANNELS_NUMBER),\n"
       << "  .PORTSIZE_we(CHANNELS_NUMBER),\n"
       << "  .PORTSIZE_d(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_d(BITSIZE_dq),\n"
       << "  .PORTSIZE_q(CHANNELS_NUMBER),\n"
       << "  .BITSIZE_q(BITSIZE_dq)) array_impl(.clock(clock),\n"
       << "  .setup_port(setup_port),\n"
       << "  .ce({" << container_to_string(ce, ",") << "}),\n"
       << "  .we({" << container_to_string(we, ",") << "}),\n"
       << "  .address({" << container_to_string(address, ",") << "}),\n"
       << "  .d({" << container_to_string(d, ",") << "})";
   if(q.size())
   {
      out << ",\n  .q({" << container_to_string(q, ",") << "})";
   }
   out << ");\n";
}
