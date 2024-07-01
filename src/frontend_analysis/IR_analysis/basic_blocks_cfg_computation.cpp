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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file basic_blocks_cfg_computation.cpp
 * @brief Build basic block control flow graph data structure starting from the tree_manager.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "basic_blocks_cfg_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif
#include "operations_graph_constructor.hpp"

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis/IR_analysis
#include "operations_cfg_computation.hpp"

/// graph include
#include "graph.hpp"

/// STD include
#include <fstream>

/// STL include
#include <list>

/// parser/compiler include
#include "token_interface.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "behavioral_writer_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

BasicBlocksCfgComputation::BasicBlocksCfgComputation(const ParameterConstRef _parameters,
                                                     const application_managerRef _AppM, unsigned int _function_id,
                                                     const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BASIC_BLOCKS_CFG_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BasicBlocksCfgComputation::~BasicBlocksCfgComputation() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BasicBlocksCfgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         if(parameters->isOption(OPT_writer_language))
         {
            relationships.insert(std::make_pair(HDL_VAR_DECL_FIX, SAME_FUNCTION));
         }
         else
         {
            relationships.insert(std::make_pair(VAR_DECL_FIX, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COND_EXPR_RESTRUCTURING, SAME_FUNCTION));
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FANOUT_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
#if HAVE_ILP_BUILT
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void BasicBlocksCfgComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->bbgc->Clear();
#if HAVE_HOST_PROFILING_BUILT
      function_behavior->profiling_information->Clear();
#endif
   }
}

DesignFlowStep_Status BasicBlocksCfgComputation::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto bbgc = function_behavior->bbgc;
   const auto fd = GetPointer<const function_decl>(TM->GetTreeNode(function_id));
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointer<const statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number != BB_ENTRY && bb->number != BB_EXIT)
      {
         continue;
      }
      bbgc->add_vertex(bb);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added basic block with index " + STR(bb->number));
      if(bb->number == BB_EXIT)
      {
         const auto exit = bbgc->Cget_vertex(BB_EXIT);
         bbgc->connect_to_entry(exit);
      }
   }
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number == BB_ENTRY || bb->number == BB_EXIT)
      {
         continue;
      }
      bbgc->add_vertex(bb);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added basic block with index " + STR(bb->number));
   }
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number == BB_ENTRY || bb->number == BB_EXIT)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering connections for BB" + STR(bb->number));
      const auto current = bbgc->Cget_vertex(bb->number);
      if(bb->list_of_pred.empty() || std::count(bb->list_of_pred.begin(), bb->list_of_pred.end(), bloc::ENTRY_BLOCK_ID))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Connecting Basic block " + STR(bb->number) + " to ENTRY");
         bbgc->connect_to_entry(current);
      }
      for(const auto su : bb->list_of_succ)
      {
         if(su == bloc::EXIT_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Connecting Basic block " + STR(bb->number) + " to EXIT");
            bbgc->connect_to_exit(current);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Connecting Basic block " + STR(bb->number) + " to " + STR(su));
            bbgc->AddEdge(current, bbgc->Cget_vertex(su), CFG_SELECTOR);
            // Considering label
            if(su == bb->true_edge)
            {
               bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(su), CFG_SELECTOR, T_COND);
            }
            if(su == bb->false_edge)
            {
               bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(su), CFG_SELECTOR, F_COND);
            }
         }
      }
      const auto& statements = bb->CGetStmtList();
      if(!statements.empty())
      {
         const auto& last = statements.back();
         /// switch statements
         if(last->get_kind() == gimple_switch_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->BB" + STR(bb->number) + " ends with a switch");
            // Map between gimple_label and index of basic block
            std::map<tree_nodeRef, unsigned int> label_to_bb;
            for(const auto su : bb->list_of_succ)
            {
               THROW_ASSERT(sl->list_of_bloc.at(su)->CGetStmtList().size(), "Empty Basic Block");
               const auto first = sl->list_of_bloc.at(su)->CGetStmtList().front();
               THROW_ASSERT(first->get_kind() == gimple_label_K, "First operation of BB" + STR(su) + " is a " +
                                                                     first->get_kind_text() + ": " + first->ToString());
               const auto le = GetPointerS<const gimple_label>(first);
               label_to_bb[le->op] = su;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Gimple label of BB" + STR(su) + " is " + STR(le->op->index));
            }
            const auto se = GetPointer<const gimple_switch>(last);
            THROW_ASSERT(se->op1, "case_label_exprs not found");
            const auto tv = GetPointer<const tree_vec>(se->op1);

            for(const auto& op : tv->list_of_op)
            {
               const auto cl = GetPointer<const case_label_expr>(op);
               THROW_ASSERT(label_to_bb.count(cl->got),
                            "There is not corresponding case_label_exprs with index " + STR(cl->got->index));
               if(cl->default_flag)
               {
                  bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(label_to_bb[cl->got]), CFG_SELECTOR, default_COND);
               }
               else
               {
                  bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(label_to_bb[cl->got]), CFG_SELECTOR, op->index);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         /// computed goto
         else if(last->get_kind() == gimple_goto_K && bb->list_of_succ.size() > 1)
         {
            // Map between gimple_label and index of basic block
            for(const auto su : bb->list_of_succ)
            {
               bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(su), CFG_SELECTOR, su);
            }
         }
         /// multi-way if
         else if(last->get_kind() == gimple_multi_way_if_K)
         {
            const auto gmwi = GetPointerS<const gimple_multi_way_if>(last);
            for(const auto& cond : gmwi->list_of_cond)
            {
               bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(cond.second), CFG_SELECTOR,
                                      cond.first ? cond.first->index : default_COND);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered connections for BB" + STR(bb->number));
   }
   const auto exit = bbgc->Cget_vertex(BB_EXIT);
   const auto fcfg = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   BOOST_FOREACH(vertex v, boost::vertices(*fcfg))
   {
      if(boost::out_degree(v, *fcfg) == 0 && v != exit)
      {
         bbgc->AddEdge(v, exit, CFG_SELECTOR);
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB)->WriteDot("BB_CFG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
