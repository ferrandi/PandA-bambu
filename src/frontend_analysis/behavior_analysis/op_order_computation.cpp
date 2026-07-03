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
 * @file op_order_computation.cpp
 * @brief Analysis step computing a topological order of the operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "op_order_computation.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

OpOrderComputation::OpOrderComputation(const ParameterConstRef _Param, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_ORDER_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

DesignFlowStep_Status OpOrderComputation::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting order computation on Operation CFG");
   const auto cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
   std::list<OpGraph::vertex_descriptor> sorted_vertices;
   cfg.TopologicalSort(sorted_vertices);

   function_behavior->map_levels.clear();
   function_behavior->deque_levels.clear();

   unsigned int index = 0;
   for(const auto vertex : sorted_vertices)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Assigning vertex " + cfg.CGetNodeInfo(vertex).vertex_name);
      function_behavior->add_level(vertex, index++);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

#ifndef NDEBUG
   THROW_ASSERT(sorted_vertices.size() == cfg.num_vertices(),
                "Operation CFG topological sort did not return all vertices");
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
OpOrderComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(OP_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void OpOrderComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->map_levels.clear();
      function_behavior->deque_levels.clear();
   }
}
