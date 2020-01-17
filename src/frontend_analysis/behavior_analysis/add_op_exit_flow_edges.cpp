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
 * @file add_op_exit_flow_edges.hpp
 * @brief Analysis step which adds flow edges for scheduling to operation graphs
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "add_op_exit_flow_edges.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "basic_block.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// tree include
#include "behavioral_helper.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AddOpExitFlowEdges::AddOpExitFlowEdges(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_OP_EXIT_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

AddOpExitFlowEdges::~AddOpExitFlowEdges() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AddOpExitFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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

void AddOpExitFlowEdges::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphConstRef flg = function_behavior->CGetOpGraph(FunctionBehavior::FLG);
      if(boost::num_vertices(*flg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*flg); edge != edge_end; edge++)
         {
            if((GET_TYPE(flg, boost::target(*edge, *flg)) & TYPE_LAST_OP) != 0)
               function_behavior->ogc->RemoveSelector(*edge, FLG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status AddOpExitFlowEdges::InternalExec()
{
   /// The control flow graph of operation
   const OpGraphConstRef fcfg = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);

   /// The control flow graph of basic block
   const auto basic_block_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);

   /// Adding operation to empty return
   VertexIterator v, v_end;
   for(boost::tie(v, v_end) = boost::vertices(*fcfg); v != v_end; ++v)
   {
      if((GET_TYPE(fcfg, *v) & TYPE_LAST_OP) != 0)
      {
/// NOTE: the old version of this code added a flow edge from all the operations which reach last op and not only from the operations of the same basic block: was there any actual reason to do in this way?
#if 1
         for(const auto operation : basic_block_graph->CGetBBNodeInfo(basic_block_graph->CGetBBGraphInfo()->bb_index_map.find(fcfg->CGetOpNodeInfo(*v)->bb_index)->second)->statements_list)
         {
            const auto reachability = function_behavior->CheckReachability(operation, *v);
            if(reachability and ((GET_TYPE(fcfg, operation) & TYPE_LAST_OP) == 0))
            {
               function_behavior->ogc->AddEdge(operation, *v, FLG_SELECTOR);
            }
         }
#else
         VertexIterator u, u_end;
         for(boost::tie(u, u_end) = boost::vertices(*fcfg); u != u_end; u++)
         {
            const bool reachability = function_behavior->CheckReachability(*u, *v);
            if(reachability and ((GET_TYPE(fcfg, *u) & TYPE_LAST_OP) == 0))
            {
               function_behavior->ogc->AddEdge(*u, *v, FLG_SELECTOR);
            }
         }
#endif
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::FLG)->WriteDot("OP_FL.dot");
      function_behavior->CGetOpGraph(FunctionBehavior::FFLSAODG)->WriteDot("OP_FFLSAODG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
