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
 * @file BinaryOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_BINARY_OP_NODE_HPP_
#define _RANGE_ANALYSIS_BINARY_OP_NODE_HPP_
#include "OpNode.hpp"
#include "refcount.hpp"
#include "tree_common.hpp"

class NodeContainer;
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(application_manager);

class BinaryOpNode : public OpNode
{
 private:
   // The first operand.
   VarNode* source1;

   // The second operand.
   VarNode* source2;

   // The opcode of the operation.
   kind opcode;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   BinaryOpNode(VarNode* sink, VarNode* source1, VarNode* source2, const tree_nodeConstRef& inst, kind opcode);
   BinaryOpNode(const BinaryOpNode&) = delete;
   BinaryOpNode(BinaryOpNode&&) = delete;
   BinaryOpNode& operator=(const BinaryOpNode&) = delete;
   BinaryOpNode& operator=(BinaryOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void replaceSource(VarNode* _old, VarNode* _new) override;

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   inline kind getOpcode() const
   {
      return opcode;
   }

   inline VarNode* getSource1() const
   {
      return source1;
   }

   inline VarNode* getSource2() const
   {
      return source2;
   }

   static inline bool classof(BinaryOpNode const* /*unused*/)
   {
      return true;
   }

   static inline bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OpNodeType::OpNodeType_Binary;
   }

   static RangeRef evaluate(kind opcode, Range::bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2,
                            bool opSigned);

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_BINARY_OP_NODE_HPP_