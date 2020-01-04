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
 * @file bb_order_computation.cpp
 * @brief Analysis step computing a topological order of the basic_block.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "bb_order_computation.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "level_constructor.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"

BBOrderComputation::BBOrderComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_ORDER_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BBOrderComputation::~BBOrderComputation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BBOrderComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
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

void BBOrderComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->bb_map_levels.clear();
      function_behavior->bb_deque_levels.clear();
   }
}

DesignFlowStep_Status BBOrderComputation::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting order computation on function " + function_behavior->CGetBehavioralHelper()->get_function_name());
   const BBGraphConstRef ebb = function_behavior->CGetBBGraph(FunctionBehavior::EBB);
   VertexIterator bb_v, bb_v_end;
   /// Mark for visit in different functions.
   std::list<vertex> to_visit;
   unsigned int index;
   std::map<graph::vertex_descriptor, bool> MARK;

   /// MARK initialization
   for(boost::tie(bb_v, bb_v_end) = boost::vertices(*ebb); bb_v != bb_v_end; ++bb_v)
   {
      MARK[*bb_v] = false;
   }
   /// Vertex list to be visited
   to_visit.push_front(ebb->CGetBBGraphInfo()->entry_vertex);
   index = 0;
   while(!to_visit.empty())
   {
      vertex actual = to_visit.front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking vertex BB" + boost::lexical_cast<std::string>(ebb->CGetBBNodeInfo(actual)->block->number));
      to_visit.pop_front();
      MARK[actual] = true;
      function_behavior->bb_lm->add(actual, index++);
      graph::out_edge_iterator o, o_end;
      graph::in_edge_iterator i, i_end;
      vertex then = NULL_VERTEX;
      /// Examining successors
      for(boost::tie(o, o_end) = boost::out_edges(actual, *ebb); o != o_end; o++)
      {
         bool toadd = true;
         vertex next = boost::target(*o, *ebb);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking successor vertex BB" + boost::lexical_cast<std::string>(ebb->CGetBBNodeInfo(next)->block->number));
         /// Checking if all successor's predecessors have been examinated
         for(boost::tie(i, i_end) = boost::in_edges(next, *ebb); i != i_end; i++)
         {
            if(!MARK[boost::source(*i, *ebb)])
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking its predecessor BB" + boost::lexical_cast<std::string>(ebb->CGetBBNodeInfo(boost::source(*i, *ebb))->block->number) + " - Not marked");
               toadd = false;
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking its predecessor BB" + boost::lexical_cast<std::string>(ebb->CGetBBNodeInfo(boost::source(*i, *ebb))->block->number) + " - Marked");
         }
         if(toadd)
         {
            if(ebb->CGetBBEdgeInfo(*o)->cfg_edge_T())
               then = next;
            /// Vertex can be added to list
            if(next != then)
            {
               to_visit.push_front(next);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      if(then)
      {
         to_visit.push_front(then);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   /// Checking if EXIT has been added
   const std::map<vertex, unsigned int>& bb_map_levels = function_behavior->get_bb_map_levels();
   if(bb_map_levels.find(ebb->CGetBBGraphInfo()->exit_vertex) == bb_map_levels.end())
   {
      function_behavior->bb_lm->add(ebb->CGetBBGraphInfo()->exit_vertex, index++);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}
