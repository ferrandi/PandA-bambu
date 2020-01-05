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
 *              Copyright (C) 2014-2020 Politecnico di Milano
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
 * @file sdc_scheduling.hpp
 * @brief Class definition of the sdc scheduling
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef SDC_SCHEDULING_HPP
#define SDC_SCHEDULING_HPP

/// Superclass include
#include "scheduling.hpp"

/// Utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "utility.hpp"
#include <set>

CONSTREF_FORWARD_DECL(AllocationInformation);
class bb_vertex_order_by_map;
CONSTREF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(meilp_solver);
class OpVertexSet;
REF_FORWARD_DECL(SDCSorter);

class SDCScheduling : public Scheduling
{
 protected:
   /// The operation graph used to perform scheduling
   OpGraphConstRef op_graph;

   /// The basic block cfg graph
   BBGraphConstRef basic_block_graph;

   /// The operation graph with feedback used to perform scheduling
   OpGraphConstRef feedback_op_graph;

   /// The behavioral helper
   BehavioralHelperConstRef behavioral_helper;

   /// The allocation
   AllocationInformationConstRef allocation_information;

   /// The binding
   fu_bindingRef res_binding;

   /// The clock period
   double clock_period;

   /// The margin which has to be introduced with respect to clock period
   double margin;

   /// Map operation-stage to variable index
   CustomMap<std::pair<vertex, unsigned int>, unsigned int> operation_to_varindex;

   /// The set of unbounded operations
   CustomSet<vertex> unbounded_operations;

   /// For each shared resource, the operations mapped on it
   CustomMap<unsigned int, CustomSet<vertex>> sharing_operations;

   /// Set of reachable operations (in scheduling graph)
   CustomMap<vertex, CustomSet<vertex>> full_reachability_map;

   /// The set of limited resources
   CustomSet<unsigned int> limited_resources;

#ifndef NDEBUG
   /// The set of temporary flow edges added to the op_graph
   CustomSet<EdgeDescriptor> temp_edges;
#endif

   /**
    * Execute the SDCScheduling_Algorithm::SPECULATIVE_LOOP version of the algorithm
    */
   void ExecLoop();

   /**
    * Initialize the step
    */
   void Initialize() override;

   /**
    * Add constraints to force execution in different steps of operations which cannot be chained
    * @param solver is the solver to which constraints have to be added
    * @param op_graph is the operation graph
    * @param operation is the first operation to be considered
    */
   void AddDelayConstraints(const meilp_solverRef solver, const OpGraphConstRef filtered_op_graph,
#ifndef NDEBUG
                            const OpGraphConstRef debug_filtered_op_graph,
#endif
                            const std::set<vertex, bb_vertex_order_by_map>& loop_bbs);

   /**
    * Add constraints to force consecutive execution of dependent operations
    * @param solver is the solver to which constraints have to be added
    * @param source is the first operation
    * @param target is the second operation
    * @param simultaneous tells if the two operations can be executed in the same clock cycle (chained) or not
    */
   void AddDependenceConstraint(const meilp_solverRef solver, const vertex first_operation, const vertex second_operation, const bool simultaneous) const;

   /**
    * Add constraints to force consecutive execution of different pipeline stages
    * @param solver is the solver to which constraints have to be added
    * @param operation is the pipelined operation
    */
   void AddStageConstraints(const meilp_solverRef solver, const vertex operation) const;

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /// Result of SPECULATIVE_LOOP: the list of movement to be performed (first element is the operation, second element is the old basic block, third element is the new basic block)
   /// Movements have to be performed in order
   std::list<std::vector<unsigned int>> movements_list;

   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param HLSmgr is the HLS manager
    * @param function_id is the function index of the function
    * @param design_flow_manager is the hls design flow
    * @param hls_flow_step_specialization specifies how specialize this step
    */
   SDCScheduling(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Destructor
    */
   ~SDCScheduling() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
