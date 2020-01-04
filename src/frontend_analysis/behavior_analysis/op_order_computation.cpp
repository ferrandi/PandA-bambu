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
 * @file op_order_computation.cpp
 * @brief Analysis step computing a topological order of the operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Header include
#include "op_order_computation.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "level_constructor.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

OpOrderComputation::OpOrderComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_ORDER_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

OpOrderComputation::~OpOrderComputation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> OpOrderComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
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

DesignFlowStep_Status OpOrderComputation::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting order computation on Operation CFG");
   const OpGraphConstRef cfg = function_behavior->CGetOpGraph(FunctionBehavior::ECFG);
   graph::vertex_iterator v, v_end;
   std::list<vertex> to_visit;
   unsigned int index;
   /// Mark for visit in different functions.
   std::map<graph::vertex_descriptor, bool> MARK;

   /// MARK initialization
   for(boost::tie(v, v_end) = boost::vertices(*cfg); v != v_end; ++v)
   {
      MARK[*v] = false;
   }
   // Vertex list to be visited
   to_visit.push_front(function_behavior->ogc->CgetIndex(ENTRY));
   index = 0;
   while(!to_visit.empty())
   {
      vertex actual = to_visit.front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking vertex " + GET_NAME(cfg, actual) + " : " + boost::lexical_cast<std::string>(index));
      to_visit.pop_front();
      MARK[actual] = true;
      function_behavior->lm->add(actual, index++);
      graph::out_edge_iterator o, o_end;
      graph::in_edge_iterator i, i_end;
      vertex then = NULL_VERTEX;
      /// Examining successors
      for(boost::tie(o, o_end) = boost::out_edges(actual, *cfg); o != o_end; o++)
      {
         bool toadd = true;
         vertex next = boost::target(*o, *cfg);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successor " + GET_NAME(cfg, next));
         /// Checking if all successor's predecessors have been examinated
         for(boost::tie(i, i_end) = boost::in_edges(next, *cfg); i != i_end; i++)
         {
            if(!MARK[boost::source(*i, *cfg)])
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not adding because of predecessor " + GET_NAME(cfg, boost::source(*i, *cfg)));
               toadd = false;
               break;
            }
         }
         if(toadd)
         {
            if(Cget_edge_info<OpEdgeInfo>(*o, *cfg) && CFG_TRUE_CHECK(cfg, *o))
               then = next;
            /// Vertex can be added to list
            if(next != then)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding next " + GET_NAME(cfg, next));
               to_visit.push_front(next);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      if(then)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding then " + GET_NAME(cfg, then));
         to_visit.push_front(then);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   /// Checking if EXIT has been added
   const std::map<vertex, unsigned int>& map_levels = function_behavior->get_map_levels();
   if(map_levels.find(function_behavior->ogc->CgetIndex(EXIT)) == map_levels.end())
   {
      function_behavior->lm->add(function_behavior->ogc->CgetIndex(EXIT), index++);
   }
#ifndef NDEBUG
   const auto exit = cfg->CGetOpGraphInfo()->exit_vertex;
   for(boost::tie(v, v_end) = boost::vertices(*cfg); v != v_end; v++)
   {
      if(*v == exit)
         continue;
      THROW_ASSERT(MARK[*v], "Operation " + GET_NAME(cfg, *v) + " not analyzed");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}
