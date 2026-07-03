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
 * @file raw_writer.cpp
 * @brief IR node writer. This class exploiting the visitor design pattern writes an IR node according to the raw
 * format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "raw_writer.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "token_interface.hpp"

#include <cstddef>
#include <list>
#include <utility>
#include <vector>

/**
 * Macro which writes on an output stream a named field of string type.
 */
#define WRITE_NFIELD_STRING(os, field_name, field_value) os << " " << (field_name) << ": \"" << (field_value) << "\""

/**
 * Macro which writes on an output stream a named field.
 */
#define WRITE_NFIELD(os, name, field) os << " " << (name) << ": " << field

/**
 * Macro which writes on an output stream an unnamed field.
 */
#define WRITE_UFIELD(os, field) os << " " << field

/**
 * Macro which writes on an output stream an unnamed field.
 */
#define WRITE_UFIELD_STRING(os, field) os << " \"" << (field) << "\""

/**
 * Macro which writes on an output stream a string. IDENTIFIER_NODE case
 */
#define WRITE_STRG_IDENTIFIER(os, field) \
   os << " "                             \
      << "strg: \"" << (field) << "\""

/**
 * Macro which writes on an output stream the IR_LocInfo fields.
 */
#define WRITE_LOCINFO(os, include_name, line_number, column_number) \
   os << " "                                                        \
      << "loc_info: \"" << (include_name) << "\":" << (line_number) << ":" << column_number

raw_writer::raw_writer(std::ostream& _os) : os(_os)
{
}

void raw_writer::write_when_not_null(const std::string& str, const ir_nodeRef& t) const
{
   if(t)
   {
      os << " " << str << ": @" << t->index;
   }
}

void raw_writer::write_when_not_null_bloc(const std::string& str, const blocRef& t)
{
   if(t)
   {
      os << " " << str << ":";
   }
   t->visit(this);
}

void raw_writer::write_when_not_null_point_to(const std::string& type, const PointToSolution& solution) const
{
   if(solution.anything)
   {
      os << " " << type << " : \"anything\"";
   }
   if(solution.escaped)
   {
      os << " " << type << " : \"escaped\"";
   }
   if(solution.ipa_escaped)
   {
      os << " " << type << " : \"ipa_escaped\"";
   }
   if(solution.nonlocal)
   {
      os << " " << type << " : \"nonlocal\"";
   }
   if(solution.null)
   {
      os << " " << type << " : \"null\"";
   }
   for(const auto& var : solution.variables)
   {
      write_when_not_null(type + "_vars", var);
   }
}

void raw_writer::operator()(const ir_node* obj, unsigned int&)
{
   os << obj->get_kind_text();
}

void raw_writer::operator()(const ir_reindex* obj, unsigned int&)
{
   THROW_ERROR("ir_node not supported: " + std::string(obj->get_kind_text()));
}

void raw_writer::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->line_number || obj->column_number || !obj->include_name.empty())
   {
      WRITE_LOCINFO(os, obj->include_name, obj->line_number, obj->column_number);
   }
}

void raw_writer::operator()(const decl_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_NAME), obj->name);
   write_when_not_null(STOK(TOK_MNGL), obj->mngl);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_PARENT), obj->parent);
   obj->IR_LocInfo::visit(this);
   if(obj->operating_system_flag)
   {
      WRITE_TOKEN(os, TOK_OPERATING_SYSTEM);
   }
   if(obj->library_system_flag)
   {
      WRITE_TOKEN(os, TOK_LIBRARY_SYSTEM);
   }
   if(obj->libbambu_flag)
   {
      WRITE_TOKEN(os, TOK_LIBBAMBU);
   }
}

void raw_writer::operator()(const expr_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   obj->IR_LocInfo::visit(this);
}

void raw_writer::operator()(const node_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_PARENT), obj->parent);
   write_when_not_null(STOK(TOK_PREDICATE), obj->predicate);
   WRITE_NFIELD(os, STOK(TOK_BB_INDEX), obj->bb_index);

   write_when_not_null(STOK(TOK_MEMUSE), obj->memuse);
   write_when_not_null(STOK(TOK_MEMDEF), obj->memdef);

   for(const auto& vuse : obj->vuses)
   {
      write_when_not_null(STOK(TOK_VUSE), vuse);
   }

   write_when_not_null(STOK(TOK_VDEF), obj->vdef);
   for(const auto& vover : obj->vovers)
   {
      write_when_not_null(STOK(TOK_VOVER), vover);
   }
   obj->IR_LocInfo::visit(this);
}

void raw_writer::operator()(const unary_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const binary_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
}

void raw_writer::operator()(const ternary_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   write_when_not_null(STOK(TOK_OP), obj->op2);
}

void raw_writer::operator()(const type_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   if(obj->bitsizealloc != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_BITSIZEALLOC), obj->bitsizealloc);
   }
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->system_flag)
   {
      WRITE_TOKEN(os, TOK_SYSTEM);
   }
   if(obj->libbambu_flag)
   {
      WRITE_TOKEN(os, TOK_LIBBAMBU);
   }
}

void raw_writer::operator()(const cst_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
}

void raw_writer::operator()(const array_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_ELTS), obj->elts);
   if(obj->nelements != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_NELEMENTS), obj->nelements);
   }
}

void raw_writer::operator()(const call_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_FN), obj->fn);
   std::vector<ir_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
   {
      write_when_not_null(STOK(TOK_ARG), *arg);
   }
}

void raw_writer::operator()(const call_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->node_stmt::visit(this);
   write_when_not_null(STOK(TOK_FN), obj->fn);
   std::vector<ir_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
   {
      write_when_not_null(STOK(TOK_ARG), *arg);
   }
}

void raw_writer::operator()(const constructor_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   auto vend = obj->list_of_idx_valu.end();
   for(auto i = obj->list_of_idx_valu.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_IDX), i->first);
      write_when_not_null(STOK(TOK_VALU), i->second);
   }
}

void raw_writer::operator()(const field_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   if(obj->bitfield)
   {
      WRITE_TOKEN(os, TOK_BITFIELD);
   }
   if(obj->bitsizealloc != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_BITSIZEALLOC), obj->bitsizealloc);
   }
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   WRITE_NFIELD(os, STOK(TOK_OFFSET), obj->offset);
}

void raw_writer::operator()(const function_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);

   write_when_not_null(STOK(TOK_FN), obj->fn);
   auto vend2 = obj->list_of_args.end();
   for(auto i = obj->list_of_args.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_ARG), *i);
   }

   if(obj->builtin_flag)
   {
      WRITE_TOKEN(os, TOK_BUILTIN);
   }
   if(obj->static_flag)
   {
      WRITE_TOKEN(os, TOK_STATIC);
   }
   if(obj->writing_memory)
   {
      WRITE_TOKEN(os, TOK_WRITING_MEMORY);
   }
   if(obj->reading_memory)
   {
      WRITE_TOKEN(os, TOK_READING_MEMORY);
   }
   if(obj->pipeline_enabled)
   {
      WRITE_TOKEN(os, TOK_PIPELINE_ENABLED);
   }
   if(obj->initiation_time)
   {
      WRITE_NFIELD(os, STOK(TOK_INITIATION_TIME), obj->initiation_time);
   }
   write_when_not_null(STOK(TOK_BODY), obj->body);
}

void raw_writer::operator()(const function_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_RETN), obj->retn);
   auto vend2 = obj->list_of_args_type.end();
   for(auto i = obj->list_of_args_type.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_ARG), *i);
   }
   if(obj->varargs_flag)
   {
      WRITE_TOKEN(os, TOK_VARARGS);
   }
}

void raw_writer::operator()(const assign_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->node_stmt::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   if(obj->temporary_address)
   {
      WRITE_TOKEN(os, TOK_ADDR);
   }
}

void raw_writer::operator()(const identifier_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   WRITE_STRG_IDENTIFIER(os, obj->strg);
}

void raw_writer::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_VALUE), obj->value);
}

void raw_writer::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   if(obj->bitsize != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_BITSIZE), obj->bitsize);
   }
   if(obj->unsigned_flag)
   {
      WRITE_TOKEN(os, TOK_UNSIGNED);
   }
}

void raw_writer::operator()(const argument_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   if(obj->bitsizealloc != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_BITSIZEALLOC), obj->bitsizealloc);
   }
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->readonly_flag)
   {
      WRITE_TOKEN(os, TOK_READONLY);
   }
}

void raw_writer::operator()(const phi_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->node_stmt::visit(this);
   write_when_not_null(STOK(TOK_RES), obj->res);
   for(const auto& def_edge : obj->CGetDefEdgesList())
   {
      write_when_not_null(STOK(TOK_DEF), def_edge.first);
      WRITE_NFIELD(os, STOK(TOK_EDGE), def_edge.second);
   }
   if(obj->virtual_flag)
   {
      WRITE_TOKEN(os, TOK_VIRTUAL);
   }
}

void raw_writer::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_PTD), obj->ptd);
}

void raw_writer::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   if(obj->overflow_flag)
   {
      WRITE_TOKEN(os, TOK_OVERFLOW);
   }
   if(!obj->valr.empty())
   {
      WRITE_NFIELD_STRING(os, STOK(TOK_VALR), obj->valr);
   }
   if(!obj->valx.empty())
   {
      WRITE_NFIELD_STRING(os, STOK(TOK_VALX), obj->valx);
   }
}

void raw_writer::operator()(const real_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_BITSIZE), obj->bitsize);
}

void raw_writer::operator()(const struct_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_NAME), obj->name);

   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }

   auto vend1 = obj->list_of_flds.end();
   for(auto i = obj->list_of_flds.begin(); i != vend1; ++i)
   {
      write_when_not_null(STOK(TOK_FLDS), *i);
   }
}

void raw_writer::operator()(const return_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->node_stmt::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const ssa_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_VAR), obj->var);
   if(obj->vers != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_VERS), obj->vers);
   }
   write_when_not_null_point_to("use", obj->use_set);
   const auto& def_stmt = obj->GetDefStmt();
   write_when_not_null(STOK(TOK_DEF_STMT), def_stmt);
   for(const auto& use_stmt : obj->CGetUseStmts())
   {
      for(size_t counter = 0; counter < use_stmt.second; counter++)
      {
         write_when_not_null(STOK(TOK_USE_STMT), use_stmt.first);
      }
   }
   if(obj->virtual_flag)
   {
      WRITE_TOKEN(os, TOK_VIRTUAL);
   }
   if(obj->default_flag)
   {
      WRITE_TOKEN(os, TOK_DEFAULT);
   }
   write_when_not_null(STOK(TOK_MIN), obj->min);
   write_when_not_null(STOK(TOK_MAX), obj->max);
   if(!obj->bit_values.empty())
   {
      WRITE_NFIELD(os, STOK(TOK_BIT_VALUES), obj->bit_values);
   }
}

void raw_writer::operator()(const statement_list_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->ir_node::visit(this);
   auto mend = obj->list_of_bloc.end();
   for(auto i = obj->list_of_bloc.begin(); i != mend; ++i)
   {
      write_when_not_null_bloc(STOK(TOK_BLOC), i->second);
   }
}

void raw_writer::operator()(const lut_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP0), obj->op0);
   write_when_not_null(STOK(TOK_OP1), obj->op1);
   write_when_not_null(STOK(TOK_OP2), obj->op2);
   write_when_not_null(STOK(TOK_OP3), obj->op3);
   write_when_not_null(STOK(TOK_OP4), obj->op4);
   write_when_not_null(STOK(TOK_OP5), obj->op5);
   write_when_not_null(STOK(TOK_OP6), obj->op6);
   write_when_not_null(STOK(TOK_OP7), obj->op7);
   write_when_not_null(STOK(TOK_OP8), obj->op8);
}

void raw_writer::operator()(const variable_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);

   if(obj->extern_flag)
   {
      WRITE_TOKEN(os, TOK_EXTERN);
   }
   if(obj->static_flag)
   {
      WRITE_TOKEN(os, TOK_STATIC);
   }
   write_when_not_null(STOK(TOK_INIT), obj->init);
   if(obj->bitsizealloc != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_BITSIZEALLOC), obj->bitsizealloc);
   }
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->readonly_flag)
   {
      WRITE_TOKEN(os, TOK_READONLY);
   }
   if(!obj->bit_values.empty())
   {
      WRITE_NFIELD(os, STOK(TOK_BIT_VALUES), obj->bit_values);
   }
   if(obj->addr_not_taken)
   {
      WRITE_TOKEN(os, TOK_ADDR_NOT_TAKEN);
   }
}

void raw_writer::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   auto vend = obj->list_of_valu.end();
   for(auto i = obj->list_of_valu.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_VALU), *i);
   }
}

void raw_writer::operator()(const vector_ty_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_ELTS), obj->elts);
}

void raw_writer::operator()(const bloc* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   WRITE_UFIELD(os, obj->number);
   WRITE_NFIELD(os, STOK(TOK_LOOP_ID), obj->loop_id);
   auto vend1 = obj->list_of_pred.end();
   for(auto i = obj->list_of_pred.begin(); i != vend1; ++i)
   {
      if(*i == bloc::ENTRY_BLOCK_ID)
      {
         WRITE_NFIELD(os, STOK(TOK_PRED), STOK(TOK_ENTRY));
      }
      else
      {
         WRITE_NFIELD(os, STOK(TOK_PRED), *i);
      }
   }
   auto vend2 = obj->list_of_succ.end();
   for(auto i = obj->list_of_succ.begin(); i != vend2; ++i)
   {
      if(*i == bloc::EXIT_BLOCK_ID)
      {
         WRITE_NFIELD(os, STOK(TOK_SUCC), STOK(TOK_EXIT));
      }
      else
      {
         WRITE_NFIELD(os, STOK(TOK_SUCC), *i);
      }
   }
   for(const auto& phi : obj->CGetPhiList())
   {
      write_when_not_null(STOK(TOK_PHI), phi);
   }
   for(const auto& stmt : obj->CGetStmtList())
   {
      write_when_not_null(STOK(TOK_STMT), stmt);
   }
}

void raw_writer::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->node_stmt::visit(this);
   for(const auto& cond : obj->list_of_cond)
   {
      write_when_not_null(STOK(TOK_OP), cond.first);
      WRITE_NFIELD(os, STOK(TOK_BLOC), cond.second);
   }
}
