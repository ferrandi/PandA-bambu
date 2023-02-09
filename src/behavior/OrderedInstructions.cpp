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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file OrderedInstructions.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "OrderedInstructions.hpp"

#include "Dominance.hpp"
#include "basic_block.hpp"
#include "exceptions.hpp"
#include "tree_basic_block.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

bool OrderedBasicBlock::instComesBefore(const struct gimple_node* A, const struct gimple_node* B)
{
   const struct gimple_node* Inst = nullptr;
   THROW_ASSERT(!(LastInstFound == BBInst.end() && NextInstPos != 0), "Instruction supposed to be in NumberedInsts");

   // Start the search with the instruction found in the last lookup round.
   auto II = BBInst.begin();
   auto IE = BBInst.end();
   if(LastInstFound != IE)
   {
      II = std::next(LastInstFound);
   }

   // Number all instructions up to the point where we find 'A' or 'B'.
   for(; II != IE; ++II)
   {
      Inst = GetPointer<const gimple_node>(GET_CONST_NODE(*II));
      NumberedInsts[Inst] = NextInstPos++;
      if(Inst == A || Inst == B)
      {
         break;
      }
   }

   THROW_ASSERT(II != IE, "Instruction not found?");
   THROW_ASSERT((Inst == A || Inst == B), "Should find A or B");
   LastInstFound = II;
   return Inst != B;
}

OrderedBasicBlock::OrderedBasicBlock(const blocRef& BasicB)
    : LastInstFound(BasicB->CGetStmtList().end()), NextInstPos(0), BBInst(BasicB->CGetStmtList()), BB(BasicB)
{
   unsigned int phiPos = 0U;
   for(const auto& gp : BasicB->CGetPhiList())
   {
      NumberedInsts.insert({GetPointer<const gimple_node>(GET_CONST_NODE(gp)), phiPos++});
   }
}

bool OrderedBasicBlock::dominates(const struct gimple_node* A, const struct gimple_node* B)
{
   THROW_ASSERT(A->bb_index == B->bb_index, "Instructions must be in the same basic block!");
   THROW_ASSERT(A->bb_index == BB->number, "Instructions must be in the tracked block!");

   // Phi statements always comes before non-phi statements
   if(A->get_kind() == gimple_phi_K && B->get_kind() != gimple_phi_K)
   {
      return true;
   }
   else if(A->get_kind() != gimple_phi_K && B->get_kind() == gimple_phi_K)
   {
      return false;
   }

   // First we lookup the instructions. If they don't exist, lookup will give us
   // back ::end(). If they both exist, we compare the numbers. Otherwise, if NA
   // exists and NB doesn't, it means NA must come before NB because we would
   // have numbered NB as well if it didn't. The same is true for NB. If it
   // exists, but NA does not, NA must come after it. If neither exist, we need
   // to number the block and cache the results instComesBefore.
   const auto NAI = NumberedInsts.find(A);
   const auto NBI = NumberedInsts.find(B);
   if(NAI != NumberedInsts.end() && NBI != NumberedInsts.end())
   {
      return NAI->second < NBI->second;
   }
   if(NAI != NumberedInsts.end())
   {
      return B->get_kind() !=
             gimple_phi_K; // Not found phi nodes have been just added from this step in front of all other phis
   }
   if(NBI != NumberedInsts.end())
   {
      return false;
   }
   THROW_ASSERT(A->get_kind() != gimple_phi_K, "Non dato, not given, nicht gegeben, pas donné, no dado, non detur");

   return instComesBefore(A, B);
}

OrderedInstructions::OrderedInstructions(BBGraphConstRef _DT) : DT(_DT)
{
}

bool OrderedInstructions::dominates(const unsigned int BBIA, const unsigned int BBIB) const
{
   const auto& BBmap = DT->CGetBBGraphInfo()->bb_index_map;

   const auto BBAi_v = BBmap.find(BBIA);
   const auto BBBi_v = BBmap.find(BBIB);

   if(BBIA == BBIB)
   {
      return true;
   }

   if(BBAi_v->second == BBBi_v->second)
   {
      // Intermediate BB shadowing incoming BB
      const auto& b_pred = DT->CGetBBNodeInfo(BBBi_v->second)->block->list_of_pred;
      return std::find(b_pred.begin(), b_pred.end(), BBIA) != b_pred.end();
   }
   THROW_ASSERT(BBAi_v != BBmap.end(), "Unknown BB index (" + STR(BBIA) + ")");
   THROW_ASSERT(BBBi_v != BBmap.end(), "Unknown BB index (" + STR(BBIB) + ")");

   // An unreachable block is dominated by anything.
   if(!DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), BBBi_v->second))
   {
      return true;
   }

   // And dominates nothing.
   if(!DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), BBAi_v->second))
   {
      return false;
   }

   /// When block B is reachable from block A in the DT, A dominates B
   /// This because the DT used is a tree composed by immediate dominators only
   if(DT->IsReachable(BBAi_v->second, BBBi_v->second))
   {
      return true;
   }

   return false;
}

bool OrderedInstructions::dominates(const struct gimple_node* InstA, const struct gimple_node* InstB) const
{
   THROW_ASSERT(InstA, "Instruction A cannot be null");
   THROW_ASSERT(InstB, "Instruction B cannot be null");

   const auto BBIA = InstA->bb_index;
   const auto BBIB = InstB->bb_index;

   // Use ordered basic block to do dominance check in case the 2 instructions
   // are in the same basic block.
   if(BBIA == BBIB)
   {
      auto OBB = OBBMap.find(BBIA);
      if(OBB == OBBMap.end())
      {
         const auto BBmap = DT->CGetBBGraphInfo()->bb_index_map;
         const auto BBvertex = BBmap.find(BBIA);
         THROW_ASSERT(BBvertex != BBmap.end(), "Unknown BB index (" + STR(BBIA) + ")");

         const auto BB = DT->CGetBBNodeInfo(BBvertex->second)->block;
         THROW_ASSERT(BB->number == BBIA,
                      "Intermediate BB not allowed here"); // Intermediate BB shadows its incoming BB, thus its index
                                                           // is different from associated vertex
         OBB = OBBMap.insert({BBIA, absl::make_unique<OrderedBasicBlock>(BB)}).first;
      }
      return OBB->second->dominates(InstA, InstB);
   }

   return dominates(BBIA, BBIB);
}

void OrderedInstructions::invalidateBlock(unsigned int BBI)
{
   OBBMap.erase(BBI);
}

const BBGraphConstRef& OrderedInstructions::getDT() const
{
   return DT;
}
