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
 * @file ir_common.hpp
 * @brief This C++ header file contains common macros for the IR structure
 *
 * This C++ header file define some macros useful during the IR structure hierarchy definition and manipulation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IR_COMMON_HPP
#define IR_COMMON_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#define IR_NODE_NAME(r, data, elem) #elem,

#define IR_NODE_KIND(r, data, elem) BOOST_PP_CAT(elem, _K),

#define UNARY_EXPRESSION_IR_NODES                                                                          \
   (abs_node)(addr_node)(not_node)(fptoi_node)(itofp_node)(unaligned_mem_access_node)(neg_node)(nop_node)( \
       bitcast_node)(mem_access_node)

#define BINARY_EXPRESSION_IR_NODES                                                                                     \
   (and_node)(or_node)(xor_node)(eq_node)(ge_node)(gt_node)(le_node)(shl_node)(lt_node)(max_node)(min_node)(sub_node)( \
       mul_node)(ne_node)(add_node)(gep_node)(fdiv_node)(shr_node)(idiv_node)(irem_node)(widen_mul_node)(              \
       extract_bit_node)(add_sat_node)(sub_sat_node)(extractvalue_node)(extractelement_node)(frem_node)

#define TERNARY_EXPRESSION_IR_NODES                                                                        \
   (concat_bit_node)(select_node)(shufflevector_node)(ternary_add_node)(ternary_as_node)(ternary_sa_node)( \
       ternary_ss_node)(fshl_node)(fshr_node)(insertvalue_node)(insertelement_node)

#define MISCELLANEOUS_EXPR_IR_NODES (call_node)(lut_node)

#define NODE_STMTS (assign_stmt)(call_stmt)(nop_stmt)(phi_stmt)(return_stmt)(multi_way_if_stmt)

#define MISCELLANEOUS_OBJ_IR_NODES (constructor_node)(identifier_node)(ssa_node)(statement_list_node)(ir_reindex)

#define TYPE_NODE_IR_NODES                                                                                            \
   (array_ty_node)(function_ty_node)(integer_ty_node)(pointer_ty_node)(real_ty_node)(struct_ty_node)(vector_ty_node)( \
       void_ty_node)

#define CONST_OBJ_IR_NODES (constant_int_val_node)(constant_fp_val_node)(constant_vector_val_node)

#define DECL_NODE_IR_NODES (field_val_node)(function_val_node)(argument_val_node)(module_unit_node)(variable_val_node)

#define CPP_NODES

/// basic block ir_node
#define BASIC_BLOCK_IR_NODES (bloc)

enum kind : int
{
   BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, BINARY_EXPRESSION_IR_NODES)
       BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY,
                             CONST_OBJ_IR_NODES) BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, DECL_NODE_IR_NODES)
           BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, NODE_STMTS)
               BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_IR_NODES)
                   BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_IR_NODES)
                       BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, TERNARY_EXPRESSION_IR_NODES)
                           BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, TYPE_NODE_IR_NODES)
                               BOOST_PP_SEQ_FOR_EACH(IR_NODE_KIND, BOOST_PP_EMPTY, UNARY_EXPRESSION_IR_NODES(last_ir))
};

/**
 * Macro which define a function that return the parameter as a enum kind.
 */
#define GET_KIND(meth)                 \
   enum kind get_kind() const override \
   {                                   \
      return (meth##_K);               \
   }

#endif
