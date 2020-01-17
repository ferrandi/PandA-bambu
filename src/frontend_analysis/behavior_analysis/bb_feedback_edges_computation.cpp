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
 * @file bb_feedback_edges_computation.cpp
 * @brief Analysis step computing feedback edges of basic block control flow graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "bb_feedback_edges_computation.hpp"

#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "operations_graph_constructor.hpp"

#include "Parameter.hpp"

/// tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"

bb_feedback_edges_computation::bb_feedback_edges_computation(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_FEEDBACK_EDGES_IDENTIFICATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

bb_feedback_edges_computation::~bb_feedback_edges_computation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> bb_feedback_edges_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_COMPUTATION, SAME_FUNCTION));
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
   const BBGraphRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const BehavioralHelperConstRef helper = function_behavior->CGetBehavioralHelper();
   /// then consider loops
   std::list<LoopConstRef> loops = function_behavior->CGetLoops()->GetList();
   std::list<LoopConstRef>::const_iterator loop_end = loops.end();
   for(std::list<LoopConstRef>::const_iterator loop = loops.begin(); loop != loop_end; ++loop)
   {
      if((*loop)->GetId() == 0)
         continue;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR((*loop)->GetId()));
      for(auto sp_back_edge : (*loop)->get_sp_back_edges())
      {
         vertex from_bb = sp_back_edge.first;
         vertex to_bb = sp_back_edge.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Transforming " + STR(fbb->CGetBBNodeInfo(from_bb)->block->number) + "->" + STR(fbb->CGetBBNodeInfo(to_bb)->block->number));
         function_behavior->bbgc->RemoveEdge(from_bb, to_bb, CFG_SELECTOR);
         function_behavior->bbgc->AddEdge(from_bb, to_bb, FB_CFG_SELECTOR);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR((*loop)->GetId()));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_FCFG.dot");
      function_behavior->GetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_CFG.dot");
   }
   /// FIXME: check to identify irreducible loops, since Loop::IsReducible does not work
   try
   {
      const BBGraphRef cfg_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
      std::list<vertex> vertices;
      cfg_graph->TopologicalSort(vertices);
   }
   catch(const char* msg)
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB)->WriteDot("Error.dot");
      THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, helper->get_function_name() + " cannot be synthesized: irreducible loops are not yet supported");
   }
   catch(const std::string& msg)
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB)->WriteDot("Error.dot");
      THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, helper->get_function_name() + " cannot be synthesized: irreducible loops are not yet supported");
   }
   catch(const std::exception& ex)
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB)->WriteDot("Error.dot");
      THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, helper->get_function_name() + " cannot be synthesized: irreducible loops are not yet supported");
   }
   catch(...)
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB)->WriteDot("Error.dot");
      THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, helper->get_function_name() + " cannot be synthesized: irreducible loops are not yet supported");
   }
   return DesignFlowStep_Status::SUCCESS;
}
