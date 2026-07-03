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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file UnaryOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_
#define _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_
#include "OpNode.hpp"
#include "range_analysis_helper.hpp"
#include "refcount.hpp"

#include <functional>

class NodeContainer;
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(application_manager);

class UnaryOpNode : public OpNode
{
 private:
   // The source node of the operation.
   VarNode* source;

   // The opcode of the operation.
   kind_R opcode;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range eval() const override;

 public:
   UnaryOpNode(VarNode* sink, VarNode* source, const ir_nodeConstRef& inst, kind_R opcode);
   UnaryOpNode(const UnaryOpNode&) = delete;
   UnaryOpNode(UnaryOpNode&&) = delete;
   UnaryOpNode& operator=(const UnaryOpNode&) = delete;
   UnaryOpNode& operator=(UnaryOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void replaceSource(const VarNode* _old, VarNode* _new) override;

   std::string getName() const override;
   void print(std::ostream& OS) const override;

   inline kind_R getOpcode() const
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
      return BO->getValueId() == OpNodeType::OpNodeType_Unary || BO->getValueId() == OpNodeType::OpNodeType_Sigma;
   }

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_UNARY_OP_NODE_HPP_
