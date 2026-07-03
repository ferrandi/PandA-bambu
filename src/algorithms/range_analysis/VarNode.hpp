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
 * @file VarNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_VAR_NODE_HPP_
#define _RANGE_ANALYSIS_VAR_NODE_HPP_
#include "Range.hpp"
#include "refcount.hpp"

#include <functional>
#include <string>

CONSTREF_FORWARD_DECL(ir_node);

enum updateType
{
   ut_None = 0,
   ut_Range = 1,
   ut_BitValue = 2,
};

class VarNode
{
 public:
   using key_type = unsigned long long;
   using key_compare = std::less<key_type>;

   /**
    * @brief Construct a new Var Node object
    *
    * @param _V Represented SSA variable or constant
    * @param _function_id Function index of the function where the represented variable is defined
    * @param _use_bbi Index of the basic block where the represented variable is used
    */
   explicit VarNode(const ir_nodeConstRef& _V, unsigned int _function_id, unsigned int _use_bbi);
   VarNode(const VarNode&) = delete;
   VarNode(VarNode&&) = delete;
   VarNode& operator=(const VarNode&) = delete;
   VarNode& operator=(VarNode&&) = delete;

   /**
    * @brief Initializes the value of the node.
    *
    * @param outside
    */
   void init(bool outside);

   inline key_type getId() const
   {
      return id;
   }

   /**
    * @brief Returns the range of the variable represented by this node.
    *
    * @return Range
    */
   inline const Range& getRange() const
   {
      return interval;
   }

   /**
    * @brief Returns the variable represented by this node.
    *
    * @return const ir_nodeConstRef&
    */
   inline const ir_nodeConstRef& getValue() const
   {
      return V;
   }

   inline unsigned int getFunctionId() const
   {
      return function_id;
   }

   inline Range::bw_t getBitWidth() const
   {
      return interval.getBitWidth();
   }

   /**
    * @brief Changes the status of the variable represented by this node.
    *
    * @param newInterval
    */
   inline void setRange(const Range& newInterval)
   {
      interval = newInterval;
   }

   Range getMaxRange() const;

   inline char getAbstractState()
   {
      return abstractState;
   }

   /**
    * @brief The possible states are '0', '+', '-' and '?'.
    *
    */
   void storeAbstractState();

   /// Pretty print.
   void print(std::ostream& OS) const;
   std::string ToString() const;

   static key_type makeId(const ir_nodeConstRef& V, unsigned int use_bbi);

 private:
   const key_type id;

   /* The program variable */
   const ir_nodeConstRef V;

   /* ID of the associated function */
   unsigned int function_id;

   /* A Range associated to the variable, that is, its interval inferred by the analysis. */
   Range interval;

   /* Used by the crop meet operator */
   char abstractState;
};

inline std::ostream& operator<<(std::ostream& OS, const VarNode* VN)
{
   VN->print(OS);
   return OS;
}

#endif // _RANGE_ANALYSIS_VAR_NODE_HPP_