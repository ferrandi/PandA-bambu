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
 * @file AddArtificialCallFlowEdges.cpp
 * @brief Analysis step which adds flow edges to builtin bambu time functions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "AddArtificialCallFlowEdges.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "basic_block.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// boost include
#include <boost/range/adaptor/reversed.hpp>

/// tree include
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AddArtificialCallFlowEdges::AddArtificialCallFlowEdges(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_ARTIFICIAL_CALL_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

AddArtificialCallFlowEdges::~AddArtificialCallFlowEdges() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AddArtificialCallFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_REACHABILITY_COMPUTATION, SAME_FUNCTION));
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

DesignFlowStep_Status AddArtificialCallFlowEdges::InternalExec()
{
   /// The control flow graph of basic blocks
   const BBGraphConstRef bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);

   /// The control flow graph of operation
   const OpGraphConstRef op_graph = function_behavior->CGetOpGraph(FunctionBehavior::CFG);

   const auto BH = function_behavior->CGetBehavioralHelper();
   /// Adding operation to empty return
   VertexIterator v, v_end;
   for(boost::tie(v, v_end) = boost::vertices(*bb_graph); v != v_end; ++v)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb_graph->CGetBBNodeInfo(*v)->block->number));
      const auto& statements_list = bb_graph->CGetBBNodeInfo(*v)->statements_list;
      for(const auto stmt : statements_list)
      {
         const OpNodeInfoConstRef node_info = op_graph->CGetOpNodeInfo(stmt);
         const unsigned int st_tn_id = node_info->GetNodeId();
         if(not BH->CanBeMoved(st_tn_id))
         {
            bool previous = true;
            for(const auto other_stmt : statements_list)
            {
               if(other_stmt == stmt)
               {
                  previous = false;
                  continue;
               }
               if(previous)
                  function_behavior->ogc->AddEdge(other_stmt, stmt, FLG_SELECTOR);
               else
                  function_behavior->ogc->AddEdge(stmt, other_stmt, FLG_SELECTOR);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(bb_graph->CGetBBNodeInfo(*v)->block->number));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::FLG)->WriteDot("OP_FL.dot");
      function_behavior->CGetOpGraph(FunctionBehavior::FFLSAODG)->WriteDot("OP_FFLSAODG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
