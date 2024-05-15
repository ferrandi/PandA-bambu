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
 * @file dead_code_elimination.cpp
 * @brief Eliminate dead code
 *
 * @author Andrea Cuoccio <andrea.cuoccio@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// header include
#include "dead_code_elimination.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "sdc_scheduling.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "var_pp_functor.hpp"

/// STD includes
#include <fstream>
#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>

dead_code_elimination::dead_code_elimination(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                             unsigned int _function_id,
                                             const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DEAD_CODE_ELIMINATION, _design_flow_manager, _parameters),
      restart_if_opt(false),
      restart_mwi_opt(false),
      restart_mem(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dead_code_elimination::~dead_code_elimination() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
dead_code_elimination::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, CALLING_FUNCTIONS));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(restart_mem)
            {
               relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
            }
            if(restart_if_opt)
            {
               relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
            if(restart_mwi_opt)
            {
               relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
            if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
            {
               relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            }
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool dead_code_elimination::HasToBeExecuted() const
{
   if(FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return true;
   }
   std::map<unsigned int, bool> cur_writing_memory;
   std::map<unsigned int, bool> cur_reading_memory;
   const auto TM = AppM->get_tree_manager();
   for(const auto i : AppM->CGetCallGraphManager()->get_called_by(function_id))
   {
      const auto curr_tn = TM->GetTreeNode(i);
      const auto fdCalled = GetPointerS<const function_decl>(curr_tn);
      cur_writing_memory[i] = fdCalled->writing_memory;
      cur_reading_memory[i] = fdCalled->reading_memory;
   }
   return cur_writing_memory != last_writing_memory || cur_reading_memory != last_reading_memory;
}

void dead_code_elimination::fix_sdc_motion(DesignFlowManagerConstRef design_flow_manager, unsigned int function_id,
                                           tree_nodeRef removedStmt)
{
   const auto design_flow_graph = design_flow_manager->CGetDesignFlowGraph();
   const auto sdc_scheduling_step = design_flow_manager->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(
       HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), function_id));
   if(sdc_scheduling_step != DesignFlowGraph::null_vertex())
   {
      const auto sdc_scheduling =
          GetPointer<SDCScheduling>(design_flow_graph->CGetNodeInfo(sdc_scheduling_step)->design_flow_step);
      const auto removed_index = removedStmt->index;
      sdc_scheduling->movements_list.remove_if(
          [&](const std::vector<unsigned int>& mv) { return mv[0] == removed_index; });
   }
}

void dead_code_elimination::fix_sdc_motion(tree_nodeRef removedStmt) const
{
   return fix_sdc_motion(design_flow_manager.lock(), function_id, removedStmt);
}

void dead_code_elimination::kill_uses(const tree_managerRef& TM, const tree_nodeRef& op0) const
{
   THROW_ASSERT(op0->get_kind() == ssa_name_K, "expected a ssa_name object");
   const auto ssa = GetPointerS<ssa_name>(op0);
   THROW_ASSERT(ssa->CGetDefStmts().size() == 1, "unexpected condition");

   if(ssa->CGetNumberUses() != 0)
   {
      const auto ssa_type = tree_helper::CGetType(op0);
      tree_nodeRef val;
      if(tree_helper::IsRealType(ssa_type))
      {
         val = TM->CreateUniqueRealCst(0.l, ssa_type);
      }
      else if(tree_helper::IsComplexType(ssa_type) || tree_helper::IsVectorType(ssa_type))
      {
         const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
         const auto utype = tree_man->GetUnsignedIntegerType();
         const auto zeroVal = TM->CreateUniqueIntegerCst(0LL, utype);
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ne_schema;
         ne_schema[TOK(TOK_TYPE)] = STR(ssa_type->index);
         ne_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
         ne_schema[TOK(TOK_OP)] = STR(zeroVal->index);
         const auto ne_id = TM->new_tree_node_id();
         TM->create_tree_node(ne_id, nop_expr_K, ne_schema);
         val = TM->GetTreeNode(ne_id);
      }
      else
      {
         val = TM->CreateUniqueIntegerCst(0LL, ssa_type);
      }
      const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
      for(const auto& use : StmtUses)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---replace constant usage before: " + use.first->ToString());
         TM->ReplaceTreeNode(use.first, op0, val);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---replace constant usage after: " + use.first->ToString());
      }
      THROW_ASSERT(ssa->CGetNumberUses() == 0, "unexpected condition");
   }
}

tree_nodeRef dead_code_elimination::kill_vdef(const tree_managerRef& TM, const tree_nodeRef& vdef)
{
   const auto v_ssa = GetPointerS<ssa_name>(vdef);
   const auto function_id = GetPointerS<gimple_node>(v_ssa->CGetDefStmt())->scpe->index;
   const auto gimple_nop_id = TM->new_tree_node_id();
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
      gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
      gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
      TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
   }
   const auto nop_stmt = TM->GetTreeNode(gimple_nop_id);
   GetPointerS<gimple_node>(nop_stmt)->vdef = vdef;
   v_ssa->SetDefStmt(nop_stmt);
   return nop_stmt;
}

tree_nodeRef dead_code_elimination::add_gimple_nop(const tree_managerRef& TM, const tree_nodeRef& cur_stmt,
                                                   const blocRef& bb)
{
   const auto gn = GetPointer<gimple_node>(cur_stmt);
   THROW_ASSERT(gn, "");
   const auto nop_stmt_id = TM->new_tree_node_id();
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_IR_schema;
      gimple_nop_IR_schema[TOK(TOK_SRCP)] =
          gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
      gimple_nop_IR_schema[TOK(TOK_SCPE)] = STR(function_id);
      TM->create_tree_node(nop_stmt_id, gimple_nop_K, gimple_nop_IR_schema);
   }
   const auto nop_stmt = TM->GetTreeNode(nop_stmt_id);
   const auto new_gn = GetPointerS<gimple_node>(nop_stmt);
   if(gn->vdef)
   {
      new_gn->vdef = gn->vdef;
      gn->vdef = nullptr;
   }
   if(gn->vuses.size())
   {
      new_gn->vuses = gn->vuses;
   }
   if(gn->vovers.size())
   {
      new_gn->vovers = gn->vovers;
   }
   bb->PushBefore(nop_stmt, cur_stmt, AppM);
   return nop_stmt;
}

blocRef dead_code_elimination::move2emptyBB(const tree_managerRef& TM, const unsigned int new_bbi,
                                            const statement_list* sl, const blocRef& bb_pred,
                                            const unsigned int cand_bb_dest, const unsigned int bb_dest_number) const
{
   const auto& bb_succ = sl->list_of_bloc.at(cand_bb_dest);
   const auto& bb_dest = sl->list_of_bloc.at(bb_dest_number);

   /// Create empty basic block
   const auto bb_new = blocRef(new bloc(new_bbi));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Created BB" + STR(bb_new->number) + " as new successor of BB" + STR(bb_pred->number));
   // sl->list_of_bloc[bb_new->number] = bb_new;

   bb_new->loop_id = bb_pred->loop_id;
   bb_new->SetSSAUsesComputed();
   bb_new->schedule = bb_pred->schedule;

   bb_new->list_of_pred.push_back(bb_pred->number);
   bb_new->list_of_succ.push_back(bb_dest->number);
   bb_dest->list_of_pred.push_back(bb_new->number);
   bb_pred->list_of_succ.erase(std::find(bb_pred->list_of_succ.begin(), bb_pred->list_of_succ.end(), bb_succ->number));
   bb_pred->list_of_succ.push_back(bb_new->number);

   bb_succ->list_of_pred.erase(std::find(bb_succ->list_of_pred.begin(), bb_succ->list_of_pred.end(), bb_pred->number));
   /// Fix PHIs
   for(const auto& phi : bb_succ->CGetPhiList())
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_pred->number)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Removing <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
            gp->RemoveDefEdge(TM, def_edge);
            break;
         }
      }
   }
   for(const auto& phi : bb_dest->CGetPhiList())
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      gimple_phi::DefEdgeList new_defedges;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_pred->number)
         {
            new_defedges.push_back(gimple_phi::DefEdge(def_edge.first, bb_new->number));
         }
      }
      for(const auto& def_edge : new_defedges)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) +
                            ">");
         gp->AddDefEdge(TM, def_edge);
      }
   }
   return bb_new;
}

/// single sweep analysis, block by block, from the bottom to up. Each ssa which is used zero times is eliminated and
/// the uses of the variables used in the assignment are recomputed multi-way and two way IFs simplified when conditions
/// are constants gimple_call without side effects are removed store-load pairs checked for simplification dead stores
/// removed
DesignFlowStep_Status dead_code_elimination::InternalExec()
{
   if(parameters->IsParameter("disable-dce") && parameters->GetParameter<unsigned int>("disable-dce") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const auto TM = AppM->get_tree_manager();
   auto fd = GetPointerS<function_decl>(TM->GetTreeNode(function_id));
   auto sl = GetPointerS<statement_list>(fd->body);
   /// Retrieve the list of block
   const auto& blocks = sl->list_of_bloc;

   bool modified = false;
   bool restart_analysis = false;
   restart_mem = false;
   restart_if_opt = false;
   restart_mwi_opt = false;

   do
   {
      restart_analysis = false;
      bool do_reachability = false;
      std::list<blocRef> new_bbs;
      const auto get_new_bbi = [&]() -> unsigned int {
         return sl->list_of_bloc.rbegin()->first + 1U + static_cast<unsigned int>(new_bbs.size());
      };
      CustomUnorderedMap<unsigned, CustomOrderedSet<unsigned>> vdefvover_map;
      // CustomUnorderedSet<unsigned> vdefvover_map;
      for(const auto& block : blocks)
      {
         const auto& stmt_list = block.second->CGetStmtList();
         for(const auto& stmt : stmt_list)
         {
            const auto gn = GetPointerS<gimple_node>(stmt);
            THROW_ASSERT(gn->vovers.empty() || gn->vdef, "unexpected condition");
            for(const auto& vo : gn->vovers)
            {
               vdefvover_map[vo->index].insert(gn->vdef->index);
               // vdefvover_map.insert(vo->index);
            }
         }
      }
      for(const auto& block : blocks)
      {
         const auto& bb = block.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
         const auto& stmt_list = bb->CGetStmtList();
         std::list<tree_nodeRef> new_vssa_nop;
         std::list<tree_nodeRef> stmts_to_be_removed;
         for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
            /// find out if it is a gimple_assign
            if((*stmt)->get_kind() == gimple_assign_K)
            {
               const auto ga = GetPointerS<gimple_assign>(*stmt);
               /// in case of virtual uses it is better not perform the elimination
               if(ga->predicate && ga->predicate->get_kind() == integer_cst_K &&
                  tree_helper::GetConstValue(ga->predicate) == 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead predicate found");
                  if(ga->vdef)
                  {
                     new_vssa_nop.push_back(kill_vdef(TM, ga->vdef));
                     ga->vdef = nullptr;
                     restart_mem = true;
                  }
                  else if(ga->op0->get_kind() == ssa_name_K)
                  {
                     if(ga->vdef || ga->vuses.size() || ga->vovers.size())
                     {
                        restart_mem = true;
                     }
                     kill_uses(TM, ga->op0);
                  }
                  else
                  {
                     THROW_ERROR("unexpected condition");
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                  stmts_to_be_removed.push_back(*stmt);
                  AppM->RegisterTransformation(GetName(), *stmt);
               }
               else
               {
                  /// op0 is the left side of the assignment, op1 is the right side
                  const auto op0 = ga->op0;
                  const auto op1_type = tree_helper::CGetType(ga->op1);
                  if(op0->get_kind() == ssa_name_K)
                  {
                     const auto ssa = GetPointerS<ssa_name>(op0);
                     /// very strict condition for the elimination
                     if(ssa->CGetNumberUses() == 0 and ssa->CGetDefStmts().size() == 1)
                     {
                        bool is_a_writing_memory_call = false;
                        bool is_a_reading_memory_call = false;
                        if(ga->op1->get_kind() == call_expr_K || ga->op1->get_kind() == aggr_init_expr_K)
                        {
                           const auto ce = GetPointerS<call_expr>(ga->op1);
                           if(ce->fn->get_kind() == addr_expr_K)
                           {
                              const auto ae = GetPointerS<const addr_expr>(ce->fn);
                              const auto fu_decl_node = ae->op;
                              THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K,
                                           "node  " + STR(fu_decl_node) + " is not function_decl but " +
                                               fu_decl_node->get_kind_text());
                              const auto fdCalled = GetPointerS<const function_decl>(fu_decl_node);
                              if(fdCalled->writing_memory || !fdCalled->body)
                              {
                                 is_a_writing_memory_call = true;
                              }
                              if(fdCalled->reading_memory)
                              {
                                 is_a_reading_memory_call = true;
                              }
                           }
                           else
                           {
                              is_a_writing_memory_call = true; /// conservative analysis
                           }
                        }
                        if(!is_a_writing_memory_call)
                        {
                           if(ga->vdef)
                           {
                              add_gimple_nop(TM, *stmt, bb);
                              restart_mem = true;
                           }
                           if(is_a_reading_memory_call || ga->vdef || ga->vuses.size() || ga->vovers.size() ||
                              ga->op1->get_kind() == addr_expr_K || ga->op1->get_kind() == mem_ref_K)
                           {
                              restart_mem = true;
                           }
                           stmts_to_be_removed.push_back(*stmt);
                           AppM->RegisterTransformation(GetName(), *stmt);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---LHS ssa used: " + STR(ssa->CGetNumberUses()) + "-" +
                                           STR(ssa->CGetDefStmts().size()));
                     }
                  }
                  else if(op0->get_kind() == mem_ref_K && !ga->artificial && !tree_helper::IsVectorType(op1_type))
                  {
                     const auto mr = GetPointerS<mem_ref>(op0);
                     THROW_ASSERT(mr->op1->get_kind() == integer_cst_K, "unexpected condition");
                     const auto type_w = tree_helper::CGetType(ga->op1);
                     const auto written_bw = tree_helper::SizeAlloc(type_w);
                     if(tree_helper::GetConstValue(mr->op1) == 0)
                     {
                        if(mr->op0->get_kind() == integer_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---do nothing with constant values");
                        }
                        else
                        {
                           THROW_ASSERT(mr->op0->get_kind() == ssa_name_K, "unexpected condition" + ga->ToString());
                           auto derefVar = GetPointerS<ssa_name>(mr->op0);
                           auto defStmts = derefVar->CGetDefStmts();
                           if(defStmts.size() == 1)
                           {
                              auto defStmt = *defStmts.begin();
                              if(defStmt->get_kind() == gimple_assign_K)
                              {
                                 const auto derefGA = GetPointerS<gimple_assign>(defStmt);
                                 if(derefGA->op1->get_kind() == addr_expr_K)
                                 {
                                    const auto addressedVar = GetPointerS<addr_expr>(derefGA->op1)->op;
                                    if(addressedVar->get_kind() == var_decl_K)
                                    {
                                       const auto varDecl = GetPointerS<var_decl>(addressedVar);
                                       if(varDecl->scpe && function_id == varDecl->scpe->index && !varDecl->static_flag)
                                       {
                                          ssa_name* ssaDef = nullptr;
                                          if(ga->vdef)
                                          {
                                             ssaDef = GetPointerS<ssa_name>(ga->vdef);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---VDEF: " + STR(ga->vdef));
                                          }
                                          else
                                          {
                                             THROW_ERROR("unexpected condition");
                                          }
                                          THROW_ASSERT(ssaDef, "unexpected condition");
                                          /// very strict condition for the elimination
                                          if(ssaDef->CGetDefStmts().size() == 1)
                                          {
                                             if(ssaDef->CGetNumberUses() == 0 &&
                                                vdefvover_map.find(ssaDef->index) == vdefvover_map.end())
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---Dead Store found");
                                                if(ga->vdef)
                                                {
                                                   add_gimple_nop(TM, *stmt, bb);
                                                }
                                                stmts_to_be_removed.push_back(*stmt);
                                                AppM->RegisterTransformation(GetName(), *stmt);
                                                restart_mem = true;
                                             }
                                             else
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---check if the associated load are dead");
                                                const TreeNodeMap<size_t> StmtUses = ssaDef->CGetUseStmts();
                                                for(const auto& use : StmtUses)
                                                {
                                                   const auto gn_used = GetPointerS<gimple_node>(use.first);
                                                   if(!gn_used->vdef && gn_used->bb_index == ga->bb_index &&
                                                      gn_used->get_kind() == gimple_assign_K)
                                                   {
                                                      const auto ga_used = GetPointerS<gimple_assign>(use.first);
                                                      if(ga_used->op0->get_kind() == ssa_name_K &&
                                                         ga_used->op1->get_kind() == mem_ref_K &&
                                                         !(ga_used->predicate &&
                                                           ga_used->predicate->get_kind() == integer_cst_K &&
                                                           tree_helper::GetConstValue(ga_used->predicate) == 0))
                                                      {
                                                         const auto mr_used = GetPointerS<mem_ref>(ga_used->op1);
                                                         if(tree_helper::GetConstValue(mr->op1) ==
                                                            tree_helper::GetConstValue(mr_used->op1))
                                                         {
                                                            const auto type_r = tree_helper::CGetType(ga_used->op0);
                                                            const auto read_bw = tree_helper::SizeAlloc(type_r);
                                                            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                           "---read_bw: " + STR(read_bw) +
                                                                               " written_bw: " + STR(written_bw));
                                                            if(mr->op0->index == mr_used->op0->index &&
                                                               written_bw == read_bw &&
                                                               tree_helper::IsSameType(
                                                                   type_r,
                                                                   type_w)) /// TODO in case read and write values are
                                                                            /// integers but of different signedness a
                                                                            /// cast could allow the load/store
                                                                            /// simplification
                                                            {
                                                               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                              "---found a candidate " +
                                                                                  use.first->ToString());
                                                               /// check if this load is killed by a following vover
                                                               auto curr_stmt = stmt;
                                                               bool found_load = false;
                                                               if(curr_stmt != stmt_list.rbegin())
                                                               {
                                                                  --curr_stmt;
                                                                  while(true)
                                                                  {
                                                                     if((*curr_stmt)->index == use.first->index)
                                                                     {
                                                                        found_load = true;
                                                                        break;
                                                                     }
                                                                     const auto gn_curr =
                                                                         GetPointerS<gimple_node>(*curr_stmt);
                                                                     if(!found_load && gn_curr->vdef &&
                                                                        (ga_used->vuses.find(gn_curr->vdef) !=
                                                                             ga_used->vuses.end() ||
                                                                         (vdefvover_map.find(ssaDef->index) !=
                                                                              vdefvover_map.end() &&
                                                                          vdefvover_map.find(ssaDef->index)
                                                                                  ->second.find(gn_curr->vdef->index) !=
                                                                              vdefvover_map.find(ssaDef->index)
                                                                                  ->second.end())))
                                                                     {
                                                                        break;
                                                                     }
                                                                     if(curr_stmt == stmt_list.rbegin())
                                                                     {
                                                                        break;
                                                                     }
                                                                     else
                                                                     {
                                                                        --curr_stmt;
                                                                     }
                                                                  }
                                                               }
                                                               if(found_load)
                                                               {
                                                                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                                 "---found a Dead Load " +
                                                                                     (*curr_stmt)->ToString());
                                                                  const auto ssa_used_op0 =
                                                                      GetPointerS<ssa_name>(ga_used->op0);
                                                                  const TreeNodeMap<size_t> StmtUsesOp0 =
                                                                      ssa_used_op0->CGetUseStmts();
                                                                  for(const auto& useop0 : StmtUsesOp0)
                                                                  {
                                                                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC,
                                                                                    debug_level,
                                                                                    "---replace var usage before: " +
                                                                                        useop0.first->ToString());
                                                                     TM->ReplaceTreeNode(useop0.first, ga_used->op0,
                                                                                         ga->op1);
                                                                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC,
                                                                                    debug_level,
                                                                                    "---replace var usage after: " +
                                                                                        useop0.first->ToString());
                                                                  }
                                                                  THROW_ASSERT(ssa_used_op0->CGetNumberUses() == 0,
                                                                               "unexpected condition");
                                                                  stmts_to_be_removed.push_back(*curr_stmt);
                                                                  AppM->RegisterTransformation(GetName(), *curr_stmt);
                                                                  restart_mem = true;
                                                               }
                                                            }
                                                         }
                                                      }
                                                   }
                                                }
                                             }
                                          }
                                          else
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---local variable later used");
                                          }
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---non local variable");
                                       }
                                    }
                                    else
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non var decl");
                                    }
                                 }
                                 else
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern1");
                                 }
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern2");
                              }
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern3");
                           }
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non-null offset in the mem_ref");
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS not ssa");
                  }
               }
            }
            else if((*stmt)->get_kind() == gimple_cond_K)
            {
               const auto gc = GetPointerS<gimple_cond>(*stmt);
               if(gc->op0->get_kind() == integer_cst_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_cond with a constant condition");
                  do_reachability = true;
                  restart_if_opt = true;
                  if(tree_helper::GetConstValue(gc->op0))
                  {
                     const auto new_bb = move2emptyBB(TM, get_new_bbi(), sl, bb, bb->false_edge, bb->true_edge);
                     new_bbs.push_back(new_bb);
                     bb->false_edge = new_bb->number;
                  }
                  else
                  {
                     const auto new_bb = move2emptyBB(TM, get_new_bbi(), sl, bb, bb->true_edge, bb->false_edge);
                     new_bbs.push_back(new_bb);
                     bb->true_edge = new_bb->number;
                  }
               }
            }
            else if((*stmt)->get_kind() == gimple_multi_way_if_K)
            {
               const auto gm = GetPointerS<gimple_multi_way_if>(*stmt);

               bool one_is_const = false;
               bool all_false = true;
               unsigned int bb_dest = 0;
               for(const auto& cond : gm->list_of_cond)
               {
                  if(!cond.first)
                  {
                     if(all_false)
                     {
                        THROW_ASSERT(!one_is_const, "only one can be true");
                        bb_dest = cond.second;
                     }
                  }
                  else if(cond.first->get_kind() == integer_cst_K)
                  {
                     if(tree_helper::GetConstValue(cond.first))
                     {
                        all_false = false;
                        THROW_ASSERT(!one_is_const, "only one can be true");
                        one_is_const = true;
                        bb_dest = cond.second;
                     }
                  }
                  else
                  {
                     all_false = false;
                  }
               }
               if(all_false || one_is_const)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---gimple_multi_way_if with constant conditions");
                  restart_mwi_opt = true;
                  for(auto& cond : gm->list_of_cond)
                  {
                     if(cond.second != bb_dest)
                     {
                        do_reachability = true;
                        const auto new_bb = move2emptyBB(TM, get_new_bbi(), sl, bb, cond.second, bb_dest);
                        new_bbs.push_back(new_bb);
                        cond.second = new_bb->number;
                     }
                  }
               }
               else
               {
                  /// remove same conditions
                  std::map<unsigned int, unsigned int> condIndex2BBdest;
                  auto do0ConstantCondRemoval = false;
                  for(auto& cond : gm->list_of_cond)
                  {
                     if(cond.first)
                     {
                        if(cond.first->get_kind() == integer_cst_K)
                        {
                           if(!blocks.at(cond.second)->CGetStmtList().empty())
                           {
                              do0ConstantCondRemoval = true;
                           }
                        }
                        else if(condIndex2BBdest.find(cond.first->index) == condIndex2BBdest.end())
                        {
                           condIndex2BBdest[cond.first->index] = cond.second;
                        }
                        else if(!blocks.at(cond.second)->CGetStmtList().empty())
                        {
                           do_reachability = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---gimple_multi_way_if duplicated condition from BB" + STR(bb->number) +
                                              " to BB" + STR(cond.second));
                           const auto new_bb = move2emptyBB(TM, get_new_bbi(), sl, bb, cond.second,
                                                            condIndex2BBdest.at(cond.first->index));
                           new_bbs.push_back(new_bb);
                           cond.second = new_bb->number;
                        }
                     }
                  }
                  if(do0ConstantCondRemoval)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---gimple_multi_way_if do zero condition removal");
                     const auto bb0_dest = condIndex2BBdest.begin()->second;
                     for(auto& cond : gm->list_of_cond)
                     {
                        if(cond.first)
                        {
                           if(cond.first->get_kind() == integer_cst_K)
                           {
                              THROW_ASSERT(tree_helper::GetConstValue(cond.first) == 0, "unexpected condition");
                              do_reachability = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---gimple_multi_way_if duplicated condition from BB" + STR(bb->number) +
                                                 " to BB" + STR(cond.second));
                              const auto new_bb = move2emptyBB(TM, get_new_bbi(), sl, bb, cond.second, bb0_dest);
                              new_bbs.push_back(new_bb);
                              cond.second = new_bb->number;
                           }
                        }
                     }
                  }
               }
            }
            else if((*stmt)->get_kind() == gimple_call_K)
            {
               const auto gc = GetPointerS<gimple_call>(*stmt);
               auto temp_node = gc->fn;
               function_decl* fdCalled = nullptr;

               if(temp_node->get_kind() == addr_expr_K)
               {
                  const auto ue = GetPointerS<unary_expr>(temp_node);
                  temp_node = ue->op;
                  fdCalled = GetPointer<function_decl>(temp_node);
               }
               else if(temp_node->get_kind() == obj_type_ref_K)
               {
                  temp_node = tree_helper::find_obj_type_ref_function(gc->fn);
                  fdCalled = GetPointer<function_decl>(temp_node);
               }
               if(fdCalled)
               {
                  bool is_a_writing_memory_call = false;
                  if(fdCalled->writing_memory or !fdCalled->body)
                  {
                     is_a_writing_memory_call = true;
                  }
                  if(tree_helper::is_a_nop_function_decl(fdCalled) || !is_a_writing_memory_call)
                  {
                     if(gc->vdef || gc->vuses.size() || gc->vovers.size())
                     {
                        restart_mem = true;
                     }
                     add_gimple_nop(TM, *stmt, bb);
                     stmts_to_be_removed.push_back(*stmt);
                     AppM->RegisterTransformation(GetName(), *stmt);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                  }
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not gimple_assign statement");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
         }
         if(!stmts_to_be_removed.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Restart dead code");
            modified = true;
            restart_analysis = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Removing " + STR(stmts_to_be_removed.size()) + " dead statements");
            for(const auto& curr_el : stmts_to_be_removed)
            {
               bb->RemoveStmt(curr_el, AppM);
               fix_sdc_motion(curr_el);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_el->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead statements");
            for(const auto& vssa_nop : new_vssa_nop)
            {
               bb->PushFront(vssa_nop, AppM);
            }
            if(bb->CGetStmtList().empty() && bb->CGetPhiList().empty())
            {
               restart_if_opt = true;
               restart_mwi_opt = true;
            }
         }
         /*
          * check also phi operations. if a phi assigns an ssa which is not used
          * anymore, the phi can be removed
          */
         const auto& phi_list = bb->CGetPhiList();
         std::list<tree_nodeRef> phis_to_be_removed;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phis");
         for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*phi)->ToString());
            THROW_ASSERT((*phi)->get_kind() == gimple_phi_K,
                         (*phi)->ToString() + " is of kind " + tree_node::GetString((*phi)->get_kind()));
            const auto gphi = GetPointerS<gimple_phi>(*phi);
            const auto res = gphi->res;
            THROW_ASSERT(res->get_kind() == ssa_name_K,
                         res->ToString() + " is of kind " + tree_node::GetString(res->get_kind()));
            const auto ssa = GetPointerS<ssa_name>(res);
            // very strict condition for the elimination
            if(ssa->CGetNumberUses() == 0 && ssa->CGetDefStmts().size() == 1)
            {
               THROW_ASSERT(ssa->CGetUseStmts().empty(), "");
               phis_to_be_removed.push_back(*phi);
               AppM->RegisterTransformation(GetName(), *phi);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phi");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phis");
         if(!phis_to_be_removed.empty())
         {
            modified = true;
            restart_analysis = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Removing " + STR(phis_to_be_removed.size()) + " dead phis");
            for(const auto& curr_phi : phis_to_be_removed)
            {
               bb->RemovePhi(curr_phi);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_phi->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead phis");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + STR(bb->number));
      }
      for(const auto& bb : new_bbs)
      {
         sl->list_of_bloc[bb->number] = bb;
      }
      new_bbs.clear();
      while(do_reachability)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
         do_reachability = false;
         CustomOrderedSet<unsigned> bb_to_remove;
         CustomOrderedSet<unsigned> BB_reached;
         BB_reached.insert(bloc::ENTRY_BLOCK_ID);
         std::queue<unsigned> to_be_processed;
         to_be_processed.push(bloc::ENTRY_BLOCK_ID);
         while(!to_be_processed.empty())
         {
            auto curr = to_be_processed.front();
            to_be_processed.pop();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyzing BB" + STR(curr));
            for(auto bb : blocks.at(curr)->list_of_succ)
            {
               if(BB_reached.insert(bb).second)
               {
                  to_be_processed.push(bb);
               }
            }
         }
         BB_reached.insert(bloc::EXIT_BLOCK_ID);
         for(const auto& bb_pair : blocks)
         {
            if(BB_reached.find(bb_pair.first) == BB_reached.end())
            {
               bb_to_remove.insert(bb_pair.first);
            }
         }
         if(!bb_to_remove.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Removing " + STR(bb_to_remove.size()) + " unreachable BBs");
            for(auto bbi : bb_to_remove)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removing BB" + STR(bbi));
               do_reachability = true;
               const auto& bb = blocks.at(bbi);
               const auto phi_list = bb->CGetPhiList();
               std::list<tree_nodeRef> phis_to_be_removed;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis");
               for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + (*phi)->ToString());
                  THROW_ASSERT((*phi)->get_kind() == gimple_phi_K,
                               (*phi)->ToString() + " is of kind " + (*phi)->get_kind_text());
                  const auto gphi = GetPointerS<gimple_phi>(*phi);
                  const auto res = gphi->res;
                  THROW_ASSERT(res->get_kind() == ssa_name_K, res->ToString() + " is of kind " + res->get_kind_text());
                  const auto ssa = GetPointerS<const ssa_name>(res);
                  if(ssa->virtual_flag)
                  {
                     kill_vdef(TM, gphi->res);
                  }
                  else
                  {
                     kill_uses(TM, gphi->res);
                  }
                  bb->RemovePhi(*phi);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed phis");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing statements");
               const auto stmt_list = bb->CGetStmtList();
               for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + (*stmt)->ToString());
                  const auto gn = GetPointerS<gimple_node>(*stmt);
                  if(gn->vdef)
                  {
                     kill_vdef(TM, gn->vdef);
                     gn->vdef = nullptr;
                  }
                  else if((*stmt)->get_kind() == gimple_assign_K)
                  {
                     const auto ga = GetPointerS<gimple_assign>(*stmt);
                     if(ga->op0->get_kind() == ssa_name_K)
                     {
                        kill_uses(TM, ga->op0);
                     }
                  }
                  bb->RemoveStmt(*stmt, AppM);
                  fix_sdc_motion(*stmt);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed statements");
               if(bb->CGetStmtList().empty() && bb->CGetPhiList().empty())
               {
                  restart_if_opt = true;
                  restart_mwi_opt = true;
               }
               for(const auto sblock : bb->list_of_succ)
               {
                  if(sblock == bloc::EXIT_BLOCK_ID)
                  {
                     continue;
                  }
                  THROW_ASSERT(blocks.find(sblock) != blocks.end(), "Already removed BB" + STR(sblock));
                  const auto& succ_block = blocks.at(sblock);
                  succ_block->list_of_pred.erase(
                      std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb->number));
                  /// Fix PHIs
                  for(const auto& phi : succ_block->CGetPhiList())
                  {
                     const auto gp = GetPointerS<gimple_phi>(phi);
                     for(const auto& def_edge : gp->CGetDefEdgesList())
                     {
                        if(def_edge.second == bb->number)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Removing <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) +
                                              ">");
                           gp->RemoveDefEdge(TM, def_edge);
                           break;
                        }
                     }
                  }
               }
               bb->list_of_succ.clear();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed BB" + STR(bbi));
            }
            for(const auto bbi : bb_to_remove)
            {
               sl->list_of_bloc.erase(bbi);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed unreachable BBs");
         }
      }
   } while(restart_analysis);

   /// fix vdef
   for(const auto& block : blocks)
   {
      const auto& bb = block.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
      const auto& stmt_list = bb->CGetStmtList();
      std::list<tree_nodeRef> new_vssa_nop;
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         const auto stmt_kind = (*stmt)->get_kind();
         const function_decl* fdCalled = nullptr;
         if(stmt_kind == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(*stmt);
            const auto rhs_kind = ga->op1->get_kind();
            if(rhs_kind == call_expr_K || rhs_kind == aggr_init_expr_K)
            {
               const auto ce = GetPointerS<const call_expr>(ga->op1);
               if(ce->fn->get_kind() == addr_expr_K)
               {
                  const auto addr_node = ce->fn;
                  const auto ae = GetPointerS<const addr_expr>(addr_node);
                  const auto fu_decl_node = ae->op;
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) +
                                                                                " is not function_decl but " +
                                                                                fu_decl_node->get_kind_text());
                  fdCalled = GetPointer<const function_decl>(fu_decl_node);
               }
            }
         }
         else if(stmt_kind == gimple_call_K)
         {
            const auto gc = GetPointerS<const gimple_call>(*stmt);
            const auto op_kind = gc->fn->get_kind();
            if(op_kind == addr_expr_K)
            {
               const auto ue = GetPointerS<const unary_expr>(gc->fn);
               fdCalled = GetPointer<const function_decl>(ue->op);
            }
            else if(op_kind == obj_type_ref_K)
            {
               const auto obj_ref = tree_helper::find_obj_type_ref_function(gc->fn);
               fdCalled = GetPointer<const function_decl>(obj_ref);
            }
         }
         if(fdCalled)
         {
            const auto gn = GetPointerS<gimple_node>(*stmt);
            if(!fdCalled->writing_memory && fdCalled->body)
            {
               if(gn->vdef)
               {
                  if(fdCalled->reading_memory)
                  {
                     /// all vovers become vuse
                     for(const auto& vo : gn->vovers)
                     {
                        if(!gn->vdef || (vo->index != gn->vdef->index))
                        {
                           if(gn->AddVuse(vo))
                           {
                              THROW_ASSERT(vo->get_kind() == ssa_name_K, "");
                              GetPointerS<ssa_name>(vo)->AddUseStmt(*stmt);
                           }
                        }
                     }
                  }
                  /// fix vdef
                  new_vssa_nop.push_back(kill_vdef(TM, gn->vdef));
                  gn->vdef = nullptr;
                  for(const auto& vo : gn->vovers)
                  {
                     THROW_ASSERT(vo->get_kind() == ssa_name_K, "");
                     GetPointerS<ssa_name>(vo)->RemoveUse(*stmt);
                  }
                  gn->vovers.clear();
                  restart_mem = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Nothing is written by this function: Fixed VDEF/VOVER ");
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
      }
      for(const auto& vssa_nop : new_vssa_nop)
      {
         bb->PushFront(vssa_nop, AppM);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + STR(bb->number));
   }

   /// update function memory write flag
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Update memory read/write flag");
   fd->writing_memory = false;
   fd->reading_memory = false;
   for(const auto& block : blocks)
   {
      const auto& bb = block.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
      const auto& stmt_list = bb->CGetStmtList();
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         const auto gn = GetPointerS<const gimple_node>(*stmt);

         if(!gn->vuses.empty() && (*stmt)->get_kind() != gimple_return_K)
         {
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (1)");
         }
         if(gn->vdef)
         {
            fd->writing_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (3)");
         }
         else if(const auto ga = GetPointer<const gimple_assign>(*stmt))
         {
            if(ga->op1->get_kind() == call_expr_K || ga->op1->get_kind() == aggr_init_expr_K)
            {
               const auto ce = GetPointerS<const call_expr>(ga->op1);
               if(ce->fn->get_kind() == addr_expr_K)
               {
                  const auto addr_node = ce->fn;
                  const auto ae = GetPointerS<const addr_expr>(addr_node);
                  const auto fu_decl_node = ae->op;
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) +
                                                                                " is not function_decl but " +
                                                                                fu_decl_node->get_kind_text());
                  const auto fdCalled = GetPointerS<const function_decl>(fu_decl_node);
                  if(fdCalled->writing_memory || !fdCalled->body || fdCalled->undefined_flag)
                  {
                     fd->writing_memory = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (5)");
                  }
                  if(fdCalled->reading_memory || !fdCalled->body || fdCalled->undefined_flag)
                  {
                     fd->reading_memory = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (6)");
                  }
               }
               else
               {
                  fd->writing_memory = true; /// conservative analysis
                  fd->reading_memory = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory+writing_memory (7)");
               }
            }
         }
         else if(const auto gc = GetPointer<const gimple_call>(*stmt))
         {
            const function_decl* fdCalled = nullptr;
            if(gc->fn->get_kind() == addr_expr_K)
            {
               const auto ue = GetPointerS<const unary_expr>(gc->fn);
               fdCalled = GetPointer<const function_decl>(ue->op);
            }
            else if(gc->fn->get_kind() == obj_type_ref_K)
            {
               const auto obj_ref = tree_helper::find_obj_type_ref_function(gc->fn);
               fdCalled = GetPointerS<const function_decl>(gc->fn);
            }
            if(fdCalled)
            {
               if(fdCalled->writing_memory || !fdCalled->body || fdCalled->undefined_flag)
               {
                  fd->writing_memory = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (8)");
               }
               if(fdCalled->reading_memory || !fdCalled->body || fdCalled->undefined_flag)
               {
                  fd->reading_memory = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (9)");
               }
            }
            else
            {
               fd->writing_memory = true;
               fd->reading_memory = true;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory+writing_memory (10)");
            }
         }
         else if(GetPointer<const gimple_asm>(*stmt))
         {
            fd->writing_memory = true; /// more conservative than really needed
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory+writing_memory (11)");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + STR(bb->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---write flag " + (fd->writing_memory ? std::string("T") : std::string("F")));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---read flag " + (fd->reading_memory ? std::string("T") : std::string("F")));

   const auto calledSet = AppM->CGetCallGraphManager()->get_called_by(function_id);
   for(const auto i : calledSet)
   {
      const auto fdCalled = GetPointerS<const function_decl>(AppM->get_tree_manager()->GetTreeNode(i));
      last_writing_memory[i] = fdCalled->writing_memory;
      last_reading_memory[i] = fdCalled->reading_memory;
   }

   if(restart_mem || modified || restart_if_opt || restart_mwi_opt)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
}
