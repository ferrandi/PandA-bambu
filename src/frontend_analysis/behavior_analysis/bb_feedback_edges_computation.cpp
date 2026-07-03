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
 * @file bb_feedback_edges_computation.cpp
 * @brief Analysis step computing feedback edges of basic block control flow graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "bb_feedback_edges_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "ir_basic_block.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "string_manipulation.hpp"

bb_feedback_edges_computation::bb_feedback_edges_computation(const ParameterConstRef _parameters,
                                                             const application_managerRef _AppM,
                                                             unsigned int _function_id,
                                                             const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_FEEDBACK_EDGES_IDENTIFICATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
bb_feedback_edges_computation::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(LOOPS_COMPUTATION, SAME_FUNCTION));
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

DesignFlowStep_Status bb_feedback_edges_computation::InternalExec()
{
   const auto fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   /// then consider loops
   for(const auto& loop : function_behavior->getConstLoops()->getList())
   {
      if(loop->getLoopId() == 0)
      {
         continue;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR(loop->getLoopId()));
      for(auto [from_bb, to_bb] : loop->getBackEdges())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Transforming " + STR(fbb.CGetNodeInfo(from_bb).block->number) + "->" +
                            STR(fbb.CGetNodeInfo(to_bb).block->number));
         function_behavior->bbgc->RemoveEdge(from_bb, to_bb, CFG_SELECTOR);
         function_behavior->bbgc->AddEdge(from_bb, to_bb, FB_CFG_SELECTOR);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR(loop->getLoopId()));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto dot_path = function_behavior->GetDotPath();
      function_behavior->GetBBGraph(FunctionBehavior::FBB).writeDot(dot_path / "BB_FCFG.dot");
      function_behavior->GetBBGraph(FunctionBehavior::BB).writeDot(dot_path / "BB_CFG.dot");
   }
   /// FIXME: check to identify irreducible loops, since Loop::isReducible does not work
   try
   {
      const auto cfg_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
      std::list<BBGraph::vertex_descriptor> vertices;
      cfg_graph.TopologicalSort(vertices);
   }
   catch(...)
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB).writeDot(function_behavior->GetDotPath() / "Error.dot");
      THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, function_behavior->CGetBehavioralHelper()->GetFunctionName() +
                                                 " cannot be synthesized: irreducible loops are not yet supported");
   }
   return DesignFlowStep_Status::SUCCESS;
}
