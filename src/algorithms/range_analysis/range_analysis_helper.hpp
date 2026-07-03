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
 * @file range_analysis_helper.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_HELPER_HPP_
#define _RANGE_ANALYSIS_HELPER_HPP_

#include "Range.hpp"
#include "ir_common.hpp"
#include "refcount.hpp"

struct binary_node;
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(ir_node);

#define CASE_MISCELLANEOUS     \
   lut_node_K:                 \
   case constructor_node_K:    \
   case identifier_node_K:     \
   case ssa_node_K:            \
   case statement_list_node_K: \
   case call_node_K

#define RANGE_SPECIFIC_CODE_LIST \
   RANGECODE(unsigned_eq_node)   \
   RANGECODE(unsigned_le_node)   \
   RANGECODE(unsigned_lt_node)   \
   RANGECODE(unsigned_ge_node)   \
   RANGECODE(unsigned_gt_node)

enum kind_R : int
{
#define RANGECODE(SYM) SYM##_R,
   RANGE_SPECIFIC_CODE_LIST
#include "range.def"
       error_R
};
#undef RANGECODE

namespace range_analysis
{
   kind_R op_unsigned(kind_R op);

   kind_R op_inv(kind_R op);

   kind_R op_swap(kind_R op);

   bool isCompare(kind_R c_type);

   bool isCompare(const struct binary_node* condition);

   kind_R op_convert(kind op);

   std::string getString(kind_R op);

   /**
    * @brief Traverse IR backwards through cast statements until the proper SSA variable definition is found
    * This method traverse SSA definitions backwards through nop_node, single-edge phis and returns the
    * proper SSA variable definition, which may be a assign_stmt, phi_stmt, or nop_stmt statement.
    *
    * @param var SSA variable IR node
    * @return ir_nodeConstRef Proper SSA variable definition before any cast statement
    */
   ir_nodeConstRef castTraverse(ir_nodeConstRef var);

   /**
    * @brief Traverse IR backwards through cast statements until the proper SSA variable definition is found
    * This method traverse SSA definitions backwards through nop_node, single-edge phis and returns the
    * proper SSA variable IR node from the proper definition statement.
    *
    * @param var SSA variable IR node
    * @return ir_nodeConstRef Proper SSA variable IR node
    */
   ir_nodeConstRef castTraverseSSA(ir_nodeConstRef var);

   Range evaluate(kind_R opcode, Range::bw_t bw, const Range& op1, const Range& op2, bool opSigned);

   Range staticCompare(kind_R compare_op, Range::bw_t bw, const ir_nodeConstRef& op0, const ir_nodeConstRef& op1);

   bool isValidType(const ir_nodeConstRef& _tn);

   bool isValidInstruction(const ir_nodeConstRef& stmt, const FunctionBehaviorConstRef& FB);

   bool isSignedType(const ir_nodeConstRef& _tn);

   Range makeSatisfyingCmpRegion(kind_R pred, const Range& Other);
} // namespace range_analysis

#endif // _RANGE_ANALYSIS_HELPER_HPP_
