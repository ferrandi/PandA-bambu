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

#ifndef VCD_TRACE_HEAD_HPP
#define VCD_TRACE_HEAD_HPP

#include <list>
#include <string>

#include "sig_variation.hpp"

// include from parser/vcd/
#include "DiscrepancyOpInfo.hpp"

CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(tree_manager);

struct vcd_trace_head
{
 public:
   vcd_trace_head(const DiscrepancyOpInfo& op_info, std::string signame, const std::list<sig_variation>& fv, const std::list<sig_variation>& ov, const std::list<sig_variation>& sv, unsigned int init_state_id, unsigned long long clock_period,
                  const HLS_managerConstRef _HLSMgr, const tree_managerConstRef _TM, const bool one_hot_fsm_encoding);

   ~vcd_trace_head() = default;

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
   const DiscrepancyOpInfo& op_info;
   const bool is_phi;
   const bool is_in_reg;
   const HLS_managerConstRef HLSMgr;
   const tree_managerConstRef TM;
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
