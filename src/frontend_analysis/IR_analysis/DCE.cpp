/**
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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file DCE.cpp
 * @brief This file implements dead code elimination.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "DCE.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "sdc_scheduling_base.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "var_pp_functor.hpp"

#include <fstream>
#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>

DCE::DCE(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
         const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DCE_PASS, _design_flow_manager, _parameters),
      restart_if_opt(false),
      restart_mwi_opt(false),
      restart_mem(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
DCE::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         relationships.insert(std::make_pair(DCE_PASS, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, CALLING_FUNCTIONS));
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
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
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
            if(restart_mwi_opt)
            {
               relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
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

bool DCE::HasToBeExecuted() const
{
   if(FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return true;
   }
   std::map<unsigned int, bool> cur_writing_memory;
   std::map<unsigned int, bool> cur_reading_memory;
   const auto TM = AppM->get_ir_manager();
   for(const auto i : AppM->CGetCallGraphManager().get_called_by(function_id))
   {
      const auto curr_tn = TM->GetIRNode(i);
      const auto fdCalled = GetPointerS<const function_val_node>(curr_tn);
      cur_writing_memory[i] = fdCalled->writing_memory;
      cur_reading_memory[i] = fdCalled->reading_memory;
   }
   return cur_writing_memory != last_writing_memory || cur_reading_memory != last_reading_memory;
}

void DCE::fix_sdc_motion(const DesignFlowManager& design_flow_manager, unsigned int function_id, ir_nodeRef removedStmt)
{
   const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
   const auto sdc_scheduling_step = design_flow_manager.GetDesignFlowStep(HLSFunctionStep::ComputeSignature(
       HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), function_id));
   if(sdc_scheduling_step != DesignFlowGraph::null_vertex())
   {
      const auto sdc_scheduling =
          GetPointer<SDCScheduling_base>(design_flow_graph->CGetNodeInfo(sdc_scheduling_step)->design_flow_step);
      const auto removed_index = removedStmt->index;
      sdc_scheduling->movements_list.remove_if(
          [&](const std::vector<unsigned int>& mv) { return mv[0] == removed_index; });
   }
}

void DCE::fix_sdc_motion(ir_nodeRef removedStmt) const
{
   return fix_sdc_motion(design_flow_manager, function_id, removedStmt);
}

void DCE::kill_uses(const ir_managerRef& TM, const ir_nodeRef& op0) const
{
   THROW_ASSERT(op0->get_kind() == ssa_node_K, "expected a ssa_node object");
   const auto ssa = GetPointerS<ssa_node>(op0);

   if(ssa->CGetNumberUses() != 0)
   {
      const auto ssa_type = ir_helper::CGetType(op0);
      ir_nodeRef val;
      if(ir_helper::IsRealType(ssa_type))
      {
         val = TM->CreateUniqueRealCst(0.l, ssa_type);
      }
      else if(ir_helper::IsVectorType(ssa_type))
      {
         const auto ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
         const auto utype = ir_man->GetUnsignedIntegerType();
         const auto zeroVal = TM->CreateUniqueIntegerCst(0LL, utype);
         ir_manager::IRSchema ne_schema;
         ne_schema[TOK(TOK_TYPE)] = STR(ssa_type->index);
         ne_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
         ne_schema[TOK(TOK_OP)] = STR(zeroVal->index);
         val = TM->create_ir_node(nop_node_K, ne_schema);
      }
      else
      {
         val = TM->CreateUniqueIntegerCst(0LL, ssa_type);
      }
      const IRNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
      for(const auto& use : StmtUses)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---replace constant usage before: " + use.first->ToString());
         TM->ReplaceIRNode(use.first, op0, val);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---replace constant usage after: " + use.first->ToString());
      }
      THROW_ASSERT(ssa->CGetNumberUses() == 0, "unexpected condition");
   }
}

ir_nodeRef DCE::kill_vdef(const ir_managerRef& TM, const ir_nodeRef& vdef)
{
   const auto v_ssa = GetPointerS<ssa_node>(vdef);
   const auto function_id = GetPointerS<node_stmt>(v_ssa->GetDefStmt())->parent->index;
   ir_manager::IRSchema nop_stmt_schema;
   nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
   const auto nop_stmt = TM->create_ir_node(nop_stmt_K, nop_stmt_schema);
   GetPointerS<node_stmt>(nop_stmt)->vdef = vdef;
   v_ssa->SetDefStmt(nop_stmt);
   return nop_stmt;
}

ir_nodeRef DCE::add_nop_stmt(const ir_managerRef& TM, const ir_nodeRef& cur_stmt, const blocRef& bb)
{
   const auto gn = GetPointer<node_stmt>(cur_stmt);
   THROW_ASSERT(gn, "");
   ir_manager::IRSchema nop_stmt_IR_schema;
   nop_stmt_IR_schema[TOK(TOK_IR_LOCINFO)] =
       gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
   nop_stmt_IR_schema[TOK(TOK_PARENT)] = STR(function_id);
   const auto nop_stmt = TM->create_ir_node(nop_stmt_K, nop_stmt_IR_schema);
   const auto new_gn = GetPointerS<node_stmt>(nop_stmt);
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

blocRef DCE::move2emptyBB(const ir_managerRef& TM, const unsigned int new_bbi, const statement_list_node* sl,
                          const blocRef& bb_pred, const unsigned int cand_bb_dest,
                          const unsigned int bb_dest_number) const
{
   const auto& bb_succ = sl->list_of_bloc.at(cand_bb_dest);
   const auto& bb_dest = sl->list_of_bloc.at(bb_dest_number);

   /// Create empty basic block
   const auto bb_new = blocRef(new bloc(new_bbi));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Created BB" + STR(bb_new->number) + " as new successor of BB" + STR(bb_pred->number));

   bb_new->loop_id = bb_pred->loop_id;
   bb_new->SetSSAUsesComputed();
   bb_new->schedule = bb_pred->schedule;

   bb_new->list_of_pred.push_back(bb_pred->number);
   bb_new->list_of_succ.push_back(bb_dest->number);
   bb_dest->list_of_pred.push_back(bb_new->number);
   bb_pred->list_of_succ.erase(std::find(bb_pred->list_of_succ.begin(), bb_pred->list_of_succ.end(), bb_succ->number));
   bb_pred->list_of_succ.push_back(bb_new->number);

   bb_succ->list_of_pred.erase(std::find(bb_succ->list_of_pred.begin(), bb_succ->list_of_pred.end(), bb_pred->number));
   for(const auto& phi : bb_succ->CGetPhiList())
   {
      const auto gp = GetPointerS<phi_stmt>(phi);
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
      const auto gp = GetPointerS<phi_stmt>(phi);
      phi_stmt::DefEdgeList new_defedges;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_pred->number)
         {
            new_defedges.push_back(phi_stmt::DefEdge(def_edge.first, bb_new->number));
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

DesignFlowStep_Status DCE::InternalExec()
{
   if(parameters->IsParameter("disable-dce") && parameters->GetParameter<unsigned int>("disable-dce") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const auto TM = AppM->get_ir_manager();
   auto fd = GetPointerS<function_val_node>(TM->GetIRNode(function_id));
   auto sl = GetPointerS<statement_list_node>(fd->body);
   auto& listOfBB = sl->list_of_bloc;

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
         return listOfBB.rbegin()->first + 1U + static_cast<unsigned int>(new_bbs.size());
      };
      CustomUnorderedMap<unsigned, CustomOrderedSet<unsigned>> vdefvover_map;
      for(const auto& block : listOfBB)
      {
         const auto& stmt_list = block.second->CGetStmtList();
         for(const auto& stmt : stmt_list)
         {
            const auto gn = GetPointerS<node_stmt>(stmt);
            THROW_ASSERT(gn->vovers.empty() || gn->vdef, "unexpected condition");
            for(const auto& vo : gn->vovers)
            {
               vdefvover_map[vo->index].insert(gn->vdef->index);
            }
         }
      }
      for(const auto& block : listOfBB)
      {
         const auto& bb = block.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
         const auto& stmt_list = bb->CGetStmtList();
         std::list<ir_nodeRef> new_vssa_nop;
         std::set<ir_nodeRef> stmts_to_be_removed;
         for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
            if((*stmt)->get_kind() == assign_stmt_K)
            {
               const auto ga = GetPointerS<assign_stmt>(*stmt);
               if(ga->predicate && ga->predicate->get_kind() == constant_int_val_node_K &&
                  ir_helper::GetConstValue(ga->predicate) == 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead predicate found");
                  if(ga->vdef)
                  {
                     new_vssa_nop.push_back(kill_vdef(TM, ga->vdef));
                     ga->vdef = nullptr;
                     restart_mem = true;
                  }
                  else if(ga->op0->get_kind() == ssa_node_K)
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
                  stmts_to_be_removed.insert(*stmt);
                  AppM->RegisterTransformation(GetName(), *stmt);
               }
               else
               {
                  /// op0 is the left side of the assignment, op1 is the right side
                  const auto op0 = ga->op0;
                  const auto op1_type = ir_helper::CGetType(ga->op1);
                  if(op0->get_kind() == ssa_node_K)
                  {
                     const auto ssa = GetPointerS<ssa_node>(op0);
                     /// very strict condition for DCE
                     if(ssa->CGetNumberUses() == 0)
                     {
                        bool is_a_writing_memory_call = false;
                        bool is_a_reading_memory_call = false;
                        if(ga->op1->get_kind() == call_node_K)
                        {
                           const auto ce = GetPointerS<call_node>(ga->op1);
                           if(ce->fn->get_kind() == addr_node_K)
                           {
                              const auto ae = GetPointerS<const addr_node>(ce->fn);
                              const auto fu_decl_node = ae->op;
                              THROW_ASSERT(fu_decl_node->get_kind() == function_val_node_K,
                                           "node  " + STR(fu_decl_node) + " is not function_val_node but " +
                                               fu_decl_node->get_kind_text());
                              const auto fdCalled = GetPointerS<const function_val_node>(fu_decl_node);
                              if(fdCalled->writing_memory || !fdCalled->body)
                              {
                                 is_a_writing_memory_call = true;
                              }
                              if(fdCalled->reading_memory || !fdCalled->body)
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
                              add_nop_stmt(TM, *stmt, bb);
                              restart_mem = true;
                           }
                           if(is_a_reading_memory_call || ga->vdef || ga->vuses.size() || ga->vovers.size() ||
                              ga->op1->get_kind() == addr_node_K || ga->op1->get_kind() == mem_access_node_K)
                           {
                              restart_mem = true;
                           }
                           stmts_to_be_removed.insert(*stmt);
                           AppM->RegisterTransformation(GetName(), *stmt);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---LHS ssa used: " + STR(ssa->CGetNumberUses()));
                     }
                  }
                  else if(op0->get_kind() == mem_access_node_K && !ga->artificial && !ir_helper::IsVectorType(op1_type))
                  {
                     const auto mr = GetPointerS<mem_access_node>(op0);
                     const auto type_w = ir_helper::CGetType(ga->op1);
                     const auto written_bw = ir_helper::SizeAlloc(type_w);

                     if(mr->op->get_kind() == constant_int_val_node_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---do nothing with constant values");
                     }
                     else if(ga->predicate && ga->predicate->get_kind() != constant_int_val_node_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---do nothing with non-constant predicates");
                     }
                     else
                     {
                        THROW_ASSERT(mr->op->get_kind() == ssa_node_K, "unexpected condition" + ga->ToString());
                        auto derefVar = GetPointerS<ssa_node>(mr->op);
                        auto defStmt = derefVar->GetDefStmt();
                        if(defStmt->get_kind() == assign_stmt_K)
                        {
                           const auto derefGA = GetPointerS<assign_stmt>(defStmt);
                           if(derefGA->op1->get_kind() == addr_node_K)
                           {
                              const auto addressedVar = GetPointerS<addr_node>(derefGA->op1)->op;
                              if(addressedVar->get_kind() == variable_val_node_K)
                              {
                                 const auto varDecl = GetPointerS<variable_val_node>(addressedVar);
                                 if(varDecl->parent && function_id == varDecl->parent->index && !varDecl->static_flag)
                                 {
                                    ssa_node* ssaDef = nullptr;
                                    if(ga->vdef)
                                    {
                                       ssaDef = GetPointerS<ssa_node>(ga->vdef);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---VDEF: " + STR(ga->vdef));
                                    }
                                    else
                                    {
                                       THROW_ERROR("unexpected condition");
                                    }
                                    THROW_ASSERT(ssaDef, "unexpected condition");

                                    if(ssaDef->CGetNumberUses() == 0 &&
                                       vdefvover_map.find(ssaDef->index) == vdefvover_map.end())
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead Store found");
                                       if(ga->vdef)
                                       {
                                          add_nop_stmt(TM, *stmt, bb);
                                       }
                                       stmts_to_be_removed.insert(*stmt);
                                       AppM->RegisterTransformation(GetName(), *stmt);
                                       restart_mem = true;
                                    }
                                    else
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---check if the associated load are dead");
                                       const IRNodeMap<size_t> StmtUses = ssaDef->CGetUseStmts();
                                       for(const auto& use : StmtUses)
                                       {
                                          const auto gn_used = GetPointerS<node_stmt>(use.first);
                                          if(!gn_used->vdef && gn_used->bb_index == ga->bb_index &&
                                             gn_used->get_kind() == assign_stmt_K)
                                          {
                                             const auto ga_used = GetPointerS<assign_stmt>(use.first);
                                             if(ga_used->op0->get_kind() == ssa_node_K &&
                                                ga_used->op1->get_kind() == mem_access_node_K &&
                                                !(ga_used->predicate &&
                                                  ga_used->predicate->get_kind() == constant_int_val_node_K &&
                                                  ir_helper::GetConstValue(ga_used->predicate) == 0))
                                             {
                                                const auto mr_used = GetPointerS<mem_access_node>(ga_used->op1);

                                                const auto type_r = ir_helper::CGetType(ga_used->op0);
                                                const auto read_bw = ir_helper::SizeAlloc(type_r);
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---read_bw: " + STR(read_bw) +
                                                                   " written_bw: " + STR(written_bw));
                                                if(mr->op->index == mr_used->op->index && written_bw == read_bw &&
                                                   ir_helper::IsSameType(type_r,
                                                                         type_w)) /// TODO in case read and write values
                                                                                  /// are integers but of different
                                                                                  /// signedness a cast could allow the
                                                                                  /// load/store simplification
                                                {
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                  "---found a candidate " + use.first->ToString());
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
                                                         const auto gn_curr = GetPointerS<node_stmt>(*curr_stmt);
                                                         if(!found_load && gn_curr->vdef &&
                                                            (ga_used->vuses.find(gn_curr->vdef) !=
                                                                 ga_used->vuses.end() ||
                                                             (vdefvover_map.find(ssaDef->index) !=
                                                                  vdefvover_map.end() &&
                                                              vdefvover_map.find(ssaDef->index)
                                                                      ->second.find(gn_curr->vdef->index) !=
                                                                  vdefvover_map.find(ssaDef->index)->second.end())))
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
                                                      const auto ssa_used_op0 = GetPointerS<ssa_node>(ga_used->op0);
                                                      const IRNodeMap<size_t> StmtUsesOp0 =
                                                          ssa_used_op0->CGetUseStmts();
                                                      for(const auto& useop0 : StmtUsesOp0)
                                                      {
                                                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                        "---replace var usage before: " +
                                                                            useop0.first->ToString());
                                                         TM->ReplaceIRNode(useop0.first, ga_used->op0, ga->op1);
                                                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                        "---replace var usage after: " +
                                                                            useop0.first->ToString());
                                                      }
                                                      THROW_ASSERT(ssa_used_op0->CGetNumberUses() == 0,
                                                                   "unexpected condition");
                                                      stmts_to_be_removed.insert(*curr_stmt);
                                                      AppM->RegisterTransformation(GetName(), *curr_stmt);
                                                      restart_mem = true;
                                                   }
                                                }
                                             }
                                          }
                                       }
                                    }
                                 }
                                 else
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non local variable");
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
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS not ssa");
                  }
               }
            }
            else if((*stmt)->get_kind() == multi_way_if_stmt_K)
            {
               const auto gm = GetPointerS<multi_way_if_stmt>(*stmt);

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
                  else if(cond.first->get_kind() == constant_int_val_node_K)
                  {
                     if(ir_helper::GetConstValue(cond.first))
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
                                 "---multi_way_if_stmt with constant conditions");
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
                        if(cond.first->get_kind() == constant_int_val_node_K)
                        {
                           if(!listOfBB.at(cond.second)->CGetStmtList().empty())
                           {
                              do0ConstantCondRemoval = true;
                           }
                        }
                        else if(condIndex2BBdest.find(cond.first->index) == condIndex2BBdest.end())
                        {
                           condIndex2BBdest[cond.first->index] = cond.second;
                        }
                        else if(!listOfBB.at(cond.second)->CGetStmtList().empty())
                        {
                           do_reachability = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---multi_way_if_stmt duplicated condition from BB" + STR(bb->number) +
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
                                    "---multi_way_if_stmt do zero condition removal");
                     const auto bb0_dest = condIndex2BBdest.begin()->second;
                     for(auto& cond : gm->list_of_cond)
                     {
                        if(cond.first)
                        {
                           if(cond.first->get_kind() == constant_int_val_node_K)
                           {
                              THROW_ASSERT(ir_helper::GetConstValue(cond.first) == 0, "unexpected condition");
                              do_reachability = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---multi_way_if_stmt duplicated condition from BB" + STR(bb->number) +
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
            else if((*stmt)->get_kind() == call_stmt_K)
            {
               const auto gc = GetPointerS<call_stmt>(*stmt);
               if(gc->predicate && gc->predicate->get_kind() == constant_int_val_node_K &&
                  ir_helper::GetConstValue(gc->predicate) == 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead predicate found");
                  if(gc->vdef || gc->vuses.size() || gc->vovers.size())
                  {
                     restart_mem = true;
                  }
                  if(gc->vdef)
                  {
                     add_nop_stmt(TM, *stmt, bb);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                  stmts_to_be_removed.insert(*stmt);
                  AppM->RegisterTransformation(GetName(), *stmt);
               }
               else
               {
                  auto temp_node = gc->fn;
                  function_val_node* fdCalled = nullptr;

                  if(temp_node->get_kind() == addr_node_K)
                  {
                     const auto ue = GetPointerS<unary_node>(temp_node);
                     temp_node = ue->op;
                     fdCalled = GetPointer<function_val_node>(temp_node);
                  }
                  if(fdCalled)
                  {
                     bool is_a_writing_memory_call = false;
                     if(fdCalled->writing_memory || !fdCalled->body)
                     {
                        is_a_writing_memory_call = true;
                     }
                     if(ir_helper::is_a_nop_function_decl(fdCalled) || !is_a_writing_memory_call)
                     {
                        if(gc->vdef || gc->vuses.size() || gc->vovers.size())
                        {
                           restart_mem = true;
                        }
                        add_nop_stmt(TM, *stmt, bb);
                        stmts_to_be_removed.insert(*stmt);
                        AppM->RegisterTransformation(GetName(), *stmt);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                     }
                  }
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not assign_stmt statement");
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
         std::list<ir_nodeRef> phis_to_be_removed;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phis");
         for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*phi)->ToString());
            THROW_ASSERT((*phi)->get_kind() == phi_stmt_K,
                         (*phi)->ToString() + " is of kind " + ir_node::GetString((*phi)->get_kind()));
            const auto gphi = GetPointerS<phi_stmt>(*phi);
            const auto res = gphi->res;
            THROW_ASSERT(res->get_kind() == ssa_node_K,
                         res->ToString() + " is of kind " + ir_node::GetString(res->get_kind()));
            const auto ssa = GetPointerS<ssa_node>(res);
            if(ssa->CGetNumberUses() == 0)
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
         listOfBB[bb->number] = bb;
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
            for(auto bb : listOfBB.at(curr)->list_of_succ)
            {
               if(BB_reached.insert(bb).second)
               {
                  to_be_processed.push(bb);
               }
            }
         }
         BB_reached.insert(bloc::EXIT_BLOCK_ID);
         for(const auto& bb_pair : listOfBB)
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
               const auto& bb = listOfBB.at(bbi);
               const auto phi_list = bb->CGetPhiList();
               std::list<ir_nodeRef> phis_to_be_removed;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis");
               for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + (*phi)->ToString());
                  THROW_ASSERT((*phi)->get_kind() == phi_stmt_K,
                               (*phi)->ToString() + " is of kind " + (*phi)->get_kind_text());
                  const auto gphi = GetPointerS<phi_stmt>(*phi);
                  const auto res = gphi->res;
                  THROW_ASSERT(res->get_kind() == ssa_node_K, res->ToString() + " is of kind " + res->get_kind_text());
                  const auto ssa = GetPointerS<const ssa_node>(res);
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
                  const auto gn = GetPointerS<node_stmt>(*stmt);
                  if(gn->vdef)
                  {
                     kill_vdef(TM, gn->vdef);
                     gn->vdef = nullptr;
                  }
                  else if((*stmt)->get_kind() == assign_stmt_K)
                  {
                     const auto ga = GetPointerS<assign_stmt>(*stmt);
                     if(ga->op0->get_kind() == ssa_node_K)
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
                  THROW_ASSERT(listOfBB.find(sblock) != listOfBB.end(), "Already removed BB" + STR(sblock));
                  const auto& succ_block = listOfBB.at(sblock);
                  succ_block->list_of_pred.erase(
                      std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb->number));
                  /// Fix PHIs
                  for(const auto& phi : succ_block->CGetPhiList())
                  {
                     const auto gp = GetPointerS<phi_stmt>(phi);
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
               listOfBB.erase(bbi);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed unreachable BBs");
         }
      }
   } while(restart_analysis);

   /// fix vdef
   for(const auto& block : listOfBB)
   {
      const auto& bb = block.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
      const auto& stmt_list = bb->CGetStmtList();
      std::list<ir_nodeRef> new_vssa_nop;
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         const auto stmt_kind = (*stmt)->get_kind();
         const function_val_node* fdCalled = nullptr;
         if(stmt_kind == assign_stmt_K)
         {
            const auto ga = GetPointerS<const assign_stmt>(*stmt);
            const auto rhs_kind = ga->op1->get_kind();
            if(rhs_kind == call_node_K)
            {
               const auto ce = GetPointerS<const call_node>(ga->op1);
               if(ce->fn->get_kind() == addr_node_K)
               {
                  const auto addr_ref = ce->fn;
                  const auto ae = GetPointerS<const addr_node>(addr_ref);
                  const auto fu_decl_node = ae->op;
                  THROW_ASSERT(fu_decl_node->get_kind() == function_val_node_K, "node  " + STR(fu_decl_node) +
                                                                                    " is not function_val_node but " +
                                                                                    fu_decl_node->get_kind_text());
                  fdCalled = GetPointer<const function_val_node>(fu_decl_node);
               }
            }
         }
         else if(stmt_kind == call_stmt_K)
         {
            const auto gc = GetPointerS<const call_stmt>(*stmt);
            const auto op_kind = gc->fn->get_kind();
            if(op_kind == addr_node_K)
            {
               const auto ue = GetPointerS<const unary_node>(gc->fn);
               fdCalled = GetPointer<const function_val_node>(ue->op);
            }
         }
         if(fdCalled)
         {
            const auto gn = GetPointerS<node_stmt>(*stmt);
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
                              THROW_ASSERT(vo->get_kind() == ssa_node_K, "");
                              GetPointerS<ssa_node>(vo)->AddUseStmt(*stmt);
                           }
                        }
                     }
                  }
                  /// fix vdef
                  new_vssa_nop.push_back(kill_vdef(TM, gn->vdef));
                  gn->vdef = nullptr;
                  for(const auto& vo : gn->vovers)
                  {
                     THROW_ASSERT(vo->get_kind() == ssa_node_K, "");
                     GetPointerS<ssa_node>(vo)->RemoveUse(*stmt);
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
   for(const auto& block : listOfBB)
   {
      const auto& bb = block.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb->number));
      const auto& stmt_list = bb->CGetStmtList();
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         const auto gn = GetPointerS<const node_stmt>(*stmt);

         if(!gn->vuses.empty() && (*stmt)->get_kind() != return_stmt_K)
         {
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (1)");
         }
         if(gn->vdef)
         {
            fd->writing_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (3)");
         }
         else if(const auto ga = GetPointer<const assign_stmt>(*stmt))
         {
            if(ga->op1->get_kind() == call_node_K)
            {
               const auto ce = GetPointerS<const call_node>(ga->op1);
               if(ce->fn->get_kind() == addr_node_K)
               {
                  const auto addr_ref = ce->fn;
                  const auto ae = GetPointerS<const addr_node>(addr_ref);
                  const auto fu_decl_node = ae->op;
                  THROW_ASSERT(fu_decl_node->get_kind() == function_val_node_K, "node  " + STR(fu_decl_node) +
                                                                                    " is not function_val_node but " +
                                                                                    fu_decl_node->get_kind_text());
                  const auto fdCalled = GetPointerS<const function_val_node>(fu_decl_node);
                  if(fdCalled->writing_memory || !fdCalled->body)
                  {
                     fd->writing_memory = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (5)");
                  }
                  if(fdCalled->reading_memory || !fdCalled->body)
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
         else if(const auto gc = GetPointer<const call_stmt>(*stmt))
         {
            const function_val_node* fdCalled = nullptr;
            if(gc->fn->get_kind() == addr_node_K)
            {
               const auto ue = GetPointerS<const unary_node>(gc->fn);
               fdCalled = GetPointer<const function_val_node>(ue->op);
            }
            if(fdCalled)
            {
               if(fdCalled->writing_memory || !fdCalled->body)
               {
                  fd->writing_memory = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (8)");
               }
               if(fdCalled->reading_memory || !fdCalled->body)
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + STR(bb->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---write flag " + (fd->writing_memory ? std::string("T") : std::string("F")));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---read flag " + (fd->reading_memory ? std::string("T") : std::string("F")));

   const auto calledSet = AppM->CGetCallGraphManager().get_called_by(function_id);
   for(const auto i : calledSet)
   {
      const auto fdCalled = GetPointerS<const function_val_node>(AppM->get_ir_manager()->GetIRNode(i));
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
