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
 * @file UnaryOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_
#define _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_
#include "OpNode.hpp"
#include "refcount.hpp"
#include "tree_common.hpp"

#include <functional>

class NodeContainer;
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(application_manager);

class UnaryOpNode : public OpNode
{
 private:
   // The source node of the operation.
   VarNode* source;

   // The opcode of the operation.
   kind opcode;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   UnaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source,
               kind opcode);
   UnaryOpNode(const UnaryOpNode&) = delete;
   UnaryOpNode(UnaryOpNode&&) = delete;
   UnaryOpNode& operator=(const UnaryOpNode&) = delete;
   UnaryOpNode& operator=(UnaryOpNode&&) = delete;

   OperationId getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   inline kind getOpcode() const
   {
      return opcode;
   }

   inline VarNode* getSource() const
   {
      return source;
   }

   static inline bool classof(UnaryOpNode const*)
   {
      return true;
   }

   static inline bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::UnaryOpId || BO->getValueId() == OperationId::SigmaOpId;
   }

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_