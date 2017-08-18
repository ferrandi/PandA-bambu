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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Castellana <michele.castellana@mail.polimi.it>
 */

#include "discrepancy_analysis_c_writer.hpp"

// behavior/ includes
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "call_graph.hpp"

// HLS/ includes
#include "hls_manager.hpp"
#include "hls.hpp"

// includes from HLS/binding/module/
#include "fu_binding.hpp"

// include from HLS/memory
#include "memory.hpp"

// includes from HLS/module_allocation/
#include "allocation_information.hpp"

// HLS/stg/ includes
#include "state_transition_graph_manager.hpp"
#include "state_transition_graph.hpp"

// includes from HLS/vcd
#include "Discrepancy.hpp"

// design_flow/backend/ToC/ includes
#include "hls_c_backend_information.hpp"

// include from design_flow/ToHDL/
#include "language_writer.hpp"

// include from technology/physical_library/
#include "technology_node.hpp"

// include from technology/physical_library/models/
#include "time_model.hpp"

// tree/ include
#include "tree_helper.hpp"
#include "tree_reindex.hpp"
#include "tree_manager.hpp"

// utility/ includes
#include "indented_output_stream.hpp"
#include "behavioral_helper.hpp"
#include "utility.hpp"
#include "math_function.hpp"

#include "Parameter.hpp"

/*
 * Newer versions of gcc support integer variables larger than 64 bits.
 * These are not supported by bambu which treats them as vectors. But the
 * tree_helper::is_vector function returns false for those large integers.
 * This function is used to detect when an integer variable (non-vector)
 * is actually handled as a vector from bambu
 */
static bool is_large_integer(const unsigned int t_id, const tree_managerConstRef T_M)
{
   const tree_nodeRef node_type = T_M->get_tree_node_const(t_id);
   type_node * tn = GetPointer<type_node>(node_type);
   THROW_ASSERT(tn, "type_id " + STR(t_id) + " is not a type");
   if (node_type->get_kind() != integer_type_K)
      return false;
   integer_type * it = GetPointer<integer_type>(node_type);
   THROW_ASSERT(it, "type " + STR(t_id) + " is not an integer type");
   if ((it->prec != tn->algn and it->prec > 64) or (tn->algn == 128))
      return true;

   return false;
}

DiscrepancyAnalysisCWriter::DiscrepancyAnalysisCWriter
(
   const HLSCBackendInformationConstRef _hls_c_backend_information,
   const application_managerConstRef _AppM,
   const InstructionWriterRef _instruction_writer,
   const IndentedOutputStreamRef _indented_output_stream,
   const ParameterConstRef _parameters,
   bool _verbose
) :
   HLSCWriter
   (
      _hls_c_backend_information,
      _AppM,
      _instruction_writer,
      _indented_output_stream,
      _parameters,
      _verbose
   ),
   Discrepancy(_hls_c_backend_information->HLSMgr->RDiscr)
{
   THROW_ASSERT(Param->isOption(OPT_discrepancy) and
         Param->getOption<bool>(OPT_discrepancy),
         "Step "  + STR(__PRETTY_FUNCTION__) + " should not be added without discrepancy");
   THROW_ASSERT(Discrepancy, "Discrepancy data structure is not correctly initialized");
}

DiscrepancyAnalysisCWriter::~DiscrepancyAnalysisCWriter()
{}

void DiscrepancyAnalysisCWriter::writePreInstructionInfo
(const FunctionBehaviorConstRef FB, const vertex statement)
{
   const OpGraphConstRef instrGraph = FB->CGetOpGraph(FunctionBehavior::FCFG);
   const OpNodeInfoConstRef node_info = instrGraph->CGetOpNodeInfo(statement);
   const unsigned int st_tn_id = node_info->GetNodeId();
   if (st_tn_id == 0 || st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
      return;
   const tree_nodeConstRef curr_tn = TM->CGetTreeNode(st_tn_id);
   const auto kind = curr_tn->get_kind();
   if (kind == gimple_return_K)
   {
      indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CONTEXT_END\\n\");\n");
   }
   else if (kind == gimple_call_K)
   {
      THROW_ASSERT(not node_info->called.empty(),
            "tree node " + STR(st_tn_id) + " is a gimple_call but does not actually call a function");
      THROW_ASSERT(node_info->called.size() == 1,
            "tree node " + STR(st_tn_id) + " calls more than a function");
      const unsigned int called_id = *node_info->called.begin();
      const tree_nodeConstRef called_fun_decl_node = TM->CGetTreeNode(called_id);
      const function_decl * fu_dec = GetPointer<const function_decl>(called_fun_decl_node);
      if (GetPointer<const identifier_node>(GET_NODE(fu_dec->name))->strg == BUILTIN_WAIT_CALL)
      {
         /*
          * This operation calls a function with a function pointer, which is
          * implemented in bambu with the builtin function * __builtin_wait_call()
          * When a function is called with a function pointer we always need to
          * have its C source code, so it always has to be printed back in C.
          */
         indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CALL_ID " + STR(st_tn_id) + "\\n\");\n");
         return;
      }
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper();
      if (BH->has_implementation() and BH->function_has_to_be_printed(called_id))
         indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CALL_ID " + STR(st_tn_id) + "\\n\");\n");
   }
   else if (kind == gimple_assign_K)
   {
      const gimple_assign * g_as_node = GetPointer<const gimple_assign>(curr_tn);
      tree_nodeRef rhs = GET_NODE(g_as_node->op1);
      if (rhs->get_kind() == call_expr_K || rhs->get_kind() == aggr_init_expr_K)
      {
         THROW_ASSERT(not node_info->called.empty(),
               "rhs of gimple_assign node " + STR(st_tn_id) + " is a call_expr but does not actually call a function");
         THROW_ASSERT(node_info->called.size() == 1,
               "rhs of gimple_assign node " + STR(st_tn_id) + " is a call_expr but calls more than a function");
         const unsigned int called_id = *node_info->called.begin();
         const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper();
         if (BH->has_implementation() and BH->function_has_to_be_printed(called_id))
            indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CALL_ID " + STR(st_tn_id) + "\\n\");\n");
      }
   }
   return;
}

void DiscrepancyAnalysisCWriter::writePostInstructionInfo
(const FunctionBehaviorConstRef fun_behavior, const vertex statement)
{
   const OpGraphConstRef instrGraph = fun_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   const unsigned int st_tn_id = instrGraph->CGetOpNodeInfo(statement)->GetNodeId();
   if (st_tn_id == 0 || st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
      return;
   const tree_nodeConstRef curr_tn = TM->CGetTreeNode(st_tn_id);
   if (curr_tn->get_kind() != gimple_assign_K and curr_tn->get_kind() != gimple_phi_K)
      return;

   const BehavioralHelperConstRef BH = fun_behavior->CGetBehavioralHelper();
   if (Param->isOption(OPT_discrepancy_only))
   {
      const auto discrepancy_functions =
         Param->getOption<const CustomSet<std::string> >(OPT_discrepancy_only);
      std::string fu_name = BH->get_function_name();
      if (not discrepancy_functions.empty() and
            discrepancy_functions.find(fu_name) == discrepancy_functions.end())
         return;
   }
   const unsigned int funId = BH->get_function_index();
   const hlsConstRef hls = hls_c_backend_information->HLSMgr->get_HLS(funId);

   technology_nodeRef fu_tech_n = hls->allocation_information->get_fu(hls->Rfu->get_assign(statement));
   technology_nodeRef op_tech_n = GetPointer<functional_unit>(fu_tech_n)->get_operation
      (tree_helper::normalized_ID(instrGraph->CGetOpNodeInfo(statement)->GetOperation()));

   const operation * oper = GetPointer<operation>(op_tech_n);
   if (!oper)
      return;

   const gimple_assign * g_as_node = GetPointer<const gimple_assign>(curr_tn);
   const gimple_phi * g_phi_node = GetPointer<const gimple_phi>(curr_tn);
   bool is_virtual = false;
   unsigned int ssa_node_index;
   tree_nodeRef ssa;
   if (g_as_node)
   {
      ssa = GET_NODE(g_as_node->op0);
      ssa_node_index = GET_INDEX_NODE(g_as_node->op0);
   }
   else if (g_phi_node)
   {
      ssa = GET_NODE(g_phi_node->res);
      ssa_node_index = GET_INDEX_NODE(g_phi_node->res);
      is_virtual = g_phi_node->virtual_flag;
   }

   if (ssa and ssa->get_kind() == ssa_name_K and (not is_virtual))
   {
      /*
       * print statements that increase the counters used for coverage statistics
       */
      indented_output_stream->Append("__bambu_discrepancy_tot_assigned_ssa++;\n");
      const ssa_name * ssaname = GetPointer<const ssa_name>(ssa);
      bool is_temporary = ssaname->var ? GetPointer<const decl_node>(GET_NODE(ssaname->var))->artificial_flag : true;
      bool is_discrepancy_address = Discrepancy->address_ssa.find(ssa) != Discrepancy->address_ssa.end();
      bool is_lost = Discrepancy->ssa_to_skip.find(ssa) != Discrepancy->ssa_to_skip.end();
      bool has_no_meaning_in_software = is_discrepancy_address and
         Discrepancy->ssa_to_skip_if_address.find(ssa) != Discrepancy->ssa_to_skip_if_address.end();
      if (is_lost or has_no_meaning_in_software)
      {
         indented_output_stream->Append("__bambu_discrepancy_tot_lost_ssa++;\n");
         if (is_discrepancy_address)
         {
            indented_output_stream->Append("__bambu_discrepancy_tot_lost_addr_ssa++;\n");
            if (is_temporary)
            {
               indented_output_stream->Append("__bambu_discrepancy_temp_lost_addr_ssa++;\n");
            }
            if (has_no_meaning_in_software)
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
            THROW_ASSERT(not has_no_meaning_in_software,
                  "Tree Node " + STR(ssa_node_index) + " is has no meaning in software but is not an address");
            indented_output_stream->Append("__bambu_discrepancy_tot_lost_int_ssa++;\n");
            if (is_temporary)
            {
               indented_output_stream->Append("__bambu_discrepancy_temp_lost_int_ssa++;\n");
            }
         }
         return;
      }
      else
      {
         indented_output_stream->Append("__bambu_discrepancy_tot_check_ssa++;\n");
         if (is_discrepancy_address)
         {
            indented_output_stream->Append("__bambu_discrepancy_tot_check_addr_ssa++;\n");
            if (is_temporary)
            {
               indented_output_stream->Append("__bambu_discrepancy_temp_check_addr_ssa++;\n");
            }
         }
         else
         {
            indented_output_stream->Append("__bambu_discrepancy_tot_check_int_ssa++;\n");
            if (is_temporary)
            {
               indented_output_stream->Append("__bambu_discrepancy_temp_check_int_ssa++;\n");
            }
         }
      }
      Discrepancy->n_checked_operations++;
      /*
       * print statements to print informations on the instruction
       */
      indented_output_stream->Append
         ("fprintf(__bambu_discrepancy_fp, \"INSTR stg_id " + STR(funId) +
          " op_id " + STR(instrGraph->CGetOpNodeInfo(statement)->GetNodeId()));

      if (oper->is_bounded())
      {
         indented_output_stream->Append(" B ");
         indented_output_stream->Append(STR(oper->time_m->get_cycles()));
      }
      else
      {
         indented_output_stream->Append(" U");
      }
      indented_output_stream->Append("\\n\");\n");

      /*
       * collect information on the scheduling of the operation to be printed
       */
      const StateTransitionGraphManagerConstRef STGMan = hls->STG;
      const StateTransitionGraphInfoConstRef stg_info =
         STGMan->CGetStg()->CGetStateTransitionGraphInfo();

      std::set<unsigned int> init_state_ids;
      std::set<unsigned int> exec_state_ids;
      std::set<unsigned int> end_state_ids;

      for (const auto & s : STGMan->get_starting_states(statement))
         init_state_ids.insert(stg_info->vertex_to_state_id.at(s));
      for (const auto & s : STGMan->get_execution_states(statement))
         exec_state_ids.insert(stg_info->vertex_to_state_id.at(s));
      for (const auto & s : STGMan->get_ending_states(statement))
         end_state_ids.insert(stg_info->vertex_to_state_id.at(s));

      THROW_ASSERT(not init_state_ids.empty(), "operation not properly scheduled: "
         "number of init states = " + STR(init_state_ids.size()));
      THROW_ASSERT(not exec_state_ids.empty(), "operation not properly scheduled: "
         "number of exec states = " + STR(exec_state_ids.size()));
      THROW_ASSERT(not end_state_ids.empty(), "operation not properly scheduled: "
         "number of ending states = " + STR(end_state_ids.size()));
      /*
       * print statements to print scheduling information on the operation
       */
      indented_output_stream->Append ("fprintf(__bambu_discrepancy_fp, \"SCHED start");
      for (const auto & s : init_state_ids)
         indented_output_stream->Append(" " + STR(s));
      indented_output_stream->Append("; exec");
      for (const auto & s : exec_state_ids)
         indented_output_stream->Append(" " + STR(s));
      indented_output_stream->Append("; end");
      for (const auto & s : end_state_ids)
         indented_output_stream->Append(" " + STR(s));
      indented_output_stream->Append(";\\n\");\n");

      const unsigned int type_id = tree_helper::get_type_index(TM, ssa_node_index);
      const unsigned int type_bitsize = tree_helper::size(TM, type_id);
      const std::string var_name = BH->PrintVariable(ssa_node_index);
      const unsigned int ssa_bitsize = tree_helper::Size(ssa);
      if (ssa_bitsize > type_bitsize)
      {
         THROW_ERROR(std::string("variable size mismatch: ") +
            "ssa node id = " + STR(ssa_node_index) + " has size = " + STR(ssa_bitsize) +
            " type node id = " + STR(type_id) + " has size = " + STR(type_bitsize));
      }
      /* tree_nodeRef for gimple lvalue */
      indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, "
         "\"ASSIGN ssa_id " + STR(ssa_node_index) + "; ssa " + var_name + "; btsz " + STR(ssa_bitsize) + "; val #\");\n");

      const bool is_real = BH->is_real(type_id);
      const bool is_vector = BH->is_a_vector(type_id);
      const bool is_complex = BH->is_a_complex(type_id);

      if (is_real or is_complex or is_vector or
            BH->is_a_struct(type_id) or BH->is_an_union(type_id) or
            is_large_integer(type_id, TM))
      {
         indented_output_stream->Append("_Ptd2Bin_(__bambu_discrepancy_fp, (unsigned char*)&" +
               var_name + ", " + STR(type_bitsize) + ");\n");
      }
      else
      {
         indented_output_stream->Append("_Dec2Bin_(__bambu_discrepancy_fp, " +
               var_name + ", " + STR(type_bitsize) + ");\n");
      }

      indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"; type ");

      THROW_ASSERT(not(is_discrepancy_address and (is_real or is_complex)),
            "type id = " + STR(type_id) +
            " is complex = " + STR(is_complex) +
            " is real = " + STR(is_real));

      unsigned int vec_base_bitsize = type_bitsize;
      if (is_vector)
      {
         THROW_ASSERT(ssa_bitsize == type_bitsize,
               "ssa node id = " + STR(ssa_node_index) +
               " type node id = " + STR(type_id));

         unsigned int elem_bitsize =
            tree_helper::size(TM, tree_helper::GetElements(TM, type_id));

         THROW_ASSERT(type_bitsize % elem_bitsize == 0,
               "ssa node id = " + STR(ssa_node_index) +
               " type node id = " + STR(type_id) +
               " type_bitsize = " + STR(type_bitsize) +
               " elem_bitsize = " + STR(elem_bitsize));

         vec_base_bitsize = elem_bitsize;
         indented_output_stream->Append("V ");
      }
      else
      {
         indented_output_stream->Append("NV ");
      }
      indented_output_stream->Append(STR(vec_base_bitsize));

      if (is_complex)
      {
         THROW_ASSERT(vec_base_bitsize % 2 == 0,
               "complex variables must have size multiple of 2"
               "\nssa node id = " + STR(ssa_node_index) +
               "\ntype node id = " + STR(type_id) +
               "\nvec_base_bitsize = " + STR(vec_base_bitsize));
         indented_output_stream->Append(" C ");
      }
      else
      {
            indented_output_stream->Append(" NC ");
      }

      if (is_real)
            indented_output_stream->Append("R ");
      else
            indented_output_stream->Append("NR ");

      if (is_discrepancy_address)
         indented_output_stream->Append("A");
      else
         indented_output_stream->Append("NA");

      indented_output_stream->Append("\\n\");\n");

   }
   return;
}

void DiscrepancyAnalysisCWriter::WriteGlobalDeclarations()
{
   CWriter::WriteGlobalDeclarations();
   WriteTestbenchGlobalVars();
   WriteTestbenchHelperFunctions();
   indented_output_stream->Append("// Global declarations for printing of discrepancy data\n");
   /* global variable for fprintf to print discrepancy data */
   indented_output_stream->Append("FILE * __bambu_discrepancy_fp;\n\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_context = 0;\n");

   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_assigned_ssa = 0;\n");

   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_lost_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_check_ssa = 0;\n");

   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_lost_addr_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_temp_lost_addr_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_opt_lost_addr_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_lost_int_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_temp_lost_int_ssa = 0;\n");

   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_check_addr_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_temp_check_addr_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_tot_check_int_ssa = 0;\n");
   indented_output_stream->Append("long long unsigned int __bambu_discrepancy_temp_check_int_ssa = 0;\n");
   /*
    * extra exit function to print out the statistics even in case
    * __builtin_exit is called, which normally just terminates the program
    */
   indented_output_stream->Append("void __bambu_discrepancy_exit(void) __attribute__ ((destructor(101)));\n");
   indented_output_stream->Append("void __bambu_discrepancy_exit(void)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (__standard_exit) {\n");
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CONTEXT_END\\n\");\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("fflush(__bambu_discrepancy_fp);\n");
   indented_output_stream->Append("fputs(\"DISCREPANCY REPORT\\n\", stdout);\n");
   if (Param->isOption(OPT_cat_args))
      indented_output_stream->Append("fputs(\"" + Param->getOption<std::string>(OPT_program_name) + " executed with: " + Param->getOption<std::string>(OPT_cat_args) + "\\n\", stdout);\n");
   indented_output_stream->Append("fprintf(stdout, "
      "\"Assigned ssa = %llu\\nChecked ssa = %llu\\nLost ssa = %llu\\n\", "
      "__bambu_discrepancy_tot_assigned_ssa, __bambu_discrepancy_tot_check_ssa, __bambu_discrepancy_tot_lost_ssa);\n");
   indented_output_stream->Append("fprintf(stdout, "
      "\"Normal ssa  = %llu\\nAddress ssa  = %llu\\n\", "
      "__bambu_discrepancy_tot_lost_int_ssa + __bambu_discrepancy_tot_check_int_ssa, "
      "__bambu_discrepancy_tot_lost_addr_ssa + __bambu_discrepancy_tot_check_addr_ssa);\n");
   indented_output_stream->Append("fprintf(stdout, "
      "\"CHECKED: %llu\\nNormal ssa = %llu\\nNormal tmp ssa = %llu\\nAddr ssa = %llu\\nAddr tmp ssa = %llu\\n\", "
      "__bambu_discrepancy_tot_check_ssa, __bambu_discrepancy_tot_check_int_ssa, __bambu_discrepancy_temp_check_int_ssa, "
      "__bambu_discrepancy_tot_check_addr_ssa, __bambu_discrepancy_temp_check_addr_ssa);\n");
   indented_output_stream->Append("fprintf(stdout, "
      "\"LOST: %llu\\nNormal ssa lost = %llu\\nNormal tmp ssa lost = %llu\\nAddr ssa lost = %llu\\nAddr tmp ssa lost = %llu\\nOpt tmp ssa lost = %llu\\n\", "
      "__bambu_discrepancy_tot_lost_ssa, __bambu_discrepancy_tot_lost_int_ssa, __bambu_discrepancy_temp_lost_int_ssa, "
      "__bambu_discrepancy_tot_lost_addr_ssa, __bambu_discrepancy_temp_lost_addr_ssa, __bambu_discrepancy_opt_lost_addr_ssa);\n");
   indented_output_stream->Append("}\n\n");
   return;
}

void DiscrepancyAnalysisCWriter::WriteExtraInitCode()
{
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto top_fun_id = *(top_function_ids.begin());
   const FunctionBehaviorConstRef fun_behavior = AppM->CGetFunctionBehavior(top_fun_id);
   const BehavioralHelperConstRef behavioral_helper = fun_behavior->CGetBehavioralHelper();
   std::string discrepancy_data_filename =
      Param->getOption<std::string>(OPT_output_directory) + "/simulation/" +
      behavioral_helper->get_function_name() + "_discrepancy.data";

   indented_output_stream->Append("__bambu_discrepancy_fp = fopen(\"" + discrepancy_data_filename + "\", \"w\");\n");
   indented_output_stream->Append("if (!__bambu_discrepancy_fp) {\n");
   indented_output_stream->Append("perror(\"can't open file: " + discrepancy_data_filename + "\");\n");
   indented_output_stream->Append("exit(1);\n");
   indented_output_stream->Append("}\n\n");

   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CONTEXT LL_%llu\\n\", __bambu_discrepancy_context);\n");
   const CustomSet<unsigned int> & glbl_vars =  hls_c_backend_information->HLSMgr->get_global_variables();
   for (const auto & var : glbl_vars)
   {
      if (hls_c_backend_information->HLSMgr->Rmem->has_base_address(var) and (not tree_helper::is_system(TM, var)))
      {
         const unsigned int bitsize = tree_helper::size(TM, var);
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1, "bitsize of a varibale in memory must be mutliple of 8 --> is " + STR(bitsize));
         indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"VARDECL_ID " + STR(var) +
               " VAR_ADDR LL_%lu\\n\", &" + STR(behavioral_helper->PrintVariable(var)) +");//size " + STR(compute_n_bytes(bitsize)) + "\n");
      }
   }
   return;
}

void DiscrepancyAnalysisCWriter::WriteExtraCodeBeforeEveryMainCall()
{
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CALL_ID 0\\n\");\n");
}

void DiscrepancyAnalysisCWriter::DeclareLocalVariables
(
   const CustomSet<unsigned int> & to_be_declared,
   CustomSet<unsigned int> & already_declared_variables,
   CustomSet<std::string> & locally_declared_types,
   const BehavioralHelperConstRef BH,
   const var_pp_functorConstRef varFunc
)
{
   HLSCWriter::DeclareLocalVariables(to_be_declared, already_declared_variables, locally_declared_types, BH, varFunc);
   indented_output_stream->Append("__bambu_discrepancy_context++;\n");
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CONTEXT LL_%llu\\n\", __bambu_discrepancy_context);\n");
   indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"CALLED_ID " + STR(BH->get_function_index()) + "\\n\");\n");
   const FunctionBehaviorConstRef FB = hls_c_backend_information->HLSMgr->CGetFunctionBehavior(BH->get_function_index());
   const std::list<unsigned int> & params = BH->get_parameters();
   for (const auto & par : params)
   {
      if (FB->is_variable_mem(par))
      {
         const unsigned int bitsize = tree_helper::size(TM, par);
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1, "bitsize of a varibale in memory must be mutliple of 8 --> is " + STR(bitsize));
         indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"VARDECL_ID " + STR(par) +
               " VAR_ADDR LL_%lu\\n\", &" + STR(BH->PrintVariable(par)) +");//size " + STR(compute_n_bytes(bitsize)) + "\n");
      }
   }
   for (const auto & var : to_be_declared)
   {
      if (FB->is_variable_mem(var))
      {
         const unsigned int bitsize = tree_helper::size(TM, var);
         THROW_ASSERT(bitsize % 8 == 0 || bitsize == 1, "bitsize of a varibale in memory must be mutliple of 8 --> is " + STR(bitsize));
         indented_output_stream->Append("fprintf(__bambu_discrepancy_fp, \"VARDECL_ID " + STR(var) +
               " VAR_ADDR LL_%lu\\n\", &" + STR(BH->PrintVariable(var)) +");//size " + STR(compute_n_bytes(bitsize)) + "\n");
      }
   }
}

void DiscrepancyAnalysisCWriter::WriteFunctionImplementation(unsigned int function_index)
{
   CWriter::WriteFunctionImplementation(function_index);
}

void DiscrepancyAnalysisCWriter::WriteBBHeader(unsigned int bb_number)
{
   indented_output_stream->Append(
         "fprintf(__bambu_discrepancy_fp, \"BB " + STR(bb_number) + "\\n\");\n");
}

void DiscrepancyAnalysisCWriter::WriteFunctionDeclaration(const unsigned int funId)
{
   const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   const std::string & funName = behavioral_helper->get_function_name();
   if (not tree_helper::is_static(TM, funId) and not tree_helper::is_extern(TM, funId) and (funName != "main"))
      indented_output_stream->Append("static ");
   HLSCWriter::WriteFunctionDeclaration(funId);
}

void DiscrepancyAnalysisCWriter::WriteBuiltinWaitCall()
{
   CWriter::WriteBuiltinWaitCall();
}
