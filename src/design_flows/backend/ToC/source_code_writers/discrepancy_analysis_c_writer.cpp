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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include "discrepancy_analysis_c_writer.hpp"

#include "Discrepancy.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "allocation_information.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "c_backend_information.hpp"
#include "c_writer.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_c_writer.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "language_writer.hpp"
#include "memory.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "technology_node.hpp"
#include "var_pp_functor.hpp"
#include <filesystem>
#include <string>
#include <vector>

#define INT_TYPE 0
#define UINT_TYPE 1
#define FLOAT_TYPE 2
#define DOUBLE_TYPE 3
#define F_SIGN(out, in) (((out & 3) << 2) | (in & 3))
#define F_TYPE_IN(f_sign) (f_sign & 3)
#define F_TYPE_OUT(f_sign) ((f_sign >> 2) & 3)

/*
 * Integer variables larger than 64 bits are not supported by bambu which treats them as vectors. But the
 * ir_helper::is_vector function returns false for those large integers. This function is used to detect when an integer
 * variable (non-vector is actually handled as a vector from bambu.
 */
static inline bool is_large_integer(const ir_nodeConstRef& tn)
{
   const auto type = GetPointer<const type_node>(tn);
   THROW_ASSERT(type, "type_id " + STR(tn->index) + " is not a type");
   if(tn->get_kind() != integer_ty_node_K)
   {
      return false;
   }
   const auto it = GetPointer<const integer_ty_node>(tn);
   THROW_ASSERT(it, "type " + STR(tn->index) + " is not an integer type");
   if((it->bitsize != type->algn && it->bitsize > 64) || (type->algn == 128))
   {
      return true;
   }

   return false;
}

static void maybe_record_call_id(const HLS_managerConstRef& HLSMgr, const IndentedOutputStreamRef& out,
                                 const unsigned int called_id, const unsigned int st_tn_id)
{
   const auto bh = HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper();
   if(bh->has_implementation() && bh->function_has_to_be_printed(called_id))
   {
      out->Append("__bambu_discrepancy_last_call_id = " + STR(st_tn_id) + ";\n");
   }
}

static void append_addr_table_entry(const IndentedOutputStreamRef& out, const unsigned int var_id,
                                    const std::string& var_name)
{
   out->Append("if(!__bambu_discrepancy_first_addr) { fputc(',', __bambu_discrepancy_fp); }\n");
   out->Append("__bambu_discrepancy_first_addr = 0;\n");
   out->Append("fprintf(__bambu_discrepancy_fp, \"\\x7b\\\"var_id\\\":" + STR(var_id) + ",\\\"addr\\\":%lu\\x7d\", &" +
               var_name + ");\n");
}

static void append_context_end_event(const IndentedOutputStreamRef& out)
{
   out->Append("__bambu_discrepancy_emit_event_sep();\n");
   out->Append("fprintf(__bambu_discrepancy_fp, \"\\x7b\\\"type\\\":\\\"context_end\\\"\\x7d\");\n");
   out->Append("fputc('\\n', __bambu_discrepancy_fp);\n");
}

static void append_control_flow_event(const IndentedOutputStreamRef& out, const unsigned int bb_number)
{
   out->Append("__bambu_discrepancy_emit_event_sep();\n");
   out->Append("fprintf(__bambu_discrepancy_fp, \"\\x7b\\\"type\\\":\\\"control_flow\\\",\\\"bb\\\":" + STR(bb_number) +
               "\\x7d\");\n");
   out->Append("fputc('\\n', __bambu_discrepancy_fp);\n");
}

DiscrepancyAnalysisCWriter::DiscrepancyAnalysisCWriter(const CBackendInformationConstRef _c_backend_information,
                                                       const HLS_managerConstRef _HLSMgr,
                                                       const InstructionWriterRef _instruction_writer,
                                                       const IndentedOutputStreamRef _indented_output_stream)
    : HLSCWriter(_c_backend_information, _HLSMgr, _instruction_writer, _indented_output_stream),
      Discrepancy(_HLSMgr->RDiscr)
{
   THROW_ASSERT((Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)),
                "Step " + STR(__PRETTY_FUNCTION__) + " should not be added without discrepancy");
   THROW_ASSERT(Discrepancy, "Discrepancy data structure is not correctly initialized");
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

void DiscrepancyAnalysisCWriter::InternalInitialize()
{
   CWriter::InternalInitialize();
}

void DiscrepancyAnalysisCWriter::InternalWriteFile()
{
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_fnode->index);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetFunctionName();
   const auto return_type = ir_helper::GetFunctionReturnType(top_fnode);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "-->Discrepancy analysis testbench generation for function " + top_fname);

   const auto& test_vectors = HLSMgr->RSim->test_vectors;

   indented_output_stream->Append("int main()\n{\n");
   // write additional initialization code needed by subclasses
   WriteExtraInitCode();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written extra init code");

   // parameters declaration
   WriteParamDecl(top_bh);

   // declaration of the return variable of the top function, if not void
   if(return_type)
   {
      const auto ret_type = ir_helper::PrintType(return_type);
      if(ir_helper::IsVectorType(return_type))
      {
         THROW_ERROR("return type of function under test " + top_fname + " is " + STR(ret_type) +
                     "\nco-simulation does not support vectorized return types at top level");
      }
      indented_output_stream->Append("// return variable initialization\n");
      indented_output_stream->Append(ret_type + " " RETURN_PORT_NAME ";\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written parameters declaration");
   // ---- WRITE PARAMETERS INITIALIZATION AND FUNCTION CALLS ----
   for(unsigned int v_idx = 0; v_idx < test_vectors.size(); v_idx++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for test vector " + STR(v_idx));
      indented_output_stream->Append("{\n");
      const auto& curr_test_vector = test_vectors.at(v_idx);
      // write parameter initialization
      indented_output_stream->Append("// parameter initialization\n");
      WriteParamInitialization(top_bh, curr_test_vector);
      WriteExtraCodeBeforeEveryMainCall();
      // write the call to the top function to be tested
      indented_output_stream->Append("// function call\n");
      WriteTestbenchFunctionCall(top_bh);

      indented_output_stream->Append("}\n");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization for test vector " + STR(v_idx));
   }
   // print exit statements
   indented_output_stream->Append("__standard_exit = 1;\n");
   indented_output_stream->Append("exit(0);\n");
   indented_output_stream->Append("}\n");
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Prepared testbench");
}

void DiscrepancyAnalysisCWriter::writePreInstructionInfo(const FunctionBehaviorConstRef FB,
                                                         OpGraph::vertex_descriptor statement)
{
   const auto instrGraph = FB->GetOpGraph(FunctionBehavior::FCFG);
   const auto& node_info = instrGraph.CGetNodeInfo(statement);
   const auto st_tn_id = node_info.GetNodeId();
   if(st_tn_id == 0 || st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
   {
      return;
   }
   const ir_nodeConstRef curr_tn = TM->GetIRNode(st_tn_id);
   const auto kind = curr_tn->get_kind();
   if(kind == return_stmt_K)
   {
      append_context_end_event(indented_output_stream);
   }
   else if(kind == call_stmt_K)
   {
      THROW_ASSERT(!node_info.called.empty(),
                   "IR node " + STR(st_tn_id) + " is a call_stmt but does not actually call a function");
      THROW_ASSERT(node_info.called.size() == 1, "IR node " + STR(st_tn_id) + " calls more than a function");
      const auto called_id = *node_info.called.begin();
      const ir_nodeConstRef called_fun_decl_node = TM->GetIRNode(called_id);
      const auto* fu_dec = GetPointer<const function_val_node>(called_fun_decl_node);
      if(GetPointer<const identifier_node>(fu_dec->name)->strg == BUILTIN_WAIT_CALL)
      {
         /*
          * This operation calls a function with a function pointer, which is
          * implemented in bambu with the builtin function * __builtin_wait_call()
          * When a function is called with a function pointer we always need to
          * have its C source code, so it always has to be printed back in C.
          */
         indented_output_stream->Append("__bambu_discrepancy_last_call_id = " + STR(st_tn_id) + ";\n");
         return;
      }
      maybe_record_call_id(HLSMgr, indented_output_stream, called_id, st_tn_id);
   }
   else if(kind == assign_stmt_K)
   {
      const auto* g_as_node = GetPointer<const assign_stmt>(curr_tn);
      if(g_as_node->op1->get_kind() == call_node_K)
      {
         THROW_ASSERT(!node_info.called.empty(), "rhs of assign_stmt node " + STR(st_tn_id) +
                                                     " is a call_node but does not actually call a function");
         THROW_ASSERT(node_info.called.size() == 1,
                      "rhs of assign_stmt node " + STR(st_tn_id) + " is a call_node but calls more than a function");
         const auto called_id = *node_info.called.begin();
         maybe_record_call_id(HLSMgr, indented_output_stream, called_id, st_tn_id);
      }
   }
}

void DiscrepancyAnalysisCWriter::writePostInstructionInfo(const FunctionBehaviorConstRef fun_behavior,
                                                          OpGraph::vertex_descriptor statement)
{
   const auto instrGraph = fun_behavior->GetOpGraph(FunctionBehavior::FCFG);
   const auto st_tn_id = instrGraph.CGetNodeInfo(statement).GetNodeId();
   if(st_tn_id == 0 || st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
   {
      return;
   }
   const ir_nodeConstRef curr_tn = TM->GetIRNode(st_tn_id);
   if(curr_tn->get_kind() != assign_stmt_K && curr_tn->get_kind() != phi_stmt_K)
   {
      return;
   }

   const BehavioralHelperConstRef BH = fun_behavior->CGetBehavioralHelper();
   if(Param->isOption(OPT_discrepancy_only))
   {
      const auto discrepancy_functions = Param->getOption<CustomSet<std::string>>(OPT_discrepancy_only);
      if(!discrepancy_functions.empty() &&
         discrepancy_functions.find(BH->GetFunctionName()) == discrepancy_functions.end())
      {
         return;
      }
   }
   const hlsConstRef hls = HLSMgr->get_HLS(BH->get_function_index());

   technology_nodeRef fu_tech_n = hls->allocation_information->get_fu(hls->Rfu->get_assign(statement));
   technology_nodeRef op_tech_n = GetPointer<functional_unit>(fu_tech_n)->get_operation(
       ir_helper::NormalizeTypename(instrGraph.CGetNodeInfo(statement).GetOperation()));

   const operation* oper = GetPointer<operation>(op_tech_n);
   if(!oper)
   {
      return;
   }

   const auto g_as_node = GetPointer<const assign_stmt>(curr_tn);
   const auto g_phi_node = GetPointer<const phi_stmt>(curr_tn);
   bool is_virtual = false;
   ir_nodeRef ssa;
   if(g_as_node)
   {
      ssa = g_as_node->op0;
   }
   else if(g_phi_node)
   {
      ssa = g_phi_node->res;
      is_virtual = g_phi_node->virtual_flag;
   }

   if(ssa && ssa->get_kind() == ssa_node_K && !is_virtual)
   {
      /*
       * print statements that increase the counters used for coverage statistics
       */
      indented_output_stream->Append("__bambu_discrepancy_tot_assigned_ssa++;\n");
      bool is_discrepancy_address = Discrepancy->address_ssa.count(ssa);
      bool is_lost = Discrepancy->ssa_to_skip.count(ssa);
      bool has_no_meaning_in_software = is_discrepancy_address && Discrepancy->ssa_to_skip_if_address.count(ssa);
      if(is_lost || has_no_meaning_in_software)
      {
         indented_output_stream->Append("__bambu_discrepancy_tot_lost_ssa++;\n");
         if(is_discrepancy_address)
         {
            indented_output_stream->Append("__bambu_discrepancy_tot_lost_addr_ssa++;\n");
            if(has_no_meaning_in_software)
            {
               /*
                * These are ssa inserted by hls bit value optimization (like
                * treating sums with bid shifts to use smaller hw components for
                * sums). They are not present in the original source code and
                * they have no meaning in sw, because they come from aggressive
                * bit manipulations, which can originate intermediate address
                * values that cannot be mapped in sw in any way.
                */
               indented_output_stream->Append("__bambu_discrepancy_opt_lost_addr_ssa++;\n");
               Discrepancy->n_checked_operations++;
            }
         }
         else
         {
            THROW_ASSERT(!has_no_meaning_in_software,
                         "IR node " + STR(ssa->index) + " has no meaning in software but is not an address");
            indented_output_stream->Append("__bambu_discrepancy_tot_lost_int_ssa++;\n");
         }
         return;
      }

      indented_output_stream->Append("__bambu_discrepancy_tot_check_ssa++;\n");
      if(is_discrepancy_address)
      {
         indented_output_stream->Append("__bambu_discrepancy_tot_check_addr_ssa++;\n");
      }
      else
      {
         indented_output_stream->Append("__bambu_discrepancy_tot_check_int_ssa++;\n");
      }
      Discrepancy->n_checked_operations++;
      /*
       * print statements to print information on the instruction
       */
      const auto ssa_type = ir_helper::CGetType(ssa);
      const auto type_bitsize = ir_helper::SizeAlloc(ssa_type);
      const auto var_name = BH->PrintVariable(ssa->index);
      const auto ssa_bitsize = ir_helper::Size(ssa);
      if(ssa_bitsize > type_bitsize)
      {
         THROW_ERROR(std::string("variable size mismatch: ") + "ssa node id = " + STR(ssa->index) + " has size = " +
                     STR(ssa_bitsize) + " type node id = " + STR(ssa_type->index) + " has size = " + STR(type_bitsize));
      }
      const bool is_real = ir_helper::IsRealType(ssa_type);
      const bool is_vector = ir_helper::IsVectorType(ssa_type);

      THROW_ASSERT(!(is_discrepancy_address && (is_real)),
                   "variable " + STR(var_name) + " with node id " + STR(ssa->index) +
                       " has type id = " + STR(ssa_type->index) + " is real = " + STR(is_real));

      indented_output_stream->Append("__bambu_discrepancy_emit_event_sep();\n");
      indented_output_stream->Append(
          "fprintf(__bambu_discrepancy_fp, \"\\x7b\\\"type\\\":\\\"op_trace\\\",\\\"op_id\\\":" +
          STR(instrGraph.CGetNodeInfo(statement).GetNodeId()) + ",\\\"value\\\":\\\"\");\n");

      if(is_real || is_vector || ir_helper::IsStructType(ssa_type) || is_large_integer(ssa_type))
      {
         indented_output_stream->Append("_Ptd2Bin_(__bambu_discrepancy_fp, (unsigned char*)&" + var_name + ", " +
                                        STR(type_bitsize) + ");\n");
      }
      else
      {
         indented_output_stream->Append("_Dec2Bin_(__bambu_discrepancy_fp, (long long int)" + var_name + ", " +
                                        STR(type_bitsize) + ");\n");
      }

      indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"\\\"\\x7d\");\n");
      indented_output_stream->Append("fputc('\\n', __bambu_discrepancy_fp);\n");
      /// check if we need to add a check for floating operation correctness
      if(g_as_node)
      {
         const auto rhs = g_as_node->op1;
         if(rhs->get_kind() == call_node_K)
         {
            indented_output_stream->Append("//" + oper->get_name() + "\n");
            const auto& node_info = instrGraph.CGetNodeInfo(statement);
            THROW_ASSERT(!node_info.called.empty(), "rhs of assign_stmt node " + STR(st_tn_id) +
                                                        " is a call_node but does not actually call a function");
            THROW_ASSERT(node_info.called.size() == 1,
                         "rhs of assign_stmt node " + STR(st_tn_id) + " is a call_node but calls more than a function");
            const auto called_id = *node_info.called.begin();
            const auto BHC = HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper();
            if(BHC->has_implementation() && BHC->function_has_to_be_printed(called_id))
            {
               const auto ce = GetPointerS<const call_node>(rhs);
               const auto& actual_args = ce->args;
               const auto op0 = ce->fn;
               if(op0->get_kind() == addr_node_K && (actual_args.size() == 1 || actual_args.size() == 2))
               {
                  const auto ue = GetPointerS<const unary_node>(op0);
                  const auto fn = ue->op;
                  THROW_ASSERT(fn->get_kind() == function_val_node_K,
                               "IR node not currently supported " + fn->get_kind_text());
                  const auto fd = GetPointerS<const function_val_node>(fn);
                  if(fd)
                  {
                     static const std::map<std::string, std::pair<unsigned int, std::string>>
                         basic_unary_operations_relation = {
                             {"__int8_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, INT_TYPE), "(float)(int)"}},
                             {"__int16_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, INT_TYPE), "(float)(int)"}},
                             {"__int32_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, INT_TYPE), "(float)(int)"}},
                             {"__int32_to_floate11m52b_1023nih", {F_SIGN(DOUBLE_TYPE, INT_TYPE), "(double)(int)"}},
                             {"__uint8_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, UINT_TYPE), "(float)"}},
                             {"__uint16_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, UINT_TYPE), "(float)"}},
                             {"__uint32_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, UINT_TYPE), "(float)"}},
                             {"__uint32_to_floate11m52b_1023nih", {F_SIGN(DOUBLE_TYPE, UINT_TYPE), "(double)"}},
                             {"__int64_to_floate8m23b_127nih",
                              {F_SIGN(FLOAT_TYPE, INT_TYPE), "(float)(long long int)"}},
                             {"__int64_to_floate11m52b_1023nih",
                              {F_SIGN(DOUBLE_TYPE, INT_TYPE), "(double)(long long int)"}},
                             {"__uint64_to_floate8m23b_127nih", {F_SIGN(FLOAT_TYPE, UINT_TYPE), "(float)"}},
                             {"__uint64_to_floate11m52b_1023nih", {F_SIGN(DOUBLE_TYPE, UINT_TYPE), "(double)"}},
                             {"__float64_to_float32_ieee", {F_SIGN(FLOAT_TYPE, DOUBLE_TYPE), "(float)"}},
                             {"__float32_to_float64_ieee", {F_SIGN(DOUBLE_TYPE, FLOAT_TYPE), "(double)"}},
                             {"__float_to_int32e8m23b_127nih", {F_SIGN(INT_TYPE, FLOAT_TYPE), "(int)"}},
                             {"__float_to_int64e8m23b_127nih", {F_SIGN(INT_TYPE, FLOAT_TYPE), "(long long int)"}},
                             {"__float_to_uint32e8m23b_127nih", {F_SIGN(UINT_TYPE, FLOAT_TYPE), "(unsigned int)"}},
                             {"__float_to_uint64e8m23b_127nih",
                              {F_SIGN(UINT_TYPE, FLOAT_TYPE), "(unsigned long long int)"}},
                             {"__float_to_int32e11m52b_1023nih", {F_SIGN(INT_TYPE, DOUBLE_TYPE), "(int)"}},
                             {"__float_to_int64e11m52b_1023nih", {F_SIGN(INT_TYPE, DOUBLE_TYPE), "(long long int)"}},
                             {"__float_to_uint32e11m52b_1023nih", {F_SIGN(UINT_TYPE, DOUBLE_TYPE), "(unsigned int)"}},
                             {"__float_to_uint64e11m52b_1023nih",
                              {F_SIGN(UINT_TYPE, DOUBLE_TYPE), "(unsigned long long int)"}},
                         };
                     static const std::map<std::string, std::pair<bool, std::string>> basic_binary_operations_relation =
                         {
                             {"__float_adde8m23b_127nih", {false, "+"}},
                             {"__float_sube8m23b_127nih", {false, "-"}},
                             {"__float_mule8m23b_127nih", {false, "*"}},
                             {"__float_divSRT4e8m23b_127nih", {false, "/"}},
                             {"__float_divGe8m23b_127nih", {false, "/"}},
                             {"__float_lee8m23b_127nih", {false, "<="}},
                             {"__float_lte8m23b_127nih", {false, "<"}},
                             {"__float_gee8m23b_127nih", {false, ">="}},
                             {"__float_gte8m23b_127nih", {false, ">"}},
                             {"__float_eqe8m23b_127nih", {false, "=="}},
                             {"__float_ltgt_quiete8m23b_127nih", {false, "!="}},
                             {"__float_adde11m52b_1023nih", {true, "+"}},
                             {"__float_sube11m52b_1023nih", {true, "-"}},
                             {"__float_mule11m52b_1023nih", {true, "*"}},
                             {"__float_divSRT4e11m52b_1023nih", {true, "/"}},
                             {"__float_divGe11m52b_1023nih", {true, "/"}},
                             {"__float_lee11m52b_1023nih", {true, "<="}},
                             {"__float_lte11m52b_1023nih", {true, "<"}},
                             {"__float_gee11m52b_1023nih", {true, ">="}},
                             {"__float_gte11m52b_1023nih", {true, ">"}},
                             {"__float_eqe11m52b_1023nih", {true, "=="}},
                             {"__float_ltgt_quiete11m52b_1023nih", {true, "!="}},
                         };
                     const auto unary_op_relation = basic_unary_operations_relation.find(oper->get_name());
                     const auto binary_op_relation = basic_binary_operations_relation.find(oper->get_name());
                     // Also check actual args count since they could have been optimized out
                     if(unary_op_relation != basic_unary_operations_relation.end() && actual_args.size() >= 1)
                     {
                        const auto& f_sign = unary_op_relation->second.first;
                        const auto& cast_op = unary_op_relation->second.second;
                        auto var1 = BHC->PrintVariable(actual_args.at(0)->index);
                        auto res_name = var_name;
                        if(F_TYPE_IN(f_sign) & FLOAT_TYPE)
                        {
                           const std::string bitcast_helper =
                               F_TYPE_IN(f_sign) == FLOAT_TYPE ? "_Int32_ViewConvert" : "_Int64_ViewConvert";
                           var1 = bitcast_helper + "(" + var1 + ")";
                        }
                        if(F_TYPE_OUT(f_sign) & FLOAT_TYPE)
                        {
                           const std::string bitcast_helper =
                               F_TYPE_OUT(f_sign) == FLOAT_TYPE ? "_Int32_ViewConvert" : "_Int64_ViewConvert";
                           res_name = bitcast_helper + "(" + res_name + ")";
                        }
                        const auto computation = "(" + cast_op + "(" + var1 + "))";
                        const auto check_string0 = "\"" + res_name + "==" + computation + "\"";
                        if(F_TYPE_OUT(f_sign) & FLOAT_TYPE)
                        {
                           const auto check_string1 =
                               (F_TYPE_OUT(f_sign) == FLOAT_TYPE ? "_FPs32Mismatch_(" : "_FPs64Mismatch_(") +
                               computation + ", " + res_name + "," + STR(Param->getOption<double>(OPT_max_ulp)) + ")";
                           indented_output_stream->Append(
                               (F_TYPE_OUT(f_sign) == FLOAT_TYPE ? "_CheckBuiltinFPs32_(" : "_CheckBuiltinFPs64_(") +
                               check_string0 + ", " + check_string1 + "," + computation + "," + var_name + "," + var1 +
                               ",0);\n");
                        }
                        else
                        {
                           const auto int_format = F_TYPE_OUT(f_sign) == INT_TYPE ? "%lld" : "%llu";
                           const auto int_cast =
                               F_TYPE_OUT(f_sign) == INT_TYPE ? "(long long int)" : "(long long unsigned int)";
                           const auto fp_format = F_TYPE_IN(f_sign) == FLOAT_TYPE ? "%.20e" : "%.35e";
                           indented_output_stream->Append(
                               "if(" + res_name + "!=" + computation +
                               ") { printf(\"\\n\\n***********************************************************\\nERROR "
                               "ON A BASIC FLOATING POINT OPERATION : %s : expected=" +
                               int_format + " res=" + int_format + "a=%a (" + fp_format +
                               ")\\n***********************************************************\\n\\n\", " +
                               check_string0 + ", " + int_cast + computation + ", " + int_cast + res_name + ", " +
                               var1 + ", " + var1 + ");\nexit(1);\n}\n");
                        }
                     }
                     else if(binary_op_relation != basic_binary_operations_relation.end() && actual_args.size() >= 2)
                     {
                        const auto& is_double_type = binary_op_relation->second.first;
                        const auto& op_symbol = binary_op_relation->second.second;
                        const std::string bitcast_helper = is_double_type ? "_Int64_ViewConvert" : "_Int32_ViewConvert";
                        const auto var1 = bitcast_helper + "(" + BHC->PrintVariable(actual_args.at(0)->index) + ")";
                        const auto var2 = bitcast_helper + "(" + BHC->PrintVariable(actual_args.at(1)->index) + ")";
                        const auto res_name = bitcast_helper + "(" + var_name + ")";
                        const auto computation = "(" + var1 + op_symbol + var2 + ")";
                        const auto check_string0 = "\"" + bitcast_helper + "(" + var_name + ")==" + computation + "\"";
                        const auto check_string1 = (is_double_type ? "_FPs64Mismatch_" : "_FPs32Mismatch_") +
                                                   std::string("(") + computation + ", " + res_name + "," +
                                                   STR(Param->getOption<double>(OPT_max_ulp)) + ")";
                        indented_output_stream->Append(
                            (is_double_type ? "_CheckBuiltinFPs64_(" : "_CheckBuiltinFPs32_(") + check_string0 + ", " +
                            check_string1 + "," + computation + "," + res_name + "," + var1 + "," + var2 + ");\n");
                     }
                  }
               }
            }
         }
      }
   }
}

void DiscrepancyAnalysisCWriter::InternalWriteGlobalDeclarations()
{
   CWriter::InternalWriteGlobalDeclarations();
   // testbench helpers
   indented_output_stream->Append(R"(
void _Dec2Bin_(FILE* __bambu_testbench_fp, long long int num, unsigned int precision)
{
unsigned int i;
unsigned long long int ull_value = (unsigned long long int)num;
for(i = 0; i < precision; ++i)
   fprintf(__bambu_testbench_fp, "%c", (((1LLU << (precision - i - 1)) & ull_value) ? '1' : '0'));
}

void _Ptd2Bin_(FILE* __bambu_testbench_fp, unsigned char* num, unsigned int precision)
{
unsigned int i, j;
char value;
if(precision % 8)
{
value = *(num + precision / 8);
for(j = 8 - precision % 8; j < 8; ++j)
   fprintf(__bambu_testbench_fp, "%c", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));
}
for(i = 0; i < 8 * (precision / 8); i = i + 8)
{
value = *(num + (precision / 8) - (i / 8) - 1);
for(j = 0; j < 8; ++j)
   fprintf(__bambu_testbench_fp, "%c", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));
}
}

float _Int32_ViewConvert(unsigned int i)
{
union
{
unsigned int bits;
float fp32;
} vc;
vc.bits = i;
return vc.fp32;
}

double _Int64_ViewConvert(unsigned long long i)
{
union
{
unsigned long long bits;
double fp64;
} vc;
vc.bits = i;
return vc.fp64;
}

unsigned char _FPs32Mismatch_(float c, float e, float max_ulp)
{
unsigned int binary_c = *((unsigned int*)&c);
unsigned int binary_e = *((unsigned int*)&e);
unsigned int binary_abs_c = binary_c & (~(1U << 31));
unsigned int binary_abs_e = binary_e & (~(1U << 31));
unsigned int denom_0 = 0x34000000;
unsigned int denom_e = ((binary_abs_e >> 23) - 23) << 23;
float cme = c - e;
unsigned int binary_cme = *((unsigned int*)&cme);
unsigned int binary_abs_cme = binary_cme & (~(1U << 31));
float abs_cme = *((float*)&binary_abs_cme);
float ulp = 0.0;
if(binary_abs_c > 0X7F800000 && binary_abs_c > 0X7F800000)
   return 0;
else if(binary_abs_c == 0X7F800000 && binary_abs_e == 0X7F800000)
{
if((binary_c >> 31) != (binary_e >> 31))
   return 1;
else
   return 0;
}
else if(binary_abs_c == 0X7F800000 || binary_abs_e == 0X7F800000 || binary_abs_c > 0X7F800000 || binary_abs_e == 0X7F800000)
   return 0;
else
{
if(binary_abs_e == 0)
   ulp = abs_cme / (*((float*)&denom_0));
else
   ulp = abs_cme / (*((float*)&denom_e));
return ulp > max_ulp;
}
}

unsigned char _FPs64Mismatch_(double c, double e, double max_ulp)
{
unsigned long long int binary_c = *((unsigned long long int*)&c);
unsigned long long int binary_e = *((unsigned long long int*)&e);
unsigned long long int binary_abs_c = binary_c & (~(1ULL << 63));
unsigned long long int binary_abs_e = binary_e & (~(1ULL << 63));
unsigned long long int denom_0 = 0x3CB0000000000000;
unsigned long long int denom_e = ((binary_abs_e >> 52) - 52) << 52;
double cme = c - e;
unsigned long long int binary_cme = *((unsigned int*)&cme);
unsigned long long int binary_abs_cme = binary_cme & (~(1U << 31));
double abs_cme = *((double*)&binary_abs_cme);
double ulp = 0.0;
if(binary_abs_c > 0X7FF0000000000000 && binary_abs_c > 0X7FF0000000000000)
   return 0;
else if(binary_abs_c == 0X7FF0000000000000 && binary_abs_e == 0X7FF0000000000000)
{
if((binary_c >> 63) != (binary_e >> 63))
   return 1;
else
   return 0;
}
else if(binary_abs_c == 0X7FF0000000000000 || binary_abs_e == 0X7FF0000000000000 || binary_abs_c > 0X7FF0000000000000 || binary_abs_e == 0X7FF0000000000000)
   return 0;
else
{
if(binary_abs_e == 0)
   ulp = abs_cme / (*((double*)&denom_0));
else
   ulp = abs_cme / (*((double*)&denom_e));
return ulp > max_ulp;
}
}

void _CheckBuiltinFPs32_(char* chk_str, unsigned char neq, float par_expected, float par_res, float par_a, float par_b)
{
if(neq)
{
printf("\n\n***********************************************************\nERROR ON A BASIC FLOATING POINT "
       "OPERATION : %s : expected=%a (%.20e) res=%a (%.20e) a=%a (%.20e) b=%a "
       "(%.20e)\n***********************************************************\n\n",
       chk_str, par_expected, par_expected, par_res, par_res, par_a, par_a, par_b, par_b);
exit(1);
}
}

void _CheckBuiltinFPs64_(char* chk_str, unsigned char neq, double par_expected, double par_res, double par_a,
                  double par_b)
{
if(neq)
{
printf("\n\n***********************************************************\nERROR ON A BASIC FLOATING POINT "
       "OPERATION : %s : expected=%a (%.35e) res=%a (%.35e) a=%a (%.35e) b=%a "
       "(%.35e)\n***********************************************************\n\n",
       chk_str, par_expected, par_expected, par_res, par_res, par_a, par_a, par_b, par_b);
exit(1);
}
}
)");

   indented_output_stream->Append("unsigned int __standard_exit;\n");
   indented_output_stream->Append("FILE* __bambu_discrepancy_fp;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_context = 0;\n");
   indented_output_stream->Append("unsigned int __bambu_discrepancy_last_call_id = 0;\n");
   indented_output_stream->Append("unsigned char __bambu_discrepancy_first_event = 1;\n");

   indented_output_stream->Append(R"(
long long unsigned int __bambu_discrepancy_tot_assigned_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_lost_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_check_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_lost_addr_ssa = 0;
long long unsigned int __bambu_discrepancy_temp_lost_addr_ssa = 0;
long long unsigned int __bambu_discrepancy_opt_lost_addr_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_lost_int_ssa = 0;
long long unsigned int __bambu_discrepancy_temp_lost_int_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_check_addr_ssa = 0;
long long unsigned int __bambu_discrepancy_temp_check_addr_ssa = 0;
long long unsigned int __bambu_discrepancy_tot_check_int_ssa = 0;
long long unsigned int __bambu_discrepancy_temp_check_int_ssa = 0;

static void __bambu_discrepancy_emit_event_sep(void)
{
if(__bambu_discrepancy_first_event)
{
__bambu_discrepancy_first_event = 0;
}
else
{
fputc(',', __bambu_discrepancy_fp);
}
}
)");

   /*
    * extra exit function to print out the statistics even in case
    * __builtin_exit is called, which normally just terminates the program
    */
   indented_output_stream->Append("void __bambu_discrepancy_exit(void) __attribute__ ((destructor(101)));\n");
   indented_output_stream->Append("void __bambu_discrepancy_exit(void)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if(__standard_exit) {\n");
   append_context_end_event(indented_output_stream);
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("fputs(\"]\\x7d\\n\", __bambu_discrepancy_fp);\n");
   indented_output_stream->Append("fflush(__bambu_discrepancy_fp);\n");
   indented_output_stream->Append("fclose(__bambu_discrepancy_fp);\n");

   /*
    * if we're using hw discrepancy don't print anything related to ssa
    * variables or to results of operations, because only control flow is
    * checked
    */

   indented_output_stream->Append("fputs(\"DISCREPANCY REPORT\\n\", stdout);\n");
   if(Param->isOption(OPT_cat_args))
   {
      indented_output_stream->Append("fputs(\"Bambu executed with: " + Param->getOption<std::string>(OPT_cat_args) +
                                     "\\n\", stdout);\n");
   }
   indented_output_stream->Append("fprintf(stdout, "
                                  "\"Assigned ssa = %llu\\nChecked ssa = %llu\\nLost ssa = %llu\\n\", "
                                  "__bambu_discrepancy_tot_assigned_ssa, __bambu_discrepancy_tot_check_ssa, "
                                  "__bambu_discrepancy_tot_lost_ssa);\n");
   indented_output_stream->Append("fprintf(stdout, "
                                  "\"Normal ssa  = %llu\\nAddress ssa  = %llu\\n\", "
                                  "__bambu_discrepancy_tot_lost_int_ssa + __bambu_discrepancy_tot_check_int_ssa, "
                                  "__bambu_discrepancy_tot_lost_addr_ssa + __bambu_discrepancy_tot_check_addr_ssa);\n");
   indented_output_stream->Append(
       "fprintf(stdout, "
       "\"CHECKED: %llu\\nNormal ssa = %llu\\nNormal tmp ssa = %llu\\nAddr ssa = %llu\\nAddr tmp ssa = %llu\\n\", "
       "__bambu_discrepancy_tot_check_ssa, __bambu_discrepancy_tot_check_int_ssa, "
       "__bambu_discrepancy_temp_check_int_ssa, "
       "__bambu_discrepancy_tot_check_addr_ssa, __bambu_discrepancy_temp_check_addr_ssa);\n");
   indented_output_stream->Append("fprintf(stdout, "
                                  "\"LOST: %llu\\nNormal ssa lost = %llu\\nNormal tmp ssa lost = %llu\\nAddr ssa "
                                  "lost = %llu\\nAddr tmp ssa lost = %llu\\nOpt tmp ssa lost = %llu\\n\", "
                                  "__bambu_discrepancy_tot_lost_ssa, __bambu_discrepancy_tot_lost_int_ssa, "
                                  "__bambu_discrepancy_temp_lost_int_ssa, "
                                  "__bambu_discrepancy_tot_lost_addr_ssa, __bambu_discrepancy_temp_lost_addr_ssa, "
                                  "__bambu_discrepancy_opt_lost_addr_ssa);\n");

   indented_output_stream->Append("}\n\n");
   return;
}

void DiscrepancyAnalysisCWriter::WriteExtraInitCode()
{
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto fun_behavior = HLSMgr->CGetFunctionBehavior(top_fnode->index);
   const auto behavioral_helper = fun_behavior->CGetBehavioralHelper();
   Discrepancy->c_trace_filename = std::filesystem::path(c_backend_info->out_filename).parent_path().string() +
                                   behavioral_helper->GetFunctionName() + "_discrepancy.data";

   indented_output_stream->Append("__standard_exit = 0;\n");
   indented_output_stream->Append("__bambu_discrepancy_fp = fopen(\"" + Discrepancy->c_trace_filename +
                                  "\", \"w\");\n");
   indented_output_stream->Append("if(!__bambu_discrepancy_fp) {\n");
   indented_output_stream->Append("perror(\"can't open file: " + Discrepancy->c_trace_filename + "\");\n");
   indented_output_stream->Append("exit(1);\n");
   indented_output_stream->Append("}\n\n");

   indented_output_stream->Append("fputs(\"\\x7b\\\"events\\\":[\\n\", __bambu_discrepancy_fp);\n");
   indented_output_stream->Append("__bambu_discrepancy_emit_event_sep();\n");
   indented_output_stream->Append(
       "fprintf(__bambu_discrepancy_fp, "
       "\"\\x7b\\\"type\\\":\\\"initial_context\\\",\\\"context\\\":%llu,\\\"addr_table\\\":[\", "
       "__bambu_discrepancy_context);\n");
   indented_output_stream->Append("unsigned char __bambu_discrepancy_first_addr = 1;\n");
   for(const auto& var_node : HLSMgr->GetGlobalVariables())
   {
      if(HLSMgr->Rmem->has_base_address(var_node->index) && !ir_helper::IsSystemType(var_node))
      {
#if HAVE_ASSERTS
         const auto bitsize = ir_helper::SizeAlloc(var_node);
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1,
                      "bitsize of a variable in memory must be multiple of 8 --> is " + STR(bitsize));
#endif
         append_addr_table_entry(indented_output_stream, var_node->index,
                                 STR(behavioral_helper->PrintVariable(var_node->index)));
      }
   }
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"]\\x7d\");\n");
   indented_output_stream->Append("fputc('\\n', __bambu_discrepancy_fp);\n");
}

void DiscrepancyAnalysisCWriter::WriteExtraCodeBeforeEveryMainCall()
{
   indented_output_stream->Append("__bambu_discrepancy_last_call_id = 0;\n");
}

void DiscrepancyAnalysisCWriter::DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared,
                                                       CustomSet<unsigned int>& already_declared_variables,
                                                       CustomSet<std::string>& locally_declared_types,
                                                       const BehavioralHelperConstRef BH,
                                                       const std::unique_ptr<var_pp_functor>& varFunc)
{
   HLSCWriter::DeclareLocalVariables(to_be_declared, already_declared_variables, locally_declared_types, BH, varFunc);
   indented_output_stream->Append("__bambu_discrepancy_context++;\n");
   indented_output_stream->Append("__bambu_discrepancy_emit_event_sep();\n");
   indented_output_stream->Append(
       "fprintf(__bambu_discrepancy_fp, \"\\x7b\\\"type\\\":\\\"context\\\",\\\"call_id\\\":%u,\\\"context\\\":%llu,"
       "\\\"called_id\\\":" +
       STR(BH->get_function_index()) +
       ",\\\"addr_table\\\":[\", __bambu_discrepancy_last_call_id, __bambu_discrepancy_context);\n");
   indented_output_stream->Append("unsigned char __bambu_discrepancy_first_addr = 1;\n");

   const auto FB = HLSMgr->CGetFunctionBehavior(BH->get_function_index());
   for(const auto& par : BH->GetParameters())
   {
      if(FB->is_variable_mem(par->index))
      {
#if HAVE_ASSERTS
         const auto bitsize = ir_helper::SizeAlloc(par);
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1,
                      "bitsize of a variable in memory must be multiple of 8 --> is " + STR(bitsize));
#endif
         append_addr_table_entry(indented_output_stream, par->index, STR(BH->PrintVariable(par->index)));
      }
   }
   for(const auto& var : to_be_declared)
   {
      if(FB->is_variable_mem(var))
      {
#if HAVE_ASSERTS
         const auto bitsize = ir_helper::SizeAlloc(TM->GetIRNode(var));
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1,
                      "bitsize of a variable in memory must be multiple of 8 --> is " + STR(bitsize));
#endif
         append_addr_table_entry(indented_output_stream, var, STR(BH->PrintVariable(var)));
      }
   }
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"]\\x7d\");\n");
   indented_output_stream->Append("fputc('\\n', __bambu_discrepancy_fp);\n");
   indented_output_stream->Append("__bambu_discrepancy_last_call_id = 0;\n");
}

void DiscrepancyAnalysisCWriter::WriteFunctionImplementation(unsigned int function_index)
{
   const auto fnode = TM->GetIRNode(function_index);
   THROW_ASSERT(GetPointer<function_val_node>(fnode), "expected a function decl");
   bool prepend_static = !ir_helper::IsStaticDeclaration(fnode) && !ir_helper::IsExternDeclaration(fnode) &&
                         (ir_helper::GetFunctionName(fnode) != "main");
   if(prepend_static)
   {
      GetPointerS<function_val_node>(fnode)->static_flag = true;
   }
   CWriter::WriteFunctionImplementation(function_index);
   if(prepend_static)
   {
      GetPointerS<function_val_node>(fnode)->static_flag = false;
   }
}

void DiscrepancyAnalysisCWriter::WriteBBHeader(const unsigned int bb_number, const unsigned int)
{
   append_control_flow_event(indented_output_stream, bb_number);
}

void DiscrepancyAnalysisCWriter::WriteFunctionDeclaration(const unsigned int funId)
{
   const auto fnode = TM->GetIRNode(funId);
   THROW_ASSERT(GetPointer<function_val_node>(fnode), "expected a function decl");
   const auto prepend_static = !ir_helper::IsStaticDeclaration(fnode) && !ir_helper::IsExternDeclaration(fnode) &&
                               (ir_helper::GetFunctionName(fnode) != "main");
   if(prepend_static)
   {
      GetPointerS<function_val_node>(fnode)->static_flag = true;
   }
   HLSCWriter::WriteFunctionDeclaration(funId);
   if(prepend_static)
   {
      GetPointerS<function_val_node>(fnode)->static_flag = false;
   }
}

void DiscrepancyAnalysisCWriter::WriteBuiltinWaitCall()
{
   CWriter::WriteBuiltinWaitCall();
}
