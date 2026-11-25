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
 * @file ReadWriteDPArrayHDLGenerator.cpp
 * @brief Implementation of the HDL generator for the dual-port read/write array interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "ReadWriteDPArrayHDLGenerator.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_objects.hpp"

#include <algorithm>

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
   i_q0,
   i_q1,
   i_last
};

enum out_port
{
   o_out1 = 0,
   o_address0,
   o_address1,
   o_ce0,
   o_ce1,
   o_we0,
   o_we1,
   o_d0,
   o_d1,
   o_last
};

ReadWriteDPArrayHDLGenerator::ReadWriteDPArrayHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWriteDPArrayHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                                                gc_vertex_descriptor /* op_v */,
                                                const HDLWriter_Language /* language */,
                                                const std::vector<HDLGenerator::parameter>& /* _p */,
                                                const std::vector<HDLGenerator::parameter>& _ports_in,
                                                const std::vector<HDLGenerator::parameter>& _ports_out,
                                                const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager().GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetFunctionName();
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(top_fname);
   THROW_ASSERT(func_arch, "Expected function architecture for function " + top_fname);
   const auto stall_port =
       std::find_if(_ports_in.begin(), _ports_in.end(), [](const auto& port) { return port.name == STALL_PORT_NAME; });
   const unsigned int operand_offset = stall_port == _ports_in.end() ? 0U : 1U;
   const auto& in1 = _ports_in[i_in1 + operand_offset];
   const auto& in3 = _ports_in[i_in3 + operand_offset];
   const auto& in4 = _ports_in[i_in4 + operand_offset];
   const auto q0_port = i_q0 + operand_offset;
   const auto q1_port = i_q1 + operand_offset;
   const auto isAlignedPowerOfTwo = in4.alignment == ceil_pow2(in4.alignment);
   const auto stall_term = stall_port == _ports_in.end() ? "" : " & ~" + stall_port->name;
   const auto request0 = _ports_in[i_start].name + "[0]" + stall_term;
   const auto request1 = _ports_in[i_start].name + "[1]" + stall_term;

   out << "//" << (isAlignedPowerOfTwo ? "T" : "F") << "\n";
   out << "assign " << _ports_out[o_ce0].name << " = " << request0 << ";\n";
   out << "assign " << _ports_out[o_ce1].name << " = " << request1 << ";\n";

   if(isAlignedPowerOfTwo)
   {
      out << "assign " << _ports_out[o_address0].name << " = " << in4.name << "[BITSIZE_" << in4.name << "*0+:BITSIZE_"
          << in4.name << "] / " << in4.alignment << ";\n";
      out << "assign " << _ports_out[o_address1].name << " = " << in4.name << "[BITSIZE_" << in4.name << "*1+:BITSIZE_"
          << in4.name << "] / " << in4.alignment << ";\n";
   }
   else
   {
      out << "assign " << _ports_out[o_address0].name << " = " << in4.name << "[2+(BITSIZE_" << in4.name
          << ")*0+:BITSIZE_" << in4.name << " - 2] / " << in4.alignment / 4 << ";\n";
      out << "assign " << _ports_out[o_address1].name << " = " << in4.name << "[2+BITSIZE_" << in4.name
          << "*1+:BITSIZE_" << in4.name << " - 2] / " << in4.alignment / 4 << ";\n";
   }

   if(_ports_in.size() > q0_port)
   {
      out << "assign " << _ports_out[o_out1].name << "[BITSIZE_" << _ports_out[o_out1].name << "*0+:BITSIZE_"
          << _ports_out[o_out1].name << "] = " << _ports_in[q0_port].name << ";\n";
   }
   if(_ports_in.size() > q1_port)
   {
      out << "assign " << _ports_out[o_out1].name << "[BITSIZE_" << _ports_out[o_out1].name << "*1+:BITSIZE_"
          << _ports_out[o_out1].name << "] = " << _ports_in[q1_port].name << ";\n";
   }

   if(_ports_out.size() > o_d1)
   {
      out << "assign " << _ports_out[o_we0].name << " = " << request0 << " & (|" << in1.name << "[BITSIZE_" << in1.name
          << "*0+:BITSIZE_" << in1.name << "]);\n";
      out << "assign " << _ports_out[o_d0].name << " = " << in3.name << "[BITSIZE_" << in3.name << "*0+:BITSIZE_"
          << in3.name << "];\n";
      out << "assign " << _ports_out[o_we1].name << " = " << request1 << " & (|" << in1.name << "[BITSIZE_" << in1.name
          << "*1+:BITSIZE_" << in1.name << "]);\n";
      out << "assign " << _ports_out[o_d1].name << " = " << in3.name << "[BITSIZE_" << in3.name << "*1+:BITSIZE_"
          << in3.name << "];\n";
   }
}
