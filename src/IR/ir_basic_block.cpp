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
 * @file ir_basic_block.cpp
 * @brief Data structure describing a basic block at IR level.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "ir_basic_block.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"

const unsigned int bloc::ENTRY_BLOCK_ID = BB_ENTRY;
const unsigned int bloc::EXIT_BLOCK_ID = BB_EXIT;

bloc::bloc(unsigned int _number) : updated_ssa_uses(false), number(_number), loop_id(0)
{
}

void bloc::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   SEQ_VISIT_MEMBER(mask, list_of_phi, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_stmt, visit(v));
   /// live in and out not visited by design
}

bool bloc::check_function_call(const ir_nodeRef& statement, assign_stmt* ga, unsigned int& called_function_id)
{
   called_function_id = 0;
   if(ga)
   {
      const auto ce = GetPointerS<const call_node>(ga->op1);
      if(ce->fn->get_kind() == addr_node_K)
      {
         const auto fn = GetPointerS<const addr_node>(ce->fn)->op;
         if(fn->get_kind() == function_val_node_K)
         {
            called_function_id = fn->index;
            return true;
         }
      }
   }
   else
   {
      const auto gc = GetPointerS<call_stmt>(statement);
      if(gc->fn->get_kind() == addr_node_K)
      {
         const auto fn = GetPointerS<const addr_node>(gc->fn)->op;
         if(fn->get_kind() == function_val_node_K)
         {
            called_function_id = fn->index;
            return true;
         }
      }
   }
   return false;
}

void bloc::ReorderLUTs()
{
   IRNodeSet current_uses;
   for(const auto& phi : list_of_phi)
   {
      auto gp = GetPointer<phi_stmt>(phi);
      current_uses.insert(gp->res);
   }

   auto allDefinedP = [&](ir_nodeRef stmt) -> bool {
      const auto& uses = ir_helper::ComputeSsaUses(stmt);
      for(const auto& u : uses)
      {
         if(current_uses.find(u.first) == current_uses.end())
         {
            auto ssa_ref = u.first;
            auto ssa = GetPointer<ssa_node>(ssa_ref);
            if(ssa->virtual_flag || GetPointer<node_stmt>(ssa->GetDefStmt())->bb_index != number)
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

   /// Try to place postponed statements after a new SSA has been defined.
   /// Handles both SSA-defining and non-SSA-defining (predicated stores/calls) statements.
   auto drainPostponed = [&](std::list<ir_nodeRef>& list_of_postponed_stmt, std::list<ir_nodeRef>::iterator next_stmt) {
      bool restart_postponed = false;
      do
      {
         restart_postponed = false;
         auto posPostponed = list_of_postponed_stmt.begin();
         while(posPostponed != list_of_postponed_stmt.end())
         {
            if(allDefinedP(*posPostponed))
            {
               list_of_stmt.insert(next_stmt, *posPostponed);
               auto gaPostponed = GetPointer<assign_stmt>(*posPostponed);
               if(gaPostponed && gaPostponed->op0 && gaPostponed->op0->get_kind() == ssa_node_K)
               {
                  current_uses.insert(gaPostponed->op0);
                  restart_postponed = true;
               }
               if(schedule)
               {
                  schedule->UpdateTime((*posPostponed)->index);
               }
               posPostponed = list_of_postponed_stmt.erase(posPostponed);
               if(restart_postponed)
               {
                  break;
               }
            }
            else
            {
               ++posPostponed;
            }
         }
      } while(restart_postponed);
   };

   std::list<ir_nodeRef> list_of_postponed_stmt;
   auto pos = list_of_stmt.begin();
   while(pos != list_of_stmt.end())
   {
      if((*pos)->get_kind() == assign_stmt_K)
      {
         auto ga = GetPointer<assign_stmt>(*pos);
         if(ga->op0->get_kind() != ssa_node_K)
         {
            if(not allDefinedP(*pos))
            {
               list_of_postponed_stmt.push_back(*pos);
               const auto next_stmt = std::next(pos);
               list_of_stmt.erase(pos);
               pos = next_stmt;
            }
            else
            {
               ++pos;
            }
            continue;
         }

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
            drainPostponed(list_of_postponed_stmt, next_stmt);
            pos = next_stmt;
         }
      }
      else
      {
         if(not allDefinedP(*pos))
         {
            list_of_postponed_stmt.push_back(*pos);
            const auto next_stmt = std::next(pos);
            list_of_stmt.erase(pos);
            pos = next_stmt;
         }
         else
         {
            ++pos;
         }
      }
   }
   // Safety net: any stmt still in list_of_postponed_stmt could not be topologically ordered.
   // Re-append them to prevent silent loss of stmts (which leaves stale SSA use_stmts entries).
   for(const auto& postponed : list_of_postponed_stmt)
   {
      list_of_stmt.push_back(postponed);
      if(schedule)
      {
         schedule->UpdateTime(postponed->index);
      }
   }
}

void bloc::manageCallGraph(const application_managerRef& AppM, const ir_nodeRef& statement)
{
   const auto ga = GetPointer<assign_stmt>(statement);
   if((ga && ga->op1->get_kind() == call_node_K) || statement->get_kind() == call_stmt_K)
   {
      const auto& CGM = AppM->GetCallGraphManager();
      THROW_ASSERT(GetPointerS<const node_stmt>(statement)->parent, "statement " + statement->ToString());
      unsigned int called_function_id;
      if(check_function_call(statement, ga, called_function_id))
      {
         const auto function_id = GetPointerS<const node_stmt>(statement)->parent->index;
         if(CGM.IsVertex(function_id))
         {
            CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function_id,
                                                    statement->index, FunctionEdgeInfo::CallType::direct_call, 0);
         }
      }
   }
}

void bloc::update_new_stmt(const application_managerRef& AppM, const ir_nodeRef& new_stmt)
{
   THROW_ASSERT(new_stmt, "");
   if(AppM)
   {
      manageCallGraph(AppM, new_stmt);
   }
   const auto gn = GetPointer<node_stmt>(new_stmt);
   THROW_ASSERT(gn, "");
   gn->bb_index = number;

   if(GET_PTD_NODE(gn->vdef))
   {
      THROW_ASSERT(GET_PTD_NODE(gn->vdef)->get_kind() == ssa_node_K, "");
      GetPointerS<ssa_node>(GET_PTD_NODE(gn->vdef))->SetDefStmt(new_stmt);
   }

   if(GET_PTD_NODE(gn->memdef))
   {
      THROW_ASSERT(GET_PTD_NODE(gn->memdef)->get_kind() == ssa_node_K, "");
      GetPointerS<ssa_node>(GET_PTD_NODE(gn->memdef))->SetDefStmt(new_stmt);
   }

   if(new_stmt->get_kind() == assign_stmt_K)
   {
      const auto ga = GetPointerS<assign_stmt>(new_stmt);
      if(GET_PTD_NODE(ga->op0) && GET_PTD_NODE(ga->op0)->get_kind() == ssa_node_K)
      {
         GetPointerS<ssa_node>(GET_PTD_NODE(ga->op0))->SetDefStmt(new_stmt);
      }
   }
   else if(new_stmt->get_kind() == phi_stmt_K)
   {
      const auto gp = GetPointerS<phi_stmt>(new_stmt);
      if(GET_PTD_NODE(gp->res) && GET_PTD_NODE(gp->res)->get_kind() == ssa_node_K)
      {
         GetPointerS<ssa_node>(GET_PTD_NODE(gp->res))->SetDefStmt(new_stmt);
      }
   }

   if(updated_ssa_uses)
   {
      const auto& uses = ir_helper::ComputeSsaUses(new_stmt);
      for(const auto& [var, counter] : uses)
      {
         for(size_t i = 0; i < counter; ++i)
         {
            GetPointerS<ssa_node>(var)->AddUseStmt(new_stmt);
         }
      }
   }

   if(schedule)
   {
      schedule->UpdateTime(new_stmt->index);
   }
}

const std::list<ir_nodeRef>& bloc::CGetStmtList() const
{
   return list_of_stmt;
}

void bloc::PushBefore(const ir_nodeRef new_stmt, const ir_nodeRef existing_stmt, const application_managerRef AppM)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT((!new_stmt) || (new_stmt->get_kind() != phi_stmt_K),
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

void bloc::PushAfter(const ir_nodeRef new_stmt, const ir_nodeRef existing_stmt, const application_managerRef AppM)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT((!new_stmt) || (new_stmt->get_kind() != phi_stmt_K),
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

void bloc::PushFront(const ir_nodeRef statement, const application_managerRef AppM)
{
   THROW_ASSERT((!statement) || (statement->get_kind() != phi_stmt_K),
                "Adding phi " + statement->ToString() + " to statements list");
   if(list_of_stmt.size())
   {
      list_of_stmt.push_front(statement);
   }
   else
   {
      list_of_stmt.insert(std::next(list_of_stmt.begin()), statement);
   }
   update_new_stmt(AppM, statement);
}

void bloc::PushBack(const ir_nodeRef statement, const application_managerRef AppM)
{
   THROW_ASSERT(number, "Trying to add statement to entry");
   THROW_ASSERT(!GET_PTD_NODE(statement) || GET_PTD_NODE(statement)->get_kind() != phi_stmt_K,
                "Adding phi " + GET_PTD_NODE(statement)->ToString() + " to statements list");
   if(list_of_stmt.size() && GET_PTD_NODE(list_of_stmt.back()) && ir_helper::LastStatement(list_of_stmt.back()))
   {
      THROW_ASSERT(!GET_PTD_NODE(statement) || !ir_helper::LastStatement(GET_PTD_NODE(statement)),
                   "Expected one last statement only: last: " + STR(list_of_stmt.back()) +
                       " | curr: " + STR(statement));
      list_of_stmt.insert(std::prev(list_of_stmt.end()), statement);
   }
   else
   {
      list_of_stmt.push_back(statement);
   }
   if(GET_PTD_NODE(statement))
   {
      update_new_stmt(AppM, GET_PTD_NODE(statement));
   }
}

void bloc::Replace(const ir_nodeRef old_stmt, const ir_nodeRef new_stmt, const bool move_virtuals,
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
         const auto old_ga = GetPointer<node_stmt>(old_stmt);
         const auto new_ga = GetPointer<node_stmt>(new_stmt);
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

void bloc::RemoveStmt(const ir_nodeRef statement, const application_managerRef AppM)
{
   const auto ga = GetPointer<assign_stmt>(statement);
   if(AppM && ((ga && ga->op1->get_kind() == call_node_K) || statement->get_kind() == call_stmt_K))
   {
      auto& CGM = AppM->GetCallGraphManager();
      unsigned int called_function_id;
      if(check_function_call(statement, ga, called_function_id))
      {
         THROW_ASSERT(GetPointerS<const node_stmt>(statement)->parent, "statement " + statement->ToString());
         const auto fun_id = GetPointerS<const node_stmt>(statement)->parent->index;
         CGM.RemoveCallPoint(fun_id, called_function_id, statement->index);
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
   GetPointerS<node_stmt>(statement)->bb_index = 0;
   THROW_ASSERT(original_size != list_of_stmt.size(),
                "Statement " + statement->ToString() + " not removed from BB" + STR(number));
   if(updated_ssa_uses)
   {
      const auto& uses = ir_helper::ComputeSsaUses(statement);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointerS<ssa_node>(use.first)->RemoveUse(statement);
         }
      }
   }
   // TODO: fix memdef and vdef
}

const std::list<ir_nodeRef>& bloc::CGetPhiList() const
{
   return list_of_phi;
}

void bloc::AddPhi(const ir_nodeRef phi)
{
   list_of_phi.push_back(phi);
   if(GET_PTD_NODE(phi))
   {
      update_new_stmt(nullptr, GET_PTD_NODE(phi));
   }
}

void bloc::RemovePhi(const ir_nodeRef phi)
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
   GetPointerS<node_stmt>(phi)->bb_index = 0;
   THROW_ASSERT(original_size != list_of_phi.size(), "Phi" + phi->ToString() + " not removed");
   if(updated_ssa_uses)
   {
      const auto& uses = ir_helper::ComputeSsaUses(phi);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointerS<ssa_node>(use.first)->RemoveUse(phi);
         }
      }
   }
   // TODO: fix memdef and vdef
}

void bloc::SetSSAUsesComputed()
{
   THROW_ASSERT(not updated_ssa_uses, "SSA uses already set as updated");
   updated_ssa_uses = true;
}

std::string bloc::ToString() const
{
   std::string res;
   if(number != ENTRY_BLOCK_ID && number != EXIT_BLOCK_ID &&
      (list_of_pred.size() != 1 || list_of_pred.front() != ENTRY_BLOCK_ID))
   {
      res += "BB" + STR(number) + ":  ; preds =";
      bool first_p = false;
      for(auto p : list_of_pred)
      {
         if(!first_p)
         {
            first_p = true;
         }
         else
         {
            res += ", ";
         }
         res += "BB" + STR(p);
      }
      res += "\n";
   }
   for(const auto& ph : list_of_phi)
   {
      res += "  " + ph->ToString() + "\n";
   }
   for(const auto& stm : list_of_stmt)
   {
      res += "  " + stm->ToString();
      res += "\n";
   }
   if(!list_of_stmt.empty())
   {
      auto last_kind = list_of_stmt.back()->get_kind();
      if(last_kind == return_stmt_K || last_kind == multi_way_if_stmt_K)
      {
         /// do nothing
      }
      else
      {
         THROW_ASSERT(list_of_succ.size() == 1,
                      "unexpected pattern:" + STR(number) + " " + STR(list_of_succ.size()) + " " + res);
         res += "  br label BB" + STR(list_of_succ.front()) + "\n";
      }
   }

   return res;
}
