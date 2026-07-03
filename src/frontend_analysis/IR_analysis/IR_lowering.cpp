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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file IR_lowering.cpp
 * @brief Decompose some complex statements into a set of simpler operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "IR_lowering.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step_factory.hpp"
#include "exceptions.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "kcm_constmul.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#define FMT_HEADER_ONLY 1

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include "config_HAVE_ASSERTS.hpp"

IR_lowering::IR_lowering(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id,
                         const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, IR_LOWERING, _design_flow_manager, Param),
      constdiv_lowering_mode("auto"),
      constdiv_dsp_scale_k(1.0),
      constmultdiv_score("weighted"),
      constmultdiv_decision_metric("auto"),
      constmultdiv_lut_cost_model("auto"),
      constmultdiv_w_latency(1.0),
      constmultdiv_w_area(1.0),
      constmul_enable(true),
      constmul_balance_tree(true),
      constmul_balance_tree_min_terms(4),
      constmul_max_terms(16),
      constmul_max_depth(8),
      constmul_try_factor_forms(true),
      constmul_enable_small_factor_chains(true),
      constmul_dsp_scale_k(1.0),
      constmul_kcm_enable(true),
      constmul_kcm_alpha(0),
      constmul_kcm_sum_strategy("tree"),
      constmul_kcm_merge_table_add(false),
      constmul_kcm_cost_model("heuristic"),
      constdivmul_params_initialized(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
IR_lowering::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(EXTRACT_COND_OP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_VDEF, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

void IR_lowering::Initialize()
{
   TM = AppM->get_ir_manager();
   ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
}

void IR_lowering::ComputeRelationships(DesignFlowStepSet& relationship,
                                       const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

bool IR_lowering::reached_max_transformation_limit(const ir_nodeRef& stmt)
{
   if(stmt)
   {
      const auto ga = GetPointer<const assign_stmt>(stmt);
      if(ga)
      {
         const auto op0 = ga->op0;
         const auto op1 = ga->op1;
         if(op1->get_kind() == select_node_K)
         {
            return false;
         }
         if(op0->get_kind() == ssa_node_K and op1->get_kind() == variable_val_node_K)
         {
            return false;
         }
      }
   }
   if(not AppM->ApplyNewTransformation())
   {
      return true;
   }
   return false;
}
