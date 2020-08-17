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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 */
#include "vcd_utility.hpp"

#include <boost/filesystem/operations.hpp>
#include <iterator>

// include from ./
#include "Parameter.hpp"

// include from behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include from circuit/
#include "structural_manager.hpp"

// include from design_flows/
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

// includes from design_flows/backend/ToC/
#include "c_backend_step_factory.hpp"
#include "hls_c_backend_information.hpp"

// includes from design_flows/backend/ToHDL
#include "language_writer.hpp"

// include from frontend_analysis/
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

// include from HLS/
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

// include from HLS/binding/register/
#include "reg_binding.hpp"

// include from HLS/memory/
#include "memory.hpp"

// include from HLS/stg/
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

// include from HLS/vcd/
#include "Discrepancy.hpp"
#include "vcd_trace_head.hpp"

// include from parser/discrepancy/
#include "parse_discrepancy.hpp"

// include from tree/
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

// include from utility
#include "cpu_stats.hpp"
#include "cpu_time.hpp"

// external include
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/algorithm/string/case_conv.hpp>
#include <cfloat>
#include <fstream>
#include <utility>

DiscrepancyLog::DiscrepancyLog(const HLS_managerConstRef HLSMgr, const vcd_trace_head& t, const uint64_t c_context, std::string _c_val, const unsigned int _el_idx, const std::string::size_type _first_c_bit, const std::string::size_type _c_size,
                               const unsigned int _b)
    : op_start_time(t.op_start_time),
      op_end_time(t.op_end_time),
      type(t.op_info.type),
      op_id(t.op_info.op_id),
      ssa_id(t.op_info.ssa_name_node_id),
      fun_id(t.op_info.stg_fun_id),
      op_start_state(t.fsm_ss_it->value),
      fu_name(HLSMgr->CGetFunctionBehavior(t.op_info.stg_fun_id)->CGetBehavioralHelper()->get_function_name()),
      stmt_string(HLSMgr->get_tree_manager()->CGetTreeNode(t.op_info.op_id)->ToString()),
      c_val(std::move(_c_val)),
      vcd_val(t.out_var_it->value),
      fullsigname(t.fullsigname),
      context(c_context),
      bitsize(t.op_info.bitsize),
      el_idx(_el_idx),
      first_c_bit(_first_c_bit),
      c_size(_c_size),
      base_index(_b)
{
}

DiscrepancyLog::~DiscrepancyLog() = default;

vcd_utility::vcd_utility(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::VCD_UTILITY),
      TM(HLSMgr->get_tree_manager()),
      Discr(_HLSMgr->RDiscr),
      allow_uninitialized((parameters->isOption(OPT_discrepancy_force) and parameters->getOption<bool>(OPT_discrepancy_force))),
      present_state_name(static_cast<HDLWriter_Language>(_parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VERILOG ? "_present_state" : "present_state")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   THROW_ASSERT(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy), "Step " + STR(__PRETTY_FUNCTION__) + " should not be added without discrepancy");
   THROW_ASSERT(HLSMgr->RDiscr, "Discr data structure is not correctly initialized");
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> vcd_utility::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_SIGNAL_SELECTION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::C_TESTBENCH_EXECUTION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

static const std::list<sig_variation>& get_signal_variations(const vcd_parser::vcd_trace_t& vcd_trace, const std::string& scope, const std::string& signal_name)
{
   const auto scopes_end = vcd_trace.end();
   const auto scopes_it = vcd_trace.find(scope);
   if(scopes_it == scopes_end)
   {
      THROW_ERROR("cannot find scope: " + scope);
   }
   const auto signal_end = scopes_it->second.end();
   const auto signal_it = scopes_it->second.find(signal_name);
   if(signal_it == signal_end)
   {
      THROW_ERROR("cannot find signal: " + signal_name + " in scope: " + scope);
   }
   return signal_it->second;
}

unsigned long long vcd_utility::GetClockPeriod(const vcd_parser::vcd_trace_t& vcd_trace) const
{
   std::string top_scope = Discr->unfolded_v_to_scope.at(Discr->unfolded_root_v);
   const std::string controller_scope = top_scope + "Controller_i" + STR(HIERARCHY_SEPARATOR);
   const std::string clock_signal_name = STR(CLOCK_PORT_NAME);
   const std::list<sig_variation>& clock_sig_variations = get_signal_variations(vcd_trace, controller_scope, clock_signal_name);
   auto clock_var_it = clock_sig_variations.begin();
   const auto clock_var_beg = clock_var_it;
   const auto clock_var_end = clock_sig_variations.end();
   const auto same_value = [clock_var_beg](const sig_variation& v) { return v.value == clock_var_beg->value; };
   clock_var_it = std::find_if(std::next(clock_var_it), clock_var_end, same_value);
   if(clock_var_it == clock_var_end)
      THROW_ERROR("clock signal for top function does not complete a cycle");
   unsigned long long clock_period = clock_var_it->time_stamp - clock_var_beg->time_stamp;
   THROW_ASSERT(clock_period > 0, "clock period is 0\n");
   return clock_period;
}

DesignFlowStep_Status vcd_utility::Exec()
{
   std::string vendor;
   const auto tgt_device = HLSMgr->get_HLS_target()->get_target_device();
   if(tgt_device->has_parameter("vendor"))
   {
      vendor = tgt_device->get_parameter<std::string>("vendor");
      boost::algorithm::to_lower(vendor);
   }
   auto isOneHot = [&](unsigned int funId) -> bool {
      auto one_hot_encoding = false;
      if(parameters->getOption<std::string>(OPT_fsm_encoding) == "one-hot")
         one_hot_encoding = true;
      else if(parameters->getOption<std::string>(OPT_fsm_encoding) == "auto" && vendor == "xilinx" && HLSMgr->get_HLS(funId)->STG->get_number_of_states() < 256)
         one_hot_encoding = true;
      return one_hot_encoding;
   };

   const CallGraphManagerConstRef cg_man = HLSMgr->CGetCallGraphManager();
   const CallGraphConstRef cg = cg_man->CGetCallGraph();
   // cleanup member data structures to allow multiple executions of this step
   THROW_ASSERT(Discr, "Discr data structure is not correctly initialized");
   possibly_lost_address = 0;
   mismatched_integers = 0;
   // TODO: should we clear the discrepancy lists of the previous executions or
   // keep them and work incrementally on them??
   discr_list.clear();
   soft_discr_list.clear();

   std::ofstream disc_stat_file;
   std::string disc_stat_filename = parameters->getOption<std::string>(OPT_output_directory) + "/simulation/dynamic_discrepancy_stats";
   disc_stat_file.open(disc_stat_filename, std::ofstream::app);
   if(not disc_stat_file.is_open())
      THROW_ERROR("can't open file " + disc_stat_filename);

   std::string vcd_filename = parameters->getOption<std::string>(OPT_output_directory) + "/simulation/test.vcd";
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Parsing vcd file " + vcd_filename);
   long vcd_parse_time = 0;
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
      START_TIME(vcd_parse_time);
   /* create vcd parser obj */
   vcd_parser vcd_parser(parameters);
   /* parse the selected signals */
   vcd_parser::vcd_trace_t vcd_trace = vcd_parser.parse_vcd(vcd_filename, HLSMgr->RDiscr->selected_vcd_signals);

   if(output_level >= OUTPUT_LEVEL_VERBOSE)
      STOP_TIME(vcd_parse_time);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Parsed vcd file " + vcd_filename + " in " + print_cpu_time(vcd_parse_time) + " seconds");
   /* parse the discrepancy trace coming from C execution */
   const std::string& discrepancy_data_filename = HLSMgr->RDiscr->c_trace_filename;
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Parsing C trace file " + discrepancy_data_filename);
   long ctrace_parse_time = 0;
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
      START_TIME(ctrace_parse_time);
   parse_discrepancy(discrepancy_data_filename, Discr);
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
      STOP_TIME(ctrace_parse_time);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Parsed C trace file " + discrepancy_data_filename + " in " + print_cpu_time(ctrace_parse_time) + " seconds");

   auto& c_op_trace = Discr->c_op_trace;
   if(c_op_trace.empty())
   {
      THROW_WARNING("Discrepancy Analysis: the trace of the C execution is empty. Discrepancy Analysis cannot be performed");
      if((not parameters->isOption(OPT_no_clean)) or (!parameters->getOption<bool>(OPT_no_clean)))
      {
         if((not parameters->isOption(OPT_generate_vcd)) or (!parameters->getOption<bool>(OPT_generate_vcd)))
         {
            boost::filesystem::remove(vcd_filename);
         }
         boost::filesystem::remove(discrepancy_data_filename);
      }
      disc_stat_file << "Possibly lost address checks = 0 (empty trace)\n";
      disc_stat_file << "Mismatched integers = 0 (empty trace)\n";
      disc_stat_file.flush();
      disc_stat_file.close();
      return DesignFlowStep_Status::SUCCESS;
   }

   std::map<unsigned int, std::map<std::string, struct vcd_trace_head>> op_id_to_scope_to_vcd_head;
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Starting discrepancy analysis");
   long discrepancy_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM)
      START_TIME(discrepancy_time);
   for(const auto& c : c_op_trace)
   {
      const DiscrepancyOpInfo& op_info = c.first;
      const std::list<std::pair<uint64_t, std::string>>& optrace = c.second;
      THROW_ASSERT(HLSMgr->RDiscr->opid_to_outsignal.find(op_info.op_id) != HLSMgr->RDiscr->opid_to_outsignal.end(), "can't find out signal for operation " + STR(op_info.op_id));
      std::string& outsigname = HLSMgr->RDiscr->opid_to_outsignal.at(op_info.op_id);
      /*
       * setup the heads for the multiple vcd traces in hw related to an
       * operation in C
       */
      THROW_ASSERT(Discr->f_id_to_scope.find(op_info.stg_fun_id) != Discr->f_id_to_scope.end(), "no scope for function " + STR(op_info.stg_fun_id));
      const auto& scope_set = Discr->f_id_to_scope.at(op_info.stg_fun_id);
      if(not scope_set.empty())
         op_id_to_scope_to_vcd_head[op_info.op_id];
      for(const std::string& scope : scope_set)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing scope " + scope);
         const std::string controller_scope = scope + "Controller_i" + STR(HIERARCHY_SEPARATOR);
         const std::string datapath_scope = scope + "Datapath_i" + STR(HIERARCHY_SEPARATOR);
         std::string fullsigname = datapath_scope + outsigname;
         /* select the variations of the output sign&l */
         const std::list<sig_variation>& op_out_vars = get_signal_variations(vcd_trace, datapath_scope, outsigname);
         /* select the variations of state signal of the state machine */
         const std::list<sig_variation>& present_state_vars = get_signal_variations(vcd_trace, controller_scope, present_state_name);
         /* select the variations of the start port signals */
         const std::list<sig_variation>& start_vars = get_signal_variations(vcd_trace, controller_scope, STR(START_PORT_NAME));
         /*
          * calculate the initial state of the FSM. this is used by the
          * vcd_trace_head to compute the exact starting time for the operation,
          * when the initial state of the FSM is a starting state for this operation
          */
         const StateTransitionGraphManagerConstRef stg_man = HLSMgr->get_HLS(op_info.stg_fun_id)->STG;
         vertex entry = stg_man->get_entry_state();
         const StateTransitionGraphConstRef stg = stg_man->CGetStg();
         THROW_ASSERT(boost::out_degree(entry, *stg) == 1, "Non deterministic initial state");
         OutEdgeIterator oe, oend;
         tie(oe, oend) = boost::out_edges(entry, *stg);
         vertex first_state = boost::target(*oe, *stg);
         const unsigned int initial_state_id = stg->CGetStateTransitionGraphInfo()->vertex_to_state_id.at(first_state);

         op_id_to_scope_to_vcd_head.at(op_info.op_id).insert(std::make_pair(scope, vcd_trace_head(op_info, fullsigname, present_state_vars, op_out_vars, start_vars, initial_state_id, GetClockPeriod(vcd_trace), HLSMgr, TM, isOneHot(op_info.stg_fun_id))));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed scope " + scope);
      }

      /*
       * Start looking for discrepancies. The analysis is performed on the trace
       * of one C operation at a time, with multiple heads analyzing the vcd
       * traces. Indeed when using duplicated modules for a single C function
       * there may be many vcd traces for every single C trace, one for every
       * duplicated module. Every vcd_trace_head has its own state. The
       * comparison works iterating on the C trace, for every assignment in C
       * the corresponding vcd_trace_head is retrieved and it is processed
       * according to its state.
       */
      auto ct_it = optrace.begin();
      const auto ct_end = optrace.cend();
      while(ct_it != ct_end)
      {
         struct vcd_trace_head& vcd_head = op_id_to_scope_to_vcd_head.at(op_info.op_id).at(Discr->context_to_scope.at(ct_it->first));
         switch(vcd_head.state)
         {
            case vcd_trace_head::checked:
            {
               if(vcd_head.more_executions_in_this_hw_state())
               {
                  vcd_head.state = vcd_trace_head::suspended;
               }
               else
               {
                  vcd_head.state = vcd_trace_head::running;
               }
               ct_it++;
               break;
            }
            case vcd_trace_head::running:
            case vcd_trace_head::suspended:
            case vcd_trace_head::uninitialized:
            {
               vcd_head.advance();
               if(vcd_head.state == vcd_trace_head::init_fail)
               {
                  print_failed_vcd_head(vcd_head, isOneHot(op_info.stg_fun_id), OUTPUT_LEVEL_VERBOSE);
               }
               break;
            }
            case vcd_trace_head::initialized:
            {
               if((not discr_list.empty()) and vcd_head.ends_after(discr_list.front().op_end_time))
               {
                  vcd_head.state = vcd_trace_head::after_discrepancy;
                  break;
               }
               if(detect_mismatch(vcd_head, ct_it->first, ct_it->second))
               {
                  vcd_head.state = vcd_trace_head::discrepancy_found;
               }
               else
               {
                  vcd_head.exec_times_in_current_state++;
                  vcd_head.state = vcd_trace_head::checked;
               }
               break;
            }
            case vcd_trace_head::init_fail:
            case vcd_trace_head::discrepancy_found:
            case vcd_trace_head::after_discrepancy:
            {
               ct_it++;
               break;
            }
            default:
            {
               THROW_UNREACHABLE("invalid state for vcd_trace_head");
               break;
            }
         }
      }
   }

   if(output_level >= OUTPUT_LEVEL_MINIMUM)
      STOP_TIME(discrepancy_time);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Discrepancy analysis executed in " + print_cpu_time(discrepancy_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Possibly lost address checks = " + STR(possibly_lost_address));

   disc_stat_file << "Possibly lost address checks = ";
   disc_stat_file << possibly_lost_address << "\n";
   disc_stat_file << "Mismatched integers = ";
   disc_stat_file << mismatched_integers << "\n";
   disc_stat_file.flush();
   disc_stat_file.close();

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---DISCREPANCY CHECKS: " + STR(Discr->n_checked_operations) + "/" + STR(Discr->n_total_operations));

   bool first_discrepancy_print = true;
   if(not discr_list.empty() or not soft_discr_list.empty())
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                     "\n\n\n"
                     "/====================================\\\n"
                     "|      FINAL DISCREPANCY REPORT      |\n"
                     "\\====================================/");
      if(not discr_list.empty())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                        "\n\n"
                        "/--------------------------------\\\n"
                        "|       HARD DISCREPANCIES       |\n"
                        "\\--------------------------------/");
         for(const auto& l : discr_list)
            print_discrepancy(l, isOneHot(l.fun_id), OUTPUT_LEVEL_NONE);
      }
      if(not soft_discr_list.empty())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                        "\n\n"
                        "/--------------------------------\\\n"
                        "|       SOFT DISCREPANCIES       |\n"
                        "\\--------------------------------/");
         for(const auto& l : soft_discr_list)
            print_discrepancy(l, isOneHot(l.fun_id), OUTPUT_LEVEL_NONE);
      }
      first_discrepancy_print = false;
   }

   bool persisting_warning = false;
   bool before_discrepancy = false;
   bool suspended_op_never_ends = false;
   // print any persisting warning that starts before the first discrepancy
   for(const auto& id_to_scope_to_head : op_id_to_scope_to_vcd_head)
   {
      for(const auto& t : id_to_scope_to_head.second)
      {
         before_discrepancy = discr_list.empty() or not t.second.starts_after(discr_list.front().op_start_time);

         suspended_op_never_ends = t.second.state == vcd_trace_head::suspended and t.second.consecutive_state_executions != std::numeric_limits<decltype(t.second.consecutive_state_executions)>::max();

         if(before_discrepancy and t.second.has_been_initialized and (t.second.state == vcd_trace_head::init_fail or suspended_op_never_ends))
         {
            if(first_discrepancy_print)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                              "\n\n\n"
                              "/====================================\\\n"
                              "|      FINAL DISCREPANCY REPORT      |\n"
                              "\\====================================/");
               first_discrepancy_print = false;
            }
            if(not persisting_warning)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                              "\n\n"
                              "/------------------------------\\\n"
                              "|       PERSISTING ERRORS      |\n"
                              "\\------------------------------/");
            }
            print_failed_vcd_head(t.second, isOneHot(t.second.op_info.stg_fun_id), OUTPUT_LEVEL_NONE);
            persisting_warning = true;
         }
      }
   }
   if(persisting_warning or not discr_list.empty() or not soft_discr_list.empty())
   {
      THROW_ERROR("DISCREPANCY FOUND");
   }
   else
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---DISCREPANCY NOT FOUND");
      if((not parameters->isOption(OPT_no_clean)) or (!parameters->getOption<bool>(OPT_no_clean)))
      {
         if((not parameters->isOption(OPT_generate_vcd)) or (!parameters->getOption<bool>(OPT_generate_vcd)))
         {
            boost::filesystem::remove(vcd_filename);
         }
         boost::filesystem::remove(discrepancy_data_filename);
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}

bool vcd_utility::HasToBeExecuted() const
{
   return true;
}

bool vcd_utility::detect_mismatch(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val)
{
   const std::string& vcd_val = t.out_var_it->value;
   bool is_mismatch = false;
   if(parameters->isOption(OPT_discrepancy_force) and parameters->getOption<bool>(OPT_discrepancy_force) and vcd_val.find_first_not_of("01") != std::string::npos)
   {
      /*
       * The user is saying that he/she guarantees that there are no uninitialized
       * variables in the original C specification. For this reason any value in
       * vcd different from 0 or 1 is considered an error, independently of the
       * values in C.
       * The last four parameters are set to 0, because they are irrelevant in
       * case of an error like this.
       */
      update_discr_list(t, c_context, c_val, 0, 0, 0, 0);
      return true;
   }

   if(t.op_info.type & DISCR_VECTOR)
   {
      THROW_ASSERT(c_val.length() % t.op_info.vec_base_bitsize == 0, "ssa_name = " + STR(t.op_info.ssa_name) +
                                                                         "\n"
                                                                         "ssa node id = " +
                                                                         STR(t.op_info.ssa_name_node_id) +
                                                                         "\n"
                                                                         "c_val.length() = " +
                                                                         STR(c_val.length()) +
                                                                         "\n"
                                                                         "vec_base_bitsize = " +
                                                                         STR(t.op_info.vec_base_bitsize) + "\n");

      THROW_ASSERT(vcd_val.length() >= c_val.length(), "ssa_name = " + STR(t.op_info.ssa_name) +
                                                           "\n"
                                                           "ssa node id = " +
                                                           STR(t.op_info.ssa_name_node_id) +
                                                           "\n"
                                                           "c_val.length() = " +
                                                           STR(c_val.length()) +
                                                           "\n"
                                                           "vcd signal = " +
                                                           t.fullsigname +
                                                           "\n"
                                                           "vcd length = " +
                                                           STR(vcd_val.length()) + "\n");

      for(unsigned int i = 0u; i < c_val.length() / t.op_info.vec_base_bitsize; i++)
      {
         is_mismatch |= detect_mismatch_element(t, c_context, c_val, i);
      }
   }
   else
   {
      is_mismatch = detect_mismatch_element(t, c_context, c_val, 0u);
   }
   return is_mismatch;
}

bool vcd_utility::detect_mismatch_element(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val, const unsigned int el_idx)
{
#if HAVE_ASSERTS
   const std::string& vcd_val = t.out_var_it->value;
#endif

   std::string::size_type first_c_bit = el_idx * t.op_info.vec_base_bitsize;
   std::string::size_type c_elem_size = t.op_info.vec_base_bitsize;

   THROW_ASSERT(c_val.length() >= first_c_bit + c_elem_size, "ssa_name = " + STR(t.op_info.ssa_name) +
                                                                 "\n"
                                                                 "ssa node id = " +
                                                                 STR(t.op_info.ssa_name_node_id) +
                                                                 "\n"
                                                                 "c_val.length() = " +
                                                                 STR(c_val.length()) +
                                                                 "\n"
                                                                 "first_c_bit = " +
                                                                 STR(first_c_bit) +
                                                                 "\n"
                                                                 "c_elem_size = " +
                                                                 STR(c_elem_size) + "\n");

   THROW_ASSERT(t.out_var_it->value.length() > first_c_bit, "ssa_name = " + STR(t.op_info.ssa_name) +
                                                                "\n"
                                                                "ssa node id = " +
                                                                STR(t.op_info.ssa_name_node_id) +
                                                                "\n"
                                                                "c_val.length() = " +
                                                                STR(c_val.length()) +
                                                                "\n"
                                                                "first_c_bit = " +
                                                                STR(first_c_bit) +
                                                                "\n"
                                                                "vcd signal = " +
                                                                t.fullsigname +
                                                                "\n"
                                                                "vcd length = " +
                                                                STR(vcd_val.length()) + "\n");

   bool is_mismatch = false;
   if(t.op_info.type & DISCR_COMPLEX)
   {
      THROW_ASSERT(t.out_var_it->value.length() >= c_val.length(), "ssa_name = " + STR(t.op_info.ssa_name) +
                                                                       "\n"
                                                                       "ssa node id = " +
                                                                       STR(t.op_info.ssa_name_node_id) +
                                                                       "\n"
                                                                       "c_val.length() = " +
                                                                       STR(c_val.length()) +
                                                                       "\n"
                                                                       "first_c_bit = " +
                                                                       STR(first_c_bit) +
                                                                       "\n"
                                                                       "vcd signal = " +
                                                                       t.fullsigname +
                                                                       "\n"
                                                                       "vcd length = " +
                                                                       STR(vcd_val.length()) + "\n");

      std::string::size_type part_size = c_elem_size / 2;

      is_mismatch = detect_mismatch_simple(t, c_context, c_val, el_idx, first_c_bit, part_size);

      is_mismatch |= detect_mismatch_simple(t, c_context, c_val, el_idx, first_c_bit + part_size, part_size);
   }
   else
   {
      is_mismatch = detect_mismatch_simple(t, c_context, c_val, el_idx, first_c_bit, c_elem_size);
   }
   return is_mismatch;
}

bool vcd_utility::detect_mismatch_simple(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val, const unsigned int el_idx, const std::string::size_type first_c_bit, const std::string::size_type c_size)
{
   unsigned int base_index = 0; // this is set when an address mismatch is detected
   const bool resized = c_val.length() != c_size;
   const std::string& resized_c_val = resized ? c_val.substr(first_c_bit, c_size) : c_val;

   THROW_ASSERT(not resized or (t.op_info.type & (DISCR_COMPLEX | DISCR_VECTOR)), "operation " + STR(t.op_info.op_id) + " assigns ssa " + STR(t.op_info.ssa_name) + ": only complex or vectors are supposed to be resized for discrepancy analysis");

   const std::string& vcd = t.out_var_it->value;

   auto vcd_first_bit = first_c_bit;
   if(vcd.size() > c_val.size())
      vcd_first_bit += vcd.size() - c_val.size();

   const std::string& resized_vcd_val = resized ? vcd.substr(vcd_first_bit, c_size) : vcd;

   bool discrepancy_found = detect_regular_mismatch(t, resized_c_val, resized_vcd_val);

   if(discrepancy_found and (t.op_info.type & DISCR_ADDR))
   {
      THROW_ASSERT(not(t.op_info.type & DISCR_REAL) and not(t.op_info.type & DISCR_COMPLEX), "address ssa cannot be real or complex: "
                                                                                             "real = " +
                                                                                                 STR(static_cast<bool>(t.op_info.type & DISCR_VECTOR)) + "complex = " + STR(static_cast<bool>(t.op_info.type & DISCR_COMPLEX)));
      mismatched_integers++;
      /*
       * if the ssa stores an address first we try to find a regular
       * mismatch. if we find it, this means that the value cannot be
       * directly compared as an integer, so we have to translate it in
       * the address space of the hw accelerator.
       */
      discrepancy_found = detect_address_mismatch(t.op_info, c_context, resized_c_val, resized_vcd_val, base_index);
   }
   if(discrepancy_found)
   {
      update_discr_list(t, c_context, c_val, el_idx, first_c_bit, c_size, base_index);
   }
   return discrepancy_found;
}

void vcd_utility::update_discr_list(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val, const unsigned int el_idx, const std::string::size_type first_c_bit, const std::string::size_type c_size, const unsigned int base_index)
{
   bool hard_discrepancy = true;
   if(parameters->isOption(OPT_discrepancy_permissive_ptrs) and parameters->getOption<bool>(OPT_discrepancy_permissive_ptrs) and (t.op_info.type & DISCR_ADDR))
   {
      hard_discrepancy = false;
   }

   if(hard_discrepancy)
   {
      if(not discr_list.empty() and t.op_end_time < discr_list.front().op_end_time)
      {
         /*
          * if the list is not empty look for end times to see if the discrepancy
          * that we just found is to be considered the first discrepancy happened
          * in the execution.
          * if the new end_time is _strictly_ smaller than the previous, then
          * this is actually the first discrepancy, hence the list of the
          * discr_list must be clreared before insertion.
          */
         discr_list.clear();
      }
      discr_list.emplace_back(HLSMgr, t, c_context, c_val, el_idx, first_c_bit, c_size, base_index);

      /*
       * remove from the list of soft discrepancies those that happen after the
       * first hard discrepancy
       */
      std::list<decltype(soft_discr_list.begin())> soft_discr_to_remove;
      auto soft_discr_it = soft_discr_list.begin();
      const auto soft_discr_end = soft_discr_list.cend();

      for(; soft_discr_it != soft_discr_end; soft_discr_it++)
      {
         if(soft_discr_it->op_end_time > t.op_end_time)
         {
            soft_discr_to_remove.push_back(soft_discr_it);
         }
      }

      for(const auto& s : soft_discr_to_remove)
      {
         soft_discr_list.erase(s);
      }
   }
   else
   {
      if(discr_list.empty() or t.op_end_time <= discr_list.front().op_end_time)
      {
         soft_discr_list.emplace_back(HLSMgr, t, c_context, c_val, el_idx, first_c_bit, c_size, base_index);
      }
   }
   return;
}

bool vcd_utility::detect_binary_float_mismatch(const std::string& c_val, const std::string& resized_vcd_val) const
{
   THROW_ASSERT(c_val.length() == resized_vcd_val.length(), STR(c_val.length()) + " != " + STR(resized_vcd_val.length()));
   union {
      float f;
      unsigned int b;
   } expected, computed;
   computed.b = 0;
   expected.b = 0;
   size_t len = c_val.length();
   for(size_t i = 0; i < len; i++)
   {
      computed.b |= (resized_vcd_val.at(i) == '1') ? (1u << (len - i - 1)) : 0;
      expected.b |= (c_val.at(i) == '1') ? (1u << (len - i - 1)) : 0;
   }
   float ulp = 0.0;
   float& c = computed.f;
   float& e = expected.f;
   if(std::isnan(c) and std::isnan(e))
   {
      return false;
   }
   else if(std::isinf(c) and std::isinf(e))
   {
      if(std::signbit(c) != std::signbit(e))
         return true;
      else
         return false;
   }
   else if(std::isinf(c) or std::isinf(e) or std::isnan(c) or std::isnan(e))
   {
      return false;
   }
   else
   {
      if(e == 0.0f)
         ulp = std::fabs(c - e) / std::ldexp(1.0f, -(FLT_MANT_DIG - 1));
      else
         ulp = std::fabs(c - e) / std::ldexp(1.0f, std::ilogb(e) - (FLT_MANT_DIG - 1));

      return static_cast<double>(ulp) > parameters->getOption<double>(OPT_max_ulp);
   }
   return false;
}

bool vcd_utility::detect_binary_double_mismatch(const std::string& c_val, const std::string& resized_vcd_val) const
{
   THROW_ASSERT(c_val.length() == resized_vcd_val.length(), STR(c_val.length()) + " != " + STR(resized_vcd_val.length()));
   union {
      double f;
      unsigned long long b;
   } expected, computed;
   computed.b = 0;
   expected.b = 0;
   size_t len = c_val.length();
   for(size_t i = 0; i < len; i++)
   {
      computed.b |= (resized_vcd_val.at(i) == '1') ? (1ULL << (len - i - 1)) : 0;
      expected.b |= (c_val.at(i) == '1') ? (1ULL << (len - i - 1)) : 0;
   }
   double ulp = 0.0;
   double& c = computed.f;
   double& e = expected.f;
   if(std::isnan(c) and std::isnan(e))
   {
      return false;
   }
   else if(std::isinf(c) and std::isinf(e))
   {
      if(std::signbit(c) != std::signbit(e))
         return true;
      else
         return false;
   }
   else if(std::isinf(c) or std::isinf(e) or std::isnan(c) or std::isnan(e))
   {
      return false;
   }
   else
   {
      if(e == 0.0)
         ulp = std::fabs(c - e) / std::ldexp(1.0, -(DBL_MANT_DIG - 1));
      else
         ulp = std::fabs(c - e) / std::ldexp(1.0, std::ilogb(e) - (DBL_MANT_DIG - 1));

      return ulp > parameters->getOption<double>(OPT_max_ulp);
   }
   return false;
}

bool vcd_utility::detect_fixed_address_mismatch(const DiscrepancyOpInfo& op_info, const uint64_t c_context, const std::string& c_val, const std::string& vcd_val, const unsigned int base_index) const
{
   const auto context_it = Discr->c_addr_map.find(c_context);
   THROW_ASSERT(context_it != Discr->c_addr_map.end(), "no address map found for context " + STR(c_context));
   const auto var_id_to_base_address_it = context_it->second.find(base_index);
   if(var_id_to_base_address_it == context_it->second.end())
      return false;

   uint64_t c_addr = std::stoull(c_val.c_str(), nullptr, 2);
   uint64_t c_base_addr = var_id_to_base_address_it->second;
   bool c_offset_is_negative = c_addr < c_base_addr;
   uint64_t c_addr_offset = c_addr - c_base_addr;
   if(c_offset_is_negative)
      c_addr_offset = c_base_addr - c_addr;
   /* don't detect mismatch for out of bounds address */
   const uint64_t memory_area_bitsize = std::max(8u, tree_helper::size(TM, tree_helper::get_type_index(TM, base_index)));
   THROW_ASSERT(memory_area_bitsize % 8 == 0, "bitsize of a variable in memory must be multiple of 8 --> is " + STR(memory_area_bitsize));
   if(c_offset_is_negative or ((memory_area_bitsize / 8) <= (c_addr - c_base_addr)))
      return false;
   /*
    * if the value of the hw address is retrieved with a load, the vcd signal
    * representing the pointer can be wider than 32 bits. in that case we take
    * the lowest 32 bits
    */
   const std::string& resized_vcd_val = c_val.length() < vcd_val.length() ? vcd_val.substr(vcd_val.length() - c_val.length()) : vcd_val;
   /*
    * detect a mismatch if some of the resized address bits are not zeros or ones
    */
   if(resized_vcd_val.find_first_not_of("01") != std::string::npos)
      return true;
   uint64_t vcd_addr = std::stoull(resized_vcd_val.c_str(), nullptr, 2);
   uint64_t vcd_base_addr = HLSMgr->Rmem->get_base_address(base_index, op_info.stg_fun_id);
   bool vcd_offset_is_negative = vcd_addr < vcd_base_addr;
   uint64_t vcd_addr_offset = vcd_addr - vcd_base_addr;
   if(vcd_offset_is_negative)
      vcd_addr_offset = vcd_base_addr - vcd_addr;
   /* if the vcd is out of range even if the address in C is in range, we have a mismatch */
   if(vcd_offset_is_negative or ((memory_area_bitsize / 8) <= (vcd_addr - vcd_base_addr)))
      return true;
   else
      return c_addr_offset != vcd_addr_offset;
}

bool vcd_utility::detect_address_mismatch(const DiscrepancyOpInfo& op_info, const uint64_t c_context, const std::string& c_val, const std::string& vcd_val, unsigned int& base_index)
{
   const auto* ssa = GetPointer<const ssa_name>(TM->get_tree_node_const(op_info.ssa_name_node_id));
   base_index = tree_helper::get_base_index(TM, op_info.ssa_name_node_id);
   if(base_index != 0 and HLSMgr->Rmem->has_base_address(base_index))
   {
      return detect_fixed_address_mismatch(op_info, c_context, c_val, vcd_val, base_index);
   }
   else
   {
      if(ssa->use_set->is_fully_resolved())
      {
#if HAVE_ASSERTS
         bool at_least_one_pointed_variable_is_in_scope = false;
#endif
         for(const tree_nodeRef& pointed_var_decl_id : ssa->use_set->variables)
         {
            base_index = tree_helper::get_base_index(TM, GET_INDEX_NODE(pointed_var_decl_id));
            THROW_ASSERT(base_index != 0 and HLSMgr->Rmem->has_base_address(base_index), "opid = " + STR(op_info.op_id) + " base index = " + STR(base_index) + " has no base address");
            THROW_ASSERT(Discr->c_addr_map.find(c_context) != Discr->c_addr_map.end(), "no address map found for context " + STR(c_context));
            if(Discr->c_addr_map.at(c_context).find(base_index) != Discr->c_addr_map.at(c_context).end())
            {
#if HAVE_ASSERTS
               at_least_one_pointed_variable_is_in_scope = true;
#endif
               if(detect_fixed_address_mismatch(op_info, c_context, c_val, vcd_val, base_index))
                  return true;
            }
         }
         THROW_ASSERT(at_least_one_pointed_variable_is_in_scope == true, "no pointed variable are in scope for opid = " + STR(op_info.op_id));
      }
      else
      {
         base_index = 0;
         const uint64_t c_addr = static_cast<unsigned int>(std::stoull(c_val.c_str(), nullptr, 2));
         CustomUnorderedMap<uint64_t, unsigned int> addr2base_index;
         CustomOrderedSet<uint64_t> addrSet;
         for(const auto& addr : Discr->c_addr_map.at(c_context))
         {
            addr2base_index[addr.second] = addr.first;
            addrSet.insert(addr.second);
         }
         for(auto it_set = addrSet.rbegin(); it_set != addrSet.rend(); ++it_set)
            if(c_addr <= *it_set)
               base_index = addr2base_index.at(*it_set);

         if(base_index)
         {
            return detect_fixed_address_mismatch(op_info, c_context, c_val, vcd_val, base_index);
         }
         else
         {
            possibly_lost_address++;
            return false;
         }
      }
   }
   return false;
}

bool vcd_utility::detect_regular_mismatch(const vcd_trace_head& t, const std::string& c_val, const std::string& vcd_val) const
{
   if(t.op_info.type & DISCR_REAL)
   {
      THROW_ASSERT(c_val.length() <= vcd_val.length(), "ssa_name = " + t.op_info.ssa_name +
                                                           "\n"
                                                           "vcd signal = " +
                                                           t.fullsigname +
                                                           "\n"
                                                           "c_val.length() = " +
                                                           STR(c_val.length()) +
                                                           "\n"
                                                           "vcd_val.length() = " +
                                                           STR(vcd_val.length()) + "\n");

      const bool to_resize = c_val.length() < vcd_val.length();
      const std::string resized_vcd_val = to_resize ? (vcd_val.substr(vcd_val.length() - c_val.length())) : vcd_val;

      if(c_val.length() == 32)
      {
         if(resized_vcd_val.find_first_not_of("01") != std::string::npos)
            return allow_uninitialized;
         return detect_binary_float_mismatch(c_val, resized_vcd_val);
      }
      else if(c_val.length() == 64)
      {
         if(resized_vcd_val.find_first_not_of("01") != std::string::npos)
            return allow_uninitialized;
         return detect_binary_double_mismatch(c_val, resized_vcd_val);
      }
      else
      {
         THROW_UNREACHABLE("only floating point with bitsize 32 or 64 are currently supported\n"
                           "ssa_name = " +
                           t.op_info.ssa_name +
                           "\n"
                           "c_val.length() = " +
                           STR(c_val.length()) + "\n");
         return true;
      }
   }
   else // is an integer
   {
      std::string bitvalue = GetPointer<const ssa_name>(GET_NODE(TM->CGetTreeReindex(t.op_info.ssa_name_node_id)))->bit_values;
      auto first_not_x_pos = bitvalue.find_first_not_of("xX");
      if(first_not_x_pos == std::string::npos)
         return false;
      if(bitvalue.size() < vcd_val.size())
         first_not_x_pos += vcd_val.size() - bitvalue.size();
      const std::string vcd_trimmed_val = vcd_val.substr(first_not_x_pos);
      const std::string& longer = (vcd_trimmed_val.length() <= c_val.length()) ? c_val : vcd_trimmed_val;
      const std::string& shorter = (vcd_trimmed_val.length() <= c_val.length()) ? vcd_trimmed_val : c_val;
      if(longer.find_first_not_of("01") != std::string::npos or shorter.find_first_not_of("01") != std::string::npos)
         return allow_uninitialized;
      return shorter != longer.substr(longer.length() - shorter.length());
   }
}

void vcd_utility::print_discrepancy(const DiscrepancyLog& l, bool one_hot_encoding, const int verbosity) const
{
   std::string out_msg = "\n/--------------------------------------------------------------------\n"
                         "|  ERROR in signal " +
                         l.fullsigname +
                         "\n"
                         "|  value in vcd: " +
                         l.vcd_val +
                         "\n"
                         "|  value in C:   " +
                         l.c_val +
                         "\n"
                         "|  bitvalue: " +
                         STR(l.bitsize) +
                         "\n"
                         "|  operation id: " +
                         STR(l.fun_id) + "_" + STR(l.op_id) +
                         "\n"
                         "|  in function: " +
                         l.fu_name +
                         "\n"
                         "|  in context: " +
                         STR(l.context) +
                         "\n"
                         "|  operation: " +
                         l.stmt_string +
                         "\n"
                         "|  discrepancy time = " +
                         STR(l.op_end_time) +
                         "\n"
                         "|  failed operation starts at time " +
                         STR(l.op_start_time) +
                         "\n"
                         "|  when fsm state is " +
                         compute_fsm_state_from_vcd_string(l.op_start_state, one_hot_encoding) +
                         "\n"
                         "|  assigned ssa id " +
                         STR(GetPointer<const ssa_name>(GET_NODE(TM->CGetTreeReindex(l.ssa_id)))) +
                         "\n"
                         "|  bitvalue string for ssa id is " +
                         GetPointer<const ssa_name>(GET_NODE(TM->CGetTreeReindex(l.ssa_id)))->bit_values + "\n";

   if(l.type & DISCR_ADDR)
   {
      out_msg += "|  THE DISCREPANCY IS ON A SIGNAL THAT MAY REPRESENT AN ADDRESS\n";

      const auto context_it = Discr->c_addr_map.find(l.context);
      THROW_ASSERT(context_it != Discr->c_addr_map.end(), "context " + STR(l.context) + " not find");
      const auto var_id_to_base_address_it = context_it->second.find(l.base_index);
      if(var_id_to_base_address_it != context_it->second.end())
      {
         uint64_t c_base_addr = var_id_to_base_address_it->second;
         out_msg += "|  referenced variable decl id: " + STR(l.base_index) + "\n";

         out_msg += "|  base address in C: " + STR(c_base_addr) + "\n";

         uint64_t c_addr = std::stoull(l.c_val.c_str(), nullptr, 2);
         out_msg += "|  address in C: " + STR(c_addr) + "\n";

         uint64_t c_addr_offset = c_addr - c_base_addr;
         THROW_ASSERT(c_addr >= c_base_addr, "discrepancies for out of range addresses should not be raised:"
                                             " c_addr = " +
                                                 STR(c_addr) + " c_base_addr = " + STR(c_base_addr));
         out_msg += "|  address offset in C: " + STR(c_addr_offset) + "\n";
#if HAVE_ASSERTS
         const uint64_t memory_area_bitsize = std::max(8u, tree_helper::size(TM, tree_helper::get_type_index(TM, l.base_index)));
         THROW_ASSERT(memory_area_bitsize % 8 == 0, "bitsize of a variable in memory must be multiple of 8 --> is " + STR(memory_area_bitsize) + ": this should be catched by an assertion earlier");
#endif
      }
      else
      {
         out_msg += "|  the address is not in range for any variable in context " + STR(l.context) + "\n";
      }

      std::string resized_vcd_val;
      if(l.c_val.length() < l.vcd_val.length())
         resized_vcd_val = l.vcd_val.substr(l.vcd_val.length() - l.c_val.length());
      const std::string& vcd = resized_vcd_val.empty() ? l.vcd_val : resized_vcd_val;
      if(vcd.find_first_not_of("01") == std::string::npos)
      {
         uint64_t vcd_base_addr = HLSMgr->Rmem->get_base_address(l.base_index, l.fun_id);
         out_msg += "|  base address in vcd: " + STR(vcd_base_addr) + "\n";

         uint64_t vcd_addr = std::stoull(vcd.c_str(), nullptr, 2);
         out_msg += "|  address in vcd: " + STR(vcd_addr) + "\n";

         uint64_t vcd_addr_offset = vcd_addr >= vcd_base_addr ? vcd_addr - vcd_base_addr : vcd_base_addr - vcd_addr;
         out_msg += "|  address offset in vcd: " + (vcd_addr >= vcd_base_addr ? std::string("") : std::string("-")) + STR(vcd_addr_offset) + "\n";
      }
      else
      {
         out_msg += "|  address bits in vcd: " + vcd +
                    "\n"
                    "|  this cannot be converted to a valid address since it contains values different from 0 and 1\n";
      }
   }
   if(l.type & DISCR_VECTOR)
   {
      out_msg += "|  THE DISCREPANCY IS IN A SIGNAL REPRESENTING A VECTORIZED VARIABLE\n";
   }
   if(l.type & DISCR_COMPLEX)
   {
      out_msg += "|  THE DISCREPANCY IS IN A COMPLEX VARIABLE\n";
   }
   out_msg += "\\--------------------------------------------------------------------\n";

   INDENT_OUT_MEX(verbosity, output_level, out_msg);
   return;
}

void vcd_utility::print_failed_vcd_head(const vcd_trace_head& t, bool one_hot_encoding, const int verbosity) const
{
   std::string out_msg = "\n/--------------------------------------------------------------------\n"
                         "|  ERROR in signal " +
                         t.fullsigname + "\n";

   const std::string state = compute_fsm_state_from_vcd_string(t.fsm_ss_it->value, one_hot_encoding);
   switch(t.failed)
   {
      case vcd_trace_head::function_does_not_start:
      {
         out_msg += "|  operation " + STR(t.op_info.stg_fun_id) + "_" + STR(t.op_info.op_id) +
                    "\n"
                    "|  is scheduled in the initial state S_0 of a function which does not start\n"
                    "|  failed operation starts at time " +
                    STR(t.op_start_time) +
                    "\n"
                    "|  when fsm state is " +
                    state + "\n";
         break;
      }
      case vcd_trace_head::no_start_state:
      {
         out_msg += "|  cannot find next start state for operation " + STR(t.op_info.stg_fun_id) + "_" + STR(t.op_info.op_id) +
                    "\n"
                    "|  previous execution started at time  " +
                    STR(t.op_start_time) +
                    "\n"
                    "|  when fsm state was " +
                    state +
                    "\n"
                    "|  and ended at time " +
                    STR(t.op_end_time) + "\n";
         break;
      }
      case vcd_trace_head::no_end_state:
      {
         out_msg += "|  cannot find next end state for operation " + STR(t.op_info.stg_fun_id) + "_" + STR(t.op_info.op_id) +
                    "\n"
                    "|  never reaches an end in HDL simulation\n"
                    "|  failed operation starts at time " +
                    STR(t.op_start_time) +
                    "\n"
                    "|  when fsm state is " +
                    state + "\n";
         break;
      }
      case vcd_trace_head::fail_none:
      default:
      {
         THROW_UNREACHABLE("unexpected failure state for vcd head\n");
      }
   }
   out_msg += "\\--------------------------------------------------------------------\n";
   INDENT_OUT_MEX(verbosity, output_level, out_msg);
}

std::string vcd_utility::compute_fsm_state_from_vcd_string(const std::string& vcd_state_string, bool one_hot_encoding) const
{
   if(vcd_state_string.find_first_not_of("01") != std::string::npos)
   {
      /*
       * the vcd state string contains values that are not 0 or 1. something
       * went wrong, so we print the raw state string as it was found in vcd
       */
      return vcd_state_string;
   }

   if(one_hot_encoding)
   {
      const auto first_1_pos = vcd_state_string.find_first_of('1');
      const auto last_1_pos = vcd_state_string.find_last_of('1');
      if(first_1_pos != last_1_pos)
      {
         /*
          * not really one hot encoding. something went wrong. so print the raw
          * state string as it was found in vcd
          */
         return vcd_state_string;
      }
      return STR(vcd_state_string.length() - first_1_pos - 1);
   }
   else
   {
      return STR(std::stoul(vcd_state_string.c_str(), nullptr, 2));
   }
}
