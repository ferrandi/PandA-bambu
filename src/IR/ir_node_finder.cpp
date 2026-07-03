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
 * @file ir_node_finder.cpp
 * @brief IR node finder. This class exploiting the visitor design pattern finds an IR node in an ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_node_finder.hpp"

#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "token_interface.hpp"

#include <boost/lexical_cast.hpp>

template <class type>
static bool check_value_opt(const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_element,
                            const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_end,
                            const type& value)
{
   return it_element == it_end || value == boost::lexical_cast<type>(it_element->second);
}

#define CHECK_VALUE_OPT(token, value) check_value_opt(ir_node_schema.find(TOK(token)), ir_node_schema.end(), value)

static bool check_ir_node_opt(const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_element,
                              const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_end,
                              const ir_nodeRef& tn, const std::string&)
{
   return it_element == it_end || (tn && tn->index == std::stoull(it_element->second));
}

#define CHECK_IR_NODE_OPT(token, ir_node_ref) \
   check_ir_node_opt(ir_node_schema.find(TOK(token)), ir_node_schema.end(), ir_node_ref, STOK(token))

#define IR_NOT_YET_IMPLEMENTED(token)                                    \
   THROW_ASSERT(ir_node_schema.find(TOK(token)) == ir_node_schema.end(), \
                std::string("field not yet supported ") + STOK(token))

void ir_node_finder::operator()(const ir_node* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

void ir_node_finder::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

void ir_node_finder::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && (ir_node_schema.find(TOK(TOK_IR_LOCINFO)) == ir_node_schema.end() ||
                           ir_node_schema.find(TOK(TOK_IR_LOCINFO))->second ==
                               obj->include_name + ":" + std::to_string(obj->line_number) + ":" +
                                   std::to_string(obj->column_number));
}

void ir_node_finder::operator()(const decl_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_IR_NODE_OPT(TOK_NAME, obj->name) and CHECK_IR_NODE_OPT(TOK_MNGL, obj->mngl) and
              CHECK_IR_NODE_OPT(TOK_TYPE, obj->type) and
              CHECK_VALUE_OPT(TOK_OPERATING_SYSTEM, obj->operating_system_flag) and
              CHECK_VALUE_OPT(TOK_LIBRARY_SYSTEM, obj->library_system_flag) and
              CHECK_VALUE_OPT(TOK_LIBBAMBU, obj->libbambu_flag);
}

void ir_node_finder::operator()(const expr_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_TYPE, obj->type);
}

void ir_node_finder::operator()(const node_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_IR_NODE_OPT(TOK_VUSE, obj->memuse) and CHECK_IR_NODE_OPT(TOK_VDEF, obj->memdef) and
              CHECK_IR_NODE_OPT(TOK_PREDICATE, obj->predicate);
   /// FIXME: check list_of_dep_vuses
}

void ir_node_finder::operator()(const unary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP, obj->op);
}

void ir_node_finder::operator()(const binary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP0, obj->op0) && CHECK_IR_NODE_OPT(TOK_OP1, obj->op1);
}

void ir_node_finder::operator()(const ternary_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP0, obj->op0) && CHECK_IR_NODE_OPT(TOK_OP1, obj->op1) &&
              CHECK_IR_NODE_OPT(TOK_OP2, obj->op2);
}

void ir_node_finder::operator()(const type_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_BITSIZEALLOC, obj->bitsizealloc) and
              CHECK_VALUE_OPT(TOK_SYSTEM, obj->system_flag) and CHECK_VALUE_OPT(TOK_ALGN, obj->algn);
}

void ir_node_finder::operator()(const cst_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_TYPE, obj->type);
}

void ir_node_finder::operator()(const array_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_ELTS, obj->elts) && CHECK_VALUE_OPT(TOK_NELEMENTS, obj->nelements);
}

void ir_node_finder::operator()(const call_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_FN, obj->fn);
   /// FIXME: check args
}

void ir_node_finder::operator()(const call_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_FN, obj->fn);
   /// FIXME: check args
}

void ir_node_finder::operator()(const constructor_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_TYPE, obj->type);
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

void ir_node_finder::operator()(const field_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) &&
              CHECK_VALUE_OPT(TOK_BITSIZEALLOC, obj->bitsizealloc) && CHECK_VALUE_OPT(TOK_PACKED, obj->packed_flag) &&
              CHECK_VALUE_OPT(TOK_OFFSET, obj->offset);
}

void ir_node_finder::operator()(const function_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_FN, obj->fn) && CHECK_VALUE_OPT(TOK_BUILTIN, obj->builtin_flag) &&
              CHECK_VALUE_OPT(TOK_STATIC, obj->static_flag) &&
              CHECK_VALUE_OPT(TOK_WRITING_MEMORY, obj->writing_memory) &&
              CHECK_VALUE_OPT(TOK_READING_MEMORY, obj->reading_memory) &&
              CHECK_VALUE_OPT(TOK_PIPELINE_ENABLED, obj->pipeline_enabled) &&
              CHECK_VALUE_OPT(TOK_INITIATION_TIME, obj->initiation_time) && CHECK_IR_NODE_OPT(TOK_BODY, obj->body);
   /// FIXME: check args
}

void ir_node_finder::operator()(const function_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_RETN, obj->retn) && CHECK_VALUE_OPT(TOK_VARARGS, obj->varargs_flag);
   /// FIXME: check argstype
}

void ir_node_finder::operator()(const assign_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP, obj->op0) && CHECK_IR_NODE_OPT(TOK_OP, obj->op1) &&
              CHECK_VALUE_OPT(TOK_ADDR, obj->temporary_address);
}

void ir_node_finder::operator()(const identifier_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void ir_node_finder::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_VALUE_OPT(TOK_VALUE, obj->value);
}

void ir_node_finder::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_BITSIZE, obj->bitsize) &&
              // if(obj->str != "")
              //   WRITE_UFIELD(os, obj->str);
              CHECK_VALUE_OPT(TOK_UNSIGNED, obj->unsigned_flag);
}

void ir_node_finder::operator()(const argument_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_BITSIZEALLOC, obj->bitsizealloc) &&
              CHECK_VALUE_OPT(TOK_ALGN, obj->algn) && CHECK_VALUE_OPT(TOK_READONLY, obj->readonly_flag);
}

void ir_node_finder::operator()(const phi_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_IR_NODE_OPT(TOK_RES, obj->res);
   IR_NOT_YET_IMPLEMENTED(TOK_DEF);
   IR_NOT_YET_IMPLEMENTED(TOK_EDGE);
   // std::vector<std::pair< ir_nodeRef, int> >::const_iterator vend = obj->list_of_def_edge.end();
   // for (std::vector<std::pair< ir_nodeRef, int> >::const_iterator i = obj->list_of_def_edge.begin(); i != vend;
   // i++)
   //{
   //   write_when_not_null(STOK(TOK_DEF), i->first);
   //   WRITE_NFIELD(os, STOK(TOK_EDGE), i->second);
   //}
}

void ir_node_finder::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_PTD, obj->ptd);
}

void ir_node_finder::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_OVERFLOW, obj->overflow_flag) && CHECK_VALUE_OPT(TOK_VALR, obj->valr) &&
              CHECK_VALUE_OPT(TOK_VALX, obj->valx);
}

void ir_node_finder::operator()(const real_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_BITSIZE, obj->bitsize);
}

void ir_node_finder::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_NAME, obj->name) && CHECK_VALUE_OPT(TOK_PACKED, obj->packed_flag);
   IR_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<ir_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
}

void ir_node_finder::operator()(const return_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP, obj->op);
}

void ir_node_finder::operator()(const ssa_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_IR_NODE_OPT(TOK_VAR, obj->type) && CHECK_IR_NODE_OPT(TOK_VAR, obj->var) &&
              CHECK_VALUE_OPT(TOK_VERS, obj->vers) &&
              // CHECK_IR_NODE_OPT(TOK_PTR_INFO, obj->ptr_info) &&
              CHECK_VALUE_OPT(TOK_VIRTUAL, obj->virtual_flag) && CHECK_IR_NODE_OPT(TOK_MIN, obj->min) &&
              CHECK_IR_NODE_OPT(TOK_MAX, obj->max) && CHECK_VALUE_OPT(TOK_BIT_VALUES, obj->bit_values);
   IR_NOT_YET_IMPLEMENTED(TOK_DEF_STMT);
}

void ir_node_finder::operator()(const statement_list_node* obj, unsigned int& mask)
{
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

void ir_node_finder::operator()(const lut_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_OP0, obj->op0) && CHECK_IR_NODE_OPT(TOK_OP1, obj->op1) &&
              CHECK_IR_NODE_OPT(TOK_OP2, obj->op2) && CHECK_IR_NODE_OPT(TOK_OP3, obj->op3) &&
              CHECK_IR_NODE_OPT(TOK_OP4, obj->op4) && CHECK_IR_NODE_OPT(TOK_OP5, obj->op5) &&
              CHECK_IR_NODE_OPT(TOK_OP6, obj->op6) && CHECK_IR_NODE_OPT(TOK_OP7, obj->op7) &&
              CHECK_IR_NODE_OPT(TOK_OP8, obj->op8);
}

void ir_node_finder::operator()(const variable_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_EXTERN, obj->extern_flag) &&
              CHECK_VALUE_OPT(TOK_ADDR_NOT_TAKEN, obj->addr_not_taken) &&
              CHECK_VALUE_OPT(TOK_STATIC, obj->static_flag) && CHECK_IR_NODE_OPT(TOK_INIT, obj->init) &&
              CHECK_VALUE_OPT(TOK_BITSIZEALLOC, obj->bitsizealloc) && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) &&
              CHECK_VALUE_OPT(TOK_READONLY, obj->readonly_flag) && CHECK_VALUE_OPT(TOK_BIT_VALUES, obj->bit_values);
   IR_NOT_YET_IMPLEMENTED(TOK_ADDR_STMT);
   IR_NOT_YET_IMPLEMENTED(TOK_DEF_STMT);
   IR_NOT_YET_IMPLEMENTED(TOK_USE_STMT);
}

void ir_node_finder::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   IR_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<ir_nodeRef>::const_iterator vend = obj->list_of_valu.end();
   // for (std::vector<ir_nodeRef>::const_iterator i = obj->list_of_valu.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VALU), *i);
}

void ir_node_finder::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_IR_NODE_OPT(TOK_ELTS, obj->elts);
}

void ir_node_finder::operator()(const bloc* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   // WRITE_UFIELD(os, obj->number);
   find_res = find_res && CHECK_VALUE_OPT(TOK_LOOP_ID, obj->loop_id);

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
}

void ir_node_finder::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   ir_node_mask::operator()(obj, mask);
   IR_NOT_YET_IMPLEMENTED(TOK_OP);
}
