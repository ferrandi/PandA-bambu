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
#ifndef VCD_UTILITY_HPP
#define VCD_UTILITY_HPP
#include "hls_function_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"
#include "vcd_parser.hpp"

#include <list>
#include <string>

CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(Discrepancy);
REF_FORWARD_DECL(ir_manager);

struct vcd_trace_head;

struct DiscrepancyLog
{
   unsigned long long op_start_time;
   unsigned long long op_end_time;
   unsigned int op_id;
   unsigned int ssa_id;
   unsigned int fun_id;
   std::string op_start_state;
   std::string fu_name;
   std::string stmt_string;
   std::string c_val;
   std::string vcd_val;
   std::string fullsigname;
   uint64_t context;
   unsigned int bitsize;
   unsigned int el_idx;
   // these are valid only if the variable is a vector or a complex
   std::string::size_type first_c_bit;
   std::string::size_type c_size;
   /**
    * Valid only when the discrepancy is on an address. It is an index used in the memory
    * allocation step to retrieve the base address in HW of the memory module where an object is mapped.
    */
   unsigned int base_index;

   DiscrepancyLog(const HLS_managerConstRef HLSMgr, const vcd_trace_head& t, const uint64_t c_context,
                  std::string _c_val, const unsigned int el_idx, const std::string::size_type _first_c_bit,
                  const std::string::size_type _c_size, const unsigned int _b);
};

class vcd_utility : public HLS_step
{
 public:
   vcd_utility(const ParameterConstRef parameters, const HLS_managerRef HLSMgr,
               const DesignFlowManager& design_flow_manager);

   /* Destructor */

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;

   static bool IsOneHotFSM(unsigned int function_id, const HLS_managerRef HLSMgr);

 protected:
   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   const ir_managerRef TM;

   const DiscrepancyRef Discr;

   unsigned long long possibly_lost_address{0};

   unsigned long long mismatched_integers{0};

   bool allow_uninitialized;

   std::list<DiscrepancyLog> discr_list;

   std::list<DiscrepancyLog> soft_discr_list;

   /// The name of the present state signal
   std::string present_state_name;

   unsigned long long GetClockPeriod(const vcd_parser::vcd_trace_t& vcd_trace) const;

   void GenerateDiscrepancyTrace() const;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   bool detect_mismatch(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val);

   bool detect_regular_mismatch(const vcd_trace_head& t, const std::string& c_val, const std::string& vcd_val) const;

   bool detect_binary_float_mismatch(const std::string& c_val, const std::string& resized_vcd_val) const;

   bool detect_binary_double_mismatch(const std::string& c_val, const std::string& resized_vcd_val) const;

   bool detect_address_mismatch(const unsigned int op_id, const uint64_t c_context, const std::string& c_val,
                                const std::string& vcd_val, unsigned int& base_index);

   bool detect_fixed_address_mismatch(const unsigned int op_id, const uint64_t c_context, const std::string& c_val,
                                      const std::string& vcd_val, const unsigned int base_index) const;

   bool detect_mismatch_element(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val,
                                const unsigned int el_idx, const unsigned int vec_base_bitsize);

   bool detect_mismatch_simple(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val,
                               const unsigned int el_idx, const std::string::size_type first_c_bit,
                               const std::string::size_type c_size);

   void update_discr_list(const vcd_trace_head& t, const uint64_t c_context, const std::string& c_val,
                          const unsigned int el_idx, const std::string::size_type first_c_bit,
                          const std::string::size_type c_size, const unsigned int base_index);

   void print_failed_vcd_head(const vcd_trace_head& t, bool one_hot_encoding, const int verbosity) const;

   void print_discrepancy(const DiscrepancyLog& l, bool one_hot_encoding, const int verbosity) const;

   std::string compute_fsm_state_from_vcd_string(const std::string& vcd_state_string, bool one_hot_encoding) const;
};
#endif
