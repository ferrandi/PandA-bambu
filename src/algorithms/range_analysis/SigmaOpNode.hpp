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
 * @file SigmaOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_
#define _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_
#include "UnaryOpNode.hpp"

class SigmaOpNode : public UnaryOpNode
{
 private:
   // The symbolic source node of the operation.
   VarNode* SymbolicSource;

   bool unresolved;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range eval() const override;

 public:
   SigmaOpNode(const ValueRangeRef& intersect, VarNode* sink, VarNode* source);
   SigmaOpNode(const SigmaOpNode&) = delete;
   SigmaOpNode(SigmaOpNode&&) = delete;
   SigmaOpNode& operator=(const SigmaOpNode&) = delete;
   SigmaOpNode& operator=(SigmaOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   std::string getName() const override;
   void print(std::ostream& OS) const override;

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
};

#endif // _RANGE_ANALYSIS_SIGMA_OP_NODE_HPP_
