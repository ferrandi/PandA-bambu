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
 * @file VarNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_VAR_NODE_HPP_
#define _RANGE_ANALYSIS_VAR_NODE_HPP_
#include "Range.hpp"
#include "refcount.hpp"

#include <functional>
#include <string>

CONSTREF_FORWARD_DECL(tree_node);

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
   explicit VarNode(const tree_nodeConstRef& _V, unsigned int _function_id, unsigned int _use_bbi);
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
    * @return RangeConstRef
    */
   inline RangeConstRef getRange() const
   {
      return interval;
   }

   /**
    * @brief Returns the variable represented by this node.
    *
    * @return const tree_nodeConstRef&
    */
   inline const tree_nodeConstRef& getValue() const
   {
      return V;
   }

   inline unsigned int getFunctionId() const
   {
      return function_id;
   }

   inline Range::bw_t getBitWidth() const
   {
      return interval->getBitWidth();
   }

   /**
    * @brief Changes the status of the variable represented by this node.
    *
    * @param newInterval
    */
   inline void setRange(const RangeConstRef& newInterval)
   {
      interval.reset(newInterval->clone());
   }

   RangeRef getMaxRange() const;

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

   static key_type makeId(const tree_nodeConstRef& V, unsigned int use_bbi);

 private:
   const key_type id;

   /* The program variable */
   const tree_nodeConstRef V;

   /* ID of the associated function */
   unsigned int function_id;

   /* A Range associated to the variable, that is, its interval inferred by the analysis. */
   RangeConstRef interval;

   /* Used by the crop meet operator */
   char abstractState;
};

inline std::ostream& operator<<(std::ostream& OS, const VarNode* VN)
{
   VN->print(OS);
   return OS;
}

#endif // _RANGE_ANALYSIS_VAR_NODE_HPP_