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
 * @file OrderedInstructions.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef ORDERED_INSTRUCTIONS_HPP
#define ORDERED_INSTRUCTIONS_HPP

#include "custom_map.hpp"
#include "refcount.hpp"

#include <list>
#include <memory>

CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_node);
struct BBGraph;
struct node_stmt;

class OrderedBasicBlock
{
   /// Map a instruction to its position in a BasicBlock.
   mutable CustomMap<unsigned int, unsigned int> NumberedInsts;

   /// Keep track of last instruction inserted into \p NumberedInsts.
   /// It speeds up queries for uncached instructions by providing a start point
   /// for new queries in OrderedBasicBlock::comesBefore.
   mutable std::list<ir_nodeRef>::const_iterator LastInstFound;

   /// The position/number to tag the next instruction to be found.
   mutable unsigned NextInstPos;

   /// The source BasicBlock instruction list
   const std::list<ir_nodeRef>& BBInst;

   /// The source BasicBlock to map.
   const blocRef BB;

   /// Given no cached results, find if \p A comes before \p B in \p BB.
   /// Cache and number out instruction while walking \p BB.
   bool instComesBefore(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const;

 public:
   explicit OrderedBasicBlock(const blocRef& BasicB);

   /// Find out whether \p A dominates \p B, meaning whether \p A
   /// comes before \p B in \p BB. This is a simplification that considers
   /// cached instruction positions and ignores other basic blocks, being
   /// only relevant to compare relative instructions positions inside \p BB.
   /// Returns false for A == B.
   bool dominates(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const;
};

struct DFSInfo
{
   unsigned int DFSIn{0};
   unsigned int DFSOut{0};

   bool operator==(const DFSInfo& other) const
   {
      return DFSIn == other.DFSIn && DFSOut == other.DFSOut;
   }

   bool operator<(const DFSInfo& other) const
   {
      return DFSIn < other.DFSIn && other.DFSOut < DFSOut;
   }

   bool operator<=(const DFSInfo& other) const
   {
      return DFSIn <= other.DFSIn && other.DFSOut <= DFSOut;
   }
};

class OrderedInstructions
{
   /// Used to check dominance for instructions in same basic block.
   const CustomMap<unsigned int, OrderedBasicBlock> OBBMap;

   const CustomMap<unsigned int, DFSInfo> dominators;

 public:
   explicit OrderedInstructions(const BBGraph& _DT);

   DFSInfo info(unsigned int BBI) const;

   bool dominates(const unsigned int BBIA, const unsigned int BBIB) const;

   /// Return true if first instruction dominates the second.
   bool dominates(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const;
};

#endif // ORDERED_INSTRUCTIONS_HPP