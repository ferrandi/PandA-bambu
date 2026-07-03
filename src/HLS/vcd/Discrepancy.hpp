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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#ifndef DISCREPANCY_HPP
#define DISCREPANCY_HPP

#include "Parameter.hpp"
#include "UnfoldedCallGraph.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "ir_node.hpp"
#include "refcount.hpp"
#include "vcd_parser.hpp"

#include <string>

REF_FORWARD_DECL(structural_manager);

struct CallSitesInfo
{
   /// Maps every function to the calls it performs
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>> fu_id_to_call_ids;
   /// Maps every id of a call site to the id of the called function
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>> call_id_to_called_id;
   /// Set of indirect calls
   CustomUnorderedSet<unsigned int> indirect_calls;
   /// Set of taken addresses
   CustomUnorderedSet<unsigned int> taken_addresses;
};

struct HWDiscrepancyInfo
{
   /**
    * Maps every function ID to a set of states that must always be checked
    * by the hardware discrepancy control flow checker.
    */
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>> fu_id_to_states_to_check;

   /**
    * Maps every function ID to a set of states that must be checked
    * by the hardware discrepancy control flow checker if the execution flow
    * comes from a feedback_edges
    */
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>> fu_id_to_feedback_states_to_check;

   /**
    * Maps every function ID to a set EdgeDescriptors. Each edge
    * represents an edge along which the epp counter have to be reset.
    * These edges are StateTransition edges of the StateTransitionGraph of
    * the associated function.
    */
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<UnfoldedCallGraph::edge_descriptor>> fu_id_to_reset_edges;

   /**
    * Maps every function ID to the bitsize of the epp trace that is
    * necessary for checking the control flow of that function.
    * This bitsize is does not depend on the instance of the function, since
    * the epp edge increments are always the same and this bitsize is the
    * number of bits necessary to represent them
    */
   CustomUnorderedMap<unsigned int, size_t> fu_id_to_epp_trace_bitsize;

   CustomUnorderedMap<unsigned int, size_t> fu_id_to_max_epp_path_val;

   /**
    * This set contains the ids of functions for which the control flow
    * hardware discrepancy analyssi is not necessary.
    * If the FSM of a function is linear, i.e. it does not contain branches
    * or loops, the control flow cannot diverge during its execution, so it
    * is not necessary to check it with control flow discrepancy analysis.
    */
   CustomUnorderedSet<unsigned int> fu_id_control_flow_skip;
};

struct Discrepancy
{
   /// Reference to a struct holding info on the call sites
   CallSitesInfo call_sites_info;

   /// Reference to a struct holding info on the control flow traces for hw discrepancy analysis
   HWDiscrepancyInfo hw_discrepancy_info;

   /// Reference to the unfolded call graph used for the discrepancy analysis
   UnfoldedCallGraph DiscrepancyCallGraph;

   /// UnfoldedVertexDescriptor of the root of the DiscrepancyCallGraph
   UnfoldedCallGraph::vertex_descriptor unfolded_root_v{UnfoldedCallGraph::null_vertex()};

   /**
    * A map to store the vcd signals to be dumped. The key is the scope, and
    * the mapped set contains all the signals to be dumped for that scope
    */
   vcd_parser::vcd_filter_t selected_vcd_signals;

   /**
    * A map to store the name of the output signal of every operation.
    * The key is the operation id, the mapped value is the signal name
    */
   CustomUnorderedMap<unsigned int, std::string> opid_to_outsignal;

   /// Map every vertex of the UnfoldedCallGraph to a scope in HW
   CustomUnorderedMap<UnfoldedCallGraph::vertex_descriptor, std::string> unfolded_v_to_scope;

   /// Map every fun_id to the set of HW scopes of the functional modules
   CustomUnorderedMap<unsigned int, CustomOrderedSet<std::string>> f_id_to_scope;

   /**
    * Set of IR nodes representing the ssa_node to be skipped in discrepancy analysis
    */
   IRNodeSet ssa_to_skip;

   /**
    * Set of IR nodes. SSA in this set must not be checked in the
    * discrepancy analysis if they are also marked as addresses.
    */
   IRNodeSet ssa_to_skip_if_address;

   /**
    * Set of IR nodes representing the ssa_node to be treated as addresses in discrepancy analysis
    */
   IRNodeSet address_ssa;

   /**
    * Map a discrepancy info to the list of pairs representing the
    * corresponding assignments in C. The firts element of every pair is the
    * context, the second element of the pair (the string) is the binary
    * string representation assigned by the operation identified by the
    * primary key.
    */
   std::map<unsigned int, std::list<std::pair<uint64_t, std::string>>> c_op_trace;

   /**
    * This contains the control flow traces gathered from software execution.
    * The primary key is is a function id, the secondary key is a software
    * call context id, the mapped value is a list of BB identifiers traversed
    * during the execution of the call in that context.
    */
   CustomUnorderedMap<unsigned int, std::map<uint64_t, std::list<unsigned int>>> c_control_flow_trace;

   /**
    * Address map used for address discrepancy analysis. The primary key is
    * the context, the secondary key is the variable id, the mapped value is
    * the base address
    */
   CustomUnorderedMap<uint64_t, CustomUnorderedMapStable<unsigned int, uint64_t>> c_addr_map;

   /**
    * Maps every call context in the discrepancy trace to the corresponding
    * scope in the generated HW.
    */
   CustomUnorderedMap<uint64_t, std::string> context_to_scope;

   /// name of the file that contains the c trace to parse
   std::string c_trace_filename;

   unsigned long long n_total_operations{0};

   unsigned long long n_checked_operations{0};

   Discrepancy() : call_sites_info(), hw_discrepancy_info(), DiscrepancyCallGraph()
   {
   }

   void clear()
   {
      call_sites_info = CallSitesInfo();
      DiscrepancyCallGraph = UnfoldedCallGraph();
      unfolded_root_v = {};
      selected_vcd_signals.clear();
      opid_to_outsignal.clear();
      unfolded_v_to_scope.clear();
      f_id_to_scope.clear();
      c_op_trace.clear();
      c_control_flow_trace.clear();
      c_addr_map.clear();
      context_to_scope.clear();
      c_trace_filename.clear();
      n_total_operations = 0;
      n_checked_operations = 0;
   }
};

using DiscrepancyRef = refcount<Discrepancy>;
using DiscrepancyConstRef = refcount<const Discrepancy>;
#endif
