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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file OrderedInstructions.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "OrderedInstructions.hpp"

#include "basic_block.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_node.hpp"

OrderedBasicBlock::OrderedBasicBlock(const blocRef& BasicB)
    : LastInstFound(BasicB->CGetStmtList().end()), NextInstPos(0), BBInst(BasicB->CGetStmtList()), BB(BasicB)
{
   unsigned int phiPos = 0U;
   for(const auto& gp : BasicB->CGetPhiList())
   {
      NumberedInsts.insert({gp->index, phiPos++});
   }
}

bool OrderedBasicBlock::instComesBefore(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const
{
   unsigned int idx = 0;
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
      idx = (*II)->index;
      NumberedInsts[idx] = NextInstPos++;
      if(idx == A->index || idx == B->index)
      {
         break;
      }
   }

   THROW_ASSERT(II != IE, "Instruction not found?");
   THROW_ASSERT((idx == A->index || idx == B->index), "Should find A or B");
   LastInstFound = II;
   return idx != B->index;
}

bool OrderedBasicBlock::dominates(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const
{
   THROW_ASSERT(GetPointerS<const node_stmt>(A)->bb_index == GetPointerS<const node_stmt>(B)->bb_index,
                "Instructions must be in the same basic block!");
   THROW_ASSERT(GetPointerS<const node_stmt>(A)->bb_index == BB->number, "Instructions must be in the tracked block!");

   // Phi statements always comes before non-phi statements
   if(A->get_kind() == phi_stmt_K && B->get_kind() != phi_stmt_K)
   {
      return true;
   }
   else if(A->get_kind() != phi_stmt_K && B->get_kind() == phi_stmt_K)
   {
      return false;
   }

   // First we lookup the instructions. If they don't exist, lookup will give us
   // back ::end(). If they both exist, we compare the numbers. Otherwise, if NA
   // exists and NB doesn't, it means NA must come before NB because we would
   // have numbered NB as well if it didn't. The same is true for NB. If it
   // exists, but NA does not, NA must come after it. If neither exist, we need
   // to number the block and cache the results instComesBefore.
   const auto NAI = NumberedInsts.find(A->index);
   const auto NBI = NumberedInsts.find(B->index);
   if(NAI != NumberedInsts.end() && NBI != NumberedInsts.end())
   {
      return NAI->second < NBI->second;
   }
   if(NAI != NumberedInsts.end())
   {
      return B->get_kind() !=
             phi_stmt_K; // Not found phi nodes have been just added from this step in front of all other phis
   }
   if(NBI != NumberedInsts.end())
   {
      return false;
   }
   THROW_ASSERT(A->get_kind() != phi_stmt_K, "Non dato, not given, nicht gegeben, pas donné, no dado, non detur");

   return instComesBefore(A, B);
}

struct DTVisitor : public boost::default_dfs_visitor
{
 public:
   DTVisitor(CustomMap<unsigned int, DFSInfo>& infos) : _step(0), _infos(infos)
   {
   }

   void discover_vertex(BBGraph::vertex_descriptor u, const BBGraph& g)
   {
      const auto& BB = g.CGetNodeInfo(u).block;
      _infos[BB->number].DFSIn = _step++;
   }

   void finish_vertex(BBGraph::vertex_descriptor u, const BBGraph& g)
   {
      const auto& BB = g.CGetNodeInfo(u).block;
      _infos[BB->number].DFSOut = _step++;
   }

 private:
   unsigned int _step;
   CustomMap<unsigned int, DFSInfo>& _infos;
};

static CustomMap<unsigned int, DFSInfo> compute_dominator_vector(const BBGraph& DT)
{
   CustomMap<unsigned int, DFSInfo> dominators;
   DTVisitor dtv(dominators);
   const auto entryVertex = DT.CGetGraphInfo().bb_index_map.at(bloc::ENTRY_BLOCK_ID);
   std::vector<boost::default_color_type> color_vec(DT.num_vertices(), boost::white_color);
   boost::depth_first_visit(
       DT, entryVertex, dtv,
       boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index, DT), boost::white_color));
   return dominators;
}

static CustomMap<unsigned int, OrderedBasicBlock> compute_ordered_bbs(const BBGraph& DT)
{
   CustomMap<unsigned int, OrderedBasicBlock> OBBmap;
   for(auto v : DT.vertices())
   {
      const auto& bb = DT.CGetNodeInfo(v).block;
      OBBmap.emplace(bb->number, OrderedBasicBlock(bb));
   }
   return OBBmap;
}

OrderedInstructions::OrderedInstructions(const BBGraph& DT)
    : OBBMap(compute_ordered_bbs(DT)), dominators(compute_dominator_vector(DT))
{
}

DFSInfo OrderedInstructions::info(unsigned int BBI) const
{
   const auto it = dominators.find(BBI);
   return it != dominators.end() ? it->second : DFSInfo();
}

bool OrderedInstructions::dominates(const unsigned int BBIA, const unsigned int BBIB) const
{
   if(BBIA == BBIB)
   {
      return true;
   }

   const auto a_it = dominators.find(BBIA);
   const auto b_it = dominators.find(BBIB);

   // An unreachable block is dominated by anything.
   if(b_it == dominators.end())
   {
      return true;
   }

   // And dominates nothing.
   if(a_it == dominators.end())
   {
      return false;
   }

   return a_it->second < b_it->second;
}

bool OrderedInstructions::dominates(const ir_nodeConstRef& InstA, const ir_nodeConstRef& InstB) const
{
   THROW_ASSERT(InstA, "Instruction A cannot be null");
   THROW_ASSERT(InstB, "Instruction B cannot be null");

   const auto BBIA = GetPointerS<const node_stmt>(InstA)->bb_index;
   const auto BBIB = GetPointerS<const node_stmt>(InstB)->bb_index;

   // Use ordered basic block to do dominance check in case the 2 instructions
   // are in the same basic block.
   if(BBIA == BBIB)
   {
      auto OBB = OBBMap.find(BBIA);
      THROW_ASSERT(OBB != OBBMap.end(), "BB" + std::to_string(BBIA) + " not found");
      return OBB->second.dominates(InstA, InstB);
   }
   return dominates(BBIA, BBIB);
}
