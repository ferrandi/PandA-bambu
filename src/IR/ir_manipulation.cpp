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
 * @file ir_manipulation.cpp
 * @brief Class implementing some useful functions to create IR nodes and to
 * manipulate the IR manager.
 *
 * This class implements some useful functions to create IR nodes and to
 * manipulate the IR manager.
 *
 * @author Stefano Viazzi <stefano.viazzi@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_manipulation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_node_dup.hpp"
#include "ir_reindex.hpp"
#include "ir_reindex_remove.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <boost/range/adaptor/reversed.hpp>

#include <algorithm>
#include <iostream>

using IRSchema = ir_manager::IRSchema;

#define IR_NOT_YET_IMPLEMENTED(token) THROW_ERROR(std::string("field not yet supported ") + STOK(token))
#define ALGN_VOID 8
#define ALGN_BIT_SIZE 64
#define BITSIZE_BIT_SIZE 64
#define SIZE_VALUE_BIT_SIZE 64
#define ALGN_BOOL 8
#define BITSIZE_BOOL 1
#define SIZE_VALUE_BOOL 8
#define ALGN_UNSIGNED_INT 32
#define BITSIZE_UNSIGNED_INT 32
#define SIZE_VALUE_UNSIGNED_INT 32
#define ALGN_UNSIGNED_LONG_LONG_INT 64
#define BITSIZE_UNSIGNED_LONG_LONG_INT 64
#define SIZE_VALUE_UNSIGNED_LONG_LONG_INT 64
#define ALGN_INT 32
#define BITSIZE_INT 32
#define SIZE_VALUE_INT 32
#define ALGN_POINTER_M64 64
#define ALGN_POINTER_M32 32
#define SIZE_VALUE_POINTER_M32 32
#define SIZE_VALUE_POINTER_M64 64
#define SIZE_VALUE_FUNCTION 8

ir_manipulation::ir_manipulation(const ir_managerRef& _IRM, const ParameterConstRef& _parameters,
                                 const application_managerRef _AppM)
    : IRM(_IRM), AppM(_AppM), parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

/// EXPRESSION_IR_NODES

/// TODO weight_node to fix in ir_node_factory.cpp
/// Create an unary operation
ir_nodeRef ir_manipulation::create_unary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op,
                                                   const std::string& loc_info, kind operation_kind) const
{
   /// Check if the ir_node given are ir_reindex
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");

   /// Check if the ir_node type is a unary expression
   switch(operation_kind)
   {
      case CASE_UNARY_NODES:
      {
         break;
      }

      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR("The operation given is not a unary expression");
   }

   /// Check if it is a correct node type
   switch(type->get_kind())
   {
      case array_ty_node_K:
      case function_ty_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(type->index) + std::string("): ") +
                     type->get_kind_text());
   }

   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_OP)] = STR(op->index);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   const auto tn = IRM->create_ir_node(operation_kind, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

/// TODO weight_node to fix in ir_node_factory.cpp
/// Create a binary expression
ir_nodeRef ir_manipulation::create_binary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op0,
                                                    const ir_nodeRef& op1, const std::string& loc_info,
                                                    kind operation_kind) const
{
   /// Check if the ir_node given are ir_reindex
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");

   /// Check if the ir_node type is a binary expression
   switch(operation_kind)
   {
      case CASE_BINARY_NODES:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR("The operation given is not a binary expression");
   }

   /// Check if it is a correct node type
   switch(type->get_kind())
   {
      case array_ty_node_K:
      case function_ty_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(type->index) + std::string("): ") +
                     type->get_kind_text());
   }

   if(operation_kind == eq_node_K || operation_kind == ne_node_K || operation_kind == lt_node_K ||
      operation_kind == le_node_K || operation_kind == gt_node_K || operation_kind == ge_node_K)
   {
      THROW_ASSERT((ir_helper::IsVectorType(type) && ir_helper::IsBooleanType(ir_helper::CGetElements(type))) ||
                       ir_helper::IsBooleanType(type),
                   "");
   }

   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_OP0)] = STR(op0->index);
   IR_schema[TOK(TOK_OP1)] = STR(op1->index);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;

   const auto tn = IRM->create_ir_node(operation_kind, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

/// TODO weight_node to fix in ir_node_factory.cpp
/// Create a ternary expression
ir_nodeRef ir_manipulation::create_ternary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op0,
                                                     const ir_nodeRef& op1, const ir_nodeRef& op2,
                                                     const std::string& loc_info, kind operation_kind) const
{
   /// Check if the ir_node given are ir_reindex
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");

   /// Check if the ir_node type is a ternary expression
   switch(operation_kind)
   {
      case CASE_TERNARY_NODES:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR("The operation given is not a ternary expression");
   }

   /// Check if it is a correct node type
   switch(type->get_kind())
   {
      case array_ty_node_K:
      case function_ty_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(type->index) + std::string("): ") +
                     type->get_kind_text());
   }

   /// some checks
   if(operation_kind == select_node_K)
   {
      THROW_ASSERT(op1->index == 0 || ir_helper::Size(ir_helper::CGetType(op1)) == ir_helper::Size(type),
                   "unexpected pattern (<" + STR(ir_helper::Size(ir_helper::CGetType(op1))) + ">" +
                       STR(ir_helper::CGetType(op1)) + " != <" + STR(ir_helper::Size(type)) + ">" + type->ToString() +
                       ")");
      THROW_ASSERT(op2->index == 0 || ir_helper::Size(ir_helper::CGetType(op2)) == ir_helper::Size(type),
                   "unexpected pattern (<" + STR(ir_helper::Size(ir_helper::CGetType(op2))) + ">" +
                       STR(ir_helper::CGetType(op2)) + " != <" + STR(ir_helper::Size(type)) + ">" + type->ToString() +
                       ")");
   }
   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_OP0)] = STR(op0->index);
   IR_schema[TOK(TOK_OP1)] = STR(op1->index);
   IR_schema[TOK(TOK_OP2)] = STR(op2->index);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;

   const auto tn = IRM->create_ir_node(operation_kind, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

ir_nodeRef ir_manipulation::create_lut_node(const ir_nodeConstRef& type, const ir_nodeRef& op0, const ir_nodeRef& op1,
                                            const ir_nodeRef& op2, const ir_nodeRef& op3, const ir_nodeRef& op4,
                                            const ir_nodeRef& op5, const ir_nodeRef& op6, const ir_nodeRef& op7,
                                            const ir_nodeRef& op8, const std::string& loc_info) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");
   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_OP0)] = STR(op0->index);
   IR_schema[TOK(TOK_OP1)] = STR(op1->index);
   if(op2)
   {
      IR_schema[TOK(TOK_OP2)] = STR(op2->index);
   }
   if(op3)
   {
      IR_schema[TOK(TOK_OP3)] = STR(op3->index);
   }
   if(op4)
   {
      IR_schema[TOK(TOK_OP4)] = STR(op4->index);
   }
   if(op5)
   {
      IR_schema[TOK(TOK_OP5)] = STR(op5->index);
   }
   if(op6)
   {
      IR_schema[TOK(TOK_OP6)] = STR(op6->index);
   }
   if(op7)
   {
      IR_schema[TOK(TOK_OP7)] = STR(op7->index);
   }
   if(op8)
   {
      IR_schema[TOK(TOK_OP8)] = STR(op8->index);
   }
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;

   const auto tn = IRM->create_ir_node(lut_node_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

ir_nodeRef ir_manipulation::create_extract_bit_node(const ir_nodeRef& op0, const ir_nodeRef& op1,
                                                    const std::string& loc_info) const
{
   auto boolType = GetBooleanType();
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");
   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(boolType->index);
   IR_schema[TOK(TOK_OP0)] = STR(op0->index);
   IR_schema[TOK(TOK_OP1)] = STR(op1->index);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   const auto tn = IRM->create_ir_node(extract_bit_node_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}
/// CONST_OBJ_IR_NODES

/// Create an constant_int_val_node node
ir_nodeRef ir_manipulation::CreateIntegerCst(const ir_nodeConstRef& type, integer_cst_t value,
                                             const unsigned int integer_cst_nid) const
{
   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_VALUE)] = STR(value);

   ir_nodeRef node_ref;

   node_ref = IRM->create_ir_node(integer_cst_nid, constant_int_val_node_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(node_ref->index) + " (" + node_ref->get_kind_text() + ")");
   return node_ref;
}

/// IDENTIFIER_IR_NODE

/// Create an identifier node
ir_nodeRef ir_manipulation::create_identifier_node(const std::string& strg) const
{
   THROW_ASSERT(!strg.empty(), "It requires a non empty string");

   unsigned int node_nid = IRM->find_identifier_nodeID(strg);

   ir_nodeRef node_ref;
   if(!node_nid)
   {
      IRSchema IR_schema;
      IR_schema[TOK(TOK_STRG)] = strg;
      node_ref = IRM->create_ir_node(identifier_node_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(node_ref->index) + " (" + node_ref->get_kind_text() + " " + strg + ")");
   }
   else
   {
      node_ref = IRM->GetIRNode(node_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Found   node " + STR(node_ref->index) + " (" + node_ref->get_kind_text() + " " + strg + ")");
   }
   return node_ref;
}

/// DECL_NODES

/// Create a variable_val_node
ir_nodeRef ir_manipulation::create_var_decl(const ir_nodeRef& name, const ir_nodeConstRef& type,
                                            const ir_nodeRef& parent, unsigned int bitsizealloc, const ir_nodeRef& init,
                                            const std::string& loc_info, unsigned int algn, bool extern_flag,
                                            bool static_flag, bool readonly_flag, const std::string& bit_values,
                                            bool addr_not_taken) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");
   IRSchema IR_schema;
   if(name)
   {
      IR_schema[TOK(TOK_NAME)] = STR(name->index);
   }
   if(init)
   {
      IR_schema[TOK(TOK_INIT)] = STR(init->index);
   }

   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_PARENT)] = STR(parent->index);
   IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(bitsizealloc);
   IR_schema[TOK(TOK_ALGN)] = STR(algn);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   IR_schema[TOK(TOK_EXTERN)] = STR(extern_flag);
   IR_schema[TOK(TOK_STATIC)] = STR(static_flag);
   IR_schema[TOK(TOK_READONLY)] = STR(readonly_flag);
   IR_schema[TOK(TOK_BIT_VALUES)] = bit_values;
   IR_schema[TOK(TOK_ADDR_NOT_TAKEN)] = STR(addr_not_taken);

   const auto tn = IRM->create_ir_node(variable_val_node_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

ir_nodeRef ir_manipulation::create_translation_unit_decl() const
{
   ir_nodeRef translation_unit_decl_node;
   IRSchema IR_schema;
   IR_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   unsigned int translation_unit_decl_nid = IRM->find0(module_unit_node_K, IR_schema);
   if(!translation_unit_decl_nid)
   {
      translation_unit_decl_node = IRM->create_ir_node(module_unit_node_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(translation_unit_decl_node->index) + " (" +
                        translation_unit_decl_node->get_kind_text() + ")");
   }
   else
   {
      translation_unit_decl_node = IRM->GetIRNode(translation_unit_decl_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Found   node " + STR(translation_unit_decl_node->index) + " (" +
                        translation_unit_decl_node->get_kind_text() + ")");
   }
   return translation_unit_decl_node;
}

/// Create argument_val_node
ir_nodeRef ir_manipulation::create_parm_decl(const ir_nodeRef& name, const ir_nodeConstRef& type,
                                             const ir_nodeRef& parent, const ir_nodeRef& init,
                                             const std::string& loc_info, bool readonly_flag) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");
   const auto tnode = GetPointer<const type_node>(type);

   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   IR_schema[TOK(TOK_PARENT)] = STR(parent->index);
   IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(tnode->bitsizealloc);
   IR_schema[TOK(TOK_ALGN)] = STR(tnode->algn);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   IR_schema[TOK(TOK_READONLY)] = STR(readonly_flag);
   if(name)
   {
      IR_schema[TOK(TOK_NAME)] = STR(name->index);
   }
   if(init)
   {
      IR_schema[TOK(TOK_INIT)] = STR(init->index);
   }

   const auto tn = IRM->create_ir_node(argument_val_node_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(tn->index) + " (" + tn->get_kind_text() + ")");
   return tn;
}

/// TYPE_OBJ

/// Create a void type
ir_nodeRef ir_manipulation::GetVoidType() const
{
   IRSchema IR_schema;
   ir_nodeRef void_node;

   IR_schema[TOK(TOK_ALGN)] = STR(ALGN_VOID);
   // If a void type already exists, it is not created a new one
   unsigned int void_type_nid = IRM->find0(void_ty_node_K, IR_schema);

   if(!void_type_nid)
   {
      void_node = IRM->create_ir_node(void_ty_node_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(void_node->index) + " (void_ty_node)");

      ir_reindex_remove(*IRM).operator()(void_node);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(void_node->index) + " (" + void_node->get_kind_text() + " void)");
   }
   else
   {
      void_node = IRM->GetIRNode(void_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Found   node " + STR(void_node->index) + " (" + void_node->get_kind_text() + " void)");
   }
   return void_node;
}

ir_nodeRef ir_manipulation::GetParametricIntegerType(unsigned algn, unsigned bitsize, bool unsigned_p,
                                                     unsigned int size_value) const
{
   IRSchema IR_schema;
   ir_nodeRef type_node;

   IR_schema[TOK(TOK_ALGN)] = STR(algn);
   IR_schema[TOK(TOK_BITSIZE)] = STR(bitsize);
   IR_schema[TOK(TOK_UNSIGNED)] = STR(unsigned_p);
   auto type_nid = this->IRM->find0(integer_ty_node_K, IR_schema);

   if(!type_nid)
   {
      ir_reindex_remove trr(*IRM);
      IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(size_value);

      type_node = IRM->create_ir_node(integer_ty_node_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(type_node->index) + " (" + type_node->get_kind_text() + ")");
      trr(type_node);
   }
   else
   {
      type_node = IRM->GetIRNode(type_nid);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Found  node " + STR(type_node->index) + " (" + type_node->get_kind_text() + ")");
   }
   return type_node;
}

/// Create a size type
ir_nodeRef ir_manipulation::GetSizeType() const
{
   return GetParametricIntegerType(ALGN_BIT_SIZE, BITSIZE_BIT_SIZE, true, SIZE_VALUE_BIT_SIZE);
}

/// Create a bit_size type
ir_nodeRef ir_manipulation::GetBitsizeType() const
{
   return GetSizeType();
}

/// Create a boolean type
ir_nodeRef ir_manipulation::GetBooleanType() const
{
   return GetParametricIntegerType(ALGN_BOOL, BITSIZE_BOOL, true, SIZE_VALUE_BOOL);
}

/// Create an unsigned integer type
ir_nodeRef ir_manipulation::GetUnsignedIntegerType() const
{
   return GetParametricIntegerType(ALGN_UNSIGNED_INT, BITSIZE_UNSIGNED_INT, true, SIZE_VALUE_UNSIGNED_INT);
}

ir_nodeRef ir_manipulation::GetUnsignedLongLongType() const
{
   return GetParametricIntegerType(ALGN_UNSIGNED_LONG_LONG_INT, BITSIZE_UNSIGNED_LONG_LONG_INT, true,
                                   SIZE_VALUE_UNSIGNED_LONG_LONG_INT);
}

/// Create an integer type
ir_nodeRef ir_manipulation::GetSignedIntegerType() const
{
   return GetParametricIntegerType(ALGN_INT, BITSIZE_INT, true, SIZE_VALUE_INT);
}

/// Create a pointer type
ir_nodeRef ir_manipulation::GetPointerType(const ir_nodeConstRef& ptd, unsigned long long algn) const
{
   IRSchema IR_schema;
   IR_schema[TOK(TOK_PTD)] = STR(ptd->index);
   auto m64P = parameters->getOption<std::string>(OPT_cc_m_env).find("-m64") != std::string::npos;
   if(!algn)
   {
      algn = m64P ? ALGN_POINTER_M64 : ALGN_POINTER_M32;
   }
   IR_schema[TOK(TOK_ALGN)] = STR(algn);

   ir_nodeRef pointer_type_node;

   unsigned int pointer_type_nid = this->IRM->find0(pointer_ty_node_K, IR_schema);

   if(!pointer_type_nid)
   {
      const auto size_node = m64P ? SIZE_VALUE_POINTER_M64 : SIZE_VALUE_POINTER_M32;

      IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(size_node);
      pointer_type_node = IRM->create_ir_node(pointer_ty_node_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "Created node " + STR(pointer_type_nid) + " (pointer_ty_node)");
   }
   else
   {
      pointer_type_node = IRM->GetIRNode(pointer_type_nid);
   }
   return pointer_type_node;
}

ir_nodeRef ir_manipulation::GetCustomIntegerType(unsigned long long bitsize, bool unsigned_p) const
{
   return GetParametricIntegerType(get_aligned_bitsize(static_cast<unsigned>(bitsize)), static_cast<unsigned>(bitsize),
                                   unsigned_p, static_cast<unsigned>(bitsize));
}

ir_nodeRef ir_manipulation::GetFunctionType(const ir_nodeConstRef& returnType,
                                            const std::vector<ir_nodeConstRef>& argsT) const
{
   IRSchema IR_schema;

   IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(SIZE_VALUE_FUNCTION);
   IR_schema[TOK(TOK_ALGN)] = STR(8);
   IR_schema[TOK(TOK_RETN)] = STR(returnType->index);
   std::string args_string;
   for(const auto& arg : argsT)
   {
      if(!args_string.empty())
      {
         args_string += "_";
      }
      args_string += STR(arg->index);
   }
   IR_schema[TOK(TOK_ARG)] = args_string;
   return IRM->create_ir_node(function_ty_node_K, IR_schema);
}

/// MISCELLANEOUS_OBJ_IR_NODES

/// SSA_NODE

/// Create a ssa_node node
ir_nodeRef ir_manipulation::create_ssa_name(const ir_nodeConstRef& var, const ir_nodeConstRef& type,
                                            const ir_nodeConstRef& min, const ir_nodeConstRef& max,
                                            bool virtual_flag) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Creating ssa starting from var " + (var ? var->ToString() : "") + " and type " +
                      (type ? type->ToString() : ""));

   IRSchema IR_schema;
   if(type)
   {
      IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   }
   if(var)
   {
      THROW_ASSERT(var->get_kind() == variable_val_node_K or var->get_kind() == argument_val_node_K,
                   var->get_kind_text());
      IR_schema[TOK(TOK_VAR)] = STR(var->index);
   }
   if(min)
   {
      IR_schema[TOK(TOK_MIN)] = STR(min->index);
   }
   if(max)
   {
      IR_schema[TOK(TOK_MAX)] = STR(max->index);
   }
   IR_schema[TOK(TOK_VERS)] = STR(IRM->get_next_vers());
   IR_schema[TOK(TOK_VIRTUAL)] = STR(virtual_flag);

   const auto node_ref = IRM->create_ir_node(ssa_node_K, IR_schema);

   // TODO: use statements list of just created ssa_node should be always empty, shouldn't it?
   if(var && GetPointerS<ssa_node>(node_ref)->CGetUseStmts().empty())
   {
      THROW_ASSERT(var->get_kind() == variable_val_node_K || var->get_kind() == argument_val_node_K,
                   var->get_kind_text());
      IRSchema nop_stmt_IR_schema;
      if(var->get_kind() == variable_val_node_K && GetPointerS<const variable_val_node>(var)->parent)
      {
         nop_stmt_IR_schema[TOK(TOK_PARENT)] = STR(GetPointerS<const variable_val_node>(var)->parent->index);
      }
      if(var->get_kind() == argument_val_node_K)
      {
         nop_stmt_IR_schema[TOK(TOK_PARENT)] = STR(GetPointerS<const argument_val_node>(var)->parent->index);
      }
      nop_stmt_IR_schema[TOK(TOK_IR_LOCINFO)] = GetPointer<const IR_LocInfo>(var)->include_name + ":" +
                                                STR(GetPointer<const IR_LocInfo>(var)->line_number) + ":" +
                                                STR(GetPointer<const IR_LocInfo>(var)->column_number);
      const auto nop_stmt_node_ref = IRM->create_ir_node(nop_stmt_K, nop_stmt_IR_schema);
      GetPointerS<ssa_node>(node_ref)->SetDefStmt(nop_stmt_node_ref);
   }

   const auto sn = GetPointerS<ssa_node>(node_ref);
   sn->virtual_flag = virtual_flag;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + node_ref->ToString());
   return node_ref;
}

/// ASSIGN_STMT

/// Create a assign_stmt
ir_nodeRef ir_manipulation::create_assign_stmt(const ir_nodeRef& op0, const ir_nodeRef& op1,
                                               unsigned int function_decl_nid, const std::string& loc_info) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");

   IRSchema IR_schema;
   if(op1->get_kind() == call_node_K)
   {
      IR_schema[TOK(TOK_PREDICATE)] = STR(IRM->CreateUniqueIntegerCst(1, GetBooleanType())->index);
   }
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   IR_schema[TOK(TOK_OP0)] = STR(op0->index);
   IR_schema[TOK(TOK_OP1)] = STR(op1->index);
   IR_schema[TOK(TOK_PARENT)] = STR(function_decl_nid);
   // TODO: function decl arg should be a ir_nodeRef
   THROW_ASSERT(!function_decl_nid || IRM->GetIRNode(function_decl_nid),
                "Function must exist when statement is created.");
   const auto node_ref = IRM->create_ir_node(assign_stmt_K, IR_schema);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created node " + STR(node_ref));

   THROW_ASSERT(op0->index == 0 || op1->index == 0 ||
                    ir_helper::Size(ir_helper::CGetType(op0)) == ir_helper::Size(ir_helper::CGetType(op1)),
                "unexpected pattern - " + node_ref->ToString() + " (lhs = <" +
                    STR(ir_helper::Size(ir_helper::CGetType(op0))) + ">(" + STR(ir_helper::CGetType(op0)) +
                    "), rhs = <" + STR(ir_helper::Size(ir_helper::CGetType(op1))) + ">(" +
                    STR(ir_helper::CGetType(op1)) + "))");
   return node_ref;
}

ir_nodeRef ir_manipulation::CreateAssignStmt(const ir_nodeConstRef& type, const ir_nodeConstRef& min,
                                             const ir_nodeConstRef& max, const ir_nodeRef& op,
                                             unsigned int function_decl_nid, const std::string& loc_info) const
{
   const auto ssa_vd = create_ssa_name(ir_nodeConstRef(), type, min, max);
   const auto ga = create_assign_stmt(ssa_vd, op, function_decl_nid, loc_info);
   GetPointerS<ssa_node>(ssa_vd)->SetDefStmt(ga);
   return ga;
}

/// CALL_STMT
ir_nodeRef ir_manipulation::create_call_stmt(const ir_nodeConstRef& called_function,
                                             const std::vector<ir_nodeRef>& args, unsigned int function_decl_nid,
                                             const std::string& loc_info) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");
   IRSchema ae_IR_schema, gc_IR_schema;

   const auto function_ty_node = ir_helper::CGetType(called_function);
#if HAVE_ASSERTS
   const auto formal_count = GetPointer<const function_val_node>(called_function)->list_of_args.size();
   THROW_ASSERT(formal_count == args.size(), "Formal parameters count different from actual parameters count: " +
                                                 STR(formal_count) + " != " + STR(args.size()));
#endif
   ae_IR_schema[TOK(TOK_OP)] = STR(called_function->index);
   ae_IR_schema[TOK(TOK_TYPE)] = STR(GetPointerType(function_ty_node)->index);
   ae_IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;

   std::string args_string;
   for(const auto& arg : args)
   {
      if(!args_string.empty())
      {
         args_string += "_";
      }
      args_string += STR(arg->index);
   }

   const auto ae = IRM->create_ir_node(addr_node_K, ae_IR_schema);
   gc_IR_schema[TOK(TOK_ARG)] = args_string;
   gc_IR_schema[TOK(TOK_FN)] = STR(ae->index);
   gc_IR_schema[TOK(TOK_TYPE)] = STR(function_ty_node->index);
   gc_IR_schema[TOK(TOK_PREDICATE)] = STR(IRM->CreateUniqueIntegerCst(1, GetBooleanType())->index);
   gc_IR_schema[TOK(TOK_PARENT)] = STR(function_decl_nid);
   // TODO: function decl arg should be a ir_nodeRef
   THROW_ASSERT(!function_decl_nid || IRM->GetIRNode(function_decl_nid),
                "Function must exist when statement is created.");
   gc_IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   const auto node_ref = IRM->create_ir_node(call_stmt_K, gc_IR_schema);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created node " + STR(node_ref));
   return node_ref;
}

/// Create return_stmt
ir_nodeRef ir_manipulation::create_return_stmt(const ir_nodeConstRef& type, const ir_nodeConstRef& expr,
                                               unsigned int function_decl_nid, const std::string& loc_info) const
{
   THROW_ASSERT(!loc_info.empty(), "It requires a non empty string");

   IRSchema IR_schema;
   if(type)
   {
      IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   }
   else
   {
      IR_schema[TOK(TOK_TYPE)] = STR(GetVoidType()->index);
   }
   if(expr)
   {
      IR_schema[TOK(TOK_OP)] = STR(expr->index);
   }
   IR_schema[TOK(TOK_PARENT)] = STR(function_decl_nid);
   // TODO: function decl arg should be a ir_nodeRef
   THROW_ASSERT(!function_decl_nid || IRM->GetIRNode(function_decl_nid),
                "Function must exist when statement is created.");
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;

   const auto node_ref = IRM->create_ir_node(return_stmt_K, IR_schema);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(node_ref->index) + " (" + node_ref->get_kind_text() + ")");
   return node_ref;
}

/// PHI_STMT

/// Create a phi_stmt
ir_nodeRef ir_manipulation::create_phi_node(ir_nodeRef& ssa_res,
                                            const std::vector<std::pair<ir_nodeRef, unsigned int>>& list_of_def_edge,
                                            unsigned int function_decl_nid, bool virtual_flag) const
{
   auto iterator = list_of_def_edge.begin();
   ir_nodeRef ssa_ref = iterator->first;
   auto* sn_ref = GetPointer<ssa_node>(ssa_ref);
   for(++iterator; iterator != list_of_def_edge.end(); ++iterator)
   {
      ir_nodeRef tn = iterator->first;
      if(!sn_ref && GetPointer<ssa_node>(tn))
      {
         ssa_ref = tn;
         sn_ref = GetPointer<ssa_node>(ssa_ref);
      }
   }
   if(sn_ref)
   {
      ssa_res = create_ssa_name(sn_ref->var, sn_ref->type, sn_ref->min, sn_ref->max, virtual_flag);
   }
   else
   {
      const auto ssa_res_type_node = ir_helper::CGetType(list_of_def_edge.begin()->first);
      ssa_res = create_ssa_name(ir_nodeRef(), ssa_res_type_node, ir_nodeRef(), ir_nodeRef(), virtual_flag);
   }

   // Create the phi_stmt
   IRSchema IR_schema;
   IR_schema[TOK(TOK_RES)] = STR(ssa_res->index);
   IR_schema[TOK(TOK_PARENT)] = STR(function_decl_nid);
   // TODO: function decl arg should be a ir_nodeRef
   THROW_ASSERT(!function_decl_nid || IRM->GetIRNode(function_decl_nid),
                "Function must exist when statement is created.");
   IR_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   const auto phi_tn = IRM->create_ir_node(phi_stmt_K, IR_schema);

   auto* pn = GetPointerS<phi_stmt>(phi_tn);
   pn->virtual_flag = virtual_flag;

   for(const auto& def_edge : list_of_def_edge)
   {
      THROW_ASSERT(ir_helper::Size(ir_helper::CGetType(ssa_res)) ==
                       ir_helper::Size(ir_helper::CGetType(def_edge.first)),
                   "unexpected condition - lhs = <" + STR(ir_helper::Size(ir_helper::CGetType(ssa_res))) + ">" +
                       STR(ir_helper::CGetType(ssa_res)) + ", rhs = <" +
                       STR(ir_helper::Size(ir_helper::CGetType(def_edge.first))) + ">" +
                       STR(ir_helper::CGetType(def_edge.first)));
      pn->AddDefEdge(IRM, def_edge);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Created node " + STR(phi_tn->index) + " (" + phi_tn->get_kind_text() + ")");
   return phi_tn;
}

ir_nodeRef ir_manipulation::create_function_decl(const std::string& function_name, const ir_nodeRef& parent,
                                                 const std::vector<ir_nodeConstRef>& argsT,
                                                 const ir_nodeConstRef& returnType, const std::string& loc_info,
                                                 bool with_body) const
{
   auto fd_node = IRM->GetFunction(function_name);
   if(fd_node)
   {
      return fd_node;
   }

   IRSchema IR_schema;

   unsigned int function_name_id = IRM->find_identifier_nodeID(function_name);
   if(!function_name_id)
   {
      IR_schema[TOK(TOK_STRG)] = function_name;
      function_name_id = IRM->create_ir_node(identifier_node_K, IR_schema)->index;
   }
   else
   {
      THROW_ERROR("identifier in use by someone else");
   }
   IR_schema.clear();

   const auto function_ty_node = GetFunctionType(returnType, argsT);
   IR_schema[TOK(TOK_NAME)] = STR(function_name_id);
   IR_schema[TOK(TOK_TYPE)] = STR(function_ty_node->index);
   IR_schema[TOK(TOK_PARENT)] = STR(parent->index);
   IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   if(with_body)
   {
      auto sl_node = IRM->create_ir_node(statement_list_node_K, {});
      IR_schema[TOK(TOK_BODY)] = STR(sl_node->index);
   }
   fd_node = IRM->create_ir_node(function_val_node_K, IR_schema);
   IR_schema.clear();

   /// add argument_val_node to the function_val_node
   auto fd = GetPointerS<function_val_node>(fd_node);
   unsigned int Pindex = 0;
   for(const auto& par_type : argsT)
   {
      const auto p_name = "_P" + STR(Pindex);
      const auto p_identifier = create_identifier_node(p_name);
      const auto p_decl = create_parm_decl(p_identifier, par_type, fd_node, ir_nodeRef(), loc_info, false);
      fd->AddArg(p_decl);
      ++Pindex;
   }
   return fd_node;
}

ir_nodeRef ir_manipulation::CreateOrExpr(const ir_nodeConstRef& lhs, const ir_nodeConstRef& rhs, const blocRef& block,
                                         unsigned int function_decl_nid) const
{
   /// Create the or expr
   const auto bt = GetBooleanType();
   IRSchema boe_schema;
   boe_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   boe_schema[TOK(TOK_TYPE)] = STR(bt->index);
   boe_schema[TOK(TOK_OP0)] = STR(lhs->index);
   boe_schema[TOK(TOK_OP1)] = STR(rhs->index);
   const auto boe = IRM->create_ir_node(or_node_K, boe_schema);

   auto ga = CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt), boe,
                              function_decl_nid, BUILTIN_LOCINFO);
   if(block)
   {
      block->PushBack(ga, AppM);
   }
   return GetPointerS<const assign_stmt>(ga)->op0;
}

ir_nodeRef ir_manipulation::CreateAndExpr(const ir_nodeConstRef& lhs, const ir_nodeConstRef& rhs, const blocRef& block,
                                          unsigned int function_decl_nid) const
{
   /// Create the and expr
   const auto bt = GetBooleanType();
   IRSchema bae_schema;
   bae_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   bae_schema[TOK(TOK_TYPE)] = STR(bt->index);
   bae_schema[TOK(TOK_OP0)] = STR(lhs->index);
   bae_schema[TOK(TOK_OP1)] = STR(rhs->index);
   const auto bae = IRM->create_ir_node(and_node_K, bae_schema);

   auto ga = CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt), bae,
                              function_decl_nid, BUILTIN_LOCINFO);
   if(block)
   {
      block->PushBack(ga, AppM);
   }
   return GetPointerS<const assign_stmt>(ga)->op0;
}

ir_nodeRef ir_manipulation::CreateNotExpr(const ir_nodeConstRef& condition, const blocRef& block,
                                          unsigned int function_decl_nid) const
{
   /// Create the not expr
   const auto bt = GetBooleanType();
   IRSchema bne_schema;
   bne_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   bne_schema[TOK(TOK_TYPE)] = STR(bt->index);
   bne_schema[TOK(TOK_OP)] = STR(condition->index);
   const auto bne = IRM->create_ir_node(not_node_K, bne_schema);

   auto ga = CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt), bne,
                              function_decl_nid, BUILTIN_LOCINFO);
   if(block)
   {
      block->PushBack(ga, AppM);
   }
   return GetPointerS<const assign_stmt>(ga)->op0;
}

ir_nodeRef ir_manipulation::ExtractCondition(const ir_nodeRef& op0, const blocRef& block,
                                             unsigned int function_decl_nid, std::string include_name,
                                             unsigned line_number, unsigned column_number) const
{
   THROW_ASSERT(block, "expected basic block");

   if(ir_helper::IsBooleanType(op0) && (op0->get_kind() == ssa_node_K || ir_helper::IsConstant(op0)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition already available as " + op0->ToString());
      return op0;
   }
   else
   {
      const auto bt = GetBooleanType();
      const auto gc_locinfo = include_name + ":" + STR(line_number) + ":" + STR(column_number);
      ir_nodeRef ret = nullptr;
      if(ir_helper::IsBooleanType(op0))
      {
         const auto ga = CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt),
                                          op0, function_decl_nid, gc_locinfo);
         block->PushBack(ga, AppM);
         ret = GetPointerS<const assign_stmt>(ga)->op0;
      }
      else if(op0->get_kind() == constant_int_val_node_K)
      {
         const auto cst_val = ir_helper::GetConstValue(op0);
         ret = IRM->CreateUniqueIntegerCst(cst_val ? 1 : 0, bt);
      }
      else
      {
         const auto constNE0 = IRM->CreateUniqueIntegerCst(0, ir_helper::CGetType(op0));
         const auto cond_op0 = create_binary_operation(bt, op0, constNE0, gc_locinfo, ne_node_K);
         const auto op0_ga =
             CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt), cond_op0,
                              function_decl_nid, gc_locinfo);
         block->PushBack(op0_ga, AppM);
         ret = GetPointerS<const assign_stmt>(op0_ga)->op0;
      }
      THROW_ASSERT(ret, "");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition created is " + ret->ToString());
      return ret;
   }
}

ir_nodeRef ir_manipulation::CreateNopExpr(const ir_nodeConstRef& operand, const ir_nodeConstRef& type,
                                          const ir_nodeConstRef& min, const ir_nodeConstRef& max,
                                          unsigned int function_decl_nid) const
{
   IRSchema ne_schema;
   ne_schema[TOK(TOK_TYPE)] = STR(type->index);
   ne_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   ne_schema[TOK(TOK_OP)] = STR(operand->index);
   const auto ne = IRM->create_ir_node(nop_node_K, ne_schema);

   if(operand->get_kind() != ssa_node_K && operand->get_kind() != constant_int_val_node_K)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// IR node in release
      THROW_UNREACHABLE("Cannot create nop expr from something that is not a ssa: " + operand->ToString());
      return ir_nodeRef();
   }

   const auto ga = CreateAssignStmt(type, min, max, ne, function_decl_nid, BUILTIN_LOCINFO);
   if(operand->get_kind() == ssa_node_K)
   {
      GetPointerS<ssa_node>(GetPointerS<assign_stmt>(ga)->op0)->use_set = GetPointerS<const ssa_node>(operand)->use_set;
   }
   return ga;
}

ir_nodeRef ir_manipulation::CreateUnsigned(const ir_nodeConstRef& signed_type) const
{
   THROW_ASSERT(signed_type->get_kind() == integer_ty_node_K, signed_type->ToString() + " is not an integer type");

   const auto int_signed_type = GetPointerS<const integer_ty_node>(signed_type);
   THROW_ASSERT(!int_signed_type->unsigned_flag, signed_type->ToString() + " is not signed");
   return GetParametricIntegerType(int_signed_type->algn, int_signed_type->bitsize, true,
                                   int_signed_type->bitsizealloc);
}

ir_nodeRef ir_manipulation::CreateEqExpr(const ir_nodeConstRef& lhs, const ir_nodeConstRef& rhs, const blocRef& block,
                                         unsigned int function_decl_nid) const
{
   IRSchema eq_node_schema;
   /// Create the eq expr
   const auto bt = GetBooleanType();
   eq_node_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   eq_node_schema[TOK(TOK_TYPE)] = STR(bt->index);
   eq_node_schema[TOK(TOK_OP0)] = STR(lhs->index);
   eq_node_schema[TOK(TOK_OP1)] = STR(rhs->index);
   const auto eq = IRM->create_ir_node(eq_node_K, eq_node_schema);

   auto ga = CreateAssignStmt(bt, IRM->CreateUniqueIntegerCst(0, bt), IRM->CreateUniqueIntegerCst(1, bt), eq,
                              function_decl_nid, BUILTIN_LOCINFO);
   if(block)
   {
      block->PushBack(ga, AppM);
   }
   return GetPointerS<const assign_stmt>(ga)->op0;
}

ir_nodeRef ir_manipulation::CreateCallExpr(const ir_nodeConstRef& called_function, const std::vector<ir_nodeRef>& args,
                                           const std::string& loc_info) const
{
   IRSchema ae_IR_schema, ce_IR_schema;
   ae_IR_schema[TOK(TOK_OP)] = STR(called_function->index);
   const auto ft = ir_helper::CGetType(called_function);
   ae_IR_schema[TOK(TOK_TYPE)] = STR(GetPointerType(ft)->index);
   ae_IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   auto ae_node = IRM->create_ir_node(addr_node_K, ae_IR_schema);

   std::string args_string;
   for(const auto& arg : args)
   {
      if(!args_string.empty())
      {
         args_string += "_";
      }
      args_string += STR(arg->index);
   }
   ce_IR_schema[TOK(TOK_ARG)] = args_string;
   ce_IR_schema[TOK(TOK_FN)] = STR(ae_node->index);
   ce_IR_schema[TOK(TOK_TYPE)] = STR(ir_helper::GetFunctionReturnType(called_function, false)->index);
   ce_IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   return IRM->create_ir_node(call_node_K, ce_IR_schema);
}

ir_nodeRef ir_manipulation::CreateAddrExpr(const ir_nodeConstRef& tn, const std::string& loc_info) const
{
   IRSchema ae_IR_schema;
   const auto type_node = ir_helper::CGetType(tn);
   ae_IR_schema[TOK(TOK_OP)] = STR(tn->index);
   auto align = 8u;
   if(tn->get_kind() == variable_val_node_K)
   {
      auto vd = GetPointer<const variable_val_node>(tn);
      align = vd->algn;
   }
   const auto ptr_type = GetPointerType(type_node, align);
   ae_IR_schema[TOK(TOK_TYPE)] = STR(ptr_type->index);
   ae_IR_schema[TOK(TOK_IR_LOCINFO)] = loc_info;
   return IRM->create_ir_node(addr_node_K, ae_IR_schema);
}

ir_nodeRef ir_manipulation::CreateAssignStmtAddrExpr(const ir_nodeConstRef& tn, unsigned int function_decl_nid,
                                                     const std::string& loc_info) const
{
   auto addr_tn = CreateAddrExpr(tn, loc_info);
   const auto ptr_type = GetPointerS<addr_node>(addr_tn)->type;
   auto assign_node = CreateAssignStmt(ptr_type, ir_nodeRef(), ir_nodeRef(), addr_tn, function_decl_nid, loc_info);
   auto ga = GetPointerS<assign_stmt>(assign_node);
   auto ssa = GetPointerS<ssa_node>(ga->op0);
   ssa->use_set.Add(IRM->GetIRNode(tn->index));
   return assign_node;
}

ir_nodeRef ir_manipulation::CreateVectorType(const ir_nodeConstRef& elt_type, integer_cst_t number_of_elements) const
{
   const auto bitsizealloc = number_of_elements * ir_helper::SizeAlloc(elt_type);

   IRSchema IR_schema;
   IR_schema[TOK(TOK_ELTS)] = STR(elt_type->index);
   IR_schema[TOK(TOK_BITSIZEALLOC)] = STR(bitsizealloc);

   auto vector_type_id = IRM->find0(vector_ty_node_K, IR_schema);

   /// not_found decl
   if(vector_type_id == 0)
   {
      IR_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
      return IRM->create_ir_node(vector_ty_node_K, IR_schema);
   }
   return IRM->GetIRNode(vector_type_id);
}

ir_nodeRef ir_manipulation::CloneFunction(const ir_nodeRef& tn, const std::string& fsuffix)
{
   THROW_ASSERT(tn->get_kind() == function_val_node_K, "Type node is not a function_val_node");
   const auto fd = GetPointerS<const function_val_node>(tn);
   THROW_ASSERT(fd->name->get_kind() == identifier_node_K, "operator based function not supported ");
   const auto fname = ir_helper::GetFunctionName(tn);
   const auto fu_node = IRM->GetFunction(fname + fsuffix);
   if(fu_node)
   {
      return fu_node;
   }
   const auto clone_fname = create_identifier_node(fname + fsuffix);
   CustomUnorderedMapStable<unsigned int, unsigned int> remapping;
   ir_node_dup tnd(remapping, AppM);
   remapping[fd->name->index] = clone_fname->index;
   if(fd->mngl)
   {
      const auto clone_mngl = create_identifier_node(fname + fsuffix);
      remapping[fd->mngl->index] = clone_mngl->index;
   }
   const auto clone_fd = tnd.create_ir_node(tn, ir_node_dup_mode::FUNCTION);
   return IRM->GetIRNode(clone_fd);
}

unsigned int ir_manipulation::InlineFunctionCall(const ir_nodeRef& call_tn, const ir_nodeRef& caller_node)
{
   auto [fn, args, retval] = [&]() -> std::tuple<ir_nodeRef, std::vector<ir_nodeRef>, ir_nodeRef> {
      if(call_tn->get_kind() == call_stmt_K)
      {
         const auto gc = GetPointerS<const call_stmt>(call_tn);
         return {gc->fn, gc->args, nullptr};
      }
      if(call_tn->get_kind() == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(call_tn);
         THROW_ASSERT(ga->op1->get_kind() == call_node_K, "unexpected condition");
         const auto ce = GetPointerS<const call_node>(ga->op1);
         return {ce->fn, ce->args, ga->op0};
      }
      THROW_UNREACHABLE("Unsupported call statement: " + call_tn->get_kind_text() + " " + STR(call_tn));
      return {nullptr, std::vector<ir_nodeRef>(), nullptr};
   }();
   if(fn->get_kind() == addr_node_K)
   {
      fn = GetPointerS<const unary_node>(fn)->op;
   }
   THROW_ASSERT(fn->get_kind() == function_val_node_K, "Call statement should address a function declaration");

   const auto fd = GetPointer<function_val_node>(caller_node);
   auto sl = GetPointerS<statement_list_node>(fd->body);
   const auto& block = sl->list_of_bloc.at(GetPointer<const node_stmt>(call_tn)->bb_index);
   const auto splitBBI = sl->list_of_bloc.rbegin()->first + 1;
   THROW_ASSERT(!sl->list_of_bloc.count(splitBBI), "");
   const auto splitBB = sl->list_of_bloc[splitBBI] = blocRef(new bloc(splitBBI));
   splitBB->loop_id = block->loop_id;
   splitBB->SetSSAUsesComputed();
   THROW_ASSERT(!block->schedule, "Inlining should not be allowed after scheduling");

   std::replace(block->list_of_pred.begin(), block->list_of_pred.end(), block->number, splitBB->number);
   splitBB->list_of_succ.assign(block->list_of_succ.cbegin(), block->list_of_succ.cend());
   block->list_of_succ.clear();

   for(const auto& bbi : splitBB->list_of_succ)
   {
      THROW_ASSERT(sl->list_of_bloc.count(bbi), "");
      const auto& bb = sl->list_of_bloc.at(bbi);
      for(const auto& phi : bb->CGetPhiList())
      {
         auto gp = GetPointerS<phi_stmt>(phi);
         const auto defFrom = std::find_if(gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(),
                                           [&](const phi_stmt::DefEdge& de) { return de.second == block->number; });
         if(defFrom != gp->CGetDefEdgesList().end())
         {
            gp->ReplaceDefEdge(IRM, *defFrom, {defFrom->first, splitBBI});
         }
      }
   }
   {
      auto it = std::find_if(block->CGetStmtList().begin(), block->CGetStmtList().end(),
                             [&](const ir_nodeRef& tn) { return tn->index == call_tn->index; });
      THROW_ASSERT(it != block->CGetStmtList().end(), "");
      ++it;
      while(it != block->CGetStmtList().end())
      {
         const auto mv_stmt = *it;
         ++it;
         block->RemoveStmt(mv_stmt, AppM);
         splitBB->PushBack(mv_stmt, AppM);
      }
      block->RemoveStmt(call_tn, AppM);
   }

   const auto max_loop_id = [&]() {
      unsigned int mlid = 0;
      for(const auto& ibb : sl->list_of_bloc)
      {
         mlid = std::max(mlid, ibb.second->loop_id);
         std::replace(ibb.second->list_of_pred.begin(), ibb.second->list_of_pred.end(), block->number, splitBB->number);
      }
      return mlid;
   }();
   const auto inline_fd = GetPointerS<const function_val_node>(fn);
   auto output_level = parameters->getOption<int>(OPT_output_level);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "Function call to " + ir_helper::GetFunctionName(fn) + " inlined in " +
                      ir_helper::GetFunctionName(caller_node));

   CustomUnorderedMapStable<unsigned int, unsigned int> remapping;

   // Remap inlined function declaration with caller function declaration
   remapping.insert({inline_fd->index, fd->index});

   // Remap param ssa with actual inlined call arguments
   for(size_t i = 0; i < args.size(); ++i)
   {
      const auto& parm = inline_fd->list_of_args.at(i);
      const auto parm_ssa_idx = AppM->getSSAFromParm(inline_fd->index, parm->index);
      THROW_ASSERT(parm_ssa_idx, "unexpected condition");
      remapping.insert({parm_ssa_idx, args.at(i)->index});
   }

   ir_node_dup tnd(remapping, AppM, splitBBI + 1, max_loop_id + 1, true);
   const auto dup_sl_id = tnd.create_ir_node(inline_fd->body, ir_node_dup_mode::RENAME);
   const auto dup_sl = GetPointer<const statement_list_node>(IRM->GetIRNode(dup_sl_id));
   THROW_ASSERT(dup_sl, "");

   std::vector<std::pair<ir_nodeRef, unsigned int>> list_of_def_edge;
   for(const auto& ibb : dup_sl->list_of_bloc)
   {
      if(ibb.first == bloc::ENTRY_BLOCK_ID || ibb.first == bloc::EXIT_BLOCK_ID)
      {
         continue;
      }
      auto& bb = ibb.second;
      sl->add_bloc(bb);
      for(auto it = bb->list_of_pred.begin(); it != bb->list_of_pred.end(); ++it)
      {
         if(*it == bloc::ENTRY_BLOCK_ID)
         {
            *it = block->number;
            block->list_of_succ.push_back(bb->number);
         }
      }
      for(auto it = bb->list_of_succ.begin(); it != bb->list_of_succ.end(); ++it)
      {
         const auto has_abort_call = [&]() -> bool {
            if(!bb->CGetStmtList().empty())
            {
               const auto& last_stmt = bb->CGetStmtList().back();
               if(last_stmt->get_kind() == call_stmt_K)
               {
                  const auto gc = GetPointerS<const call_stmt>(last_stmt);
                  auto call_fd = gc->fn;
                  const auto ae = GetPointer<addr_node>(call_fd);
                  if(ae)
                  {
                     call_fd = ae->op;
                  }
                  const auto fu_name = ir_helper::GetFunctionName(call_fd);
                  if(fu_name == "abort" || fu_name == "exit")
                  {
                     return true;
                  }
               }
            }
            return false;
         }();
         if(*it == bloc::EXIT_BLOCK_ID && !has_abort_call)
         {
            *it = splitBB->number;
            splitBB->list_of_pred.push_back(bb->number);
         }
      }
      for(auto it = bb->CGetStmtList().begin(); it != bb->CGetStmtList().end();)
      {
         const auto stmt = *it;
         ++it;

         if(stmt->get_kind() == return_stmt_K)
         {
            if(retval)
            {
               const auto gr = GetPointerS<const return_stmt>(stmt);
               THROW_ASSERT(gr->op, "");
               list_of_def_edge.push_back(std::make_pair(gr->op, bb->number));
            }
            bb->RemoveStmt(stmt, AppM);
         }
      }
   }
   THROW_ASSERT(block->list_of_succ.size() == 1, "There should be only one entry point.");
   if(retval)
   {
      THROW_ASSERT(!list_of_def_edge.empty(), "unexpected condition");
      ir_nodeRef phi_res;
      const auto ret_phi = create_phi_node(phi_res, list_of_def_edge, fd->index);
      auto* gp = GetPointerS<phi_stmt>(ret_phi);
      gp->artificial = true;
      gp->SetSSAUsesComputed();
      splitBB->AddPhi(ret_phi);
      IRM->ReplaceIRNode(ret_phi, phi_res, retval);
   }
   if(splitBB->list_of_pred.empty())
   {
      THROW_ASSERT(splitBB->CGetStmtList().empty() && splitBB->CGetPhiList().empty(),
                   "Unreachable BB after inlined call statement must be empty.");
      for(const auto& succ_bbi : splitBB->list_of_succ)
      {
         const auto& succ_bb = sl->list_of_bloc[succ_bbi];
         const auto new_end = std::remove(succ_bb->list_of_pred.begin(), succ_bb->list_of_pred.end(), splitBB->number);
         succ_bb->list_of_pred.erase(new_end, succ_bb->list_of_pred.end());
      }
      sl->list_of_bloc.erase(splitBB->number);
   }
   return splitBB->number;
}

bool ir_manipulation::VersionFunctionCall(const ir_nodeRef& call_tn, const ir_nodeRef& caller_node,
                                          const std::string& version_suffix)
{
   auto [called_fn, args, retval] = [&]() -> std::tuple<ir_nodeRef, std::vector<ir_nodeRef>, ir_nodeRef> {
      if(call_tn->get_kind() == call_stmt_K)
      {
         const auto gc = GetPointerS<const call_stmt>(call_tn);
         return {gc->fn, gc->args, nullptr};
      }
      if(call_tn->get_kind() == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(call_tn);
         THROW_ASSERT(ga->op1->get_kind() == call_node_K, "unexpected condition");
         const auto ce = GetPointerS<const call_node>(ga->op1);
         return {ce->fn, ce->args, ga->op0};
      }
      THROW_UNREACHABLE("Unsupported call statement: " + call_tn->get_kind_text() + " " + STR(call_tn));
      return {nullptr, std::vector<ir_nodeRef>(), nullptr};
   }();
   if(called_fn->get_kind() == addr_node_K)
   {
      called_fn = GetPointerS<const unary_node>(called_fn)->op;
   }
   THROW_ASSERT(called_fn->get_kind() == function_val_node_K, "Call statement should address a function declaration");

   if(ends_with(ir_helper::GetFunctionName(called_fn), version_suffix))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call already versioned...");
      return false;
   }
   const auto version_fn = CloneFunction(called_fn, version_suffix);
   auto output_level = parameters->getOption<int>(OPT_output_level);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "Function call " + call_tn->ToString() + " versioned in " + ir_helper::GetFunctionName(caller_node));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call before versioning : " + STR(call_tn));
   const auto caller_id = caller_node->index;
   if(retval)
   {
      const auto has_args = !GetPointer<const function_val_node>(version_fn)->list_of_args.empty();
      const auto ce = CreateCallExpr(version_fn, has_args ? args : std::vector<ir_nodeRef>(), BUILTIN_LOCINFO);
      const auto ga = GetPointerS<const assign_stmt>(call_tn);
      CustomUnorderedSet<unsigned int> already_visited;
      AppM->GetCallGraphManager().RemoveCallPoint(caller_id, called_fn->index, call_tn->index);
      IRM->ReplaceIRNode(call_tn, ga->op1, ce);
      CallGraphManager::addCallPointAndExpand(already_visited, AppM, caller_id, version_fn->index, call_tn->index,
                                              FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call after versioning  : " + STR(call_tn));
   }
   else
   {
      const auto version_call = create_call_stmt(version_fn, args, caller_id, BUILTIN_LOCINFO);
      const auto caller_fd = GetPointer<function_val_node>(caller_node);
      const auto call_bbi = GetPointer<node_stmt>(call_tn)->bb_index;
      const auto& call_bb = GetPointer<statement_list_node>(caller_fd->body)->list_of_bloc.at(call_bbi);
      call_bb->Replace(call_tn, version_call, true, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call after versioning  : " + STR(version_call));
   }
   return true;
}
