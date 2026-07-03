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
 * @file ir_node_factory.cpp
 * @brief IR node factory. This class, exploiting the visitor design pattern, adds an IR node to the ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_node_factory.hpp"

#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "token_interface.hpp"
#include "utility.hpp"

#define CREATE_IR_NODE_CASE_BODY(ir_node_name, node_id) \
   {                                                    \
      auto tnn = new ir_node_name(node_id);             \
      cur = ir_nodeRef(tnn);                            \
      TM.AddIRNode(cur);                                \
      curr_ir_node_ptr = tnn;                           \
      tnn->visit(this);                                 \
      curr_ir_node_ptr = nullptr;                       \
      break;                                            \
   }

ir_nodeRef ir_node_factory::create_ir_node(unsigned int node_id, enum kind ir_node_type)
{
   ir_nodeRef cur;
   switch(ir_node_type)
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
      case multi_way_if_stmt_K:
         CREATE_IR_NODE_CASE_BODY(multi_way_if_stmt, node_id)
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
      case identifier_node_K: /// special care is reserved for identifier_nodes
      {
         if(ir_node_schema.find(TOK(TOK_STRG)) != ir_node_schema.end())
         {
            cur = ir_nodeRef(new identifier_node(node_id, ir_node_schema.find(TOK(TOK_STRG))->second, &TM));
         }
         else
         {
            THROW_ERROR("Incorrect schema for identifier_node: no TOK_STRG");
         }
         TM.AddIRNode(cur);
         break;
      }
      case widen_mul_node_K:
         CREATE_IR_NODE_CASE_BODY(widen_mul_node, node_id)
      case nop_stmt_K:
         CREATE_IR_NODE_CASE_BODY(nop_stmt, node_id)
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
         THROW_UNREACHABLE("Creation of IR node of type " + STR(ir_node_type) + " not implemented");
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   THROW_ASSERT(cur, "");
   return cur;
}

void ir_node_factory::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + obj->get_kind_text());
}

void ir_node_factory::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + obj->get_kind_text());
}

#define SET_NODE_ID_OPT(token, field, type)                                                      \
   if(ir_node_schema.find(TOK(token)) != ir_node_schema.end())                                   \
   {                                                                                             \
      auto node_id = static_cast<unsigned>(std::stoul(ir_node_schema.find(TOK(token))->second)); \
      static_cast<type*>(curr_ir_node_ptr)->field = TM.GetIRReindex(node_id);                    \
   }

#define SET_NODE_ID(token, field, type)                                                          \
   {                                                                                             \
      THROW_ASSERT(ir_node_schema.find(TOK(token)) != ir_node_schema.end(),                      \
                   std::string("ir_node_schema must have ") + STOK(token) + " value");           \
      auto node_id = static_cast<unsigned>(std::stoul(ir_node_schema.find(TOK(token))->second)); \
      static_cast<type*>(curr_ir_node_ptr)->field = TM.GetIRReindex(node_id);                    \
   }

#define SET_VALUE_OPT(token, field, type)                                                      \
   if(ir_node_schema.find(TOK(token)) != ir_node_schema.end())                                 \
   {                                                                                           \
      static_cast<type*>(curr_ir_node_ptr)->field =                                            \
          boost::lexical_cast<decltype(type::field)>(ir_node_schema.find(TOK(token))->second); \
   }

#define SET_VALUE(token, field, type)                                               \
   THROW_ASSERT(ir_node_schema.find(TOK(token)) != ir_node_schema.end(),            \
                std::string("IR node schema must have ") + STOK(token) + " value"); \
   static_cast<type*>(curr_ir_node_ptr)->field =                                    \
       boost::lexical_cast<decltype(type::field)>(ir_node_schema.find(TOK(token))->second);

#define IR_NOT_YET_IMPLEMENTED(token)                                    \
   THROW_ASSERT(ir_node_schema.find(TOK(token)) == ir_node_schema.end(), \
                std::string("field not yet supported ") + STOK(token))

void ir_node_factory::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   // NOTE: const_cast below are "safe" since the following assert must be true
   THROW_ASSERT(obj == dynamic_cast<IR_LocInfo*>(curr_ir_node_ptr), "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ASSERT(ir_node_schema.find(TOK(TOK_IR_LOCINFO)) != ir_node_schema.end(),
                "ir_node_schema must have TOK_IR_LOCINFO value");
   const std::string& loc_info_str = ir_node_schema.find(TOK(TOK_IR_LOCINFO))->second;
   std::string::size_type colon_pos2 = loc_info_str.rfind(':');
   std::string::size_type colon_pos = loc_info_str.rfind(':', colon_pos2 - 1);
   const_cast<IR_LocInfo*>(obj)->include_name = loc_info_str.substr(0, colon_pos);
   const_cast<IR_LocInfo*>(obj)->line_number =
       static_cast<unsigned>(std::stoul(loc_info_str.substr(colon_pos + 1, colon_pos2 - colon_pos - 1)));
   const_cast<IR_LocInfo*>(obj)->column_number = static_cast<unsigned>(std::stoul(loc_info_str.substr(colon_pos2 + 1)));
}

void ir_node_factory::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_NAME, name, decl_node);
   SET_NODE_ID_OPT(TOK_MNGL, mngl, decl_node);
   SET_NODE_ID_OPT(TOK_TYPE, type, decl_node);
   SET_NODE_ID_OPT(TOK_PARENT, parent, decl_node);
   SET_VALUE_OPT(TOK_OPERATING_SYSTEM, operating_system_flag, decl_node);
   SET_VALUE_OPT(TOK_LIBRARY_SYSTEM, library_system_flag, decl_node);
   SET_VALUE_OPT(TOK_LIBBAMBU, libbambu_flag, decl_node);
}

void ir_node_factory::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_TYPE, type, expr_node);
}

void ir_node_factory::operator()(const node_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_MEMUSE, memuse, node_stmt);
   SET_NODE_ID_OPT(TOK_MEMDEF, memdef, node_stmt);
   SET_NODE_ID_OPT(TOK_PARENT, parent, node_stmt);
   SET_NODE_ID_OPT(TOK_PREDICATE, predicate, node_stmt);
   SET_VALUE_OPT(TOK_BB_INDEX, bb_index, node_stmt);
   IR_NOT_YET_IMPLEMENTED(TOK_VUSE);
   IR_NOT_YET_IMPLEMENTED(TOK_VDEF);
}

void ir_node_factory::operator()(const unary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, unary_node);
}

void ir_node_factory::operator()(const binary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, binary_node);
   SET_NODE_ID(TOK_OP1, op1, binary_node);
}

void ir_node_factory::operator()(const ternary_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, ternary_node);
   SET_NODE_ID(TOK_OP1, op1, ternary_node);
   SET_NODE_ID_OPT(TOK_OP2, op2, ternary_node);
}

void ir_node_factory::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_BITSIZEALLOC, bitsizealloc, type_node);
   SET_VALUE_OPT(TOK_SYSTEM, system_flag, type_node);
   SET_VALUE_OPT(TOK_ALGN, algn, type_node);
}

void ir_node_factory::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, cst_node);
}

void ir_node_factory::operator()(const array_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ELTS, elts, array_ty_node);
   SET_VALUE_OPT(TOK_NELEMENTS, nelements, array_ty_node);
}

void ir_node_factory::operator()(const call_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_FN, fn, call_node);
   if(ir_node_schema.find(TOK(TOK_ARG)) != ir_node_schema.end())
   {
      const auto args = string_to_container<std::vector<unsigned int>>(ir_node_schema.find(TOK(TOK_ARG))->second, "_");
      for(const auto arg : args)
      {
         static_cast<call_node*>(curr_ir_node_ptr)->args.push_back(TM.GetIRReindex(arg));
      }
   }
}

void ir_node_factory::operator()(const call_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_FN, fn, call_stmt);
   if(ir_node_schema.find(TOK(TOK_ARG)) != ir_node_schema.end())
   {
      const auto args = string_to_container<std::vector<unsigned int>>(ir_node_schema.find(TOK(TOK_ARG))->second, "_");
      for(const auto arg : args)
      {
         static_cast<call_stmt*>(curr_ir_node_ptr)->args.push_back(TM.GetIRReindex(arg));
      }
   }
}

void ir_node_factory::operator()(const constructor_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, constructor_node);
   IR_NOT_YET_IMPLEMENTED(TOK_IDX);
   IR_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<std::pair< ir_nodeRef, ir_nodeRef> >::const_iterator vend = obj->list_of_idx_valu.end();
   // for (std::vector<std::pair< ir_nodeRef, ir_nodeRef> >::const_iterator i = obj->list_of_idx_valu.begin(); i !=
   // vend; i++)
   //{
   //   write_when_not_null(STOK(TOK_IDX), i->first);
   //   write_when_not_null(STOK(TOK_VALU), i->second);
   //}
}

void ir_node_factory::operator()(const field_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_ALGN, algn, field_val_node);
   SET_VALUE_OPT(TOK_BITSIZEALLOC, bitsizealloc, field_val_node);
   SET_VALUE_OPT(TOK_PACKED, packed_flag, field_val_node);
   SET_VALUE_OPT(TOK_OFFSET, offset, field_val_node);
}

void ir_node_factory::operator()(const function_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_FN, fn, function_val_node);
   IR_NOT_YET_IMPLEMENTED(TOK_ARG);
   // std::vector<ir_nodeRef>::const_iterator vend2 = obj->list_of_args.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_args.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_ARGS), *i);

   SET_VALUE_OPT(TOK_STATIC, static_flag, function_val_node);
   SET_VALUE_OPT(TOK_WRITING_MEMORY, writing_memory, function_val_node);
   SET_VALUE_OPT(TOK_READING_MEMORY, reading_memory, function_val_node);
   SET_VALUE_OPT(TOK_PIPELINE_ENABLED, pipeline_enabled, function_val_node);
   SET_VALUE_OPT(TOK_INITIATION_TIME, initiation_time, function_val_node);
   SET_NODE_ID_OPT(TOK_BODY, body, function_val_node);
}

void ir_node_factory::operator()(const function_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_RETN, retn, function_ty_node);
   if(ir_node_schema.find(TOK(TOK_ARG)) != ir_node_schema.end())
   {
      const auto args = string_to_container<std::vector<unsigned int>>(ir_node_schema.find(TOK(TOK_ARG))->second, "_");
      for(const auto arg : args)
      {
         static_cast<function_ty_node*>(curr_ir_node_ptr)->list_of_args_type.push_back(TM.GetIRReindex(arg));
      }
   }
   SET_VALUE_OPT(TOK_VARARGS, varargs_flag, function_ty_node);
}

void ir_node_factory::operator()(const assign_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, assign_stmt);
   SET_NODE_ID(TOK_OP1, op1, assign_stmt);
   SET_VALUE_OPT(TOK_ADDR, temporary_address, assign_stmt);
}

void ir_node_factory::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void ir_node_factory::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_VALUE, value, constant_int_val_node);
}

void ir_node_factory::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_BITSIZE, bitsize, integer_ty_node);
   SET_VALUE_OPT(TOK_UNSIGNED, unsigned_flag, integer_ty_node);
}

void ir_node_factory::operator()(const argument_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_BITSIZEALLOC, bitsizealloc, argument_val_node);
   SET_VALUE_OPT(TOK_ALGN, algn, argument_val_node);
   SET_VALUE_OPT(TOK_READONLY, readonly_flag, argument_val_node);
}

void ir_node_factory::operator()(const phi_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_RES, res, phi_stmt);
   SET_VALUE_OPT(TOK_VIRTUAL, virtual_flag, phi_stmt);
   // IR_NOT_YET_IMPLEMENTED(TOK_DEF);
   // IR_NOT_YET_IMPLEMENTED(TOK_EDGE);
   // std::vector<std::pair< ir_nodeRef, int> >::const_iterator vend = obj->list_of_def_edge.end();
   // for (std::vector<std::pair< ir_nodeRef, int> >::const_iterator i = obj->list_of_def_edge.begin(); i != vend;
   // i++)
   //{
   //   write_when_not_null(STOK(TOK_DEF), i->first);
   //   WRITE_NFIELD(os, STOK(TOK_EDGE), i->second);
   //}
}

void ir_node_factory::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_PTD, ptd, pointer_ty_node);
}

void ir_node_factory::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_OVERFLOW, overflow_flag, constant_fp_val_node);
   SET_VALUE_OPT(TOK_VALR, valr, constant_fp_val_node);
   SET_VALUE_OPT(TOK_VALX, valx, constant_fp_val_node);
}

void ir_node_factory::operator()(const real_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_BITSIZE, bitsize, real_ty_node);
}

void ir_node_factory::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_NAME, name, struct_ty_node);
   SET_VALUE_OPT(TOK_PACKED, packed_flag, struct_ty_node);

   IR_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<ir_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
}

void ir_node_factory::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, return_stmt);
}

void ir_node_factory::operator()(const ssa_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_TYPE, type, ssa_node);
   SET_NODE_ID_OPT(TOK_VAR, var, ssa_node);
   SET_VALUE_OPT(TOK_VERS, vers, ssa_node);
   // SET_NODE_ID_OPT(TOK_PTR_INFO,ptr_info,ssa_node);

   SET_VALUE_OPT(TOK_VIRTUAL, virtual_flag, ssa_node);
   SET_NODE_ID_OPT(TOK_MIN, min, ssa_node);
   SET_NODE_ID_OPT(TOK_MAX, max, ssa_node);
   SET_VALUE_OPT(TOK_BIT_VALUES, bit_values, ssa_node);
}

void ir_node_factory::operator()(const statement_list_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   IR_NOT_YET_IMPLEMENTED(TOK_STMT);
   // std::vector<ir_nodeRef>::const_iterator vend = obj->list_of_stmt.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);
   IR_NOT_YET_IMPLEMENTED(TOK_BLOC);
   // std::map<int, blocRef>::const_iterator mend = obj->list_of_bloc.end();
   // for (std::map<int, blocRef>::const_iterator i = obj->list_of_bloc.begin(); i != mend; i++)
   //   write_when_not_null_bloc(STOK(TOK_BLOC), i->second);
}

void ir_node_factory::operator()(const lut_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP0, op0, lut_node);
   SET_NODE_ID_OPT(TOK_OP1, op1, lut_node);
   SET_NODE_ID_OPT(TOK_OP2, op2, lut_node);
   SET_NODE_ID_OPT(TOK_OP3, op3, lut_node);
   SET_NODE_ID_OPT(TOK_OP4, op4, lut_node);
   SET_NODE_ID_OPT(TOK_OP5, op5, lut_node);
   SET_NODE_ID_OPT(TOK_OP6, op6, lut_node);
   SET_NODE_ID_OPT(TOK_OP7, op7, lut_node);
   SET_NODE_ID_OPT(TOK_OP8, op8, lut_node);
}

void ir_node_factory::operator()(const variable_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_EXTERN, extern_flag, variable_val_node);
   SET_VALUE_OPT(TOK_ADDR_NOT_TAKEN, addr_not_taken, variable_val_node);
   SET_VALUE_OPT(TOK_STATIC, static_flag, variable_val_node);
   SET_NODE_ID_OPT(TOK_INIT, init, variable_val_node);
   SET_VALUE_OPT(TOK_BITSIZEALLOC, bitsizealloc, variable_val_node);
   SET_VALUE_OPT(TOK_ALGN, algn, variable_val_node);
   SET_VALUE_OPT(TOK_READONLY, readonly_flag, variable_val_node);
   SET_VALUE_OPT(TOK_BIT_VALUES, bit_values, variable_val_node);
}

void ir_node_factory::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   IR_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<ir_nodeRef>::const_iterator vend = obj->list_of_valu.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_valu.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VALU), *i);
}

void ir_node_factory::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ELTS, elts, vector_ty_node);
}

void ir_node_factory::operator()(const bloc* obj, unsigned int& mask)
{
#define SET_BLOC_VALUE_OPT(token, field)                                                       \
   if(ir_node_schema.find(TOK(token)) != ir_node_schema.end())                                 \
   {                                                                                           \
      dynamic_cast<bloc*>(curr_ir_node_ptr)->field =                                           \
          boost::lexical_cast<decltype(bloc::field)>(ir_node_schema.find(TOK(token))->second); \
   }

   ir_node_mask::operator()(obj, mask);
   // WRITE_UFIELD(os, obj->number);
   SET_BLOC_VALUE_OPT(TOK_LOOP_ID, loop_id);
   IR_NOT_YET_IMPLEMENTED(TOK_PRED);
   // std::vector<int>::const_iterator vend1 = obj->list_of_pred.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_pred.begin(); i != vend1; i++)
   //   if(*i == bloc::ENTRY_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_PRED), STOK(TOK_ENTRY));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_PRED), *i);
   IR_NOT_YET_IMPLEMENTED(TOK_SUCC);
   // std::vector<int>::const_iterator vend2 = obj->list_of_succ.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_succ.begin(); i != vend2; i++)
   //   if(*i == bloc::EXIT_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_SUCC), STOK(TOK_EXIT));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_SUCC), *i);
   IR_NOT_YET_IMPLEMENTED(TOK_PHI);
   // std::vector<ir_nodeRef>::const_iterator vend3 = obj->list_of_phi.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_phi.begin(); i != vend3; i++)
   //   write_when_not_null(STOK(TOK_PHI), *i);
   // std::vector<ir_nodeRef>::const_iterator vend4 = obj->list_of_stmt.end();
   IR_NOT_YET_IMPLEMENTED(TOK_STMT);
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend4; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);

#undef SET_BLOC_VALUE_OPT
}

void ir_node_factory::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_ir_node_ptr, "wrong factory setup");
   ir_node_mask::operator()(obj, mask);
   /// not yet implemented
}
