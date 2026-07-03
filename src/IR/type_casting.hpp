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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file type_casting.hpp
 * @brief IR node visitor collecting the types used in type casting
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TYPE_CASTING_HPP
#define TYPE_CASTING_HPP
#include "ir_node.hpp"
#include "refcount.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

REF_FORWARD_DECL(type_casting);

#define LOCAL_OBJ_NOT_SPECIALIZED_SEQ                                                                            \
   (module_unit_node)(void_ty_node) TERNARY_EXPRESSION_IR_NODES(nop_stmt)(PointToSolution)(abs_node)(addr_node)( \
       not_node)(fptoi_node)(itofp_node)(unaligned_mem_access_node)(neg_node)(nop_node)(bitcast_node)(and_node)( \
       or_node)(xor_node)(eq_node)(ge_node)(gt_node)(le_node)(shl_node)(lt_node)(max_node)(min_node)(sub_node)(  \
       mul_node)(ne_node)(add_node)(gep_node)(fdiv_node)(shr_node)(idiv_node)(irem_node)(widen_mul_node)(        \
       extract_bit_node)(add_sat_node)(sub_sat_node)(extractvalue_node)(extractelement_node)(frem_node)

struct type_casting : public ir_node_visitor
{
   /// default constructor
   explicit type_casting(IRNodeConstSet& _types) : types(_types)
   {
   }

   void operator()(const mem_access_node* obj, unsigned int& mask) override;

   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECLO, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACROO, BOOST_PP_EMPTY, LOCAL_OBJ_NOT_SPECIALIZED_SEQ)

 private:
   /// set of types used in type casting
   IRNodeConstSet& types;

   /// already visited
   CustomUnorderedSet<unsigned int> visited;
};

#endif
