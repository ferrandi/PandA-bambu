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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file OrderedInstructions.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ORDERED_INSTRUCTIONS_HPP
#define ORDERED_INSTRUCTIONS_HPP

#include "custom_map.hpp"
#include "refcount.hpp"

#include <list>
#include <memory>

CONSTREF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(tree_node);
struct gimple_node;

class OrderedBasicBlock
{
   /// Map a instruction to its position in a BasicBlock.
   CustomMap<unsigned int, unsigned int> NumberedInsts;

   /// Keep track of last instruction inserted into \p NumberedInsts.
   /// It speeds up queries for uncached instructions by providing a start point
   /// for new queries in OrderedBasicBlock::comesBefore.
   std::list<tree_nodeRef>::const_iterator LastInstFound;

   /// The position/number to tag the next instruction to be found.
   unsigned NextInstPos;

   /// The source BasicBlock instruction list
   const std::list<tree_nodeRef>& BBInst;

   /// The source BasicBlock to map.
   const blocRef BB;

   /// Given no cached results, find if \p A comes before \p B in \p BB.
   /// Cache and number out instruction while walking \p BB.
   bool instComesBefore(const tree_nodeConstRef& A, const tree_nodeConstRef& B);

 public:
   explicit OrderedBasicBlock(const blocRef& BasicB);

   /// Find out whether \p A dominates \p B, meaning whether \p A
   /// comes before \p B in \p BB. This is a simplification that considers
   /// cached instruction positions and ignores other basic blocks, being
   /// only relevant to compare relative instructions positions inside \p BB.
   /// Returns false for A == B.
   bool dominates(const tree_nodeConstRef& A, const tree_nodeConstRef& B);
};

class OrderedInstructions
{
   /// Used to check dominance for instructions in same basic block.
   mutable CustomMap<unsigned int, std::unique_ptr<OrderedBasicBlock>> OBBMap;

   /// The dominator tree of the parent function.
   const BBGraphConstRef DT;

 public:
   /// Constructor.
   explicit OrderedInstructions(BBGraphConstRef _DT);

   bool dominates(const unsigned int BBIA, const unsigned int BBIB) const;

   /// Return true if first instruction dominates the second.
   bool dominates(const tree_nodeConstRef& A, const tree_nodeConstRef& B) const;

   /// Invalidate the OrderedBasicBlock cache when its basic block changes.
   /// i.e. If an instruction is deleted or added to the basic block, the user
   /// should call this function to invalidate the OrderedBasicBlock cache for
   /// this basic block.
   void invalidateBlock(unsigned int BBI);

   const BBGraphConstRef& getDT() const;
};

#endif // ORDERED_INSTRUCTIONS_HPP