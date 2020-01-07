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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file bb_reachability_computation.cpp
 * @brief Analysis step computing reachability between operations
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "bb_reachability_computation.hpp"

///. includes
#include "Parameter.hpp"

/// algorithms/loops_detection includes
#include "loop.hpp"
#include "loops.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// graph includes
#include "graph.hpp"

/// tree includes
#include "tree_basic_block.hpp"

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

BBReachabilityComputation::BBReachabilityComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_REACHABILITY_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BBReachabilityComputation::~BBReachabilityComputation() = default;

void BBReachabilityComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->bb_reachability.clear();
      function_behavior->feedback_bb_reachability.clear();
   }
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BBReachabilityComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ADD_BB_ECFG_EDGES, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
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

DesignFlowStep_Status BBReachabilityComputation::InternalExec()
{
   /// The extended control flow graph of basic block
   const BBGraphConstRef ecfg = function_behavior->CGetBBGraph(FunctionBehavior::EBB);

   /// The reachability among basic blocks
   auto& bb_reachability = function_behavior->bb_reachability;
   auto& feedback_bb_reachability = function_behavior->feedback_bb_reachability;

   std::deque<vertex> container;
   boost::topological_sort(*ecfg, std::back_inserter(container));
   std::deque<vertex>::const_iterator it, it_end;
   it_end = container.end();
   for(it = container.begin(); it != it_end; ++it)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Examining basic block " + boost::lexical_cast<std::string>(ecfg->CGetBBNodeInfo(*it)->block->number));
      OutEdgeIterator eo, eo_end;
      for(boost::tie(eo, eo_end) = boost::out_edges(*it, *ecfg); eo != eo_end; eo++)
      {
         vertex previous = boost::target(*eo, *ecfg);
         bb_reachability[*it].insert(previous);
         bb_reachability[*it].insert(bb_reachability[previous].begin(), bb_reachability[previous].end());
      }
   }

   feedback_bb_reachability = bb_reachability;

   /// Get first level loops
   const LoopConstRef zero_loop = function_behavior->CGetLoops()->CGetLoop(0);
   const CustomOrderedSet<LoopConstRef>& first_level_loops = zero_loop->GetChildren();
   CustomOrderedSet<LoopConstRef>::const_iterator first_level_loop, first_level_loop_end = first_level_loops.end();
   for(first_level_loop = first_level_loops.begin(); first_level_loop != first_level_loop_end; ++first_level_loop)
   {
      CustomUnorderedSet<vertex> loop_blocks;
      (*first_level_loop)->get_recursively_bb(loop_blocks);
      CustomUnorderedSet<vertex>::const_iterator loop_block, loop_block_end = loop_blocks.end();
      for(loop_block = loop_blocks.begin(); loop_block != loop_block_end; ++loop_block)
      {
         feedback_bb_reachability[*loop_block].insert(loop_blocks.begin(), loop_blocks.end());
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
