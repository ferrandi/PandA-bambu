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
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file ReadValidHDLGenerator.cpp
 * @brief Implementation of the HDL generator for a valid-based read interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "ReadValidHDLGenerator.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "structural_objects.hpp"

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
   i_data,
   i_vld,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_last
};

ReadValidHDLGenerator::ReadValidHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadValidHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                                         gc_vertex_descriptor /* op_v */, const HDLWriter_Language /* language */,
                                         const std::vector<HDLGenerator::parameter>& /* _p */,
                                         const std::vector<HDLGenerator::parameter>& _ports_in,
                                         const std::vector<HDLGenerator::parameter>& _ports_out,
                                         const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager().GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   if(iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end())
   {
      THROW_ERROR("Registered valid interface not yet implemented.");
   }
   out << "parameter ASYNC=0;\n\n"; // TODO: this should be controlled by InterfaceInfer step
   out << "reg started;\n"
       << "wire started0, active;\n\n"
       << "assign active = started | " << _ports_in[i_start].name << ";\n\n";

   out << R"(generate
if(ASYNC)
begin
  reg validated;
  wire validated0;
  reg [)"
       << (_ports_in[i_data].type_size - 1) << R"(:0] stored;

  always @(posedge clock 1RESET_EDGE)
  begin
    if (1RESET_VALUE)
    begin
      started <= 0;
      validated <= 0;
      stored <= 0;
    end
    else
    begin
      started <= started0;
      validated <= validated0;
      stored <= )"
       << _ports_in[i_data].name << R"(;
    end
  end

  assign started0 = active & ~(validated | )"
       << _ports_in[i_vld].name << R"();
  assign validated0 = (validated | )"
       << _ports_in[i_vld].name << R"() & ~active;

  assign )"
       << _ports_out[o_out1].name << " = " << _ports_in[i_vld].name << " ? " << _ports_in[i_data].name << R"( : stored;
  assign )"
       << _ports_out[o_done].name << " = (active & " << _ports_in[i_vld].name << ") | (validated & "
       << _ports_in[i_start].name << R"();
end
else
begin
  always @(posedge clock 1RESET_EDGE)
  begin
    if (1RESET_VALUE)
    begin
      started <= 0;
    end
    else
    begin
      started <= started0;
    end
  end

  assign started0 = active & ~)"
       << _ports_in[i_vld].name << R"(;

  assign )"
       << _ports_out[o_out1].name << " = " << _ports_in[i_data].name << R"(;
  assign )"
       << _ports_out[o_done].name << " = active & " << _ports_in[i_vld].name << R"(;
end
endgenerate)";
}
