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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file csim_testbench_c_writer.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "csim_testbench_c_writer.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "var_pp_functor.hpp"

CSimTestbenchCWriter::CSimTestbenchCWriter(const HLS_managerConstRef _HLSMgr,
                                           const InstructionWriterRef _instruction_writer,
                                           const IndentedOutputStreamRef _indented_output_stream,
                                           const std::filesystem::path& _csim_output,
                                           const std::filesystem::path& _bbp_output)
    : CWriter(_HLSMgr, _instruction_writer, _indented_output_stream), csim_output(_csim_output), bbp_output(_bbp_output)
{
}

void CSimTestbenchCWriter::InternalInitialize()
{
   declared_functions = HLSMgr->CGetCallGraphManager().GetRootFunctions();
}

void CSimTestbenchCWriter::InternalWriteHeader()
{
   indented_output_stream->Append("#define __BAMBU_IPC_ENTITY MDPI_ENTITY_SIM\n");
   indented_output_stream->Append("#include <mdpi/mdpi_csim.h>\n");
   indented_output_stream->Append("#include <mdpi/mdpi_debug.h>\n\n");
   indented_output_stream->Append("#include <assert.h>\n");
   indented_output_stream->Append("#include <stdlib.h>\n");

   CWriter::InternalWriteHeader();
}

void CSimTestbenchCWriter::InternalWriteGlobalDeclarations()
{
   const auto bbp_output_str = bbp_output.empty() ? "NULL" : ("\"" + bbp_output.string() + "\"");
   indented_output_stream->Append("const char* __bambu_csim_results_filename = \"" + csim_output.string() + "\";\n");
   indented_output_stream->Append("const char* __bambu_csim_design_stats_filename = " + bbp_output_str + ";\n");
   indented_output_stream->Append(R"(
#define STATE_READY    0b0000001
#define STATE_SETUP    0b0000010
#define STATE_RUNNING  0b0000100
#define STATE_END      0b0001000
#define STATE_ERROR    0b0010000
#define STATE_ABORT    0b0100000
#define SIM_DONE       0b1000000

)");
}

void CSimTestbenchCWriter::InternalWriteFile()
{
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   const auto& top_functions = CGM.GetRootFunctions();
   for(const auto top_id : top_functions)
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper();
      const auto fnode = TM->GetIRNode(top_id);
      const auto ret_type = ir_helper::GetFunctionReturnType(fnode);
      const auto fsymbol = BH->GetFunctionName();
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);

      std::string arg_decls;
      std::string arg_setup;
      std::string dut_call;
      std::string rdelay = "const unsigned *__bambu_artificial_delay_read = (unsigned[]){";
      std::string wdelay = "const unsigned *__bambu_artificial_delay_write = (unsigned[]){";
      if(ret_type)
      {
         const auto ret_type_str = ir_helper::PrintType(ret_type);
         arg_decls += ret_type_str + " retval;\n";
         dut_call += "retval = ";
      }
      dut_call += fsymbol + "(";

      size_t if_idx = 0;
      for(const auto& arg : BH->GetParameters())
      {
         const auto arg_name = BH->PrintVariable(arg->index);
         auto raw_typename = ir_helper::PrintType(arg);
         if(raw_typename.find("(*)") != std::string::npos)
         {
            raw_typename = raw_typename.substr(0, raw_typename.find("(*)")) + "*";
         }
         const auto& parm_attrs = func_arch->parms.at(arg_name);
         const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
         const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + arg_name);
         const auto& arg_interface = iface_attrs.at(FunctionArchitecture::iface_mode);
         const auto& arg_bitsize = iface_attrs.at(FunctionArchitecture::iface_bitwidth);

         arg_decls += raw_typename + " " + arg_name + ";\n";
         arg_setup +=
             "if_setup_" + arg_interface + "(" + std::to_string(if_idx) + ", " + arg_name + ", " + arg_bitsize + ");\n";
         if(if_idx > 0)
         {
            dut_call += ", ";
         }
         dut_call += arg_name;
         rdelay += "0, ";
         wdelay += "0, ";
         ++if_idx;
      }
      dut_call += ");\n";
      if(ret_type)
      {
         dut_call += "if_done_retval(" + std::to_string(if_idx) + ", retval, " +
                     std::to_string(ir_helper::Size(ret_type)) + ");\n";
         rdelay += "0, ";
         wdelay += "0, ";
      }
      rdelay += std::to_string(Param->getOption<int>(OPT_mem_delay_read)) + "};\n";
      wdelay += std::to_string(Param->getOption<int>(OPT_mem_delay_write)) + "};\n";

      indented_output_stream->Append(rdelay);
      indented_output_stream->Append(wdelay);

      indented_output_stream->Append("\n\nstatic int dut_" + fsymbol + "()\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("unsigned int __present_state;\n");
      indented_output_stream->Append("struct __bambu_csim_function_stats* __top_stats;\n\n");
      indented_output_stream->Append(arg_decls);
      indented_output_stream->Append("\n__top_stats = __bambu_csim_get_stats(&__bambu_csim_current_run, " +
                                     std::to_string(top_id) + ");");
      indented_output_stream->Append(R"(
while(1)
{
__present_state = m_next(__present_state);
switch(__present_state & 0xFF)
{
case STATE_READY:
   break;
case STATE_SETUP:
{
)");
      indented_output_stream->Append(arg_setup);
      indented_output_stream->Append("\n__bambu_csim_module_push(__top_stats, 0);\n");
      indented_output_stream->Append("__bambu_csim_results_append(__bambu_csim_results_filename, \"0|\");\n\n");
      indented_output_stream->Append("/* STATE_RUNNING */\n");
      indented_output_stream->Append(dut_call);
      indented_output_stream->Append("\n/* STATE_DONE */");
      indented_output_stream->Append(R"(
__bambu_csim_module_pop(1);
info("Run %u: %llu cycles.\n", __bambu_csim_current_run.run_id, __top_stats->last_cycle);
__bambu_csim_design_stats_rotate(&__bambu_csim_current_run, __bambu_csim_design_stats_filename);
__bambu_csim_results_append(__bambu_csim_results_filename, "%llu,", __top_stats->last_cycle * 2);
__present_state = STATE_READY;
break;
}
case STATE_ERROR:
   error("Testbench reported error state: %u.\n", __present_state >> 8);
   return EXIT_SUCCESS;
case STATE_ABORT:
   error("Testbench aborted.\n");
   return EXIT_SUCCESS;
case STATE_END:
   return EXIT_SUCCESS;
default:
   error("Unknown state: %u (%u).\n", __present_state & 0xFF, __present_state >> 8);
   return EXIT_FAILURE;
}
}
return EXIT_SUCCESS;
)");
      indented_output_stream->Append("}\n");
   }

   const auto BH = HLSMgr->CGetFunctionBehavior(*top_functions.begin())->CGetBehavioralHelper();
   const auto fsymbol = BH->GetFunctionName();
   indented_output_stream->Append(R"(
int main()
{
int retval;

remove(__bambu_csim_results_filename);
if(__bambu_csim_design_stats_filename)
{
remove(__bambu_csim_design_stats_filename);
}

)");
   indented_output_stream->Append("retval = dut_" + fsymbol + "();");
   indented_output_stream->Append(R"(
if(retval != EXIT_SUCCESS)
{
error("DUT failed with code: %d\n", retval);
}
retval = m_fini() >> 8;
__bambu_csim_results_append(__bambu_csim_results_filename, "\n%d", retval);
return retval;
})");
}
