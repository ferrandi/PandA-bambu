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
 * @file add_op_phi_flow_edges.cpp
 * @brief Analysis step which adds flow edges from the computation of the condition(s) of gimple_cond and gimple_multi_way_if to phi
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Header include
#include "add_op_phi_flow_edges.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// tree includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

AddOpPhiFlowEdges::AddOpPhiFlowEdges(const application_managerRef _AppM, const unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_OP_PHI_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

AddOpPhiFlowEdges::~AddOpPhiFlowEdges() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AddOpPhiFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
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

void AddOpPhiFlowEdges::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphConstRef flg = function_behavior->CGetOpGraph(FunctionBehavior::FLG);
      if(boost::num_vertices(*flg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*flg); edge != edge_end; edge++)
         {
            if((GET_TYPE(flg, boost::target(*edge, *flg)) & TYPE_PHI) != 0)
               function_behavior->ogc->RemoveSelector(*edge, FLG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status AddOpPhiFlowEdges::InternalExec()
{
   /// The control flow graph of operation
   const OpGraphConstRef fcfg = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);

   /// The control flow graph of basic block
   const auto bb_fcfg = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const auto dom_tree = function_behavior->CGetBBGraph(FunctionBehavior::DOM_TREE);

   const auto bb_index_map = dom_tree->CGetBBGraphInfo()->bb_index_map;

   VertexIterator operation, operation_end;
   for(boost::tie(operation, operation_end) = boost::vertices(*fcfg); operation != operation_end; operation++)
   {
      if((GET_TYPE(fcfg, *operation) & TYPE_PHI) != 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + GET_NAME(fcfg, *operation));
         const auto bb_index = fcfg->CGetOpNodeInfo(*operation)->bb_index;
         const auto bb_vertex = bb_index_map.find(bb_index)->second;
         /// Check if the phi is not in the target of a feedback edge
         bool found_feedback_edge = false;
         InEdgeIterator ie, ie_end;
         for(boost::tie(ie, ie_end) = boost::in_edges(bb_vertex, *bb_fcfg); ie != ie_end; ie++)
         {
            if(bb_fcfg->GetSelector(*ie) & FB_CFG_SELECTOR)
            {
               found_feedback_edge = true;
               break;
            }
         }
         if(found_feedback_edge)
            continue;
         boost::tie(ie, ie_end) = boost::in_edges(bb_vertex, *dom_tree);
         const auto dominator = boost::source(*ie, *dom_tree);
         const auto dominator_statements = dom_tree->CGetBBNodeInfo(dominator)->block->CGetStmtList();
         THROW_ASSERT(dominator_statements.size(), "BB" + STR(dom_tree->CGetBBNodeInfo(dominator)->block->number) + " is empty");
         const auto last_statement = GET_NODE(dominator_statements.back());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Last statement is " + last_statement->ToString());
         if(last_statement->get_kind() == gimple_cond_K)
         {
            const auto gc = GetPointer<const gimple_cond>(last_statement);
            if(GET_NODE(gc->op0)->get_kind() == integer_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + GET_NAME(fcfg, *operation));
               continue;
            }
            THROW_ASSERT(GET_NODE(gc->op0)->get_kind() == ssa_name_K, "Operand of gimple_cond is " + gc->op0->ToString());
            const auto ssa = GetPointer<const ssa_name>(GET_NODE(gc->op0));
            const auto def_stmt_node = ssa->CGetDefStmt();
            if(GetPointer<gimple_nop>(GET_NODE(def_stmt_node)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + GET_NAME(fcfg, *operation));
               continue;
            }
            const auto def_stmt_index = def_stmt_node->index;
            THROW_ASSERT(fcfg->CGetOpGraphInfo()->tree_node_to_operation.find(def_stmt_index) != fcfg->CGetOpGraphInfo()->tree_node_to_operation.end(), "Vertex corresponding to " + ssa->CGetDefStmt()->ToString() + " not found");
            const auto def_stmt_vertex = fcfg->CGetOpGraphInfo()->tree_node_to_operation.find(def_stmt_index)->second;
            function_behavior->ogc->AddEdge(def_stmt_vertex, *operation, FLG_SELECTOR);
         }
         else if(last_statement->get_kind() == gimple_multi_way_if_K)
         {
            /// Approximation: we consider all conditions
            const auto gmwi = GetPointer<const gimple_multi_way_if>(last_statement);
            for(const auto& cond : gmwi->list_of_cond)
            {
               const auto cond_bb = bb_index_map.find(cond.second)->second;
               if(cond.first and ((cond.second == bb_index) or function_behavior->CheckBBReachability(cond_bb, bb_vertex)))
               {
                  if(GET_NODE(cond.first)->get_kind() == integer_cst_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + GET_NAME(fcfg, *operation));
                     continue;
                  }
                  THROW_ASSERT(GET_NODE(cond.first)->get_kind() == ssa_name_K, "Operand of gimple_multi_way_if is " + cond.first->ToString());
                  const auto ssa = GetPointer<const ssa_name>(GET_NODE(cond.first));
                  THROW_ASSERT(ssa, GET_NODE(cond.first)->get_kind_text() + " " + STR(cond.first));
                  const auto def_stmt_index = ssa->CGetDefStmt()->index;
                  const auto def_stmt_vertex = fcfg->CGetOpGraphInfo()->tree_node_to_operation.find(def_stmt_index)->second;
                  function_behavior->ogc->AddEdge(def_stmt_vertex, *operation, FLG_SELECTOR);
               }
            }
         }
         else if(last_statement->get_kind() == gimple_switch_K)
         {
            const auto gs = GetPointer<const gimple_switch>(last_statement);
            THROW_ASSERT(GET_NODE(gs->op0)->get_kind() == ssa_name_K, "Operand of gimple_switch is " + gs->op0->ToString());
            const auto ssa = GetPointer<const ssa_name>(GET_NODE(gs->op0));
            if(not ssa->var or GET_NODE(ssa->var)->get_kind() != parm_decl_K)
            {
               const auto def_stmt_index = ssa->CGetDefStmt()->index;
               THROW_ASSERT(fcfg->CGetOpGraphInfo()->tree_node_to_operation.find(def_stmt_index) != fcfg->CGetOpGraphInfo()->tree_node_to_operation.end(), "Vertex corresponding to " + ssa->CGetDefStmt()->ToString() + " not found");
               const auto def_stmt_vertex = fcfg->CGetOpGraphInfo()->tree_node_to_operation.find(def_stmt_index)->second;
               function_behavior->ogc->AddEdge(def_stmt_vertex, *operation, FLG_SELECTOR);
            }
         }
         else
         {
            THROW_UNREACHABLE("gimple phi is dominated by " + last_statement->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + GET_NAME(fcfg, *operation));
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
