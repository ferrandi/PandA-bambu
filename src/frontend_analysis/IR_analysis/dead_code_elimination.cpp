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
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "sdc_scheduling.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// STD includes
#include <fstream>
#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>

dead_code_elimination::dead_code_elimination(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DEAD_CODE_ELIMINATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dead_code_elimination::~dead_code_elimination() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> dead_code_elimination::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, CALLED_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(MEM_CG_EXT, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool dead_code_elimination::HasToBeExecuted() const
{
   if(FunctionFrontendFlowStep::HasToBeExecuted())
      return true;
   std::map<unsigned int, bool> cur_writing_memory;
   std::map<unsigned int, bool> cur_reading_memory;
   const CallGraphManagerConstRef CGMan = AppM->CGetCallGraphManager();
   for(const auto i : AppM->CGetCallGraphManager()->get_called_by(function_id))
   {
      const tree_nodeRef curr_tn = AppM->get_tree_manager()->GetTreeNode(i);
      auto* fdCalled = GetPointer<function_decl>(curr_tn);
      cur_writing_memory[i] = fdCalled->writing_memory;
      cur_reading_memory[i] = fdCalled->reading_memory;
   }
   return cur_writing_memory != last_writing_memory || cur_reading_memory != last_reading_memory;
}

void dead_code_elimination::fix_sdc_motion(DesignFlowManagerConstRef design_flow_manager, unsigned int function_id, tree_nodeRef removedStmt)
{
   const auto design_flow_graph = design_flow_manager->CGetDesignFlowGraph();
   const auto sdc_scheduling_step = design_flow_manager->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), function_id));
   if(sdc_scheduling_step)
   {
      const auto sdc_scheduling = GetPointer<SDCScheduling>(design_flow_graph->CGetDesignFlowStepInfo(sdc_scheduling_step)->design_flow_step);
      auto& movements_list = sdc_scheduling->movements_list;
      const auto removed_index = GET_INDEX_CONST_NODE(removedStmt);
      movements_list.remove_if([&](const std::vector<unsigned int>& mv) { return mv[0] == removed_index; });
   }
}

void dead_code_elimination::fix_sdc_motion(tree_nodeRef removedStmt) const
{
   return fix_sdc_motion(design_flow_manager.lock(), function_id, removedStmt);
}

void dead_code_elimination::kill_uses(const tree_managerRef TM, tree_nodeRef op0) const
{
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "expected a tree_reindex object");
   THROW_ASSERT(GET_NODE(op0)->get_kind() == ssa_name_K, "expected a ssa_name object");
   auto op_ssa_index = GET_INDEX_NODE(op0);
   auto ssa = GetPointer<ssa_name>(GET_NODE(op0));
   THROW_ASSERT(ssa->CGetDefStmts().size() == 1, "unexpected condition");

   if(ssa->CGetNumberUses() != 0)
   {
      unsigned int type_index = tree_helper::get_type_index(TM, op_ssa_index);
      tree_nodeRef val;
      if(tree_helper::is_real(TM, op_ssa_index))
      {
         const auto data_value_id = TM->new_tree_node_id();
         const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
         val = tree_man->CreateRealCst(TM->GetTreeReindex(type_index), 0.l, data_value_id);
      }
      else if(tree_helper::is_a_complex(TM, op_ssa_index) || tree_helper::is_a_vector(TM, op_ssa_index))
      {
         const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
         auto utype = tree_man->create_default_unsigned_integer_type();
         auto zeroVal = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), GET_INDEX_NODE(utype));
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ne_schema, ga_schema;
         ne_schema[TOK(TOK_TYPE)] = STR(type_index);
         ne_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         ne_schema[TOK(TOK_OP)] = STR(zeroVal->index);
         const auto ne_id = TM->new_tree_node_id();
         TM->create_tree_node(ne_id, nop_expr_K, ne_schema);
         val = TM->GetTreeReindex(ne_id);
      }
      else
      {
         val = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), type_index);
      }
      const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
      for(const auto& use : StmtUses)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
         TM->ReplaceTreeNode(use.first, op0, val);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
      }
      THROW_ASSERT(ssa->CGetNumberUses() == 0, "unexpected condition");
   }
}

void dead_code_elimination::kill_vdef(const tree_managerRef TM, tree_nodeRef vdef)
{
   const auto gimple_nop_id = TM->new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
   gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
   GetPointer<ssa_name>(GET_NODE(vdef))->SetDefStmt(TM->GetTreeReindex(gimple_nop_id));
}

unsigned dead_code_elimination::move2emptyBB(const tree_managerRef TM, statement_list* sl, unsigned pred, blocRef bb_pred, unsigned cand_bb_dest, unsigned bb_dest) const
{
   auto succ_block = sl->list_of_bloc.at(cand_bb_dest);

   /// Create empty basic block
   const auto new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(new_basic_block_index) + " as new successor of BB" + STR(pred));
   auto new_block = blocRef(new bloc(new_basic_block_index));
   sl->list_of_bloc[new_basic_block_index] = new_block;

   new_block->loop_id = bb_pred->loop_id;
   new_block->SetSSAUsesComputed();
   new_block->schedule = bb_pred->schedule;

   new_block->list_of_pred.push_back(pred);
   new_block->list_of_succ.push_back(bb_dest);
   auto bb_dest_block = sl->list_of_bloc.at(bb_dest);
   bb_dest_block->list_of_pred.push_back(new_basic_block_index);
   bb_pred->list_of_succ.erase(std::find(bb_pred->list_of_succ.begin(), bb_pred->list_of_succ.end(), cand_bb_dest));
   bb_pred->list_of_succ.push_back(new_basic_block_index);

   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), pred));
   /// Fix PHIs
   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second != pred)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Readding <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
            new_list_of_def_edge.push_back(def_edge);
         }
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
   }
   for(auto phi : bb_dest_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" + STR(new_basic_block_index) + ">");
            new_list_of_def_edge.push_back(decltype(def_edge)(def_edge.first, new_basic_block_index));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Readding <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
         new_list_of_def_edge.push_back(def_edge);
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
   }
   return new_basic_block_index;
}

void dead_code_elimination::add_gimple_nop(gimple_node* gc, const tree_managerRef TM, tree_nodeRef cur_stmt, blocRef bb)
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_IR_schema;
   gimple_nop_IR_schema[TOK(TOK_SRCP)] = gc->include_name + ":" + STR(gc->line_number) + ":" + STR(gc->column_number);
   unsigned int gimple_nop_node_id = TM->new_tree_node_id();
   TM->create_tree_node(gimple_nop_node_id, gimple_nop_K, gimple_nop_IR_schema);
   tree_nodeRef gimple_nop_node_ref = TM->GetTreeReindex(gimple_nop_node_id);
   const auto old_gc = GetPointer<gimple_node>(GET_NODE(cur_stmt));
   const auto new_gc = GetPointer<gimple_node>(GET_NODE(gimple_nop_node_ref));
   THROW_ASSERT(old_gc, "");
   THROW_ASSERT(new_gc, "");
   if(old_gc->memdef)
   {
      new_gc->memdef = old_gc->memdef;
      GetPointer<ssa_name>(GET_NODE(new_gc->memdef))->SetDefStmt(gimple_nop_node_ref);
      old_gc->memdef = tree_nodeRef();
   }
   if(old_gc->memuse)
   {
      new_gc->memuse = old_gc->memuse;
      GetPointer<ssa_name>(GET_NODE(new_gc->memuse))->AddUseStmt(gimple_nop_node_ref);
   }
   if(old_gc->vdef)
   {
      new_gc->vdef = old_gc->vdef;
      GetPointer<ssa_name>(GET_NODE(new_gc->vdef))->SetDefStmt(gimple_nop_node_ref);
      old_gc->vdef = tree_nodeRef();
   }
   if(old_gc->vuses.size())
   {
      new_gc->vuses = old_gc->vuses;
      for(const auto& v : new_gc->vuses)
      {
         GetPointer<ssa_name>(GET_NODE(v))->AddUseStmt(gimple_nop_node_ref);
      }
   }
   if(old_gc->vovers.size())
   {
      new_gc->vovers = old_gc->vovers;
      old_gc->vovers.clear();
   }
   bb->PushBefore(gimple_nop_node_ref, cur_stmt);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added statement " + GET_NODE(gimple_nop_node_ref)->ToString());
}

/// single sweep analysis, block by block, from the bottom to up. Each ssa which is used zero times is eliminated and the uses of the variables used in the assignment are recomputed
/// multi-way and two way IFs simplified when conditions are constants
/// gimple_call without side effects are removed
/// store-load pairs checked for simplification
/// dead stores removed
DesignFlowStep_Status dead_code_elimination::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();

   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   /// Retrieve the list of block
   std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator block_it, block_it_end;
   block_it_end = blocks.end();
   const bool is_single_write_memory = GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->IsSingleWriteMemory();

   bool modified = false;
   bool restart_analysis;
   do
   {
      restart_analysis = false;
      bool do_reachability = false;
      CustomUnorderedMap<unsigned, CustomOrderedSet<unsigned>> vdefvover_map;
      // CustomUnorderedSet<unsigned> vdefvover_map;
      for(block_it = blocks.begin(); block_it != block_it_end; ++block_it)
      {
         const auto& stmt_list = block_it->second->CGetStmtList();
         for(auto stmt = stmt_list.begin(); stmt != stmt_list.end(); stmt++)
         {
            auto gn = GetPointer<gimple_node>(GET_NODE(*stmt));
            THROW_ASSERT(gn->vovers.empty() || gn->vdef, "unexpected condition");
            for(auto vo : gn->vovers)
            {
               vdefvover_map[GET_INDEX_NODE(vo)].insert(GET_INDEX_NODE(gn->vdef));
               // vdefvover_map.insert(GET_INDEX_NODE(vo));
            }
         }
      }
      for(block_it = blocks.begin(); block_it != block_it_end; ++block_it)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + boost::lexical_cast<std::string>(block_it->second->number));
         const auto& stmt_list = block_it->second->CGetStmtList();
         std::list<tree_nodeRef> stmts_to_be_removed;
         for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
               break;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
            /// find out if it is a gimple_assign
            if(GET_NODE(*stmt)->get_kind() == gimple_assign_K)
            {
               auto ga = GetPointer<gimple_assign>(GET_NODE(*stmt));
               /// in case of virtual uses it is better not perform the elimination
               if(ga->predicate && GET_NODE(ga->predicate)->get_kind() == integer_cst_K && GetPointer<integer_cst>(GET_NODE(ga->predicate))->value == 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead predicate found");
                  if(ga->vdef && !is_single_write_memory)
                  {
                     kill_vdef(TM, ga->vdef);
                     ga->vdef = tree_nodeRef();
                  }
                  else if(ga->memdef && is_single_write_memory)
                  {
                     kill_vdef(TM, ga->memdef);
                     ga->memdef = tree_nodeRef();
                  }
                  else if(GET_NODE(ga->op0)->get_kind() == ssa_name_K)
                  {
                     kill_uses(TM, ga->op0);
                  }
                  else
                     THROW_ERROR("unexpected condition");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                  stmts_to_be_removed.push_back(*stmt);
#ifndef NDEBUG
                  AppM->RegisterTransformation(GetName(), *stmt);
#endif
               }
               else
               {
                  /// op0 is the left side of the assignment, op1 is the right side
                  const tree_nodeRef op0 = GET_NODE(ga->op0);
                  const auto op1_type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op1));
                  if(op0->get_kind() == ssa_name_K)
                  {
                     auto* ssa = GetPointer<ssa_name>(op0);
                     /// very strict condition for the elimination
                     if(ssa->CGetNumberUses() == 0 and ssa->CGetDefStmts().size() == 1)
                     {
                        bool is_a_writing_memory_call = false;
                        if(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
                        {
                           auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
                           if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
                           {
                              const auto addr_node = GET_NODE(ce->fn);
                              const auto* ae = GetPointer<const addr_expr>(addr_node);
                              const auto fu_decl_node = GET_NODE(ae->op);
                              THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) + " is not function_decl but " + fu_decl_node->get_kind_text());
                              auto fdCalled = GetPointer<function_decl>(fu_decl_node);
                              if(fdCalled->writing_memory || !fdCalled->body)
                                 is_a_writing_memory_call = true;
                           }
                           else
                           {
                              is_a_writing_memory_call = true; /// conservative analysis
                           }
                        }
                        if(!is_a_writing_memory_call)
                        {
                           if(ga->vdef)
                              add_gimple_nop(ga, TM, *stmt, (block_it)->second);
                           stmts_to_be_removed.push_back(*stmt);
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), *stmt);
#endif
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                        }
                     }
                     else
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS ssa used: " + STR(ssa->CGetNumberUses()) + "-" + STR(ssa->CGetDefStmts().size()));
                  }
                  else if(op0->get_kind() == mem_ref_K && !ga->artificial && !tree_helper::is_a_vector(TM, op1_type_index))
                  {
                     auto* mr = GetPointer<mem_ref>(op0);
                     THROW_ASSERT(GET_NODE(mr->op1)->get_kind() == integer_cst_K, "unexpected condition");
                     auto type_w_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op1));
                     auto written_bw = resize_to_1_8_16_32_64_128_256_512(tree_helper::size(TM, type_w_index));
                     if(written_bw == 1)
                        written_bw = 8;
                     if(GetPointer<integer_cst>(GET_NODE(mr->op1))->value == 0)
                     {
                        THROW_ASSERT(GET_NODE(mr->op0)->get_kind() == ssa_name_K, "unexpected condition");
                        auto derefVar = GetPointer<ssa_name>(GET_NODE(mr->op0));
                        auto& defStmts = derefVar->CGetDefStmts();
                        if(defStmts.size() == 1)
                        {
                           auto defStmt = *defStmts.begin();
                           if(GET_NODE(defStmt)->get_kind() == gimple_assign_K)
                           {
                              auto derefGA = GetPointer<gimple_assign>(GET_NODE(defStmt));
                              if(GET_NODE(derefGA->op1)->get_kind() == addr_expr_K)
                              {
                                 auto addressedVar = GetPointer<addr_expr>(GET_NODE(derefGA->op1))->op;
                                 if(GET_NODE(addressedVar)->get_kind() == var_decl_K)
                                 {
                                    auto varDecl = GetPointer<var_decl>(GET_NODE(addressedVar));
                                    if(varDecl->scpe && function_id == GET_INDEX_NODE(varDecl->scpe) && !varDecl->static_flag)
                                    {
                                       ssa_name* ssaDef = nullptr;
                                       unsigned ssaDefIndex = 0;
                                       if(is_single_write_memory && ga->memdef)
                                       {
                                          ssaDef = GetPointer<ssa_name>(GET_NODE(ga->memdef));
                                          ssaDefIndex = GET_INDEX_NODE(ga->memdef);
                                       }
                                       else if(!is_single_write_memory && ga->vdef)
                                       {
                                          ssaDef = GetPointer<ssa_name>(GET_NODE(ga->vdef));
                                          ssaDefIndex = GET_INDEX_NODE(ga->vdef);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "VDEF: " + STR(GET_INDEX_NODE(ga->vdef)));
                                       }
                                       else
                                       {
                                          THROW_ERROR("unexpected condition");
                                       }
                                       THROW_ASSERT(ssaDef && ssaDefIndex, "unexpected condition");
                                       /// very strict condition for the elimination
                                       if(ssaDef->CGetDefStmts().size() == 1)
                                       {
                                          if(ssaDef->CGetNumberUses() == 0 and vdefvover_map.find(ssaDefIndex) == vdefvover_map.end())
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead Store found");
                                             if(ga->vdef)
                                                add_gimple_nop(ga, TM, *stmt, (block_it)->second);
                                             stmts_to_be_removed.push_back(*stmt);
#ifndef NDEBUG
                                             AppM->RegisterTransformation(GetName(), *stmt);
#endif
                                          }
                                          else
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---check if the associated load are dead");
                                             const TreeNodeMap<size_t> StmtUses = ssaDef->CGetUseStmts();
                                             for(const auto& use : StmtUses)
                                             {
                                                const auto gn_used = GetPointer<gimple_node>(GET_NODE(use.first));
                                                if(!gn_used->vdef and gn_used->bb_index == ga->bb_index and gn_used->get_kind() == gimple_assign_K)
                                                {
                                                   const auto ga_used = GetPointer<gimple_assign>(GET_NODE(use.first));
                                                   if(GET_NODE(ga_used->op0)->get_kind() == ssa_name_K and GET_NODE(ga_used->op1)->get_kind() == mem_ref_K &&
                                                      !(ga_used->predicate && GET_NODE(ga_used->predicate)->get_kind() == integer_cst_K && GetPointer<integer_cst>(GET_NODE(ga_used->predicate))->value == 0))
                                                   {
                                                      const auto mr_used = GetPointer<mem_ref>(GET_NODE(ga_used->op1));
                                                      if(GetPointer<integer_cst>(GET_NODE(mr->op1))->value == GetPointer<integer_cst>(GET_NODE(mr_used->op1))->value)
                                                      {
                                                         auto type_r_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(ga_used->op0));
                                                         auto read_bw = resize_to_1_8_16_32_64_128_256_512(tree_helper::size(TM, type_r_index));
                                                         if(read_bw == 1)
                                                            read_bw = 8;
                                                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---read_bw: " + STR(read_bw) + " written_bw: " + STR(written_bw));
                                                         if(GET_INDEX_NODE(mr->op0) == GET_INDEX_NODE(mr_used->op0) && written_bw == read_bw)
                                                         {
                                                            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---found a candidate " + GET_NODE(use.first)->ToString());
                                                            /// check if this load is killed by a following vover
                                                            auto curr_stmt = stmt;
                                                            bool found_load = false;
                                                            if(curr_stmt != stmt_list.rbegin())
                                                            {
                                                               --curr_stmt;
                                                               while(1)
                                                               {
                                                                  const auto gn_curr = GetPointer<gimple_node>(GET_NODE(*curr_stmt));
                                                                  if(GET_INDEX_NODE(*curr_stmt) == GET_INDEX_NODE(use.first))
                                                                  {
                                                                     found_load = true;
                                                                     break;
                                                                  }
                                                                  if(!found_load && gn_curr->vdef &&
                                                                     (ga_used->vuses.find(gn_curr->vdef) != ga_used->vuses.end() ||
                                                                      (vdefvover_map.find(ssaDefIndex) != vdefvover_map.end() && vdefvover_map.find(ssaDefIndex)->second.find(GET_INDEX_NODE(gn_curr->vdef)) != vdefvover_map.find(ssaDefIndex)->second.end())))
                                                                     break;
                                                                  if(curr_stmt == stmt_list.rbegin())
                                                                     break;
                                                                  else
                                                                     --curr_stmt;
                                                               }
                                                            }
                                                            if(found_load)
                                                            {
                                                               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---found a Dead Load " + GET_NODE(*curr_stmt)->ToString());
                                                               auto ssa_used_op0 = GetPointer<ssa_name>(GET_NODE(ga_used->op0));
                                                               const TreeNodeMap<size_t> StmtUsesOp0 = ssa_used_op0->CGetUseStmts();
                                                               for(const auto& useop0 : StmtUsesOp0)
                                                               {
                                                                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + useop0.first->ToString());
                                                                  TM->ReplaceTreeNode(useop0.first, ga_used->op0, ga->op1);
                                                                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + useop0.first->ToString());
                                                               }
                                                               THROW_ASSERT(ssa_used_op0->CGetNumberUses() == 0, "unexpected condition");
                                                               stmts_to_be_removed.push_back(*curr_stmt);
#ifndef NDEBUG
                                                               AppM->RegisterTransformation(GetName(), *curr_stmt);
#endif
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
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---local variable later used");
                                       }
                                    }
                                    else
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non local variable");
                                 }
                                 else
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non var decl");
                              }
                              else
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern1");
                           }
                           else
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern2");
                        }
                        else
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not supported pattern3");
                     }
                     else
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---non-null offset in the mem_ref");
                  }
                  else
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS not ssa");
               }
            }
            else if(GET_NODE(*stmt)->get_kind() == gimple_cond_K)
            {
               auto gc = GetPointer<gimple_cond>(GET_NODE(*stmt));
               if(GET_NODE(gc->op0)->get_kind() == integer_cst_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_cond with a constant condition");
                  auto val = GetPointer<integer_cst>(GET_NODE(gc->op0))->value;
                  auto bb = block_it->second;
                  do_reachability = true;
                  modified = true;
                  if(val)
                  {
                     bb->false_edge = move2emptyBB(TM, sl, block_it->first, bb, bb->false_edge, bb->true_edge);
                  }
                  else
                  {
                     bb->true_edge = move2emptyBB(TM, sl, block_it->first, bb, bb->true_edge, bb->false_edge);
                  }
               }
            }
            else if(GET_NODE(*stmt)->get_kind() == gimple_multi_way_if_K)
            {
               auto gm = GetPointer<gimple_multi_way_if>(GET_NODE(*stmt));

               bool one_is_const = false;
               bool all_false = true;
               unsigned int bb_dest = 0;
               for(auto cond : gm->list_of_cond)
               {
                  if(!cond.first)
                  {
                     if(all_false)
                     {
                        THROW_ASSERT(!one_is_const, "only one can be true");
                        bb_dest = cond.second;
                     }
                  }
                  else if(GET_NODE(cond.first)->get_kind() == integer_cst_K)
                  {
                     if(GetPointer<integer_cst>(GET_NODE(cond.first))->value)
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_multi_way_if with constant conditions");
                  modified = true;
                  auto bbIndex = block_it->first;
                  auto bb = block_it->second;
                  for(auto& cond : gm->list_of_cond)
                  {
                     if(cond.second != bb_dest)
                     {
                        do_reachability = true;
                        cond.second = move2emptyBB(TM, sl, bbIndex, bb, cond.second, bb_dest);
                     }
                  }
               }
               else
               {
                  /// remove same conditions
                  std::map<unsigned int, unsigned int> condIndex2BBdest;
                  auto bbIndex = block_it->first;
                  auto bb = block_it->second;
                  auto do0ConstantCondRemoval = false;
                  for(auto& cond : gm->list_of_cond)
                  {
                     if(cond.first)
                     {
                        if(GET_NODE(cond.first)->get_kind() == integer_cst_K)
                        {
                           if(!blocks[cond.second]->CGetStmtList().empty())
                              do0ConstantCondRemoval = true;
                        }
                        else if(condIndex2BBdest.find(GET_INDEX_NODE(cond.first)) == condIndex2BBdest.end())
                        {
                           condIndex2BBdest[GET_INDEX_NODE(cond.first)] = cond.second;
                        }
                        else if(!blocks[cond.second]->CGetStmtList().empty())
                        {
                           do_reachability = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_multi_way_if duplicated condition from " + STR(bbIndex) + " to " + STR(cond.second));
                           cond.second = move2emptyBB(TM, sl, bbIndex, bb, cond.second, condIndex2BBdest.find(GET_INDEX_NODE(cond.first))->second);
                        }
                     }
                  }
                  if(do0ConstantCondRemoval)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_multi_way_if do zero condition removal");
                     auto bb0_dest = condIndex2BBdest.begin()->second;
                     for(auto& cond : gm->list_of_cond)
                     {
                        if(cond.first)
                        {
                           if(GET_NODE(cond.first)->get_kind() == integer_cst_K)
                           {
                              THROW_ASSERT(GetPointer<integer_cst>(GET_NODE(cond.first))->value == 0, "unexpected condition");
                              do_reachability = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple_multi_way_if duplicated condition from " + STR(bbIndex) + " to " + STR(cond.second));
                              cond.second = move2emptyBB(TM, sl, bbIndex, bb, cond.second, bb0_dest);
                           }
                        }
                     }
                  }
               }
            }
            else if(GET_NODE(*stmt)->get_kind() == gimple_call_K)
            {
               auto* gc = GetPointer<gimple_call>(GET_NODE(*stmt));
               tree_nodeRef temp_node = GET_NODE(gc->fn);
               function_decl* fdCalled = nullptr;

               if(temp_node->get_kind() == addr_expr_K)
               {
                  auto* ue = GetPointer<unary_expr>(temp_node);
                  temp_node = ue->op;
                  fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
               }
               else if(temp_node->get_kind() == obj_type_ref_K)
               {
                  temp_node = tree_helper::find_obj_type_ref_function(gc->fn);
                  fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
               }
               if(fdCalled)
               {
                  bool is_a_writing_memory_call = false;
                  if(fdCalled->writing_memory or !fdCalled->body)
                     is_a_writing_memory_call = true;
                  if(tree_helper::is_a_nop_function_decl(fdCalled) or !is_a_writing_memory_call)
                  {
                     add_gimple_nop(gc, TM, *stmt, (block_it)->second);

                     stmts_to_be_removed.push_back(*stmt);
#ifndef NDEBUG
                     AppM->RegisterTransformation(GetName(), *stmt);
#endif
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dead code found");
                  }
               }
            }
            else
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not gimple_assign statement");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
         }
         if(not stmts_to_be_removed.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--Restart dead code");
            modified = true;
            restart_analysis = true;
         }
         if(not stmts_to_be_removed.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(stmts_to_be_removed.size()) + " dead statements");
            for(auto curr_el : stmts_to_be_removed)
            {
               auto* ga = GetPointer<gimple_assign>(GET_NODE(curr_el));
               if((ga and (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)) or GetPointer<gimple_call>(GET_NODE(curr_el)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call expr can be removed");
                  const CallGraphManagerRef cg_man = AppM->GetCallGraphManager();
                  const vertex fun_cg_vertex = cg_man->GetVertex(function_id);
                  const CallGraphConstRef cg = cg_man->CGetCallGraph();
                  CustomOrderedSet<EdgeDescriptor> to_remove;
                  OutEdgeIterator oei, oei_end;
                  boost::tie(oei, oei_end) = boost::out_edges(fun_cg_vertex, *cg);
                  const unsigned int call_id = GET_INDEX_NODE(curr_el);
                  for(; oei != oei_end; oei++)
                  {
                     const CustomOrderedSet<unsigned int>& direct_calls = cg->CGetFunctionEdgeInfo(*oei)->direct_call_points;
                     auto call_it = direct_calls.find(call_id);
                     if(call_it != direct_calls.end())
                     {
                        to_remove.insert(*oei);
                     }
                  }
                  THROW_ASSERT(to_remove.size(), "Call to be removed not found in call graph");
                  for(const EdgeDescriptor& e : to_remove)
                  {
                     cg_man->RemoveCallPoint(e, call_id);
                  }
                  if(parameters->getOption<bool>(OPT_print_dot) && debug_level >= DEBUG_LEVEL_PEDANTIC)
                  {
                     AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph" + GetSignature() + ".dot");
                  }
               }
               block_it->second->RemoveStmt(curr_el);
               fix_sdc_motion(curr_el);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_el->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead statements");
         }
         /*
          * check also phi operations. if a phi assigns an ssa which is not used
          * anymore, the phi can be removed
          */
         const auto& phi_list = block_it->second->CGetPhiList();
         std::list<tree_nodeRef> phis_to_be_removed;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phis");
         for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
               break;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*phi)->ToString());
            THROW_ASSERT(GET_NODE(*phi)->get_kind() == gimple_phi_K, GET_NODE(*phi)->ToString() + " is of kind " + tree_node::GetString(GET_NODE(*phi)->get_kind()));
            auto gphi = GetPointer<gimple_phi>(GET_NODE(*phi));
            const tree_nodeRef res = GET_NODE(gphi->res);
            THROW_ASSERT(res->get_kind() == ssa_name_K, res->ToString() + " is of kind " + tree_node::GetString(res->get_kind()));
            const ssa_name* ssa = GetPointer<ssa_name>(res);
            // very strict condition for the elimination
            if(ssa->CGetNumberUses() == 0 and ssa->CGetDefStmts().size() == 1)
            {
               phis_to_be_removed.push_back(*phi);
#ifndef NDEBUG
               AppM->RegisterTransformation(GetName(), *phi);
#endif
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phi");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phis");
         if(not phis_to_be_removed.empty())
         {
            modified = true;
            restart_analysis = true;
         }
         if(not phis_to_be_removed.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(phis_to_be_removed.size()) + " dead phis");
            for(auto curr_phi : phis_to_be_removed)
            {
               block_it->second->RemovePhi(curr_phi);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_phi->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead phis");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + boost::lexical_cast<std::string>(block_it->second->number));
      }
      while(do_reachability)
      {
         do_reachability = false;
         CustomOrderedSet<unsigned> BB_reached;
         std::queue<unsigned> to_be_processed;
         to_be_processed.push(bloc::ENTRY_BLOCK_ID);
         BB_reached.insert(bloc::ENTRY_BLOCK_ID);
         while(!to_be_processed.empty())
         {
            auto curr = to_be_processed.front();
            to_be_processed.pop();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyzing BB" + STR(curr));
            for(auto bb : blocks.at(curr)->list_of_succ)
            {
               if(BB_reached.find(bb) == BB_reached.end())
               {
                  to_be_processed.push(bb);
                  BB_reached.insert(bb);
               }
            }
         }
         CustomOrderedSet<unsigned> bb_to_remove;
         for(auto bb_pair : blocks)
         {
            if(BB_reached.find(bb_pair.first) == BB_reached.end())
            {
               if(bb_pair.first != bloc::EXIT_BLOCK_ID)
                  bb_to_remove.insert(bb_pair.first);
            }
         }
         if(not bb_to_remove.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(bb_to_remove.size()) + " unreachable BBs");
            for(auto bb : bb_to_remove)
            {
               do_reachability = true;
               const auto& phi_list = blocks.at(bb)->CGetPhiList();
               std::list<tree_nodeRef> phis_to_be_removed;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phis (RA)");
               for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*phi)->ToString());
                  THROW_ASSERT(GET_NODE(*phi)->get_kind() == gimple_phi_K, GET_NODE(*phi)->ToString() + " is of kind " + tree_node::GetString(GET_NODE(*phi)->get_kind()));
                  auto gphi = GetPointer<gimple_phi>(GET_NODE(*phi));
                  const tree_nodeRef res = GET_NODE(gphi->res);
                  THROW_ASSERT(res->get_kind() == ssa_name_K, res->ToString() + " is of kind " + tree_node::GetString(res->get_kind()));
                  const ssa_name* ssa = GetPointer<ssa_name>(res);
                  if(ssa->virtual_flag)
                     kill_vdef(TM, gphi->res);
                  else
                     kill_uses(TM, gphi->res);
                  phis_to_be_removed.push_back(*phi);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phi");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phis");

               if(not phis_to_be_removed.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(phis_to_be_removed.size()) + " dead phis");
                  for(auto curr_phi : phis_to_be_removed)
                  {
                     blocks.at(bb)->RemovePhi(curr_phi);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_phi->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead phis");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statements (RA)");
               const auto& stmt_list = blocks.at(bb)->CGetStmtList();
               std::list<tree_nodeRef> stmts_to_be_removed;
               for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
                  auto node_stmt = GET_NODE(*stmt);
                  auto gn = GetPointer<gimple_node>(node_stmt);
                  if(gn->vdef && !is_single_write_memory)
                  {
                     kill_vdef(TM, gn->vdef);
                     gn->vdef = tree_nodeRef();
                  }
                  else if(gn->memdef && is_single_write_memory)
                  {
                     kill_vdef(TM, gn->memdef);
                     gn->memdef = tree_nodeRef();
                  }
                  else if(node_stmt->get_kind() == gimple_assign_K)
                  {
                     auto ga = GetPointer<gimple_assign>(node_stmt);
                     if(GET_NODE(ga->op0)->get_kind() == ssa_name_K)
                        kill_uses(TM, ga->op0);
                  }
                  stmts_to_be_removed.push_back(*stmt);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed stmt " + (*stmt)->ToString());
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statements");
               if(!stmts_to_be_removed.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(stmts_to_be_removed.size()) + " dead statements");
                  for(auto curr_el : stmts_to_be_removed)
                  {
                     auto* ga = GetPointer<gimple_assign>(GET_NODE(curr_el));
                     if((ga and (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)) || GET_NODE(curr_el)->get_kind() == gimple_call_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call expr can be removed");
                        const CallGraphManagerRef cg_man = AppM->GetCallGraphManager();
                        const vertex fun_cg_vertex = cg_man->GetVertex(function_id);
                        const CallGraphConstRef cg = cg_man->CGetCallGraph();
                        CustomOrderedSet<EdgeDescriptor> to_remove;
                        OutEdgeIterator oei, oei_end;
                        boost::tie(oei, oei_end) = boost::out_edges(fun_cg_vertex, *cg);
                        const unsigned int call_id = GET_INDEX_NODE(curr_el);
                        for(; oei != oei_end; oei++)
                        {
                           const CustomOrderedSet<unsigned int>& direct_calls = cg->CGetFunctionEdgeInfo(*oei)->direct_call_points;
                           auto call_it = direct_calls.find(call_id);
                           if(call_it != direct_calls.end())
                           {
                              to_remove.insert(*oei);
                           }
                        }
                        THROW_ASSERT(to_remove.size(), "Call to be removed not found in call graph");
                        for(const EdgeDescriptor& e : to_remove)
                        {
                           cg_man->RemoveCallPoint(e, call_id);
                        }
                        if(parameters->getOption<bool>(OPT_print_dot) && debug_level >= DEBUG_LEVEL_PEDANTIC)
                        {
                           AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph" + GetSignature() + ".dot");
                        }
                     }
                     blocks.at(bb)->RemoveStmt(curr_el);
                     fix_sdc_motion(curr_el);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + curr_el->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead statements");
               }
               for(auto sblock : blocks.at(bb)->list_of_succ)
               {
                  if(sblock == bloc::EXIT_BLOCK_ID)
                     continue;
                  THROW_ASSERT(blocks.find(sblock) != blocks.end(), "Already removed BB" + STR(sblock));
                  auto succ_block = blocks.at(sblock);
                  succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb));
                  /// Fix PHIs
                  for(auto phi : succ_block->CGetPhiList())
                  {
                     auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
                     gimple_phi::DefEdgeList new_list_of_def_edge;
                     for(const auto& def_edge : gp->CGetDefEdgesList())
                     {
                        if(def_edge.second != bb)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Readding <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
                           new_list_of_def_edge.push_back(def_edge);
                        }
                     }
                     gp->SetDefEdgeList(TM, new_list_of_def_edge);
                  }
               }
               blocks.at(bb)->list_of_succ.clear();
            }
            for(auto bb : bb_to_remove)
            {
               blocks.erase(bb);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed unreachable BBs");
         }
      }
   } while(restart_analysis);

   /// fix vdef/memdef
   for(block_it = blocks.begin(); block_it != block_it_end; ++block_it)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + boost::lexical_cast<std::string>(block_it->second->number));
      const auto& stmt_list = block_it->second->CGetStmtList();
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         auto* gn = GetPointer<gimple_node>(GET_NODE(*stmt));
         if(auto ga = GetPointer<gimple_assign>(GET_NODE(*stmt)))
         {
            if(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
            {
               auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
               if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
               {
                  const auto addr_node = GET_NODE(ce->fn);
                  const auto* ae = GetPointer<const addr_expr>(addr_node);
                  const auto fu_decl_node = GET_NODE(ae->op);
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) + " is not function_decl but " + fu_decl_node->get_kind_text());
                  auto fdCalled = GetPointer<function_decl>(fu_decl_node);
                  if(fdCalled->writing_memory || !fdCalled->body)
                     ;
                  else
                  {
                     if(gn->vdef && !is_single_write_memory)
                     {
                        if(fdCalled->reading_memory)
                        {
                           /// all vovers become vuse
                           for(auto vo : gn->vovers)
                           {
                              if(not gn->vdef || (GET_INDEX_NODE(vo) != GET_INDEX_NODE(gn->vdef)))
                              {
                                 gn->vuses.insert(vo);
                                 GetPointer<ssa_name>(GET_NODE(vo))->AddUseStmt(*stmt);
                              }
                           }
                        }
                        /// fix vdef
                        kill_vdef(TM, gn->vdef);
                        gn->vdef = tree_nodeRef();
                        gn->vovers.clear();
                        modified = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Nothing is written by this function: Fixed VDEF/VOVER ");
                     }
                     else if(gn->memdef && is_single_write_memory)
                     {
                        if(not fdCalled->reading_memory)
                        {
                           /// fix memdef
                           kill_vdef(TM, gn->memdef);
                           gn->memdef = tree_nodeRef();
                           modified = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Nothing is written by this function: Fixed MEMDEF ");
                        }
                     }
                  }
               }
            }
         }
         else if(auto gc = GetPointer<gimple_call>(GET_NODE(*stmt)))
         {
            tree_nodeRef temp_node = GET_NODE(gc->fn);
            function_decl* fdCalled = nullptr;

            if(temp_node->get_kind() == addr_expr_K)
            {
               auto* ue = GetPointer<unary_expr>(temp_node);
               temp_node = ue->op;
               fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
            }
            else if(temp_node->get_kind() == obj_type_ref_K)
            {
               temp_node = tree_helper::find_obj_type_ref_function(gc->fn);
               fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
            }
            if(fdCalled)
            {
               if(fdCalled->writing_memory || !fdCalled->body)
                  ;
               else
               {
                  if(gn->vdef && !is_single_write_memory)
                  {
                     if(fdCalled->reading_memory)
                     {
                        /// all vovers become vuse
                        for(auto vo : gn->vovers)
                        {
                           if(not gn->vdef || (GET_INDEX_NODE(vo) != GET_INDEX_NODE(gn->vdef)))
                           {
                              gn->vuses.insert(vo);
                              GetPointer<ssa_name>(GET_NODE(vo))->AddUseStmt(*stmt);
                           }
                        }
                     }
                     /// fix vdef
                     kill_vdef(TM, gn->vdef);
                     gn->vdef = tree_nodeRef();
                     gn->vovers.clear();
                     modified = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Nothing is written by this function: Fixed VDEF/VOVER ");
                  }
                  else if(gn->memdef && is_single_write_memory)
                  {
                     if(not fdCalled->reading_memory)
                     {
                        /// fix memdef
                        kill_vdef(TM, gn->memdef);
                        gn->memdef = tree_nodeRef();
                        modified = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Nothing is written by this function: Fixed MEMDEF ");
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + boost::lexical_cast<std::string>(block_it->second->number));
   }

   /// update function memory write flag
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Update memory read/write flag");
   fd->writing_memory = false;
   fd->reading_memory = false;
   for(block_it = blocks.begin(); block_it != block_it_end; ++block_it)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + boost::lexical_cast<std::string>(block_it->second->number));
      const auto& stmt_list = block_it->second->CGetStmtList();
      for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
         auto* gn = GetPointer<gimple_node>(GET_NODE(*stmt));

         if(not gn->vuses.empty() && !is_single_write_memory && GET_NODE(*stmt)->get_kind() != gimple_return_K)
         {
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (1)");
         }
         else if(gn->memuse && is_single_write_memory)
         {
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory (2)");
         }
         if(gn->vdef && !is_single_write_memory)
         {
            fd->writing_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (3)");
         }
         else if(gn->memdef && is_single_write_memory)
         {
            fd->writing_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- writing_memory (4)");
         }
         else if(auto ga = GetPointer<gimple_assign>(GET_NODE(*stmt)))
         {
            if(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
            {
               auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
               if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
               {
                  const auto addr_node = GET_NODE(ce->fn);
                  const auto* ae = GetPointer<const addr_expr>(addr_node);
                  const auto fu_decl_node = GET_NODE(ae->op);
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) + " is not function_decl but " + fu_decl_node->get_kind_text());
                  auto fdCalled = GetPointer<function_decl>(fu_decl_node);
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
         else if(auto gc = GetPointer<gimple_call>(GET_NODE(*stmt)))
         {
            tree_nodeRef temp_node = GET_NODE(gc->fn);
            function_decl* fdCalled = nullptr;

            if(temp_node->get_kind() == addr_expr_K)
            {
               auto* ue = GetPointer<unary_expr>(temp_node);
               temp_node = ue->op;
               fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
            }
            else if(temp_node->get_kind() == obj_type_ref_K)
            {
               temp_node = tree_helper::find_obj_type_ref_function(gc->fn);
               fdCalled = GetPointer<function_decl>(GET_NODE(temp_node));
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
         else if(GetPointer<gimple_asm>(GET_NODE(*stmt)))
         {
            fd->writing_memory = true; /// more conservative than really needed
            fd->reading_memory = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- reading_memory+writing_memory (11)");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + boost::lexical_cast<std::string>(block_it->second->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---write flag " + (fd->writing_memory ? std::string("T") : std::string("F")));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---read flag " + (fd->reading_memory ? std::string("T") : std::string("F")));
   const CallGraphManagerConstRef CGMan = AppM->CGetCallGraphManager();
   CustomOrderedSet<unsigned int> calledSet = AppM->CGetCallGraphManager()->get_called_by(function_id);
   for(const auto i : calledSet)
   {
      auto* fdCalled = GetPointer<function_decl>(AppM->get_tree_manager()->GetTreeNode(i));
      last_writing_memory[i] = fdCalled->writing_memory;
      last_reading_memory[i] = fdCalled->reading_memory;
   }
   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
}
