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
 * @file type_casting.cpp
 * @brief tree node visitor collecting the types used in type casting
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "type_casting.hpp"

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

void type_casting::operator()(const mem_ref* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
   else
   {
      types.insert(GET_INDEX_NODE(obj->type));
      tree_node_visitor::operator()(obj, mask);
   }
}

void type_casting::operator()(const indirect_ref* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
   else
   {
      types.insert(GET_INDEX_NODE(obj->type));
      tree_node_visitor::operator()(obj, mask);
   }
}

void type_casting::operator()(const tree_node* obj, unsigned int&)
{
   visited.insert(obj->index);
}

void type_casting::operator()(const WeightedNode*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const tree_reindex*, unsigned int&)
{
}

void type_casting::operator()(const attr*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const srcp*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const decl_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const expr_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const unary_expr* obj, unsigned int& mask)
{
   if(obj->get_kind() == view_convert_expr_K)
   {
      types.insert(obj->type->index);
      types.insert(tree_helper::CGetType(GET_NODE(obj->op))->index);
   }
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const binary_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const ternary_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const type_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const memory_tag* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const cst_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const error_mark* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const array_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_asm* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const baselink* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_bind* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const binfo* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const block* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const call_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const lut_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_call* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const case_label_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const cast_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const complex_cst* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const complex_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_cond* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const const_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const constructor* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const enumeral_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const expr_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const field_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const function_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const function_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_assign* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_goto* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const handler* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const identifier_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const integer_cst* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const integer_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_label* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const method_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const namespace_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const overload* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const parm_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_phi* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const pointer_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const real_cst* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const real_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const record_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const reference_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const result_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_return* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const return_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const scope_ref* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const ssa_name* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const statement_list* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const string_cst* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_switch* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const target_expr* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const template_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const template_parm_index* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const tree_list* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const tree_vec* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const try_block* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const type_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const union_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const var_decl* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const vector_cst* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const vector_type* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const bloc*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const gimple_while* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_for* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const null_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const omp_for_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const omp_simd_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const omp_declare_simd_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const omp_target_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const omp_critical_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const omp_task_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const map_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const call_hw_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const call_point_hw_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const issue_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const blackbox_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const profiling_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void type_casting::operator()(const statistical_profiling*, unsigned int& mask)
{
   mask = NO_VISIT;
}
