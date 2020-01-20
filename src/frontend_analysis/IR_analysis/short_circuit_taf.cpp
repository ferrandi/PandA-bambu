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
 * @file short_circuit_taf.cpp
 * @brief Analysis step rebuilding a short circuit in a single gimple_cond with the condition in three address form.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "short_circuit_taf.hpp"

#include "phi_opt.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
/// HLS include
#include "hls_step.hpp"
#endif

/// Parameter include
#include "Parameter.hpp"

/// design flow manager include
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls_manager.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_set.hpp"
#include <utility>
#include <vector>

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

short_circuit_taf::short_circuit_taf(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SHORT_CIRCUIT_TAF, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

short_circuit_taf::~short_circuit_taf() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> short_circuit_taf::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UPDATE_SCHEDULE, SAME_FUNCTION));
         }
#endif
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         /// We can check if single_write_memory is true only after technology was loaded
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         if(design_flow_manager.lock()->GetStatus(technology_flow_signature) == DesignFlowStep_Status::EMPTY)
         {
            if(GetPointer<const HLS_manager>(AppM) and not GetPointer<const HLS_manager>(AppM)->IsSingleWriteMemory())
            {
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CLEAN_VIRTUAL_PHI, SAME_FUNCTION));
            }
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
               break;
            }
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::UNCHANGED:
            case DesignFlowStep_Status::UNEXECUTED:
            case DesignFlowStep_Status::UNNECESSARY:
            {
               break;
            }
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::NONEXISTENT:
            default:
               THROW_UNREACHABLE("");
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(INTERFACE_INFER, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void short_circuit_taf::Initialize()
{
}

bool short_circuit_taf::check_phis(unsigned int curr_bb, std::map<unsigned int, blocRef>& list_of_bloc)
{
   for(const auto& phi : list_of_bloc[curr_bb]->CGetPhiList())
   {
      auto* cb_phi = GetPointer<gimple_phi>(GET_NODE(phi));
      if(cb_phi->virtual_flag)
         return false;
   }
   return true;
}

DesignFlowStep_Status short_circuit_taf::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it, it_end = list_of_bloc.end();

   /// compute merging candidates
   CustomUnorderedSet<unsigned int> merging_candidates;
   for(it = list_of_bloc.begin(); it != it_end; ++it)
   {
      if(it->first == bloc::ENTRY_BLOCK_ID || it->first == bloc::EXIT_BLOCK_ID)
         continue;
      unsigned int n_pred_bb = 0;
      if(it->second->list_of_pred.size() <= 1)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(it->first));
      for(auto const pred : it->second->list_of_pred)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing predecessor BB" + STR(pred));
         if(pred != bloc::ENTRY_BLOCK_ID and it->first != pred and list_of_bloc[pred]->CGetStmtList().size() and GET_NODE(list_of_bloc[pred]->CGetStmtList().back())->get_kind() == gimple_cond_K)
         {
            ++n_pred_bb;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ok");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
         }
      }
      if(n_pred_bb > 1 && check_phis(it->first, list_of_bloc))
      {
         merging_candidates.insert(it->first);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added candidate BB" + STR(it->first));
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped BB" + STR(it->first));
      }
   }
   if(!merging_candidates.empty())
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Merging candidate number " + boost::lexical_cast<std::string>(merging_candidates.size()));
   else
      return DesignFlowStep_Status::UNCHANGED;

   /// find the first to merge
   unsigned int bb1 = static_cast<unsigned int>(-1), bb2 = static_cast<unsigned int>(-1), merging_candidate = 0;
   bool bb1_true = false;
   bool bb2_true = false;
   bool mergeable_pair_found;
   bool something_change = false;

   int merge_n = 0;
   do
   {
      mergeable_pair_found = false;
      CustomUnorderedSet<unsigned int>::const_iterator it_mc_end = merging_candidates.end();
      for(CustomUnorderedSet<unsigned int>::const_iterator it_mc = merging_candidates.begin(); !mergeable_pair_found && it_mc != it_mc_end; ++it_mc)
      {
         merging_candidate = *it_mc;
         mergeable_pair_found = check_merging_candidate(bb1, bb2, merging_candidate, bb1_true, bb2_true, list_of_bloc);
      }
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         break;
      }
#endif
      if(mergeable_pair_found)
      {
#ifndef NDEBUG
         AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
         if(create_gimple_cond(bb1, bb2, bb1_true, list_of_bloc, bb2_true, merging_candidate))
         {
            something_change = true;
            restructure_CFG(bb1, bb2, merging_candidate, list_of_bloc);
            merging_candidates.erase(bb1);
            if(!check_merging_candidate(bb1, bb2, merging_candidate, bb1_true, bb2_true, list_of_bloc))
               merging_candidates.erase(merging_candidate);
            if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               WriteBBGraphDot("BB_After_" + GetName() + "_merge_" + STR(merge_n) + ".dot");
            merge_n++;
         }
         else /// cond expr not completely collapsed
            merging_candidates.erase(merging_candidate);
      }
   } while(mergeable_pair_found);
   function_behavior->UpdateBBVersion();
   if(something_change)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
}

bool short_circuit_taf::check_merging_candidate(unsigned int& bb1, unsigned int& bb2, unsigned int merging_candidate, bool& bb1_true, bool& bb2_true, std::map<unsigned int, blocRef>& list_of_bloc)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking merging candidate BB" + STR(merging_candidate));
   bool mergeable_pair_found = false;
   /// let bb1 the upper if
   THROW_ASSERT(list_of_bloc.find(merging_candidate) != list_of_bloc.end(), "merging_candidate is not included in list_of_bloc");
   const std::vector<unsigned int>::const_iterator it_pred_end = list_of_bloc[merging_candidate]->list_of_pred.end();
   for(std::vector<unsigned int>::const_iterator it_bb1_pred = list_of_bloc[merging_candidate]->list_of_pred.begin(); !mergeable_pair_found && it_pred_end != it_bb1_pred; ++it_bb1_pred)
   {
      bb1 = *it_bb1_pred;
      if(bb1 == bloc::ENTRY_BLOCK_ID || bb1 == merging_candidate)
         continue;
      if(list_of_bloc[bb1]->CGetStmtList().empty())
         continue;
      if(GET_NODE(list_of_bloc[bb1]->CGetStmtList().back())->get_kind() != gimple_cond_K)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining merging candidate predecessor BB" + STR(bb1));
      THROW_ASSERT(list_of_bloc[bb1]->true_edge > 0, "bb1 has to be an if statement " + boost::lexical_cast<std::string>(bb1) + " " + boost::lexical_cast<std::string>(merging_candidate));
      if(list_of_bloc[bb1]->true_edge == merging_candidate)
         bb1_true = true;
      else
         bb1_true = false;
      /// let search bb2, the lower if
      for(std::vector<unsigned int>::const_iterator it_bb2_pred = list_of_bloc[merging_candidate]->list_of_pred.begin(); !mergeable_pair_found && it_pred_end != it_bb2_pred; ++it_bb2_pred)
      {
         bb2 = *it_bb2_pred;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining merging candidate nested predecessor " + boost::lexical_cast<std::string>(bb2));
         if(bb2 == bloc::ENTRY_BLOCK_ID || bb2 == merging_candidate)
            continue;
         if(list_of_bloc[bb2]->list_of_pred.size() > 1)
            continue;
         if(list_of_bloc[bb2]->CGetStmtList().size() != 1)
            continue;
         if(list_of_bloc[bb2]->CGetPhiList().size() != 0)
            continue;
         if(GET_NODE(list_of_bloc[bb2]->CGetStmtList().back())->get_kind() != gimple_cond_K)
            continue;
         THROW_ASSERT(list_of_bloc[bb2]->true_edge > 0, "bb2 has to be an if statement " + boost::lexical_cast<std::string>(bb2) + " " + boost::lexical_cast<std::string>(merging_candidate));
         // This check is needed for empty while loop with short circuit (e. g. 20000314-1.c)
         if(list_of_bloc[bb2]->true_edge == bb1 || list_of_bloc[bb2]->false_edge == bb1)
            continue;
         if(bb1_true && list_of_bloc[bb1]->false_edge == bb2)
         {
            if(list_of_bloc[bb2]->true_edge == merging_candidate)
               bb2_true = true;
            else
               bb2_true = false;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pair found: " << boost::lexical_cast<std::string>(bb1) << " and " << boost::lexical_cast<std::string>(bb2));
            mergeable_pair_found = true;
            if(bb2_true)
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bb1 T " + boost::lexical_cast<std::string>(bb1) + " bb2 T " + boost::lexical_cast<std::string>(bb2) + " MC " + boost::lexical_cast<std::string>(merging_candidate));
            else
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bb1 T " + boost::lexical_cast<std::string>(bb1) + " bb2 F " + boost::lexical_cast<std::string>(bb2) + " MC " + boost::lexical_cast<std::string>(merging_candidate));
         }
         else if(!bb1_true && list_of_bloc[bb1]->true_edge == bb2)
         {
            if(list_of_bloc[bb2]->true_edge == merging_candidate)
               bb2_true = true;
            else
               bb2_true = false;
            mergeable_pair_found = true;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pair found: " << boost::lexical_cast<std::string>(bb1) << " and " << boost::lexical_cast<std::string>(bb2));
            if(bb2_true)
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bb1 F " + boost::lexical_cast<std::string>(bb1) + " bb2 T " + boost::lexical_cast<std::string>(bb2) + " MC " + boost::lexical_cast<std::string>(merging_candidate));
            else
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bb1 F " + boost::lexical_cast<std::string>(bb1) + " bb2 F " + boost::lexical_cast<std::string>(bb2) + " MC " + boost::lexical_cast<std::string>(merging_candidate));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined mergin candidate predecessor BB" + STR(bb1));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked merging candidate");
   return mergeable_pair_found;
}

bool short_circuit_taf::create_gimple_cond(unsigned int bb1, unsigned int bb2, bool bb1_true, std::map<unsigned int, blocRef>& list_of_bloc, bool or_type, unsigned int merging_candidate)
{
   const tree_managerRef TM = AppM->get_tree_manager();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating new cond expr: " + boost::lexical_cast<std::string>(bb1) + " is the first basic block, " + boost::lexical_cast<std::string>(bb2) + " is the second basic block");
   /// If there are more than one statements in the basic block containing cond2, then do not merge conditions (in case speculation step should manage the code motion)
   if(list_of_bloc[bb2]->CGetStmtList().size() != 1)
      return false;
   const auto list_of_stmt_cond2 = list_of_bloc[bb2]->CGetStmtList();

   /// identify the first gimple_cond
   const auto& list_of_stmt_cond1 = list_of_bloc[bb1]->CGetStmtList();
   THROW_ASSERT(GET_NODE(list_of_stmt_cond1.back())->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   tree_nodeRef cond_statement = list_of_stmt_cond1.back();
   list_of_bloc[bb1]->RemoveStmt(cond_statement);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First gimple cond is " + STR(cond_statement));
   auto* ce1 = GetPointer<gimple_cond>(GET_NODE(cond_statement));
   unsigned int cond1_index = GET_INDEX_NODE(ce1->op0);
   const auto type_node = tree_helper::CGetType(GET_NODE(ce1->op0));
   const tree_manipulationConstRef tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters));
   unsigned int type_index = tree_helper::is_bool(TM, type_node->index) ? type_node->index : tree_man->create_boolean_type()->index;
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   /// create the ssa_var representing the condition for bb1
   unsigned int ssa1_vers = TM->get_next_vers();
   unsigned int ssa1_node_nid = TM->new_tree_node_id();
   IR_schema[TOK(TOK_TYPE)] = STR(type_index);
   IR_schema[TOK(TOK_VERS)] = STR(ssa1_vers);
   IR_schema[TOK(TOK_VOLATILE)] = STR(false);
   IR_schema[TOK(TOK_VIRTUAL)] = STR(false);
   TM->create_tree_node(ssa1_node_nid, ssa_name_K, IR_schema);
   IR_schema.clear();
   tree_nodeRef ssa1_cond_node = TM->GetTreeReindex(ssa1_node_nid);

   /// create the assignment between condition for bb1 and the new ssa var
   unsigned int cond1_gimple_stmt_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(ssa1_node_nid);
   IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(cond1_index);
   TM->create_tree_node(cond1_gimple_stmt_id, gimple_assign_K, IR_schema);
   IR_schema.clear();
   tree_nodeRef cond1_created_stmt = TM->GetTreeReindex(cond1_gimple_stmt_id);
   /// and then add to the bb1 statement list
   list_of_bloc[bb1]->PushBack(cond1_created_stmt);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created statement in BB" + STR(bb1) + " - " + STR(cond1_created_stmt));
   cond1_index = ssa1_node_nid;

   /// fix merging_candidate phis
   if(list_of_bloc[merging_candidate]->CGetPhiList().size())
   {
      for(const auto& phi : list_of_bloc[merging_candidate]->CGetPhiList())
      {
         auto* mc_phi = GetPointer<gimple_phi>(GET_NODE(phi));
         std::pair<tree_nodeRef, unsigned int> def_edge_to_be_removed(tree_nodeRef(), 0);
         std::pair<tree_nodeRef, unsigned int> def_edge_to_be_updated(tree_nodeRef(), 0);
         for(const auto& def_edge : mc_phi->CGetDefEdgesList())
         {
            if(def_edge.second == bb1)
               def_edge_to_be_removed = def_edge;
            else if(def_edge.second == bb2)
               def_edge_to_be_updated = def_edge;
         }
         THROW_ASSERT(def_edge_to_be_removed.first != tree_nodeRef(), "unexpected condition");
         THROW_ASSERT(def_edge_to_be_updated.first != tree_nodeRef(), "unexpected condition");
         unsigned int op1 = GET_INDEX_NODE(def_edge_to_be_removed.first);
         unsigned int op2 = GET_INDEX_NODE(def_edge_to_be_updated.first);
         if(!bb1_true)
            std::swap(op1, op2);

         unsigned int res_type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(mc_phi->res));

         /// create the ssa_var representing the result of the cond_expr
         unsigned int ssa_vers = TM->get_next_vers();
         unsigned int ssa_node_nid = TM->new_tree_node_id();
         IR_schema[TOK(TOK_TYPE)] = STR(res_type_index);
         IR_schema[TOK(TOK_VERS)] = STR(ssa_vers);
         IR_schema[TOK(TOK_VOLATILE)] = STR(false);
         IR_schema[TOK(TOK_VIRTUAL)] = STR(false);
         TM->create_tree_node(ssa_node_nid, ssa_name_K, IR_schema);
         IR_schema.clear();
         tree_nodeRef ssa_cond_node = TM->GetTreeReindex(ssa_node_nid);
         unsigned int cond_expr_id = TM->new_tree_node_id();
         IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(res_type_index);
         IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(cond1_index);
         IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(op1);
         IR_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(op2);
         TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, res_type_index) ? vec_cond_expr_K : cond_expr_K), IR_schema);
         IR_schema.clear();
         /// second, create the gimple assignment
         unsigned int gimple_stmt_id = TM->new_tree_node_id();
         IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(ssa_node_nid);
         IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(cond_expr_id);
         IR_schema[TOK(TOK_ORIG)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(phi));
         TM->create_tree_node(gimple_stmt_id, gimple_assign_K, IR_schema);
         IR_schema.clear();
         tree_nodeRef created_stmt = TM->GetTreeReindex(gimple_stmt_id);

         /// and then add to the statement list
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created new assignment: " + STR(created_stmt));
         list_of_bloc[bb1]->PushBack(created_stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
         mc_phi->ReplaceDefEdge(TM, def_edge_to_be_updated, gimple_phi::DefEdge(ssa_cond_node, def_edge_to_be_updated.second));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
         mc_phi->RemoveDefEdge(TM, def_edge_to_be_removed);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
      }
   }

   if((bb1_true && !or_type) || (!bb1_true && or_type))
   {
      /// cond1 has to be negate
      /// create the ssa_var representing the negated condition
      unsigned int ncond_ssa_vers = TM->get_next_vers();
      unsigned int ncond_ssa_node_nid = TM->new_tree_node_id();
      IR_schema[TOK(TOK_TYPE)] = STR(type_index);
      IR_schema[TOK(TOK_VERS)] = STR(ncond_ssa_vers);
      IR_schema[TOK(TOK_VOLATILE)] = STR(false);
      IR_schema[TOK(TOK_VIRTUAL)] = STR(false);
      TM->create_tree_node(ncond_ssa_node_nid, ssa_name_K, IR_schema);
      IR_schema.clear();
      tree_nodeRef ncond_ssa_cond_node = TM->GetTreeReindex(ncond_ssa_node_nid);

      /// create !cond1
      IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
      IR_schema[TOK(TOK_OP)] = boost::lexical_cast<std::string>(cond1_index);
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      cond1_index = TM->new_tree_node_id();
      TM->create_tree_node(cond1_index, truth_not_expr_K, IR_schema);
      IR_schema.clear();

      unsigned int ncond_gimple_stmt_id = TM->new_tree_node_id();
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(ncond_ssa_node_nid);
      IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(cond1_index);
      TM->create_tree_node(ncond_gimple_stmt_id, gimple_assign_K, IR_schema);
      IR_schema.clear();
      tree_nodeRef created_stmt = TM->GetTreeReindex(ncond_gimple_stmt_id);
      /// and then add to the bb1 statement list
      list_of_bloc[bb1]->PushBack(created_stmt);
      cond1_index = ncond_ssa_node_nid;
   }
   /// identify the second gimple_cond
   THROW_ASSERT(list_of_bloc[bb2]->CGetPhiList().size() == 0, "not expected phi nodes");

   THROW_ASSERT(GET_NODE(list_of_stmt_cond2.front())->get_kind() == gimple_cond_K, "a gimple_cond is expected");

   const auto second_stmt = list_of_stmt_cond2.front();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second gimple cond is " + STR(second_stmt));
   auto* ce2 = GetPointer<gimple_cond>(GET_NODE(second_stmt));

   unsigned int cond2_index = GET_INDEX_NODE(ce2->op0);
   // const auto type_node2 = tree_helper::CGetType(GET_NODE(ce2->op0));
   // THROW_ASSERT(type_node->get_kind() == boolean_type_K and type_node2->get_kind() == boolean_type_K, "something of unexpected is happened:"
   //                                                                                                   " type_node: " +
   //                                                                                                   STR(type_node) + " is " + type_node->get_kind_text() + " type_node2: " + STR(type_node2) + " is " + type_node2->get_kind_text());
   // unsigned int type_index2;
   // tree_helper::get_type_node(GET_NODE(ce2->op0), type_index2);
   // the following condition cannot be guaranteed
   // THROW_ASSERT(type_index == type_index2, "Different types " + STR(TM->CGetTreeNode(type_index)) + " vs " + STR(TM->CGetTreeNode(type_index2)) + " in " + ce1->ToString() + " and " + ce2->ToString());
   /// create the ssa_var representing the condition for bb2
   unsigned int ssa2_vers = TM->get_next_vers();
   unsigned int ssa2_node_nid = TM->new_tree_node_id();
   IR_schema[TOK(TOK_TYPE)] = STR(type_index);
   IR_schema[TOK(TOK_VERS)] = STR(ssa2_vers);
   IR_schema[TOK(TOK_VOLATILE)] = STR(false);
   IR_schema[TOK(TOK_VIRTUAL)] = STR(false);
   TM->create_tree_node(ssa2_node_nid, ssa_name_K, IR_schema);
   IR_schema.clear();
   tree_nodeRef ssa2_cond_node = TM->GetTreeReindex(ssa2_node_nid);

   /// create the assignment between condition for bb2 and the new ssa var
   unsigned int cond2_gimple_stmt_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(ssa2_node_nid);
   IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(cond2_index);
   TM->create_tree_node(cond2_gimple_stmt_id, gimple_assign_K, IR_schema);
   IR_schema.clear();
   tree_nodeRef cond2_created_stmt = TM->GetTreeReindex(cond2_gimple_stmt_id);
   /// and then add to the bb1 statement list
   list_of_bloc[bb1]->PushBack(cond2_created_stmt);
   cond2_index = ssa2_node_nid;

   /// create (!)cond1 or cond2
   IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
   IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(cond1_index);
   IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(cond2_index);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   unsigned int expr_index = TM->new_tree_node_id();
   if(or_type)
      TM->create_tree_node(expr_index, truth_or_expr_K, IR_schema);
   else
      TM->create_tree_node(expr_index, truth_and_expr_K, IR_schema);
   IR_schema.clear();
   /// The expression contained in ce2 must now be the newly created expression,
   /// identified by expr_index
   /// Temporary remove statement to remove old uses
   list_of_bloc[bb2]->RemoveStmt(second_stmt);
   ce2->op0 = TM->GetTreeReindex(expr_index);

   /// Readding the statement
   list_of_bloc[bb2]->PushBack(second_stmt);

   /// add the statements of bb1 to bb2
   while(list_of_stmt_cond1.size())
   {
      const tree_nodeRef stmt = list_of_stmt_cond1.back();
      list_of_bloc[bb1]->RemoveStmt(stmt);
      list_of_bloc[bb2]->PushFront(stmt);
   }
   /// add the phi of bb1 to bb2
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Moving phis");
   const auto& bb1_phi_list = list_of_bloc[bb1]->CGetPhiList();
   while(bb1_phi_list.size())
   {
      const tree_nodeRef phi = bb1_phi_list.back();
      list_of_bloc[bb1]->RemovePhi(phi);
      list_of_bloc[bb2]->AddPhi(phi);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved phis");
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "GIMPLE_COND built");
   return true;
}

void short_circuit_taf::restructure_CFG(unsigned int bb1, unsigned int bb2, unsigned int merging_candidate, std::map<unsigned int, blocRef>& list_of_bloc)
{
   /// fix bb2 predecessor
   std::vector<unsigned int>::iterator pos;
   std::vector<unsigned int>::const_iterator it_bb1_pred_end = list_of_bloc[bb1]->list_of_pred.end();
   for(std::vector<unsigned int>::const_iterator it_bb1_pred = list_of_bloc[bb1]->list_of_pred.begin(); it_bb1_pred_end != it_bb1_pred; ++it_bb1_pred)
   {
      list_of_bloc[bb2]->list_of_pred.push_back(*it_bb1_pred);
      pos = std::find(list_of_bloc[*it_bb1_pred]->list_of_succ.begin(), list_of_bloc[*it_bb1_pred]->list_of_succ.end(), bb1);
      *pos = bb2;
      if(list_of_bloc[*it_bb1_pred]->true_edge == bb1)
         list_of_bloc[*it_bb1_pred]->true_edge = bb2;
      else if(list_of_bloc[*it_bb1_pred]->false_edge == bb1)
         list_of_bloc[*it_bb1_pred]->false_edge = bb2;
   }
   pos = std::find(list_of_bloc[bb2]->list_of_pred.begin(), list_of_bloc[bb2]->list_of_pred.end(), bb1);
   list_of_bloc[bb2]->list_of_pred.erase(pos);
   /// fix bb1 empty block
   pos = std::find(list_of_bloc[merging_candidate]->list_of_pred.begin(), list_of_bloc[merging_candidate]->list_of_pred.end(), bb1);
   list_of_bloc[merging_candidate]->list_of_pred.erase(pos);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Removed BB " + boost::lexical_cast<std::string>(bb1));
   /// check for BB with gimple_multi_way_if
   fix_multi_way_if(bb1, list_of_bloc, bb2);
   list_of_bloc.erase(bb1);
}

void short_circuit_taf::fix_multi_way_if(unsigned int curr_bb, std::map<unsigned int, blocRef>& list_of_bloc, unsigned int succ)
{
   std::vector<unsigned int>::const_iterator lop_it_end = list_of_bloc[curr_bb]->list_of_pred.end();
   for(auto lop_it = list_of_bloc[curr_bb]->list_of_pred.begin(); lop_it_end != lop_it; ++lop_it)
   {
      const auto list_of_pred_stmt = list_of_bloc[*lop_it]->CGetStmtList();
      tree_nodeRef cond_statement = list_of_pred_stmt.begin() != list_of_pred_stmt.end() ? list_of_pred_stmt.back() : tree_nodeRef();
      tree_nodeRef cond_statement_node = cond_statement ? GET_NODE(cond_statement) : cond_statement;
      if(cond_statement_node && GetPointer<gimple_multi_way_if>(cond_statement_node))
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(cond_statement_node);
         for(auto& cond : gmwi->list_of_cond)
            if(cond.second == curr_bb)
               cond.second = succ;
      }
   }
}
