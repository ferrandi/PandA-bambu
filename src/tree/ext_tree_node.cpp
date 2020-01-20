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
 * @file ext_tree_node.cpp
 * @brief Class implementation of the tree_node structures not present in gcc intermediate representation
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $
 *
 */

#include "ext_tree_node.hpp"

void null_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void gimple_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, scope, visit(v));
   VISIT_MEMBER(mask, directive, visit(v));
}

void call_hw_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, map_pragma, visit(v));
}

void call_point_hw_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, map_pragma, visit(v));
}

void profiling_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void statistical_profiling::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, profiling_pragma, visit(v));
}

void omp_parallel_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_sections_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_parallel_sections_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

void omp_section_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

omp_target_pragma::omp_target_pragma(unsigned int i) : omp_pragma(i)
{
}

void omp_target_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

omp_critical_pragma::omp_critical_pragma(unsigned int i) : omp_pragma(i)
{
}

void omp_critical_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

omp_task_pragma::omp_task_pragma(unsigned int i) : omp_pragma(i)
{
}

void omp_task_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_atomic_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_for_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_simd_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void omp_declare_simd_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, omp_pragma, visit(v));
}

void map_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void issue_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void blackbox_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, issue_pragma, visit(v));
}

void omp_pragma::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void gimple_while::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
}

void gimple_for::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_while, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, op2, visit(v));
}

void gimple_multi_way_if::add_cond(const tree_nodeRef& cond, unsigned int bb_ind)
{
   list_of_cond.emplace_back(cond, bb_ind);
}

void gimple_multi_way_if::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   for(auto cond : list_of_cond)
   {
      VISIT_MEMBER_NAMED(list_of_cond, mask, cond.first, visit(v));
   }
}
