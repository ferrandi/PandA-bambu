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
 * @file OpNode.hpp
 * @brief This class represents a generic operation in range analysis
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_OP_NODE_HPP_
#define _RANGE_ANALYSIS_OP_NODE_HPP_
#include "ValueRange.hpp"
#include "VarNode.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(ValueRange);
REF_FORWARD_DECL(ValueRange);

class OpNode
{
 public:
   enum OpNodeType
   {
      OpNodeType_Unary,
      OpNodeType_Sigma,
      OpNodeType_Binary,
      OpNodeType_Ternary,
      OpNodeType_Phi,
      OpNodeType_ControlDep,
      OpNodeType_Load
   };

   virtual ~OpNode() = default;
   OpNode(const OpNode&) = delete;
   OpNode(OpNode&&) = delete;
   OpNode& operator=(const OpNode&) = delete;
   OpNode& operator=(OpNode&&) = delete;

   /**
    * @brief Return the instruction that originated this op node
    *
    * @return const ir_nodeConstRef&
    */
   inline ir_nodeConstRef getInstruction() const
   {
      return inst;
   }

   /**
    * @brief Returns the range of the operation.
    *
    * @return ValueRangeConstRef
    */
   inline ValueRangeConstRef getIntersect() const
   {
      return intersect;
   }

   /**
    * @brief Changes the interval of the operation.
    *
    * @param newIntersect
    */
   inline void setIntersect(const Range& newIntersect)
   {
      intersect->setRange(newIntersect);
   }

   inline void setIntersect(const ValueRangeRef& _intersect)
   {
      intersect = _intersect;
   }

   /**
    * @brief Returns the target of the operation, that is, where the result will be stored.
    *
    * @return VarNode*
    */
   inline VarNode* getSink() const
   {
      return sink;
   }

   /**
    * @brief Replace symbolic intervals with hard-wired constants.
    */
   void solveFuture();

   /**
    * @brief Given the input of the operation and the operation that will be performed, evaluates the result of the
    * operation.
    *
    * @return Range
    */
   virtual Range eval() const = 0;

   virtual std::vector<VarNode*> getSources() const = 0;

   virtual void replaceSource(const VarNode* _old, VarNode* _new) = 0;

   /// Prints the content of the operation.
   virtual std::string getName() const = 0;
   virtual void print(std::ostream& OS) const = 0;

   inline std::string ToString() const
   {
      std::stringstream ss;
      print(ss);
      return ss.str();
   }

   virtual OpNodeType getValueId() const = 0;

#ifndef NDEBUG
   static int debug_level;
#endif

   static inline bool classof(OpNode const*)
   {
      return true;
   }

 protected:
   OpNode(VarNode* sink, const ir_nodeConstRef& inst);

 private:
   /**
    * @brief The range of the operation
    * Each operation has a range associated to it. This range is obtained by inspecting the branches in the source
    * program and extracting its condition and intervals.
    */
   ValueRangeRef intersect;

   // The target of the operation, that is, the node which
   // will store the result of the operation.
   VarNode* sink;

   /* The instruction that originated this op node */
   const ir_nodeConstRef inst;
};

template <typename T>
inline T* GetOp(OpNode* t)
{
   return T::classof(t) ? static_cast<T*>(t) : nullptr;
}

template <typename T>
inline const T* GetOp(const OpNode* t)
{
   return T::classof(t) ? static_cast<const T*>(t) : nullptr;
}

#endif // _RANGE_ANALYSIS_OP_NODE_HPP_
