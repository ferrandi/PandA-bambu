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
 *              Copyright (C) 2024-2026 Politecnico di Milano
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
 * @file ir_reindex_remove.cpp
 * @brief IR reindex remove class
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "ir_reindex_remove.hpp"

#include "application_manager.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "string_manipulation.hpp"

#include <string>
#include <utility>
#include <vector>

ir_reindex_remove::ir_reindex_remove(const ir_manager& _TM) : TM(_TM), source_tn(nullptr)
{
}

void ir_reindex_remove::operator()(const ir_nodeRef& tn)
{
   source_tn = tn;
   tn->visit(this);
}

void ir_reindex_remove::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + obj->get_kind_text());
}

void ir_reindex_remove::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + obj->get_kind_text());
}

void ir_reindex_remove::fix_reference(ir_nodeRef& tn) const
{
   if(tn)
   {
      tn = TM.GetIRNode(tn->index);
   }
}

#define node_fix_reference(field, type) fix_reference(GetPointerS<type>(source_tn)->field)

#define seq_fix_reference(list_field, type)                       \
   if(!GetPointerS<type>(source_tn)->list_field.empty())          \
   {                                                              \
      for(auto& field : GetPointerS<type>(source_tn)->list_field) \
      {                                                           \
         fix_reference(field);                                    \
      }                                                           \
   }

#define set_fix_reference(set_field, type)                       \
   if(!GetPointerS<type>(source_tn)->set_field.empty())          \
   {                                                             \
      IRNodeSet fix_set;                                         \
      for(auto& field : GetPointerS<type>(source_tn)->set_field) \
      {                                                          \
         ir_nodeRef tn = field;                                  \
         fix_reference(tn);                                      \
         fix_set.insert(tn);                                     \
      }                                                          \
      GetPointerS<type>(source_tn)->set_field = fix_set;         \
   }

void ir_reindex_remove::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   node_fix_reference(name, decl_node);
   node_fix_reference(mngl, decl_node);
   node_fix_reference(type, decl_node);
   node_fix_reference(parent, decl_node);
}

void ir_reindex_remove::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(type, expr_node);
}

void ir_reindex_remove::operator()(const node_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(memuse, node_stmt);
   node_fix_reference(memdef, node_stmt);
   set_fix_reference(vuses, node_stmt);
   node_fix_reference(vdef, node_stmt);
   set_fix_reference(vovers, node_stmt);
   node_fix_reference(parent, node_stmt);
}

void ir_reindex_remove::operator()(const unary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op, unary_node);
}

void ir_reindex_remove::operator()(const binary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op0, binary_node);
   node_fix_reference(op1, binary_node);
}

void ir_reindex_remove::operator()(const ternary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op0, ternary_node);
   node_fix_reference(op1, ternary_node);
   node_fix_reference(op2, ternary_node);
}

void ir_reindex_remove::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(type, cst_node);
}

void ir_reindex_remove::operator()(const array_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(elts, array_ty_node);
}

void ir_reindex_remove::operator()(const call_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(fn, call_node);
   seq_fix_reference(args, call_node);
}

void ir_reindex_remove::operator()(const call_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(fn, call_stmt);
   seq_fix_reference(args, call_stmt);
}

void ir_reindex_remove::operator()(const constructor_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(type, constructor_node);
   if(!GetPointerS<constructor_node>(source_tn)->list_of_idx_valu.empty())
   {
      for(auto& [idx, valu] : GetPointerS<constructor_node>(source_tn)->list_of_idx_valu)
      {
         fix_reference(idx);
         fix_reference(valu);
      }
   }
}

void ir_reindex_remove::operator()(const field_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const function_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   node_fix_reference(fn, function_val_node);
   seq_fix_reference(list_of_args, function_val_node);
   node_fix_reference(body, function_val_node);
}

void ir_reindex_remove::operator()(const function_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(retn, function_ty_node);
   seq_fix_reference(list_of_args_type, function_ty_node);
}

void ir_reindex_remove::operator()(const assign_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op0, assign_stmt);
   node_fix_reference(op1, assign_stmt);
   node_fix_reference(predicate, assign_stmt);
}

void ir_reindex_remove::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const argument_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(parent, argument_val_node);
}

void ir_reindex_remove::operator()(const phi_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   node_fix_reference(res, phi_stmt);
   if(!GetPointerS<phi_stmt>(source_tn)->list_of_def_edge.empty())
   {
      phi_stmt::DefEdgeList fix_set;
      for(auto& [def, edge] : GetPointerS<phi_stmt>(source_tn)->list_of_def_edge)
      {
         fix_reference(def);
      }
   }
}

void ir_reindex_remove::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(ptd, pointer_ty_node);
}

void ir_reindex_remove::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const real_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
}

void ir_reindex_remove::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(name, struct_ty_node);

   seq_fix_reference(list_of_flds, struct_ty_node);
}

void ir_reindex_remove::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op, return_stmt);
}

void ir_reindex_remove::operator()(const ssa_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   node_fix_reference(type, ssa_node);
   node_fix_reference(var, ssa_node);
   seq_fix_reference(use_set.variables, ssa_node);

   auto def = GetPointerS<ssa_node>(source_tn)->GetDefStmt();
   fix_reference(def);
   GetPointerS<ssa_node>(source_tn)->SetDefStmt(def);

   node_fix_reference(min, ssa_node);
   node_fix_reference(max, ssa_node);
}

void ir_reindex_remove::operator()(const statement_list_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   for(const auto& [bbi, bb] : GetPointerS<statement_list_node>(source_tn)->list_of_bloc)
   {
      for(auto& field : bb->list_of_phi)
      {
         fix_reference(field);
      }

      for(auto& field : bb->list_of_stmt)
      {
         fix_reference(field);
      }
   }
}

void ir_reindex_remove::operator()(const lut_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(op0, lut_node);
   node_fix_reference(op1, lut_node);
   node_fix_reference(op2, lut_node);
   node_fix_reference(op3, lut_node);
   node_fix_reference(op4, lut_node);
   node_fix_reference(op5, lut_node);
   node_fix_reference(op6, lut_node);
   node_fix_reference(op7, lut_node);
   node_fix_reference(op8, lut_node);
}

void ir_reindex_remove::operator()(const variable_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   node_fix_reference(init, variable_val_node);
}

void ir_reindex_remove::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   seq_fix_reference(list_of_valu, constant_vector_val_node);
}

void ir_reindex_remove::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   node_fix_reference(elts, vector_ty_node);
}

void ir_reindex_remove::operator()(const bloc*, unsigned int&)
{
   THROW_ERROR("bloc node not supported");
}

void ir_reindex_remove::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ASSERT(source_tn, "");
   for(auto& [cond, edge] : GetPointerS<multi_way_if_stmt>(source_tn)->list_of_cond)
   {
      if(cond)
      {
         fix_reference(cond);
      }
   }
}
