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
 * @file ir_node_mask.cpp
 * @brief IR node mask. This class factorize the mask initialization common to all visitor classes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_node_mask.hpp"

#include "exceptions.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"

#include <string>

#define CREATE_IR_NODE_CASE_BODY(ir_node_name, node_id)

void ir_node_mask::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node yet supported: " + std::string(obj->get_kind_text()));
}

void ir_node_mask::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node yet supported: " + std::string(obj->get_kind_text()));
}

void ir_node_mask::operator()(const IR_LocInfo*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const decl_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, decl_node::IR_LocInfo);
}

void ir_node_mask::operator()(const expr_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, expr_node::IR_LocInfo);
}

void ir_node_mask::operator()(const node_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, node_stmt::IR_LocInfo);
}

void ir_node_mask::operator()(const unary_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, unary_node::expr_node);
}

void ir_node_mask::operator()(const binary_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, binary_node::expr_node);
}

void ir_node_mask::operator()(const ternary_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, ternary_node::expr_node);
}

void ir_node_mask::operator()(const type_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const cst_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const array_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, array_ty_node::type_node);
}

void ir_node_mask::operator()(const call_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, call_node::expr_node);
}

void ir_node_mask::operator()(const call_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, call_stmt::node_stmt);
}

void ir_node_mask::operator()(const constructor_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const field_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, field_val_node::decl_node);
}

void ir_node_mask::operator()(const function_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, function_val_node::decl_node);
}

void ir_node_mask::operator()(const function_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, function_ty_node::type_node);
}

void ir_node_mask::operator()(const assign_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, assign_stmt::node_stmt);
}

void ir_node_mask::operator()(const identifier_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const constant_int_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, constant_int_val_node::cst_node);
}

void ir_node_mask::operator()(const integer_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, integer_ty_node::type_node);
}

void ir_node_mask::operator()(const argument_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, argument_val_node::decl_node);
}

void ir_node_mask::operator()(const phi_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, phi_stmt::node_stmt);
}

void ir_node_mask::operator()(const pointer_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, pointer_ty_node::type_node);
}

void ir_node_mask::operator()(const constant_fp_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, constant_fp_val_node::cst_node);
}

void ir_node_mask::operator()(const real_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, real_ty_node::type_node);
}

void ir_node_mask::operator()(const struct_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, struct_ty_node::type_node);
}

void ir_node_mask::operator()(const return_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, return_stmt::node_stmt);
}

void ir_node_mask::operator()(const ssa_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const statement_list_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const lut_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, lut_node::expr_node);
}

void ir_node_mask::operator()(const variable_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, variable_val_node::decl_node);
}

void ir_node_mask::operator()(const constant_vector_val_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, constant_vector_val_node::cst_node);
}

void ir_node_mask::operator()(const vector_ty_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, vector_ty_node::type_node);
}

void ir_node_mask::operator()(const bloc*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_mask::operator()(const multi_way_if_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, multi_way_if_stmt::node_stmt);
}
