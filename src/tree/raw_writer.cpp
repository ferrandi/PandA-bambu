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
 * @file raw_writer.cpp
 * @brief tree node writer. This class exploiting the visitor design pattern write a tree node according to the raw format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Autoheader include
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp"
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_HAVE_RTL_BUILT.hpp"

/// Header include
#include "raw_writer.hpp"

#include "custom_map.hpp" // for map, map<>::cons...
#include "custom_set.hpp" // for unordered_set<>:...
#include <cstddef>        // for size_t
#include <list>           // for list, list<>::co...
#include <utility>        // for pair
#include <vector>         // for vector, vector<>...

#include "exceptions.hpp" // for THROW_ERROR

/// parser/treegcc include
#include "token_interface.hpp"

#if HAVE_RTL_BUILT
#include "rtl_node.hpp"
#endif

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#if HAVE_CODE_ESTIMATION_BUILT
#include "weight_information.hpp"
#endif

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
 * Macro which writes on an output stream a string with its length. IDENTIFIER_NODE case
 */
#define WRITE_STRGLNGT_IDENTIFIER(os, field) \
   os << " "                                 \
      << "strg: \"" << (field) << "\" lngt: " << (field).size()

/**
 * Macro which writes on an output stream a string with its length. STRING_CST case
 */
#define WRITE_STRGLNGT_STRING(os, field) \
   (os << " "                            \
       << "strg: \"" << (field) << "\" lngt: " << (field).size() + 1)

/**
 * Macro which writes on an output stream the srcp fields.
 */
#define WRITE_SRCP(os, include_name, line_number, column_number) \
   os << " "                                                     \
      << "srcp: \"" << (include_name) << "\":" << (line_number) << ":" << column_number

raw_writer::raw_writer(
#if HAVE_MAPPING_BUILT
    const ComponentTypeRef& _driving_component,
#endif
    std::ostream& _os)
    :
#if HAVE_MAPPING_BUILT
      driving_component(_driving_component),
#endif
      os(_os)
{
}

void raw_writer::write_when_not_null(const std::string& str, const tree_nodeRef& t) const
{
   if(t)
   {
      os << " " << str << ": @" << GET_INDEX_NODE(t);
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

void raw_writer::write_when_not_null_point_to(const std::string& type, const PointToSolutionRef& solution) const
{
   if(solution->anything)
   {
      os << " " << type << " : \"anything\"";
   }
   if(solution->escaped)
   {
      os << " " << type << " : \"escaped\"";
   }
   if(solution->ipa_escaped)
   {
      os << " " << type << " : \"ipa_escaped\"";
   }
   if(solution->nonlocal)
   {
      os << " " << type << " : \"nonlocal\"";
   }
   if(solution->null)
   {
      os << " " << type << " : \"null\"";
   }
   const std::vector<tree_nodeRef>& variables = solution->variables;
   std::vector<tree_nodeRef>::const_iterator variable, variable_end = variables.end();
   for(variable = variables.begin(); variable != variable_end; ++variable)
   {
      write_when_not_null(type + "_vars", *variable);
   }
}

void raw_writer::operator()(const tree_node* obj, unsigned int&)
{
   os << obj->get_kind_text();
}

void raw_writer::operator()(const WeightedNode* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   /// TODO: these fields should be printed by this method. At the moment it is not possible because of ordering of fields produced by gcc
   // WRITE_NFIELD(os, STOK(TOK_TIME_WEIGHT), obj->recursive_weight);
   // WRITE_NFIELD(os, STOK(TOK_SIZE_WEIGHT), obj->instruction_size);
}

void raw_writer::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void raw_writer::operator()(const attr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   for(const auto attr : obj->list_attr)
   {
      WRITE_TOKEN2(os, attr);
   }
}

void raw_writer::operator()(const srcp* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->line_number || obj->column_number || !obj->include_name.empty())
   {
      WRITE_SRCP(os, obj->include_name, obj->line_number, obj->column_number);
   }
}

void raw_writer::operator()(const decl_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_NAME), obj->name);
   write_when_not_null(STOK(TOK_MNGL), obj->mngl);
   write_when_not_null(STOK(TOK_ORIG), obj->orig);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_SCPE), obj->scpe);
   obj->srcp::visit(this);
   write_when_not_null(STOK(TOK_ATTRIBUTES), obj->attributes);
   if(obj->artificial_flag)
   {
      WRITE_TOKEN(os, TOK_ARTIFICIAL);
   }
   if(obj->operating_system_flag)
   {
      WRITE_TOKEN(os, TOK_OPERATING_SYSTEM);
   }
   if(obj->library_system_flag)
   {
      WRITE_TOKEN(os, TOK_LIBRARY_SYSTEM);
   }
#if HAVE_BAMBU_BUILT
   if(obj->libbambu_flag)
   {
      WRITE_TOKEN(os, TOK_LIBBAMBU);
   }
#endif
   write_when_not_null(STOK(TOK_CHAN), obj->chan);
   if(obj->C_flag)
   {
      WRITE_TOKEN(os, TOK_C);
   }
   if(obj->uid != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_UID), obj->uid);
   }
}

void raw_writer::operator()(const expr_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   obj->srcp::visit(this);
#if HAVE_CODE_ESTIMATION_BUILT
   obj->weight_information->recursive_weight.find(driving_component);
   if(obj->weight_information->recursive_weight.find(driving_component) != obj->weight_information->recursive_weight.end())
      WRITE_NFIELD(os, STOK(TOK_TIME_WEIGHT), obj->weight_information->recursive_weight.find(driving_component)->second);
   if(obj->weight_information->instruction_size)
      WRITE_NFIELD(os, STOK(TOK_SIZE_WEIGHT), obj->weight_information->instruction_size);
#endif
#if HAVE_CODE_ESTIMATION_BUILT && HAVE_RTL_BUILT
   write_when_not_null_rtl(obj->weight_information->rtl_nodes);
#endif
}

void raw_writer::operator()(const gimple_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   write_when_not_null(STOK(TOK_SCPE), obj->scpe);
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
   obj->srcp::visit(this);
#if HAVE_CODE_ESTIMATION_BUILT
   if(obj->weight_information->recursive_weight.find(driving_component) != obj->weight_information->recursive_weight.end())
      WRITE_NFIELD(os, STOK(TOK_TIME_WEIGHT), obj->weight_information->recursive_weight.find(driving_component)->second);
   if(obj->weight_information->instruction_size)
      WRITE_NFIELD(os, STOK(TOK_SIZE_WEIGHT), obj->weight_information->instruction_size);
#endif
   auto vend2 = obj->pragmas.end();
   for(auto i = obj->pragmas.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_PRAGMA), *i);
   }
#if HAVE_CODE_ESTIMATION_BUILT && HAVE_RTL_BUILT
   write_when_not_null_rtl(obj->weight_information->rtl_nodes);
#endif
}

void raw_writer::operator()(const unary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const binary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
}

void raw_writer::operator()(const ternary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   write_when_not_null(STOK(TOK_OP), obj->op2);
}

void raw_writer::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   write_when_not_null(STOK(TOK_OP), obj->op2);
   write_when_not_null(STOK(TOK_OP), obj->op3);
}

void raw_writer::operator()(const type_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->qual != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
   {
      WRITE_NFIELD(os, STOK(TOK_QUAL), TI_getTokenName(obj->qual));
   }
   write_when_not_null(STOK(TOK_NAME), obj->name);
   write_when_not_null(STOK(TOK_UNQL), obj->unql);
   write_when_not_null(STOK(TOK_SIZE), obj->size);
   write_when_not_null(STOK(TOK_SCPE), obj->scpe);
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   if(obj->system_flag)
   {
      WRITE_TOKEN(os, TOK_SYSTEM);
   }
#if HAVE_BAMBU_BUILT
   if(obj->libbambu_flag)
   {
      WRITE_TOKEN(os, TOK_LIBBAMBU);
   }
#endif
}

void raw_writer::operator()(const memory_tag* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   auto vend = obj->list_of_aliases.end();
   for(auto i = obj->list_of_aliases.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_ALIAS), *i);
   }
}

void raw_writer::operator()(const cst_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
}

void raw_writer::operator()(const error_mark* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const array_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_ELTS), obj->elts);
   write_when_not_null(STOK(TOK_DOMN), obj->domn);
}

void raw_writer::operator()(const gimple_asm* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   if(obj->volatile_flag)
   {
      WRITE_TOKEN(os, TOK_VOLATILE);
   }
   WRITE_NFIELD_STRING(os, STOK(TOK_STR), obj->str);
   write_when_not_null(STOK(TOK_OUT), obj->out);
   write_when_not_null(STOK(TOK_IN), obj->in);
   write_when_not_null(STOK(TOK_CLOB), obj->clob);
}

void raw_writer::operator()(const baselink* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
}

void raw_writer::operator()(const gimple_bind* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   auto vend = obj->list_of_vars.end();
   for(auto i = obj->list_of_vars.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_VARS), *i);
   }
   write_when_not_null(STOK(TOK_BODY), obj->body);
}

void raw_writer::operator()(const binfo* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   if(obj->virt_flag)
   {
      WRITE_TOKEN(os, TOK_VIRT);
   }
   WRITE_NFIELD(os, STOK(TOK_BASES), obj->bases);
   auto vend = obj->list_of_access_binf.end();
   for(auto i = obj->list_of_access_binf.begin(); i != vend; ++i)
   {
      WRITE_TOKEN2(os, i->first);
      write_when_not_null(STOK(TOK_BINF), i->second);
   }
}

void raw_writer::operator()(const block* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->bl_flag)
   {
      WRITE_UFIELD_STRING(os, obj->bl);
   }
}

void raw_writer::operator()(const call_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_FN), obj->fn);
   std::vector<tree_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
   {
      write_when_not_null(STOK(TOK_ARG), *arg);
   }
}

void raw_writer::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->call_expr::visit(this);
   WRITE_NFIELD(os, STOK(TOK_CTOR), obj->ctor);
   write_when_not_null(STOK(TOK_SLOT), obj->slot);
}

void raw_writer::operator()(const gimple_call* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_FN), obj->fn);
   std::vector<tree_nodeRef>::const_iterator arg, arg_end = obj->args.end();
   for(arg = obj->args.begin(); arg != arg_end; ++arg)
   {
      write_when_not_null(STOK(TOK_ARG), *arg);
   }
   write_when_not_null_point_to("use", obj->use_set);
   write_when_not_null_point_to("clb", obj->clobbered_set);
}

void raw_writer::operator()(const case_label_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   if(obj->default_flag)
   {
      WRITE_TOKEN(os, TOK_DEFAULT);
   }
   write_when_not_null(STOK(TOK_GOTO), obj->got);
}

void raw_writer::operator()(const cast_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const complex_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   write_when_not_null(STOK(TOK_REAL), obj->real);
   write_when_not_null(STOK(TOK_IMAG), obj->imag);
}

void raw_writer::operator()(const complex_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   if(obj->unsigned_flag)
   {
      WRITE_TOKEN(os, TOK_UNSIGNED);
   }
   if(obj->real_flag)
   {
      WRITE_TOKEN(os, TOK_REAL);
   }
}

void raw_writer::operator()(const gimple_cond* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
}

void raw_writer::operator()(const const_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_CNST), obj->cnst);
}

void raw_writer::operator()(const constructor* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   auto vend = obj->list_of_idx_valu.end();
   for(auto i = obj->list_of_idx_valu.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_IDX), i->first);
      write_when_not_null(STOK(TOK_VALU), i->second);
   }
}

void raw_writer::operator()(const enumeral_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   if(obj->prec != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_PREC), obj->prec);
   }
   if(obj->unsigned_flag)
   {
      WRITE_TOKEN(os, TOK_UNSIGNED);
   }
   write_when_not_null(STOK(TOK_MIN), obj->min);
   write_when_not_null(STOK(TOK_MAX), obj->max);
   write_when_not_null(STOK(TOK_CSTS), obj->csts);
}

void raw_writer::operator()(const expr_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->line != -1)
   {
      WRITE_NFIELD(os, STOK(TOK_LINE), obj->line);
   }
   write_when_not_null(STOK(TOK_EXPR), obj->expr);
   write_when_not_null(STOK(TOK_NEXT), obj->next);
}

void raw_writer::operator()(const field_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   obj->attr::visit(this);
   write_when_not_null(STOK(TOK_INIT), obj->init);
   write_when_not_null(STOK(TOK_SIZE), obj->size);
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   write_when_not_null(STOK(TOK_BPOS), obj->bpos);
   write_when_not_null(STOK(TOK_SMT_ANN), obj->smt_ann);
}

void raw_writer::operator()(const function_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   if(obj->operator_flag)
   {
      WRITE_TOKEN(os, TOK_OPERATOR);
   }
   auto vend = obj->list_of_op_names.end();
   for(auto i = obj->list_of_op_names.begin(); i != vend; ++i)
   {
      WRITE_UFIELD_STRING(os, *i);
   }
   obj->attr::visit(this);
   write_when_not_null(STOK(TOK_TMPL_PARMS), obj->tmpl_parms);
   write_when_not_null(STOK(TOK_TMPL_ARGS), obj->tmpl_args);

   if(obj->fixd_flag)
   {
      WRITE_NFIELD(os, STOK(TOK_FIXD), obj->fixd);
   }
   if(obj->virt_flag)
   {
      WRITE_NFIELD(os, STOK(TOK_VIRT), obj->virt);
   }
   write_when_not_null(STOK(TOK_FN), obj->fn);
   auto vend2 = obj->list_of_args.end();
   for(auto i = obj->list_of_args.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_ARG), *i);
   }

   if(obj->undefined_flag)
   {
      WRITE_TOKEN(os, TOK_UNDEFINED);
   }
   if(obj->builtin_flag)
   {
      WRITE_TOKEN(os, TOK_BUILTIN);
   }
   if(obj->static_flag)
   {
      WRITE_TOKEN(os, TOK_STATIC);
   }
   if(obj->hwcall_flag)
   {
      WRITE_TOKEN(os, TOK_HWCALL);
   }
   if(obj->reverse_restrict_flag)
   {
      WRITE_TOKEN(os, TOK_REVERSE_RESTRICT);
   }
   if(obj->writing_memory)
   {
      WRITE_TOKEN(os, TOK_WRITING_MEMORY);
   }
   if(obj->reading_memory)
   {
      WRITE_TOKEN(os, TOK_READING_MEMORY);
   }
#if HAVE_FROM_PRAGMA_BUILT
   if(obj->omp_atomic)
   {
      WRITE_TOKEN(os, TOK_OMP_ATOMIC);
   }
   if(obj->omp_for_wrapper)
   {
      WRITE_TOKEN(os, TOK_OMP_FOR_WRAPPER);
   }
   if(obj->omp_body_loop)
   {
      WRITE_TOKEN(os, TOK_OMP_BODY_LOOP);
   }
   if(!obj->omp_critical.empty())
   {
      WRITE_UFIELD_STRING(os, obj->omp_critical);
   }
#endif
   write_when_not_null(STOK(TOK_BODY), obj->body);
   write_when_not_null(STOK(TOK_INLINE_BODY), obj->inline_body);
}

void raw_writer::operator()(const function_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_RETN), obj->retn);
   write_when_not_null(STOK(TOK_PRMS), obj->prms);
   if(obj->varargs_flag)
   {
      WRITE_TOKEN(os, TOK_VARARGS);
   }
}

void raw_writer::operator()(const gimple_assign* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   write_when_not_null(STOK(TOK_PREDICATE), obj->predicate);
   write_when_not_null(STOK(TOK_ORIG), obj->orig);
   if(obj->init_assignment)
   {
      WRITE_TOKEN(os, TOK_INIT);
   }
   write_when_not_null_point_to("use", obj->use_set);
   write_when_not_null_point_to("clb", obj->clobbered_set);
   if(obj->clobber)
   {
      WRITE_TOKEN(os, TOK_CLOBBER);
   }
   if(obj->temporary_address)
   {
      WRITE_TOKEN(os, TOK_ADDR);
   }
}

void raw_writer::operator()(const gimple_goto* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const handler* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->line != -1)
   {
      WRITE_NFIELD(os, STOK(TOK_LINE), obj->line);
   }
   write_when_not_null(STOK(TOK_BODY), obj->body);
}

void raw_writer::operator()(const identifier_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->operator_flag)
   {
      WRITE_TOKEN(os, TOK_OPERATOR);
   }
   else
   {
      WRITE_STRGLNGT_IDENTIFIER(os, obj->strg);
   }
}

void raw_writer::operator()(const integer_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_VALUE), obj->value);
}

void raw_writer::operator()(const integer_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   if(obj->prec != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_PREC), obj->prec);
   }
   if(!obj->str.empty())
   {
      WRITE_UFIELD(os, obj->str);
   }
   if(obj->unsigned_flag)
   {
      WRITE_TOKEN(os, TOK_UNSIGNED);
   }
   write_when_not_null(STOK(TOK_MIN), obj->min);
   write_when_not_null(STOK(TOK_MAX), obj->max);
}

void raw_writer::operator()(const gimple_label* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const method_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->function_type::visit(this);
   write_when_not_null(STOK(TOK_CLAS), obj->clas);
}

void raw_writer::operator()(const namespace_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_DCLS), obj->dcls);
}

void raw_writer::operator()(const overload* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_CRNT), obj->crnt);
   write_when_not_null(STOK(TOK_CHAN), obj->chan);
}

void raw_writer::operator()(const parm_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_ARGT), obj->argt);
   write_when_not_null(STOK(TOK_SIZE), obj->size);
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   WRITE_NFIELD(os, STOK(TOK_USED), obj->used);
   if(obj->register_flag)
   {
      WRITE_TOKEN(os, TOK_REGISTER);
   }
   if(obj->readonly_flag)
   {
      WRITE_TOKEN(os, TOK_READONLY);
   }
   write_when_not_null(STOK(TOK_SMT_ANN), obj->smt_ann);
}

void raw_writer::operator()(const gimple_phi* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
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

void raw_writer::operator()(const pointer_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_PTD), obj->ptd);
}

void raw_writer::operator()(const real_cst* obj, unsigned int& mask)
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

void raw_writer::operator()(const real_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_PREC), obj->prec);
}

void raw_writer::operator()(const record_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_TMPL_PARMS), obj->tmpl_parms);
   write_when_not_null(STOK(TOK_TMPL_ARGS), obj->tmpl_args);

   if(obj->ptrmem_flag)
   {
      WRITE_TOKEN(os, TOK_PTRMEM);
   }
   write_when_not_null(STOK(TOK_PTD), obj->ptd);
   write_when_not_null(STOK(TOK_CLS), obj->cls);
   write_when_not_null(STOK(TOK_BFLD), obj->bfld);
   write_when_not_null(STOK(TOK_VFLD), obj->vfld);
   if(obj->spec_flag)
   {
      WRITE_TOKEN(os, TOK_SPEC);
   }
   if(obj->struct_flag)
   {
      WRITE_TOKEN(os, TOK_STRUCT);
   }
   auto vend1 = obj->list_of_flds.end();
   for(auto i = obj->list_of_flds.begin(); i != vend1; ++i)
   {
      write_when_not_null(STOK(TOK_FLDS), *i);
   }
   auto vend2 = obj->list_of_fncs.end();
   for(auto i = obj->list_of_fncs.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_FNCS), *i);
   }
   write_when_not_null(STOK(TOK_BINF), obj->binf);
}

void raw_writer::operator()(const reference_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_REFD), obj->refd);
}

void raw_writer::operator()(const result_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_INIT), obj->init);
   write_when_not_null(STOK(TOK_SIZE), obj->size);
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   write_when_not_null(STOK(TOK_SMT_ANN), obj->smt_ann);
}

void raw_writer::operator()(const gimple_return* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
}

void raw_writer::operator()(const return_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->line != -1)
   {
      WRITE_NFIELD(os, STOK(TOK_LINE), obj->line);
   }
   write_when_not_null(STOK(TOK_EXPR), obj->expr);
}

void raw_writer::operator()(const scope_ref* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
}

void raw_writer::operator()(const ssa_name* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_VAR), obj->var);
   if(obj->vers != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_VERS), obj->vers);
   }
   if(obj->orig_vers != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ORIG_VERS), obj->orig_vers);
   }
   write_when_not_null_point_to("use", obj->use_set);
   if(obj->volatile_flag)
   {
      WRITE_TOKEN(os, TOK_VOLATILE);
   }
   else
   {
      for(const auto& def_stmt : obj->CGetDefStmts())
      {
         write_when_not_null(STOK(TOK_DEF_STMT), def_stmt);
      }
   }
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

void raw_writer::operator()(const statement_list* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   auto vend = obj->list_of_stmt.end();
   for(auto i = obj->list_of_stmt.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_STMT), *i);
   }
   auto mend = obj->list_of_bloc.end();
   for(auto i = obj->list_of_bloc.begin(); i != mend; ++i)
   {
      write_when_not_null_bloc(STOK(TOK_BLOC), i->second);
   }
}

void raw_writer::operator()(const string_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   WRITE_STRGLNGT_STRING(os, obj->strg);
}

void raw_writer::operator()(const gimple_switch* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
}

void raw_writer::operator()(const target_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_DECL), obj->decl);
   write_when_not_null(STOK(TOK_INIT), obj->init);
   write_when_not_null(STOK(TOK_CLNP), obj->clnp);
}

void raw_writer::operator()(const lut_expr* obj, unsigned int& mask)
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

void raw_writer::operator()(const template_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_RSLT), obj->rslt);
   write_when_not_null(STOK(TOK_INST), obj->inst);
   write_when_not_null(STOK(TOK_SPCS), obj->spcs);
   write_when_not_null(STOK(TOK_PRMS), obj->prms);
}

void raw_writer::operator()(const template_parm_index* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_DECL), obj->decl);
   if(obj->constant_flag)
   {
      WRITE_TOKEN(os, TOK_CONSTANT);
   }
   if(obj->readonly_flag)
   {
      WRITE_TOKEN(os, TOK_READONLY);
   }
   WRITE_NFIELD(os, STOK(TOK_IDX), obj->idx);
   WRITE_NFIELD(os, STOK(TOK_LEVEL), obj->level);
   WRITE_NFIELD(os, STOK(TOK_ORIG_LEVEL), obj->orig_level);
}

void raw_writer::operator()(const tree_list* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   write_when_not_null(STOK(TOK_PURP), obj->purp);
   write_when_not_null(STOK(TOK_VALU), obj->valu);
   write_when_not_null(STOK(TOK_CHAN), obj->chan);
}

void raw_writer::operator()(const tree_vec* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->lngt != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_LNGT), obj->lngt);
   }
   auto vend = obj->list_of_op.end();
   for(auto i = obj->list_of_op.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_OP), *i);
   }
}

void raw_writer::operator()(const try_block* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   if(obj->line != -1)
   {
      WRITE_NFIELD(os, STOK(TOK_LINE), obj->line);
   }
   write_when_not_null(STOK(TOK_BODY), obj->body);
   write_when_not_null(STOK(TOK_HDLR), obj->hdlr);
   write_when_not_null(STOK(TOK_NEXT), obj->next);
}

void raw_writer::operator()(const type_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
   write_when_not_null(STOK(TOK_TMPL_PARMS), obj->tmpl_parms);
   write_when_not_null(STOK(TOK_TMPL_ARGS), obj->tmpl_args);
}

void raw_writer::operator()(const union_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   WRITE_TOKEN(os, TOK_UNION);
   auto vend1 = obj->list_of_flds.end();
   for(auto i = obj->list_of_flds.begin(); i != vend1; ++i)
   {
      write_when_not_null(STOK(TOK_FLDS), *i);
   }
   auto vend2 = obj->list_of_fncs.end();
   for(auto i = obj->list_of_fncs.begin(); i != vend2; ++i)
   {
      write_when_not_null(STOK(TOK_FNCS), *i);
   }
   write_when_not_null(STOK(TOK_BINF), obj->binf);
}

void raw_writer::operator()(const var_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);

   if(obj->use_tmpl != -1)
   {
      WRITE_NFIELD(os, STOK(TOK_USE_TMPL), obj->use_tmpl);
   }
   obj->attr::visit(this);
   if(obj->static_static_flag)
   {
      WRITE_TOKEN(os, TOK_STATIC);
      WRITE_TOKEN(os, TOK_STATIC);
   }
   if(obj->extern_flag)
   {
      WRITE_TOKEN(os, TOK_EXTERN);
   }
   if(obj->static_flag)
   {
      WRITE_TOKEN(os, TOK_STATIC);
   }
   write_when_not_null(STOK(TOK_INIT), obj->init);
   write_when_not_null(STOK(TOK_SIZE), obj->size);
   if(obj->algn != 0)
   {
      WRITE_NFIELD(os, STOK(TOK_ALGN), obj->algn);
   }
   if(obj->packed_flag)
   {
      WRITE_TOKEN(os, TOK_PACKED);
   }
   WRITE_NFIELD(os, STOK(TOK_USED), obj->used);
   if(obj->register_flag)
   {
      WRITE_TOKEN(os, TOK_REGISTER);
   }
   if(obj->readonly_flag)
   {
      WRITE_TOKEN(os, TOK_READONLY);
   }
   if(!obj->bit_values.empty())
   {
      WRITE_NFIELD(os, STOK(TOK_BIT_VALUES), obj->bit_values);
   }
   write_when_not_null(STOK(TOK_SMT_ANN), obj->smt_ann);
   CustomUnorderedSet<tree_nodeRef>::const_iterator var, var_end;
   var_end = obj->defs.end();
   for(var = obj->defs.begin(); var != var_end; ++var)
   {
      write_when_not_null(STOK(TOK_DEF_STMT), *var);
   }
   var_end = obj->uses.end();
   for(var = obj->uses.begin(); var != var_end; ++var)
   {
      write_when_not_null(STOK(TOK_USE_STMT), *var);
   }
   var_end = obj->addressings.end();
   for(var = obj->addressings.begin(); var != var_end; ++var)
   {
      write_when_not_null(STOK(TOK_ADDR_STMT), *var);
   }
   if(obj->addr_taken)
   {
      WRITE_TOKEN(os, TOK_ADDR_TAKEN);
   }
   if(obj->addr_not_taken)
   {
      WRITE_TOKEN(os, TOK_ADDR_NOT_TAKEN);
   }
}

void raw_writer::operator()(const vector_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
   auto vend = obj->list_of_valu.end();
   for(auto i = obj->list_of_valu.begin(); i != vend; ++i)
   {
      write_when_not_null(STOK(TOK_VALU), *i);
   }
}

void raw_writer::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_ARG), obj->arg);
}

void raw_writer::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_ARG), obj->arg);
}

void raw_writer::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
   write_when_not_null(STOK(TOK_PARAM_PACKS), obj->param_packs);
   write_when_not_null(STOK(TOK_ARG), obj->arg);
}

void raw_writer::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op);
   write_when_not_null(STOK(TOK_PARAM_PACKS), obj->param_packs);
   write_when_not_null(STOK(TOK_ARG), obj->arg);
}

void raw_writer::operator()(const vector_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
   write_when_not_null(STOK(TOK_ELTS), obj->elts);
}

void raw_writer::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_SYMBOL), obj->symbol);
   write_when_not_null(STOK(TOK_BASE), obj->base);
   write_when_not_null(STOK(TOK_IDX), obj->idx);
   write_when_not_null(STOK(TOK_STEP), obj->step);
   write_when_not_null(STOK(TOK_OFFSET), obj->offset);
   write_when_not_null(STOK(TOK_ORIG), obj->orig);
   write_when_not_null(STOK(TOK_TAG), obj->tag);
}

void raw_writer::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   write_when_not_null(STOK(TOK_TYPE), obj->type);
   write_when_not_null(STOK(TOK_BASE), obj->base);
   write_when_not_null(STOK(TOK_OFFSET), obj->offset);
   write_when_not_null(STOK(TOK_IDX), obj->idx);
   write_when_not_null(STOK(TOK_STEP), obj->step);
   write_when_not_null(STOK(TOK_IDX2), obj->idx2);
}

void raw_writer::operator()(const bloc* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   WRITE_UFIELD(os, obj->number);
   if(obj->hpl)
   {
      WRITE_TOKEN(os, TOK_HPL);
   }
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
   if(obj->true_edge > 0)
   {
      WRITE_NFIELD(os, STOK(TOK_TRUE_EDGE), obj->true_edge);
   }
   if(obj->false_edge > 0)
   {
      WRITE_NFIELD(os, STOK(TOK_FALSE_EDGE), obj->false_edge);
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

void raw_writer::operator()(const gimple_while* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
}

void raw_writer::operator()(const gimple_for* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_while::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op1);
   write_when_not_null(STOK(TOK_OP), obj->op2);
}

void raw_writer::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   for(const auto& cond : obj->list_of_cond)
   {
      write_when_not_null(STOK(TOK_OP), cond.first);
      WRITE_NFIELD(os, STOK(TOK_BLOC), cond.second);
   }
}

void raw_writer::operator()(const null_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_IS_BLOCK), (obj->is_block ? "true" : "false"));
   WRITE_NFIELD(os, STOK(TOK_OPEN), (obj->is_opening ? "true" : "false"));
   WRITE_NFIELD(os, STOK(TOK_LINE), obj->line);
   write_when_not_null(STOK(TOK_PRAGMA_SCOPE), obj->scope);
   write_when_not_null(STOK(TOK_PRAGMA_DIRECTIVE), obj->directive);
}

void raw_writer::operator()(const omp_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
   WRITE_NFIELD(os, STOK(TOK_PRAGMA_OMP_SHORTCUT), (obj->is_shortcut ? "true" : "false"));
}

void raw_writer::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
   WRITE_NFIELD(os, STOK(TOK_PRAGMA_OMP_SHORTCUT), (obj->is_shortcut ? "true" : "false"));
}

void raw_writer::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
   write_when_not_null(STOK(TOK_OP), obj->op0);
   write_when_not_null(STOK(TOK_OP), obj->op1);
}

void raw_writer::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void raw_writer::operator()(const map_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_HW_COMPONENT), obj->HW_component);
   WRITE_NFIELD(os, STOK(TOK_ID_IMPLEMENTATION), obj->ID_implementation);
}

void raw_writer::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   WRITE_NFIELD(os, STOK(TOK_HW_COMPONENT), obj->HW_component);
   WRITE_NFIELD(os, STOK(TOK_ID_IMPLEMENTATION), obj->ID_implementation);
   WRITE_NFIELD(os, STOK(TOK_RECURSIVE), obj->recursive);
}

void raw_writer::operator()(const issue_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->issue_pragma::visit(this);
}

void raw_writer::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void raw_writer::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->profiling_pragma::visit(this);
}
