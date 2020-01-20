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

#if HAVE_BAMBU_BUILT
/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// tree includes
#include "string_manipulation.hpp" // for STR
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

const unsigned int bloc::ENTRY_BLOCK_ID = 0;
const unsigned int bloc::EXIT_BLOCK_ID = 1;

bloc::bloc(unsigned int _number) : removed_phi(0), updated_ssa_uses(false), number(_number), loop_id(0), hpl(0), true_edge(0), false_edge(0)
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

void bloc::PushFront(const tree_nodeRef statement)
{
   THROW_ASSERT(not GET_NODE(statement) or GET_NODE(statement)->get_kind() != gimple_phi_K, "Adding phi " + statement->ToString() + " to statements list");
   if(list_of_stmt.size() and GET_NODE(list_of_stmt.front())->get_kind() != gimple_label_K)
   {
      list_of_stmt.push_front(statement);
   }
   else
   {
      list_of_stmt.insert(std::next(list_of_stmt.begin()), statement);
   }
   /// This check is necessary since during parsing of statement list statement has not yet been filled
   if(GET_NODE(statement) and GetPointer<gimple_node>(GET_NODE(statement)))
   {
      GetPointer<gimple_node>(GET_NODE(statement))->bb_index = number;
   }
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(statement);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->AddUseStmt(statement);
         }
      }
   }
   auto gn = GetPointer<gimple_node>(GET_NODE(statement));
   if(gn->vdef)
   {
      GetPointer<ssa_name>(GET_NODE(gn->vdef))->SetDefStmt(statement);
   }
   auto ga = GetPointer<gimple_assign>(GET_NODE(statement));
   if(ga)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(ga->op0));
      if(sn)
      {
         sn->SetDefStmt(statement);
      }
   }
#if HAVE_BAMBU_BUILT
   if(schedule)
   {
      schedule->UpdateTime(statement->index);
   }
#endif
}

void bloc::PushBack(const tree_nodeRef statement)
{
   THROW_ASSERT(number, "Trying to add " + statement->ToString() + " to entry");
   THROW_ASSERT(not GET_NODE(statement) or GET_NODE(statement)->get_kind() != gimple_phi_K, "Adding phi " + statement->ToString() + " to statements list");
   if(list_of_stmt.empty())
   {
      list_of_stmt.push_back(statement);
   }
   else
   {
      auto current_last_stmt = list_of_stmt.back();
      if(tree_helper::LastStatement(current_last_stmt))
      {
         list_of_stmt.insert(std::prev(list_of_stmt.end()), statement);
      }
      else
      {
         list_of_stmt.push_back(statement);
      }
   }
   /// This check is necessary since during parsing of statement list statement has not yet been filled
   if(GET_NODE(statement) and GetPointer<gimple_node>(GET_NODE(statement)))
   {
      GetPointer<gimple_node>(GET_NODE(statement))->bb_index = number;
      auto gn = GetPointer<gimple_node>(GET_NODE(statement));
      if(gn->vdef and GET_NODE(gn->vdef))
      {
         GetPointer<ssa_name>(GET_NODE(gn->vdef))->SetDefStmt(statement);
      }
      auto ga = GetPointer<gimple_assign>(GET_NODE(statement));
      if(ga)
      {
         auto sn = GetPointer<ssa_name>(GET_NODE(ga->op0));
         if(sn)
         {
            sn->SetDefStmt(statement);
         }
      }
   }
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(statement);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->AddUseStmt(statement);
         }
      }
   }
#if HAVE_BAMBU_BUILT
   if(schedule)
   {
      schedule->UpdateTime(statement->index);
   }
#endif
}

void bloc::Replace(const tree_nodeRef old_stmt, const tree_nodeRef new_stmt, const bool move_virtuals)
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
         RemoveStmt(old_stmt);
         const auto old_ga = GetPointer<gimple_node>(GET_NODE(old_stmt));
         const auto new_ga = GetPointer<gimple_node>(GET_NODE(new_stmt));
         THROW_ASSERT(old_ga, "");
         THROW_ASSERT(new_ga, "");
         THROW_ASSERT(not old_ga->memdef or move_virtuals, STR(old_stmt) + " defines virtuals");
         if(move_virtuals)
         {
            if(old_ga->memdef)
            {
               new_ga->memdef = old_ga->memdef;
               GetPointer<ssa_name>(GET_NODE(new_ga->memdef))->SetDefStmt(new_stmt);
            }
            if(old_ga->memuse)
            {
               new_ga->memuse = old_ga->memuse;
               GetPointer<ssa_name>(GET_NODE(new_ga->memuse))->AddUseStmt(new_stmt);
            }
            if(old_ga->vdef)
            {
               new_ga->vdef = old_ga->vdef;
               GetPointer<ssa_name>(GET_NODE(new_ga->vdef))->SetDefStmt(new_stmt);
            }
            if(old_ga->vuses.size())
            {
               new_ga->vuses = old_ga->vuses;
               for(const auto& v : new_ga->vuses)
               {
                  GetPointer<ssa_name>(GET_NODE(v))->AddUseStmt(new_stmt);
               }
            }
            if(old_ga->vovers.size())
            {
               new_ga->vovers = old_ga->vovers;
            }
         }
         if(next_stmt != list_of_stmt.end())
            PushBefore(new_stmt, *next_stmt);
         else
            PushBack(new_stmt);
         break;
      }
   }
   THROW_ASSERT(replaced, STR(old_stmt) + " not found");
}

void bloc::PushBefore(const tree_nodeRef new_stmt, const tree_nodeRef existing_stmt)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT(not GET_NODE(new_stmt) or GET_NODE(new_stmt)->get_kind() != gimple_phi_K, "Adding phi " + new_stmt->ToString() + " to statements list");
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
   auto gn = GetPointer<gimple_node>(GET_NODE(new_stmt));
   THROW_ASSERT(gn, STR(new_stmt));
   gn->bb_index = number;
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(new_stmt);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->AddUseStmt(new_stmt);
         }
      }
   }
   if(gn->vdef)
   {
      GetPointer<ssa_name>(GET_NODE(gn->vdef))->SetDefStmt(new_stmt);
   }
   auto ga = GetPointer<gimple_assign>(GET_NODE(new_stmt));
   if(ga)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(ga->op0));
      if(sn)
      {
         sn->SetDefStmt(new_stmt);
      }
   }
#if HAVE_BAMBU_BUILT
   if(schedule)
   {
      schedule->UpdateTime(new_stmt->index);
   }
#endif
}

void bloc::PushAfter(const tree_nodeRef new_stmt, const tree_nodeRef existing_stmt)
{
   THROW_ASSERT(number != ENTRY_BLOCK_ID, "Trying to add " + new_stmt->ToString() + " to entry");
   THROW_ASSERT(not GET_NODE(new_stmt) or GET_NODE(new_stmt)->get_kind() != gimple_phi_K, "Adding phi " + new_stmt->ToString() + " to statements list");
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
   GetPointer<gimple_node>(GET_NODE(new_stmt))->bb_index = number;
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(new_stmt);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->AddUseStmt(new_stmt);
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

void bloc::ReorderLUTs()
{
   TreeNodeSet current_uses;
   for(auto phi : list_of_phi)
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
            for(auto u : uses)
            {
               if(current_uses.find(u.first) == current_uses.end())
               {
                  auto ssa_node = GET_NODE(u.first);
                  auto ssa = GetPointer<ssa_name>(ssa_node);
                  if(ssa->virtual_flag || GetPointer<gimple_node>(GET_NODE(ssa->CGetDefStmt()))->bb_index != number)
                     current_uses.insert(u.first);
                  else
                     return false;
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
                     ++posPostponed;
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

void bloc::AddPhi(const tree_nodeRef phi)
{
   /// This check is necessary since during parsing of statement list statement has not yet been filled
   if(GET_NODE(phi) and GetPointer<gimple_phi>(GET_NODE(phi)))
      GetPointer<gimple_phi>(GET_NODE(phi))->bb_index = number;
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(phi);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->AddUseStmt(phi);
         }
      }
   }
   if(GET_NODE(phi) and GetPointer<gimple_phi>(GET_NODE(phi)))
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      if(GET_NODE(gp->res) and GetPointer<ssa_name>(GET_NODE(gp->res)))
      {
         auto sn = GetPointer<ssa_name>(GET_NODE(gp->res));
         sn->SetDefStmt(phi);
      }
   }
   list_of_phi.push_back(phi);
#if HAVE_BAMBU_BUILT
   if(schedule)
   {
      schedule->UpdateTime(phi->index);
   }
#endif
}

const std::list<tree_nodeRef>& bloc::CGetPhiList() const
{
   return list_of_phi;
}

const std::list<tree_nodeRef>& bloc::CGetStmtList() const
{
   return list_of_stmt;
}

void bloc::RemoveStmt(const tree_nodeRef statement)
{
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
   GetPointer<gimple_node>(GET_NODE(statement))->bb_index = 0;
   THROW_ASSERT(original_size != list_of_stmt.size(), "Statement " + statement->ToString() + " not removed from BB" + STR(number));
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(statement);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->RemoveUse(statement);
         }
      }
   }
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
   GetPointer<gimple_node>(GET_NODE(phi))->bb_index = 0;
   removed_phi++;
   THROW_ASSERT(original_size != list_of_phi.size(), "Phi" + phi->ToString() + " not removed");
   if(updated_ssa_uses)
   {
      const auto& uses = tree_helper::ComputeSsaUses(phi);
      for(const auto& use : uses)
      {
         for(size_t counter = 0; counter < use.second; counter++)
         {
            GetPointer<ssa_name>(GET_NODE(use.first))->RemoveUse(phi);
         }
      }
   }
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
