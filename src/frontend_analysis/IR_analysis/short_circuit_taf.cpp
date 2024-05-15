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

/// HLS include
#if HAVE_ILP_BUILT
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#endif

/// Parameter include
#include "Parameter.hpp"

/// design flow manager include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls_manager.hpp"

/// parser/compiler include
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

/// utility includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

short_circuit_taf::short_circuit_taf(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                     unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SHORT_CIRCUIT_TAF, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

short_circuit_taf::~short_circuit_taf() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
short_circuit_taf::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
         }
#endif
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         /// Not executed
         if(GetStatus() != DesignFlowStep_Status::SUCCESS)
         {
            if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and
               GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
               GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
            {
               /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
               const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step =
                      design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
                  if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
                     function_behavior->GetBBVersion())
                  {
                     relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
                     break;
                  }
               }
            }
         }
         else
         {
            relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
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

DesignFlowStep_Status short_circuit_taf::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto temp = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<const function_decl>(temp);
   const auto sl = GetPointer<statement_list>(fd->body);

   /// compute merging candidates
   CustomUnorderedSet<unsigned int> merging_candidates;
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      if(idx_bb.first == bloc::ENTRY_BLOCK_ID || idx_bb.first == bloc::EXIT_BLOCK_ID)
      {
         continue;
      }
      unsigned int n_pred_bb = 0;
      if(idx_bb.second->list_of_pred.size() <= 1)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(idx_bb.first));
      for(auto const pred : idx_bb.second->list_of_pred)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing predecessor BB" + STR(pred));
         if(pred != bloc::ENTRY_BLOCK_ID && idx_bb.first != pred && sl->list_of_bloc.at(pred)->CGetStmtList().size() &&
            sl->list_of_bloc.at(pred)->CGetStmtList().back()->get_kind() == gimple_cond_K)
         {
            ++n_pred_bb;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ok");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
         }
      }
      if(n_pred_bb > 1 && check_phis(idx_bb.first, sl->list_of_bloc))
      {
         merging_candidates.insert(idx_bb.first);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added candidate BB" + STR(idx_bb.first));
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped BB" + STR(idx_bb.first));
      }
   }
   if(!merging_candidates.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Merging candidate number " + STR(merging_candidates.size()));
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   /// find the first to merge
   auto bb1 = static_cast<unsigned int>(-1), bb2 = static_cast<unsigned int>(-1), merging_candidate = 0U;
   bool bb1_true = false;
   bool bb2_true = false;
   bool mergeable_pair_found;
   bool something_change = false;

   int merge_n = 0;
   do
   {
      mergeable_pair_found = false;
      const auto it_mc_end = merging_candidates.cend();
      for(auto it_mc = merging_candidates.cbegin(); !mergeable_pair_found && it_mc != it_mc_end; ++it_mc)
      {
         merging_candidate = *it_mc;
         mergeable_pair_found =
             check_merging_candidate(bb1, bb2, merging_candidate, bb1_true, bb2_true, sl->list_of_bloc);
      }
      if(!AppM->ApplyNewTransformation())
      {
         break;
      }
      if(mergeable_pair_found)
      {
         AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
         if(create_gimple_cond(bb1, bb2, bb1_true, sl->list_of_bloc, bb2_true, merging_candidate))
         {
            something_change = true;
            restructure_CFG(bb1, bb2, merging_candidate, sl->list_of_bloc);
            merging_candidates.erase(bb1);
            if(!check_merging_candidate(bb1, bb2, merging_candidate, bb1_true, bb2_true, sl->list_of_bloc))
            {
               merging_candidates.erase(merging_candidate);
            }
            if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
               (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
            {
               WriteBBGraphDot("BB_After_" + GetName() + "_merge_" + STR(merge_n) + ".dot");
            }
            merge_n++;
         }
         else
         { /// cond expr not completely collapsed
            merging_candidates.erase(merging_candidate);
         }
      }
   } while(mergeable_pair_found);

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

bool short_circuit_taf::check_merging_candidate(unsigned int& bb1, unsigned int& bb2, unsigned int merging_candidate,
                                                bool& bb1_true, bool& bb2_true,
                                                const std::map<unsigned int, blocRef>& list_of_bloc)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking merging candidate BB" + STR(merging_candidate));
   bool mergeable_pair_found = false;
   /// let bb1 the upper if
   THROW_ASSERT(list_of_bloc.find(merging_candidate) != list_of_bloc.end(),
                "merging_candidate is not included in list_of_bloc");
   const auto it_pred_end = list_of_bloc.at(merging_candidate)->list_of_pred.end();
   for(auto it_bb1_pred = list_of_bloc.at(merging_candidate)->list_of_pred.begin();
       !mergeable_pair_found && it_pred_end != it_bb1_pred; ++it_bb1_pred)
   {
      bb1 = *it_bb1_pred;
      if(bb1 == bloc::ENTRY_BLOCK_ID || bb1 == merging_candidate)
      {
         continue;
      }
      if(list_of_bloc.at(bb1)->CGetStmtList().empty())
      {
         continue;
      }
      if(list_of_bloc.at(bb1)->CGetStmtList().back()->get_kind() != gimple_cond_K)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Examining merging candidate predecessor BB" + STR(bb1));
      THROW_ASSERT(list_of_bloc.at(bb1)->true_edge > 0,
                   "bb1 has to be an if statement " + STR(bb1) + " " + STR(merging_candidate));
      if(list_of_bloc.at(bb1)->true_edge == merging_candidate)
      {
         bb1_true = true;
      }
      else
      {
         bb1_true = false;
      }
      /// let search bb2, the lower if
      for(auto it_bb2_pred = list_of_bloc.at(merging_candidate)->list_of_pred.begin();
          !mergeable_pair_found && it_pred_end != it_bb2_pred; ++it_bb2_pred)
      {
         bb2 = *it_bb2_pred;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       "Examining merging candidate nested predecessor " + STR(bb2));
         if(bb2 == bloc::ENTRY_BLOCK_ID || bb2 == merging_candidate)
         {
            continue;
         }
         if(list_of_bloc.at(bb2)->list_of_pred.size() > 1)
         {
            continue;
         }
         if(list_of_bloc.at(bb2)->CGetStmtList().size() != 1)
         {
            continue;
         }
         if(list_of_bloc.at(bb2)->CGetPhiList().size() != 0)
         {
            continue;
         }
         if(list_of_bloc.at(bb2)->CGetStmtList().back()->get_kind() != gimple_cond_K)
         {
            continue;
         }
         THROW_ASSERT(list_of_bloc.at(bb2)->true_edge > 0,
                      "bb2 has to be an if statement " + STR(bb2) + " " + STR(merging_candidate));
         // This check is needed for empty while loop with short circuit (e. g. 20000314-1.c)
         if(list_of_bloc.at(bb2)->true_edge == bb1 || list_of_bloc.at(bb2)->false_edge == bb1)
         {
            continue;
         }
         if(bb1_true && list_of_bloc.at(bb1)->false_edge == bb2)
         {
            if(list_of_bloc.at(bb2)->true_edge == merging_candidate)
            {
               bb2_true = true;
            }
            else
            {
               bb2_true = false;
            }
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pair found: " << STR(bb1) << " and " << STR(bb2));
            mergeable_pair_found = true;
            if(bb2_true)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "bb1 T " + STR(bb1) + " bb2 T " + STR(bb2) + " MC " + STR(merging_candidate));
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "bb1 T " + STR(bb1) + " bb2 F " + STR(bb2) + " MC " + STR(merging_candidate));
            }
         }
         else if(!bb1_true && list_of_bloc.at(bb1)->true_edge == bb2)
         {
            if(list_of_bloc.at(bb2)->true_edge == merging_candidate)
            {
               bb2_true = true;
            }
            else
            {
               bb2_true = false;
            }
            mergeable_pair_found = true;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pair found: " << STR(bb1) << " and " << STR(bb2));
            if(bb2_true)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "bb1 F " + STR(bb1) + " bb2 T " + STR(bb2) + " MC " + STR(merging_candidate));
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "bb1 F " + STR(bb1) + " bb2 F " + STR(bb2) + " MC " + STR(merging_candidate));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined mergin candidate predecessor BB" + STR(bb1));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked merging candidate");
   return mergeable_pair_found;
}

bool short_circuit_taf::create_gimple_cond(unsigned int bb1, unsigned int bb2, bool bb1_true,
                                           const std::map<unsigned int, blocRef>& list_of_bloc, bool or_type,
                                           unsigned int merging_candidate)
{
   const tree_managerRef TM = AppM->get_tree_manager();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Creating new cond expr: " + STR(bb1) + " is the first basic block, " + STR(bb2) +
                     " is the second basic block");
   /// If there are more than one statements in the basic block containing cond2, then do not merge conditions (in case
   /// speculation step should manage the code motion)
   if(list_of_bloc.at(bb2)->CGetStmtList().size() != 1)
   {
      return false;
   }
   const auto list_of_stmt_cond2 = list_of_bloc.at(bb2)->CGetStmtList();

   /// identify the first gimple_cond
   const auto& list_of_stmt_cond1 = list_of_bloc.at(bb1)->CGetStmtList();
   THROW_ASSERT(list_of_stmt_cond1.back()->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   const auto cond_statement = list_of_stmt_cond1.back();
   list_of_bloc.at(bb1)->RemoveStmt(cond_statement, AppM);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First gimple cond is " + STR(cond_statement));
   const auto ce1 = GetPointer<const gimple_cond>(cond_statement);
   auto cond1 = ce1->op0;
   auto type_node = tree_helper::CGetType(cond1);
   const auto tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters, AppM));
   if(!tree_helper::IsBooleanType(type_node))
   {
      type_node = tree_man->GetBooleanType();
   }

   /// create the assignment between condition for bb1 and the new ssa var
   const auto cond1_created_stmt =
       tree_man->CreateGimpleAssign(type_node, nullptr, nullptr, cond1, function_id, BUILTIN_SRCP);
   const auto ssa1_cond_node = GetPointer<const gimple_assign>(cond1_created_stmt)->op0;
   /// and then add to the bb1 statement list
   list_of_bloc.at(bb1)->PushBack(cond1_created_stmt, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Created statement in BB" + STR(bb1) + " - " + STR(cond1_created_stmt));
   cond1 = ssa1_cond_node;

   /// fix merging_candidate phis
   if(list_of_bloc.at(merging_candidate)->CGetPhiList().size())
   {
      for(const auto& phi : list_of_bloc.at(merging_candidate)->CGetPhiList())
      {
         const auto mc_phi = GetPointer<gimple_phi>(phi);
         std::pair<tree_nodeRef, unsigned int> def_edge_to_be_removed(tree_nodeRef(), 0);
         std::pair<tree_nodeRef, unsigned int> def_edge_to_be_updated(tree_nodeRef(), 0);
         for(const auto& def_edge : mc_phi->CGetDefEdgesList())
         {
            if(def_edge.second == bb1)
            {
               def_edge_to_be_removed = def_edge;
            }
            else if(def_edge.second == bb2)
            {
               def_edge_to_be_updated = def_edge;
            }
         }
         THROW_ASSERT(def_edge_to_be_removed.first, "unexpected condition");
         THROW_ASSERT(def_edge_to_be_updated.first, "unexpected condition");
         auto op1 = def_edge_to_be_removed.first;
         auto op2 = def_edge_to_be_updated.first;
         if(!bb1_true)
         {
            std::swap(op1, op2);
         }

         /// second, create the gimple assignment
         const auto res_type = tree_helper::CGetType(mc_phi->res);
         auto condition_type = tree_helper::CGetType(cond1);
         auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
         const auto cond_expr_node = tree_man->create_ternary_operation(
             res_type, cond1, op1, op2, BUILTIN_SRCP, (isAVectorType ? vec_cond_expr_K : cond_expr_K));
         const auto created_stmt =
             tree_man->CreateGimpleAssign(res_type, nullptr, nullptr, cond_expr_node, function_id, BUILTIN_SRCP);
         const auto ssa_cond_node = GetPointer<const gimple_assign>(created_stmt)->op0;

         /// and then add to the statement list
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Created new assignment: " + created_stmt->ToString());
         list_of_bloc.at(bb1)->PushBack(created_stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
         mc_phi->ReplaceDefEdge(TM, def_edge_to_be_updated,
                                gimple_phi::DefEdge(ssa_cond_node, def_edge_to_be_updated.second));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
         mc_phi->RemoveDefEdge(TM, def_edge_to_be_removed);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi is " + mc_phi->ToString());
      }
   }

   if((bb1_true && !or_type) || (!bb1_true && or_type))
   {
      /// create !cond1
      const auto not_cond1 = tree_man->create_unary_operation(type_node, cond1, BUILTIN_SRCP, truth_not_expr_K);
      const auto created_stmt =
          tree_man->CreateGimpleAssign(type_node, nullptr, nullptr, not_cond1, function_id, BUILTIN_SRCP);
      cond1 = GetPointer<const gimple_assign>(created_stmt)->op0;
      /// and then add to the bb1 statement list
      list_of_bloc.at(bb1)->PushBack(created_stmt, AppM);
   }
   /// identify the second gimple_cond
   THROW_ASSERT(list_of_bloc.at(bb2)->CGetPhiList().size() == 0, "not expected phi nodes");

   THROW_ASSERT(list_of_stmt_cond2.front()->get_kind() == gimple_cond_K, "a gimple_cond is expected");

   const auto second_stmt = list_of_stmt_cond2.front();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second gimple cond is " + STR(second_stmt));
   auto ce2 = GetPointer<gimple_cond>(second_stmt);
   auto cond2 = ce2->op0;

   /// create the assignment between condition for bb2 and the new ssa var
   const auto cond2_created_stmt =
       tree_man->CreateGimpleAssign(type_node, nullptr, nullptr, cond2, function_id, BUILTIN_SRCP);
   cond2 = GetPointer<const gimple_assign>(cond2_created_stmt)->op0;
   /// and then add to the bb1 statement list
   list_of_bloc.at(bb1)->PushBack(cond2_created_stmt, AppM);

   /// The expression contained in ce2 must now be the newly created expression,
   /// identified by expr_index
   /// Temporary remove statement to remove old uses
   list_of_bloc.at(bb2)->RemoveStmt(second_stmt, AppM);
   /// create (!)cond1 or cond2
   ce2->op0 = tree_man->create_binary_operation(type_node, cond1, cond2, BUILTIN_SRCP,
                                                (or_type ? truth_or_expr_K : truth_and_expr_K));

   /// Readding the statement
   list_of_bloc.at(bb2)->PushBack(second_stmt, AppM);

   /// add the statements of bb1 to bb2
   while(list_of_stmt_cond1.size())
   {
      const auto stmt = list_of_stmt_cond1.back();
      list_of_bloc.at(bb1)->RemoveStmt(stmt, AppM);
      list_of_bloc.at(bb2)->PushFront(stmt, AppM);
   }
   /// add the phi of bb1 to bb2
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Moving phis");
   const auto& bb1_phi_list = list_of_bloc.at(bb1)->CGetPhiList();
   while(bb1_phi_list.size())
   {
      const auto phi = bb1_phi_list.back();
      list_of_bloc.at(bb1)->RemovePhi(phi);
      list_of_bloc.at(bb2)->AddPhi(phi);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved phis");
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "GIMPLE_COND built");
   return true;
}

void short_circuit_taf::restructure_CFG(unsigned int bb1, unsigned int bb2, unsigned int merging_candidate,
                                        std::map<unsigned int, blocRef>& list_of_bloc)
{
   /// fix bb2 predecessor
   std::vector<unsigned int>::iterator pos;
   for(const auto& bb1_pred : list_of_bloc.at(bb1)->list_of_pred)
   {
      list_of_bloc.at(bb2)->list_of_pred.push_back(bb1_pred);
      pos = std::find(list_of_bloc.at(bb1_pred)->list_of_succ.begin(), list_of_bloc.at(bb1_pred)->list_of_succ.end(),
                      bb1);
      *pos = bb2;
      if(list_of_bloc.at(bb1_pred)->true_edge == bb1)
      {
         list_of_bloc.at(bb1_pred)->true_edge = bb2;
      }
      else if(list_of_bloc.at(bb1_pred)->false_edge == bb1)
      {
         list_of_bloc.at(bb1_pred)->false_edge = bb2;
      }
   }
   pos = std::find(list_of_bloc.at(bb2)->list_of_pred.begin(), list_of_bloc.at(bb2)->list_of_pred.end(), bb1);
   list_of_bloc.at(bb2)->list_of_pred.erase(pos);
   /// fix bb1 empty block
   pos = std::find(list_of_bloc.at(merging_candidate)->list_of_pred.begin(),
                   list_of_bloc.at(merging_candidate)->list_of_pred.end(), bb1);
   list_of_bloc.at(merging_candidate)->list_of_pred.erase(pos);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Removed BB " + STR(bb1));
   /// check for BB with gimple_multi_way_if
   for(const auto& pred : list_of_bloc.at(bb1)->list_of_pred)
   {
      const auto list_of_pred_stmt = list_of_bloc.at(pred)->CGetStmtList();
      const auto cond_statement =
          list_of_pred_stmt.begin() != list_of_pred_stmt.end() ? list_of_pred_stmt.back() : tree_nodeRef();
      const auto cond_statement_node = cond_statement ? cond_statement : cond_statement;
      if(cond_statement_node && GetPointer<gimple_multi_way_if>(cond_statement_node))
      {
         const auto gmwi = GetPointerS<gimple_multi_way_if>(cond_statement_node);
         for(auto& cond : gmwi->list_of_cond)
         {
            if(cond.second == bb1)
            {
               cond.second = bb2;
            }
         }
      }
   }
   list_of_bloc.erase(bb1);
}

bool short_circuit_taf::check_phis(unsigned int curr_bb, const std::map<unsigned int, blocRef>& list_of_bloc)
{
   for(const auto& phi : list_of_bloc.at(curr_bb)->CGetPhiList())
   {
      const auto cb_phi = GetPointerS<const gimple_phi>(phi);
      if(cb_phi->virtual_flag)
      {
         return false;
      }
   }
   return true;
}

bool short_circuit_taf::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return false;
   }

#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
   {
      if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
         GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
      {
         /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
         const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
         if(update_schedule != DesignFlowGraph::null_vertex())
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step =
                design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
            if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
               function_behavior->GetBBVersion())
            {
               return false;
            }
            else
            {
               return true;
            }
         }
         else
         {
            return false;
         }
      }
      else
      {
         return true;
      }
   }
#endif

   return true;
}
