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
 * @file OpNode.hpp
 * @brief This class represents a generic operation in range analysis
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_OP_NODE_HPP_
#define _RANGE_ANALYSIS_OP_NODE_HPP_
#include "ValueRange.hpp"
#include "VarNode.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(ValueRange);
REF_FORWARD_DECL(ValueRange);

class OpNode
{
 private:
   /// The range of the operation. Each operation has a range associated to it.
   /// This range is obtained by inspecting the branches in the source program
   /// and extracting its condition and intervals.
   ValueRangeRef intersect;
   // The target of the operation, that is, the node which
   // will store the result of the operation.
   VarNode* sink;
   // The instruction that originated this op node
   const tree_nodeConstRef inst;

 protected:
   /// We do not want people creating objects of this class,
   /// but we want to inherit from it.
   OpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);

 public:
   enum class OperationId
   {
      UnaryOpId,
      SigmaOpId,
      BinaryOpId,
      TernaryOpId,
      PhiOpId,
      ControlDepId,
      LoadOpId,
      StoreOpId
   };

#ifndef NDEBUG
   static int debug_level;
#endif

   /// The dtor. It's virtual because this is a base class.
   virtual ~OpNode() = default;
   // We do not want people creating objects of this class.
   OpNode(const OpNode&) = delete;
   OpNode(OpNode&&) = delete;
   OpNode& operator=(const OpNode&) = delete;
   OpNode& operator=(OpNode&&) = delete;

   /**
    * @brief Return the instruction that originated this op node
    *
    * @return const tree_nodeConstRef&
    */
   inline const tree_nodeConstRef& getInstruction() const
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
   inline void setIntersect(const RangeConstRef& newIntersect)
   {
      intersect->setRange(newIntersect);
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
    *
    * @param future
    */
   void solveFuture();

   /**
    * @brief Given the input of the operation and the operation that will be performed, evaluates the result of the
    * operation.
    *
    * @return RangeRef
    */
   virtual RangeRef eval() const = 0;

   virtual std::vector<VarNode*> getSources() const = 0;

   /// Prints the content of the operation.
   virtual void print(std::ostream& OS) const = 0;
   virtual void printDot(std::ostream& OS) const = 0;

   inline std::string ToString() const
   {
      std::stringstream ss;
      print(ss);
      return ss.str();
   }

   virtual OperationId getValueId() const = 0;

   static inline bool classof(OpNode const*)
   {
      return true;
   }
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