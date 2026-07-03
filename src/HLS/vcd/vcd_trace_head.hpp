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

#ifndef VCD_TRACE_HEAD_HPP
#define VCD_TRACE_HEAD_HPP

#include "refcount.hpp"
#include "sig_variation.hpp"

#include <list>
#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(ir_manager);

struct op_timing_info
{
   bool is_bounded{};
   unsigned long n_cycles{};
};

struct op_state_info
{
   std::vector<unsigned int> start_states;
   std::vector<unsigned int> exec_states;
   std::vector<unsigned int> end_states;
};

struct vcd_trace_head
{
 public:
   vcd_trace_head(const unsigned int op_id, std::string signame, const std::list<sig_variation>& fv,
                  const std::list<sig_variation>& ov, const std::list<sig_variation>& sv, unsigned int init_state_id,
                  unsigned long long clock_period, const HLS_managerConstRef _HLSMgr, const ir_managerConstRef _TM,
                  const bool one_hot_fsm_encoding);

   void advance();

   bool starts_after(unsigned long long t) const
   {
      return op_start_time > t;
   }

   bool ends_after(unsigned long long t) const
   {
      return op_end_time > t;
   }

   bool more_executions_in_this_hw_state() const
   {
      return exec_times_in_current_state < consecutive_state_executions;
   }

   enum vcd_head_state
   {
      uninitialized,
      init_fail,
      initialized,
      after_discrepancy,
      discrepancy_found,
      checked,
      suspended,
      running
   };
   enum vcd_head_state state;

   enum vcd_head_failure
   {
      no_start_state,
      no_end_state,
      function_does_not_start,
      fail_none
   };
   enum vcd_head_failure failed;

 protected:
   void set_consecutive_state_executions();

   void unbounded_find_end_time();

   void update();

   void detect_new_start_end_times();

   const bool one_hot_fsm_encoding;

 public:
   const unsigned int op_id;
   const unsigned int fsm_fun_id;
   const op_timing_info op_timing;
   const op_state_info op_states;
   const bool is_phi;
   const bool is_in_reg;
   const HLS_managerConstRef HLSMgr;
   const ir_managerConstRef TM;
   const unsigned int initial_state_id;
   const std::list<sig_variation>& fsm_vars;
   std::list<sig_variation>::const_iterator fsm_ss_it; // start state iterator
   std::list<sig_variation>::const_iterator fsm_end;
   const std::list<sig_variation>& out_vars;
   std::list<sig_variation>::const_iterator out_var_it;
   std::list<sig_variation>::const_iterator out_var_end;
   const std::list<sig_variation>& start_vars;
   std::list<sig_variation>::const_iterator sp_var_it;
   std::list<sig_variation>::const_iterator sp_var_end;
   const std::string fullsigname;
   unsigned long long op_start_time;
   unsigned long long op_end_time;
   const unsigned long long clock_period;
   unsigned long long exec_times_in_current_state;
   unsigned long long consecutive_state_executions;
   bool has_been_initialized;
   bool fsm_has_a_single_state;
   bool start_state_is_initial;
};

#endif
