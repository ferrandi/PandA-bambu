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
 * @file loops_computation.cpp
 * @brief Analysis step performing loops computation.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "loops_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "ir_basic_block.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "string_manipulation.hpp"

#include <iosfwd>

loops_computation::loops_computation(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                     unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LOOPS_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
loops_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
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

void loops_computation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->loops = LoopsRef();
   }
}

DesignFlowStep_Status loops_computation::InternalExec()
{
   auto fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);

   const auto entry_vertex = fbb.CGetGraphInfo().entry_vertex;
   THROW_ASSERT(function_behavior->dominators, "Dominators has to be computed!");
   function_behavior->loops = LoopsRef(new Loops(fbb, entry_vertex, *function_behavior->dominators));
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->getConstLoops()->writeDot(function_behavior->GetDotPath() / "LF.dot");
   }
   const auto& loops = function_behavior->getConstLoops()->getList();
   for(const auto& loop : loops)
   {
      /// FIXME: zero loop
      if(loop->getLoopId() == 0)
      {
         continue;
      }
      const auto blocks = loop->getBlocks();
      for(const auto bb_v : blocks)
      {
         auto& bb_node_info = fbb.GetNodeInfo(bb_v);
         bb_node_info.loop_id = loop->getLoopId();
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       "  Basic block " + std::to_string(bb_node_info.block->number));
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Number of reducible loops: " + std::to_string(function_behavior->getConstLoops()->numLoops()));
   return DesignFlowStep_Status::SUCCESS;
}
