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
 * @file split_return.cpp
 * @brief Simple transformations that remove almost empty basic blocks having a single phi and a return statement.
 *        The transformation remove this BB by propagating back the return statement.
 *
 * @author Hamidreza Hanafi <hamidreza.hanafi@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "split_return.hpp"

#include "phi_opt.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"

/// Parameter include
#include "Parameter.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
#include "hls_step.hpp"
#endif

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_set.hpp"

/// tree includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

SplitReturn::SplitReturn(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SPLIT_RETURN, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

SplitReturn::~SplitReturn() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> SplitReturn::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         if(parameters->isOption(OPT_bitvalue_ipa) and parameters->getOption<bool>(OPT_bitvalue_ipa))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE_IPA, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(RANGE_ANALYSIS, WHOLE_APPLICATION));
         }
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool SplitReturn::HasToBeExecuted() const
{
   if(parameters->IsParameter("disable-SplitReturn") && parameters->GetParameter<int>("disable-SplitReturn") == 1)
   {
      return false;
   }
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
   if(parameters->isOption(OPT_scheduling_algorithm) and parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
      return GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch && FunctionFrontendFlowStep::HasToBeExecuted();
   else
#endif
      return FunctionFrontendFlowStep::HasToBeExecuted();
}

static void create_return_and_fix_cfg(const tree_managerRef TM, std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& gimple_return_schema, statement_list* sl, blocRef pred_block, blocRef bb_block, int DEBUG_PARAMETER(debug_level))
{
   auto bb_index = bb_block->number;
   const auto gimple_node_id = TM->new_tree_node_id();
   TM->create_tree_node(gimple_node_id, gimple_return_K, gimple_return_schema);
   gimple_return_schema.clear();
   if(pred_block->list_of_succ.size() == 1)
   {
      pred_block->PushBack(TM->GetTreeReindex(gimple_node_id));
      pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
      pred_block->list_of_succ.push_back(bb_block->list_of_succ.front());
   }
   else
   {
      const auto new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(new_basic_block_index) + " as new successor of BB" + STR(pred_block->number));
      auto new_block = blocRef(new bloc(new_basic_block_index));
      sl->list_of_bloc[new_basic_block_index] = new_block;
      new_block->loop_id = bb_block->loop_id;
      new_block->SetSSAUsesComputed();
      new_block->schedule = bb_block->schedule;
      new_block->PushBack(TM->GetTreeReindex(gimple_node_id));
      /// Add predecessor as pred basic block
      new_block->list_of_pred.push_back(pred_block->number);
      /// Add successor as succ basic block
      new_block->list_of_succ.push_back(bb_block->list_of_succ.front());
      /// Fix successor of predecessor
      pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
      pred_block->list_of_succ.push_back(new_basic_block_index);
      if(pred_block->true_edge == bb_index)
         pred_block->true_edge = new_basic_block_index;
      if(pred_block->false_edge == bb_index)
         pred_block->false_edge = new_basic_block_index;
      /// Fix gimple_multi_way_if
      if(pred_block->CGetStmtList().size())
      {
         auto pred_last_stmt = GET_NODE(pred_block->CGetStmtList().back());
         if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            auto gmwi = GetPointer<gimple_multi_way_if>(pred_last_stmt);
            for(auto& cond : gmwi->list_of_cond)
            {
               if(cond.second == bb_index)
               {
                  cond.second = new_basic_block_index;
               }
            }
         }
      }
   }
}

DesignFlowStep_Status SplitReturn::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   bool isChanged = false;
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_return_schema;
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator iit, iit_end = list_of_bloc.end();
   std::list<unsigned int> to_be_erase;
   for(iit = list_of_bloc.begin(); iit != iit_end; ++iit)
   {
      blocRef& bb_block = iit->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Considering BB" + STR(bb_block->number) + " " + STR(bb_block->CGetPhiList().size()) + " " + STR(bb_block->CGetStmtList().size()));
      if(bb_block->list_of_pred.size() > 1 && bb_block->CGetPhiList().size() == 1 && bb_block->CGetStmtList().size() == 1)
      {
         auto bb_index = bb_block->number;
         const auto gp = GetPointer<const gimple_phi>(GET_NODE(bb_block->CGetPhiList().front()));
         const auto gr = GetPointer<const gimple_return>(GET_NODE(bb_block->CGetStmtList().front()));
         if(gr && gr->op && GET_INDEX_NODE(gp->res) == GET_INDEX_NODE(gr->op))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- There is a split return possible at BB" + STR(bb_index));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Create return statement based of def edges");
            for(auto def_edge : gp->CGetDefEdgesList())
            {
               unsigned int op_node_nid = GET_INDEX_NODE(def_edge.first);
               gimple_return_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
               gimple_return_schema[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(gr->scpe));
               gimple_return_schema[TOK(TOK_OP)] = STR(op_node_nid);
               auto pred_block = sl->list_of_bloc[def_edge.second];
               create_return_and_fix_cfg(TM, gimple_return_schema, sl, pred_block, bb_block, debug_level);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
            bb_block->RemovePhi(bb_block->CGetPhiList().front());
            bb_block->RemoveStmt(bb_block->CGetStmtList().front());
            to_be_erase.push_back(bb_index);
            isChanged = true;
         }
         else if(gr && gp->virtual_flag && GET_INDEX_NODE(gp->res) == GET_INDEX_NODE(gr->memuse))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- There is a split return possible at BB" + STR(bb_index));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Create return statement based of def edges");
            for(auto def_edge : gp->CGetDefEdgesList())
            {
               unsigned int op_node_nid = GET_INDEX_NODE(def_edge.first);
               gimple_return_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
               gimple_return_schema[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(gr->scpe));
               if(gr->op)
                  gimple_return_schema[TOK(TOK_OP)] = STR(GET_INDEX_NODE(gr->op));
               gimple_return_schema[TOK(TOK_MEMUSE)] = STR(op_node_nid);
               auto pred_block = sl->list_of_bloc[def_edge.second];
               create_return_and_fix_cfg(TM, gimple_return_schema, sl, pred_block, bb_block, debug_level);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
            bb_block->RemovePhi(bb_block->CGetPhiList().front());
            bb_block->RemoveStmt(bb_block->CGetStmtList().front());
            to_be_erase.push_back(bb_index);
            isChanged = true;
         }
      }
      else if(bb_block->list_of_pred.size() > 1 && bb_block->CGetPhiList().size() == 0 && bb_block->CGetStmtList().size() == 1)
      {
         auto bb_index = bb_block->number;
         const auto gr = GetPointer<const gimple_return>(GET_NODE(bb_block->CGetStmtList().front()));
         if(gr)
         {
            for(auto pred_block_index : bb_block->list_of_pred)
            {
               auto pred_block = sl->list_of_bloc[pred_block_index];
               gimple_return_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
               gimple_return_schema[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(gr->scpe));
               if(gr->op)
                  gimple_return_schema[TOK(TOK_OP)] = STR(GET_INDEX_NODE(gr->op));
               create_return_and_fix_cfg(TM, gimple_return_schema, sl, pred_block, bb_block, debug_level);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
            bb_block->RemoveStmt(bb_block->CGetStmtList().front());
            to_be_erase.push_back(bb_index);
            isChanged = true;
         }
      }
   }
   for(auto bb_index : to_be_erase)
      list_of_bloc.erase(bb_index);

   if(isChanged)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
}
