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
 *              Copyright (C) 2014-2024 Politecnico di Milano
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
 * @file sdc_scheduling2.hpp
 * @brief New SDC scheduler
 *
  @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SDC_SCHEDULING2_HPP
#define SDC_SCHEDULING2_HPP

/// Superclass include
#include "sdc_scheduling_base.hpp"

#include "basic_block.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "utility.hpp"
#include <set>

CONSTREF_FORWARD_DECL(AllocationInformation);
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(memory);
REF_FORWARD_DECL(fu_binding);

class SDCScheduling2 : public SDCScheduling_base
{
   /**
    * Initialize the step
    */
   void Initialize() override;

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
   ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param HLSmgr is the HLS manager
    * @param function_id is the function index of the function
    * @param design_flow_manager is the hls design flow
    * @param hls_flow_step_specialization specifies how specialize this step
    */
   SDCScheduling2(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, unsigned int function_id,
                  const DesignFlowManagerConstRef design_flow_manager,
                  const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Destructor
    */
   ~SDCScheduling2() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

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

   static void sdc_schedule(std::map<vertex, int>& vals_vertex, const hlsRef HLS, const HLS_managerRef HLSMgr,
                            unsigned function_id, const OpVertexSet& loop_operations,
                            const std::set<vertex, bb_vertex_order_by_map>& loop_bbs,
                            const BBGraphConstRef basic_block_graph, const OpGraphConstRef filtered_op_graph,
                            const AllocationInformationConstRef allocation_information,
                            const ParameterConstRef parameters, int debug_level);
};
#endif
