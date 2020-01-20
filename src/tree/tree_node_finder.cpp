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
 * @file tree_node_finder.cpp
 * @brief tree node finder. This class exploiting the visitor design pattern find a tree node in a tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "tree_node_finder.hpp"
#include "exceptions.hpp"         // for THROW_ASSERT
#include "token_interface.hpp"    // for TOK, STOK, TreeV...
#include <boost/lexical_cast.hpp> // for lexical_cast

#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

template <class type>
static bool check_value_opt(const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_element, const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_end, const type& value)
{
   return it_element == it_end || value == boost::lexical_cast<type>(it_element->second);
}

static bool check_value_opt(const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_element, const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_end,
                            const TreeVocabularyTokenTypes_TokenEnum& value)
{
   return it_element == it_end || value == static_cast<TreeVocabularyTokenTypes_TokenEnum>(boost::lexical_cast<unsigned int>(it_element->second));
}

#define CHECK_VALUE_OPT(token, value) check_value_opt(tree_node_schema.find(TOK(token)), tree_node_schema.end(), value)

static bool check_tree_node_opt(const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_element, const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>::const_iterator& it_end, const tree_nodeRef& tn,
                                const std::string&)
{
   return it_element == it_end || (tn && GET_INDEX_NODE(tn) == boost::lexical_cast<unsigned int>(it_element->second));
}

#define CHECK_TREE_NODE_OPT(token, treeN) check_tree_node_opt(tree_node_schema.find(TOK(token)), tree_node_schema.end(), treeN, STOK(token))

#define TREE_NOT_YET_IMPLEMENTED(token) THROW_ASSERT(tree_node_schema.find(TOK(token)) == tree_node_schema.end(), std::string("field not yet supported ") + STOK(token))

void tree_node_finder::operator()(const tree_node* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_finder::operator()(const WeightedNode* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_finder::operator()(const attr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_ATTRIBUTES);
}

void tree_node_finder::operator()(const srcp* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && (tree_node_schema.find(TOK(TOK_SRCP)) == tree_node_schema.end() ||
                           tree_node_schema.find(TOK(TOK_SRCP))->second == obj->include_name + ":" + boost::lexical_cast<std::string>(obj->line_number) + ":" + boost::lexical_cast<std::string>(obj->column_number));
}

void tree_node_finder::operator()(const decl_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_TREE_NODE_OPT(TOK_NAME, obj->name) and CHECK_TREE_NODE_OPT(TOK_MNGL, obj->mngl) and CHECK_TREE_NODE_OPT(TOK_ORIG, obj->orig) and CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type) and CHECK_TREE_NODE_OPT(TOK_SCPE, obj->scpe) and
              CHECK_TREE_NODE_OPT(TOK_ATTRIBUTES, obj->attributes) and CHECK_TREE_NODE_OPT(TOK_CHAN, obj->chan) and CHECK_VALUE_OPT(TOK_ARTIFICIAL, obj->artificial_flag) and CHECK_VALUE_OPT(TOK_PACKED, obj->packed_flag) and
              CHECK_VALUE_OPT(TOK_OPERATING_SYSTEM, obj->operating_system_flag) and CHECK_VALUE_OPT(TOK_LIBRARY_SYSTEM, obj->library_system_flag) and
#if HAVE_BAMBU_BUILT
              CHECK_VALUE_OPT(TOK_LIBBAMBU, obj->libbambu_flag) and
#endif
              CHECK_VALUE_OPT(TOK_C, obj->C_flag);
}

void tree_node_finder::operator()(const expr_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type);
}

void tree_node_finder::operator()(const gimple_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_TREE_NODE_OPT(TOK_VUSE, obj->memuse) and CHECK_TREE_NODE_OPT(TOK_VDEF, obj->memdef);
   /// FIXME: check list_of_dep_vuses
}

void tree_node_finder::operator()(const unary_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op);
}

void tree_node_finder::operator()(const binary_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1);
}

void tree_node_finder::operator()(const ternary_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1) && CHECK_TREE_NODE_OPT(TOK_OP2, obj->op2);
}

void tree_node_finder::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1) && CHECK_TREE_NODE_OPT(TOK_OP2, obj->op2) && CHECK_TREE_NODE_OPT(TOK_OP3, obj->op3);
}

void tree_node_finder::operator()(const type_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_QUAL, obj->qual) and CHECK_TREE_NODE_OPT(TOK_NAME, obj->name) and CHECK_TREE_NODE_OPT(TOK_UNQL, obj->unql) and CHECK_TREE_NODE_OPT(TOK_SIZE, obj->size) and CHECK_TREE_NODE_OPT(TOK_SCPE, obj->scpe) and
              CHECK_VALUE_OPT(TOK_PACKED, obj->packed_flag) and CHECK_VALUE_OPT(TOK_SYSTEM, obj->system_flag) and CHECK_VALUE_OPT(TOK_ALGN, obj->algn);
}

void tree_node_finder::operator()(const memory_tag* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_ALIAS);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_aliases.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_aliases.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_ALIAS), *i);
}

void tree_node_finder::operator()(const cst_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type);
}

void tree_node_finder::operator()(const error_mark* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const array_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_ELTS, obj->elts) && CHECK_TREE_NODE_OPT(TOK_DOMN, obj->domn);
}

void tree_node_finder::operator()(const gimple_asm* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_VOLATILE, obj->volatile_flag) && CHECK_VALUE_OPT(TOK_STR, obj->str) && CHECK_TREE_NODE_OPT(TOK_OUT, obj->out) && CHECK_TREE_NODE_OPT(TOK_IN, obj->in) && CHECK_TREE_NODE_OPT(TOK_CLOB, obj->clob);
}

void tree_node_finder::operator()(const baselink* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type);
}

void tree_node_finder::operator()(const gimple_bind* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_VARS);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_vars.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_vars.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VARS), *i);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_BODY, obj->body);
}

void tree_node_finder::operator()(const binfo* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type) && CHECK_VALUE_OPT(TOK_VIRT, obj->virt_flag) && CHECK_VALUE_OPT(TOK_BASES, obj->bases);
   TREE_NOT_YET_IMPLEMENTED(TOK_BINF);
   // std::vector<std::pair< unsigned int, tree_nodeRef> >::const_iterator vend = obj->list_of_access_binf.end();
   // for (std::vector<std::pair< unsigned int, tree_nodeRef> >::const_iterator i = obj->list_of_access_binf.begin(); i != vend; i++)
   //{
   //   WRITE_TOKEN2(os, i->first);
   //   write_when_not_null(STOK(TOK_BINF), i->second);
   //}
}

void tree_node_finder::operator()(const block* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   // if (obj->bl_flag)
   //   WRITE_UFIELD_STRING(os, obj->bl);
}

void tree_node_finder::operator()(const call_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_FN, obj->fn);
   /// FIXME: check args
}

void tree_node_finder::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_CTOR, obj->ctor) && CHECK_TREE_NODE_OPT(TOK_SLOT, obj->slot);
}

void tree_node_finder::operator()(const gimple_call* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_FN, obj->fn);
   /// FIXME: check args
}

void tree_node_finder::operator()(const case_label_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1) && CHECK_VALUE_OPT(TOK_DEFAULT, obj->default_flag) && CHECK_TREE_NODE_OPT(TOK_GOTO, obj->got);
}

void tree_node_finder::operator()(const cast_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op);
}

void tree_node_finder::operator()(const complex_cst* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_REAL, obj->real) && CHECK_TREE_NODE_OPT(TOK_IMAG, obj->imag);
}

void tree_node_finder::operator()(const complex_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_UNSIGNED, obj->unsigned_flag);
   find_res = find_res && CHECK_VALUE_OPT(TOK_REAL, obj->real_flag);
}

void tree_node_finder::operator()(const gimple_cond* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0);
}

void tree_node_finder::operator()(const const_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_CNST, obj->cnst);
}

void tree_node_finder::operator()(const constructor* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type);
   TREE_NOT_YET_IMPLEMENTED(TOK_IDX);
   TREE_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<std::pair< tree_nodeRef, tree_nodeRef> >::const_iterator vend = obj->list_of_idx_valu.end();
   // for (std::vector<std::pair< tree_nodeRef, tree_nodeRef> >::const_iterator i = obj->list_of_idx_valu.begin(); i != vend; i++)
   //{
   //   write_when_not_null(STOK(TOK_IDX), i->first);
   //   write_when_not_null(STOK(TOK_VALU), i->second);
   //}
}

void tree_node_finder::operator()(const enumeral_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_PREC, obj->prec) && CHECK_VALUE_OPT(TOK_UNSIGNED, obj->unsigned_flag) && CHECK_TREE_NODE_OPT(TOK_MIN, obj->min) && CHECK_TREE_NODE_OPT(TOK_MAX, obj->max) && CHECK_TREE_NODE_OPT(TOK_CSTS, obj->csts);
}

void tree_node_finder::operator()(const expr_stmt* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_LINE, obj->line) && CHECK_TREE_NODE_OPT(TOK_EXPR, obj->expr) && CHECK_TREE_NODE_OPT(TOK_NEXT, obj->next);
}

void tree_node_finder::operator()(const field_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) && CHECK_TREE_NODE_OPT(TOK_INIT, obj->init) && CHECK_TREE_NODE_OPT(TOK_SIZE, obj->size) && CHECK_TREE_NODE_OPT(TOK_BPOS, obj->bpos) && CHECK_TREE_NODE_OPT(TOK_SMT_ANN, obj->smt_ann);
}

void tree_node_finder::operator()(const function_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   // std::vector<std::string>::const_iterator vend = obj->list_of_op_names.end();
   // for (std::vector<std::string>::const_iterator i = obj->list_of_op_names.begin(); i != vend; i++)
   //   WRITE_UFIELD_STRING(os, *i);
   find_res = find_res && CHECK_VALUE_OPT(TOK_OPERATOR, obj->operator_flag) && CHECK_TREE_NODE_OPT(TOK_TMPL_PARMS, obj->tmpl_parms) && CHECK_TREE_NODE_OPT(TOK_TMPL_ARGS, obj->tmpl_args) &&

              CHECK_VALUE_OPT(TOK_FIXD, obj->fixd) && CHECK_VALUE_OPT(TOK_VIRT, obj->virt) && CHECK_TREE_NODE_OPT(TOK_FN, obj->fn) && CHECK_VALUE_OPT(TOK_UNDEFINED, obj->undefined_flag) && CHECK_VALUE_OPT(TOK_BUILTIN, obj->builtin_flag) &&
              CHECK_VALUE_OPT(TOK_STATIC, obj->static_flag) && CHECK_VALUE_OPT(TOK_HWCALL, obj->hwcall_flag) && CHECK_VALUE_OPT(TOK_REVERSE_RESTRICT, obj->reverse_restrict_flag) && CHECK_TREE_NODE_OPT(TOK_BODY, obj->body) &&
              CHECK_TREE_NODE_OPT(TOK_INLINE_BODY, obj->inline_body);
   /// FIXME: check args
}

void tree_node_finder::operator()(const function_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_RETN, obj->retn) && CHECK_TREE_NODE_OPT(TOK_PRMS, obj->prms) && CHECK_VALUE_OPT(TOK_VARARGS, obj->varargs_flag);
}

void tree_node_finder::operator()(const gimple_assign* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP, obj->op1) && CHECK_TREE_NODE_OPT(TOK_PREDICATE, obj->predicate) && CHECK_TREE_NODE_OPT(TOK_ORIG, obj->orig) && CHECK_VALUE_OPT(TOK_INIT, obj->init_assignment) &&
              CHECK_VALUE_OPT(TOK_CLOBBER, obj->clobber) && CHECK_VALUE_OPT(TOK_ADDR, obj->temporary_address);
}

void tree_node_finder::operator()(const gimple_goto* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op);
}

void tree_node_finder::operator()(const handler* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_LINE, obj->line) && CHECK_TREE_NODE_OPT(TOK_BODY, obj->body);
}

void tree_node_finder::operator()(const identifier_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void tree_node_finder::operator()(const integer_cst* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res and CHECK_VALUE_OPT(TOK_VALUE, obj->value);
}

void tree_node_finder::operator()(const integer_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_PREC, obj->prec) &&
              // if (obj->str != "")
              //   WRITE_UFIELD(os, obj->str);
              CHECK_VALUE_OPT(TOK_UNSIGNED, obj->unsigned_flag) && CHECK_TREE_NODE_OPT(TOK_MIN, obj->min) && CHECK_TREE_NODE_OPT(TOK_MAX, obj->max);
}

void tree_node_finder::operator()(const gimple_label* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op);
}

void tree_node_finder::operator()(const method_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_CLAS, obj->clas);
}

void tree_node_finder::operator()(const namespace_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_DCLS, obj->dcls);
}

void tree_node_finder::operator()(const overload* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_CRNT, obj->crnt) && CHECK_TREE_NODE_OPT(TOK_CHAN, obj->chan);
}

void tree_node_finder::operator()(const parm_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_ARGT, obj->argt) && CHECK_TREE_NODE_OPT(TOK_SIZE, obj->size) && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) && CHECK_VALUE_OPT(TOK_USED, obj->used) && CHECK_VALUE_OPT(TOK_REGISTER, obj->register_flag) &&
              CHECK_VALUE_OPT(TOK_READONLY, obj->readonly_flag) && CHECK_TREE_NODE_OPT(TOK_SMT_ANN, obj->smt_ann);
}

void tree_node_finder::operator()(const gimple_phi* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_RES, obj->res);
   TREE_NOT_YET_IMPLEMENTED(TOK_DEF);
   TREE_NOT_YET_IMPLEMENTED(TOK_EDGE);
   // std::vector<std::pair< tree_nodeRef, int> >::const_iterator vend = obj->list_of_def_edge.end();
   // for (std::vector<std::pair< tree_nodeRef, int> >::const_iterator i = obj->list_of_def_edge.begin(); i != vend; i++)
   //{
   //   write_when_not_null(STOK(TOK_DEF), i->first);
   //   WRITE_NFIELD(os, STOK(TOK_EDGE), i->second);
   //}
}

void tree_node_finder::operator()(const pointer_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_PTD, obj->ptd);
}

void tree_node_finder::operator()(const real_cst* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_OVERFLOW, obj->overflow_flag) && CHECK_VALUE_OPT(TOK_VALR, obj->valr) && CHECK_VALUE_OPT(TOK_VALX, obj->valx);
}

void tree_node_finder::operator()(const real_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_PREC, obj->prec);
}

void tree_node_finder::operator()(const record_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TMPL_PARMS, obj->tmpl_parms) && CHECK_TREE_NODE_OPT(TOK_TMPL_ARGS, obj->tmpl_args) && CHECK_VALUE_OPT(TOK_PTRMEM, obj->ptrmem_flag) && CHECK_TREE_NODE_OPT(TOK_PTD, obj->ptd) &&
              CHECK_TREE_NODE_OPT(TOK_CLS, obj->cls) && CHECK_TREE_NODE_OPT(TOK_BFLD, obj->bfld) && CHECK_TREE_NODE_OPT(TOK_VFLD, obj->vfld) && CHECK_VALUE_OPT(TOK_SPEC, obj->spec_flag) && CHECK_VALUE_OPT(TOK_STRUCT, obj->struct_flag) &&
              CHECK_TREE_NODE_OPT(TOK_BINF, obj->binf);
   TREE_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<tree_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_FNCS);
   // std::vector<tree_nodeRef>::const_iterator vend2 = obj->list_of_fncs.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_fncs.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_FNCS), *i);
}

void tree_node_finder::operator()(const reference_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_REFD, obj->refd);
}

void tree_node_finder::operator()(const result_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_INIT, obj->init) && CHECK_TREE_NODE_OPT(TOK_SIZE, obj->size) && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) && CHECK_TREE_NODE_OPT(TOK_SMT_ANN, obj->smt_ann);
}

void tree_node_finder::operator()(const gimple_return* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op);
}

void tree_node_finder::operator()(const return_stmt* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_LINE, obj->line) && CHECK_TREE_NODE_OPT(TOK_EXPR, obj->expr);
}

void tree_node_finder::operator()(const scope_ref* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP, obj->op1);
}

void tree_node_finder::operator()(const ssa_name* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_VAR, obj->type) && CHECK_TREE_NODE_OPT(TOK_VAR, obj->var) && CHECK_VALUE_OPT(TOK_VERS, obj->vers) && CHECK_VALUE_OPT(TOK_ORIG_VERS, obj->orig_vers) &&
              // CHECK_TREE_NODE_OPT(TOK_PTR_INFO, obj->ptr_info) &&
              CHECK_VALUE_OPT(TOK_VIRTUAL, obj->virtual_flag) && CHECK_VALUE_OPT(TOK_VOLATILE, obj->volatile_flag) && CHECK_TREE_NODE_OPT(TOK_MIN, obj->min) && CHECK_TREE_NODE_OPT(TOK_MAX, obj->max) && CHECK_VALUE_OPT(TOK_BIT_VALUES, obj->bit_values);
   TREE_NOT_YET_IMPLEMENTED(TOK_DEF_STMT);
}

void tree_node_finder::operator()(const statement_list* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   TREE_NOT_YET_IMPLEMENTED(TOK_STMT);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_stmt.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_BLOC);
   // std::map<int, blocRef>::const_iterator mend = obj->list_of_bloc.end();
   // for (std::map<int, blocRef>::const_iterator i = obj->list_of_bloc.begin(); i != mend; i++)
   //   write_when_not_null_bloc(STOK(TOK_BLOC), i->second);
}

void tree_node_finder::operator()(const string_cst* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_VALUE_OPT(TOK_STRG, obj->strg);
}

void tree_node_finder::operator()(const gimple_switch* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1);
}

void tree_node_finder::operator()(const target_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_DECL, obj->decl) && CHECK_TREE_NODE_OPT(TOK_INIT, obj->init) && CHECK_TREE_NODE_OPT(TOK_CLNP, obj->clnp);
}

void tree_node_finder::operator()(const lut_expr* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0) && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1) && CHECK_TREE_NODE_OPT(TOK_OP2, obj->op2) && CHECK_TREE_NODE_OPT(TOK_OP3, obj->op3) && CHECK_TREE_NODE_OPT(TOK_OP4, obj->op4) &&
              CHECK_TREE_NODE_OPT(TOK_OP5, obj->op5) && CHECK_TREE_NODE_OPT(TOK_OP6, obj->op6) && CHECK_TREE_NODE_OPT(TOK_OP7, obj->op7) && CHECK_TREE_NODE_OPT(TOK_OP8, obj->op8);
}

void tree_node_finder::operator()(const template_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_RSLT, obj->rslt) && CHECK_TREE_NODE_OPT(TOK_INST, obj->inst) && CHECK_TREE_NODE_OPT(TOK_SPCS, obj->spcs) && CHECK_TREE_NODE_OPT(TOK_PRMS, obj->prms);
}

void tree_node_finder::operator()(const template_parm_index* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type) && CHECK_TREE_NODE_OPT(TOK_DECL, obj->decl) && CHECK_VALUE_OPT(TOK_CONSTANT, obj->constant_flag) && CHECK_VALUE_OPT(TOK_READONLY, obj->readonly_flag) &&
              CHECK_VALUE_OPT(TOK_IDX, obj->idx) && CHECK_VALUE_OPT(TOK_LEVEL, obj->level) && CHECK_VALUE_OPT(TOK_ORIG_LEVEL, obj->orig_level);
}

void tree_node_finder::operator()(const tree_list* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_PURP, obj->purp) && CHECK_TREE_NODE_OPT(TOK_VALU, obj->valu) && CHECK_TREE_NODE_OPT(TOK_CHAN, obj->chan);
}

void tree_node_finder::operator()(const tree_vec* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_LNGT, obj->lngt);
   TREE_NOT_YET_IMPLEMENTED(TOK_OP);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_op.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_op.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_OP), *i);
}

void tree_node_finder::operator()(const try_block* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_LINE, obj->line) && CHECK_TREE_NODE_OPT(TOK_BODY, obj->body) && CHECK_TREE_NODE_OPT(TOK_HDLR, obj->hdlr) && CHECK_TREE_NODE_OPT(TOK_NEXT, obj->next);
}

void tree_node_finder::operator()(const type_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TMPL_PARMS, obj->tmpl_parms) && CHECK_TREE_NODE_OPT(TOK_TMPL_ARGS, obj->tmpl_args);
}

void tree_node_finder::operator()(const union_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_BINF, obj->binf);
   TREE_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<tree_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_FNCS);
   // std::vector<tree_nodeRef>::const_iterator vend2 = obj->list_of_fncs.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_fncs.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_FNCS), *i);
}

void tree_node_finder::operator()(const var_decl* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_VALUE_OPT(TOK_USE_TMPL, obj->use_tmpl) && CHECK_VALUE_OPT(TOK_STATIC_STATIC, obj->static_static_flag) && CHECK_VALUE_OPT(TOK_EXTERN, obj->extern_flag) && CHECK_VALUE_OPT(TOK_ADDR_TAKEN, obj->addr_taken) &&
              CHECK_VALUE_OPT(TOK_ADDR_NOT_TAKEN, obj->addr_not_taken) && CHECK_VALUE_OPT(TOK_STATIC, obj->static_flag) && CHECK_TREE_NODE_OPT(TOK_INIT, obj->init) && CHECK_TREE_NODE_OPT(TOK_SIZE, obj->size) && CHECK_VALUE_OPT(TOK_ALGN, obj->algn) &&
              CHECK_VALUE_OPT(TOK_USED, obj->used) && CHECK_VALUE_OPT(TOK_REGISTER, obj->register_flag) && CHECK_VALUE_OPT(TOK_READONLY, obj->readonly_flag) && CHECK_VALUE_OPT(TOK_BIT_VALUES, obj->bit_values) &&
              CHECK_TREE_NODE_OPT(TOK_SMT_ANN, obj->smt_ann);
   TREE_NOT_YET_IMPLEMENTED(TOK_ATTRIBUTES);
   TREE_NOT_YET_IMPLEMENTED(TOK_ADDR_STMT);
   TREE_NOT_YET_IMPLEMENTED(TOK_DEF_STMT);
   TREE_NOT_YET_IMPLEMENTED(TOK_USE_STMT);
}

void tree_node_finder::operator()(const vector_cst* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_valu.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_valu.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VALU), *i);
}

void tree_node_finder::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_ARG, obj->arg);
}

void tree_node_finder::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_ARG, obj->arg);
}

void tree_node_finder::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op) && CHECK_TREE_NODE_OPT(TOK_PARAM_PACKS, obj->param_packs) && CHECK_TREE_NODE_OPT(TOK_ARG, obj->arg);
}

void tree_node_finder::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP, obj->op) && CHECK_TREE_NODE_OPT(TOK_PARAM_PACKS, obj->param_packs) && CHECK_TREE_NODE_OPT(TOK_ARG, obj->arg);
}

void tree_node_finder::operator()(const vector_type* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_ELTS, obj->elts);
}

void tree_node_finder::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type) && CHECK_TREE_NODE_OPT(TOK_SYMBOL, obj->symbol) && CHECK_TREE_NODE_OPT(TOK_BASE, obj->base) && CHECK_TREE_NODE_OPT(TOK_IDX, obj->idx) && CHECK_TREE_NODE_OPT(TOK_STEP, obj->step) &&
              CHECK_TREE_NODE_OPT(TOK_OFFSET, obj->offset) && CHECK_TREE_NODE_OPT(TOK_ORIG, obj->orig) && CHECK_TREE_NODE_OPT(TOK_TAG, obj->tag);
}

void tree_node_finder::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_TYPE, obj->type) && CHECK_TREE_NODE_OPT(TOK_BASE, obj->base) && CHECK_TREE_NODE_OPT(TOK_IDX, obj->idx) && CHECK_TREE_NODE_OPT(TOK_STEP, obj->step) && CHECK_TREE_NODE_OPT(TOK_IDX2, obj->idx2) &&
              CHECK_TREE_NODE_OPT(TOK_OFFSET, obj->offset);
}

void tree_node_finder::operator()(const bloc* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   // WRITE_UFIELD(os, obj->number);
   find_res = find_res && CHECK_VALUE_OPT(TOK_HPL, obj->hpl) && CHECK_VALUE_OPT(TOK_LOOP_ID, obj->loop_id) && CHECK_VALUE_OPT(TOK_TRUE_EDGE, obj->true_edge) && CHECK_VALUE_OPT(TOK_FALSE_EDGE, obj->false_edge);

   TREE_NOT_YET_IMPLEMENTED(TOK_PRED);
   // std::vector<int>::const_iterator vend1 = obj->list_of_pred.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_pred.begin(); i != vend1; i++)
   //   if (*i == bloc::ENTRY_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_PRED), STOK(TOK_ENTRY));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_PRED), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_SUCC);
   // std::vector<int>::const_iterator vend2 = obj->list_of_succ.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_succ.begin(); i != vend2; i++)
   //   if (*i == bloc::EXIT_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_SUCC), STOK(TOK_EXIT));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_SUCC), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_PHI);
   // std::vector<tree_nodeRef>::const_iterator vend3 = obj->list_of_phi.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_phi.begin(); i != vend3; i++)
   //   write_when_not_null(STOK(TOK_PHI), *i);
   // std::vector<tree_nodeRef>::const_iterator vend4 = obj->list_of_stmt.end();
   TREE_NOT_YET_IMPLEMENTED(TOK_STMT);
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend4; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);
}

void tree_node_finder::operator()(const gimple_while* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP0, obj->op0);
}

void tree_node_finder::operator()(const gimple_for* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   find_res = find_res && CHECK_TREE_NODE_OPT(TOK_OP1, obj->op1) && CHECK_TREE_NODE_OPT(TOK_OP2, obj->op2);
}

void tree_node_finder::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_OP);
}

void tree_node_finder::operator()(const null_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}
void tree_node_finder::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const map_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const issue_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}

void tree_node_finder::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
}
