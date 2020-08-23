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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file tree_manipulation.cpp
 * @brief Class implementing some useful functions to create tree nodes and to
 * manipulate the tree manager.
 *
 * This class implements some useful functions to create tree nodes and to
 * manipulate the tree manager.
 *
 * @author ste <stefano.viazzi@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "tree_manipulation.hpp"

#include "config_HAVE_HEXFLOAT.hpp"

#include <algorithm>              // for find
#include <boost/lexical_cast.hpp> // for lexical_cast
#include <boost/range/adaptor/reversed.hpp>
#include <boost/smart_ptr/shared_ptr.hpp> // for shared_ptr

#include "Parameter.hpp"           // for Parameter
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_VERY_PEDANTIC
#include "exceptions.hpp"          // for THROW_ASSERT, THROW_ERROR
#include "string_manipulation.hpp" // for STR GET_CLASS

/// tree includes
#include "ext_tree_node.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include <iostream>
#if !HAVE_HEXFLOAT
#include <cstdio>
#endif

unsigned int tree_manipulation::goto_label_unique_id = 0;

#define TREE_NOT_YET_IMPLEMENTED(token) THROW_ERROR(std::string("field not yet supported ") + STOK(token))

/// Constructor
tree_manipulation::tree_manipulation(const tree_managerRef& _TreeM, const ParameterConstRef& _parameters)
    : TreeM(_TreeM), reuse(_parameters->IsParameter("reuse_gimple") ? _parameters->GetParameter<bool>("reuse_gimple") : true), parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}
tree_manipulation::tree_manipulation(const tree_managerRef& _TreeM, const ParameterConstRef& _parameters, bool _reuse) : TreeM(_TreeM), reuse(_reuse), parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

/// Destructor
tree_manipulation::~tree_manipulation() = default;

/// EXPRESSION_TREE_NODES

/// TODO weight_node to fix in tree_node_factory.cpp
/// Create an unary operation
tree_nodeRef tree_manipulation::create_unary_operation(const tree_nodeRef& type, const tree_nodeRef& op, const std::string& srcp, kind operation_kind) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");
   THROW_ASSERT(op->get_kind() == tree_reindex_K, "Operator node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   /// Check if the tree_node type is a unary expression
   switch(operation_kind)
   {
      case CASE_UNARY_EXPRESSION:
      {
         break;
      }

      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR("The operation given is not a unary expression");
   }

   /// Check if it is a correct node type
   switch(GET_NODE(type)->get_kind())
   {
      case array_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case enumeral_type_K:
      case function_type_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case typename_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(GET_INDEX_NODE(type)) + std::string("): ") + GET_NODE(type)->get_kind_text());
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(type));
   IR_schema[TOK(TOK_OP)] = STR(GET_INDEX_NODE(op));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, operation_kind, IR_schema);
   tree_nodeRef return_tree_reindex = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(return_tree_reindex)) + " (" + GET_NODE(return_tree_reindex)->get_kind_text() + ")");

   return return_tree_reindex;
}

/// TODO weight_node to fix in tree_node_factory.cpp
/// Create a binary expression
tree_nodeRef tree_manipulation::create_binary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp, kind operation_kind) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Operator 0 node is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Operator 1 node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   /// Check if the tree_node type is a binary expression
   switch(operation_kind)
   {
      case CASE_BINARY_EXPRESSION:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR("The operation given is not a binary expression");
   }

   /// Check if it is a correct node type
   switch(GET_NODE(type)->get_kind())
   {
      case array_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case enumeral_type_K:
      case function_type_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(GET_INDEX_NODE(type)) + std::string("): ") + GET_NODE(type)->get_kind_text());
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(type));
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, operation_kind, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// TODO weight_node to fix in tree_node_factory.cpp
/// Create a ternary expression
tree_nodeRef tree_manipulation::create_ternary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const std::string& srcp, kind operation_kind) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type is not a tree reindex");
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Operand 0 is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Operand 1 is not a tree reindex");
   THROW_ASSERT(op2->get_kind() == tree_reindex_K, "Operand 3 is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   /// Check if the tree_node type is a ternary expression
   switch(operation_kind)
   {
      case CASE_TERNARY_EXPRESSION:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR("The operation given is not a ternary expression");
   }

   /// Check if it is a correct node type
   switch(GET_NODE(type)->get_kind())
   {
      case array_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case enumeral_type_K:
      case function_type_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(GET_INDEX_NODE(type)) + std::string("): ") + GET_NODE(type)->get_kind_text());
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(type));
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   IR_schema[TOK(TOK_OP2)] = STR(GET_INDEX_NODE(op2));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, operation_kind, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// TODO weight_node to fix in tree_node_factory.cpp
/// Create a quaternary expression
tree_nodeRef tree_manipulation::create_quaternary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const tree_nodeRef& op3, const std::string& srcp, kind operation_kind) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Operand 0 node is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Operand 1 node is not a tree reindex");
   THROW_ASSERT(op2->get_kind() == tree_reindex_K, "Operand 2 node is not a tree reindex");
   THROW_ASSERT(op3->get_kind() == tree_reindex_K, "Operand 3 node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   /// Check if the tree_node type is a quaternary expression
   switch(operation_kind)
   {
      case CASE_QUATERNARY_EXPRESSION:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR("The operation given is not a quaternary expression");
   }

   switch(GET_NODE(type)->get_kind())
   {
      case array_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case enumeral_type_K:
      case function_type_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR(std::string("Type node not supported (") + STR(GET_INDEX_NODE(type)) + std::string("): ") + GET_NODE(type)->get_kind_text());
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(type));
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   IR_schema[TOK(TOK_OP2)] = STR(GET_INDEX_NODE(op2));
   IR_schema[TOK(TOK_OP3)] = STR(GET_INDEX_NODE(op3));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, operation_kind, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

tree_nodeRef tree_manipulation::create_lut_expr(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const tree_nodeRef& op3, const tree_nodeRef& op4, const tree_nodeRef& op5, const tree_nodeRef& op6,
                                                const tree_nodeRef& op7, const tree_nodeRef& op8, const std::string& srcp) const
{
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Operand 0 node is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Operand 1 node is not a tree reindex");
   THROW_ASSERT(!op2 || op2->get_kind() == tree_reindex_K, "Operand 2 node is not a tree reindex");
   THROW_ASSERT(!op3 || op3->get_kind() == tree_reindex_K, "Operand 3 node is not a tree reindex");
   THROW_ASSERT(!op4 || op4->get_kind() == tree_reindex_K, "Operand 4 node is not a tree reindex");
   THROW_ASSERT(!op5 || op5->get_kind() == tree_reindex_K, "Operand 5 node is not a tree reindex");
   THROW_ASSERT(!op6 || op6->get_kind() == tree_reindex_K, "Operand 6 node is not a tree reindex");
   THROW_ASSERT(!op7 || op7->get_kind() == tree_reindex_K, "Operand 7 node is not a tree reindex");
   THROW_ASSERT(!op8 || op8->get_kind() == tree_reindex_K, "Operand 8 node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(type));
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   if(op2)
      IR_schema[TOK(TOK_OP2)] = STR(GET_INDEX_NODE(op2));
   if(op3)
      IR_schema[TOK(TOK_OP3)] = STR(GET_INDEX_NODE(op3));
   if(op4)
      IR_schema[TOK(TOK_OP4)] = STR(GET_INDEX_NODE(op4));
   if(op5)
      IR_schema[TOK(TOK_OP5)] = STR(GET_INDEX_NODE(op5));
   if(op6)
      IR_schema[TOK(TOK_OP6)] = STR(GET_INDEX_NODE(op6));
   if(op7)
      IR_schema[TOK(TOK_OP7)] = STR(GET_INDEX_NODE(op7));
   if(op8)
      IR_schema[TOK(TOK_OP8)] = STR(GET_INDEX_NODE(op8));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, lut_expr_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

tree_nodeRef tree_manipulation::create_extract_bit_expr(const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp) const
{
   auto boolType = create_boolean_type();
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Operand 0 node is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Operand 1 node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(boolType));
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, extract_bit_expr_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}
/// CONST_OBJ_TREE_NODES

/// Create an integer_cst node
tree_nodeRef tree_manipulation::CreateIntegerCst(const tree_nodeConstRef& type, const long long int value, const unsigned int integer_cst_nid) const
{
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");

   unsigned int type_node_nid = type->index;
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type_node_nid);
   IR_schema[TOK(TOK_VALUE)] = STR(value);

   this->TreeM->create_tree_node(integer_cst_nid, integer_cst_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(integer_cst_nid);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

tree_nodeRef tree_manipulation::CreateRealCst(const tree_nodeConstRef& type, const long double value, const unsigned int real_cst_nid) const
{
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Type node is not a tree reindex");

   unsigned int type_node_nid = type->index;
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type_node_nid);

   std::stringstream ssX;
#if HAVE_HEXFLOAT
   ssX << std::hexfloat << value;
#else
   {
      char buffer[256];
      sprintf(buffer, "%La", value);
      ssX << buffer;
   }
#endif
   IR_schema[TOK(TOK_VALR)] = ssX.str();
   IR_schema[TOK(TOK_VALX)] = ssX.str();

   this->TreeM->create_tree_node(real_cst_nid, real_cst_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(real_cst_nid);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// IDENTIFIER_TREE_NODE

/// Create an identifier node
tree_nodeRef tree_manipulation::create_identifier_node(const std::string& strg) const
{
   THROW_ASSERT(!strg.empty(), "It requires a non empty string");

   ///@37     identifier_node  strg: "int" lngt: 3
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef node_ref;

   IR_schema[TOK(TOK_STRG)] = strg;
   unsigned int node_nid = this->TreeM->find(identifier_node_K, IR_schema);

   if(!node_nid)
   {
      node_nid = this->TreeM->new_tree_node_id();
      this->TreeM->create_tree_node(node_nid, identifier_node_K, IR_schema);
      node_ref = TreeM->GetTreeReindex(node_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + " " + strg + ")");
   }
   else
   {
      node_ref = TreeM->GetTreeReindex(node_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + " " + strg + ")");
   }
   return node_ref;
}

/// DECL_NODES

/// Create a var_decl
tree_nodeRef tree_manipulation::create_var_decl(const tree_nodeRef& name, const tree_nodeConstRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& smt_ann, const tree_nodeRef& init, const std::string& srcp, unsigned int algn,
                                                int used, bool artificial_flag, int use_tmpl, bool static_static_flag, bool extern_flag, bool static_flag, bool register_flag, bool readonly_flag, const std::string& bit_values, bool addr_taken,
                                                bool addr_not_taken) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(scpe->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(size->get_kind() == tree_reindex_K, "Node is not a tree reindex");

   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   unsigned int node_nid = this->TreeM->new_tree_node_id();

   unsigned int type_node_nid = GET_INDEX_CONST_NODE(type);
   unsigned int size_node_nid = GET_INDEX_NODE(size);
   unsigned int scpe_node_nid = GET_INDEX_NODE(scpe);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   if(name)
   {
      THROW_ASSERT(name->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int name_node_nid = GET_INDEX_NODE(name);
      IR_schema[TOK(TOK_NAME)] = STR(name_node_nid);
   }

   if(smt_ann)
   {
      THROW_ASSERT(smt_ann->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int smt_node_nid = GET_INDEX_NODE(smt_ann);
      IR_schema[TOK(TOK_SMT_ANN)] = STR(smt_node_nid);
   }

   if(init)
   {
      THROW_ASSERT(init->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int init_nid = GET_INDEX_NODE(init);
      IR_schema[TOK(TOK_INIT)] = STR(init_nid);
   }

   IR_schema[TOK(TOK_TYPE)] = STR(type_node_nid);
   IR_schema[TOK(TOK_SCPE)] = STR(scpe_node_nid);
   IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
   IR_schema[TOK(TOK_ALGN)] = STR(algn);
   IR_schema[TOK(TOK_USED)] = STR(used);
   IR_schema[TOK(TOK_SRCP)] = srcp;
   IR_schema[TOK(TOK_USE_TMPL)] = STR(use_tmpl);
   IR_schema[TOK(TOK_STATIC_STATIC)] = STR(static_static_flag);
   IR_schema[TOK(TOK_EXTERN)] = STR(extern_flag);
   IR_schema[TOK(TOK_STATIC)] = STR(static_flag);
   IR_schema[TOK(TOK_REGISTER)] = STR(register_flag);
   IR_schema[TOK(TOK_READONLY)] = STR(readonly_flag);
   IR_schema[TOK(TOK_BIT_VALUES)] = bit_values;
   IR_schema[TOK(TOK_ADDR_TAKEN)] = STR(addr_taken);
   IR_schema[TOK(TOK_ADDR_NOT_TAKEN)] = STR(addr_not_taken);
   IR_schema[TOK(TOK_ARTIFICIAL)] = STR(artificial_flag);

   this->TreeM->create_tree_node(node_nid, var_decl_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

tree_nodeRef tree_manipulation::create_translation_unit_decl() const
{
   tree_nodeRef translation_unit_decl_node;
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   unsigned int translation_unit_decl_nid = this->TreeM->find(translation_unit_decl_K, IR_schema);
   if(!translation_unit_decl_nid)
   {
      translation_unit_decl_nid = this->TreeM->new_tree_node_id();
      this->TreeM->create_tree_node(translation_unit_decl_nid, translation_unit_decl_K, IR_schema);
      translation_unit_decl_node = TreeM->GetTreeReindex(translation_unit_decl_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(translation_unit_decl_node)) + " (" + GET_NODE(translation_unit_decl_node)->get_kind_text() + ")");
   }
   else
   {
      translation_unit_decl_node = TreeM->GetTreeReindex(translation_unit_decl_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(translation_unit_decl_node)) + " (" + GET_NODE(translation_unit_decl_node)->get_kind_text() + ")");
   }
   return translation_unit_decl_node;
}

/// Create result_decl
tree_nodeRef tree_manipulation::create_result_decl(const tree_nodeRef& name, const tree_nodeRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& smt_ann, const tree_nodeRef& init, const std::string& srcp, unsigned int algn,
                                                   bool artificial_flag) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(scpe->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(size->get_kind() == tree_reindex_K, "Node is not a tree reindex");

   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   unsigned int node_nid = this->TreeM->new_tree_node_id();

   unsigned int type_node_nid = GET_INDEX_NODE(type);
   unsigned int size_node_nid = GET_INDEX_NODE(size);
   unsigned int scpe_node_nid = GET_INDEX_NODE(scpe);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   IR_schema[TOK(TOK_TYPE)] = STR(type_node_nid);
   IR_schema[TOK(TOK_SCPE)] = STR(scpe_node_nid);
   IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
   IR_schema[TOK(TOK_ALGN)] = STR(algn);
   IR_schema[TOK(TOK_SRCP)] = srcp;
   IR_schema[TOK(TOK_ARTIFICIAL)] = STR(artificial_flag);

   if(smt_ann)
   {
      THROW_ASSERT(smt_ann->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int smt_node_nid = GET_INDEX_NODE(smt_ann);
      IR_schema[TOK(TOK_SMT_ANN)] = STR(smt_node_nid);
   }

   if(name)
   {
      THROW_ASSERT(name->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int name_node_nid = GET_INDEX_NODE(name);
      IR_schema[TOK(TOK_NAME)] = STR(name_node_nid);
   }

   if(init)
   {
      THROW_ASSERT(init->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int init_nid = GET_INDEX_NODE(init);
      IR_schema[TOK(TOK_INIT)] = STR(init_nid);
   }

   this->TreeM->create_tree_node(node_nid, result_decl_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// Create parm_decl
tree_nodeRef tree_manipulation::create_parm_decl(const tree_nodeRef& name, const tree_nodeRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& argt, const tree_nodeRef& smt_ann, const tree_nodeRef& init,
                                                 const std::string& srcp, unsigned int algn, int used, bool register_flag, bool readonly_flag) const
{
   /// Check if the tree_node given are tree_reindex
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(scpe->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(size->get_kind() == tree_reindex_K, "Node is not a tree reindex");

   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   unsigned int node_nid = this->TreeM->new_tree_node_id();

   unsigned int type_node_nid = GET_INDEX_NODE(type);
   unsigned int size_node_nid = GET_INDEX_NODE(size);
   unsigned int scpe_node_nid = GET_INDEX_NODE(scpe);
   unsigned int argt_node_nid = GET_INDEX_NODE(argt);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type_node_nid);
   IR_schema[TOK(TOK_SCPE)] = STR(scpe_node_nid);
   IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
   IR_schema[TOK(TOK_ARGT)] = STR(argt_node_nid);
   IR_schema[TOK(TOK_ALGN)] = STR(algn);
   IR_schema[TOK(TOK_USED)] = STR(used);
   IR_schema[TOK(TOK_SRCP)] = srcp;
   IR_schema[TOK(TOK_REGISTER)] = STR(register_flag);
   IR_schema[TOK(TOK_READONLY)] = STR(readonly_flag);

   if(smt_ann)
   {
      THROW_ASSERT(smt_ann->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int smt_node_nid = GET_INDEX_NODE(smt_ann);
      IR_schema[TOK(TOK_SMT_ANN)] = STR(smt_node_nid);
   }

   if(name)
   {
      THROW_ASSERT(name->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int name_node_nid = GET_INDEX_NODE(name);
      IR_schema[TOK(TOK_NAME)] = STR(name_node_nid);
   }

   if(init)
   {
      THROW_ASSERT(init->get_kind() == tree_reindex_K, "Node is not a tree reindex");
      unsigned int init_nid = GET_INDEX_NODE(init);
      IR_schema[TOK(TOK_INIT)] = STR(init_nid);
   }

   this->TreeM->create_tree_node(node_nid, parm_decl_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// TYPE_OBJ

/// Create a void type
tree_nodeRef tree_manipulation::create_void_type() const
{
   ///@41     void_type        name: @58      algn: 8
   ///@58     type_decl        name: @63      type: @41      srcp:
   ///"<built-in>:0:0"
   ///@63     identifier_node  strg: "void" lngt: 4
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef void_node;

   IR_schema[TOK(TOK_ALGN)] = STR(ALGN_VOID);
   // If a void type already exists, it is not created a new one
   unsigned int void_type_nid = this->TreeM->find(void_type_K, IR_schema);

   if(!void_type_nid)
   {
      void_type_nid = this->TreeM->new_tree_node_id();
      unsigned int type_decl_nid = this->TreeM->new_tree_node_id();
      unsigned int void_identifier_nid = GET_INDEX_NODE(create_identifier_node("void")); //@63

      /// Create the void_type
      ///@41  void_type        name: @58      algn: 8
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      this->TreeM->create_tree_node(void_type_nid, void_type_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(void_type_nid) + " (void_type)");

      /// Create the type_decl
      ///@58 type_decl name: @63 type: @41 srcp: "<built-in>:0:0"
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(void_identifier_nid);
      IR_schema[TOK(TOK_TYPE)] = STR(void_type_nid);
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(type_decl_nid, type_decl_K, IR_schema);
      void_node = TreeM->GetTreeReindex(void_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(void_node)) + " (" + GET_NODE(void_node)->get_kind_text() + " void)");
   }
   else
   {
      void_node = TreeM->GetTreeReindex(void_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(void_node)) + " (" + GET_NODE(void_node)->get_kind_text() + " void)");
   }

   return void_node;
}

/// Create a bit_size type
tree_nodeRef tree_manipulation::create_bit_size_type() const
{
   ///@32    identifier_node  strg: "bitsizetype"  lngt: 13
   ///@18    integer_type   name: @32   size: @33   algn: 64    prec: 64 unsigned
   /// min : @34   max : @35
   ///@33    integer_cst      type: @18      low : 64
   ///@34    integer_cst      type: @18      low : 0
   ///@35    integer_cst      type: @18      low : -1
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef bit_size_node;

   ///@32    identifier_node  strg: "bitsizetype"  lngt: 13
   tree_nodeRef bit_size_identifier_node = create_identifier_node("bitsizetype");
   unsigned int bit_size_identifier_nid = GET_INDEX_NODE(bit_size_identifier_node);

   ///@18    integer_type   name: @32   size: @33   algn: 64    prec: 64 unsigned
   /// min : @34   max : @35
   IR_schema[TOK(TOK_NAME)] = STR(bit_size_identifier_nid); ///@32
   unsigned int bit_size_type_nid = this->TreeM->find(integer_type_K, IR_schema);

   /// not_found decl
   if(!bit_size_type_nid)
   {
      bit_size_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();
      unsigned int min_node_nid = this->TreeM->new_tree_node_id();
      unsigned int max_node_nid = this->TreeM->new_tree_node_id();
      ///@18    integer_type   name: @32   size: @33   algn: 64    prec: 64
      /// unsigned   min : @34   max : @35
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_BIT_SIZE);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_BIT_SIZE);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      IR_schema[TOK(TOK_MIN)] = STR(min_node_nid);
      IR_schema[TOK(TOK_MAX)] = STR(max_node_nid);
      this->TreeM->create_tree_node(bit_size_type_nid, integer_type_K, IR_schema);
      bit_size_node = TreeM->GetTreeReindex(bit_size_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(bit_size_node)) + " (" + GET_NODE(bit_size_node)->get_kind_text() + " bit_size)");

      ///@33    integer_cst      type: @18      low : 64
      CreateIntegerCst(bit_size_node, SIZE_VALUE_BIT_SIZE, size_node_nid);

      ///@34    integer_cst      type: @18      low : 0
      CreateIntegerCst(bit_size_node, MIN_VALUE_BIT_SIZE, min_node_nid);

      ///@35    integer_cst      type: @18      low : -1
      CreateIntegerCst(bit_size_node, MAX_VALUE_BIT_SIZE, max_node_nid);
   }
   else
   {
      bit_size_node = TreeM->GetTreeReindex(bit_size_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(bit_size_node)) + " (" + GET_NODE(bit_size_node)->get_kind_text() + " bit_size)");
   }
   return bit_size_node;
}

/// Create a size type
tree_nodeRef tree_manipulation::create_size_type() const
{
   //@124    identifier_node  strg: "sizetype"             lngt: 8
   //@96     integer_type     name: @124     size: @15      algn: 32
   //                         prec: 32       unsigned       min : @125
   //                         max : @126

   ///@32    identifier_node  strg: "sizetype"  lngt: 13
   ///@18    integer_type   name: @32   size: @33   algn: 64    prec: 64 unsigned
   /// min : @34   max : @35
   ///@33    integer_cst      type: @18      low : 64
   ///@34    integer_cst      type: @18      low : 0
   ///@35    integer_cst      type: @18      low : -1
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef size_node;

   //@124    identifier_node  strg: "sizetype"             lngt: 8
   /// sizetype will translated in unsigned long
   tree_nodeRef size_identifier_node = create_identifier_node("sizetype");
   unsigned int size_identifier_nid = GET_INDEX_NODE(size_identifier_node);

   //@96     integer_type name: @124 size: @15 algn: 32 prec: 32 unsigned min :
   //@125 max : @126
   IR_schema[TOK(TOK_NAME)] = STR(size_identifier_nid); ///@124
   unsigned int size_type_nid = this->TreeM->find(integer_type_K, IR_schema);

   /// not_found decl
   if(!size_type_nid)
   {
      size_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();
      unsigned int min_node_nid = this->TreeM->new_tree_node_id();
      unsigned int max_node_nid = this->TreeM->new_tree_node_id();
      ///@18    integer_type   name: @32   size: @33   algn: 64    prec: 64
      /// unsigned   min : @34   max : @35
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_BIT_SIZE);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_BIT_SIZE);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      IR_schema[TOK(TOK_MIN)] = STR(min_node_nid);
      IR_schema[TOK(TOK_MAX)] = STR(max_node_nid);
      this->TreeM->create_tree_node(size_type_nid, integer_type_K, IR_schema);
      size_node = TreeM->GetTreeReindex(size_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(size_node)) + " (" + GET_NODE(size_node)->get_kind_text() + " bit_size)");

      ///@33    integer_cst      type: @18      low : 64
      CreateIntegerCst(size_node, SIZE_VALUE_BIT_SIZE, size_node_nid);

      ///@34    integer_cst      type: @18      low : 0
      CreateIntegerCst(size_node, MIN_VALUE_BIT_SIZE, min_node_nid);

      ///@35    integer_cst      type: @18      low : -1
      CreateIntegerCst(size_node, MAX_VALUE_BIT_SIZE, max_node_nid);
   }
   else
   {
      size_node = TreeM->GetTreeReindex(size_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(size_node)) + " (" + GET_NODE(size_node)->get_kind_text() + " bit_size)");
   }
   return size_node;
}

/// Create a boolean type
tree_nodeRef tree_manipulation::create_boolean_type() const
{
   ///@48 boolean_type name: @55 size: @7 algn: 8
   ///@55 type_decl name: @58 type: @48 srcp: "<built-in>:0:0"
   ///@58 identifier_node strg: "_Bool" lngt: 5
   ///@7 integer_cst type: @21 low: 8       @21 is bit_size

   ///@58 identifier_node strg: "_Bool" lngt: 5
   tree_nodeRef boolean_type_node;

   tree_nodeRef boolean_identifier_node = create_identifier_node("_Bool");
   unsigned int boolean_identifier_nid = GET_INDEX_NODE(boolean_identifier_node); ///@58

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   ///@55 type_decl name: @58 type: @48 srcp: "<built-in>:0:0"
   IR_schema[TOK(TOK_NAME)] = STR(boolean_identifier_nid); //@58
   unsigned int type_decl_nid = this->TreeM->find(type_decl_K, IR_schema);

   /// not_found decl
   if(!type_decl_nid)
   {
      type_decl_nid = this->TreeM->new_tree_node_id();
      unsigned int boolean_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();

      ///@55 type_decl name: @58 type: @48 srcp: "<built-in>:0:0"
      IR_schema[TOK(TOK_TYPE)] = STR(boolean_type_nid); //@48
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(type_decl_nid, type_decl_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(type_decl_nid) + " (type_decl boolean)");

      ///@48 boolean_type name: @55 size: @7 algn: 8
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid); //@55
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid); //@7
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_BOOLEAN);
      this->TreeM->create_tree_node(boolean_type_nid, boolean_type_K, IR_schema);
      boolean_type_node = TreeM->GetTreeReindex(boolean_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(boolean_type_node)) + " (" + GET_NODE(boolean_type_node)->get_kind_text() + " boolean)");

      ///@7 integer_cst type: @21 low: 8       @21 is bit_size
      tree_nodeRef bit_size_node = create_bit_size_type(); //@21

      CreateIntegerCst(bit_size_node, SIZE_VALUE_BOOL, size_node_nid);
   }
   else
   {
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_BOOLEAN);

      unsigned int boolean_type_nid = this->TreeM->find(boolean_type_K, IR_schema);

      if(!boolean_type_nid)
      {
         THROW_ERROR("Something wrong happened!");
      }

      boolean_type_node = TreeM->GetTreeReindex(boolean_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(boolean_type_node)) + " (" + GET_NODE(boolean_type_node)->get_kind_text() + " boolean)");
   }
   return boolean_type_node;
}

/// Create an unsigned integer type
tree_nodeRef tree_manipulation::create_default_unsigned_integer_type() const
{
   ///@41     identifier_node  strg: "unsigned int"         lngt: 12
   ///@8      integer_type     name: @20      size: @12      algn: 32      prec:
   /// 32       unsigned       min : @21    max : @22
   ///@20     type_decl        name: @41      type: @8       srcp:
   ///"<built-in>:0:0"
   ///@12     integer_cst      type: @18      low : 32
   ///@21     integer_cst      type: @8       low : 0
   ///@22     integer_cst      type: @8       low : -1
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef integer_type_node;

   ///@41     identifier_node  strg: "unsigned int"         lngt: 12
   tree_nodeRef unsigned_int_identifier_node = create_identifier_node("unsigned int");
   unsigned int unsigned_int_identifier_nid = GET_INDEX_NODE(unsigned_int_identifier_node); ///@41

   ///@20     type_decl        name: @41      type: @8       srcp:
   ///"<built-in>:0:0"
   IR_schema.clear();
   IR_schema[TOK(TOK_NAME)] = STR(unsigned_int_identifier_nid);
   unsigned int type_decl_nid = this->TreeM->find(type_decl_K, IR_schema);

   /// not_found decl
   if(!type_decl_nid)
   {
      type_decl_nid = this->TreeM->new_tree_node_id();
      unsigned int integer_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();
      unsigned int min_node_nid = this->TreeM->new_tree_node_id();
      unsigned int max_node_nid = this->TreeM->new_tree_node_id();

      ///@20     type_decl        name: @41      type: @8       srcp:
      ///"<built-in>:0:0"
      IR_schema[TOK(TOK_TYPE)] = STR(integer_type_nid); //@8
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(type_decl_nid, type_decl_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(type_decl_nid) + " (type_decl unsigned_int)");

      ///@8      integer_type     name: @20      size: @12      algn: 32 prec: 32
      /// unsigned       min : @21    max : @22
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_UNSIGNED_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_UNSIGNED_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      IR_schema[TOK(TOK_MIN)] = STR(min_node_nid);
      IR_schema[TOK(TOK_MAX)] = STR(max_node_nid);
      this->TreeM->create_tree_node(integer_type_nid, integer_type_K, IR_schema);
      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " unsigned_int)");

      tree_nodeRef bit_size_node = create_bit_size_type();

      ///@12     integer_cst      type: @18      low : 32
      CreateIntegerCst(bit_size_node, SIZE_VALUE_UNSIGNED_INT, size_node_nid);
      ///@21     integer_cst      type: @8       low : 0
      CreateIntegerCst(integer_type_node, MIN_VALUE_UNSIGNED_INT, min_node_nid);
      ///@22     integer_cst      type: @8       low : -1
      CreateIntegerCst(integer_type_node, MAX_VALUE_UNSIGNED_INT, max_node_nid);
   }
   else
   {
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_UNSIGNED_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_UNSIGNED_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      unsigned int integer_type_nid = this->TreeM->find(integer_type_K, IR_schema);

      if(!integer_type_nid)
      {
         THROW_ERROR("Something wrong happened!");
      }

      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " unsigned int)");
   }
   return integer_type_node;
}

tree_nodeRef tree_manipulation::CreateDefaultUnsignedLongLongInt() const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef integer_type_node;

   tree_nodeRef unsigned_int_identifier_node = create_identifier_node("unsigned long long int");
   unsigned int unsigned_int_identifier_nid = GET_INDEX_NODE(unsigned_int_identifier_node); ///@41

   tree_nodeRef bit_size_node = create_bit_size_type();

   IR_schema.clear();
   IR_schema[TOK(TOK_NAME)] = STR(unsigned_int_identifier_nid);
   unsigned int type_decl_nid = this->TreeM->find(type_decl_K, IR_schema);

   /// not_found decl
   if(!type_decl_nid)
   {
      type_decl_nid = this->TreeM->new_tree_node_id();
      unsigned int integer_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();
      unsigned int min_node_nid = this->TreeM->new_tree_node_id();
      unsigned int max_node_nid = this->TreeM->new_tree_node_id();

      IR_schema[TOK(TOK_TYPE)] = STR(integer_type_nid); //@8
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(type_decl_nid, type_decl_K, IR_schema);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(type_decl_nid) + " (type_decl unsigned_int)");

      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_UNSIGNED_LONG_LONG_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_UNSIGNED_LONG_LONG_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      IR_schema[TOK(TOK_MIN)] = STR(min_node_nid);
      IR_schema[TOK(TOK_MAX)] = STR(max_node_nid);
      this->TreeM->create_tree_node(integer_type_nid, integer_type_K, IR_schema);
      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " unsigned long long int int)");

      CreateIntegerCst(bit_size_node, SIZE_VALUE_UNSIGNED_LONG_LONG_INT, size_node_nid);
      CreateIntegerCst(integer_type_node, MIN_VALUE_UNSIGNED_LONG_LONG_INT, min_node_nid);
      CreateIntegerCst(integer_type_node, MAX_VALUE_UNSIGNED_LONG_LONG_INT, max_node_nid);
   }
   else
   {
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_UNSIGNED_LONG_LONG_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_UNSIGNED_LONG_LONG_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(true);
      unsigned int integer_type_nid = this->TreeM->find(integer_type_K, IR_schema);

      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found  node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " unsigned long long int)");
   }
   return integer_type_node;
}

/// Create an integer type
tree_nodeRef tree_manipulation::create_default_integer_type() const
{
   ///@36     identifier_node  strg: "int" lngt: 3
   ///@19     type_decl        name: @36      type: @8       srcp:
   ///"<built-in>:0:0"
   ///@8      integer_type     name: @19      size: @11   algn: 32   prec: 32 min
   ///: @20      max : @21
   ///@11     integer_cst      type: @18      low : 32
   ///@20     integer_cst      type: @8       high: -1  low : -2147483648
   ///@21     integer_cst      type: @8       low : 2147483647

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   tree_nodeRef integer_type_node;

   ///@36     identifier_node  strg: "int" lngt: 3
   tree_nodeRef int_identifier_node = create_identifier_node("int");
   unsigned int int_identifier_nid = GET_INDEX_NODE(int_identifier_node); ///@36

   ///@19     type_decl        name: @36      type: @8       srcp:
   ///"<built-in>:0:0"
   IR_schema.clear();
   IR_schema[TOK(TOK_NAME)] = STR(int_identifier_nid);
   unsigned int type_decl_nid = this->TreeM->find(type_decl_K, IR_schema);

   /// not_found decl
   if(!type_decl_nid)
   {
      type_decl_nid = this->TreeM->new_tree_node_id();
      unsigned int integer_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();
      unsigned int min_node_nid = this->TreeM->new_tree_node_id();
      unsigned int max_node_nid = this->TreeM->new_tree_node_id();

      ///@20     type_decl        name: @41      type: @8       srcp:
      ///"<built-in>:0:0"
      IR_schema[TOK(TOK_TYPE)] = STR(integer_type_nid); //@8
      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(type_decl_nid, type_decl_K, IR_schema);

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(type_decl_nid) + " (type_decl int)");

      ///@8      integer_type     name: @19      size: @11   algn: 32   prec: 32
      /// min : @20      max : @21
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(false);
      IR_schema[TOK(TOK_MIN)] = STR(min_node_nid);
      IR_schema[TOK(TOK_MAX)] = STR(max_node_nid);
      this->TreeM->create_tree_node(integer_type_nid, integer_type_K, IR_schema);
      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " int)");

      tree_nodeRef bit_size_node = create_bit_size_type();
      ///@11     integer_cst      type: @18      low : 32
      CreateIntegerCst(bit_size_node, SIZE_VALUE_INT, size_node_nid);

      ///@20     integer_cst      type: @8       high: -1  low : -2147483648
      CreateIntegerCst(integer_type_node, MIN_VALUE_INT, min_node_nid);

      ///@21     integer_cst      type: @8       low : 2147483647
      CreateIntegerCst(integer_type_node, MAX_VALUE_INT, max_node_nid);
   }
   else
   {
      IR_schema.clear();
      IR_schema[TOK(TOK_NAME)] = STR(type_decl_nid);
      IR_schema[TOK(TOK_ALGN)] = STR(ALGN_INT);
      IR_schema[TOK(TOK_PREC)] = STR(PREC_INT);
      IR_schema[TOK(TOK_UNSIGNED)] = STR(false);

      unsigned int integer_type_nid = this->TreeM->find(integer_type_K, IR_schema);

      if(!integer_type_nid)
      {
         THROW_ERROR("Something wrong happened!");
      }

      integer_type_node = TreeM->GetTreeReindex(integer_type_nid);

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Found   node " + STR(GET_INDEX_NODE(integer_type_node)) + " (" + GET_NODE(integer_type_node)->get_kind_text() + " int)");
   }
   return integer_type_node;
}

/// Create a pointer type
tree_nodeRef tree_manipulation::create_pointer_type(const tree_nodeConstRef& ptd, unsigned int algn) const
{
   ///@15     pointer_type     size: @12      algn: 32       ptd : @9     @9 type
   /// of the pointer
   ///@12     integer_cst      type: @26      low : 32       @26 is bit_size_type
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_PTD)] = STR(ptd->index);
   IR_schema[TOK(TOK_ALGN)] = STR(algn);

   tree_nodeRef pointer_type_node;

   unsigned int pointer_type_nid = this->TreeM->find(pointer_type_K, IR_schema);

   if(!pointer_type_nid)
   {
      pointer_type_nid = this->TreeM->new_tree_node_id();
      unsigned int size_node_nid = this->TreeM->new_tree_node_id();

      ///@12     integer_cst      type: @26      low : 32       @26 is
      /// bit_size_type
      IR_schema[TOK(TOK_SIZE)] = STR(size_node_nid);
      this->TreeM->create_tree_node(pointer_type_nid, pointer_type_K, IR_schema);
      pointer_type_node = TreeM->GetTreeReindex(pointer_type_nid);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(pointer_type_nid) + " (pointer_type)");

      tree_nodeRef bit_size_node = create_bit_size_type();

      ///@12     integer_cst      type: @26      low : 32       @26 is
      /// bit_size_type
      CreateIntegerCst(bit_size_node, SIZE_VALUE_POINTER, size_node_nid);
   }
   else
   {
      pointer_type_node = TreeM->GetTreeReindex(pointer_type_nid);
   }
   return pointer_type_node;
}

tree_nodeRef tree_manipulation::create_integer_type_with_prec(unsigned int prec, bool unsigned_p) const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   IR_schema[TOK(TOK_ALGN)] = STR(prec);
   IR_schema[TOK(TOK_PREC)] = STR(prec);
   IR_schema[TOK(TOK_UNSIGNED)] = STR(unsigned_p);
   unsigned int integer_type_nid = TreeM->find(integer_type_K, IR_schema);

   if(!integer_type_nid)
   {
      integer_type_nid = TreeM->new_tree_node_id();
      tree_nodeRef size_node = TreeM->CreateUniqueIntegerCst(prec, integer_type_nid);
      IR_schema[TOK(TOK_SIZE)] = STR(GET_INDEX_NODE(size_node));
      tree_nodeRef min_node = TreeM->CreateUniqueIntegerCst((unsigned_p ? 0 : (1LL << (prec - 1))), integer_type_nid);
      IR_schema[TOK(TOK_MIN)] = STR(GET_INDEX_NODE(min_node));
      tree_nodeRef max_node = TreeM->CreateUniqueIntegerCst((unsigned_p ? (~0LL) : static_cast<long long int>((~0ULL) >> 1)), integer_type_nid);
      IR_schema[TOK(TOK_MAX)] = STR(GET_INDEX_NODE(max_node));
      TreeM->create_tree_node(integer_type_nid, integer_type_K, IR_schema);
   }
   return TreeM->GetTreeReindex(integer_type_nid);
}

/// MISCELLANEOUS_OBJ_TREE_NODES

/// SSA_NAME

/// Create a ssa_name node
tree_nodeRef tree_manipulation::create_ssa_name(const tree_nodeConstRef& var, const tree_nodeConstRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max, bool volatile_flag, bool virtual_flag) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating ssa starting from var " + (var ? var->ToString() : "") + " and type " + (type ? type->ToString() : ""));
   THROW_ASSERT(!var || var->get_kind() == tree_reindex_K, "Var node is not a tree reindex");
   THROW_ASSERT(!min || min->get_kind() == tree_reindex_K, "min node is not a tree reindex");
   THROW_ASSERT(!max || max->get_kind() == tree_reindex_K, "max node is not a tree reindex");

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int vers = this->TreeM->get_next_vers();
   unsigned int node_nid = this->TreeM->new_tree_node_id();

   if(type)
   {
      IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   }
   if(var)
   {
      THROW_ASSERT(GET_CONST_NODE(var)->get_kind() == var_decl_K or GET_CONST_NODE(var)->get_kind() == parm_decl_K, GET_CONST_NODE(var)->get_kind_text());
      IR_schema[TOK(TOK_VAR)] = STR(var->index);
   }
   if(min)
      IR_schema[TOK(TOK_MIN)] = STR(min->index);
   if(max)
      IR_schema[TOK(TOK_MAX)] = STR(max->index);
   IR_schema[TOK(TOK_VERS)] = STR(vers);
   IR_schema[TOK(TOK_VOLATILE)] = STR(volatile_flag);
   IR_schema[TOK(TOK_VIRTUAL)] = STR(virtual_flag);

   this->TreeM->create_tree_node(node_nid, ssa_name_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   const tree_nodeRef curr_node = GET_NODE(node_ref);

   if(var and GetPointer<ssa_name>(curr_node)->CGetUseStmts().empty())
   {
      THROW_ASSERT(GET_CONST_NODE(var)->get_kind() == var_decl_K or GET_CONST_NODE(var)->get_kind() == parm_decl_K, GET_CONST_NODE(var)->get_kind_text());
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_IR_schema;
      gimple_nop_IR_schema[TOK(TOK_SRCP)] = GetPointer<const srcp>(GET_CONST_NODE(var))->include_name + ":" + STR(GetPointer<const srcp>(GET_CONST_NODE(var))->line_number) + ":" + STR(GetPointer<const srcp>(GET_CONST_NODE(var))->column_number);
      unsigned int gimple_nop_node_id = this->TreeM->new_tree_node_id();
      this->TreeM->create_tree_node(gimple_nop_node_id, gimple_nop_K, gimple_nop_IR_schema);
      tree_nodeRef gimple_nop_node_ref = TreeM->GetTreeReindex(gimple_nop_node_id);
      GetPointer<ssa_name>(curr_node)->AddDefStmt(gimple_nop_node_ref);
   }

   auto* sn = GetPointer<ssa_name>(curr_node);
   sn->virtual_flag = virtual_flag;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + node_ref->ToString());

   return node_ref;
}

/// GIMPLE_ASSIGN

/// Create a gimple_assign
tree_nodeRef tree_manipulation::create_gimple_modify_stmt(const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp, const unsigned int bb_index) const
{
   THROW_ASSERT(op0->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(op1->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   unsigned int node_nid = this->TreeM->new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_SRCP)] = srcp;
   IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(op0));
   IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(op1));
   this->TreeM->create_tree_node(node_nid, gimple_assign_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   GetPointer<gimple_node>(GET_NODE(node_ref))->bb_index = bb_index;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created node " + STR(node_ref));

   return node_ref;
}

tree_nodeRef tree_manipulation::CreateGimpleAssign(const tree_nodeRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max, const tree_nodeRef& op, unsigned int bb_index, const std::string& srcp) const
{
   tree_nodeRef ssa_vd = create_ssa_name(tree_nodeRef(), type, min, max);
   auto ga = create_gimple_modify_stmt(ssa_vd, op, srcp, bb_index);
   GetPointer<ssa_name>(TreeM->get_tree_node_const(ssa_vd->index))->SetDefStmt(TreeM->GetTreeReindex(ga->index));
   return ga;
}

/// GIMPLE_CALL
tree_nodeRef tree_manipulation::create_gimple_call(const tree_nodeConstRef& called_function, const std::vector<tree_nodeRef>& args, const std::string& srcp, const unsigned int bb_index) const
{
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ae_IR_schema, gc_IR_schema;

   const auto function_type = tree_helper::CGetType(GET_CONST_NODE(called_function));
   ae_IR_schema[TOK(TOK_OP)] = STR(called_function->index);
   ae_IR_schema[TOK(TOK_TYPE)] = STR(create_pointer_type(function_type, ALGN_POINTER)->index);
   ae_IR_schema[TOK(TOK_SRCP)] = srcp;

   std::string args_string;
   for(const auto& arg : args)
   {
      if(!args_string.empty())
      {
         args_string += "_";
      }
      args_string += STR(arg->index);
   }

   const unsigned int ae_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(ae_id, addr_expr_K, ae_IR_schema);
   gc_IR_schema[TOK(TOK_ARG)] = args_string;
   gc_IR_schema[TOK(TOK_FN)] = STR(ae_id);
   gc_IR_schema[TOK(TOK_TYPE)] = STR(function_type->index);
   gc_IR_schema[TOK(TOK_SRCP)] = srcp;
   const unsigned int gc_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(gc_id, gimple_call_K, gc_IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(gc_id);
   GetPointer<gimple_node>(GET_NODE(node_ref))->bb_index = bb_index;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created node " + STR(node_ref));
   return node_ref;
}

/// GIMPLE_COND

/// Create a gimple_cond with one operand (type void)
tree_nodeRef tree_manipulation::create_gimple_cond(const tree_nodeRef& expr, const std::string& srcp, unsigned int bb_index) const
{
   THROW_ASSERT(expr->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   /// schema used to create the nodes
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   unsigned int void_type_nid = GET_INDEX_NODE(create_void_type());
   unsigned int expr_nid = GET_INDEX_NODE(expr);
   unsigned int gimple_cond_name_nid = this->TreeM->new_tree_node_id();

   ///@116 gimple_cond type: @62 srcp: "array.c":1 op: @115
   IR_schema.clear();
   IR_schema[TOK(TOK_TYPE)] = STR(void_type_nid);
   IR_schema[TOK(TOK_OP0)] = STR(expr_nid);
   IR_schema[TOK(TOK_SRCP)] = srcp;
   this->TreeM->create_tree_node(gimple_cond_name_nid, gimple_cond_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(gimple_cond_name_nid);
   GetPointer<gimple_node>(GET_NODE(node_ref))->bb_index = bb_index;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   return node_ref;
}

/// Create gimple_return
tree_nodeRef tree_manipulation::create_gimple_return(const tree_nodeRef& type, const tree_nodeRef& expr, const std::string& srcp, unsigned int bb_index) const
{
   THROW_ASSERT(type->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(expr->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT(!srcp.empty(), "It requires a non empty string");

   auto node_nid = this->TreeM->new_tree_node_id();
   auto type_id = GET_INDEX_NODE(type);
   auto op_node_nid = GET_INDEX_NODE(expr);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_OP)] = STR(op_node_nid);
   IR_schema[TOK(TOK_TYPE)] = STR(type_id);
   IR_schema[TOK(TOK_SRCP)] = srcp;

   this->TreeM->create_tree_node(node_nid, gimple_return_K, IR_schema);
   tree_nodeRef node_ref = TreeM->GetTreeReindex(node_nid);
   GetPointer<gimple_node>(GET_NODE(node_ref))->bb_index = bb_index;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(node_ref)) + " (" + GET_NODE(node_ref)->get_kind_text() + ")");

   /// Add definition in ssa_name
   return node_ref;
}

/// GIMPLE_PHI

/// Create a gimple_phi
tree_nodeRef tree_manipulation::create_phi_node(tree_nodeRef& ssa_res, const std::vector<std::pair<tree_nodeRef, unsigned int>>& list_of_def_edge, const tree_nodeRef& scpe, unsigned int bb_index, bool virtual_flag) const
{
   auto iterator = list_of_def_edge.begin();
   tree_nodeRef ssa_ref = iterator->first;
   auto* sn_ref = GetPointer<ssa_name>(GET_NODE(ssa_ref));
   THROW_ASSERT(ssa_ref->get_kind() == tree_reindex_K, "ssa_name res is not a tree_reindex node");
   for(++iterator; iterator != list_of_def_edge.end(); ++iterator)
   {
      tree_nodeRef tn = iterator->first;
      THROW_ASSERT(tn->get_kind() == tree_reindex_K, "ssa_name res is not a tree_reindex node");
      if(!sn_ref && GetPointer<ssa_name>(GET_NODE(tn)))
      {
         ssa_ref = tn;
         sn_ref = GetPointer<ssa_name>(GET_NODE(ssa_ref));
      }
   }
   if(sn_ref)
   {
      ssa_res = create_ssa_name(sn_ref->var, sn_ref->type, sn_ref->min, sn_ref->max, false, sn_ref->virtual_flag);
   }
   else
   {
      unsigned ssa_res_type_index;
      const tree_nodeRef ssa_res_type_node = tree_helper::get_type_node(GET_NODE(list_of_def_edge.begin()->first), ssa_res_type_index);
      ssa_res = create_ssa_name(tree_nodeRef(), ssa_res_type_node, tree_nodeRef(), tree_nodeRef(), false, false);
   }

   unsigned int phi_node_nid = this->TreeM->new_tree_node_id();

   // Create the gimple_phi
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_RES)] = STR(GET_INDEX_NODE(ssa_res));
   IR_schema[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(scpe));
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   this->TreeM->create_tree_node(phi_node_nid, gimple_phi_K, IR_schema);
   tree_nodeRef phi_stmt = TreeM->GetTreeReindex(phi_node_nid);
   GetPointer<gimple_node>(GET_NODE(phi_stmt))->bb_index = bb_index;

   auto* pn = GetPointer<gimple_phi>(GET_NODE(phi_stmt));
   pn->virtual_flag = virtual_flag;

   for(const auto& def_edge : list_of_def_edge)
   {
      pn->AddDefEdge(TreeM, def_edge);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created node " + STR(GET_INDEX_NODE(phi_stmt)) + " (" + GET_NODE(phi_stmt)->get_kind_text() + ")");

   return phi_stmt;
}

/// BASIC_BLOCK

/// Create a basic block
blocRef tree_manipulation::create_basic_block(std::map<unsigned int, blocRef>& list_of_bloc, std::vector<unsigned int> predecessors, std::vector<unsigned int> successors, std::vector<tree_nodeRef> stmt, unsigned int number, unsigned int true_edge,
                                              unsigned int false_edge, std::vector<tree_nodeRef> phi) const
{
   /// Create new basic block
   blocRef new_bb = blocRef(new bloc(number));
   // Add in the list of block the new bb
   list_of_bloc[new_bb->number] = new_bb;

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created BB " + STR(new_bb->number));

   /// Set the true_edge
   new_bb->true_edge = true_edge;
   /// Set the false_edge
   new_bb->false_edge = false_edge;

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   True Edge: " + STR(new_bb->true_edge) + "  False Edge: " + STR(new_bb->false_edge));

   /// Insert the list of predecessors
   for(auto pred : predecessors)
   {
      new_bb->list_of_pred.push_back(pred);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Added predecessor " + STR(pred));
   }
   /// Insert the list of successors
   for(auto suc : successors)
   {
      new_bb->list_of_succ.push_back(suc);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Added successor   " + STR(suc));
   }
   /// Insert the list of statement
   for(const auto& tn : stmt)
   {
      new_bb->PushBack(tn);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Added statement   " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + ").");
   }
   /// Insert the list of phi node
   for(const auto& tn : phi)
   {
      new_bb->AddPhi(tn);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Added phi   " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + ").");
   }

   return new_bb;
}

/// Add a list of statement in basic block bb
void tree_manipulation::bb_add_stmts(blocRef& bb, const std::vector<tree_nodeRef>& stmts) const
{
   /// Insert the list of statement
   for(const auto& tn : stmts)
   {
      bb_add_stmt(bb, tn);
   }
}

/// Add a statement in basic block
void tree_manipulation::bb_add_stmt(blocRef& bb, const tree_nodeRef& stmt) const
{
   /// Insert the list of statement
   bb->PushBack(stmt);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Added statement   " + STR(GET_INDEX_NODE(stmt)) + " (" + GET_NODE(stmt)->get_kind_text() + ").");
}

/// Add a list of successors in basic block bb
void tree_manipulation::bb_add_successors(blocRef& bb, const std::vector<unsigned int>& successors) const
{
   for(auto successor : successors)
   {
      bb_add_successor(bb, successor);
   }
}

/// Add a successor in basic block bb
void tree_manipulation::bb_add_successor(blocRef& bb, const unsigned int& successor) const
{
   bb->add_succ(successor);
}

/// Add a list of predecessors in basic block bb
void tree_manipulation::bb_add_predecessors(blocRef& bb, const std::vector<unsigned int>& predecessors) const
{
   for(auto predecessor : predecessors)
   {
      bb_add_predecessor(bb, predecessor);
   }
}

/// Add a predecessor in basic block bb
void tree_manipulation::bb_add_predecessor(blocRef& bb, const unsigned int& predecessor) const
{
   bb->add_pred(predecessor);
}

/// Remove a list of successors from basic block bb
void tree_manipulation::bb_remove_successors(blocRef& bb, const std::vector<unsigned int>& successors) const
{
   for(auto successor : successors)
   {
      bb_remove_successor(bb, successor);
   }
}

/// Remove a successor from basic block bb
void tree_manipulation::bb_remove_successor(blocRef& bb, const unsigned int& successor) const
{
   std::vector<unsigned int>& list_of_succ = bb->list_of_succ;
   auto iterator_stmt = std::find(list_of_succ.begin(), list_of_succ.end(), successor);
   THROW_ASSERT(iterator_stmt != list_of_succ.end(), "Successor not found!");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor " + STR(*iterator_stmt) + " is going to be removed");
   list_of_succ.erase(iterator_stmt);
}

/// Remove a list of predecessors in basic block bb.
void tree_manipulation::bb_remove_predecessors(blocRef& bb, const std::vector<unsigned int>& predecessors) const
{
   for(auto predecessor : predecessors)
   {
      bb_remove_predecessor(bb, predecessor);
   }
}

/// Remove a predecessors in basic block bb.
void tree_manipulation::bb_remove_predecessor(blocRef& bb, const unsigned int& predecessor) const
{
   std::vector<unsigned int>& list_of_pred = bb->list_of_pred;
   auto iterator_stmt = std::find(list_of_pred.begin(), list_of_pred.end(), predecessor);
   THROW_ASSERT(iterator_stmt != list_of_pred.end(), "Successor not found!");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor " + STR(*iterator_stmt) + " is going to be removed");
   list_of_pred.erase(iterator_stmt);
}

/// Remove all the predecessors from basic block bb
void tree_manipulation::bb_remove_all_predecessors(blocRef& bb) const
{
   bb->list_of_pred.erase(bb->list_of_pred.begin(), bb->list_of_pred.end());
}

/// Remove all the successors from basic block bb
void tree_manipulation::bb_remove_all_successors(blocRef& bb) const
{
   bb->list_of_succ.erase(bb->list_of_succ.begin(), bb->list_of_succ.end());
}

/// Set the false edge of basic block bb
void tree_manipulation::bb_set_false_edge(blocRef& bb, const unsigned int& false_edge_index) const
{
   bb->false_edge = false_edge_index;
}

/// Set the true edge of basic block bb
void tree_manipulation::bb_set_true_edge(blocRef& bb, const unsigned int& true_edge_index) const
{
   bb->true_edge = true_edge_index;
}

void tree_manipulation::create_label(const blocRef& block, const unsigned int function_decl_nid) const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   /// check if the first statement is a gimple_label but with a unnamed
   /// label_decl.
   if(!block->CGetStmtList().empty())
   {
      tree_nodeRef first_stmt = block->CGetStmtList().front();
      auto* le = GetPointer<gimple_label>(GET_NODE(first_stmt));
      if(le)
      {
         THROW_ASSERT(le->op && GET_NODE(le->op)->get_kind() == label_decl_K, "label expression pattern not yet supported");
         auto* ld = GetPointer<label_decl>(GET_NODE(le->op));
         THROW_ASSERT(!ld->name, "gimple_label already added");
         /// addd the name to the label_decl
         //@436     identifier_node  strg: "GOTOLABEL0"   lngt: 10
         unsigned int label_decl_name_nid_test;
         do
         {
            IR_schema[TOK(TOK_STRG)] = "GOTOLABEL" + boost::lexical_cast<std::string>(goto_label_unique_id++);
            label_decl_name_nid_test = TreeM->find(identifier_node_K, IR_schema);
         } while(label_decl_name_nid_test);
         unsigned int label_decl_name_nid = TreeM->new_tree_node_id(); // 436
         TreeM->create_tree_node(label_decl_name_nid, identifier_node_K, IR_schema);
         IR_schema.clear();
         tree_nodeRef tr_new_label_name = TreeM->GetTreeReindex(label_decl_name_nid);
         ld->name = tr_new_label_name;
         ld->artificial_flag = false;
         return;
      }
   }

   //@27     identifier_node  strg: "void"   lngt: 4
   IR_schema[TOK(TOK_STRG)] = "void";
   unsigned int type_decl_name_nid = TreeM->find(identifier_node_K, IR_schema);
   if(!type_decl_name_nid)
   {
      type_decl_name_nid = TreeM->new_tree_node_id(); // 27
      TreeM->create_tree_node(type_decl_name_nid, identifier_node_K, IR_schema);
   }
   IR_schema.clear();

   //@436     identifier_node  strg: "GOTOLABEL0"   lngt: 10
   unsigned int label_decl_name_nid_test;
   do
   {
      IR_schema[TOK(TOK_STRG)] = "GOTOLABEL" + boost::lexical_cast<std::string>(goto_label_unique_id++);
      label_decl_name_nid_test = TreeM->find(identifier_node_K, IR_schema);
   } while(label_decl_name_nid_test);
   unsigned int label_decl_name_nid = TreeM->new_tree_node_id(); // 436
   TreeM->create_tree_node(label_decl_name_nid, identifier_node_K, IR_schema);
   IR_schema.clear();

   //@23     type_decl        name: @27      type: @15      srcp: "<built-in>":0
   IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(type_decl_name_nid);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   unsigned int label_void_type_name_nid = TreeM->find(type_decl_K, IR_schema);
   unsigned int label_type_nid;
   if(!label_void_type_name_nid)
   {
      label_void_type_name_nid = TreeM->new_tree_node_id(); // 23
      label_type_nid = TreeM->new_tree_node_id();           // 15
      IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
      TreeM->create_tree_node(label_void_type_name_nid, type_decl_K, IR_schema);
      IR_schema.clear();
      //@15     void_type        name: @23      algn: 8
      IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
      IR_schema[TOK(TOK_ALGN)] = boost::lexical_cast<std::string>(8);
      TreeM->create_tree_node(label_type_nid, void_type_K, IR_schema);
   }
   else
   {
      IR_schema.clear();
      //@15     void_type        name: @23      algn: 8
      IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
      IR_schema[TOK(TOK_ALGN)] = boost::lexical_cast<std::string>(8);
      label_type_nid = TreeM->find(void_type_K, IR_schema);
      if(!label_type_nid)
      {
         label_type_nid = TreeM->new_tree_node_id();           // 15
         label_void_type_name_nid = TreeM->new_tree_node_id(); // 23
         IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
         TreeM->create_tree_node(label_type_nid, void_type_K, IR_schema);
         IR_schema.clear();
         IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(type_decl_name_nid);
         IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
         IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         TreeM->create_tree_node(label_void_type_name_nid, type_decl_K, IR_schema);
      }
      else
      {
         THROW_ASSERT(label_void_type_name_nid == GET_INDEX_NODE(GetPointer<void_type>(TreeM->get_tree_node_const(label_type_nid))->name), "somenthing of wrong happen");
      }
   }
   IR_schema.clear();

   //@77     gimple_label       type: @15      srcp: "<built-in>":0     op  :
   //@242
   unsigned int label_expr_nid = TreeM->new_tree_node_id(); // 77

   unsigned int label_decl_nid = TreeM->new_tree_node_id(); // 242
   IR_schema[TOK(TOK_OP)] = boost::lexical_cast<std::string>(label_decl_nid);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TreeM->create_tree_node(label_expr_nid, gimple_label_K, IR_schema);
   IR_schema.clear();

   //@242    label_decl       name: @436     type: @15      scpe: @32     srcp:
   //"<built-in>":0                UID : 5
   IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_decl_name_nid);
   IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
   IR_schema[TOK(TOK_SCPE)] = boost::lexical_cast<std::string>(function_decl_nid);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TreeM->create_tree_node(label_decl_nid, label_decl_K, IR_schema);
   IR_schema.clear();

   tree_nodeRef tr_new_stmt = TreeM->GetTreeReindex(label_expr_nid);
   GetPointer<gimple_node>(GET_NODE(tr_new_stmt))->bb_index = block->number;
   block->PushFront(tr_new_stmt);
#if 0
   static int nid = 0;
   std::string raw_file_name = boost::lexical_cast<std::string>(nid++) + "create_label.raw";
   std::ofstream raw_file(raw_file_name.c_str());
   raw_file << TreeM;
   raw_file.close();
#endif
}

void tree_manipulation::create_goto(const blocRef& block, const unsigned int, const unsigned int label_expr_nid) const
{
   TreeM->add_goto();
   /// compute the label_decl_nid
   auto* le = GetPointer<gimple_label>(TreeM->get_tree_node_const(label_expr_nid));
   THROW_ASSERT(le, "expected a label expression");
   unsigned int label_decl_nid = GET_INDEX_NODE(le->op);
   /// create the gimple_goto tree node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   //@27     identifier_node  strg: "void"   lngt: 4
   IR_schema[TOK(TOK_STRG)] = "void";
   unsigned int type_decl_name_nid = TreeM->find(identifier_node_K, IR_schema);
   if(!type_decl_name_nid)
   {
      type_decl_name_nid = TreeM->new_tree_node_id(); // 27
      TreeM->create_tree_node(type_decl_name_nid, identifier_node_K, IR_schema);
   }
   IR_schema.clear();

   //@23     type_decl        name: @27      type: @15      srcp: "<built-in>":0
   IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(type_decl_name_nid);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   unsigned int label_void_type_name_nid = TreeM->find(type_decl_K, IR_schema);
   unsigned int label_type_nid;
   if(!label_void_type_name_nid)
   {
      label_void_type_name_nid = TreeM->new_tree_node_id(); // 23
      label_type_nid = TreeM->new_tree_node_id();           // 15
      IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
      TreeM->create_tree_node(label_void_type_name_nid, type_decl_K, IR_schema);
      IR_schema.clear();
      //@15     void_type        name: @23      algn: 8
      IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
      IR_schema[TOK(TOK_ALGN)] = boost::lexical_cast<std::string>(8);
      TreeM->create_tree_node(label_type_nid, void_type_K, IR_schema);
   }
   else
   {
      IR_schema.clear();
      //@15     void_type        name: @23      algn: 8
      IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
      IR_schema[TOK(TOK_ALGN)] = boost::lexical_cast<std::string>(8);
      label_type_nid = TreeM->find(void_type_K, IR_schema);
      if(!label_type_nid)
      {
         label_type_nid = TreeM->new_tree_node_id();           // 15
         label_void_type_name_nid = TreeM->new_tree_node_id(); // 23
         IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(label_void_type_name_nid);
         TreeM->create_tree_node(label_type_nid, void_type_K, IR_schema);
         IR_schema.clear();
         IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(type_decl_name_nid);
         IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
         IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         TreeM->create_tree_node(label_void_type_name_nid, type_decl_K, IR_schema);
      }
      else
      {
         THROW_ASSERT(label_void_type_name_nid == GET_INDEX_NODE(GetPointer<void_type>(TreeM->get_tree_node_const(label_type_nid))->name), "somenthing of wrong happen");
      }
   }
   IR_schema.clear();

   //@60     gimple_goto        type: @15      srcp: "<built-in>":6      op  :
   //@242
   unsigned int goto_expr_nid = TreeM->new_tree_node_id(); // 60
   IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(label_type_nid);
   IR_schema[TOK(TOK_OP)] = boost::lexical_cast<std::string>(label_decl_nid);
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TreeM->create_tree_node(goto_expr_nid, gimple_goto_K, IR_schema);
   IR_schema.clear();

   tree_nodeRef tr_new_stmt = TreeM->GetTreeReindex(goto_expr_nid);
   GetPointer<gimple_node>(GET_NODE(tr_new_stmt))->bb_index = block->number;
   block->PushBack(tr_new_stmt);
}

tree_nodeRef tree_manipulation::create_function_decl(const std::string& function_name, const tree_nodeRef& scpe, const std::vector<tree_nodeRef>& argsT, const tree_nodeRef& returnType, const std::string& srcp, bool with_body) const
{
   auto function_decl_id = TreeM->function_index(function_name);
   if(function_decl_id)
   {
      return TreeM->GetTreeReindex(function_decl_id);
   }
   else
   {
      function_decl_id = TreeM->new_tree_node_id();
   }
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;

   IR_schema[TOK(TOK_STRG)] = function_name;
   unsigned int function_name_id = TreeM->find(identifier_node_K, IR_schema);
   if(!function_name_id)
   {
      function_name_id = TreeM->new_tree_node_id();
      TreeM->create_tree_node(function_name_id, identifier_node_K, IR_schema);
   }
   else
   {
      THROW_ERROR("identifier in use by someone else");
   }
   IR_schema.clear();

   unsigned int size_node_nid = this->TreeM->new_tree_node_id();
   tree_nodeRef bit_size_node = create_bit_size_type();
   CreateIntegerCst(bit_size_node, SIZE_VALUE_FUNCTION, size_node_nid);

   unsigned int tree_list_node_nid = 0, prev_tree_list_node_nid = 0;
   for(const auto& par_type : boost::adaptors::reverse(argsT))
   {
      tree_list_node_nid = this->TreeM->new_tree_node_id();
      IR_schema[TOK(TOK_VALU)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(par_type));
      if(prev_tree_list_node_nid)
      {
         IR_schema[TOK(TOK_CHAN)] = boost::lexical_cast<std::string>(prev_tree_list_node_nid);
      }
      TreeM->create_tree_node(tree_list_node_nid, tree_list_K, IR_schema);
      IR_schema.clear();
      prev_tree_list_node_nid = tree_list_node_nid;
   }

   auto function_type_id = TreeM->new_tree_node_id();
   IR_schema[TOK(TOK_SIZE)] = boost::lexical_cast<std::string>(size_node_nid);
   IR_schema[TOK(TOK_ALGN)] = boost::lexical_cast<std::string>(8);
   IR_schema[TOK(TOK_RETN)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(returnType));
   if(tree_list_node_nid)
   {
      IR_schema[TOK(TOK_PRMS)] = boost::lexical_cast<std::string>(tree_list_node_nid);
   }
   TreeM->create_tree_node(function_type_id, function_type_K, IR_schema);
   IR_schema.clear();

   IR_schema[TOK(TOK_NAME)] = boost::lexical_cast<std::string>(function_name_id);
   IR_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(function_type_id);
   IR_schema[TOK(TOK_SCPE)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(scpe));
   IR_schema[TOK(TOK_SRCP)] = srcp;
   if(with_body)
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> statement_list_schema;
      auto statement_list_id = TreeM->get_next_available_tree_node_id();
      TreeM->create_tree_node(statement_list_id, statement_list_K, statement_list_schema);
      IR_schema[TOK(TOK_BODY)] = STR(statement_list_id);
   }
   TreeM->create_tree_node(function_decl_id, function_decl_K, IR_schema);
   IR_schema.clear();

   auto function_decl_res = TreeM->GetTreeReindex(function_decl_id);
   /// add parm_decl to the function_decl
   auto* fd = GetPointer<function_decl>(TreeM->get_tree_node_const(function_decl_id));
   unsigned int Pindex = 0;
   for(const auto& par_type : argsT)
   {
      const std::string p_name = "_P" + boost::lexical_cast<std::string>(Pindex);
      const auto p_identifier = create_identifier_node(p_name);
      const auto p_decl = create_parm_decl(p_identifier, par_type, function_decl_res, GetPointer<type_node>(GET_NODE(par_type))->size, par_type, tree_nodeRef(), tree_nodeRef(), srcp, GetPointer<const type_node>(GET_NODE(par_type))->algn, 1, false, false);
      fd->AddArg(p_decl);
      ++Pindex;
   }
   if(!with_body)
      fd->undefined_flag = true;
   TreeM->add_function(function_decl_id, GET_NODE(function_decl_res));
   return function_decl_res;
}

tree_nodeRef tree_manipulation::CreateOrExpr(const tree_nodeConstRef& first_condition, const tree_nodeConstRef& second_condition, const blocRef& block) const
{
   if(block and reuse)
   {
      for(const auto& statement : block->CGetStmtList())
      {
         const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
         if(ga)
         {
            const auto toe = GetPointer<const truth_or_expr>(GET_NODE(ga->op1));
            if(toe and ((toe->op0->index == first_condition->index and toe->op1->index == second_condition->index) or (toe->op0->index == second_condition->index and toe->op1->index == first_condition->index)))
            {
               return ga->op0;
            }
         }
      }
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema, truth_or_expr_schema, gimple_assign_schema;
   /// Create the or expr
   const auto truth_or_expr_id = TreeM->new_tree_node_id();
   const auto bt = create_boolean_type();
   const auto type_index = bt->index;
   truth_or_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   truth_or_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
   truth_or_expr_schema[TOK(TOK_OP0)] = STR(first_condition->index);
   truth_or_expr_schema[TOK(TOK_OP1)] = STR(second_condition->index);
   TreeM->create_tree_node(truth_or_expr_id, truth_or_expr_K, truth_or_expr_schema);

   auto ga = CreateGimpleAssign(bt, TreeM->CreateUniqueIntegerCst(0, type_index), TreeM->CreateUniqueIntegerCst(1, type_index), TreeM->GetTreeReindex(truth_or_expr_id), 0, "<built-in>:0:0");
   if(block)
   {
      block->PushBack(ga);
   }
   return GetPointer<gimple_assign>(GET_NODE(ga))->op0;
}

tree_nodeRef tree_manipulation::CreateAndExpr(const tree_nodeConstRef& first_condition, const tree_nodeConstRef& second_condition, const blocRef& block) const
{
   if(block and reuse)
   {
      for(const auto& statement : block->CGetStmtList())
      {
         const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
         if(ga)
         {
            const auto toe = GetPointer<const truth_and_expr>(GET_NODE(ga->op1));
            if(toe and ((toe->op0->index == first_condition->index and toe->op1->index == second_condition->index) or (toe->op0->index == second_condition->index and toe->op1->index == first_condition->index)))
            {
               return ga->op0;
            }
         }
      }
   }
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema, truth_and_expr_schema, gimple_assign_schema;
   /// Create the and expr
   const auto truth_and_expr_id = TreeM->new_tree_node_id();
   const auto bt = create_boolean_type();
   const auto type_index = bt->index;
   truth_and_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   truth_and_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
   truth_and_expr_schema[TOK(TOK_OP0)] = STR(first_condition->index);
   truth_and_expr_schema[TOK(TOK_OP1)] = STR(second_condition->index);
   TreeM->create_tree_node(truth_and_expr_id, truth_and_expr_K, truth_and_expr_schema);

   auto ga = CreateGimpleAssign(bt, TreeM->CreateUniqueIntegerCst(0, type_index), TreeM->CreateUniqueIntegerCst(1, type_index), TreeM->GetTreeReindex(truth_and_expr_id), 0, "<built-in>:0:0");
   if(block)
   {
      block->PushBack(ga);
   }
   return GetPointer<gimple_assign>(GET_NODE(ga))->op0;
}

tree_nodeRef tree_manipulation::CreateNotExpr(const tree_nodeConstRef& condition, const blocRef& block) const
{
   if(block and reuse)
   {
      for(const auto& statement : block->CGetStmtList())
      {
         const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
         if(ga)
         {
            const auto tne = GetPointer<const truth_not_expr>(GET_NODE(ga->op1));
            if(tne and tne->op->index == condition->index)
            {
               return ga->op0;
            }
         }
      }
   }
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema, truth_not_expr_schema, gimple_assign_schema;
   /// Create the not expr
   const auto truth_not_expr_id = TreeM->new_tree_node_id();
   const auto bt = create_boolean_type();
   const auto type_index = bt->index;
   truth_not_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   truth_not_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
   truth_not_expr_schema[TOK(TOK_OP)] = STR(condition->index);
   TreeM->create_tree_node(truth_not_expr_id, truth_not_expr_K, truth_not_expr_schema);

   auto ga = CreateGimpleAssign(bt, TreeM->CreateUniqueIntegerCst(0, type_index), TreeM->CreateUniqueIntegerCst(1, type_index), TreeM->GetTreeReindex(truth_not_expr_id), 0, "<built-in>:0:0");
   if(block)
   {
      block->PushBack(ga);
   }
   return GetPointer<gimple_assign>(GET_NODE(ga))->op0;
}

tree_nodeRef tree_manipulation::ExtractCondition(const tree_nodeRef& condition, const blocRef& block) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Extracting condition from " + condition->ToString());

   const auto gc = GetPointer<const gimple_cond>(GET_NODE(condition));
   THROW_ASSERT(gc, "Trying to extract condition from " + condition->ToString());
   if(GET_NODE(gc->op0)->get_kind() == ssa_name_K || GetPointer<cst_node>(GET_NODE(gc->op0)) != nullptr)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition already available as " + gc->op0->ToString());
      return gc->op0;
   }
   else
   {
      if(block and reuse)
      {
         for(const auto& statement : block->CGetStmtList())
         {
            const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
            if(ga and ga->op1->index == condition->index)
            {
               return ga->op0;
            }
         }
      }
      const auto bt = create_boolean_type();
      const auto type_index = bt->index;

      auto ga = CreateGimpleAssign(bt, TreeM->CreateUniqueIntegerCst(0, type_index), TreeM->CreateUniqueIntegerCst(1, type_index), TreeM->GetTreeReindex(GET_INDEX_NODE(gc->op0)), 0, "<built-in>:0:0");
      if(block)
      {
         block->PushBack(ga);
      }
      auto ret = GetPointer<gimple_assign>(GET_NODE(ga))->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition created is " + ret->ToString());
      return ret;
   }
}

tree_nodeRef tree_manipulation::CreateNopExpr(const tree_nodeConstRef& operand, const tree_nodeConstRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max) const
{
   THROW_ASSERT(type and type->get_kind() == tree_reindex_K, "type is not a tree reindex");

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ne_schema, ga_schema;
   ne_schema[TOK(TOK_TYPE)] = STR(type->index);
   ne_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   ne_schema[TOK(TOK_OP)] = STR(operand->index);
   const auto ne_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(ne_id, nop_expr_K, ne_schema);

   const auto ssa_operand = GetPointer<const ssa_name>(GET_CONST_NODE(operand));
   const auto int_cst_operand = GetPointer<const integer_cst>(GET_CONST_NODE(operand));
   if(not ssa_operand and not int_cst_operand)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE("Cannot create nop expr from something that is not a ssa: " + operand->ToString());
      return tree_nodeRef();
   }

   auto ga = CreateGimpleAssign(TreeM->GetTreeReindex(type->index), min, max, TreeM->GetTreeReindex(ne_id), 0, "<built-in>:0:0");
   if(ssa_operand)
   {
      GetPointer<ssa_name>(GET_NODE(GetPointer<gimple_assign>(GET_NODE(ga))->op0))->use_set = ssa_operand->use_set;
   }

   return ga;
}

tree_nodeRef tree_manipulation::CreateUnsigned(const tree_nodeConstRef& signed_type) const
{
   const auto int_signed_type = GetPointer<const integer_type>(signed_type);
   if(not int_signed_type)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " is not an integer type");
      return tree_nodeRef();
   }
   THROW_ASSERT(not int_signed_type->unsigned_flag, signed_type->ToString() + " is not signed");
   if(not int_signed_type->name)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " has not a name");
      return tree_nodeRef();
   }

   const auto type_name = GetPointer<const decl_node>(GET_CONST_NODE(int_signed_type->name));

   if(not type_name)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " has not a type decl associated");
      return tree_nodeRef();
   }

   if(type_name->include_name != "<built-in>")
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " is not builtin");
      return tree_nodeRef();
   }

   const auto in = GetPointer<identifier_node>(GET_CONST_NODE(type_name->name));
   if(not in)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " has not a name");
      return tree_nodeRef();
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ut_schema;

   ut_schema[TOK(TOK_QUAL)] = STR(static_cast<int>(int_signed_type->qual));
   const auto quals = int_signed_type->qual;
   if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " has qualifiers");
      return tree_nodeRef();
   }
   if(int_signed_type->unql)
   {
      /// THROW_ASSERT cannot be used since this function has to return an empty
      /// tree node in release
      THROW_UNREACHABLE(signed_type->ToString() + " has unqualified");
      return tree_nodeRef();
   }
   ut_schema[TOK(TOK_SIZE)] = STR(int_signed_type->size->index);
   ut_schema[TOK(TOK_ALGN)] = STR(int_signed_type->algn);
   ut_schema[TOK(TOK_UNSIGNED)] = STR(false);

   const auto min = [&]() -> tree_nodeRef {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> min_schema;
      min_schema[TOK(TOK_VALUE)] = STR(0);
      const auto find = TreeM->find(integer_cst_K, min_schema);
      if(find)
      {
         return TreeM->GetTreeReindex(find);
      }
      const auto min_id = TreeM->new_tree_node_id();
      TreeM->create_tree_node(min_id, integer_cst_K, min_schema);
      return TreeM->GetTreeReindex(min_id);
   }();
   ut_schema[TOK(TOK_MIN)] = STR(min->index);

   const auto max = [&]() -> tree_nodeRef {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> max_schema;
      max_schema[TOK(TOK_VALUE)] = STR((2 * -(tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(int_signed_type->min))))) - 1);
      const auto find = TreeM->find(integer_cst_K, max_schema);
      if(find)
      {
         return TreeM->GetTreeReindex(find);
      }
      const auto max_id = TreeM->new_tree_node_id();
      TreeM->create_tree_node(max_id, integer_cst_K, max_schema);
      return TreeM->GetTreeReindex(max_id);
   }();
   ut_schema[TOK(TOK_MAX)] = STR(max->index);
   ut_schema[TOK(TOK_PREC)] = STR(int_signed_type->prec);
   ut_schema[TOK(TOK_UNSIGNED)] = STR(true);
   const auto find = TreeM->find(integer_type_K, ut_schema);
   if(find)
   {
      return TreeM->GetTreeReindex(find);
   }
   else
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> in_schema;
      std::string unsigned_str = "unsigned " + in->strg;
      in_schema[TOK(TOK_STRG)] = unsigned_str;
      auto find_in = TreeM->find(identifier_node_K, in_schema);
      if(not find_in)
      {
         find_in = TreeM->new_tree_node_id();
         TreeM->create_tree_node(find_in, identifier_node_K, in_schema);
      }
      ut_schema[TOK(TOK_NAME)] = STR(find_in);

      const auto new_tree_node_id = TreeM->new_tree_node_id();
      TreeM->create_tree_node(new_tree_node_id, integer_type_K, ut_schema);
      return TreeM->GetTreeReindex(new_tree_node_id);
   }
}

tree_nodeRef tree_manipulation::CreateEqExpr(const tree_nodeConstRef& first_operand, const tree_nodeConstRef& second_operand, const blocRef& block) const
{
   if(block and reuse)
   {
      for(const auto& statement : block->CGetStmtList())
      {
         const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
         if(ga)
         {
            const auto ee = GetPointer<const eq_expr>(GET_NODE(ga->op1));
            if(ee and ((ee->op0->index == first_operand->index and ee->op1->index == second_operand->index) or (ee->op0->index == second_operand->index and ee->op1->index == first_operand->index)))
            {
               return ga->op0;
            }
         }
      }
   }

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema, eq_expr_schema, gimple_assign_schema;
   /// Create the eq expr
   const auto bt = create_boolean_type();
   const auto type_index = bt->index;
   const auto eq_expr_id = TreeM->new_tree_node_id();
   eq_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   eq_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
   eq_expr_schema[TOK(TOK_OP0)] = STR(first_operand->index);
   eq_expr_schema[TOK(TOK_OP1)] = STR(second_operand->index);
   TreeM->create_tree_node(eq_expr_id, eq_expr_K, eq_expr_schema);

   auto ga = CreateGimpleAssign(bt, TreeM->CreateUniqueIntegerCst(0, type_index), TreeM->CreateUniqueIntegerCst(1, type_index), TreeM->GetTreeReindex(eq_expr_id), 0, "<built-in>:0:0");
   if(block)
   {
      block->PushBack(ga);
   }
   return GetPointer<gimple_assign>(GET_NODE(ga))->op0;
}

tree_nodeRef tree_manipulation::CreateCallExpr(const tree_nodeConstRef& called_function, const std::vector<tree_nodeRef>& args, const std::string& srcp) const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ae_IR_schema, ce_IR_schema;
   ae_IR_schema[TOK(TOK_OP)] = STR(called_function->index);
   const auto ft = tree_helper::CGetType(GET_CONST_NODE(called_function));
   ae_IR_schema[TOK(TOK_TYPE)] = STR(create_pointer_type(ft, ALGN_POINTER)->index);
   ae_IR_schema[TOK(TOK_SRCP)] = srcp;
   const unsigned int ae_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(ae_id, addr_expr_K, ae_IR_schema);

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
   ce_IR_schema[TOK(TOK_FN)] = STR(ae_id);
   ce_IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_CONST_NODE(GetPointer<const function_type>(ft)->retn));
   ce_IR_schema[TOK(TOK_SRCP)] = srcp;
   const unsigned int ce_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(ce_id, call_expr_K, ce_IR_schema);
   return TreeM->GetTreeReindex(ce_id);
}

tree_nodeRef tree_manipulation::CreateAddrExpr(const tree_nodeConstRef& tn, const std::string& srcp) const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ae_IR_schema;
   const auto type_node = tree_helper::CGetType(tn);
   ae_IR_schema[TOK(TOK_OP)] = STR(tn->index);
   auto align = 8u;
   if(tn->get_kind() == var_decl_K)
   {
      auto vd = GetPointer<const var_decl>(tn);
      align = vd->algn;
   }
   const auto ptr_type = create_pointer_type(type_node, align);
   ae_IR_schema[TOK(TOK_TYPE)] = STR(ptr_type->index);
   ae_IR_schema[TOK(TOK_SRCP)] = srcp;
   const unsigned int ae_id = TreeM->new_tree_node_id();
   TreeM->create_tree_node(ae_id, addr_expr_K, ae_IR_schema);
   return TreeM->CGetTreeReindex(ae_id);
}

tree_nodeRef tree_manipulation::CreateGimpleAssignAddrExpr(const tree_nodeConstRef& tn, const unsigned int bb_index, const std::string& srcp) const
{
   auto addr_tn = CreateAddrExpr(tn, srcp);
   const auto ptr_type = GetPointer<addr_expr>(GET_NODE(addr_tn))->type;
   auto assign_node = CreateGimpleAssign(ptr_type, tree_nodeRef(), tree_nodeRef(), addr_tn, bb_index, srcp);
   auto ga = GetPointer<gimple_assign>(GET_NODE(assign_node));
   auto ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));
   ssa->use_set = PointToSolutionRef(new PointToSolution());
   ssa->use_set->Add(TreeM->CGetTreeReindex(tn->index));
   return assign_node;
}

tree_nodeRef tree_manipulation::CreateVectorBooleanType(const unsigned int number_of_elements) const
{
   const auto boolean_type = create_boolean_type();
   const auto size = TreeM->CreateUniqueIntegerCst(number_of_elements, create_size_type()->index);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_ELTS)] = STR(boolean_type->index);
   IR_schema[TOK(TOK_SIZE)] = STR(size->index);

   auto vector_type_id = TreeM->find(vector_type_K, IR_schema);

   /// not_found decl
   if(vector_type_id == 0)
   {
      vector_type_id = this->TreeM->new_tree_node_id();

      IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      this->TreeM->create_tree_node(vector_type_id, vector_type_K, IR_schema);
   }
   return TreeM->GetTreeReindex(vector_type_id);
}
