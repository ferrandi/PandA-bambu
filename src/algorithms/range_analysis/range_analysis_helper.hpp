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
 * @file range_analysis_helper.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_HELPER_HPP_
#define _RANGE_ANALYSIS_HELPER_HPP_

#include "refcount.hpp"
#include "tree_common.hpp"

struct binary_expr;
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(Range);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(Range);

#define CASE_MISCELLANEOUS   \
   aggr_init_expr_K:         \
   case case_label_expr_K:   \
   case lut_expr_K:          \
   case target_expr_K:       \
   case target_mem_ref_K:    \
   case target_mem_ref461_K: \
   case binfo_K:             \
   case block_K:             \
   case constructor_K:       \
   case error_mark_K:        \
   case identifier_node_K:   \
   case ssa_name_K:          \
   case statement_list_K:    \
   case tree_list_K:         \
   case tree_vec_K:          \
   case call_expr_K

namespace range_analysis
{
   kind op_unsigned(kind op);

   kind op_inv(kind op);

   kind op_swap(kind op);

   bool isCompare(kind c_type);

   bool isCompare(const struct binary_expr* condition);

   tree_nodeConstRef branchOpRecurse(const tree_nodeConstRef op);

   bool isValidType(const tree_nodeConstRef& _tn);

   bool isValidInstruction(const tree_nodeConstRef& stmt, const FunctionBehaviorConstRef& FB);

   bool isSignedType(const tree_nodeConstRef& _tn);

   RangeRef makeSatisfyingCmpRegion(kind pred, const RangeConstRef& Other);
} // namespace range_analysis

#endif // _RANGE_ANALYSIS_HELPER_HPP_