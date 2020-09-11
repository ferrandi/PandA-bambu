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
 * @file tree_manager.cpp
 * @brief Class implementation of the manager of the tree structures extracted from the raw file.
 *
 * This file implements some of the tree_manager member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 *
 */

/// Autoheader include
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp"
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_NPROFILE.hpp"

/// Header include
#include "exceptions.hpp"          // for THROW_ASSERT, THROW...
#include "string_manipulation.hpp" // for STR GET_CLASS
#include "tree_manager.hpp"
#include <cstring>  // for strlen, size_t
#include <fstream>  // for operator<<, basic_o...
#include <iostream> // for operator<<, basic_o...
#include <list>     // for list
#include <vector>   // for vector, allocator

/// Machine include
#if HAVE_MAPPING_BUILT
#include "machine_node.hpp"
#endif
/// Parameter include
#include "Parameter.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// Tree include
#include "ext_tree_node.hpp"
#include "gimple_writer.hpp"
#include "raw_writer.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_node_factory.hpp"
#include "tree_node_finder.hpp"
#include "tree_nodes_merger.hpp"
#include "tree_reindex.hpp"
#if HAVE_CODE_ESTIMATION_BUILT
#include "weight_information.hpp"
#endif

/// Wrapper include
#include "gcc_wrapper.hpp"

#include "dbgPrintHelper.hpp"
#include "utility.hpp"

tree_manager::tree_manager(const ParameterConstRef& _Param)
    : n_pl(0),
      added_goto(0),
      removed_pointer_plus(0),
      removable_pointer_plus(0),
      unremoved_pointer_plus(0),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE)),
      last_node_id(1),
      Param(_Param),
      next_vers(0),
      collapse_into_counter(0)
{
}

tree_manager::~tree_manager() = default;

unsigned int tree_manager::get_implementation_node(unsigned int decl_node) const
{
   THROW_ASSERT(GetPointer<function_decl>(get_tree_node_const(decl_node)), "Node " + STR(decl_node) + " is not a function decl: " + get_tree_node_const(decl_node)->get_kind_text());
   if(GetPointer<function_decl>(get_tree_node_const(decl_node))->body)
   {
      return decl_node;
   }
   else
   {
      return 0;
   }
}

void tree_manager::AddTreeNode(unsigned int i, const tree_nodeRef& curr)
{
   THROW_ASSERT(i > 0, "Expected a positive index");
   THROW_ASSERT(curr, "Invalid tree node: " + STR(i));
   if(i >= last_node_id)
   {
      last_node_id = i + 1;
   }
   tree_nodes[i] = curr;
}

tree_nodeRef tree_manager::GetTreeReindex(unsigned int index)
{
   THROW_ASSERT(index > 0, "Expected a positive index (" + STR(index) + ")");
   if(index >= last_node_id)
   {
      last_node_id = index + 1;
   }
   return tree_nodeRef(new tree_reindex(index, tree_nodes[index]));
}

const tree_nodeRef tree_manager::CGetTreeReindex(const unsigned int i) const
{
   THROW_ASSERT(i > 0 and i < last_node_id, "(C) Expected a positive index less than the total number of tree nodes (" + STR(i) + ") (" + STR(last_node_id) + ")");
   THROW_ASSERT(tree_nodes.find(i) != tree_nodes.end(), "Tree node " + STR(i) + " does not exist");
   return tree_nodeRef(new tree_reindex(i, tree_nodes.at(i)));
}

tree_nodeRef tree_manager::GetTreeNode(const unsigned int index) const
{
   THROW_ASSERT(tree_nodes.find(index) != tree_nodes.end(), "Tree node with index " + STR(index) + " not found");
   return tree_nodes.find(index)->second;
}

const tree_nodeRef tree_manager::get_tree_node_const(unsigned int i) const
{
   THROW_ASSERT(i > 0 and i < last_node_id, "(C) Expected a positive index less than the total number of tree nodes (" + STR(i) + ") (" + STR(last_node_id) + ")");
   THROW_ASSERT(tree_nodes.find(i) != tree_nodes.end(), "Tree node " + STR(i) + " does not exist");
   THROW_ASSERT(tree_nodes.find(i)->second, "Tree node " + STR(i) + " is empty");
   return tree_nodes.find(i)->second;
}

const tree_nodeConstRef tree_manager::CGetTreeNode(const unsigned int i) const
{
   THROW_ASSERT(i > 0 and i < last_node_id, "(C) Expected a positive index less than the total number of tree nodes (" + STR(i) + ") (" + STR(last_node_id) + ")");
   THROW_ASSERT(tree_nodes.find(i) != tree_nodes.end(), "Tree node " + STR(i) + " does not exist");
   return tree_nodes.find(i)->second;
}

bool tree_manager::is_tree_node(unsigned int i) const
{
   return tree_nodes.find(i) != tree_nodes.end();
}

// *****************************************************************************************

unsigned int tree_manager::find_sc_main_node() const
{
   return function_index("sc_main");
}

unsigned int tree_manager::function_index(const std::string& function_name) const
{
   null_deleter null_del;
   tree_managerConstRef TM(this, null_del);
   unsigned int function_id = 0;
   for(const auto& function_decl_node : function_decl_nodes)
   {
      tree_nodeRef curr_tn = function_decl_node.second;
      auto* fd = GetPointer<function_decl>(curr_tn);
      tree_nodeRef id_name = GET_NODE(fd->name);
      std::string simple_name;
      if(id_name->get_kind() == identifier_node_K)
      {
         auto* in = GetPointer<identifier_node>(id_name);
         if(!in->operator_flag)
         {
            simple_name = in->strg;
         }
      }
      std::string name = tree_helper::print_function_name(TM, fd);
      if(name == function_name || function_name == std::string("-") || (!simple_name.empty() && function_name == simple_name))
      {
         function_id = function_decl_node.first;
      }
   }
   return function_id;
}

unsigned int tree_manager::function_index_mngl(const std::string& function_name) const
{
   null_deleter null_del;
   tree_managerConstRef TM(this, null_del);
   unsigned int function_id = 0;
   for(const auto& function_decl_node : function_decl_nodes)
   {
      tree_nodeRef curr_tn = function_decl_node.second;
      auto* fd = GetPointer<function_decl>(curr_tn);
      tree_nodeRef id_name = GET_NODE(fd->name);
      std::string simple_name, mangled_name;
      if(id_name->get_kind() == identifier_node_K)
      {
         auto* in = GetPointer<identifier_node>(id_name);
         if(!in->operator_flag)
         {
            simple_name = in->strg;
         }
      }
      if(fd->mngl)
      {
         tree_nodeRef mangled_id_name = GET_NODE(fd->mngl);
         if(mangled_id_name->get_kind() == identifier_node_K)
         {
            auto* in = GetPointer<identifier_node>(mangled_id_name);
            if(!in->operator_flag)
               mangled_name = in->strg;
         }
      }
      std::string name = tree_helper::print_function_name(TM, fd);
      if(name == function_name || function_name == std::string("-") || (!simple_name.empty() && function_name == simple_name) || (!mangled_name.empty() && mangled_name == function_name))
      {
         function_id = function_decl_node.first;
      }
   }
   return function_id;
}

void tree_manager::print(std::ostream& os) const
{
#if HAVE_MAPPING_BUILT
   std::string component_type_string = Param->getOption<std::string>(OPT_driving_component_type);
#endif
   raw_writer RW(
#if HAVE_MAPPING_BUILT
       processingElement::get_component_type(component_type_string),
#endif
       os);

   os << STOK(TOK_GCC_VERSION) << ": \"" << GccWrapper::current_gcc_version << "\"\n";
   os << STOK(TOK_PLUGIN_VERSION) << ": \"" << GccWrapper::current_plugin_version << "\"\n";

   unsigned int node_index = 0;
   for(node_index = 0; node_index <= last_node_id; node_index++)
   {
      if(tree_nodes.find(node_index) != tree_nodes.end())
      {
         os << "@" << tree_nodes.find(node_index)->first << " ";
         tree_nodes.find(node_index)->second->visit(&RW);
         os << std::endl;
      }
   }
}

void tree_manager::PrintGimple(std::ostream& os, const bool use_uid) const
{
   GimpleWriter gimple_writer(os, use_uid);

   std::map<unsigned int, tree_nodeRef>::const_iterator function, function_end = function_decl_nodes.end();
   for(function = function_decl_nodes.begin(); function != function_end; ++function)
   {
      if(GetPointer<function_decl>(function->second)->body)
      {
         function->second->visit(&gimple_writer);
      }
   }
}

void tree_manager::create_tree_node(const unsigned int node_id, enum kind tree_node_type, std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema)
{
#ifndef NDEBUG
   int function_debug_level = Param->GetFunctionDebugLevel(GET_CLASS(*this), __func__);
#endif
   tree_node_factory TNF(tree_node_schema, *this);
   TNF.create_tree_node(node_id, tree_node_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level, "---Created tree node " + STR(node_id) + ": " + STR(CGetTreeNode(node_id)));
}

unsigned int tree_manager::new_tree_node_id(const unsigned int ask)
{
   if(ask and tree_nodes.find(ask) == tree_nodes.end())
   {
      GetTreeReindex(ask);
      return ask;
   }
   unsigned int temp = last_node_id;
   GetTreeReindex(last_node_id);
   return temp;
}

unsigned int tree_manager::get_next_available_tree_node_id() const
{
   return last_node_id;
}

void tree_manager::add_function(unsigned int index, tree_nodeRef curr)
{
   function_decl_nodes[index] = curr;
}

unsigned long tree_manager::get_function_decl_node_n() const
{
   return function_decl_nodes.size();
}

unsigned int tree_manager::find(enum kind tree_node_type, const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema)
{
   if(tree_node_type == identifier_node_K)
   {
      std::string id;
      if(tree_node_schema.find(TOK(TOK_STRG)) != tree_node_schema.end())
      {
         id = tree_node_schema.find(TOK(TOK_STRG))->second;
      }
      else if(tree_node_schema.find(TOK(TOK_OPERATOR)) != tree_node_schema.end())
      {
         id = STOK(TOK_OPERATOR);
      }
      else
      {
         THROW_ERROR("Incorrect schema for identifier_node: no TOK_STRG nor TOK_OPERATOR");
      }
      return find_identifier_nodeID(id);
   }
   std::string key = tree_node::GetString(tree_node_type);
   const auto tns_end = tree_node_schema.end();
   for(auto tns = tree_node_schema.begin(); tns != tns_end; ++tns)
   {
      key += ";" + STOK2(tns->first) + "=" + tns->second;
   }
   // std::cout << "KEY: " + key + "[" << (find_cache.find(key) != find_cache.end() ? find_cache.find(key)->second : 0) << "]" << std::endl;
   if(find_cache.find(key) != find_cache.end())
   {
      return find_cache.find(key)->second;
   }
   tree_node_finder TNF(tree_node_schema);
   for(const auto& ti : tree_nodes)
   {
      unsigned int node_id = ti.first;
      /// check if the corresponding tree node has been already created or not
      if(!ti.second)
      {
         continue;
      }

      if(ti.second->get_kind() == tree_node_type and TNF.check(ti.second))
      {
         find_cache[key] = node_id;
         return node_id;
      }
   }
   return 0;
}

void tree_manager::collapse_into(const unsigned int& funID, CustomUnorderedMapUnstable<unsigned int, unsigned int>& stmt_to_bloc, const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& removed_nodes)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   const unsigned int tree_node_index = GET_INDEX_NODE(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Collapsing into " + STR(tree_node_index) + " (" + std::string(GET_NODE(tn)->get_kind_text()) + "): " + tn->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      const std::string gimple_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "collapse_into_" + STR(collapse_into_counter) + "_before_" + STR(GET_INDEX_NODE(tn)) + ".gimple";
      collapse_into_counter++;
      std::ofstream gimple_file(gimple_file_name.c_str());
      PrintGimple(gimple_file, false);
      gimple_file.close();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Writing file " + gimple_file_name);
   }
   tree_nodeRef curr_tn = GET_NODE(tn);
   switch(curr_tn->get_kind())
   {
      case gimple_assign_K:
      {
         stack.push_front(tn);
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         /// If there is a conversion to a not built-in type, type has to be declared; but if no variable survives, type will be not declared by the backend
         null_deleter null_del;
         if(GET_NODE(gm->op1)->get_kind() != nop_expr_K or not tree_helper::HasToBeDeclared(tree_managerRef(this, null_del), GET_INDEX_NODE(GetPointer<nop_expr>(GET_NODE(gm->op1))->type)))
         {
            collapse_into(funID, stmt_to_bloc, gm->op1, removed_nodes);
         }
         stack.pop_front();
         break;
      }
      case gimple_cond_K:
      {
         stack.push_front(tn);
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         collapse_into(funID, stmt_to_bloc, gc->op0, removed_nodes);
         stack.pop_front();
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(curr_tn);
         collapse_into(funID, stmt_to_bloc, ue->op, removed_nodes);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         collapse_into(funID, stmt_to_bloc, be->op0, removed_nodes);
         collapse_into(funID, stmt_to_bloc, be->op1, removed_nodes);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         collapse_into(funID, stmt_to_bloc, se->op0, removed_nodes);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         collapse_into(funID, stmt_to_bloc, te->op0, removed_nodes);
         collapse_into(funID, stmt_to_bloc, te->op1, removed_nodes);
         if(te->op2)
         {
            collapse_into(funID, stmt_to_bloc, te->op2, removed_nodes);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         collapse_into(funID, stmt_to_bloc, qe->op0, removed_nodes);
         collapse_into(funID, stmt_to_bloc, qe->op1, removed_nodes);
         if(qe->op2)
         {
            collapse_into(funID, stmt_to_bloc, qe->op2, removed_nodes);
         }
         if(qe->op3)
         {
            collapse_into(funID, stmt_to_bloc, qe->op3, removed_nodes);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         collapse_into(funID, stmt_to_bloc, le->op0, removed_nodes);
         collapse_into(funID, stmt_to_bloc, le->op1, removed_nodes);
         if(le->op2)
            collapse_into(funID, stmt_to_bloc, le->op2, removed_nodes);
         if(le->op3)
         {
            collapse_into(funID, stmt_to_bloc, le->op3, removed_nodes);
         }
         if(le->op4)
         {
            collapse_into(funID, stmt_to_bloc, le->op4, removed_nodes);
         }
         if(le->op5)
         {
            collapse_into(funID, stmt_to_bloc, le->op5, removed_nodes);
         }
         if(le->op6)
         {
            collapse_into(funID, stmt_to_bloc, le->op6, removed_nodes);
         }
         if(le->op7)
         {
            collapse_into(funID, stmt_to_bloc, le->op7, removed_nodes);
         }
         if(le->op8)
         {
            collapse_into(funID, stmt_to_bloc, le->op8, removed_nodes);
         }
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_phi_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case gimple_nop_K:
      case gimple_multi_way_if_K:
      case function_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(curr_tn);

         if(sn->CGetDefStmts().size() == 1)
         {
            auto* gm = GetPointer<gimple_assign>(GET_NODE(sn->CGetDefStmt()));
            // Don't continue if declaration of ssa variable is not a gimple_assign
            if((!gm and GET_NODE(sn->CGetDefStmt())->get_kind() == gimple_asm_K) || (!gm and GET_NODE(sn->CGetDefStmt())->get_kind() == nop_expr_K) || (!gm and GET_NODE(sn->CGetDefStmt())->get_kind() == gimple_nop_K))
            {
               break;
            }
            if(!gm)
            {
               THROW_ERROR("unexpected statement " + sn->CGetDefStmt()->ToString());
            }
            if(gm->memdef)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The gimple assignment where ssa name is defined has virtual def");
               break;
            }
            /// If there is a conversion to a not built-in type, type has to be declared; but if no variable survives, type will be not declared by the backend
            null_deleter null_del;
            if(GET_NODE(gm->op1)->get_kind() == nop_expr_K and tree_helper::HasToBeDeclared(tree_managerRef(this, null_del), GET_INDEX_NODE(GetPointer<nop_expr>(GET_NODE(gm->op1))->type)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The gimple assignment where ssa name is defined has a non-builtin nop_expr");
               break;
            }

            if((Param->getOption<bool>(OPT_compare_models) or Param->getOption<bool>(OPT_normalize_models) or Param->getOption<bool>(OPT_compare_measure_regions) or Param->isOption(OPT_hand_mapping)) and
               (GET_NODE(gm->op1)->get_kind() == call_expr_K || GET_NODE(gm->op1)->get_kind() == aggr_init_expr_K))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The gimple assignment has call_expr on the right and we are target profiling");
               break;
            }
            if(gm->predicate)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The gimple assignment is predicated");
               break;
            }
            collapse_into(funID, stmt_to_bloc, sn->CGetDefStmt(), removed_nodes);
            THROW_ASSERT(gm, "ssa name " + STR(GET_INDEX_NODE(tn)) + " not defined in a gimple modify stmt, nor in gimple_asm, nor in a nop_expr");

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition of " + STR(GET_INDEX_NODE(tn)) + ":");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(GET_INDEX_NODE(sn->CGetDefStmt())) + " (" + std::string(GET_NODE(sn->CGetDefStmt())->get_kind_text()) + ")");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Uses of " + STR(GET_INDEX_NODE(tn)) + " - " + STR(sn->CGetNumberUses()) + ":");
#ifndef NDEBUG
            for(const auto& use : sn->CGetUseStmts())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + use.first->ToString());
            }
#endif
            // Retrieve curr_block, the block which contains the conditional expression that originated the collapsing
            tree_nodeRef temp = get_tree_node_const(funID);
            auto* fd = GetPointer<function_decl>(temp);
            auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
            const std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;
#if 1
            THROW_ASSERT(!stack.empty(), "stack is empty");
            THROW_ASSERT(stmt_to_bloc.find((*(stack.rbegin()))->index) != stmt_to_bloc.end(), "BB of statement " + STR((*(stack.begin()))->index) + " not found");
            THROW_ASSERT(list_of_bloc.find(stmt_to_bloc.find((*(stack.begin()))->index)->second) != list_of_bloc.end(), "BB of statement not found");
            blocRef curr_block = list_of_bloc.find(stmt_to_bloc.find((*(stack.begin()))->index)->second)->second;
#else
            unsigned int curr_block_index;
            blocRef curr_block;
            // Find the conditional expression in the stack
            std::deque<tree_nodeRef>::iterator stack_it;
            for(stack_it = stack.begin(); stack_it != stack.end(); stack_it++)
            {
               if(GET_NODE(*stack_it)->get_kind() == gimple_cond_K)
               {
                  curr_block_index = (stmt_to_bloc.find(GET_INDEX_NODE(*stack_it)))->second;
                  curr_block = list_of_bloc.find(curr_block_index)->second;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found gimple_cond in block: " + STR((stmt_to_bloc.find(GET_INDEX_NODE(*stack_it)))->second));
                  break;
               }
            }
            THROW_ASSERT(curr_block, "Conditional expression not found in stack");
#endif

            // Retrieve def_block, the block which contains the definition of ssa_name (could be different from curr_block
            THROW_ASSERT(stmt_to_bloc.find(GET_INDEX_NODE(sn->CGetDefStmt())) != stmt_to_bloc.end(), "Statement not found in stmt_to_bloc map: " + STR(GET_INDEX_NODE(sn->CGetDefStmt())));
            const blocRef def_block = list_of_bloc.find((stmt_to_bloc.find(GET_INDEX_NODE(sn->CGetDefStmt())))->second)->second;
            THROW_ASSERT(def_block, "Definition block not found");

            // If the defined ssa variable is not only used in the conditional statement, copy the definition in the blocks
            // where it is used
            if(sn->CGetNumberUses() != 1)
            {
               /// FIXME:
               /// If right part is a subtree, it should be completly duplicated;
               /// For the moment we just stop the collapsing
               if(GET_NODE(gm->op1)->get_kind() != ssa_name_K and GET_NODE(gm->op1)->get_kind() != var_decl_K)
               {
                  break;
               }

               // This check is needed to avoid situations in which variables are modified internally to the conditional statement
               // An example is the ++ (or --) operator. In such case the collapsing procedure has to be stopped (break).
               // The control is carried out in the following way: if almost one instruction which is between the conditional
               // expression and the current gimple_modify_statement (ssa_name definition) has virtual operands, then the
               // collapsing procedure stops.
               bool in_between = false;
               bool memdef_found = false;
               for(const auto& stmt : curr_block->CGetStmtList())
               {
                  if(in_between and GET_NODE(stmt)->get_kind() == gimple_assign_K)
                  {
                     auto* gms = GetPointer<gimple_assign>(GET_NODE(stmt));
                     if(gms->memdef)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Statement " + STR(GET_INDEX_NODE(stmt)) + " contains vdef");
                        memdef_found = true;
                     }
                  }
                  if(GET_INDEX_NODE(stmt) == GET_INDEX_NODE(sn->CGetDefStmt()))
                  {
                     in_between = true;
                  }
               }
               if(memdef_found)
               {
                  break;
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---None of uses of current ssa contains virtual defs so continuing in collapsing");

               // If the definition of ssa_name is elsewhere, there is no need of copying it
               if(curr_block == def_block)
               {
                  // Group the uses of ssa variables on the basis of the basic block they belong to
                  OrderedMapStd<unsigned int, std::vector<tree_nodeRef>> copy_bloc_to_stmt;
                  for(auto const& use : sn->CGetUseStmts())
                  {
                     THROW_ASSERT(stmt_to_bloc.find(use.first->index) != stmt_to_bloc.end(), STR(use.first->index) + " is not in stmt_to_bloc");
                     unsigned int copy_block_index = (stmt_to_bloc.find(use.first->index))->second;
                     copy_bloc_to_stmt[copy_block_index].push_back(use.first);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ssa_name is used in operation " + STR(use.first->index) + " of BB" + STR(copy_block_index));
                  }

                  // Copy the definition statement into each block the ssa variable is used in
                  for(auto copy_block_it = copy_bloc_to_stmt.begin(); copy_block_it != copy_bloc_to_stmt.end(); ++copy_block_it)
                  {
                     const blocRef copy_block = list_of_bloc.find(copy_block_it->first)->second;
                     std::vector<tree_nodeRef> copy_block_use_stmts = copy_block_it->second;

                     if(copy_block != curr_block)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "Copying statement " + STR(GET_INDEX_NODE(sn->CGetDefStmt())) + " in block " + STR(copy_block_it->first) + " (!= " + STR((stmt_to_bloc.find(GET_INDEX_NODE(sn->CGetDefStmt())))->second) + ")");

                        // Create a new ssa_node, identical to the one being analyzed, but with different version number
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
                        unsigned int version = get_next_vers();
                        if(sn->var)
                        {
                           IR_schema[TOK(TOK_VAR)] = STR(GET_INDEX_NODE(sn->var));
                        }
                        else
                        {
                           IR_schema[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(sn->type));
                        }
                        IR_schema[TOK(TOK_VERS)] = STR(version);
                        IR_schema[TOK(TOK_VOLATILE)] = STR(sn->volatile_flag);
                        unsigned int sn_index = new_tree_node_id();
                        create_tree_node(sn_index, ssa_name_K, IR_schema);
                        tree_nodeRef tree_reindexRef_sn = GetTreeReindex(sn_index);
                        auto* new_sn = GetPointer<ssa_name>(GET_NODE(tree_reindexRef_sn));
                        for(const auto& use_stmt : sn->CGetUseStmts())
                        {
                           for(decltype(use_stmt.second) repetition = 0; repetition < use_stmt.second; repetition++)
                           {
                              new_sn->AddUseStmt(use_stmt.first);
                           }
                        }

                        // Replace the occurrences of the considered ssa variable with the newly created ssa variable
                        for(auto copy_block_uses_it = copy_block_use_stmts.begin(); copy_block_uses_it != copy_block_use_stmts.end(); ++copy_block_uses_it)
                        {
                           RecursiveReplaceTreeNode(*copy_block_uses_it, tn, tree_reindexRef_sn, *copy_block_uses_it, false);
                           new_sn->AddUseStmt(*copy_block_uses_it);
                        }

                        // Create a new node, the copy of the definition of ssa_name
                        IR_schema.clear();
                        IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                        IR_schema[TOK(TOK_OP0)] = STR(GET_INDEX_NODE(tree_reindexRef_sn));
                        IR_schema[TOK(TOK_OP1)] = STR(GET_INDEX_NODE(gm->op1));
                        if(gm->orig)
                        {
                           IR_schema[TOK(TOK_ORIG)] = STR(GET_INDEX_NODE(gm->orig));
                        }
                        unsigned int gm_index = new_tree_node_id();
                        create_tree_node(gm_index, gimple_assign_K, IR_schema);
                        tree_nodeRef tree_reindexRef_gm = GetTreeReindex(gm_index);

                        /// Update uses of right operand
                        if(GetPointer<const ssa_name>(GET_NODE(gm->op1)))
                        {
                           auto* sn_right = GetPointer<ssa_name>(GET_NODE(gm->op1));
                           sn_right->AddUseStmt(tree_reindexRef_gm);
                        }

                        auto* new_gm = GetPointer<gimple_assign>(GET_NODE(tree_reindexRef_gm));
#if HAVE_CODE_ESTIMATION_BUILT
                        new_gm->weight_information->instruction_size = gm->weight_information->instruction_size;
#if HAVE_RTL_BUILT
                        new_gm->weight_information->rtl_instruction_size = gm->weight_information->rtl_instruction_size;
#endif
                        new_gm->weight_information->recursive_weight = gm->weight_information->recursive_weight;
#endif
                        new_gm->memuse = gm->memuse;
                        new_gm->memdef = gm->memdef;
                        new_gm->vuses = gm->vuses;
                        new_gm->vdef = gm->vdef;
                        new_gm->vovers = gm->vovers;
                        // Need to control whether there are labels at the beginning of the block
                        // In such case, copy the statement after the labels
                        for(const auto& copy_block_stmt : copy_block->CGetStmtList())
                        {
                           if(GET_NODE(copy_block_stmt)->get_kind() != gimple_label_K)
                           {
                              copy_block->PushBefore(tree_reindexRef_gm, copy_block_stmt);
                              break;
                           }
                        }
                        // Update stmt_to_bloc map
                        stmt_to_bloc[GET_INDEX_NODE(tree_reindexRef_gm)] = copy_block_it->first;
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "copy_block = curr_block");
                     }
                  }
               }
            }

            if(gm->memuse)
            {
               // Target of collapsing is a conditional expression; we can't put into it expression with side effects
               if(gm->memdef)
               {
                  break;
               }
               tree_nodeRef top = stack.front();
               if(GET_NODE(top)->get_kind() == gimple_cond_K)
               {
                  auto* gc = GetPointer<gimple_cond>(GET_NODE(top));
                  gc->memuse = gm->memuse;
                  gc->vuses = gm->vuses;
                  gc->vovers = gm->vovers;
               }
               else if(GET_NODE(top)->get_kind() == gimple_assign_K)
               {
                  auto* top_gm = GetPointer<gimple_assign>(GET_NODE(top));
                  top_gm->memuse = gm->memuse;
                  top_gm->vuses = gm->vuses;
                  top_gm->vovers = gm->vovers;
               }
               else
               {
                  THROW_ERROR("Unsupported type of tree node during collapsing");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Substituting operand " + STR(GET_INDEX_NODE(tn)) + " with " + STR(GET_INDEX_NODE(gm->op1)));
            const tree_nodeRef tn_old = tn;
            // Change the operand into every statement in the curr_block which uses the considered ssa_name
            if(def_block == curr_block)
            {
               for(const auto& use : sn->CGetUseStmts())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking for operation " + STR(use.first->index));
                  THROW_ASSERT(stmt_to_bloc.find(use.first->index) != stmt_to_bloc.end(), "Statement not found in stmt_to_bloc map: " + STR(use.first->index));
                  unsigned int use_block_index = (stmt_to_bloc.find(use.first->index))->second;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation belongs to BB" + STR(use_block_index));
                  const blocRef use_block = list_of_bloc.find(use_block_index)->second;
                  if(use_block == curr_block)
                  {
                     auto statement = use.first;
                     RecursiveReplaceTreeNode(statement, tn_old, gm->op1, use.first, false);
                     // Erase usage information about the definition being erased of the current ssa variable in every ssa variable used in it
                     erase_usage_info(sn->CGetDefStmt(), sn->CGetDefStmt());
                     // Insert usage information about the target statement of the substitution in every ssa variable used in the definition being erased
                     insert_usage_info(sn->CGetDefStmt(), use.first);
                  }
               }
            }
#if HAVE_CODE_ESTIMATION_BUILT
            THROW_ASSERT(GetPointer<WeightedNode>(GET_NODE(stack.front())), "Tree node in front of the stack is not weighted. Kind is " + std::string(GET_NODE(stack.front())->get_kind_text()));
            if(GetPointer<WeightedNode>(GET_NODE(gm->op1)))
            {
               const std::map<ComponentTypeConstRef, ProbabilityDistribution>& weights = GetPointer<WeightedNode>(GET_NODE(gm->op1))->weight_information->recursive_weight;
               std::map<ComponentTypeConstRef, ProbabilityDistribution>::const_iterator w, w_end = weights.end();
               for(w = weights.begin(); w != w_end; ++w)
               {
                  GetPointer<WeightedNode>(GET_NODE(stack.front()))->weight_information->recursive_weight[w->first] += w->second;
               }
            }
#if HAVE_RTL_BUILT
            GetPointer<WeightedNode>(GET_NODE(stack.front()))->weight_information->rtl_instruction_size += gm->weight_information->rtl_instruction_size;
#endif
            GetPointer<WeightedNode>(GET_NODE(stack.front()))->weight_information->instruction_size += gm->weight_information->instruction_size;
#endif
            for(const auto& stmt : curr_block->CGetStmtList())
            {
               // Remove the definition statements contained in curr_block
               if(GET_INDEX_NODE(stmt) == GET_INDEX_NODE(sn->CGetDefStmt()))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed statement " + STR(stmt->index));
                  removed_nodes.insert(GET_INDEX_NODE(stmt));
                  curr_block->RemoveStmt(stmt);
                  break;
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Multiple definitions for ssa_name " + STR(GET_INDEX_NODE(tn)));
         }
         break;
      }

      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            collapse_into(funID, stmt_to_bloc, *arg, removed_nodes);
         }
         break;
      }
      case gimple_call_K:
      {
         stack.push_front(tn);
         auto* ce = GetPointer<gimple_call>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            collapse_into(funID, stmt_to_bloc, *arg, removed_nodes);
         }
         stack.pop_front();
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case constructor_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(GET_INDEX_NODE(tn)) + std::string("): ") + curr_tn->get_kind_text());
   }
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      const std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "collapse_into_" + STR(collapse_into_counter) + "_after_" + STR(tree_node_index) + ".raw";
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << *this;
      raw_file.close();
      const std::string gimple_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "collapse_into_" + STR(collapse_into_counter) + "_after_" + STR(tree_node_index) + ".gimple";
      collapse_into_counter++;
      std::ofstream gimple_file(gimple_file_name.c_str());
      PrintGimple(gimple_file, false);
      gimple_file.close();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Writing file " + gimple_file_name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended collapsing into " + STR(tree_node_index));
}

void tree_manager::ReplaceTreeNode(const tree_nodeRef& stmt, const tree_nodeRef& old_node, const tree_nodeRef& new_node)
{
   THROW_ASSERT(GetPointer<const gimple_node>(GET_NODE(stmt)), "Replacing ssa name starting from " + stmt->ToString());
   THROW_ASSERT(not GetPointer<const gimple_node>(GET_NODE(new_node)), "new node cannot be a gimple_node");
   THROW_ASSERT(not GetPointer<const gimple_node>(GET_NODE(old_node)), "old node cannot be a gimple_node: " + STR(old_node));
   /// Temporary variable used to pass first argument of RecursiveReplaceTreeNode by reference. Since it is a gimple_node it has not to be replaced
   tree_nodeRef temp = stmt;
   RecursiveReplaceTreeNode(temp, old_node, new_node, stmt, false);
}

void tree_manager::RecursiveReplaceTreeNode(tree_nodeRef& tn, const tree_nodeRef old_node, const tree_nodeRef& new_node, const tree_nodeRef& stmt, const bool definition) // NOLINT
{
#ifndef NDEBUG
   int function_debug_level = Param->GetFunctionDebugLevel(GET_CLASS(*this), __func__);
#endif
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   tree_nodeRef curr_tn = GET_NODE(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level,
                  "-->Replacing " + old_node->ToString() + "(" + STR(GET_INDEX_NODE(old_node)) + ") with " + new_node->ToString() + "(" + STR(GET_INDEX_NODE(new_node)) + ") starting from node " + STR(GET_INDEX_NODE(tn)) + "(" + STR(&tn) +
                      "): " + tn->ToString() + ")");
   if(GET_INDEX_NODE(tn) == GET_INDEX_NODE(old_node))
   {
      /// Check if we need to update uses or definitions
      const auto gn = GetPointer<const gimple_node>(GET_NODE(stmt));
      if(gn)
      {
         const auto ga = GetPointer<const gimple_assign>(GET_NODE(stmt));
         const auto gp = GetPointer<const gimple_phi>(GET_NODE(stmt));
         // Not in a assign and not in a phi or in right part of a phi or in right part of assign
         if(not definition)
         {
            THROW_ASSERT(gn->bb_index, stmt->ToString() + " is not in a basic block");
            const auto used_ssas = tree_helper::ComputeSsaUses(old_node);
            for(auto used_ssa : used_ssas)
            {
               for(decltype(used_ssa.second) counter = 0; counter < used_ssa.second; counter++)
               {
                  GetPointer<ssa_name>(GET_NODE(used_ssa.first))->RemoveUse(stmt);
               }
            }
            const auto new_used_ssas = tree_helper::ComputeSsaUses(new_node);
            for(auto new_used_ssa : new_used_ssas)
            {
               for(decltype(new_used_ssa.second) counter = 0; counter < new_used_ssa.second; counter++)
               {
                  GetPointer<ssa_name>(GET_NODE(new_used_ssa.first))->AddUseStmt(stmt);
               }
            }
         }
         else
         {
            if(gn->vdef and gn->vdef->index == old_node->index)
            {
               GetPointer<ssa_name>(GET_NODE(new_node))->SetDefStmt(stmt);
            }
            if(ga and ga->op0->index == old_node->index and GET_NODE(old_node)->get_kind() == ssa_name_K)
            {
               GetPointer<ssa_name>(GET_NODE(new_node))->SetDefStmt(stmt);
            }
            if(gp and gp->res->index == old_node->index)
            {
               THROW_ASSERT(GET_CONST_NODE(new_node)->get_kind() == ssa_name_K, GET_CONST_NODE(new_node)->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level, "---Setting " + STR(stmt) + " as new define statement of " + STR(new_node));
               GetPointer<ssa_name>(GET_NODE(new_node))->SetDefStmt(stmt);
            }
         }
      }
      tn = new_node;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level,
                     "<--Replaced " + old_node->ToString() + "(" + STR(GET_INDEX_NODE(old_node)) + ") with " + new_node->ToString() + "(" + STR(GET_INDEX_NODE(new_node)) + ") New statement " + STR(GET_INDEX_NODE(tn)) + ": " + tn->ToString() + ")");
      return;
   }
   auto gn = GetPointer<gimple_node>(curr_tn);
   if(gn)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level, "-->Checking virtuals");
      if(gn->memdef)
      {
         RecursiveReplaceTreeNode(gn->memdef, old_node, new_node, stmt, true);
      }
      if(gn->memuse)
      {
         RecursiveReplaceTreeNode(gn->memuse, old_node, new_node, stmt, false);
      }
      if(gn->vuses.find(old_node) != gn->vuses.end())
      {
         gn->vuses.erase(old_node);
         GetPointer<ssa_name>(GET_NODE(old_node))->RemoveUse(stmt);
         gn->vuses.insert(new_node);
         GetPointer<ssa_name>(GET_NODE(new_node))->AddUseStmt(stmt);
      }
      if(gn->vdef == old_node)
      {
         gn->vdef = new_node;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level, "<--Checked virtuals");
   }
   switch(curr_tn->get_kind())
   {
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         RecursiveReplaceTreeNode(gm->op0, old_node, new_node, stmt, true);
         RecursiveReplaceTreeNode(gm->op1, old_node, new_node, stmt, false);
         std::vector<tree_nodeRef>& uses = gm->use_set->variables;
         std::vector<tree_nodeRef>::iterator use, use_end = uses.end();
         for(use = uses.begin(); use != use_end; ++use)
         {
            RecursiveReplaceTreeNode(*use, old_node, new_node, stmt, false);
         }
         std::vector<tree_nodeRef>& clbs = gm->clobbered_set->variables;
         std::vector<tree_nodeRef>::iterator clb, clb_end = clbs.end();
         for(clb = clbs.begin(); clb != clb_end; ++clb)
         {
            RecursiveReplaceTreeNode(*clb, old_node, new_node, stmt, false);
         }
         if(gm->predicate)
         {
            RecursiveReplaceTreeNode(gm->predicate, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_asm_K:
      {
         auto* gasm = GetPointer<gimple_asm>(curr_tn);
         if(gasm->in)
         {
            RecursiveReplaceTreeNode(gasm->in, old_node, new_node, stmt, false);
         }
         if(gasm->out)
         {
            RecursiveReplaceTreeNode(gasm->out, old_node, new_node, stmt, true);
         }
         if(gasm->clob)
         {
            RecursiveReplaceTreeNode(gasm->clob, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         RecursiveReplaceTreeNode(gc->op0, old_node, new_node, stmt, false);
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(curr_tn);
         RecursiveReplaceTreeNode(ue->op, old_node, new_node, stmt, false);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         RecursiveReplaceTreeNode(be->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(be->op1, old_node, new_node, stmt, false);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         RecursiveReplaceTreeNode(se->op0, old_node, new_node, stmt, false);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursiveReplaceTreeNode(cond.first, old_node, new_node, stmt, false);
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         RecursiveReplaceTreeNode(te->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(te->op1, old_node, new_node, stmt, false);
         if(te->op2)
         {
            RecursiveReplaceTreeNode(te->op2, old_node, new_node, stmt, false);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         RecursiveReplaceTreeNode(qe->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(qe->op1, old_node, new_node, stmt, false);
         if(qe->op2)
         {
            RecursiveReplaceTreeNode(qe->op2, old_node, new_node, stmt, false);
         }
         if(qe->op3)
         {
            RecursiveReplaceTreeNode(qe->op3, old_node, new_node, stmt, false);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         RecursiveReplaceTreeNode(le->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(le->op1, old_node, new_node, stmt, false);
         if(le->op2)
            RecursiveReplaceTreeNode(le->op2, old_node, new_node, stmt, false);
         if(le->op3)
         {
            RecursiveReplaceTreeNode(le->op3, old_node, new_node, stmt, false);
         }
         if(le->op4)
         {
            RecursiveReplaceTreeNode(le->op4, old_node, new_node, stmt, false);
         }
         if(le->op5)
         {
            RecursiveReplaceTreeNode(le->op5, old_node, new_node, stmt, false);
         }
         if(le->op6)
         {
            RecursiveReplaceTreeNode(le->op6, old_node, new_node, stmt, false);
         }
         if(le->op7)
         {
            RecursiveReplaceTreeNode(le->op7, old_node, new_node, stmt, false);
         }
         if(le->op8)
         {
            RecursiveReplaceTreeNode(le->op8, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_phi_K:
      {
         auto* gp = GetPointer<gimple_phi>(curr_tn);
         for(auto& def_edge : gp->list_of_def_edge)
         {
            RecursiveReplaceTreeNode(def_edge.first, old_node, new_node, stmt, false);
         }
         RecursiveReplaceTreeNode(gp->res, old_node, new_node, stmt, true);
         break;
      }
      case target_mem_ref_K:
      {
         auto tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
         {
            RecursiveReplaceTreeNode(tmr->symbol, old_node, new_node, stmt, false);
         }
         if(tmr->base)
         {
            RecursiveReplaceTreeNode(tmr->base, old_node, new_node, stmt, false);
         }
         if(tmr->idx)
         {
            RecursiveReplaceTreeNode(tmr->idx, old_node, new_node, stmt, false);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         auto tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            RecursiveReplaceTreeNode(tmr->base, old_node, new_node, stmt, false);
         }
         if(tmr->idx)
         {
            RecursiveReplaceTreeNode(tmr->idx, old_node, new_node, stmt, false);
         }
         if(tmr->idx2)
         {
            RecursiveReplaceTreeNode(tmr->idx2, old_node, new_node, stmt, false);
         }
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case gimple_nop_K:
      case function_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         std::vector<tree_nodeRef>& args = ce->args;
         for(auto& arg : args)
         {
            RecursiveReplaceTreeNode(arg, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         std::vector<tree_nodeRef>& args = ce->args;
         for(auto& arg : args)
         {
            RecursiveReplaceTreeNode(arg, old_node, new_node, stmt, false);
         }
         std::vector<tree_nodeRef>& uses = ce->use_set->variables;
         std::vector<tree_nodeRef>::iterator use, use_end = uses.end();
         for(use = uses.begin(); use != use_end; ++use)
         {
            RecursiveReplaceTreeNode(*use, old_node, new_node, stmt, false);
         }
         std::vector<tree_nodeRef>& clbs = ce->clobbered_set->variables;
         std::vector<tree_nodeRef>::iterator clb, clb_end = clbs.end();
         for(clb = clbs.begin(); clb != clb_end; ++clb)
         {
            RecursiveReplaceTreeNode(*clb, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_return_K:
      {
         auto* gr = GetPointer<gimple_return>(curr_tn);
         if(gr->op)
         {
            RecursiveReplaceTreeNode(gr->op, old_node, new_node, stmt, false);
         }
         break;
      }
      case constructor_K:
      {
         auto* constr = GetPointer<constructor>(curr_tn);
         for(auto& idx_value : constr->list_of_idx_valu)
         {
            RecursiveReplaceTreeNode(idx_value.second, old_node, new_node, stmt, false);
         }
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(curr_tn);
         while(tl)
         {
            if(tl->valu)
            {
               RecursiveReplaceTreeNode(tl->valu, old_node, new_node, stmt, false);
            }
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
         }
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(GET_INDEX_NODE(tn)) + std::string("): ") + curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level,
                  "<--Replaced " + old_node->ToString() + "(" + STR(GET_INDEX_NODE(old_node)) + ") with " + new_node->ToString() + "(" + STR(GET_INDEX_NODE(new_node)) + ") New statement " + STR(GET_INDEX_NODE(tn)) + ": " + tn->ToString() + ")");
}

void tree_manager::erase_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Erase usage into node " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + "). Statement: " + STR(GET_INDEX_NODE(stmt)) + " (" + GET_NODE(stmt)->get_kind_text() + ")");
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   tree_nodeRef curr_tn = GET_NODE(tn);
   switch(curr_tn->get_kind())
   {
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         erase_usage_info(gm->op1, stmt);
         if(gm->predicate)
         {
            erase_usage_info(gm->predicate, stmt);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         erase_usage_info(gc->op0, stmt);
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         if(curr_tn->get_kind() == addr_expr_K)
         {
            /*            if(already_visited.find(curr_tn) != already_visited.end())
                        {
                           break;
                        }
                        already_visited.insert(curr_tn);*/
         }
         auto* ue = GetPointer<unary_expr>(curr_tn);
         erase_usage_info(ue->op, stmt);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         erase_usage_info(be->op0, stmt);
         erase_usage_info(be->op1, stmt);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         erase_usage_info(se->op0, stmt);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               erase_usage_info(cond.first, stmt);
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         erase_usage_info(te->op0, stmt);
         erase_usage_info(te->op1, stmt);
         if(te->op2)
         {
            erase_usage_info(te->op2, stmt);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         erase_usage_info(qe->op0, stmt);
         erase_usage_info(qe->op1, stmt);
         if(qe->op2)
         {
            erase_usage_info(qe->op2, stmt);
         }
         if(qe->op3)
         {
            erase_usage_info(qe->op3, stmt);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         erase_usage_info(le->op0, stmt);
         erase_usage_info(le->op1, stmt);
         if(le->op2)
            erase_usage_info(le->op2, stmt);
         if(le->op3)
         {
            erase_usage_info(le->op3, stmt);
         }
         if(le->op4)
         {
            erase_usage_info(le->op4, stmt);
         }
         if(le->op5)
         {
            erase_usage_info(le->op5, stmt);
         }
         if(le->op6)
         {
            erase_usage_info(le->op6, stmt);
         }
         if(le->op7)
         {
            erase_usage_info(le->op7, stmt);
         }
         if(le->op8)
         {
            erase_usage_info(le->op8, stmt);
         }
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_phi_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case function_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(curr_tn);
         for(const auto& use_stmt : sn->CGetUseStmts())
         {
            if(GET_INDEX_NODE(use_stmt.first) == GET_INDEX_NODE(stmt))
            {
               for(decltype(use_stmt.second) repetition = 0; repetition < use_stmt.second; repetition++)
               {
                  sn->RemoveUse(use_stmt.first);
               }
            }
         }
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            erase_usage_info(*arg, stmt);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            erase_usage_info(*arg, stmt);
         }
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case constructor_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case type_decl_K:
      case tree_vec_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(GET_INDEX_NODE(tn)) + std::string("): ") + curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Erased usage into node " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + "). Statement: " + STR(GET_INDEX_NODE(stmt)) + " (" + GET_NODE(stmt)->get_kind_text() + ")");
}

void tree_manager::insert_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Insert usage info into node " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + "). Statement: " + STR(GET_INDEX_NODE(stmt)) + " (" + GET_NODE(stmt)->get_kind_text() + ")");
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   tree_nodeRef curr_tn = GET_NODE(tn);
   switch(curr_tn->get_kind())
   {
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         insert_usage_info(gm->op1, stmt);
         if(gm->predicate)
         {
            insert_usage_info(gm->predicate, stmt);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         insert_usage_info(gc->op0, stmt);
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         if(curr_tn->get_kind() == addr_expr_K)
         {
            /*            if(already_visited.find(curr_tn) != already_visited.end())
                        {
                           break;
                        }
                        already_visited.insert(curr_tn);*/
         }
         auto* ue = GetPointer<unary_expr>(curr_tn);
         insert_usage_info(ue->op, stmt);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         insert_usage_info(be->op0, stmt);
         insert_usage_info(be->op1, stmt);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         insert_usage_info(se->op0, stmt);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               insert_usage_info(cond.first, stmt);
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         insert_usage_info(te->op0, stmt);
         insert_usage_info(te->op1, stmt);
         if(te->op2)
         {
            insert_usage_info(te->op2, stmt);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         insert_usage_info(qe->op0, stmt);
         insert_usage_info(qe->op1, stmt);
         if(qe->op2)
         {
            insert_usage_info(qe->op2, stmt);
         }
         if(qe->op3)
         {
            insert_usage_info(qe->op3, stmt);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         insert_usage_info(le->op0, stmt);
         insert_usage_info(le->op1, stmt);
         if(le->op2)
            insert_usage_info(le->op2, stmt);
         if(le->op3)
         {
            insert_usage_info(le->op3, stmt);
         }
         if(le->op4)
         {
            insert_usage_info(le->op4, stmt);
         }
         if(le->op5)
         {
            insert_usage_info(le->op5, stmt);
         }
         if(le->op6)
         {
            insert_usage_info(le->op6, stmt);
         }
         if(le->op7)
         {
            insert_usage_info(le->op7, stmt);
         }
         if(le->op8)
         {
            insert_usage_info(le->op8, stmt);
         }
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_phi_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case function_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(curr_tn);
         sn->AddUseStmt(stmt);
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            insert_usage_info(*arg, stmt);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            insert_usage_info(*arg, stmt);
         }
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case constructor_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(GET_INDEX_NODE(tn)) + std::string("): ") + curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Inserted usage info into node " + STR(GET_INDEX_NODE(tn)) + " (" + GET_NODE(tn)->get_kind_text() + "). Statement: " + STR(GET_INDEX_NODE(stmt)) + " (" + GET_NODE(stmt)->get_kind_text() + ")");
}

void tree_manager::add_parallel_loop()
{
   n_pl++;
}

unsigned int tree_manager::get_n_pl() const
{
   return n_pl;
}

void tree_manager::merge_tree_managers(const tree_managerRef& source_tree_manager)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting merging of new tree_manager");
   /// a declaration is uniquely identified by the name and by the scope
   /// in case the decl_node has a mangle the associated identifier_node is unique
   /// a decl_node without name is not added to the symbol table
   /// a decl_node local to a function_decl (scpe is a function_decl) is not added to the symbol table
   /// a decl_node local to a type_node without name is not added to the symbol table
   /// a static decl_node is not added to the symbol table
   /// memory_tag, parm_decl, result_decl are not added to the symbol table
   /// declaration with type_node local to a function are not considered
   /// the key of the declaration symbol table is structured as "name--scope"
   /// the value of the declaration symbol table is the nodeID of the tree_node in the tree_manager
   CustomUnorderedMapUnstable<std::string, unsigned int> global_decl_symbol_table;

   /// a type_node without name is not added to the symbol table
   /// a type_node local to a function_decl is not added to the symbol table
   /// the key of the type symbol table is structured as "name"
   /// the value of the type symbol table is the nodeID of the tree_node in the tree_manager
   CustomUnorderedMapUnstable<std::string, unsigned int> global_type_symbol_table;

   /// this table is used to give a name to unqualified record or union
   CustomUnorderedMap<unsigned int, std::string> global_type_unql_symbol_table;
   /// global static variable and function become global so we need some sort of uniquification
   CustomUnorderedSet<std::string> static_symbol_table;
   CustomUnorderedSet<std::string> static_function_header_symbol_table;
   null_deleter nullDel;
   tree_managerRef TM_this(this, nullDel);
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "before_tree_merge_" + STR(get_next_available_tree_node_id()) + ".raw";
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }

   /// build the symbol tables of tree_node inheriting from type_node and then from decl_node; decl_nodes have to be examinated later since they have to be examinated after the record/union types???
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Building the global symbol table of this tree manager");
   std::string symbol_name;
   std::string symbol_scope;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking types");
   for(const auto& ti : tree_nodes)
   {
      const tree_nodeRef tn = ti.second;
      auto* dn = GetPointer<decl_node>(tn);
      if(not dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking " + STR(ti.first));
         if(check_for_type(tn, TM_this, symbol_name, symbol_scope, global_type_symbol_table, ti.first))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            THROW_ASSERT(global_type_symbol_table.find(symbol_name) == global_type_symbol_table.end(),
                         "duplicated symbol in global_type_symbol_table: " + global_type_symbol_table.find(symbol_name)->first + " " + STR(global_type_symbol_table.find(symbol_name)->second) + " " + STR(ti.first));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is INSERTED in the symbol table " + symbol_name + " --> " + STR(ti.first));
            global_type_symbol_table[symbol_name] = ti.first;
            /// give a name to unql where possible
            if(tn->get_kind() == record_type_K and GetPointer<record_type>(tn)->unql and GetPointer<record_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               global_type_symbol_table["u struct " + symbol_name] = GET_INDEX_NODE(GetPointer<record_type>(tn)->unql);
               global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)] = "u struct " + symbol_name;
            }
            else if(tn->get_kind() == union_type_K and GetPointer<union_type>(tn)->unql and GetPointer<union_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               global_type_symbol_table["u union " + symbol_name] = GET_INDEX_NODE(GetPointer<union_type>(tn)->unql);
               global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)] = "u union " + symbol_name;
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked types");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking declarations");
   for(const auto& ti : tree_nodes)
   {
      /// check for decl_node
      const tree_nodeRef tn = ti.second;
      auto* dn = GetPointer<decl_node>(tn);
      if(dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking " + STR(ti.first));
         if(check_for_decl(tn, TM_this, symbol_name, symbol_scope, ti.first, global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not skipped");
            /// check for static
            if((GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->static_flag) or (GetPointer<var_decl>(tn) and (GetPointer<var_decl>(tn)->static_flag or GetPointer<var_decl>(tn)->static_static_flag)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Static declaration");
               THROW_ASSERT((GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->body) or (GetPointer<var_decl>(tn) and static_symbol_table.find(symbol_name + "-" + symbol_scope) == static_symbol_table.end()) or
                                (GetPointer<function_decl>(tn) and not GetPointer<function_decl>(tn)->body and static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) == static_function_header_symbol_table.end()),
                            "duplicated static symbol in the current tree_manager: " + symbol_name + "-" + symbol_scope + " " + STR(ti.first));
               if(GetPointer<function_decl>(tn) and !GetPointer<function_decl>(tn)->body)
               {
                  static_function_header_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               else
               {
                  static_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               continue;
            }
            /// check for function_decl undefined
            if(dn->get_kind() == function_decl_K and global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(GetPointer<function_decl>(tree_nodes.find(global_decl_symbol_table.find(symbol_name + "-" + symbol_scope)->second)->second)->body)
               {
                  continue;
               }
               if(GetPointer<function_decl>(tn)->undefined_flag)
               {
                  continue;
               }
               // else do overwrite
            }
            else if(dn->get_kind() == var_decl_K and global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(!GetPointer<var_decl>(tree_nodes.find(global_decl_symbol_table.find(symbol_name + "-" + symbol_scope)->second)->second)->extern_flag)
               {
                  continue;
               }
               if(GetPointer<var_decl>(tn)->extern_flag)
               {
                  continue;
               }
               // else do overwrite
            }
            else if(dn->get_kind() != function_decl_K && dn->get_kind() != var_decl_K && dn->get_kind() != type_decl_K)
            {
               // THROW_ASSERT(global_decl_symbol_table.find(symbol_name+"-"+symbol_scope) == global_decl_symbol_table.end(), "duplicated symbol in global_decl_symbol_table: "+global_decl_symbol_table.find(symbol_name+"-"+symbol_scope)->first + " == " +
               // boost::lexical_cast<std::string>(ti.first));
               continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding to global declaration table " + symbol_name + "-" + symbol_scope + " (" + STR(ti.first) + ")");

            global_decl_symbol_table[symbol_name + "-" + symbol_scope] = ti.first;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked declarations");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   auto gtust_i_end = global_type_unql_symbol_table.end();
   for(auto gtust_i = global_type_unql_symbol_table.begin(); gtust_i != gtust_i_end; ++gtust_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(gtust_i->first) + "-unqualified_type>" + gtust_i->second);
   }
   auto gst_i_end = global_decl_symbol_table.end();
   for(auto gst_i = global_decl_symbol_table.begin(); gst_i != gst_i_end; ++gst_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(gst_i->second) + "-decl>" + gst_i->first);
   }

   gst_i_end = global_type_symbol_table.end();
   for(auto gst_i = global_type_symbol_table.begin(); gst_i != gst_i_end; ++gst_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(gst_i->second) + "-type>" + gst_i->first);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Built table for this tree_manager");
   /// source static function declaration
   /// used to correctly rename declaration and definition of a function
   CustomUnorderedMapUnstable<std::string, std::string> source_static_symbol_table;
   /// the key is the old index while the values is the new index
   CustomUnorderedMapUnstable<unsigned int, unsigned int> remap;

   /// At the moment the reverse of remap; it is filled only with function_decl without body
   CustomUnorderedMapUnstable<unsigned int, unsigned int> reverse_remap;

   /// For each static function in source_tree_manager the index of the tree node (in source_tree_manager) of its forward declaration
   CustomUnorderedMapUnstable<std::string, unsigned int> static_forward_declaration_functions;

   /// For each static function in TM?source the index of the tree node (in source_tree_manager) of its implementation
   CustomUnorderedMapUnstable<std::string, unsigned int> static_implementation_functions;

   /// set of nodes that will be added to the current tree manager (this)
   OrderedSetStd<unsigned int> not_yet_remapped;
   OrderedSetStd<unsigned int> to_be_visited;
   tree_node_index_factory TNIF(remap, tree_managerRef(this, null_deleter()));

   /// FIXME: during one of the analysis of the tree nodes of source_tree_manager, new nodes can be inserted;
   /// if source_tree_manager->tree_nodes is a undirected_map, the insertion of a new node can invalidate the iterators because of rehash;
   /// for this reason we temporary copy the unordered_map in map
#if HAVE_UNORDERED
   std::map<unsigned int, tree_nodeRef> source_tree_nodes;
   for(auto source_tree_node : source_tree_manager->tree_nodes)
   {
      source_tree_nodes.insert(source_tree_node);
   }
#else
   auto& source_tree_nodes = source_tree_manager->tree_nodes;
#endif

   /// remap tree_node from source_tree_manager to this tree_manager
   /// first remap the types and then decl
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + STR(source_tree_nodes.size()) + " tree nodes of second tree manager");
   for(const auto& ti_source : source_tree_nodes)
   {
      const tree_nodeRef tn = ti_source.second;
      /// check for decl_node
      auto* dn = GetPointer<decl_node>(tn);
      if(not dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking " + STR(ti_source.first));
         if(GetPointer<identifier_node>(tn))
         {
            auto* id = GetPointer<identifier_node>(tn);
            unsigned int node_id = find_identifier_nodeID(id->strg);
            if(node_id)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Identifier FOUND: remapped!");
               remap[ti_source.first] = node_id;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Identifier NOT found: skipped!");
               continue;
            }
         }
         else if(check_for_type(tn, source_tree_manager, symbol_name, symbol_scope, global_type_symbol_table, ti_source.first))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(ti_source.first) + "-ntype>" + symbol_name + "-" + symbol_scope);
            if(tn->get_kind() == record_type_K and GetPointer<record_type>(tn)->unql and GetPointer<record_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               auto gst_it = global_type_symbol_table.find("u struct " + symbol_name);
               if(gst_it != global_type_symbol_table.end())
               {
                  remap[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)] = gst_it->second;
                  global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)] = "u struct " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)) + "-nutype>" + symbol_name);
                  // std::cout << "remap unql: " + gst_it->first << " " << gst_it->second << std::endl;
               }
               else
               {
                  unsigned int new_tree_index;
                  if(remap.find(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)) == remap.end())
                  {
                     new_tree_index = new_tree_node_id(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql));
                     not_yet_remapped.insert(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql));
                     remap[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)] = new_tree_index;
                     to_be_visited.insert(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql));
                  }
                  else
                  {
                     new_tree_index = remap[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)];
                  }
                  global_type_symbol_table["u struct " + symbol_name] = new_tree_index;
                  global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)] = "u struct " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "other " + STR(GET_INDEX_NODE(GetPointer<record_type>(tn)->unql)) + "-nutype>" + symbol_name);
               }
            }
            else if(tn->get_kind() == union_type_K and GetPointer<union_type>(tn)->unql and GetPointer<union_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               auto gst_it = global_type_symbol_table.find("u union " + symbol_name);
               if(gst_it != global_type_symbol_table.end())
               {
                  remap[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)] = gst_it->second;
                  global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)] = "u union " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)) + "-nutype>" + symbol_name);
                  // std::cout << "remap unql: " + gst_it->first << std::endl;
               }
               else
               {
                  unsigned int new_tree_index;
                  if(remap.find(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)) == remap.end())
                  {
                     new_tree_index = new_tree_node_id(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql));
                     remap[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)] = new_tree_index;
                     not_yet_remapped.insert(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql));
                     to_be_visited.insert(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql));
                  }
                  else
                  {
                     new_tree_index = remap[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)];
                  }
                  global_type_symbol_table["u union " + symbol_name] = new_tree_index;
                  global_type_unql_symbol_table[GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)] = "u union " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "other " + STR(GET_INDEX_NODE(GetPointer<union_type>(tn)->unql)) + "-nutype>" + symbol_name);
               }
            }
            auto gst_it = global_type_symbol_table.find(symbol_name);
            if(gst_it == global_type_symbol_table.end())
            {
               if(remap.find(ti_source.first) == remap.end())
               {
                  unsigned int new_tree_index = new_tree_node_id(ti_source.first);
                  global_type_symbol_table[symbol_name] = remap[ti_source.first] = new_tree_index;
                  not_yet_remapped.insert(ti_source.first);
                  to_be_visited.insert(ti_source.first);
               }
               continue;
            }
            else
            {
               // record type
               if(tn->get_kind() == record_type_K)
               {
                  // present in this tree_manager
                  if(tree_nodes.find(gst_it->second) != tree_nodes.end())
                  {
                     if(GetPointer<record_type>(tree_nodes.find(gst_it->second)->second) and GetPointer<record_type>(tree_nodes.find(gst_it->second)->second)->list_of_flds.empty() and !GetPointer<record_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RECORD TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(source_tree_nodes.find(gst_it->second) != source_tree_nodes.end(), "There is a symbol which is not present in this nor tree_manager nor the other");
                     if(GetPointer<record_type>(source_tree_nodes.find(gst_it->second)->second)->list_of_flds.empty() and !GetPointer<record_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RECORD TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
               }
               if(tn->get_kind() == union_type_K)
               {
                  // present in this tree_manager
                  if(tree_nodes.find(gst_it->second) != tree_nodes.end())
                  {
                     if(GetPointer<union_type>(tree_nodes.find(gst_it->second)->second) and GetPointer<union_type>(tree_nodes.find(gst_it->second)->second)->list_of_flds.empty() and !GetPointer<union_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "UNION TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(source_tree_nodes.find(gst_it->second) != source_tree_nodes.end(), "There is a symbol which is not present in this nor tree_manager nor the other");
                     if(GetPointer<union_type>(source_tree_nodes.find(gst_it->second)->second)->list_of_flds.empty() and !GetPointer<union_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "UNION TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
               }
               remap[ti_source.first] = gst_it->second;
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed tree nodes of second tree manager");

   /// remap tree_node from source_tree_manager to this tree_manager
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Remapping declaration nodes");
   for(auto ti_source : source_tree_nodes)
   {
      const tree_nodeRef tn = ti_source.second;
      /// check for decl_node
      auto* dn = GetPointer<decl_node>(tn);
      if(dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining declaration node " + STR(ti_source.first) + " of second tree:" + STR(tn));
         if(check_for_decl(tn, source_tree_manager, symbol_name, symbol_scope, ti_source.first, global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not suitable for symbol table");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(ti_source.first) + "-ndecl>" + symbol_name + "-" + symbol_scope);
         /// check for static
         if((GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->static_flag) or (GetPointer<var_decl>(tn) and (GetPointer<var_decl>(tn)->static_flag or GetPointer<var_decl>(tn)->static_static_flag)))
         {
            /// Management of forward declaration of static function
            if(GetPointer<function_decl>(tn))
            {
               /// Implementation node
               if(GetPointer<function_decl>(tn)->body)
               {
                  /// If this is the implementation already remaps so that also forward declaration can be remapped on the same node
                  const unsigned int new_index = new_tree_node_id(ti_source.first);
                  remap[ti_source.first] = new_index;
                  not_yet_remapped.insert(ti_source.first);
                  to_be_visited.insert(ti_source.first);

                  static_implementation_functions[symbol_name] = ti_source.first;

                  /// If we have already encountered forward declaration, remap it on the same node
                  if(static_forward_declaration_functions.find(symbol_name) != static_forward_declaration_functions.end())
                  {
                     remap[static_forward_declaration_functions[symbol_name]] = new_index;
                  }
               }
               /// Forward declaration
               else
               {
                  static_forward_declaration_functions[symbol_name] = ti_source.first;
                  /// Check if we have already encountered function implementation, remap this on the same node
                  if(static_implementation_functions.find(symbol_name) != static_implementation_functions.end())
                  {
                     remap[ti_source.first] = remap[static_implementation_functions[symbol_name]];
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Static");
            if(source_static_symbol_table.find(symbol_name) != source_static_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already present in source static symbol table");
               unsigned int node_id_source = source_tree_manager->find_identifier_nodeID(source_static_symbol_table.find(symbol_name)->second);

               /// CHECK: reference to source_tree_manager tree_node in tree_reindex
               dn->name = source_tree_manager->GetTreeReindex(node_id_source);
            }
            else if(static_symbol_table.find(symbol_name + "-" + symbol_scope) != static_symbol_table.end() or static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) != static_function_header_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already present in destination static symbol table");
               /// static function or var_decl with a conflicting name
               /// so we fix the name...
               /// and we fix in the source tree_manager
               unsigned int counter = 0;
               unsigned int node_id_this, node_id_source;
               do
               {
                  counter++;
                  node_id_this = find_identifier_nodeID(symbol_name + STR(counter));
                  node_id_source = source_tree_manager->find_identifier_nodeID(symbol_name + STR(counter));
               } while((node_id_this > 0 || node_id_source > 0));
               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
               tree_node_schema[TOK(TOK_STRG)] = symbol_name + STR(counter);
               node_id_source = source_tree_manager->new_tree_node_id(ti_source.first);
               source_tree_manager->create_tree_node(node_id_source, identifier_node_K, tree_node_schema);

               /// CHECK: reference to source_tree_manager tree_node in tree_reindex
               dn->name = source_tree_manager->GetTreeReindex(node_id_source);
               source_static_symbol_table[symbol_name] = symbol_name + STR(counter);
            }
            else
            {
               if(not GetPointer<function_decl>(tn))
               {
                  static_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               continue;
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Not Static");
            auto gst_it = global_decl_symbol_table.find(symbol_name + "-" + symbol_scope);
            if(gst_it == global_decl_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not yet in global decl symbol table");
               unsigned int new_index;
               new_index = new_tree_node_id(ti_source.first);
               global_decl_symbol_table[symbol_name + "-" + symbol_scope] = remap[ti_source.first] = new_index;
               if(dn->get_kind() == function_decl_K and not GetPointer<const function_decl>(tn)->body)
               {
                  reverse_remap[new_index] = ti_source.first;
               }
               if(dn->get_kind() == var_decl_K and GetPointer<const var_decl>(tn)->extern_flag)
               {
                  reverse_remap[new_index] = ti_source.first;
               }
               not_yet_remapped.insert(ti_source.first);
               to_be_visited.insert(ti_source.first);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               continue;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already in global decl symbol table");
               if((tn->get_kind() == function_decl_K
                   //                     and
                   //                     (
                   //                        !GetPointer<function_decl>(tree_nodes.find(gst_it->second)->second)
                   //                        ||
                   //                        !GetPointer<function_decl>(tree_nodes.find(gst_it->second)->second)->body
                   //                     )
                   and GetPointer<function_decl>(tn)->body) ||
                  // In the first tree manager there is extern type variable, in the second there is the definition
                  (tn->get_kind() == var_decl_K and (!GetPointer<var_decl>(tree_nodes.find(gst_it->second)->second) || GetPointer<var_decl>(tree_nodes.find(gst_it->second)->second)->extern_flag) and !GetPointer<var_decl>(tn)->extern_flag) ||
                  // In the first tree manager there is a function_decl without srcp, in the second there is a funcion_decl with srcp
                  (tn->get_kind() == function_decl_K and GetPointer<function_decl>(tree_nodes.find(gst_it->second)->second) and (GetPointer<function_decl>(tree_nodes.find(gst_it->second)->second))->include_name == "<built-in>" and
                   !(GetPointer<function_decl>(tn))->include_name.empty() and (GetPointer<function_decl>(tn))->include_name != "<built-in>"))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---but... forced overwriting");
                  /// The following statements force ti_source.first to overwrite gst_it->second
                  not_yet_remapped.insert(ti_source.first);
                  to_be_visited.insert(ti_source.first);
                  remap[ti_source.first] = gst_it->second;
                  /// If the symbol is in reverse_remap, it means that is a function_decl without body coming from other; it has not to be remapped
                  if(reverse_remap.find(gst_it->second) != reverse_remap.end())
                  {
                     THROW_ASSERT(not_yet_remapped.find(reverse_remap.find(gst_it->second)->second) != not_yet_remapped.end(), "Trying to cancel remapping of " + STR(reverse_remap.find(gst_it->second)->second));
                     not_yet_remapped.erase(reverse_remap.find(gst_it->second)->second);
                  }
               }
               else
               {
                  remap[ti_source.first] = gst_it->second;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      else
      {
         /// already performed
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Remapped declaration nodes");
   /// tree node visitor
   tree_node_reached TNR(remap, not_yet_remapped, tree_managerRef(this, null_deleter()));
   auto it_to_be_visited_end = to_be_visited.end();
   for(auto it_to_be_visited = to_be_visited.begin(); it_to_be_visited_end != it_to_be_visited; ++it_to_be_visited)
   {
      source_tree_manager->get_tree_node_const(*it_to_be_visited)->visit(&TNR);
   }

   /// compute the vertexes reached from all function_decl of source_tree_manager
   for(auto it = source_tree_manager->function_decl_nodes.begin(); it != source_tree_manager->function_decl_nodes.end(); ++it)
   {
      tree_nodeRef curr_tn = it->second;
      auto* fd = GetPointer<function_decl>(curr_tn);
      if(remap.find(it->first) == remap.end())
      {
         remap[it->first] = new_tree_node_id(it->first);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function decl: old " + STR(it->first) + " - new " + STR(remap[it->first]));
         fd->visit(&TNR);
         not_yet_remapped.insert(it->first);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Already remapped tree_node");
   auto it_remap_end = remap.end();
   for(auto it_remap = remap.begin(); it_remap_end != it_remap; ++it_remap)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Original " + STR(it_remap->first) + " New " + STR(it_remap->second));
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting remapping remaining nodes");
   auto it_not_yet_remapped_end = not_yet_remapped.end();
   for(auto it_not_yet_remapped = not_yet_remapped.begin(); it_not_yet_remapped_end != it_not_yet_remapped; ++it_not_yet_remapped)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Original " + STR(*it_not_yet_remapped) + " New " + STR(remap[*it_not_yet_remapped]));
      TNIF.create_tree_node(remap[*it_not_yet_remapped], source_tree_manager->get_tree_node_const(*it_not_yet_remapped));

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Type is " + std::string(source_tree_manager->get_tree_node_const(*it_not_yet_remapped)->get_kind_text()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "DONE");
   }
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "after_" + STR(get_next_available_tree_node_id()) + ".raw";
      std::cerr << raw_file_name << std::endl;
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended merging of new tree_manager");
}

bool tree_manager::check_for_decl(const tree_nodeRef& tn, const tree_managerRef& TM_this, std::string& symbol_name, std::string& symbol_scope, unsigned int ASSERT_PARAMETER(node_id),
                                  const CustomUnorderedMap<unsigned int, std::string>& global_type_unql_symbol_table)
{
   THROW_ASSERT(GetPointer<decl_node>(tn), "Node should be a declaration node");
   const decl_node* dn = GetPointer<decl_node>(tn);
   symbol_name = symbol_scope = "";
   /// check for name
   if(!dn->name)
   {
      return true;
   }
   if(GET_NODE(dn->name)->get_kind() == identifier_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---check_for_decl is considering: " + GetPointer<identifier_node>(GET_NODE(dn->name))->strg + ":" + dn->include_name);
   }
   /// check for memory_tag, parm_decl, result_decl
   if(GetPointer<memory_tag>(tn) || dn->get_kind() == parm_decl_K || dn->get_kind() == result_decl_K)
   {
      return true;
   }
   /// check for scope
   if(dn->scpe and GET_NODE(dn->scpe)->get_kind() == function_decl_K)
   {
      return true;
   }
   const function_decl* fd = GetPointer<function_decl>(tn);
   if(!fd and dn->scpe and GetPointer<type_node>(GET_NODE(dn->scpe)) and (!GetPointer<type_node>(GET_NODE(dn->scpe))->name and global_type_unql_symbol_table.find(GET_INDEX_NODE(dn->scpe)) == global_type_unql_symbol_table.end()))
   {
      return true;
   }
   if(dn->mngl)
   {
      THROW_ASSERT(GET_NODE(dn->mngl)->get_kind() == identifier_node_K, "expected an identifier_node: " + STR(GET_NODE(dn->mngl)->get_kind_text()));
      if(dn->name)
      {
         THROW_ASSERT(GET_NODE(dn->name)->get_kind() == identifier_node_K, "expected an identifier_node: " + STR(GET_NODE(dn->name)->get_kind_text()) + " " + STR(node_id));
         if(fd)
         {
            if(fd->builtin_flag)
            {
               symbol_name = GetPointer<identifier_node>(GET_NODE(dn->name))->strg;
               if(boost::algorithm::starts_with(symbol_name, "__builtin_"))
               {
                  symbol_name = symbol_name.substr(strlen("__builtin_"));
               }
               //               if(fd->undefined_flag && symbol_name.find("__builtin_") == std::string::npos)
               //                  symbol_name = "__builtin_" + symbol_name;
            }
            else
            {
               symbol_name = GetPointer<identifier_node>(GET_NODE(dn->mngl))->strg;
            }
         }
         else
         {
            symbol_name = GetPointer<identifier_node>(GET_NODE(dn->mngl))->strg;
         }
      }
      else
      {
         symbol_name = GetPointer<identifier_node>(GET_NODE(dn->mngl))->strg;
      }
   }
   else
   {
      THROW_ASSERT(GET_NODE(dn->name)->get_kind() == identifier_node_K, "expected an identifier_node: " + STR(GET_NODE(dn->name)->get_kind_text()) + " " + STR(node_id));
      symbol_name = GetPointer<identifier_node>(GET_NODE(dn->name))->strg;
      if(boost::algorithm::starts_with(symbol_name, "__builtin_"))
      {
         symbol_name = symbol_name.substr(strlen("__builtin_"));
      }
      //      if(fd && fd->undefined_flag && fd->builtin_flag && symbol_name.find("__builtin_") == std::string::npos)
      //            symbol_name = "__builtin_" + symbol_name;
   }
   if(dn->scpe and GetPointer<type_node>(GET_NODE(dn->scpe)) and GetPointer<type_node>(GET_NODE(dn->scpe))->name)
   {
      auto* type = GetPointer<type_node>(GET_NODE(dn->scpe));
      THROW_ASSERT(type, "expected a type_node: " + std::string(GET_NODE(dn->scpe)->get_kind_text()));
      /// declaration with type_node local to a function are not considered
      if(type->scpe and GET_NODE(type->scpe)->get_kind() == function_decl_K)
      {
         return true;
      }
      const auto quals = type->qual;
      std::string type_name;
      if(GET_NODE(type->name)->get_kind() == identifier_node_K)
      {
         type_name = GetPointer<identifier_node>(GET_NODE(type->name))->strg;
      }
      else
      {
         type_name = tree_helper::get_type_name(TM_this, GET_INDEX_NODE(type->name));
      }
      if(type->name and (GET_NODE(type->name)->get_kind() == type_decl_K))
      {
         if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
         {
            symbol_scope = tree_helper::return_qualifier_prefix(quals) + type_name;
         }
         else
         {
            symbol_scope = type_name;
         }
      }
      else if(type->name)
      {
         symbol_scope = "struct " + type_name;
      }
      if(fd)
      {
         symbol_scope = symbol_scope + "#F";
      }
   }
   else if(dn->scpe and GetPointer<type_node>(GET_NODE(dn->scpe)) and !GetPointer<type_node>(GET_NODE(dn->scpe))->name and global_type_unql_symbol_table.find(GET_INDEX_NODE(dn->scpe)) != global_type_unql_symbol_table.end())
   {
      symbol_scope = global_type_unql_symbol_table.find(GET_INDEX_NODE(dn->scpe))->second;
   }
   else if(fd)
   {
      if(fd->scpe && fd->static_flag)
      {
         symbol_scope = "#F:" + STR(GET_INDEX_NODE(fd->scpe));
      }
      else
      {
         symbol_scope = "#F";
      }
   }
   else
   {
      symbol_scope = "";
   }
   return false;
}

bool tree_manager::check_for_type(const tree_nodeRef& tn, const tree_managerRef& TM, std::string& symbol_name, std::string& symbol_scope, const CustomUnorderedMapUnstable<std::string, unsigned int>& global_type_symbol_table,
                                  unsigned int DEBUG_PARAMETER(node_id))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Checking for type " + STR(node_id));
   auto* type = GetPointer<type_node>(tn);
   symbol_name = symbol_scope = "";
   if(!type || !type->name)
   { /// integer_type and real_type have some duplication
      return true;
   }
   if(type->scpe and ((GET_NODE(type->scpe)->get_kind() == function_decl_K)))
   {
      return true;
   }
   const auto quals = type->qual;
   std::string type_name;
   THROW_ASSERT(GET_NODE(type->name), "wrong");
   if(GET_NODE(type->name)->get_kind() == identifier_node_K)
   {
      type_name = GetPointer<identifier_node>(GET_NODE(type->name))->strg;
   }
   else
   {
      type_name = tree_helper::get_type_name(TM, GET_INDEX_NODE(type->name));
   }
   if(type->name and (GET_NODE(type->name)->get_kind() == type_decl_K || type->unql))
   {
      if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
      {
         symbol_name = tree_helper::return_qualifier_prefix(quals) + type_name;
      }
      else if(type->unql)
      {
         symbol_name = type_name + "#unqualified";
      }
      else
      {
         symbol_name = type_name;
      }
   }
   else if(type->name and tn->get_kind() == record_type_K)
   {
      symbol_name = "struct " + type_name;
   }
   else if(type->name and tn->get_kind() == union_type_K)
   {
      symbol_name = "union " + type_name;
   }
   else if(type->name and tn->get_kind() == enumeral_type_K)
   {
      symbol_name = "enum " + type_name;
   }
   else if(type->name)
   {
      symbol_name = type_name;
   }
   else
   {
      return true;
   }

   return global_type_symbol_table.find(symbol_name) != global_type_symbol_table.end();
}

unsigned int tree_manager::find_identifier_nodeID(const std::string& str) const
{
   auto it = identifiers_unique_table.find(str);
   if(it == identifiers_unique_table.end())
   {
      return 0;
   }
   else
   {
      return it->second;
   }
}

void tree_manager::add_identifier_node(unsigned int nodeID, const bool& ASSERT_PARAMETER(op))
{
   THROW_ASSERT(op, "improper use of add_identifier_node");
   identifiers_unique_table[STOK(TOK_OPERATOR)] = nodeID;
}

const CustomUnorderedSet<unsigned int> tree_manager::GetAllFunctions() const
{
   CustomUnorderedSet<unsigned int> functions;
   std::map<unsigned int, tree_nodeRef>::const_iterator beg, end;
   for(beg = function_decl_nodes.begin(), end = function_decl_nodes.end(); beg != end; ++beg)
   {
      functions.insert(beg->first);
   }
   return functions;
}

unsigned int tree_manager::get_next_vers()
{
   if(next_vers == 0)
   {
      for(const auto& ti : tree_nodes)
      {
         if(ti.second)
         {
            tree_nodeRef tn = ti.second;
            if(tn->get_kind() == ssa_name_K)
            {
               auto* sn = GetPointer<ssa_name>(tn);
               if(sn->vers > next_vers)
               {
                  next_vers = sn->vers;
               }
            }
         }
      }
   }
#ifndef NDEBUG
   int function_debug_level = Param->GetFunctionDebugLevel(GET_CLASS(*this), __func__);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, function_debug_level, "---Created ssa " + STR(next_vers));
#endif
   return ++next_vers;
}

void tree_manager::add_goto()
{
   added_goto++;
}

unsigned int tree_manager::get_added_goto() const
{
   return added_goto;
}

void tree_manager::increment_removed_pointer_plus()
{
   removed_pointer_plus++;
}

unsigned int tree_manager::get_removed_pointer_plus() const
{
   return removed_pointer_plus;
}

void tree_manager::increment_removable_pointer_plus()
{
   removable_pointer_plus++;
}

unsigned int tree_manager::get_removable_pointer_plus() const
{
   return removable_pointer_plus;
}

void tree_manager::increment_unremoved_pointer_plus()
{
   unremoved_pointer_plus++;
}

unsigned int tree_manager::get_unremoved_pointer_plus() const
{
   return unremoved_pointer_plus;
}

tree_nodeRef tree_manager::CreateUniqueIntegerCst(long long int value, unsigned int type_index)
{
   auto key = std::make_pair(value, type_index);
   if(unique_integer_cst_map.find(key) != unique_integer_cst_map.end())
   {
      return unique_integer_cst_map.find(key)->second;
   }
   else
   {
      unsigned int integer_cst_nid = new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
      IR_schema[TOK(TOK_TYPE)] = STR(type_index);
      IR_schema[TOK(TOK_VALUE)] = STR(value);

      create_tree_node(integer_cst_nid, integer_cst_K, IR_schema);
      tree_nodeRef cost_node = GetTreeReindex(integer_cst_nid);
      unique_integer_cst_map[key] = cost_node;
      return cost_node;
   }
}

bool tree_manager::is_CPP() const
{
   return Param->isOption(OPT_input_format) && Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP;
}

bool tree_manager::is_top_function(const function_decl* fd) const
{
   if(fd->name)
   {
      tree_nodeRef id_name = GET_NODE(fd->name);
      std::string simple_name;
      if(id_name->get_kind() == identifier_node_K)
      {
         auto* in = GetPointer<identifier_node>(id_name);
         if(!in->operator_flag)
         {
            simple_name = in->strg;
         }
         if(!simple_name.empty())
         {
            if(Param->isOption(OPT_top_functions_names))
            {
               const auto top_functions_names = Param->getOption<const std::list<std::string>>(OPT_top_functions_names);
               for(const auto& top_function_name : top_functions_names)
               {
                  if(simple_name == top_function_name)
                  {
                     return true;
                  }
               }
            }
            if(Param->isOption(OPT_top_design_name))
            {
               const auto top_rtldesign_function = Param->getOption<std::string>(OPT_top_design_name);
               if(simple_name == top_rtldesign_function)
               {
                  return true;
               }
            }
         }
      }
   }
   return false;
}

bool tree_manager::check_ssa_uses(unsigned int fun_id) const
{
   tree_nodeRef fd_node = get_tree_node_const(fun_id);
   auto* fd = GetPointer<function_decl>(fd_node);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   for(const auto& bb : sl->list_of_bloc)
   {
      std::list<tree_nodeRef> whole_list = bb.second->CGetStmtList();
      for(auto& phi : bb.second->CGetPhiList())
      {
         whole_list.push_back(phi);
      }
      for(auto& statement_node : whole_list)
      {
         auto orig_string = statement_node->ToString();
         TreeNodeMap<size_t> ssas = tree_helper::ComputeSsaUses(statement_node);
         for(auto& ssa : ssas)
         {
            tree_nodeRef tn = GET_NODE(ssa.first);
            if(tn->get_kind() == ssa_name_K)
            {
               auto* sn = GetPointer<ssa_name>(tn);
               bool found = false;
               for(auto& use : sn->CGetUseStmts())
               {
                  const TreeNodeMap<size_t>& used_ssa = tree_helper::ComputeSsaUses(use.first);
                  if(used_ssa.find(tn) == used_ssa.end())
                  {
                     return false;
                  }
                  if(GET_INDEX_CONST_NODE(use.first) == GET_INDEX_CONST_NODE(statement_node))
                  {
                     found = true;
                  }
               }
               if(!found)
               {
                  std::cerr << "stmt: " << orig_string << " var: " << sn->ToString() << std::endl;
                  for(auto& stmt : sn->CGetUseStmts())
                  {
                     std::cerr << "stmt referred: " << GET_INDEX_CONST_NODE(stmt.first) << std::endl;
                  }
                  return false;
               }
            }
            else
            {
               THROW_ERROR("unexpected node");
            }
         }
      }
   }
   return true;
}
