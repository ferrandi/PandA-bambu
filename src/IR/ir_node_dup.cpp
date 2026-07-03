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
 * @file ir_node_dup.cpp
 * @brief IR node duplication class.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "ir_node_dup.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <string>
#include <utility>
#include <vector>

#define DECLARATION (2) // All nodes including declarations are duplicated (not function_val_node)

#define CREATE_IR_NODE_CASE_BODY(ir_node_name, node_id) \
   {                                                    \
      (node_id) = TM->new_ir_node_id();                 \
      remap.insert({tn->index, (node_id)});             \
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

#define RET_IR_NODE_ID_CASE_BODY(ir_node_name, node_id) \
   {                                                    \
      if(remap.find(tn->index) != remap.end())          \
         (node_id) = remap.find(tn->index)->second;     \
      else                                              \
         (node_id) = tn->index;                         \
      break;                                            \
   }

ir_node_dup::ir_node_dup(CustomUnorderedMapStable<unsigned int, unsigned int>& _remap,
                         const application_managerRef _AppM, unsigned int _remap_bbi, unsigned int _remap_loop_id,
                         bool _use_counting)
    : AppM(_AppM),
      TM(_AppM->get_ir_manager()),
      use_counting(_use_counting),
      debug_level(_AppM->get_parameter()->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE)),
      remap(_remap),
      remap_bbi(_remap_bbi),
      remap_bb(),
      remap_loop_id(_remap_loop_id),
      remap_lid(),
      curr_ir_node_ptr(nullptr),
      curr_bloc(nullptr)
{
   if(remap_bbi)
   {
      remap_bb.insert(std::make_pair(bloc::ENTRY_BLOCK_ID, bloc::ENTRY_BLOCK_ID));
      remap_bb.insert(std::make_pair(bloc::EXIT_BLOCK_ID, bloc::EXIT_BLOCK_ID));
   }
}

#ifndef NDEBUG
static std::string print_node(const ir_nodeRef& tn)
{
   if(tn->get_kind() == function_val_node_K)
   {
      return "function_val_node @" + std::to_string(tn->index) + " " + ir_helper::GetFunctionName(tn);
   }
   if(tn->get_kind() == statement_list_node_K)
   {
      return "statement_list_node @" + std::to_string(tn->index);
   }
   return tn->get_kind_text() + " " + tn->ToString();
}
#endif

unsigned int ir_node_dup::create_ir_node(const ir_nodeRef& tn, int _mode)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Duplicate " + print_node(tn));
   unsigned int node_id = 0U;
   mode = _mode;
   switch(tn->get_kind())
   {
      case abs_node_K:
         CREATE_IR_NODE_CASE_BODY(abs_node, node_id)
      case addr_node_K:
         CREATE_IR_NODE_CASE_BODY(addr_node, node_id)
      case array_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(array_ty_node, node_id)
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
         if(mode)
            CREATE_IR_NODE_CASE_BODY(constructor_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(constructor_node, node_id)
      case eq_node_K:
         CREATE_IR_NODE_CASE_BODY(eq_node, node_id)
      case field_val_node_K:
         if(mode >= ir_node_dup_mode::FUNCTION)
            CREATE_IR_NODE_CASE_BODY(field_val_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(field_val_node, node_id)
      case fptoi_node_K:
         CREATE_IR_NODE_CASE_BODY(fptoi_node, node_id)
      case itofp_node_K:
         CREATE_IR_NODE_CASE_BODY(itofp_node, node_id)
      case function_val_node_K:
         if(mode >= ir_node_dup_mode::FUNCTION)
            CREATE_IR_NODE_CASE_BODY(function_val_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(function_val_node, node_id)
      case function_ty_node_K:
         if(mode >= ir_node_dup_mode::FUNCTION)
            CREATE_IR_NODE_CASE_BODY(function_ty_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(function_ty_node, node_id)
      case ge_node_K:
         CREATE_IR_NODE_CASE_BODY(ge_node, node_id)
      case assign_stmt_K:
         CREATE_IR_NODE_CASE_BODY(assign_stmt, node_id)
      case gt_node_K:
         CREATE_IR_NODE_CASE_BODY(gt_node, node_id)
      case unaligned_mem_access_node_K:
         CREATE_IR_NODE_CASE_BODY(unaligned_mem_access_node, node_id)
      case constant_int_val_node_K:
         RET_IR_NODE_ID_CASE_BODY(constant_int_val_node, node_id)
      case integer_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(integer_ty_node, node_id)
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
         if(mode >= DECLARATION)
            CREATE_IR_NODE_CASE_BODY(argument_val_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(argument_val_node, node_id)
      case phi_stmt_K:
         if(mode)
            CREATE_IR_NODE_CASE_BODY(phi_stmt, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(phi_stmt, node_id)
      case add_node_K:
         CREATE_IR_NODE_CASE_BODY(add_node, node_id)
      case gep_node_K:
         CREATE_IR_NODE_CASE_BODY(gep_node, node_id)
      case pointer_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(pointer_ty_node, node_id)
      case fdiv_node_K:
         CREATE_IR_NODE_CASE_BODY(fdiv_node, node_id)
      case constant_fp_val_node_K:
         RET_IR_NODE_ID_CASE_BODY(constant_fp_val_node, node_id)
      case real_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(real_ty_node, node_id)
      case struct_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(struct_ty_node, node_id)
      case return_stmt_K:
         CREATE_IR_NODE_CASE_BODY(return_stmt, node_id)
      case shr_node_K:
         CREATE_IR_NODE_CASE_BODY(shr_node, node_id)
      case ssa_node_K:
         if(mode)
            CREATE_IR_NODE_CASE_BODY(ssa_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(ssa_node, node_id)
      case statement_list_node_K:
         if(mode)
            CREATE_IR_NODE_CASE_BODY(statement_list_node, node_id)
         else
            RET_IR_NODE_ID_CASE_BODY(statement_list_node, node_id)
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
         RET_IR_NODE_ID_CASE_BODY(module_unit_node, node_id)
      case idiv_node_K:
         CREATE_IR_NODE_CASE_BODY(idiv_node, node_id)
      case irem_node_K:
         CREATE_IR_NODE_CASE_BODY(irem_node, node_id)
      case variable_val_node_K:
      {
         const auto vd = GetPointerS<const variable_val_node>(tn);
         if(mode >= DECLARATION && (!vd->parent || (vd->parent->get_kind() != module_unit_node_K)))
         {
            if(vd->static_flag)
            {
               if(remap.find(tn->index) != remap.end())
               {
                  node_id = remap.find(tn->index)->second;
               }
               else
               {
                  node_id = tn->index;
               }
               const auto new_tn = GetPointerS<variable_val_node>(TM->GetIRNode(node_id));
               new_tn->parent = GetPointerS<const decl_node>(new_tn->parent)->parent;
               break;
            }
            else
               CREATE_IR_NODE_CASE_BODY(variable_val_node, node_id)
         }
         else
            RET_IR_NODE_ID_CASE_BODY(variable_val_node, node_id)
      }
      case shufflevector_node_K:
         CREATE_IR_NODE_CASE_BODY(shufflevector_node, node_id)
      case constant_vector_val_node_K:
         RET_IR_NODE_ID_CASE_BODY(constant_vector_val_node, node_id)
      case vector_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(vector_ty_node, node_id)
      case bitcast_node_K:
         CREATE_IR_NODE_CASE_BODY(bitcast_node, node_id)
      case void_ty_node_K:
         RET_IR_NODE_ID_CASE_BODY(void_ty_node, node_id)
      case nop_stmt_K:
         CREATE_IR_NODE_CASE_BODY(nop_stmt, node_id)
      case identifier_node_K:
         RET_IR_NODE_ID_CASE_BODY(identifier_node, node_id)
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
      {
         THROW_UNREACHABLE(tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Duplicated as " + print_node(TM->GetIRNode(node_id)));
   return node_id;
}

unsigned int ir_node_dup::get_bbi(unsigned int old_bbi)
{
   if(remap_bbi)
   {
      const auto t = remap_bb.insert(std::make_pair(old_bbi, remap_bbi));
      return t.second ? remap_bbi++ : t.first->second;
   }
   return old_bbi;
}

unsigned int ir_node_dup::get_loop_id(unsigned int old_loop_id)
{
   if(remap_loop_id)
   {
      const auto t = remap_lid.insert(std::make_pair(old_loop_id, remap_loop_id));
      return t.second ? remap_loop_id++ : t.first->second;
   }
   return old_loop_id;
}

void ir_node_dup::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

void ir_node_dup::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

#define SET_NODE_ID(field, type)                                              \
   if(GetPointerS<type>(source_tn)->field)                                    \
   {                                                                          \
      unsigned int node_id = GetPointerS<type>(source_tn)->field->index;      \
      if(remap.find(node_id) != remap.end())                                  \
      {                                                                       \
         node_id = remap.find(node_id)->second;                               \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;                  \
         ir_nodeRef saved_source_tn = source_tn;                              \
         node_id = create_ir_node(GetPointerS<type>(source_tn)->field, mode); \
         curr_ir_node_ptr = saved_curr_ir_node_ptr;                           \
         source_tn = saved_source_tn;                                         \
      }                                                                       \
      static_cast<type*>(curr_ir_node_ptr)->field = TM->GetIRNode(node_id);   \
   }

#define SEQ_SET_NODE_ID(list_field, type)                                                    \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                     \
   {                                                                                         \
      for(auto const& field : GetPointerS<type>(source_tn)->list_field)                      \
      {                                                                                      \
         unsigned int node_id = field->index;                                                \
         if(remap.find(node_id) != remap.end())                                              \
            node_id = remap.find(node_id)->second;                                           \
         else                                                                                \
         {                                                                                   \
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;                              \
            ir_nodeRef saved_source_tn = source_tn;                                          \
            node_id = create_ir_node(field, mode);                                           \
            curr_ir_node_ptr = saved_curr_ir_node_ptr;                                       \
            source_tn = saved_source_tn;                                                     \
         }                                                                                   \
         static_cast<type*>(curr_ir_node_ptr)->list_field.push_back(TM->GetIRNode(node_id)); \
      }                                                                                      \
   }

#define SET_SET_NODE_ID(list_field, type)                                                 \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                  \
   {                                                                                      \
      for(const auto& i : GetPointerS<type>(source_tn)->list_field)                       \
      {                                                                                   \
         unsigned int node_id = i->index;                                                 \
         if(remap.find(node_id) != remap.end())                                           \
            node_id = remap.find(node_id)->second;                                        \
         else                                                                             \
         {                                                                                \
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;                           \
            ir_nodeRef saved_source_tn = source_tn;                                       \
            node_id = create_ir_node(i, mode);                                            \
            curr_ir_node_ptr = saved_curr_ir_node_ptr;                                    \
            source_tn = saved_source_tn;                                                  \
         }                                                                                \
         static_cast<type*>(curr_ir_node_ptr)->list_field.insert(TM->GetIRNode(node_id)); \
      }                                                                                   \
   }

#define LSEQ_SET_NODE_ID(list_field, type)                                                                            \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                                              \
   {                                                                                                                  \
      std::list<ir_nodeRef>::const_iterator vend = GetPointerS<type>(source_tn)->list_field.end();                    \
      for(std::list<ir_nodeRef>::const_iterator i = GetPointerS<type>(source_tn)->list_field.begin(); i != vend; ++i) \
      {                                                                                                               \
         unsigned int node_id = (*i)->index;                                                                          \
         if(remap.find(node_id) != remap.end())                                                                       \
            node_id = remap.find(node_id)->second;                                                                    \
         else                                                                                                         \
         {                                                                                                            \
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;                                                       \
            ir_nodeRef saved_source_tn = source_tn;                                                                   \
            node_id = create_ir_node(*i, mode);                                                                       \
            curr_ir_node_ptr = saved_curr_ir_node_ptr;                                                                \
            source_tn = saved_source_tn;                                                                              \
         }                                                                                                            \
         static_cast<type*>(curr_ir_node_ptr)->list_field.push_back(TM->GetIRNode(node_id));                          \
      }                                                                                                               \
   }

#define SET_VALUE(field, type) (static_cast<type*>(curr_ir_node_ptr)->field = GetPointerS<type>(source_tn)->field)

void ir_node_dup::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   // NOTE: const_cast below are "safe" since the following assert must be true
   THROW_ASSERT(obj == dynamic_cast<IR_LocInfo*>(curr_ir_node_ptr), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   const_cast<IR_LocInfo*>(obj)->include_name = GetPointer<IR_LocInfo>(source_tn)->include_name;
   const_cast<IR_LocInfo*>(obj)->line_number = GetPointer<IR_LocInfo>(source_tn)->line_number;
   const_cast<IR_LocInfo*>(obj)->column_number = GetPointer<IR_LocInfo>(source_tn)->column_number;
}

void ir_node_dup::operator()(const decl_node* obj, unsigned int& mask)
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

void ir_node_dup::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, expr_node);
}

void ir_node_dup::operator()(const node_stmt* obj, unsigned int& mask)
{
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

void ir_node_dup::operator()(const unary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, unary_node);
}

void ir_node_dup::operator()(const binary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, binary_node);
   SET_NODE_ID(op1, binary_node);
}

void ir_node_dup::operator()(const ternary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, ternary_node);
   SET_NODE_ID(op1, ternary_node);
   SET_NODE_ID(op2, ternary_node);
}

void ir_node_dup::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsizealloc, type_node);
   SET_VALUE(system_flag, type_node);
   SET_VALUE(algn, type_node);
}

void ir_node_dup::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, cst_node);
}

void ir_node_dup::operator()(const array_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, array_ty_node);
   SET_VALUE(nelements, array_ty_node);
}

void ir_node_dup::operator()(const call_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, call_node);
   SEQ_SET_NODE_ID(args, call_node);
}

void ir_node_dup::operator()(const call_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, call_stmt);
   SEQ_SET_NODE_ID(args, call_stmt);
}

void ir_node_dup::operator()(const constructor_node* obj, unsigned int& mask)
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
         if(mode && node_id1 && remap.find(node_id1) == remap.end())
         {
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;
            ir_nodeRef saved_source_tn = source_tn;
            node_id1 = create_ir_node(i->first, mode);
            curr_ir_node_ptr = saved_curr_ir_node_ptr;
            source_tn = saved_source_tn;
         }
         else
         {
            THROW_ASSERT(!node_id1 || remap.find(node_id1) != remap.end(), "missing an index");
            node_id1 = node_id1 ? remap.find(node_id1)->second : 0;
         }
         if(mode && remap.find(node_id2) == remap.end())
         {
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;
            ir_nodeRef saved_source_tn = source_tn;
            node_id2 = create_ir_node(i->second, mode);
            curr_ir_node_ptr = saved_curr_ir_node_ptr;
            source_tn = saved_source_tn;
         }
         else
         {
            THROW_ASSERT(remap.find(node_id2) != remap.end(), "missing an index");
            node_id2 = remap.find(node_id2)->second;
         }
         if(node_id1)
         {
            static_cast<constructor_node*>(curr_ir_node_ptr)
                ->add_idx_valu(TM->GetIRNode(node_id1), TM->GetIRNode(node_id2));
         }
         else
         {
            static_cast<constructor_node*>(curr_ir_node_ptr)->add_valu(TM->GetIRNode(node_id2));
         }
      }
   }
}

void ir_node_dup::operator()(const field_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitfield, field_val_node);
   SET_VALUE(bitsizealloc, field_val_node);
   SET_VALUE(algn, field_val_node);
   SET_VALUE(packed_flag, field_val_node);
   SET_VALUE(offset, field_val_node);
}

void ir_node_dup::operator()(const function_val_node* obj, unsigned int& mask)
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
   const auto prev_mode = mode;
   if(mode == ir_node_dup_mode::FUNCTION)
   {
      // Avoid recursvie function_val_node duplication
      mode = DECLARATION;
   }
   SET_NODE_ID(body, function_val_node);
   mode = prev_mode;
}

void ir_node_dup::operator()(const function_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(retn, function_ty_node);
   SEQ_SET_NODE_ID(list_of_args_type, function_ty_node);
   SET_VALUE(varargs_flag, function_ty_node);
}

void ir_node_dup::operator()(const assign_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, assign_stmt);
   SET_NODE_ID(op1, assign_stmt);
   SET_VALUE(temporary_address, assign_stmt);
}

void ir_node_dup::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void ir_node_dup::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(value, constant_int_val_node);
}

void ir_node_dup::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsize, integer_ty_node);
   SET_VALUE(unsigned_flag, integer_ty_node);
}

void ir_node_dup::operator()(const argument_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsizealloc, argument_val_node);
   SET_NODE_ID(parent, argument_val_node);
   SET_VALUE(algn, argument_val_node);
   SET_VALUE(readonly_flag, argument_val_node);
}

void ir_node_dup::operator()(const phi_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(res, phi_stmt);
   for(const auto& def_edge : GetPointerS<phi_stmt>(source_tn)->CGetDefEdgesList())
   {
      unsigned int node_id = def_edge.first->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;
            ir_nodeRef saved_source_tn = source_tn;
            node_id = create_ir_node(def_edge.first, mode);
            curr_ir_node_ptr = saved_curr_ir_node_ptr;
            source_tn = saved_source_tn;
         }
      }
      else
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
      }

      static_cast<phi_stmt*>(curr_ir_node_ptr)
          ->AddDefEdge(TM, phi_stmt::DefEdge(TM->GetIRNode(node_id), get_bbi(def_edge.second)));
   }
   SET_VALUE(virtual_flag, phi_stmt);
   if(use_counting)
   {
      static_cast<phi_stmt*>(curr_ir_node_ptr)->SetSSAUsesComputed();
   }
}

void ir_node_dup::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(ptd, pointer_ty_node);
}

void ir_node_dup::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(overflow_flag, constant_fp_val_node);
   SET_VALUE(valr, constant_fp_val_node);
   SET_VALUE(valx, constant_fp_val_node);
}

void ir_node_dup::operator()(const real_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE(bitsize, real_ty_node);
}

void ir_node_dup::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(name, struct_ty_node);
   SET_VALUE(packed_flag, struct_ty_node);
   SEQ_SET_NODE_ID(list_of_flds, struct_ty_node);
}

void ir_node_dup::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, return_stmt);
}

void ir_node_dup::operator()(const ssa_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID(type, ssa_node);
   SET_NODE_ID(var, ssa_node);
   if(mode)
   {
      static_cast<ssa_node*>(curr_ir_node_ptr)->vers = TM->get_next_vers();
   }
   else
   {
      SET_VALUE(vers, ssa_node);
   }
   SET_VALUE(use_set.anything, ssa_node);
   SET_VALUE(use_set.escaped, ssa_node);
   SET_VALUE(use_set.ipa_escaped, ssa_node);
   SET_VALUE(use_set.nonlocal, ssa_node);
   SET_VALUE(use_set.null, ssa_node);
   SEQ_SET_NODE_ID(use_set.variables, ssa_node);

   SET_VALUE(virtual_flag, ssa_node);
   SET_VALUE(default_flag, ssa_node);
   const auto& def_stmt = GetPointerS<ssa_node>(source_tn)->GetDefStmt();
   if(mode)
   {
      unsigned int node_id = def_stmt->index;
      const auto rnode = remap.find(node_id);
      if(rnode != remap.end())
      {
         node_id = rnode->second;
      }
      else
      {
         const auto saved_curr_ir_node_ptr = curr_ir_node_ptr;
         const auto saved_source_tn = source_tn;
         node_id = create_ir_node(def_stmt, mode);
         curr_ir_node_ptr = saved_curr_ir_node_ptr;
         source_tn = saved_source_tn;
      }
      static_cast<ssa_node*>(curr_ir_node_ptr)->SetDefStmt(TM->GetIRNode(node_id));
   }
   else
   {
      static_cast<ssa_node*>(curr_ir_node_ptr)->SetDefStmt(def_stmt);
   }
   if(!mode)
   {
      SET_NODE_ID(min, ssa_node);
      SET_NODE_ID(max, ssa_node);
      SET_VALUE(bit_values, ssa_node);
      SET_VALUE(range, ssa_node);
   }
}

void ir_node_dup::operator()(const statement_list_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   const auto mend = GetPointerS<const statement_list_node>(source_tn)->list_of_bloc.end();
   for(auto i = GetPointerS<const statement_list_node>(source_tn)->list_of_bloc.begin(); i != mend; ++i)
   {
      curr_bloc = new bloc(get_bbi(i->first));
      source_bloc = i->second;
      curr_bloc->visit(this);
      THROW_ASSERT(!static_cast<statement_list_node*>(curr_ir_node_ptr)->list_of_bloc.count(get_bbi(i->first)),
                   "Block already present " + STR(get_bbi(i->first)));
      static_cast<statement_list_node*>(curr_ir_node_ptr)->add_bloc(blocRef(curr_bloc));
      curr_bloc = nullptr;
      source_bloc = blocRef();
   }
   THROW_ASSERT(static_cast<statement_list_node*>(curr_ir_node_ptr)->list_of_bloc.size() ==
                    GetPointerS<const statement_list_node>(source_tn)->list_of_bloc.size(),
                "");
}

void ir_node_dup::operator()(const lut_node* obj, unsigned int& mask)
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

void ir_node_dup::operator()(const variable_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_VALUE(static_flag, variable_val_node);
   SET_VALUE(extern_flag, variable_val_node);
   SET_VALUE(addr_not_taken, variable_val_node);
   SET_NODE_ID(init, variable_val_node);
   SET_VALUE(bitsizealloc, variable_val_node);
   SET_VALUE(algn, variable_val_node);
   SET_VALUE(readonly_flag, variable_val_node);
   if(!mode)
   {
      SET_VALUE(bit_values, variable_val_node);
   }
}

void ir_node_dup::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SEQ_SET_NODE_ID(list_of_valu, constant_vector_val_node);
}

void ir_node_dup::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, vector_ty_node);
}

void ir_node_dup::operator()(const bloc* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   curr_bloc->loop_id = get_loop_id(source_bloc->loop_id);
   std::transform(source_bloc->list_of_pred.cbegin(), source_bloc->list_of_pred.cend(),
                  std::back_inserter(curr_bloc->list_of_pred), [&](const unsigned int& bbi) { return get_bbi(bbi); });
   std::transform(source_bloc->list_of_succ.cbegin(), source_bloc->list_of_succ.cend(),
                  std::back_inserter(curr_bloc->list_of_succ), [&](const unsigned int& bbi) { return get_bbi(bbi); });

   if(use_counting)
   {
      curr_bloc->SetSSAUsesComputed();
   }
   for(const auto& phi : source_bloc->CGetPhiList())
   {
      unsigned int node_id = phi->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;
            ir_nodeRef saved_source_tn = source_tn;
            bloc* saved_curr_bloc = curr_bloc;
            node_id = create_ir_node(phi, mode);
            curr_ir_node_ptr = saved_curr_ir_node_ptr;
            source_tn = saved_source_tn;
            curr_bloc = saved_curr_bloc;
         }
      }
      else
      {
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
         node_id = remap.find(node_id)->second;
      }
      curr_bloc->AddPhi(TM->GetIRNode(node_id));
   }
   for(const auto& stmt : source_bloc->CGetStmtList())
   {
      unsigned int node_id = stmt->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            const auto saved_curr_ir_node_ptr = curr_ir_node_ptr;
            const auto saved_source_tn = source_tn;
            const auto saved_curr_bloc = curr_bloc;
            node_id = create_ir_node(stmt, mode);
            curr_ir_node_ptr = saved_curr_ir_node_ptr;
            source_tn = saved_source_tn;
            curr_bloc = saved_curr_bloc;
         }
      }
      else
      {
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
         node_id = remap.find(node_id)->second;
      }
      curr_bloc->PushBack(TM->GetIRNode(node_id), AppM);
   }
}

void ir_node_dup::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ASSERT(source_tn, "");
   if(!GetPointerS<multi_way_if_stmt>(source_tn)->list_of_cond.empty())
   {
      for(const auto& cond : GetPointerS<multi_way_if_stmt>(source_tn)->list_of_cond)
      {
         if(cond.first)
         {
            unsigned int node_id = cond.first->index;
            if(mode)
            {
               if(remap.find(node_id) != remap.end())
               {
                  node_id = remap.find(node_id)->second;
                  THROW_ASSERT(node_id, "");
               }
               else
               {
                  ir_node* saved_curr_ir_node_ptr = curr_ir_node_ptr;
                  ir_nodeRef saved_source_tn = source_tn;
                  node_id = create_ir_node(cond.first, mode);
                  curr_ir_node_ptr = saved_curr_ir_node_ptr;
                  source_tn = saved_source_tn;
               }
            }
            else
            {
               THROW_ASSERT(remap.find(node_id) != remap.end(), "missing " + STR(TM->GetIRNode(node_id)));
               node_id = remap.find(node_id)->second;
               THROW_ASSERT(node_id, "");
            }
            static_cast<multi_way_if_stmt*>(curr_ir_node_ptr)->add_cond(TM->GetIRNode(node_id), get_bbi(cond.second));
         }
         else
         {
            static_cast<multi_way_if_stmt*>(curr_ir_node_ptr)->add_cond(ir_nodeRef(), get_bbi(cond.second));
         }
      }
   }
}
