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
 *              Copyright (c) 2015-2018 Politecnico di Milano
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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#ifndef DISCREPANCY_HPP
#define DISCREPANCY_HPP

#include "Parameter.hpp"

// include from HLS/vcd/
#include "UnfoldedCallGraph.hpp"

// includes from parser/discrepancy/
#include "DiscrepancyOpInfo.hpp"

// includes from tree/
#include "tree_node.hpp"

// includes from utility/
#include "refcount.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

struct Discrepancy
{
   /// Reference to the unfolded call graph used for the discrepancy analysis
   UnfoldedCallGraph DiscrepancyCallGraph;

   /// UnfoldedVertexDescriptor of the root of the DiscrepancyCallGraph
   UnfoldedVertexDescriptor unfolded_root_v;

   /**
    * A map to store the vcd signals to be dumped. The key is the scope, and
    * the mapped set contains all the signals to be dumped for that scope
    */
   std::unordered_map<std::string, std::unordered_set<std::string>> selected_vcd_signals;

   /**
    * A map to store the name of the output signal of every operation.
    * The key is the operation id, the mapped value is the signal name
    */
   std::unordered_map<unsigned int, std::string> opid_to_outsignal;

   /// Map every vertex of the UnfoldedCallGraph to a scope in HW
   std::unordered_map<UnfoldedVertexDescriptor, std::string> unfolded_v_to_scope;

   /// Map every fun_id to the set of HW scopes of the functional modules
   std::unordered_map<unsigned int, std::set<std::string>> f_id_to_scope;

   /**
    * Set of tree nodes representing the ssa_name to be skipped in discrepancy analysis
    */
   TreeNodeSet ssa_to_skip;

   /**
    * Set of tree nodes. SSA in this set must not be checked in the
    * discrepancy analysis if they are also marked as addresses.
    */
   TreeNodeSet ssa_to_skip_if_address;

   /**
    * Set of tree nodes representing the ssa_name to be treated as addresses in discrepancy analysis
    */
   TreeNodeSet address_ssa;

   /**
    * Map a discrepancy info to the list of pairs representing the
    * corresponding assignments in C. The firts element of every pair is the
    * context, the second element of the pair (the string) is the binary
    * string representation assigned by the operation identified by the
    * primary key.
    */
   std::map<DiscrepancyOpInfo, std::list<std::pair<uint64_t, std::string>>> c_op_trace;

   /**
    * This contains the control flow traces gathered from software execution.
    * The primary key is is a function id, the secondary key is a software
    * call context id, the mapped value is a list of BB identifiers traversed
    * during the execution of the call in that context.
    */
   std::unordered_map<unsigned int, std::unordered_map<uint64_t, std::list<unsigned int>>> c_control_flow_trace;

   /**
    * Address map used for address discrepancy analysis. The primary key is
    * the context, the secondary key is the variable id, the mapped value is
    * the base address
    */
   std::unordered_map<uint64_t, std::unordered_map<unsigned int, uint64_t>> c_addr_map;

   /**
    * Maps every call context in the discrepancy trace to the corresponding
    * scope in the generated HW.
    */
   std::unordered_map<uint64_t, std::string> context_to_scope;

   /**
    * Maps every function ID to a set of states that must always be checked
    * by the hardware discrepancy control flow checker.
    */
   std::unordered_map<unsigned int, std::unordered_set<unsigned int>> fu_id_to_states_to_check;

   /**
    * Maps every function ID to a set of states that must be checked
    * by the hardware discrepancy control flow checker if the execution flow
    * comes from a feedback_edges
    */
   std::unordered_map<unsigned int, std::unordered_set<unsigned int>> fu_id_to_feedback_states_to_check;

   /**
    * Maps every function ID to a set EdgeDescriptors. Each edge
    * represents an edge along which the epp counter have to be reset.
    * These edges are StateTransition edges of the StateTransitionGraph of
    * the associated function.
    */
   std::unordered_map<unsigned int, std::unordered_set<EdgeDescriptor>> fu_id_to_reset_edges;

   /**
    * Maps every function ID to the bitsize of the epp trace that is
    * necessary for checking the control flow of that function.
    * This bitsize is does not depend on the instance of the function, since
    * the epp edge increments are always the same and this bitsize is the
    * number of bits necessary to represent them
    */
   std::unordered_map<unsigned int, size_t> fu_id_to_epp_trace_bitsize;

   /**
    * This set contains the ids of functions for which the control flow
    * hardware discrepancy analyssi is not necessary.
    * If the FSM of a function is linear, i.e. it does not contain branches
    * or loops, the control flow cannot diverge during its execution, so it
    * is not necessary to check it with control flow discrepancy analysis.
    */
   std::unordered_set<unsigned int> fu_id_control_flow_skip;

   /// name of the file that contains the c trace to parse
   std::string c_trace_filename;

   unsigned long long n_total_operations{0};

   unsigned long long n_checked_operations{0};

   Discrepancy(void) : DiscrepancyCallGraph(GraphInfoRef(new GraphInfo()))
   {
   }
};

typedef refcount<Discrepancy> DiscrepancyRef;
typedef refcount<const Discrepancy> DiscrepancyConstRef;
#endif
