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
 *              Copyright (C) 2024 Politecnico di Milano
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
 * @file tree_reindex_remove.cpp
 * @brief tree reindex remove class
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "tree_reindex_remove.hpp"

#include "application_manager.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_common.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include <string>
#include <utility>
#include <vector>

tree_reindex_remove::tree_reindex_remove(const tree_manager& _TM) : TM(_TM), source_tn(nullptr)
{
}

void tree_reindex_remove::operator()(const tree_nodeRef& tn)
{
   source_tn = tn;
   tn->visit(this);
}

void tree_reindex_remove::operator()(const tree_node* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + obj->get_kind_text());
}

void tree_reindex_remove::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + obj->get_kind_text());
}

void tree_reindex_remove::operator()(const attr* obj, unsigned int& mask)
{
   // THROW_ASSERT(obj == dynamic_cast<attr*>(source_tn.get()), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::fix_reference(tree_nodeRef& tn) const
{
   if(tn && tn->get_kind() == tree_reindex_K)
   {
      tn = TM.GetTreeNode(tn->index);
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
      TreeNodeSet fix_set;                                       \
      for(auto& field : GetPointerS<type>(source_tn)->set_field) \
      {                                                          \
         tree_nodeRef tn = field;                                \
         fix_reference(tn);                                      \
         fix_set.insert(tn);                                     \
      }                                                          \
      GetPointerS<type>(source_tn)->set_field = fix_set;         \
   }

void tree_reindex_remove::operator()(const srcp* obj, unsigned int& mask)
{
   // THROW_ASSERT(obj == dynamic_cast<srcp*>(source_tn.get()), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const WeightedNode* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(name, decl_node);
   node_fix_reference(mngl, decl_node);
   node_fix_reference(orig, decl_node);
   node_fix_reference(type, decl_node);
   node_fix_reference(scpe, decl_node);
   node_fix_reference(attributes, decl_node);
   node_fix_reference(chan, decl_node);
}

void tree_reindex_remove::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, expr_node);
}

void tree_reindex_remove::operator()(const gimple_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(memuse, gimple_node);
   node_fix_reference(memdef, gimple_node);
   set_fix_reference(vuses, gimple_node);
   node_fix_reference(vdef, gimple_node);
   set_fix_reference(vovers, gimple_node);
   seq_fix_reference(pragmas, gimple_node);
   node_fix_reference(scpe, gimple_node);
   seq_fix_reference(use_set->variables, gimple_node);
   seq_fix_reference(clobbered_set->variables, gimple_node);
}

void tree_reindex_remove::operator()(const unary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, unary_expr);
}

void tree_reindex_remove::operator()(const binary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, binary_expr);
   node_fix_reference(op1, binary_expr);
}

void tree_reindex_remove::operator()(const ternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, ternary_expr);
   node_fix_reference(op1, ternary_expr);
   node_fix_reference(op2, ternary_expr);
}

void tree_reindex_remove::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, quaternary_expr);
   node_fix_reference(op1, quaternary_expr);
   node_fix_reference(op2, quaternary_expr);
   node_fix_reference(op3, quaternary_expr);
}

void tree_reindex_remove::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(name, type_node);
   node_fix_reference(unql, type_node);
   node_fix_reference(size, type_node);
   node_fix_reference(scpe, type_node);
}

void tree_reindex_remove::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, cst_node);
}

void tree_reindex_remove::operator()(const error_mark* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const array_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(elts, array_type);
   node_fix_reference(domn, array_type);
}

void tree_reindex_remove::operator()(const gimple_asm* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(out, gimple_asm);
   node_fix_reference(in, gimple_asm);
   node_fix_reference(clob, gimple_asm);
}

void tree_reindex_remove::operator()(const baselink* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, baselink);
}

void tree_reindex_remove::operator()(const gimple_bind* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   seq_fix_reference(list_of_vars, gimple_bind);
   node_fix_reference(body, gimple_bind);
}

void tree_reindex_remove::operator()(const binfo* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, binfo);
   if(!GetPointerS<binfo>(source_tn)->list_of_access_binf.empty())
   {
      for(auto& [tok, field] : GetPointerS<binfo>(source_tn)->list_of_access_binf)
      {
         fix_reference(field);
      }
   }
}

void tree_reindex_remove::operator()(const block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const call_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(fn, call_expr);
   seq_fix_reference(args, call_expr);
}

void tree_reindex_remove::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(slot, aggr_init_expr);
}

void tree_reindex_remove::operator()(const gimple_call* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(fn, gimple_call);
   seq_fix_reference(args, gimple_call);
   seq_fix_reference(use_set->variables, gimple_call);
   seq_fix_reference(clobbered_set->variables, gimple_call);
}

void tree_reindex_remove::operator()(const case_label_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, case_label_expr);
   node_fix_reference(op1, case_label_expr);
   node_fix_reference(got, case_label_expr);
}

void tree_reindex_remove::operator()(const cast_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, cast_expr);
}

void tree_reindex_remove::operator()(const complex_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(real, complex_cst);
   node_fix_reference(imag, complex_cst);
}

void tree_reindex_remove::operator()(const complex_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const gimple_cond* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, gimple_cond);
}

void tree_reindex_remove::operator()(const const_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(cnst, const_decl);
}

void tree_reindex_remove::operator()(const constructor* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, constructor);
   if(!GetPointerS<constructor>(source_tn)->list_of_idx_valu.empty())
   {
      for(auto& [idx, valu] : GetPointerS<constructor>(source_tn)->list_of_idx_valu)
      {
         fix_reference(idx);
         fix_reference(valu);
      }
   }
}

void tree_reindex_remove::operator()(const enumeral_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(min, enumeral_type);
   node_fix_reference(max, enumeral_type);
   node_fix_reference(csts, enumeral_type);
}

void tree_reindex_remove::operator()(const expr_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(expr, expr_stmt);
   node_fix_reference(next, expr_stmt);
}

void tree_reindex_remove::operator()(const field_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(init, field_decl);
   node_fix_reference(size, field_decl);
   node_fix_reference(bpos, field_decl);
   node_fix_reference(smt_ann, field_decl);
}

void tree_reindex_remove::operator()(const function_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(tmpl_parms, function_decl);
   node_fix_reference(tmpl_args, function_decl);

   node_fix_reference(fn, function_decl);
   seq_fix_reference(list_of_args, function_decl);
   node_fix_reference(body, function_decl);
   node_fix_reference(inline_body, function_decl);
}

void tree_reindex_remove::operator()(const function_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(retn, function_type);
   node_fix_reference(prms, function_type);
}

void tree_reindex_remove::operator()(const gimple_assign* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, gimple_assign);
   node_fix_reference(op1, gimple_assign);
   node_fix_reference(predicate, gimple_assign);
   seq_fix_reference(use_set->variables, gimple_assign);
   seq_fix_reference(clobbered_set->variables, gimple_assign);
}

void tree_reindex_remove::operator()(const gimple_goto* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, gimple_goto);
}

void tree_reindex_remove::operator()(const handler* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(body, handler);
}

void tree_reindex_remove::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const integer_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const integer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(min, integer_type);
   node_fix_reference(max, integer_type);
}

void tree_reindex_remove::operator()(const gimple_label* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, gimple_label);
}

void tree_reindex_remove::operator()(const method_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(clas, method_type);
}

void tree_reindex_remove::operator()(const namespace_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(dcls, namespace_decl);
}

void tree_reindex_remove::operator()(const overload* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(crnt, overload);
   node_fix_reference(chan, overload);
}

void tree_reindex_remove::operator()(const parm_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(argt, parm_decl);
   node_fix_reference(size, parm_decl);
   node_fix_reference(scpe, parm_decl);
   node_fix_reference(smt_ann, parm_decl);
}

void tree_reindex_remove::operator()(const gimple_phi* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(res, gimple_phi);
   if(!GetPointerS<gimple_phi>(source_tn)->list_of_def_edge.empty())
   {
      gimple_phi::DefEdgeList fix_set;
      for(auto& [def, edge] : GetPointerS<gimple_phi>(source_tn)->list_of_def_edge)
      {
         fix_reference(def);
      }
   }
}

void tree_reindex_remove::operator()(const pointer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(ptd, pointer_type);
}

void tree_reindex_remove::operator()(const real_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const real_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const record_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(tmpl_parms, record_type);
   node_fix_reference(tmpl_args, record_type);

   node_fix_reference(ptd, record_type);
   node_fix_reference(cls, record_type);
   node_fix_reference(bfld, record_type);
   node_fix_reference(vfld, record_type);
   seq_fix_reference(list_of_flds, record_type);
   seq_fix_reference(list_of_fncs, record_type);
   node_fix_reference(binf, record_type);
}

void tree_reindex_remove::operator()(const reference_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(refd, reference_type);
}

void tree_reindex_remove::operator()(const result_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(init, result_decl);
   node_fix_reference(size, result_decl);
   node_fix_reference(smt_ann, result_decl);
}

void tree_reindex_remove::operator()(const gimple_return* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, gimple_return);
}

void tree_reindex_remove::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(expr, return_stmt);
}

void tree_reindex_remove::operator()(const scope_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, scope_ref);
   node_fix_reference(op1, scope_ref);
}

void tree_reindex_remove::operator()(const ssa_name* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(type, ssa_name);
   node_fix_reference(var, ssa_name);
   seq_fix_reference(use_set->variables, ssa_name);

   const auto defs = GetPointerS<ssa_name>(source_tn)->CGetDefStmts();
   for(auto it = defs.begin(); it != defs.end(); ++it)
   {
      tree_nodeRef fix = *it;
      fix_reference(fix);
      if(it == defs.begin())
      {
         GetPointerS<ssa_name>(source_tn)->SetDefStmt(fix);
      }
      else
      {
         GetPointerS<ssa_name>(source_tn)->AddDefStmt(fix);
      }
   }

   node_fix_reference(min, ssa_name);
   node_fix_reference(max, ssa_name);
}

void tree_reindex_remove::operator()(const statement_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   seq_fix_reference(list_of_stmt, statement_list);
   for(const auto& [bbi, bb] : GetPointerS<statement_list>(source_tn)->list_of_bloc)
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

void tree_reindex_remove::operator()(const string_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const gimple_switch* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, gimple_switch);
   node_fix_reference(op1, gimple_switch);
}

void tree_reindex_remove::operator()(const target_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(decl, target_expr);
   node_fix_reference(init, target_expr);
   node_fix_reference(clnp, target_expr);
}

void tree_reindex_remove::operator()(const lut_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, lut_expr);
   node_fix_reference(op1, lut_expr);
   node_fix_reference(op2, lut_expr);
   node_fix_reference(op3, lut_expr);
   node_fix_reference(op4, lut_expr);
   node_fix_reference(op5, lut_expr);
   node_fix_reference(op6, lut_expr);
   node_fix_reference(op7, lut_expr);
   node_fix_reference(op8, lut_expr);
}
void tree_reindex_remove::operator()(const template_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(rslt, template_decl);
   node_fix_reference(inst, template_decl);
   node_fix_reference(spcs, template_decl);
   node_fix_reference(prms, template_decl);
}

void tree_reindex_remove::operator()(const template_parm_index* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(type, template_parm_index);
   node_fix_reference(decl, template_parm_index);
}

void tree_reindex_remove::operator()(const tree_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(purp, tree_list);
   node_fix_reference(valu, tree_list);
   node_fix_reference(chan, tree_list);
}

void tree_reindex_remove::operator()(const tree_vec* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   seq_fix_reference(list_of_op, tree_vec);
}

void tree_reindex_remove::operator()(const try_block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(body, try_block);
   node_fix_reference(hdlr, try_block);
   node_fix_reference(next, try_block);
}

void tree_reindex_remove::operator()(const type_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(tmpl_parms, type_decl);
   node_fix_reference(tmpl_args, type_decl);
}

void tree_reindex_remove::operator()(const union_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   seq_fix_reference(list_of_flds, union_type);
   seq_fix_reference(list_of_fncs, union_type);
   node_fix_reference(binf, union_type);
}

void tree_reindex_remove::operator()(const var_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(init, var_decl);
   node_fix_reference(size, var_decl);
   node_fix_reference(smt_ann, var_decl);
   set_fix_reference(defs, var_decl);
   set_fix_reference(uses, var_decl);
   set_fix_reference(addressings, var_decl);
}

void tree_reindex_remove::operator()(const vector_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   seq_fix_reference(list_of_valu, vector_cst);
}

void tree_reindex_remove::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(arg, type_argument_pack);
}

void tree_reindex_remove::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(arg, nontype_argument_pack);
}

void tree_reindex_remove::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, type_pack_expansion);
   node_fix_reference(param_packs, type_pack_expansion);
   node_fix_reference(arg, type_pack_expansion);
}

void tree_reindex_remove::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op, expr_pack_expansion);
   node_fix_reference(param_packs, expr_pack_expansion);
   node_fix_reference(arg, expr_pack_expansion);
}

void tree_reindex_remove::operator()(const vector_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(elts, vector_type);
}

void tree_reindex_remove::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(type, target_mem_ref);
   node_fix_reference(symbol, target_mem_ref);
   node_fix_reference(base, target_mem_ref);
   node_fix_reference(idx, target_mem_ref);
   node_fix_reference(step, target_mem_ref);
   node_fix_reference(offset, target_mem_ref);
   node_fix_reference(orig, target_mem_ref);
   node_fix_reference(tag, target_mem_ref);
}

void tree_reindex_remove::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   node_fix_reference(type, target_mem_ref461);
   node_fix_reference(base, target_mem_ref461);
   node_fix_reference(idx, target_mem_ref461);
   node_fix_reference(step, target_mem_ref461);
   node_fix_reference(idx2, target_mem_ref461);
   node_fix_reference(offset, target_mem_ref461);
}

void tree_reindex_remove::operator()(const bloc*, unsigned int&)
{
   THROW_ERROR("bloc node not supported");
}

void tree_reindex_remove::operator()(const gimple_while* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, gimple_while);
}

void tree_reindex_remove::operator()(const gimple_for* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op1, gimple_for);
   node_fix_reference(op2, gimple_for);
}

void tree_reindex_remove::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ASSERT(source_tn, "");
   for(auto& [cond, edge] : GetPointerS<gimple_multi_way_if>(source_tn)->list_of_cond)
   {
      fix_reference(cond);
   }
}

void tree_reindex_remove::operator()(const null_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(scope, gimple_pragma);
   node_fix_reference(directive, gimple_pragma);
}

void tree_reindex_remove::operator()(const omp_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   node_fix_reference(op0, omp_parallel_sections_pragma);
   node_fix_reference(op1, omp_parallel_sections_pragma);
}

void tree_reindex_remove::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const map_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const issue_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_reindex_remove::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == source_tn.get(), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}
