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
 *              Copyright (C) 2014-2026 Politecnico di Milano
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
 * @file sdc_scheduling2.hpp
 * @brief New SDC scheduler
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SDC_SCHEDULING2_HPP
#define SDC_SCHEDULING2_HPP
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
   void Initialize() override;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   SDCScheduling2(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, unsigned int function_id,
                  const DesignFlowManager& design_flow_manager,
                  const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;

   static void sdc_schedule(std::map<OpGraph::vertex_descriptor, int>& vals_vertex, const hlsRef HLS,
                            const HLS_managerRef HLSMgr, unsigned function_id, const OpVertexSet& loop_operations,
                            const std::set<BBGraph::vertex_descriptor, bb_vertex_order_by_map>& loop_bbs,
                            const BBGraph& basic_block_graph, const OpGraph& filtered_op_graph,
                            const AllocationInformationConstRef allocation_information,
                            const ParameterConstRef parameters, int debug_level);
};
#endif
