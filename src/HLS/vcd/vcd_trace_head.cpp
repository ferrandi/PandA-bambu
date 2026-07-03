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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 */
#include "vcd_trace_head.hpp"

#include "allocation_information.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "storage_value_information.hpp"
#include "string_manipulation.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#include <algorithm>
#include <functional>
#include <utility>

static bool is_valid_state_string(const std::string& s, bool one_hot_fsm_encoding)
{
   if(s.find_first_not_of("10") != std::string::npos)
   {
      /*
       * the vcd state string contains values that are not 0 or 1. something
       * went wrong, so the result is always false
       */
      return false;
   }
   if(one_hot_fsm_encoding)
   {
      const auto first_1_pos = s.find_first_of('1');
      const auto last_1_pos = s.find_last_of('1');
      if(first_1_pos != last_1_pos)
      {
         /*
          * not really one hot encoding. something went wrong, so the result is
          * always false
          */
         return false;
      }
   }
   return true;
}

static unsigned int compute_state_id(const std::string& s, bool one_hot_fsm_encoding)
{
   THROW_ASSERT(is_valid_state_string(s, one_hot_fsm_encoding), "String does not represent a valid binary state: " + s);
   if(one_hot_fsm_encoding)
   {
      const auto first_1_pos = s.find_first_of('1');
      return static_cast<unsigned int>(s.length() - first_1_pos - 1U);
   }
   return static_cast<unsigned int>(std::stoul(s, nullptr, 2));
}

static bool is_binary_string_repr(const std::string& s, unsigned int id, bool one_hot_fsm_encoding)
{
   if(!is_valid_state_string(s, one_hot_fsm_encoding))
   {
      THROW_ERROR("invalid state_string: " + s);
   }

   return compute_state_id(s, one_hot_fsm_encoding) == id;
}

static bool string_represents_one_of_the_states(const std::string& val, const std::vector<unsigned int>& state_ids,
                                                bool one_hot_fsm_encoding)
{
   if(!is_valid_state_string(val, one_hot_fsm_encoding))
   {
      return false;
   }
   const auto id = compute_state_id(val, one_hot_fsm_encoding);
   return std::find(state_ids.begin(), state_ids.end(), id) != state_ids.end();
}

static unsigned int get_fsm_fun_id(const ir_managerConstRef& TM, const unsigned int op_id)
{
   const auto fid = ir_helper::GetFunctionIdFromOpId(TM, op_id);
   THROW_ASSERT(fid != 0, "op_trace event op id " + STR(op_id) + " has no function context");
   return fid;
}

static unsigned int get_ssa_name_node_id(const ir_managerConstRef& TM, const unsigned int op_id)
{
   const auto ssa_id = ir_helper::GetSsaNameNodeIdFromOpId(TM, op_id);
   THROW_ASSERT(ssa_id != 0, "op_trace event op id " + STR(op_id) + " has no ssa_node");
   return ssa_id;
}

static op_timing_info get_op_timing(const HLS_managerConstRef& HLSMgr, const unsigned int fsm_fun_id,
                                    const unsigned int op_id)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(fsm_fun_id);
   const auto& op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
   const auto& op_graph_info = op_graph.CGetGraphInfo();
   const auto op_it = op_graph_info.ir_node_to_operation.find(op_id);
   THROW_ASSERT(op_it != op_graph_info.ir_node_to_operation.end(), "cannot find operation for op id " + STR(op_id));
   const auto statement = op_it->second;
   const auto hls = HLSMgr->get_HLS(fsm_fun_id);
   const auto fu_tech_n = hls->allocation_information->get_fu(hls->Rfu->get_assign(statement));
   const auto op_tech_n = GetPointer<functional_unit>(fu_tech_n)->get_operation(
       ir_helper::NormalizeTypename(op_graph.CGetNodeInfo(statement).GetOperation()));
   const auto* oper = GetPointer<operation>(op_tech_n);
   THROW_ASSERT(oper, "missing operation info for op id " + STR(op_id));
   op_timing_info timing;
   timing.is_bounded = oper->is_bounded();
   timing.n_cycles = timing.is_bounded ? oper->time_m->get_cycles() : 0;
   return timing;
}

static op_state_info get_op_state_info(const HLS_managerConstRef& HLSMgr, const unsigned int fsm_fun_id,
                                       const unsigned int op_id)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(fsm_fun_id);
   const auto& op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
   const auto& op_graph_info = op_graph.CGetGraphInfo();
   const auto op_it = op_graph_info.ir_node_to_operation.find(op_id);
   THROW_ASSERT(op_it != op_graph_info.ir_node_to_operation.end(), "cannot find operation for op id " + STR(op_id));
   const auto statement = op_it->second;
   const auto fsm_info = HLSMgr->get_HLS(fsm_fun_id)->fsm_info;
   op_state_info info;
   const auto insert_state = [](std::vector<unsigned int>& states, const unsigned int state_id) {
      if(std::find(states.begin(), states.end(), state_id) == states.end())
      {
         states.push_back(state_id);
      }
   };
   for(const auto& kv : fsm_info->verticesWithData())
   {
      const auto& state_data = kv.second;
      const bool has_start = std::find(state_data.startingOperations.begin(), state_data.startingOperations.end(),
                                       statement) != state_data.startingOperations.end();
      const bool has_exec = std::find(state_data.executingOperations.begin(), state_data.executingOperations.end(),
                                      statement) != state_data.executingOperations.end();
      const bool has_end = std::find(state_data.endingOperations.begin(), state_data.endingOperations.end(),
                                     statement) != state_data.endingOperations.end();
      if(!(has_start || has_exec || has_end))
      {
         continue;
      }
      const auto state_id = fsm_info->getStateId(kv.first);
      if(has_start)
      {
         insert_state(info.start_states, state_id);
      }
      if(has_exec)
      {
         insert_state(info.exec_states, state_id);
      }
      if(has_end)
      {
         insert_state(info.end_states, state_id);
      }
   }
   return info;
}

static bool is_exec(const sig_variation& state_var, const op_state_info& info,
                    const unsigned int ASSERT_PARAMETER(fsm_fun_id), const unsigned int ASSERT_PARAMETER(op_id),
                    bool one_hot_fsm_encoding)
{
   THROW_ASSERT(!info.exec_states.empty(), "no executing states for operation " + STR(fsm_fun_id) + "_" + STR(op_id));
   return string_represents_one_of_the_states(state_var.value, info.exec_states, one_hot_fsm_encoding);
}

static bool is_start(const sig_variation& state_var, const op_state_info& info,
                     const unsigned int ASSERT_PARAMETER(fsm_fun_id), const unsigned int ASSERT_PARAMETER(op_id),
                     bool one_hot_fsm_encoding)
{
   THROW_ASSERT(!info.start_states.empty(), "no starting states for operation " + STR(fsm_fun_id) + "_" + STR(op_id));
   return string_represents_one_of_the_states(state_var.value, info.start_states, one_hot_fsm_encoding);
}

#if HAVE_ASSERTS
static bool is_end(const sig_variation& state_var, const op_state_info& info,
                   const unsigned int ASSERT_PARAMETER(fsm_fun_id), const unsigned int ASSERT_PARAMETER(op_id),
                   bool one_hot_fsm_encoding)
{
   THROW_ASSERT(!info.end_states.empty(), "no ending states for operation " + STR(fsm_fun_id) + "_" + STR(op_id));
   return string_represents_one_of_the_states(state_var.value, info.end_states, one_hot_fsm_encoding);
}
#endif

static bool all_ones(const std::string& s)
{
   return std::all_of(s.begin(), s.end(), [](const char c) { return c == '1'; });
}

static bool var_has_value_ones(const sig_variation& v)
{
   return all_ones(v.value);
}

static bool var_is_later_or_equal(const sig_variation& v, const unsigned long long time)
{
   return v >= time;
}

vcd_trace_head::vcd_trace_head(const unsigned int op, std::string signame, const std::list<sig_variation>& fv,
                               const std::list<sig_variation>& ov, const std::list<sig_variation>& sv,
                               unsigned int init_state_id, unsigned long long clock_p,
                               const HLS_managerConstRef _HLSMgr, const ir_managerConstRef _TM,
                               const bool _one_hot_fsm_encoding)
    : state(uninitialized),
      failed(fail_none),
      one_hot_fsm_encoding(_one_hot_fsm_encoding),
      op_id(op),
      fsm_fun_id(get_fsm_fun_id(_TM, op)),
      op_timing(get_op_timing(_HLSMgr, fsm_fun_id, op)),
      op_states(get_op_state_info(_HLSMgr, fsm_fun_id, op)),
      is_phi(_TM->GetIRNode(op)->get_kind() == phi_stmt_K),
      is_in_reg(_HLSMgr->get_HLS(fsm_fun_id)
                    ->storage_value_information->is_a_storage_value(0, get_ssa_name_node_id(_TM, op), 0)),
      HLSMgr(_HLSMgr),
      TM(_TM),
      initial_state_id(init_state_id),
      fsm_vars(fv),
      fsm_ss_it(fv.cbegin()),
      fsm_end(fv.cend()),
      out_vars(ov),
      out_var_it(ov.cbegin()),
      out_var_end(ov.cend()),
      start_vars(sv),
      sp_var_it(sv.cbegin()),
      sp_var_end(sv.cend()),
      fullsigname(std::move(signame)),
      op_start_time(0),
      op_end_time(0),
      clock_period(clock_p),
      exec_times_in_current_state(0),
      consecutive_state_executions(0),
      has_been_initialized(false),
      fsm_has_a_single_state(_HLSMgr->get_HLS(fsm_fun_id)->fsm_info->getNumberOfStates(false) == 1),
      start_state_is_initial(false)
{
}

void vcd_trace_head::set_consecutive_state_executions()
{
   THROW_ASSERT((fsm_ss_it->duration == std::numeric_limits<decltype(fsm_ss_it->duration)>::max()) ||
                    (fsm_ss_it->duration + fsm_ss_it->time_stamp - op_start_time) % clock_period == 0,
                "state transition is not aligned with clock signal\n"
                "timestamp = " +
                    STR(fsm_ss_it->time_stamp) +
                    "\n"
                    "op_start_time = " +
                    STR(op_start_time) +
                    "\n"
                    "duration = " +
                    STR(fsm_ss_it->duration) +
                    "\n"
                    "clock_period = " +
                    STR(clock_period) + "\n");
   THROW_ASSERT(state == running or state == uninitialized, "");
   if(fsm_ss_it->duration == std::numeric_limits<decltype(fsm_ss_it->duration)>::max())
   {
      if(start_state_is_initial and fsm_has_a_single_state and (sp_var_it != sp_var_end))
      {
         consecutive_state_executions = (sp_var_it->time_stamp - op_start_time) / clock_period;
      }
      else
      {
         consecutive_state_executions = std::numeric_limits<decltype(consecutive_state_executions)>::max();
      }
   }
   else
   {
      consecutive_state_executions = (fsm_ss_it->duration + fsm_ss_it->time_stamp - op_start_time) / clock_period;
   }
   exec_times_in_current_state = 0;
   THROW_ASSERT(consecutive_state_executions > 0, "state execution time is 0");
}

void vcd_trace_head::unbounded_find_end_time()
{
   if(exec_times_in_current_state == consecutive_state_executions - 1)
   {
      auto fsm_es_it = fsm_ss_it;
      THROW_ASSERT(is_end(*fsm_es_it, op_states, fsm_fun_id, op_id, one_hot_fsm_encoding),
                   "unbounded operation " + STR(fsm_fun_id) + "_" + STR(op_id) + " with starting state " +
                       fsm_es_it->value + " which is not ending\n");
      THROW_ASSERT(not is_phi, "\n/--------------------------------------------------------------------\n"
                               "|  operation: " +
                                   STR(fsm_fun_id) + "_" + STR(op_id) +
                                   "\n"
                                   "|  is UNBOUNDED and is a phi\n"
                                   "\\--------------------------------------------------------------------\n");
      /*
       * if the operation is not bounded then the state machine has a
       * dummy state after the call state, where it waits for the
       * operation to complete if the operation is not completed in the
       * same cycle as the call.
       * for this reason, to find the real ending state of an unbounded
       * operation we have to skip the advance to the following state
       * until we find that the next is not in execution or it is a new
       * starting state
       */
      if(is_exec(*std::next(fsm_es_it), op_states, fsm_fun_id, op_id, one_hot_fsm_encoding) and
         not is_start(*std::next(fsm_es_it), op_states, fsm_fun_id, op_id, one_hot_fsm_encoding))
      {
         fsm_es_it++;
         if(fsm_es_it == fsm_end)
         {
            state = init_fail;
            failed = no_end_state;
            return;
         }
         THROW_ASSERT(is_end(*fsm_es_it, op_states, fsm_fun_id, op_id, one_hot_fsm_encoding),
                      "dummy state of unbounded operation " + STR(fsm_fun_id) + "_" + STR(op_id) +
                          " must be ending state\n");
      }
      op_end_time = std::next(fsm_es_it)->time_stamp;
   }
   else
   {
      THROW_ASSERT(consecutive_state_executions > 1, STR(op_id));
      op_end_time = op_start_time + clock_period;
   }
}

void vcd_trace_head::update()
{
   /* find the next starting state for an execution of this operation */
   if(has_been_initialized && !fsm_has_a_single_state)
   {
      ++fsm_ss_it;
   }
   const auto is_starting_state = [&](const sig_variation& s) {
      return is_start(s, op_states, fsm_fun_id, op_id, one_hot_fsm_encoding);
   };
   const auto fsm_prev_ss_it = fsm_ss_it;
   fsm_ss_it = std::find_if(fsm_ss_it, fsm_end, is_starting_state);
   if(fsm_ss_it == fsm_end)
   {
      fsm_ss_it = fsm_prev_ss_it;
      state = init_fail;
      failed = no_start_state;
      return;
   }
   /*
    * now we want to detect precisely when the operation starts. this may
    * not be the exact time when the fsm enters in a starting state for
    * the operation.
    * the cases when the operation starting time is different from the
    * time when fsm enters in a starting state are 2:
    * 1) for the state S_0, which is the state where the functional unit
    *    stays until the start port rises
    * 2) when the state has a self edge. in such a case multiple execution
    *    of the same operation can be performed without variation in the
    *    vcd signal present_state.
    */
   op_start_time = fsm_ss_it->time_stamp;
   start_state_is_initial = is_binary_string_repr(fsm_ss_it->value, initial_state_id, one_hot_fsm_encoding);
   if(start_state_is_initial)
   {
      /*
       * the next executing starting states for this operation is the initial
       * state. the initial state is special, because it's also the state
       * in which the FSM of the function stays when the function is not in
       * execution. so we have to look for the start port assertion
       */
      sp_var_it = std::find_if(sp_var_it, sp_var_end, var_has_value_ones);
      if(sp_var_it == sp_var_end)
      {
         state = init_fail;
         failed = function_does_not_start;
         return;
      }
      op_start_time = sp_var_it->time_stamp;
      ++sp_var_it;
   }
   THROW_ASSERT(op_start_time >= fsm_ss_it->time_stamp, "operation start time is before starting state" +
                                                            STR(op_start_time) + " " + STR(fsm_ss_it->time_stamp));
   /*
    * here we know the correct starting state and the start time of the
    * operation. we know detect the number of consecutive state executions
    */
   set_consecutive_state_executions();
   /*
    * now we found the correct starting time for the current operation.
    * but we have to analyze the output signal at the end of the operation
    * in order to perform discrepancy analysis. so we have to detect the
    * time when the current execution of this operation ends.
    */
   if(op_timing.is_bounded)
   {
      /*
       * if the operation is bounded the ending state does not matter, because
       * the end time of the operation can be calculated from start time, given
       * that we know its execution time
       */
      /* default calculation of the end time */
      if(op_timing.n_cycles == 0)
      {
         op_end_time = op_start_time + clock_period;
      }
      else
      {
         op_end_time = op_start_time + ((op_timing.n_cycles) * clock_period);
      }
   }
   else
   {
      unbounded_find_end_time();
   }
}

void vcd_trace_head::detect_new_start_end_times()
{
   op_start_time += clock_period;
   if(op_timing.is_bounded)
   {
      op_end_time += clock_period;
   }
   else
   {
      unbounded_find_end_time();
   }
}

void vcd_trace_head::advance()
{
   switch(state)
   {
      case uninitialized:
      case running:
      {
         THROW_ASSERT(state != uninitialized or not has_been_initialized, STR(op_id));
         update();
         if(state == init_fail)
         {
            return;
         }
         if(not has_been_initialized)
         {
            has_been_initialized = true;
         }
         break;
      }
      case suspended:
      {
         detect_new_start_end_times();
         if(state == init_fail)
         {
            return;
         }
         break;
      }
      case checked:
      case init_fail:
      case initialized:
      case after_discrepancy:
      case discrepancy_found:
      default:
      {
         THROW_UNREACHABLE("advance() works only on suspended, uninitialized or running");
         return;
      }
   }
   const auto& end = op_end_time;
   const auto var_later_or_equal_than_end = [&end](const sig_variation& s) { return var_is_later_or_equal(s, end); };
   out_var_it = std::find_if(out_var_it, out_var_end, var_later_or_equal_than_end);
   --out_var_it;
   state = initialized;
}
