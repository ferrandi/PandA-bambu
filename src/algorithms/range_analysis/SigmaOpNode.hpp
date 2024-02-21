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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file SigmaOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_
#define _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_
#include "UnaryOpNode.hpp"

#include <functional>

class SigmaOpNode : public UnaryOpNode
{
 private:
   // The symbolic source node of the operation.
   VarNode* SymbolicSource;

   bool unresolved;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   SigmaOpNode(VarNode* sink, VarNode* source, const tree_nodeConstRef& inst, kind opcode);
   SigmaOpNode(const ValueRangeRef& intersect, VarNode* sink, VarNode* source, const tree_nodeConstRef& inst,
               kind opcode);
   SigmaOpNode(const SigmaOpNode&) = delete;
   SigmaOpNode(SigmaOpNode&&) = delete;
   SigmaOpNode& operator=(const SigmaOpNode&) = delete;
   SigmaOpNode& operator=(SigmaOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   inline bool isUnresolved() const
   {
      return unresolved;
   }

   inline void markResolved()
   {
      unresolved = false;
   }

   inline void markUnresolved()
   {
      unresolved = true;
   }

   static inline bool classof(SigmaOpNode const*)
   {
      return true;
   }

   static inline bool classof(UnaryOpNode const* UO)
   {
      return UO->getValueId() == OpNodeType::OpNodeType_Sigma;
   }

   static inline bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OpNodeType::OpNodeType_Sigma;
   }

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_