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
 * @file ir_nodes_merger.cpp
 * @brief IR node merger classes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_nodes_merger.hpp"

#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "token_interface.hpp"

#include <string>
#include <utility>
#include <vector>

#define CHECK_AND_ADD(ir_node_ref, visit_index)                            \
   {                                                                       \
      if((ir_node_ref) && remap.find((ir_node_ref)->index) == remap.end()) \
      {                                                                    \
         SET_VISIT_INDEX(mask, visit_index);                               \
         unsigned int node_id = (ir_node_ref)->index;                      \
         remap[node_id] = TM->new_ir_node_id();                            \
         not_yet_remapped.insert(node_id);                                 \
      }                                                                    \
   }

void ir_node_reached::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + obj->get_kind_text());
}

void ir_node_reached::operator()(const ir_reindex*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, ir_reindex::actual_ir_node);
}

void ir_node_reached::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const decl_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->name, decl_node::name);
   CHECK_AND_ADD(obj->mngl, decl_node::mngl);
   CHECK_AND_ADD(obj->type, decl_node::type);
   CHECK_AND_ADD(obj->parent, decl_node::parent);
}

void ir_node_reached::operator()(const expr_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->type, expr_node::type);
}

void ir_node_reached::operator()(const node_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->memuse, node_stmt::memuse);
   CHECK_AND_ADD(obj->memdef, node_stmt::memdef);
   for(const auto& vuse : obj->vuses)
   {
      CHECK_AND_ADD(vuse, node_stmt::vuses);
   }
   CHECK_AND_ADD(obj->vdef, node_stmt::vdef);
   for(const auto& vover : obj->vovers)
   {
      CHECK_AND_ADD(vover, node_stmt::vovers);
   }
   CHECK_AND_ADD(obj->predicate, node_stmt::predicate);
}

void ir_node_reached::operator()(const unary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op, unary_node::op);
}

void ir_node_reached::operator()(const binary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op0, binary_node::op0);
   CHECK_AND_ADD(obj->op1, binary_node::op1);
}

void ir_node_reached::operator()(const ternary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op0, ternary_node::op0);
   CHECK_AND_ADD(obj->op1, ternary_node::op1);
   CHECK_AND_ADD(obj->op2, ternary_node::op2);
}

void ir_node_reached::operator()(const type_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const cst_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->type, cst_node::type);
}

void ir_node_reached::operator()(const array_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->elts, array_ty_node::elts);
}

void ir_node_reached::operator()(const call_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->fn, call_node::fn);
   std::vector<ir_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
      CHECK_AND_ADD(*arg, call_node::args);
}

void ir_node_reached::operator()(const call_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->fn, call_stmt::fn);
   std::vector<ir_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
      CHECK_AND_ADD(*arg, call_stmt::args);
}

void ir_node_reached::operator()(const constructor_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->type, constructor_node::type);
   auto vend = obj->list_of_idx_valu.end();
   for(auto i = obj->list_of_idx_valu.begin(); i != vend; ++i)
   {
      CHECK_AND_ADD(i->first, constructor_node::list_of_idx_valu);
      CHECK_AND_ADD(i->second, constructor_node::list_of_idx_valu);
   }
}

void ir_node_reached::operator()(const field_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const function_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   CHECK_AND_ADD(obj->fn, function_val_node::fn);
   auto vend2 = obj->list_of_args.end();
   for(auto i = obj->list_of_args.begin(); i != vend2; ++i)
      CHECK_AND_ADD(*i, function_val_node::list_of_args);

   CHECK_AND_ADD(obj->body, function_val_node::body);
}

void ir_node_reached::operator()(const function_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->retn, function_ty_node::retn);
   auto vend2 = obj->list_of_args_type.end();
   for(auto i = obj->list_of_args_type.begin(); i != vend2; ++i)
   {
      CHECK_AND_ADD(*i, function_ty_node::list_of_args_type);
   }
}

void ir_node_reached::operator()(const assign_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op0, assign_stmt::op0);
   CHECK_AND_ADD(obj->op1, assign_stmt::op1);
}

void ir_node_reached::operator()(const identifier_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void ir_node_reached::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const argument_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const phi_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   CHECK_AND_ADD(obj->res, phi_stmt::res);
   for(const auto& def_edge : obj->CGetDefEdgesList())
      CHECK_AND_ADD(def_edge.first, phi_stmt::list_of_def_edge);
}

void ir_node_reached::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->ptd, pointer_ty_node::ptd);
}

void ir_node_reached::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const real_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
}

void ir_node_reached::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->name, struct_ty_node::name);

   auto vend1 = obj->list_of_flds.end();
   for(auto i = obj->list_of_flds.begin(); i != vend1; ++i)
      CHECK_AND_ADD(*i, struct_ty_node::list_of_flds);
}

void ir_node_reached::operator()(const return_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op, return_stmt::op);
}

void ir_node_reached::operator()(const ssa_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   CHECK_AND_ADD(obj->type, ssa_node::type);
   CHECK_AND_ADD(obj->var, ssa_node::var);
   for(const auto& use : obj->use_set.variables)
   {
      CHECK_AND_ADD(use, ssa_node::use_set);
   }

   auto const& def_stmt = obj->GetDefStmt();
   CHECK_AND_ADD(def_stmt, ssa_node::def_stmt);

   CHECK_AND_ADD(obj->min, ssa_node::min);
   CHECK_AND_ADD(obj->max, ssa_node::max);
}

void ir_node_reached::operator()(const statement_list_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   SET_VISIT_INDEX(mask, statement_list_node::list_of_bloc);
}

void ir_node_reached::operator()(const lut_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->op0, lut_node::op0);
   CHECK_AND_ADD(obj->op1, lut_node::op1);
   CHECK_AND_ADD(obj->op2, lut_node::op2);
   CHECK_AND_ADD(obj->op3, lut_node::op3);
   CHECK_AND_ADD(obj->op4, lut_node::op4);
   CHECK_AND_ADD(obj->op5, lut_node::op5);
   CHECK_AND_ADD(obj->op6, lut_node::op6);
   CHECK_AND_ADD(obj->op7, lut_node::op7);
   CHECK_AND_ADD(obj->op8, lut_node::op8);
}

void ir_node_reached::operator()(const variable_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   CHECK_AND_ADD(obj->init, variable_val_node::init);
}

void ir_node_reached::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   auto vend = obj->list_of_valu.end();
   for(auto i = obj->list_of_valu.begin(); i != vend; ++i)
      CHECK_AND_ADD(*i, constant_vector_val_node::list_of_valu);
}

void ir_node_reached::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   CHECK_AND_ADD(obj->elts, vector_ty_node::elts);
}

void ir_node_reached::operator()(const bloc* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   for(const auto& phi : obj->CGetPhiList())
      CHECK_AND_ADD(phi, bloc::list_of_phi);
   for(const auto& stmt : obj->CGetStmtList())
      CHECK_AND_ADD(stmt, bloc::list_of_stmt);
}

void ir_node_reached::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   for(const auto& cond : obj->list_of_cond)
      CHECK_AND_ADD(cond.first, multi_way_if_stmt::list_of_cond);
}

#define CREATE_IR_NODE_CASE_BODY(ir_node_name, node_id) \
   {                                                    \
      auto tnn = new ir_node_name(node_id);             \
      ir_nodeRef cur = ir_nodeRef(tnn);                 \
      TM->AddIRNode(cur);                               \
      curr_ir_node_ptr = tnn;                           \
      source_tn = tn;                                   \
      tnn->visit(this);                                 \
      curr_ir_node_ptr = nullptr;                       \
      source_tn = ir_nodeRef();                         \
      break;                                            \
   }

void ir_node_index_factory::create_ir_node(const unsigned int node_id, const ir_nodeRef& tn)
{
   switch(tn->get_kind())
   {
      case abs_node_K:
         CREATE_IR_NODE_CASE_BODY(abs_node, node_id)
      case addr_node_K:
         CREATE_IR_NODE_CASE_BODY(addr_node, node_id)
      case array_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(array_ty_node, node_id)
      case and_node_K:
         CREATE_IR_NODE_CASE_BODY(and_node, node_id)
      case or_node_K:
         CREATE_IR_NODE_CASE_BODY(or_node, node_id)
      case concat_bit_node_K:
         CREATE_IR_NODE_CASE_BODY(concat_bit_node, node_id)
      case not_node_K:
         CREATE_IR_NODE_CASE_BODY(not_node, node_id)
      case xor_node_K:
         CREATE_IR_NODE_CASE_BODY(xor_node, node_id)
      case call_node_K:
         CREATE_IR_NODE_CASE_BODY(call_node, node_id)
      case call_stmt_K:
         CREATE_IR_NODE_CASE_BODY(call_stmt, node_id)
      case select_node_K:
         CREATE_IR_NODE_CASE_BODY(select_node, node_id)
      case constructor_node_K:
         CREATE_IR_NODE_CASE_BODY(constructor_node, node_id)
      case eq_node_K:
         CREATE_IR_NODE_CASE_BODY(eq_node, node_id)
      case field_val_node_K:
         CREATE_IR_NODE_CASE_BODY(field_val_node, node_id)
      case fptoi_node_K:
         CREATE_IR_NODE_CASE_BODY(fptoi_node, node_id)
      case itofp_node_K:
         CREATE_IR_NODE_CASE_BODY(itofp_node, node_id)
      case function_val_node_K:
         CREATE_IR_NODE_CASE_BODY(function_val_node, node_id)
      case function_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(function_ty_node, node_id)
      case ge_node_K:
         CREATE_IR_NODE_CASE_BODY(ge_node, node_id)
      case assign_stmt_K:
         CREATE_IR_NODE_CASE_BODY(assign_stmt, node_id)
      case gt_node_K:
         CREATE_IR_NODE_CASE_BODY(gt_node, node_id)
      case unaligned_mem_access_node_K:
         CREATE_IR_NODE_CASE_BODY(unaligned_mem_access_node, node_id)
      case constant_int_val_node_K:
         CREATE_IR_NODE_CASE_BODY(constant_int_val_node, node_id)
      case integer_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(integer_ty_node, node_id)
      case le_node_K:
         CREATE_IR_NODE_CASE_BODY(le_node, node_id)
      case lut_node_K:
         CREATE_IR_NODE_CASE_BODY(lut_node, node_id)
      case shl_node_K:
         CREATE_IR_NODE_CASE_BODY(shl_node, node_id)
      case lt_node_K:
         CREATE_IR_NODE_CASE_BODY(lt_node, node_id)
      case max_node_K:
         CREATE_IR_NODE_CASE_BODY(max_node, node_id)
      case min_node_K:
         CREATE_IR_NODE_CASE_BODY(min_node, node_id)
      case sub_node_K:
         CREATE_IR_NODE_CASE_BODY(sub_node, node_id)
      case mul_node_K:
         CREATE_IR_NODE_CASE_BODY(mul_node, node_id)
      case ne_node_K:
         CREATE_IR_NODE_CASE_BODY(ne_node, node_id)
      case neg_node_K:
         CREATE_IR_NODE_CASE_BODY(neg_node, node_id)
      case nop_node_K:
         CREATE_IR_NODE_CASE_BODY(nop_node, node_id)
      case argument_val_node_K:
         CREATE_IR_NODE_CASE_BODY(argument_val_node, node_id)
      case phi_stmt_K:
         CREATE_IR_NODE_CASE_BODY(phi_stmt, node_id)
      case add_node_K:
         CREATE_IR_NODE_CASE_BODY(add_node, node_id)
      case gep_node_K:
         CREATE_IR_NODE_CASE_BODY(gep_node, node_id)
      case pointer_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(pointer_ty_node, node_id)
      case fdiv_node_K:
         CREATE_IR_NODE_CASE_BODY(fdiv_node, node_id)
      case constant_fp_val_node_K:
         CREATE_IR_NODE_CASE_BODY(constant_fp_val_node, node_id)
      case real_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(real_ty_node, node_id)
      case struct_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(struct_ty_node, node_id)
      case return_stmt_K:
         CREATE_IR_NODE_CASE_BODY(return_stmt, node_id)
      case shr_node_K:
         CREATE_IR_NODE_CASE_BODY(shr_node, node_id)
      case ssa_node_K:
         CREATE_IR_NODE_CASE_BODY(ssa_node, node_id)
      case statement_list_node_K:
         CREATE_IR_NODE_CASE_BODY(statement_list_node, node_id)
      case mem_access_node_K:
         CREATE_IR_NODE_CASE_BODY(mem_access_node, node_id)
      case ternary_add_node_K:
         CREATE_IR_NODE_CASE_BODY(ternary_add_node, node_id)
      case ternary_as_node_K:
         CREATE_IR_NODE_CASE_BODY(ternary_as_node, node_id)
      case ternary_sa_node_K:
         CREATE_IR_NODE_CASE_BODY(ternary_sa_node, node_id)
      case ternary_ss_node_K:
         CREATE_IR_NODE_CASE_BODY(ternary_ss_node, node_id)
      case module_unit_node_K:
         CREATE_IR_NODE_CASE_BODY(module_unit_node, node_id)
      case idiv_node_K:
         CREATE_IR_NODE_CASE_BODY(idiv_node, node_id)
      case irem_node_K:
         CREATE_IR_NODE_CASE_BODY(irem_node, node_id)
      case variable_val_node_K:
         CREATE_IR_NODE_CASE_BODY(variable_val_node, node_id)
      case shufflevector_node_K:
         CREATE_IR_NODE_CASE_BODY(shufflevector_node, node_id)
      case constant_vector_val_node_K:
         CREATE_IR_NODE_CASE_BODY(constant_vector_val_node, node_id)
      case vector_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(vector_ty_node, node_id)
      case bitcast_node_K:
         CREATE_IR_NODE_CASE_BODY(bitcast_node, node_id)
      case void_ty_node_K:
         CREATE_IR_NODE_CASE_BODY(void_ty_node, node_id)
      case nop_stmt_K:
         CREATE_IR_NODE_CASE_BODY(nop_stmt, node_id)
      case identifier_node_K: /// special care is reserved for identifier_nodes
      {
         ir_nodeRef cur;
         cur = ir_nodeRef(new identifier_node(node_id, GetPointerS<identifier_node>(tn)->strg, TM.get()));
         TM->AddIRNode(cur);
         break;
      }
      case widen_mul_node_K:
         CREATE_IR_NODE_CASE_BODY(widen_mul_node, node_id)
      case multi_way_if_stmt_K:
         CREATE_IR_NODE_CASE_BODY(multi_way_if_stmt, node_id)
      case extract_bit_node_K:
         CREATE_IR_NODE_CASE_BODY(extract_bit_node, node_id)
      case add_sat_node_K:
         CREATE_IR_NODE_CASE_BODY(add_sat_node, node_id)
      case sub_sat_node_K:
         CREATE_IR_NODE_CASE_BODY(sub_sat_node, node_id)
      case fshl_node_K:
         CREATE_IR_NODE_CASE_BODY(fshl_node, node_id)
      case fshr_node_K:
         CREATE_IR_NODE_CASE_BODY(fshr_node, node_id)
      case extractvalue_node_K:
         CREATE_IR_NODE_CASE_BODY(extractvalue_node, node_id)
      case insertvalue_node_K:
         CREATE_IR_NODE_CASE_BODY(insertvalue_node, node_id)
      case extractelement_node_K:
         CREATE_IR_NODE_CASE_BODY(extractelement_node, node_id)
      case insertelement_node_K:
         CREATE_IR_NODE_CASE_BODY(insertelement_node, node_id)
      case frem_node_K:
         CREATE_IR_NODE_CASE_BODY(frem_node, node_id)
      case CASE_FAKE_NODES:
      default:
         THROW_ERROR("ir_node_type node not yet supported. node type is " + std::string(tn->get_kind_text()));
   }
}

void ir_node_index_factory::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

void ir_node_index_factory::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

#define SET_NODE_ID(field, type)                                                                        \
   if(GetPointerS<type>(source_tn)->field)                                                              \
   {                                                                                                    \
      unsigned int node_id = GetPointerS<type>(source_tn)->field->index;                                \
      THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index: " + std::to_string(node_id)); \
      node_id = remap.find(node_id)->second;                                                            \
      static_cast<type*>(curr_ir_node_ptr)->field = TM->GetIRReindex(node_id);                          \
   }

#define SEQ_SET_NODE_ID(list_field, type)                                                                  \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                                   \
   {                                                                                                       \
      for(const auto& i : GetPointerS<type>(source_tn)->list_field)                                        \
      {                                                                                                    \
         unsigned int node_id = i->index;                                                                  \
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index: " + std::to_string(node_id)); \
         node_id = remap.find(node_id)->second;                                                            \
         static_cast<type*>(curr_ir_node_ptr)->list_field.push_back(TM->GetIRReindex(node_id));            \
      }                                                                                                    \
   }

#define SET_SET_NODE_ID(list_field, type)                                                                  \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                                   \
   {                                                                                                       \
      for(const auto& i : GetPointerS<type>(source_tn)->list_field)                                        \
      {                                                                                                    \
         unsigned int node_id = i->index;                                                                  \
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index: " + std::to_string(node_id)); \
         node_id = remap.find(node_id)->second;                                                            \
         static_cast<type*>(curr_ir_node_ptr)->list_field.insert(TM->GetIRReindex(node_id));               \
      }                                                                                                    \
   }

#define LSEQ_SET_NODE_ID(list_field, type)                                                                            \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                                              \
   {                                                                                                                  \
      std::list<ir_nodeRef>::const_iterator vend = GetPointerS<type>(source_tn)->list_field.end();                    \
      for(std::list<ir_nodeRef>::const_iterator i = GetPointerS<type>(source_tn)->list_field.begin(); i != vend; ++i) \
      {                                                                                                               \
         unsigned int node_id = (*i)->index;                                                                          \
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index: " + std::to_string(node_id));            \
         node_id = remap.find(node_id)->second;                                                                       \
         static_cast<type*>(curr_ir_node_ptr)->list_field.push_back(TM->GetIRReindex(node_id));                       \
      }                                                                                                               \
   }

#define SET_VALUE(field, type) (static_cast<type*>(curr_ir_node_ptr)->field = GetPointerS<type>(source_tn)->field)

void ir_node_index_factory::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   const_cast<IR_LocInfo*>(obj)->include_name = GetPointer<IR_LocInfo>(source_tn)->include_name;
   const_cast<IR_LocInfo*>(obj)->line_number = GetPointer<IR_LocInfo>(source_tn)->line_number;
   const_cast<IR_LocInfo*>(obj)->column_number = GetPointer<IR_LocInfo>(source_tn)->column_number;
}

void ir_node_index_factory::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(name, decl_node);
   SET_NODE_ID(mngl, decl_node);
   SET_NODE_ID(type, decl_node);
   SET_NODE_ID(parent, decl_node);
   SET_VALUE(operating_system_flag, decl_node);
   SET_VALUE(library_system_flag, decl_node);
}

void ir_node_index_factory::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, expr_node);
}

void ir_node_index_factory::operator()(const node_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(memuse, node_stmt);
   SET_NODE_ID(memdef, node_stmt);
   SET_SET_NODE_ID(vuses, node_stmt);
   SET_NODE_ID(vdef, node_stmt);
   SET_SET_NODE_ID(vovers, node_stmt);
   SET_NODE_ID(parent, node_stmt);
   SET_NODE_ID(predicate, node_stmt);
   SET_VALUE(bb_index, node_stmt);
}

void ir_node_index_factory::operator()(const unary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, unary_node);
}

void ir_node_index_factory::operator()(const binary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, binary_node);
   SET_NODE_ID(op1, binary_node);
}

void ir_node_index_factory::operator()(const ternary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, ternary_node);
   SET_NODE_ID(op1, ternary_node);
   SET_NODE_ID(op2, ternary_node);
}

void ir_node_index_factory::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsizealloc, type_node);
   SET_VALUE(system_flag, type_node);
   SET_VALUE(algn, type_node);
}

void ir_node_index_factory::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, cst_node);
}

void ir_node_index_factory::operator()(const array_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, array_ty_node);
   SET_VALUE(nelements, array_ty_node);
}

void ir_node_index_factory::operator()(const call_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, call_node);
   SEQ_SET_NODE_ID(args, call_node);
}

void ir_node_index_factory::operator()(const call_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, call_stmt);
   SEQ_SET_NODE_ID(args, call_stmt);
}

void ir_node_index_factory::operator()(const constructor_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, constructor_node);
   if(!GetPointerS<constructor_node>(source_tn)->list_of_idx_valu.empty())
   {
      auto vend = GetPointerS<constructor_node>(source_tn)->list_of_idx_valu.end();
      for(auto i = GetPointerS<constructor_node>(source_tn)->list_of_idx_valu.begin(); i != vend; ++i)
      {
         unsigned int node_id1 = i->first ? i->first->index : 0;
         unsigned int node_id2 = i->second->index;
         THROW_ASSERT(!node_id1 || remap.find(node_id1) != remap.end(), "missing an index");
         node_id1 = node_id1 ? remap.find(node_id1)->second : 0;
         THROW_ASSERT(remap.find(node_id2) != remap.end(), "missing an index");
         node_id2 = remap.find(node_id2)->second;
         if(node_id1)
         {
            static_cast<constructor_node*>(curr_ir_node_ptr)
                ->add_idx_valu(TM->GetIRReindex(node_id1), TM->GetIRReindex(node_id2));
         }
         else
         {
            static_cast<constructor_node*>(curr_ir_node_ptr)->add_valu(TM->GetIRReindex(node_id2));
         }
      }
   }
}

void ir_node_index_factory::operator()(const field_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(algn, field_val_node);
   SET_VALUE(bitsizealloc, field_val_node);
   SET_VALUE(offset, field_val_node);
   SET_VALUE(packed_flag, field_val_node);
}

void ir_node_index_factory::operator()(const function_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(fn, function_val_node);
   SEQ_SET_NODE_ID(list_of_args, function_val_node);
   SET_VALUE(builtin_flag, function_val_node);
   SET_VALUE(static_flag, function_val_node);
   SET_VALUE(writing_memory, function_val_node);
   SET_VALUE(reading_memory, function_val_node);
   SET_VALUE(pipeline_enabled, function_val_node);
   SET_VALUE(initiation_time, function_val_node);
   SET_NODE_ID(body, function_val_node);
}

void ir_node_index_factory::operator()(const function_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(retn, function_ty_node);
   SEQ_SET_NODE_ID(list_of_args_type, function_ty_node);
   SET_VALUE(varargs_flag, function_ty_node);
}

void ir_node_index_factory::operator()(const assign_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, assign_stmt);
   SET_NODE_ID(op1, assign_stmt);
   SET_VALUE(temporary_address, assign_stmt);
}

void ir_node_index_factory::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void ir_node_index_factory::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(value, constant_int_val_node);
}

void ir_node_index_factory::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsize, integer_ty_node);
   SET_VALUE(unsigned_flag, integer_ty_node);
}

void ir_node_index_factory::operator()(const argument_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsizealloc, argument_val_node);
   SET_VALUE(algn, argument_val_node);
   SET_VALUE(readonly_flag, argument_val_node);
}

void ir_node_index_factory::operator()(const phi_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(res, phi_stmt);
   for(const auto& def_edge : GetPointerS<phi_stmt>(source_tn)->CGetDefEdgesList())
   {
      unsigned int node_id = def_edge.first->index;
      THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
      node_id = remap.find(node_id)->second;
      static_cast<phi_stmt*>(curr_ir_node_ptr)
          ->AddDefEdge(TM, phi_stmt::DefEdge(TM->GetIRReindex(node_id), def_edge.second));
   }
   SET_VALUE(virtual_flag, phi_stmt);
}

void ir_node_index_factory::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(ptd, pointer_ty_node);
}

void ir_node_index_factory::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(overflow_flag, constant_fp_val_node);
   SET_VALUE(valr, constant_fp_val_node);
   SET_VALUE(valx, constant_fp_val_node);
}

void ir_node_index_factory::operator()(const real_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsize, real_ty_node);
}

void ir_node_index_factory::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(name, struct_ty_node);
   SET_VALUE(packed_flag, struct_ty_node);

   SEQ_SET_NODE_ID(list_of_flds, struct_ty_node);
}

void ir_node_index_factory::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, return_stmt);
}

void ir_node_index_factory::operator()(const ssa_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(type, ssa_node);
   SET_NODE_ID(var, ssa_node);
   SET_VALUE(vers, ssa_node);
   SET_VALUE(use_set.anything, ssa_node);
   SET_VALUE(use_set.escaped, ssa_node);
   SET_VALUE(use_set.ipa_escaped, ssa_node);
   SET_VALUE(use_set.nonlocal, ssa_node);
   SET_VALUE(use_set.null, ssa_node);
   SEQ_SET_NODE_ID(use_set.variables, ssa_node);

   SET_VALUE(virtual_flag, ssa_node);
   SET_VALUE(default_flag, ssa_node);
   const auto& def_stmt = GetPointerS<const ssa_node>(source_tn)->GetDefStmt();
   unsigned int node_id0 = def_stmt->index;
   THROW_ASSERT(remap.find(node_id0) != remap.end(), "missing an index: " + std::to_string(node_id0));
   node_id0 = remap.find(node_id0)->second;
   static_cast<ssa_node*>(curr_ir_node_ptr)->SetDefStmt(TM->GetIRReindex(node_id0));

   SET_NODE_ID(min, ssa_node);
   SET_NODE_ID(max, ssa_node);
   SET_VALUE(bit_values, ssa_node);
}

void ir_node_index_factory::operator()(const statement_list_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   auto mend = GetPointerS<statement_list_node>(source_tn)->list_of_bloc.end();
   for(auto i = GetPointerS<statement_list_node>(source_tn)->list_of_bloc.begin(); i != mend; ++i)
   {
      curr_bloc = new bloc(i->first);
      source_bloc = i->second;
      curr_bloc->visit(this);
      static_cast<statement_list_node*>(curr_ir_node_ptr)->add_bloc(blocRef(curr_bloc));
      curr_bloc = nullptr;
      source_bloc = blocRef();
   }
}

void ir_node_index_factory::operator()(const lut_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, lut_node);
   SET_NODE_ID(op1, lut_node);
   SET_NODE_ID(op2, lut_node);
   SET_NODE_ID(op3, lut_node);
   SET_NODE_ID(op4, lut_node);
   SET_NODE_ID(op5, lut_node);
   SET_NODE_ID(op6, lut_node);
   SET_NODE_ID(op7, lut_node);
   SET_NODE_ID(op8, lut_node);
}

void ir_node_index_factory::operator()(const variable_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_VALUE(extern_flag, variable_val_node);
   SET_VALUE(addr_not_taken, variable_val_node);
   SET_VALUE(static_flag, variable_val_node);
   SET_NODE_ID(init, variable_val_node);
   SET_VALUE(bitsizealloc, variable_val_node);
   SET_VALUE(algn, variable_val_node);
   SET_VALUE(readonly_flag, variable_val_node);
   SET_VALUE(bit_values, variable_val_node);
}

void ir_node_index_factory::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SEQ_SET_NODE_ID(list_of_valu, constant_vector_val_node);
}

void ir_node_index_factory::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, vector_ty_node);
}

void ir_node_index_factory::operator()(const bloc* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   curr_bloc->loop_id = source_bloc->loop_id;
   curr_bloc->list_of_pred = source_bloc->list_of_pred;
   curr_bloc->list_of_succ = source_bloc->list_of_succ;
   for(const auto& phi : source_bloc->CGetPhiList())
   {
      unsigned int node_id = phi->index;
      THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
      node_id = remap.find(node_id)->second;
      curr_bloc->AddPhi(TM->GetIRReindex(node_id));
   }
   for(const auto& stmt : source_bloc->CGetStmtList())
   {
      unsigned int node_id = stmt->index;
      THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
      node_id = remap.find(node_id)->second;
      curr_bloc->PushBack(TM->GetIRReindex(node_id), application_managerRef());
   }
}

void ir_node_index_factory::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   if(!GetPointerS<multi_way_if_stmt>(source_tn)->list_of_cond.empty())
   {
      for(const auto& cond : GetPointerS<multi_way_if_stmt>(source_tn)->list_of_cond)
      {
         if(cond.first)
         {
            unsigned int node_id = cond.first->index;
            THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
            node_id = remap.find(node_id)->second;
            static_cast<multi_way_if_stmt*>(curr_ir_node_ptr)->add_cond(TM->GetIRReindex(node_id), cond.second);
         }
         else
         {
            static_cast<multi_way_if_stmt*>(curr_ir_node_ptr)->add_cond(ir_nodeRef(), cond.second);
         }
      }
   }
}
