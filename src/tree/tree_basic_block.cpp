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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file tree_basic_block.cpp
 * @brief Data structure describing a basic block at tree level.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "tree_basic_block.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "tree_manager.hpp"

#if HAVE_BAMBU_BUILT
/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// tree includes
#include "string_manipulation.hpp" // for STR
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

const unsigned int bloc::ENTRY_BLOCK_ID = BB_ENTRY;
const unsigned int bloc::EXIT_BLOCK_ID = BB_EXIT;

bloc::bloc(unsigned int _number)
    : removed_phi(0), updated_ssa_uses(false), number(_number), loop_id(0), hpl(0), true_edge(0), false_edge(0)
{
}

bloc::~bloc() = default;

void bloc::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   SEQ_VISIT_MEMBER(mask, list_of_phi, tree_node, visit, tree_node_visitor, v);
   SEQ_VISIT_MEMBER(mask, list_of_stmt, tree_node, visit, tree_node_visitor, v);
   /// live in and out not visited by design
}

bool bloc::check_function_call(const tree_nodeRef& statement, gimple_assign* ga, unsigned int& called_function_id)
{
   called_function_id = 0;
   if(ga)
   {
      const auto ce = GetPointerS<const call_expr>(GET_NODE(ga->op1));
      if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
      {
         const auto fn = GetPointerS<const addr_expr>(GET_CONST_NODE(ce->fn))->op;
         if(GET_NODE(fn)->get_kind() == function_decl_K)
         {
            called_function_id = GET_INDEX_CONST_NODE(fn);
            return true;
         }
      }
   }
   else
   {
      const auto gc = GetPointerS<gimple_call>(GET_NODE(statement));
      if(GET_NODE(gc->fn)->get_kind() == addr_expr_K)
      {
         const auto fn = GetPointerS<const addr_expr>(GET_CONST_NODE(gc->fn))->op;
         if(GET_NODE(fn)->get_kind() == function_decl_K)
         {
            called_function_id = GET_INDEX_NODE(fn);
            return true;
         }
      }
   }
   return false;
}

void bloc::ReorderLUTs()
{
   TreeNodeSet current_uses;
   for(const auto& phi : list_of_phi)
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      current_uses.insert(gp->res);
   }
   std::list<tree_nodeRef> list_of_postponed_stmt;
   auto pos = list_of_stmt.begin();
   while(pos != list_of_stmt.end())
   {
      if(GET_NODE(*pos)->get_kind() == gimple_assign_K)
      {
         auto ga = GetPointer<gimple_assign>(GET_NODE(*pos));
         if(GET_NODE(ga->op0)->get_kind() != ssa_name_K)
         {
            ++pos;
            continue;
         }
         auto allDefinedP = [&](tree_nodeRef stmt) -> bool {
            const auto& uses = tree_helper::ComputeSsaUses(stmt);
            for(const auto& u : uses)
            {
               if(current_uses.find(u.first) == current_uses.end())
               {
                  auto ssa_node = GET_NODE(u.first);
                  auto ssa = GetPointer<ssa_name>(ssa_node);
                  if(ssa->virtual_flag || GetPointer<gimple_node>(GET_NODE(ssa->CGetDefStmt()))->bb_index != number)
                  {
                     current_uses.insert(u.first);
                  }
                  else
                  {
                     return false;
                  }
               }
            }
            return true;
         };

         if(not allDefinedP(*pos))
         {
            list_of_postponed_stmt.push_back(*pos);
            const auto next_stmt = std::next(pos);
            list_of_stmt.erase(pos);
            pos = next_stmt;
         }
         else
         {
            current_uses.insert(ga->op0);
            const auto next_stmt = std::next(pos);
            bool restart_postponed = false;
            do
            {
               restart_postponed = false;
               auto posPostponed = list_of_postponed_stmt.begin();
               while(posPostponed != list_of_postponed_stmt.end())
               {
                  if(allDefinedP(*posPostponed))
                  {
                     /// add back
                     list_of_stmt.insert(next_stmt, *posPostponed);
                     auto gaPostponed = GetPointer<gimple_assign>(GET_NODE(*posPostponed));
                     current_uses.insert(gaPostponed->op0);
                     restart_postponed = true;
#if HAVE_BAMBU_BUILT
                     if(schedule)
                     {
                        schedule->UpdateTime(gaPostponed->index);
                     }
#endif
                     list_of_postponed_stmt.erase(posPostponed);
                     break;
                  }
                  else
                  {
                     ++posPostponed;
                  }
               }
            } while(restart_postponed);
            pos = next_stmt;
         }
      }
      else
      {
         ++pos;
      }
   }
}

void bloc::manageCallGraph(const application_managerRef& AppM, const tree_nodeRef& statement)
{
   const auto ga = GetPointer<gimple_assign>(GET_NODE(statement));
   if((ga && (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)) ||
      GET_NODE(statement)->get_kind() == gimple_call_K)
   {
      THROW_ASSERT(AppM, "");
      const auto cg_man = AppM->GetCallGraphManager();
      THROW_ASSERT(cg_man, "");
      THROW_ASSERT(GetPointerS<const gimple_node>(GET_NODE(statement))->scpe, "statement " + statement->ToString());
      unsigned int called_function_id;
      if(check_function_call(statement, ga, called_function_id))
      {
         const auto function_id = GET_INDEX_NODE(GetPointerS<const gimple_node>(GET_NODE(statement))->scpe);
         if(cg_man->IsVertex(function_id))
         {
            CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function_id,
                                                    GET_INDEX_CONST_NODE(statement),
                                                    FunctionEdgeInfo::CallType::direct_call, 0);
         }
      }
   }
}

void bloc::update_new_stmt(const application_managerRef& AppM, const tree_nodeRef& new_stmt)
{
   /// This check is necessary since during parsing of statement list statement has not yet been filled
   if(GET_NODE(new_stmt))
   {
      manageCallGraph(AppM, new_stmt);
      const auto gn = GetPointer<gimple_node>(GET_NODE(new_stmt));
      THROW_ASSERT(gn, "");
      gn->bb_index = number;

      if(gn->vdef)
      {
         THROW_ASSERT(GET_NODE(gn->vdef)->get_kind() == ssa_name_K, "");
         GetPointerS<ssa_name>(GET_NODE(gn->vdef))->SetDefStmt(new_stmt);
      }

      if(gn->memdef)
      {
         THROW_ASSERT(GET_NODE(gn->memdef)->get_kind() == ssa_name_K, "");
         GetPointerS<ssa_name>(GET_NODE(gn->memdef))->SetDefStmt(new_stmt);
      }

      if(GET_NODE(new_stmt)->get_kind() == gimple_assign_K)
      {
         const auto ga = GetPointerS<gimple_assign>(GET_NODE(new_stmt));
         if(GET_NODE(ga->op0) && GET_NODE(ga->op0)->get_kind() == ssa_name_K)
         {
            GetPointerS<ssa_name>(GET_NODE(ga->op0))->SetDefStmt(new_stmt);
         }
      }
      else if(GET_NODE(new_stmt)->get_kind() == gimple_phi_K)
      {
         const auto gp = GetPointerS<gimple_phi>(GET_NODE(new_stmt));
         if(GET_NODE(gp->res) && GET_NODE(gp->res)->get_kind() == ssa_name_K)
         {
            GetPointerS<ssa_name>(GET_NODE(gp->res))->SetDefStmt(new_stmt);
         }
      }

      if(updated_ssa_uses)
      {
         const auto& uses = tree_helper::ComputeSsaUses(new_stmt);
         for(const auto& use : uses)
         {
            for(size_t counter = 0; counter < use.second; counter++)
            {
               GetPointerS<ssa_name>(GET_NODE(use.first))->AddUseStmt(new_stmt);
            }
         }
      }
   }
#if HAVE_BAMBU_BUILT
   if(schedule)
   {
      schedule->UpdateTime(new_stmt->index);
   }
#endif
}

const std::list<tree_nodeRef>& bloc::CGetStmtList() const
{
   return list_of_stmt;
}

void bloc::PushBefore(const tree_nodeRef new_stmt, const tree_nodeRef existing_stmt, const application_managerRef AppM)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT((!GET_NODE(new_stmt)) || (GET_NODE(new_stmt)->get_kind() != gimple_phi_K),
                "Adding phi " + new_stmt->ToString() + " to statements list");
   auto pos = list_of_stmt.begin();
   while(pos != list_of_stmt.end())
   {
      if((*pos)->index == existing_stmt->index)
      {
         break;
      }
      pos++;
   }
   THROW_ASSERT(pos != list_of_stmt.end(), existing_stmt->ToString() + " not found in BB" + STR(number));
   list_of_stmt.insert(pos, new_stmt);
   update_new_stmt(AppM, new_stmt);
}

void bloc::PushAfter(const tree_nodeRef new_stmt, const tree_nodeRef existing_stmt, const application_managerRef AppM)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT((!GET_NODE(new_stmt)) || (GET_NODE(new_stmt)->get_kind() != gimple_phi_K),
                "Adding phi " + new_stmt->ToString() + " to statements list");
   auto pos = list_of_stmt.begin();
   while(pos != list_of_stmt.end())
   {
      if((*pos)->index == existing_stmt->index)
      {
         break;
      }
      pos++;
   }
   pos++;
   list_of_stmt.insert(pos, new_stmt);
   update_new_stmt(AppM, new_stmt);
}

void bloc::PushFront(const tree_nodeRef statement, const application_managerRef AppM)
{
   THROW_ASSERT((!GET_NODE(statement)) || (GET_NODE(statement)->get_kind() != gimple_phi_K),
                "Adding phi " + statement->ToString() + " to statements list");
   if(list_of_stmt.size() && GET_NODE(list_of_stmt.front())->get_kind() != gimple_label_K)
   {
      list_of_stmt.push_front(statement);
   }
   else
   {
      list_of_stmt.insert(std::next(list_of_stmt.begin()), statement);
   }
   update_new_stmt(AppM, statement);
}

void bloc::PushBack(const tree_nodeRef statement, const application_managerRef AppM)
{
   THROW_ASSERT(number, "Trying to add " + statement->ToString() + " to entry");
   THROW_ASSERT((!GET_NODE(statement)) || (GET_NODE(statement)->get_kind() != gimple_phi_K),
                "Adding phi " + statement->ToString() + " to statements list");
   if(list_of_stmt.empty())
   {
      list_of_stmt.push_back(statement);
   }
   else
   {
      const auto& current_last_stmt = list_of_stmt.back();
      if(tree_helper::LastStatement(current_last_stmt))
      {
         list_of_stmt.insert(std::prev(list_of_stmt.end()), statement);
      }
      else
      {
         list_of_stmt.push_back(statement);
      }
   }
   update_new_stmt(AppM, statement);
}

void bloc::Replace(const tree_nodeRef old_stmt, const tree_nodeRef new_stmt, const bool move_virtuals,
                   const application_managerRef AppM)
{
#if HAVE_ASSERTS
   bool replaced = false;
#endif
   for(auto temp_stmt = list_of_stmt.begin(); temp_stmt != list_of_stmt.end(); temp_stmt++)
   {
      if((*temp_stmt)->index == old_stmt->index)
      {
#if HAVE_ASSERTS
         replaced = true;
#endif
         const auto next_stmt = std::next(temp_stmt);
         RemoveStmt(old_stmt, AppM);
         const auto old_ga = GetPointer<gimple_node>(GET_NODE(old_stmt));
         const auto new_ga = GetPointer<gimple_node>(GET_NODE(new_stmt));
         THROW_ASSERT(old_ga, "");
         THROW_ASSERT(new_ga, "");
         THROW_ASSERT(!old_ga->memdef || move_virtuals, STR(old_stmt) + " defines virtuals");
         if(move_virtuals)
         {
            if(old_ga->memdef)
            {
               THROW_ASSERT(!new_ga->memdef, "");
               new_ga->memdef = old_ga->memdef;
            }
            if(old_ga->memuse)
            {
               THROW_ASSERT(!new_ga->memuse, "");
               new_ga->memuse = old_ga->memuse;
            }
            if(old_ga->vdef)
            {
               THROW_ASSERT(!new_ga->vdef, "");
               new_ga->vdef = old_ga->vdef;
            }
            if(old_ga->vuses.size())
            {
               new_ga->vuses.insert(old_ga->vuses.begin(), old_ga->vuses.end());
            }
            if(old_ga->vovers.size())
            {
               new_ga->vovers.insert(old_ga->vovers.begin(), old_ga->vovers.end());
            }
         }
         if(next_stmt != list_of_stmt.end())
         {
            PushBefore(new_stmt, *next_stmt, AppM);
         }
         else
         {
            PushBack(new_stmt, AppM);
         }
         break;
      }
   }
   THROW_ASSERT(replaced, STR(old_stmt) + " not found");
}

void bloc::RemoveStmt(const tree_nodeRef statement, const application_managerRef AppM)
{
   const auto ga = GetPointer<gimple_assign>(GET_NODE(statement));
   if((ga && (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)) ||
      GET_NODE(statement)->get_kind() == gimple_call_K)
   {
      const auto cg_man = AppM->GetCallGraphManager();
      THROW_ASSERT(cg_man, "");
      unsigned int called_function_id;
      if(check_function_call(statement, ga, called_function_id))
      {
         THROW_ASSERT(GetPointerS<const gimple_node>(GET_NODE(statement))->scpe, "statement " + statement->ToString());
         const auto fun_id = GET_INDEX_NODE(GetPointerS<const gimple_node>(GET_NODE(statement))->scpe);
         const auto fun_cg_vertex = cg_man->GetVertex(fun_id);
         const auto cg = cg_man->CGetCallGraph();
         CustomOrderedSet<EdgeDescriptor> to_remove;
         OutEdgeIterator oei, oei_end;
         boost::tie(oei, oei_end) = boost::out_edges(fun_cg_vertex, *cg);
         const auto call_id = GET_INDEX_NODE(statement);
         for(; oei != oei_end; oei++)
         {
            const auto& direct_calls = cg->CGetFunctionEdgeInfo(*oei)->direct_call_points;
            auto call_it = direct_calls.find(call_id);
            if(call_it != direct_calls.end())
            {
               to_remove.insert(*oei);
            }
         }
         THROW_ASSERT(to_remove.size() || tree_helper::print_function_name(
                                              AppM->get_tree_manager(),
                                              GetPointer<const function_decl>(AppM->get_tree_manager()->CGetTreeNode(
                                                  called_function_id))) == BUILTIN_WAIT_CALL,
                      "Call to be removed not found in call graph " + STR(call_id) + " " + STR(fun_id) + " " +
                          STR(statement) + " | " +
                          tree_helper::print_function_name(AppM->get_tree_manager(),
                                                           GetPointerS<function_decl>(GET_NODE(
                                                               GetPointerS<gimple_node>(GET_NODE(statement))->scpe))));
         for(const auto& e : to_remove)
         {
            cg_man->RemoveCallPoint(e, call_id);
         }
      }
   }
#if HAVE_ASSERTS
   const auto original_size = list_of_stmt.size();
#endif
   for(auto temp_stmt = list_of_stmt.begin(); temp_stmt != list_of_stmt.end(); temp_stmt++)
   {
      if((*temp_stmt)->index == statement->index)
      {
         list_of_stmt.erase(temp_stmt);
         break;
      }
   }
   GetPointerS<gimple_node>(GET_NODE(statement))->bb_index = 0;
   THROW_ASSERT(original_size != list_of_stmt.size(),
                "Statement " + statement->ToString() + " not removed from BB" + STR(number));
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(statement);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointerS<ssa_name>(GET_NODE(use.first))->RemoveUse(statement);
         }
      }
   }
   // TODO: fix memdef and vdef
}

const std::list<tree_nodeRef>& bloc::CGetPhiList() const
{
   return list_of_phi;
}

void bloc::AddPhi(const tree_nodeRef phi)
{
   list_of_phi.push_back(phi);
   update_new_stmt(nullptr, phi);
}

void bloc::RemovePhi(const tree_nodeRef phi)
{
#if HAVE_ASSERTS
   const auto original_size = list_of_phi.size();
#endif
   for(auto temp_phi = list_of_phi.begin(); temp_phi != list_of_phi.end(); temp_phi++)
   {
      if((*temp_phi)->index == phi->index)
      {
         list_of_phi.erase(temp_phi);
         break;
      }
   }
   GetPointerS<gimple_node>(GET_NODE(phi))->bb_index = 0;
   removed_phi++;
   THROW_ASSERT(original_size != list_of_phi.size(), "Phi" + phi->ToString() + " not removed");
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(phi);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointerS<ssa_name>(GET_NODE(use.first))->RemoveUse(phi);
         }
      }
   }
   // TODO: fix memdef and vdef
}

size_t bloc::CGetNumberRemovedPhi() const
{
   return removed_phi;
}

void bloc::SetSSAUsesComputed()
{
   THROW_ASSERT(not updated_ssa_uses, "SSA uses already set as updated");
   updated_ssa_uses = true;
}
