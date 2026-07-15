/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file type_casting.cpp
 * @brief IR node visitor collecting the types used in type casting
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "type_casting.hpp"

/// IR include
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"

void type_casting::operator()(const mem_access_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
   else
   {
      types.insert(obj->type);
      ir_node_visitor::operator()(obj, mask);
   }
}

void type_casting::operator()(const ir_node* obj, unsigned int&)
{
   visited.insert(obj->index);
}

void type_casting::operator()(const ir_reindex*, unsigned int&)
{
}

void type_casting::operator()(const IR_LocInfo* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
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

void type_casting::operator()(const node_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const unary_node* obj, unsigned int& mask)
{
   if(obj->get_kind() == bitcast_node_K)
   {
      types.insert(obj->type);
      types.insert(ir_helper::CGetType(obj->op));
   }
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const binary_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const ternary_node* obj, unsigned int& mask)
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

void type_casting::operator()(const cst_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const array_ty_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const call_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const lut_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const call_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const constructor_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const field_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const function_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const function_ty_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const assign_stmt* obj, unsigned int& mask)
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

void type_casting::operator()(const constant_int_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const integer_ty_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const argument_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const phi_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const pointer_ty_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const constant_fp_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const real_ty_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const struct_ty_node* obj, unsigned int& mask)
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

void type_casting::operator()(const ssa_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const statement_list_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const variable_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const constant_vector_val_node* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}

void type_casting::operator()(const vector_ty_node* obj, unsigned int& mask)
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

void type_casting::operator()(const multi_way_if_stmt* obj, unsigned int& mask)
{
   if(visited.find(obj->index) != visited.end())
   {
      mask = NO_VISIT;
   }
}
