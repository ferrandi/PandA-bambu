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
 * @file op_feedback_edges_computation.cpp
 * @brief Analysis step computing Analysis step computing feedback edges for operation control flow graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "op_feedback_edges_computation.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"

/// Graph include
#include "basic_block.hpp"
#include "operations_graph_constructor.hpp"
#include "tree_basic_block.hpp"

/// Frontend include
#include "Parameter.hpp"

/// Tree include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_manager.hpp"

op_feedback_edges_computation::op_feedback_edges_computation(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_FEEDBACK_EDGES_IDENTIFICATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

op_feedback_edges_computation::~op_feedback_edges_computation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> op_feedback_edges_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_ZEBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOP_REGIONS_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_ANALYSIS_ZEBU, SAME_FUNCTION));
#endif
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status op_feedback_edges_computation::InternalExec()
{
   const BBGraphConstRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
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
         const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(to_bb);
         vertex label_vertex = bb_node_info->statements_list.front();
         const BBNodeInfoConstRef bb_node_info_from = fbb->CGetBBNodeInfo(from_bb);
         THROW_ASSERT(bb_node_info_from->statements_list.size(), "Empty block " + boost::lexical_cast<std::string>(bb_node_info_from->block->number));
         vertex goto_vertex = bb_node_info_from->statements_list.back();
         /// add the feedback control dependence and the feedback control flow graph edges
         function_behavior->ogc->RemoveEdge(goto_vertex, label_vertex, CFG_SELECTOR);
         function_behavior->ogc->AddEdge(goto_vertex, label_vertex, FB_CFG_SELECTOR);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Transforming " + STR(fbb->CGetBBNodeInfo(from_bb)->block->number) + "->" + STR(fbb->CGetBBNodeInfo(to_bb)->block->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR((*loop)->GetId()));
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::FCFG)->WriteDot("OP_FCFG.dot");
      function_behavior->CGetOpGraph(FunctionBehavior::CFG)->WriteDot("OP_CFG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
