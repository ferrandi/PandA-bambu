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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
#include "tree_manager.hpp"

#include "Parameter.hpp"
#include "compiler_wrapper.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "gimple_writer.hpp"
#include "raw_writer.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_node_factory.hpp"
#include "tree_node_finder.hpp"
#include "tree_nodes_merger.hpp"
#include "tree_reindex.hpp"
#include "tree_reindex_remove.hpp"
#include "utility.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include "config_HAVE_HEXFLOAT.hpp"
#include "config_NPROFILE.hpp"

#if !HAVE_HEXFLOAT
#include <cstdio>
#endif

tree_manager::tree_manager(const ParameterConstRef& _Param)
    : n_pl(0),
      added_goto(0),
      removed_pointer_plus(0),
      removable_pointer_plus(0),
      unremoved_pointer_plus(0),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE)),
      last_node_id(1),
      Param(_Param),
      next_vers(0)
{
}

#if TREE_MANAGER_CONTAINER_UNORDERED
void tree_manager::add_reserve(size_t n)
{
   tree_nodes.reserve(tree_nodes.size() + n);
}
#else
void tree_manager::add_reserve(size_t)
{
}
#endif

void tree_manager::FixTreeReindex()
{
   tree_reindex_remove trr(*this);
   for(auto& [idx, tn] : tree_nodes)
   {
      trr(tn);
   }
}

unsigned int tree_manager::get_implementation_node(unsigned int decl_node) const
{
   THROW_ASSERT(GetPointer<function_decl>(GetTreeNode(decl_node)),
                "Node " + STR(decl_node) + " is not a function decl: " + GetTreeNode(decl_node)->get_kind_text());
   if(GetPointer<function_decl>(GetTreeNode(decl_node))->body)
   {
      return decl_node;
   }
   else
   {
      return 0;
   }
}

void tree_manager::AddTreeNode(const tree_nodeRef& curr)
{
   THROW_ASSERT(curr && curr->get_kind() != tree_reindex_K,
                "Invalid tree node: " + (curr ? curr->get_kind_text() : "nullptr"));
   THROW_ASSERT(curr->index > 0, "Expected a positive index.");
   if(curr->index >= last_node_id)
   {
      last_node_id = curr->index + 1;
   }
   tree_nodes[curr->index] = curr;
}

tree_nodeRef tree_manager::GetTreeReindex(unsigned int index)
{
   THROW_ASSERT(index > 0, "Expected a positive index (" + STR(index) + ")");
   const auto it = tree_nodes.find(index);
   if(it != tree_nodes.end() && it->second)
   {
      THROW_ASSERT(index < last_node_id, "unexpected condition");
      return it->second;
   }
   if(index >= last_node_id)
   {
      last_node_id = index + 1;
   }
   return tree_nodeRef(new tree_reindex(index, tree_nodes[index]));
}

tree_nodeRef tree_manager::GetTreeNode(const unsigned int i) const
{
   const auto it = tree_nodes.find(i);
   THROW_ASSERT(it != tree_nodes.end(), "Tree node with index " + STR(i) + " not found");
   THROW_ASSERT(it->second, "Tree node with index " + STR(i) + " is nullptr");
   return it->second;
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
   const auto tn = GetFunction(function_name);
   return tn ? tn->index : 0;
}

tree_nodeRef tree_manager::GetFunction(const std::string& function_name) const
{
   null_deleter null_del;
   tree_managerConstRef TM(this, null_del);
   for(const auto& [idx, fdecl] : function_decl_nodes)
   {
      const auto fd = GetPointerS<const function_decl>(fdecl);
      const auto id_name = fd->name;
      std::string simple_name, mangled_name;
      if(id_name->get_kind() == identifier_node_K)
      {
         const auto in = GetPointerS<const identifier_node>(id_name);
         if(!in->operator_flag)
         {
            simple_name = in->strg;
         }
      }
      if(fd->mngl)
      {
         tree_nodeRef mangled_id_name = fd->mngl;
         if(mangled_id_name->get_kind() == identifier_node_K)
         {
            auto* in = GetPointer<identifier_node>(mangled_id_name);
            if(!in->operator_flag)
            {
               mangled_name = in->strg;
            }
         }
      }
      const auto name = [&]() {
         const auto fname = tree_helper::print_function_name(TM, fd);
         if(TM->is_CPP())
         {
            const auto demangled_name = cxa_demangle(fname);
            if(!demangled_name.empty())
            {
               return demangled_name.substr(0, demangled_name.find('('));
            }
         }
         return fname;
      }();
      if(name == function_name || function_name == std::string("-") ||
         (!simple_name.empty() && function_name == simple_name) ||
         (!mangled_name.empty() && mangled_name == function_name))
      {
         return fdecl;
      }
   }
   return nullptr;
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
      tree_nodeRef id_name = fd->name;
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
         tree_nodeRef mangled_id_name = fd->mngl;
         if(mangled_id_name->get_kind() == identifier_node_K)
         {
            auto* in = GetPointer<identifier_node>(mangled_id_name);
            if(!in->operator_flag)
            {
               mangled_name = in->strg;
            }
         }
      }
      std::string name = tree_helper::print_function_name(TM, fd);
      if(name == function_name || function_name == std::string("-") ||
         (!simple_name.empty() && function_name == simple_name) ||
         (!mangled_name.empty() && mangled_name == function_name))
      {
         function_id = function_decl_node.first;
      }
   }
   return function_id;
}

void tree_manager::print(std::ostream& os) const
{
   raw_writer RW(os);

   os << CompilerWrapper::bambu_ir_info;
#if TREE_MANAGER_CONTAINER_UNORDERED
   CustomOrderedMap<unsigned int, tree_nodeRef> ordered_tree_nodes(tree_nodes.begin(), tree_nodes.end());
   for(const auto& [idx, tn] : ordered_tree_nodes)
#else
   for(const auto& [idx, tn] : tree_nodes)
#endif
   {
      os << "@" << idx << " ";
      tn->visit(&RW);
      os << std::endl;
   }
}

void tree_manager::PrintGimple(std::ostream& os, const bool use_uid) const
{
   GimpleWriter gimple_writer(os, use_uid);

   for(const auto& [idx, fdecl] : function_decl_nodes)
   {
      if(GetPointer<function_decl>(fdecl)->body)
      {
         fdecl->visit(&gimple_writer);
      }
   }
}

tree_nodeRef tree_manager::create_tree_node(const unsigned int node_id, enum kind tree_node_type,
                                            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema)
{
   tree_node_factory TNF(tree_node_schema, *this);
   const auto tn = TNF.create_tree_node(node_id, tree_node_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
                  "---Created tree node " + STR(node_id) + ": " + STR(tn));
   return tn;
}

unsigned int tree_manager::new_tree_node_id(const unsigned int ask)
{
   if(ask && !tree_nodes.count(ask))
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

unsigned int tree_manager::find(enum kind tree_node_type,
                                const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema)
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
   // std::cout << "KEY: " + key + "[" << (find_cache.find(key) != find_cache.end() ? find_cache.at(key) : 0)
   // << "]" << std::endl;
   if(find_cache.find(key) != find_cache.end())
   {
      return find_cache.at(key);
   }
   tree_node_finder TNF(tree_node_schema);
   for(const auto& [idx, tn] : tree_nodes)
   {
      /// check if the corresponding tree node has been already created or not
      if(!tn)
      {
         continue;
      }

      if(tn->get_kind() == tree_node_type and TNF.check(tn))
      {
         find_cache[key] = idx;
         return idx;
      }
   }
   return 0;
}

void tree_manager::ReplaceTreeNode(const tree_nodeRef& stmt, const tree_nodeRef& old_node, const tree_nodeRef& new_node)
{
   THROW_ASSERT(GetPointer<const gimple_node>(stmt), "Replacing ssa name starting from " + stmt->ToString());
   THROW_ASSERT(!GetPointer<const gimple_node>(new_node), "new node cannot be a gimple_node");
   THROW_ASSERT(!GetPointer<const gimple_node>(old_node), "old node cannot be a gimple_node: " + STR(old_node));
   /// Temporary variable used to pass first argument of RecursiveReplaceTreeNode by reference. Since it is a
   /// gimple_node it has not to be replaced
   tree_nodeRef temp = stmt;
   RecursiveReplaceTreeNode(temp, old_node, new_node, stmt, false);
}

void tree_manager::RecursiveReplaceTreeNode(tree_nodeRef& tn, const tree_nodeRef old_node, const tree_nodeRef& new_node,
                                            const tree_nodeRef& stmt, const bool definition) // NOLINT
{
   tree_nodeRef curr_tn = tn;
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
                  "-->Replacing " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                      new_node->ToString() + "(" + new_node->get_kind_text() +
                      ") starting from node: " + tn->ToString() + "(" + tn->get_kind_text() + ")");
   if(tn->index == old_node->index)
   {
      /// Check if we need to update uses or definitions
      const auto gn = GetPointer<const gimple_node>(stmt);
      if(gn)
      {
         const auto ga = GetPointer<const gimple_assign>(stmt);
         const auto gp = GetPointer<const gimple_phi>(stmt);
         // Not in a assign and not in a phi or in right part of a phi or in right part of assign
         if(!definition)
         {
            THROW_ASSERT(gn->bb_index, stmt->ToString() + " is not in a basic block");
            const auto used_ssas = tree_helper::ComputeSsaUses(old_node);
            for(const auto& used_ssa : used_ssas)
            {
               for(decltype(used_ssa.second) counter = 0; counter < used_ssa.second; counter++)
               {
                  GetPointerS<ssa_name>(used_ssa.first)->RemoveUse(stmt);
               }
            }
#ifndef NDEBUG
            tn = new_node;
#endif
            const auto new_used_ssas = tree_helper::ComputeSsaUses(new_node);
            for(const auto& new_used_ssa : new_used_ssas)
            {
               for(decltype(new_used_ssa.second) counter = 0; counter < new_used_ssa.second; counter++)
               {
                  GetPointerS<ssa_name>(new_used_ssa.first)->AddUseStmt(stmt);
               }
            }
         }
         else
         {
            if(gn->vdef && gn->vdef->index == old_node->index)
            {
               const auto vssa = GetPointerS<ssa_name>(new_node);
               vssa->SetDefStmt(stmt);
            }
            if(gn->memdef && gn->memdef->index == old_node->index)
            {
               const auto vssa = GetPointerS<ssa_name>(new_node);
               vssa->SetDefStmt(stmt);
            }
            if(ga && ga->op0->index == old_node->index && old_node->get_kind() == ssa_name_K)
            {
               GetPointerS<ssa_name>(new_node)->SetDefStmt(stmt);
            }
            if(gp && gp->res->index == old_node->index && !GetPointer<cst_node>(new_node))
            {
               THROW_ASSERT(new_node->get_kind() == ssa_name_K, new_node->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
                              "---Setting " + STR(stmt) + " as new define statement of " + STR(new_node));
               GetPointerS<ssa_name>(new_node)->SetDefStmt(stmt);
            }
         }
      }
      tn = new_node;
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
                     "<--Replaced " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                         new_node->ToString() + "(" + new_node->get_kind_text() + ") New statement: " + tn->ToString());
      return;
   }
   const auto gn = GetPointer<gimple_node>(curr_tn);
   if(gn)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param), "-->Checking virtuals");
      if(gn->memdef)
      {
         RecursiveReplaceTreeNode(gn->memdef, old_node, new_node, stmt, true);
      }
      if(gn->memuse)
      {
         RecursiveReplaceTreeNode(gn->memuse, old_node, new_node, stmt, false);
      }
      const auto vuse_it = gn->vuses.find(old_node);
      if(vuse_it != gn->vuses.end())
      {
         gn->vuses.erase(vuse_it);
         const auto old_vssa = GetPointerS<ssa_name>(old_node);
         old_vssa->RemoveUse(stmt);
         if(gn->AddVuse(new_node))
         {
            const auto new_vssa = GetPointerS<ssa_name>(new_node);
            new_vssa->AddUseStmt(stmt);
         }
      }
      const auto vover_it = gn->vovers.find(old_node);
      if(vover_it != gn->vovers.end())
      {
         gn->vovers.erase(vover_it);
         const auto old_vssa = GetPointerS<ssa_name>(old_node);
         old_vssa->RemoveUse(stmt);
         if(gn->AddVover(new_node))
         {
            const auto new_vssa = GetPointerS<ssa_name>(new_node);
            new_vssa->AddUseStmt(stmt);
         }
      }
      if(gn->vdef)
      {
         RecursiveReplaceTreeNode(gn->vdef, old_node, new_node, stmt, true);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param), "<--Checked virtuals");
   }
   switch(curr_tn->get_kind())
   {
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<gimple_assign>(curr_tn);
         RecursiveReplaceTreeNode(gm->op0, old_node, new_node, stmt, new_node->get_kind() == ssa_name_K);
         RecursiveReplaceTreeNode(gm->op1, old_node, new_node, stmt, false);
         for(auto& use : gm->use_set->variables)
         {
            RecursiveReplaceTreeNode(use, old_node, new_node, stmt, false);
         }
         for(auto& clb : gm->clobbered_set->variables)
         {
            RecursiveReplaceTreeNode(clb, old_node, new_node, stmt, false);
         }
         if(gm->predicate)
         {
            RecursiveReplaceTreeNode(gm->predicate, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_asm_K:
      {
         auto* gasm = GetPointerS<gimple_asm>(curr_tn);
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
         auto* gc = GetPointerS<gimple_cond>(curr_tn);
         RecursiveReplaceTreeNode(gc->op0, old_node, new_node, stmt, false);
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointerS<unary_expr>(curr_tn);
         RecursiveReplaceTreeNode(ue->op, old_node, new_node, stmt, false);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointerS<binary_expr>(curr_tn);
         RecursiveReplaceTreeNode(be->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(be->op1, old_node, new_node, stmt, false);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointerS<gimple_switch>(curr_tn);
         RecursiveReplaceTreeNode(se->op0, old_node, new_node, stmt, false);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointerS<gimple_multi_way_if>(curr_tn);
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
         auto* te = GetPointerS<ternary_expr>(curr_tn);
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
         auto* qe = GetPointerS<quaternary_expr>(curr_tn);
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
         auto* le = GetPointerS<lut_expr>(curr_tn);
         RecursiveReplaceTreeNode(le->op0, old_node, new_node, stmt, false);
         RecursiveReplaceTreeNode(le->op1, old_node, new_node, stmt, false);
         if(le->op2)
         {
            RecursiveReplaceTreeNode(le->op2, old_node, new_node, stmt, false);
         }
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
         auto* gp = GetPointerS<gimple_phi>(curr_tn);
         for(auto& def_edge : gp->list_of_def_edge)
         {
            RecursiveReplaceTreeNode(def_edge.first, old_node, new_node, stmt, false);
         }
         RecursiveReplaceTreeNode(gp->res, old_node, new_node, stmt, true);
         break;
      }
      case target_mem_ref_K:
      {
         auto tmr = GetPointerS<target_mem_ref>(curr_tn);
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
         auto tmr = GetPointerS<target_mem_ref461>(curr_tn);
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
         auto* ce = GetPointerS<call_expr>(curr_tn);
         for(auto& arg : ce->args)
         {
            RecursiveReplaceTreeNode(arg, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* gc = GetPointerS<gimple_call>(curr_tn);
         for(auto& arg : gc->args)
         {
            RecursiveReplaceTreeNode(arg, old_node, new_node, stmt, false);
         }
         for(auto& use : gc->use_set->variables)
         {
            RecursiveReplaceTreeNode(use, old_node, new_node, stmt, false);
         }
         for(auto& clb : gc->clobbered_set->variables)
         {
            RecursiveReplaceTreeNode(clb, old_node, new_node, stmt, false);
         }
         break;
      }
      case gimple_return_K:
      {
         auto* gr = GetPointerS<gimple_return>(curr_tn);
         if(gr->op)
         {
            RecursiveReplaceTreeNode(gr->op, old_node, new_node, stmt, false);
         }
         break;
      }
      case constructor_K:
      {
         auto* constr = GetPointerS<constructor>(curr_tn);
         for(auto& idx_value : constr->list_of_idx_valu)
         {
            RecursiveReplaceTreeNode(idx_value.second, old_node, new_node, stmt, false);
         }
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointerS<tree_list>(curr_tn);
         while(tl)
         {
            if(tl->valu)
            {
               RecursiveReplaceTreeNode(tl->valu, old_node, new_node, stmt, false);
            }
            tl = tl->chan ? GetPointerS<tree_list>(tl->chan) : nullptr;
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
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") +
                     curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
                  "<--Replaced " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                      new_node->ToString() + "(" + new_node->get_kind_text() + ") New statement: " + tn->ToString());
}

void tree_manager::erase_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Erase usage into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
   tree_nodeRef curr_tn = tn;
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
         for(const auto& cond : gmwi->list_of_cond)
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
         {
            erase_usage_info(le->op2, stmt);
         }
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
            if(use_stmt.first->index == stmt->index)
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
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") +
                     curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "<--Erased usage into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
}

void tree_manager::insert_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Insert usage info into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
   tree_nodeRef curr_tn = tn;
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
         for(const auto& cond : gmwi->list_of_cond)
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
         {
            insert_usage_info(le->op2, stmt);
         }
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
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") +
                     curr_tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "<--Inserted usage info into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
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
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Starting merging of new tree_manager");
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
   const tree_managerRef TM_this(this, null_deleter{});

   // TODO: this call may be useless since FixTreeReindex is called on every tree_manager after parsing is completed,
   // thus no "un-fixed" tree_manager should be able to call merge_tree_managers
   FixTreeReindex();

   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) +
                                  "/before_tree_merge_" + STR(get_next_available_tree_node_id()) + ".raw";
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }

   /// build the symbol tables of tree_node inheriting from type_node and then from decl_node; decl_nodes have to be
   /// examinated later since they have to be examinated after the record/union types???
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Building the global symbol table of this tree manager");
   std::string symbol_name;
   std::string symbol_scope;
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Checking types");
   for(const auto& [idx, tn] : tree_nodes)
   {
      auto* dn = GetPointer<decl_node>(tn);
      if(not dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(idx));
         if(check_for_type(tn, TM_this, symbol_name, symbol_scope, global_type_symbol_table, idx))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            THROW_ASSERT(global_type_symbol_table.find(symbol_name) == global_type_symbol_table.end(),
                         "duplicated symbol in global_type_symbol_table: " + symbol_name + " " +
                             STR(global_type_symbol_table.at(symbol_name)) + " " + STR(idx));
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---Is INSERTED in the symbol table " + symbol_name + " --> " + STR(idx));
            global_type_symbol_table[symbol_name] = idx;
            /// give a name to unql where possible
            if(tn->get_kind() == record_type_K and GetPointer<record_type>(tn)->unql and
               GetPointer<record_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               global_type_symbol_table["u struct " + symbol_name] = GetPointer<record_type>(tn)->unql->index;
               global_type_unql_symbol_table[GetPointer<record_type>(tn)->unql->index] = "u struct " + symbol_name;
            }
            else if(tn->get_kind() == union_type_K and GetPointer<union_type>(tn)->unql and
                    GetPointer<union_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               global_type_symbol_table["u union " + symbol_name] = GetPointer<union_type>(tn)->unql->index;
               global_type_unql_symbol_table[GetPointer<union_type>(tn)->unql->index] = "u union " + symbol_name;
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Checked types");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Checking declarations");
   for(const auto& [idx, tn] : tree_nodes)
   {
      /// check for decl_node
      auto* dn = GetPointer<decl_node>(tn);
      if(dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(idx));
         if(check_for_decl(tn, TM_this, symbol_name, symbol_scope, idx, global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Not skipped");
            /// check for static
            if((GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->static_flag) or
               (GetPointer<var_decl>(tn) and
                (GetPointer<var_decl>(tn)->static_flag or GetPointer<var_decl>(tn)->static_static_flag)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Static declaration");
               THROW_ASSERT(
                   (GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->body) or
                       (GetPointer<var_decl>(tn) and
                        static_symbol_table.find(symbol_name + "-" + symbol_scope) == static_symbol_table.end()) or
                       (GetPointer<function_decl>(tn) and not GetPointer<function_decl>(tn)->body and
                        static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) ==
                            static_function_header_symbol_table.end()),
                   "duplicated static symbol in the current tree_manager: " + symbol_name + "-" + symbol_scope + " " +
                       STR(idx));
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
            if(dn->get_kind() == function_decl_K and
               global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(GetPointer<function_decl>(
                      tree_nodes.at(global_decl_symbol_table.at(symbol_name + "-" + symbol_scope)))
                      ->body)
               {
                  continue;
               }
               if(GetPointer<function_decl>(tn)->undefined_flag)
               {
                  continue;
               }
               // else do overwrite
            }
            else if(dn->get_kind() == var_decl_K and
                    global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(!GetPointer<var_decl>(tree_nodes.at(global_decl_symbol_table.at(symbol_name + "-" + symbol_scope)))
                       ->extern_flag)
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
               // THROW_ASSERT(global_decl_symbol_table.find(symbol_name+"-"+symbol_scope) ==
               // global_decl_symbol_table.end(), "duplicated symbol in global_decl_symbol_table:
               // "+global_decl_symbol_table.find(symbol_name+"-"+symbol_scope)->first + " == " +
               // std::to_string(idx));
               continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---Adding to global declaration table " + symbol_name + "-" + symbol_scope + " (" +
                               STR(idx) + ")");

            global_decl_symbol_table[symbol_name + "-" + symbol_scope] = idx;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Checked declarations");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");

   auto gtust_i_end = global_type_unql_symbol_table.end();
   for(auto gtust_i = global_type_unql_symbol_table.begin(); gtust_i != gtust_i_end; ++gtust_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gtust_i->first) + "-unqualified_type>" + gtust_i->second);
   }
   auto gst_i_end = global_decl_symbol_table.end();
   for(auto gst_i = global_decl_symbol_table.begin(); gst_i != gst_i_end; ++gst_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gst_i->second) + "-decl>" + gst_i->first);
   }

   gst_i_end = global_type_symbol_table.end();
   for(auto gst_i = global_type_symbol_table.begin(); gst_i != gst_i_end; ++gst_i)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gst_i->second) + "-type>" + gst_i->first);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Built table for this tree_manager");
   /// source static function declaration
   /// used to correctly rename declaration and definition of a function
   CustomUnorderedMapUnstable<std::string, std::string> source_static_symbol_table;
   /// the key is the old index while the values is the new index
   CustomUnorderedMapUnstable<unsigned int, unsigned int> remap;

   /// At the moment the reverse of remap; it is filled only with function_decl without body
   CustomUnorderedMapUnstable<unsigned int, unsigned int> reverse_remap;

   /// For each static function in source_tree_manager the index of the tree node (in source_tree_manager) of its
   /// forward declaration
   CustomUnorderedMapUnstable<std::string, unsigned int> static_forward_declaration_functions;

   /// For each static function in TM?source the index of the tree node (in source_tree_manager) of its implementation
   CustomUnorderedMapUnstable<std::string, unsigned int> static_implementation_functions;

   /// set of nodes that will be added to the current tree manager (this)
   OrderedSetStd<unsigned int> not_yet_remapped;
   OrderedSetStd<unsigned int> to_be_visited;
   tree_node_index_factory TNIF(remap, TM_this);

/// FIXME: during one of the analysis of the tree nodes of source_tree_manager, new nodes can be inserted;
/// if source_tree_manager->tree_nodes is a undirected_map, the insertion of a new node can invalidate the iterators
/// because of rehash; for this reason we temporary copy the unordered_map in map
#if TREE_MANAGER_CONTAINER_UNORDERED
   // TODO: could be just a simple copy, no need for it to be stable or something
   CustomUnorderedMapStable<unsigned int, tree_nodeRef> source_tree_nodes(source_tree_manager->tree_nodes.begin(),
                                                                          source_tree_manager->tree_nodes.end());
   tree_nodes.reserve((tree_nodes.size() + source_tree_manager->tree_nodes.size()) * 9UL / 8UL);
#else
   auto& source_tree_nodes = source_tree_manager->tree_nodes;
#endif

   /// remap tree_node from source_tree_manager to this tree_manager
   /// first remap the types and then decl
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Analyzing " + STR(source_tree_nodes.size()) + " tree nodes of second tree manager");
   for(const auto& ti_source : source_tree_nodes)
   {
      const tree_nodeRef tn = ti_source.second;
      /// check for decl_node
      auto* dn = GetPointer<decl_node>(tn);
      if(not dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(ti_source.first));
         if(GetPointer<identifier_node>(tn))
         {
            auto* id = GetPointer<identifier_node>(tn);
            unsigned int node_id = find_identifier_nodeID(id->strg);
            if(node_id)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Identifier FOUND: remapped!");
               remap[ti_source.first] = node_id;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Identifier NOT found: skipped!");
               continue;
            }
         }
         else if(check_for_type(tn, source_tree_manager, symbol_name, symbol_scope, global_type_symbol_table,
                                ti_source.first))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           STR(ti_source.first) + "-ntype>" + symbol_name + "-" + symbol_scope);
            if(tn->get_kind() == record_type_K and GetPointer<record_type>(tn)->unql and
               GetPointer<record_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               auto gst_it = global_type_symbol_table.find("u struct " + symbol_name);
               if(gst_it != global_type_symbol_table.end())
               {
                  remap[GetPointer<record_type>(tn)->unql->index] = gst_it->second;
                  global_type_unql_symbol_table[GetPointer<record_type>(tn)->unql->index] = "u struct " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 STR(GetPointer<record_type>(tn)->unql->index) + "-nutype>" + symbol_name);
                  // std::cout << "remap unql: " + gst_it->first << " " << gst_it->second << std::endl;
               }
               else
               {
                  unsigned int new_tree_index;
                  if(remap.find(GetPointer<record_type>(tn)->unql->index) == remap.end())
                  {
                     new_tree_index = new_tree_node_id(GetPointer<record_type>(tn)->unql->index);
                     not_yet_remapped.insert(GetPointer<record_type>(tn)->unql->index);
                     remap[GetPointer<record_type>(tn)->unql->index] = new_tree_index;
                     to_be_visited.insert(GetPointer<record_type>(tn)->unql->index);
                  }
                  else
                  {
                     new_tree_index = remap[GetPointer<record_type>(tn)->unql->index];
                  }
                  global_type_symbol_table["u struct " + symbol_name] = new_tree_index;
                  global_type_unql_symbol_table[GetPointer<record_type>(tn)->unql->index] = "u struct " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "other " + STR(GetPointer<record_type>(tn)->unql->index) + "-nutype>" + symbol_name);
               }
            }
            else if(tn->get_kind() == union_type_K and GetPointer<union_type>(tn)->unql and
                    GetPointer<union_type>(tn)->qual == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               auto gst_it = global_type_symbol_table.find("u union " + symbol_name);
               if(gst_it != global_type_symbol_table.end())
               {
                  remap[GetPointer<union_type>(tn)->unql->index] = gst_it->second;
                  global_type_unql_symbol_table[GetPointer<union_type>(tn)->unql->index] = "u union " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 STR(GetPointer<union_type>(tn)->unql->index) + "-nutype>" + symbol_name);
                  // std::cout << "remap unql: " + gst_it->first << std::endl;
               }
               else
               {
                  unsigned int new_tree_index;
                  if(remap.find(GetPointer<union_type>(tn)->unql->index) == remap.end())
                  {
                     new_tree_index = new_tree_node_id(GetPointer<union_type>(tn)->unql->index);
                     remap[GetPointer<union_type>(tn)->unql->index] = new_tree_index;
                     not_yet_remapped.insert(GetPointer<union_type>(tn)->unql->index);
                     to_be_visited.insert(GetPointer<union_type>(tn)->unql->index);
                  }
                  else
                  {
                     new_tree_index = remap[GetPointer<union_type>(tn)->unql->index];
                  }
                  global_type_symbol_table["u union " + symbol_name] = new_tree_index;
                  global_type_unql_symbol_table[GetPointer<union_type>(tn)->unql->index] = "u union " + symbol_name;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "other " + STR(GetPointer<union_type>(tn)->unql->index) + "-nutype>" + symbol_name);
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
                     if(GetPointer<record_type>(tree_nodes.at(gst_it->second)) and
                        GetPointer<record_type>(tree_nodes.at(gst_it->second))->list_of_flds.empty() and
                        !GetPointer<record_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "RECORD TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(source_tree_nodes.find(gst_it->second) != source_tree_nodes.end(),
                                  "There is a symbol which is not present in this nor tree_manager nor the other");
                     if(GetPointer<record_type>(source_tree_nodes.at(gst_it->second))->list_of_flds.empty() and
                        !GetPointer<record_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "RECORD TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
               }
               if(tn->get_kind() == union_type_K)
               {
                  // present in this tree_manager
                  if(tree_nodes.find(gst_it->second) != tree_nodes.end())
                  {
                     if(GetPointer<union_type>(tree_nodes.at(gst_it->second)) and
                        GetPointer<union_type>(tree_nodes.at(gst_it->second))->list_of_flds.empty() and
                        !GetPointer<union_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "UNION TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(source_tree_nodes.find(gst_it->second) != source_tree_nodes.end(),
                                  "There is a symbol which is not present in this nor tree_manager nor the other");
                     if(GetPointer<union_type>(source_tree_nodes.at(gst_it->second))->list_of_flds.empty() and
                        !GetPointer<union_type>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(ti_source.first); /// overwrite gst_it->second
                        to_be_visited.insert(ti_source.first);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "UNION TYPE: " + gst_it->first + " " + STR(ti_source.first));
                     }
                  }
               }
               remap[ti_source.first] = gst_it->second;
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Analyzed tree nodes of second tree manager");

   /// remap tree_node from source_tree_manager to this tree_manager
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Remapping declaration nodes");
   for(const auto& ti_source : source_tree_nodes)
   {
      const tree_nodeRef tn = ti_source.second;
      /// check for decl_node
      auto* dn = GetPointer<decl_node>(tn);
      if(dn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "-->Examining declaration node " + STR(ti_source.first) + " of second tree:" + STR(tn));
         if(check_for_decl(tn, source_tree_manager, symbol_name, symbol_scope, ti_source.first,
                           global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Not suitable for symbol table");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        STR(ti_source.first) + "-ndecl>" + symbol_name + "-" + symbol_scope);
         /// check for static
         if((GetPointer<function_decl>(tn) and GetPointer<function_decl>(tn)->static_flag) or
            (GetPointer<var_decl>(tn) and
             (GetPointer<var_decl>(tn)->static_flag or GetPointer<var_decl>(tn)->static_static_flag)))
         {
            /// Management of forward declaration of static function
            if(GetPointer<function_decl>(tn))
            {
               /// Implementation node
               if(GetPointer<function_decl>(tn)->body)
               {
                  /// If this is the implementation already remaps so that also forward declaration can be remapped on
                  /// the same node
                  const unsigned int new_index = new_tree_node_id(ti_source.first);
                  remap[ti_source.first] = new_index;
                  not_yet_remapped.insert(ti_source.first);
                  to_be_visited.insert(ti_source.first);

                  static_implementation_functions[symbol_name] = ti_source.first;

                  /// If we have already encountered forward declaration, remap it on the same node
                  if(static_forward_declaration_functions.find(symbol_name) !=
                     static_forward_declaration_functions.end())
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

            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Static");
            if(source_static_symbol_table.find(symbol_name) != source_static_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Already present in source static symbol table");
               unsigned int node_id_source =
                   source_tree_manager->find_identifier_nodeID(source_static_symbol_table.at(symbol_name));

               /// CHECK: reference to source_tree_manager tree_node in tree_reindex
               dn->name = source_tree_manager->GetTreeNode(node_id_source);
            }
            else if(static_symbol_table.find(symbol_name + "-" + symbol_scope) != static_symbol_table.end() or
                    static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) !=
                        static_function_header_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Already present in destination static symbol table");
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
               dn->name = source_tree_manager->GetTreeNode(node_id_source);
               source_static_symbol_table[symbol_name] = symbol_name + STR(counter);
            }
            else
            {
               if(not GetPointer<function_decl>(tn))
               {
                  static_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
               continue;
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Not Static");
            auto gst_it = global_decl_symbol_table.find(symbol_name + "-" + symbol_scope);
            if(gst_it == global_decl_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Not yet in global decl symbol table");
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
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
               continue;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Already in global decl symbol table");
               if((tn->get_kind() == function_decl_K
                   //                     and
                   //                     (
                   //                        !GetPointer<function_decl>(tree_nodes.at(gst_it->second))
                   //                        ||
                   //                        !GetPointer<function_decl>(tree_nodes.at(gst_it->second))->body
                   //                     )
                   and GetPointer<function_decl>(tn)->body) ||
                  // In the first tree manager there is extern type variable, in the second there is the definition
                  (tn->get_kind() == var_decl_K and
                   (!GetPointer<var_decl>(tree_nodes.at(gst_it->second)) ||
                    GetPointer<var_decl>(tree_nodes.at(gst_it->second))->extern_flag) and
                   !GetPointer<var_decl>(tn)->extern_flag) ||
                  // In the first tree manager there is a function_decl without srcp, in the second there is a
                  // funcion_decl with srcp
                  (tn->get_kind() == function_decl_K and GetPointer<function_decl>(tree_nodes.at(gst_it->second)) and
                   (GetPointer<function_decl>(tree_nodes.at(gst_it->second)))->include_name == "<built-in>" and
                   !(GetPointer<function_decl>(tn))->include_name.empty() and
                   (GetPointer<function_decl>(tn))->include_name != "<built-in>"))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---but... forced overwriting");
                  /// The following statements force ti_source.first to overwrite gst_it->second
                  not_yet_remapped.insert(ti_source.first);
                  to_be_visited.insert(ti_source.first);
                  remap[ti_source.first] = gst_it->second;
                  /// If the symbol is in reverse_remap, it means that is a function_decl without body coming from
                  /// other; it has not to be remapped
                  if(reverse_remap.find(gst_it->second) != reverse_remap.end())
                  {
                     THROW_ASSERT(not_yet_remapped.find(reverse_remap.at(gst_it->second)) != not_yet_remapped.end(),
                                  "Trying to cancel remapping of " + STR(reverse_remap.at(gst_it->second)));
                     not_yet_remapped.erase(reverse_remap.at(gst_it->second));
                  }
               }
               else
               {
                  remap[ti_source.first] = gst_it->second;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
      else
      {
         /// already performed
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Remapped declaration nodes");
   /// tree node visitor
   tree_node_reached TNR(remap, not_yet_remapped, TM_this);
   for(const auto idx : to_be_visited)
   {
      source_tree_manager->GetTreeNode(idx)->visit(&TNR);
   }

   /// compute the vertexes reached from all function_decl of source_tree_manager
   for(auto& [idx, curr_tn] : source_tree_manager->function_decl_nodes)
   {
      auto fd = GetPointer<function_decl>(curr_tn);
      if(remap.find(idx) == remap.end())
      {
         remap[idx] = new_tree_node_id(idx);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "Function decl: old " + STR(idx) + " - new " + STR(remap[idx]));
         fd->visit(&TNR);
         not_yet_remapped.insert(idx);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Already remapped tree_node");
#ifndef NDEBUG
   for(auto& [old_idx, new_idx] : remap)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Original " + STR(old_idx) + " New " + STR(new_idx));
   }
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Starting remapping remaining nodes");
   for(const auto idx : not_yet_remapped)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Original " + STR(idx) + " New " + STR(remap.at(idx)));
      TNIF.create_tree_node(remap.at(idx), source_tree_manager->GetTreeNode(idx));

      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "Type is " + source_tree_manager->GetTreeNode(idx)->get_kind_text());
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "DONE");
   }
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "/after_" +
                                  STR(get_next_available_tree_node_id()) + ".raw";
      std::cerr << raw_file_name << std::endl;
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Fix tree reindex nodes");
   FixTreeReindex();
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended merging of new tree_manager");
}

bool tree_manager::check_for_decl(const tree_nodeRef& tn, const tree_managerRef& TM_this, std::string& symbol_name,
                                  std::string& symbol_scope, unsigned int ASSERT_PARAMETER(node_id),
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
   if(dn->name->get_kind() == identifier_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "---check_for_decl is considering: " + GetPointer<identifier_node>(dn->name)->strg + ":" +
                         dn->include_name);
   }
   /// check for memory_tag, parm_decl, result_decl
   if(GetPointer<memory_tag>(tn) || dn->get_kind() == parm_decl_K || dn->get_kind() == result_decl_K)
   {
      return true;
   }
   /// check for scope
   if(dn->scpe and dn->scpe->get_kind() == function_decl_K)
   {
      return true;
   }
   const function_decl* fd = GetPointer<function_decl>(tn);
   if(!fd and dn->scpe and GetPointer<type_node>(dn->scpe) and
      (!GetPointer<type_node>(dn->scpe)->name and
       global_type_unql_symbol_table.find(dn->scpe->index) == global_type_unql_symbol_table.end()))
   {
      return true;
   }
   if(dn->mngl)
   {
      THROW_ASSERT(dn->mngl->get_kind() == identifier_node_K,
                   "expected an identifier_node: " + STR(dn->mngl->get_kind_text()));
      if(dn->name)
      {
         THROW_ASSERT(dn->name->get_kind() == identifier_node_K,
                      "expected an identifier_node: " + STR(dn->name->get_kind_text()) + " " + STR(node_id));
         if(fd)
         {
            if(fd->builtin_flag)
            {
               symbol_name = GetPointer<identifier_node>(dn->name)->strg;
               if(starts_with(symbol_name, "__builtin_"))
               {
                  symbol_name = symbol_name.substr(strlen("__builtin_"));
               }
               //               if(fd->undefined_flag && symbol_name.find("__builtin_") == std::string::npos)
               //                  symbol_name = "__builtin_" + symbol_name;
            }
            else
            {
               symbol_name = GetPointer<identifier_node>(dn->mngl)->strg;
            }
         }
         else
         {
            symbol_name = GetPointer<identifier_node>(dn->mngl)->strg;
         }
      }
      else
      {
         symbol_name = GetPointer<identifier_node>(dn->mngl)->strg;
      }
   }
   else
   {
      THROW_ASSERT(dn->name->get_kind() == identifier_node_K,
                   "expected an identifier_node: " + STR(dn->name->get_kind_text()) + " " + STR(node_id));
      symbol_name = GetPointer<identifier_node>(dn->name)->strg;
      if(starts_with(symbol_name, "__builtin_"))
      {
         symbol_name = symbol_name.substr(strlen("__builtin_"));
      }
      //      if(fd && fd->undefined_flag && fd->builtin_flag && symbol_name.find("__builtin_") == std::string::npos)
      //            symbol_name = "__builtin_" + symbol_name;
   }
   if(dn->scpe and GetPointer<type_node>(dn->scpe) and GetPointer<type_node>(dn->scpe)->name)
   {
      auto* type = GetPointer<type_node>(dn->scpe);
      THROW_ASSERT(type, "expected a type_node: " + std::string(dn->scpe->get_kind_text()));
      /// declaration with type_node local to a function are not considered
      if(type->scpe and type->scpe->get_kind() == function_decl_K)
      {
         return true;
      }
      const auto quals = type->qual;
      std::string type_name;
      if(type->name->get_kind() == identifier_node_K)
      {
         type_name = GetPointer<identifier_node>(type->name)->strg;
      }
      else
      {
         type_name = tree_helper::get_type_name(TM_this, type->name->index);
      }
      if(type->name and (type->name->get_kind() == type_decl_K))
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
   else if(dn->scpe and GetPointer<type_node>(dn->scpe) and !GetPointer<type_node>(dn->scpe)->name and
           global_type_unql_symbol_table.find(dn->scpe->index) != global_type_unql_symbol_table.end())
   {
      symbol_scope = global_type_unql_symbol_table.find(dn->scpe->index)->second;
   }
   else if(fd)
   {
      if(fd->scpe && fd->static_flag)
      {
         symbol_scope = "#F:" + STR(fd->scpe->index);
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

bool tree_manager::check_for_type(const tree_nodeRef& tn, const tree_managerRef& TM, std::string& symbol_name,
                                  std::string& symbol_scope,
                                  const CustomUnorderedMapUnstable<std::string, unsigned int>& global_type_symbol_table,
                                  unsigned int DEBUG_PARAMETER(node_id))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Checking for type " + STR(node_id));
   auto* type = GetPointer<type_node>(tn);
   symbol_name = symbol_scope = "";
   if(!type || !type->name)
   { /// integer_type and real_type have some duplication
      return true;
   }
   if(type->scpe and ((type->scpe->get_kind() == function_decl_K)))
   {
      return true;
   }
   const auto quals = type->qual;
   std::string type_name;
   THROW_ASSERT(type->name, "wrong");
   if(type->name->get_kind() == identifier_node_K)
   {
      type_name = GetPointer<identifier_node>(type->name)->strg;
   }
   else
   {
      type_name = tree_helper::get_type_name(TM, type->name->index);
   }
   if(type->name and (type->name->get_kind() == type_decl_K || type->unql))
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
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param), "---Created ssa " + STR(next_vers));
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

tree_nodeRef tree_manager::create_unique_const(const std::string& val, const tree_nodeConstRef& type)
{
   const auto key = std::make_pair(val, type->index);
   const auto unique_cst = unique_cst_map.find(key);
   if(unique_cst != unique_cst_map.end())
   {
      return unique_cst->second;
   }

   const auto cst_nid = new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   if(tree_helper::IsRealType(type))
   {
      IR_schema[TOK(TOK_VALR)] = val;
      IR_schema[TOK(TOK_VALX)] = val;
      create_tree_node(cst_nid, real_cst_K, IR_schema);
   }
   else
   {
      IR_schema[TOK(TOK_VALUE)] = val;
      create_tree_node(cst_nid, integer_cst_K, IR_schema);
   }
   const auto cost_node = GetTreeNode(cst_nid);
   unique_cst_map[key] = cost_node;
   return cost_node;
}

tree_nodeRef tree_manager::CreateUniqueIntegerCst(integer_cst_t value, const tree_nodeConstRef& type)
{
   const auto bitsize = tree_helper::Size(type);
   if(tree_helper::IsSignedIntegerType(type) && ((value >> (bitsize - 1)) & 1))
   {
      value |= integer_cst_t(-1) << bitsize;
      THROW_ASSERT(value < 0, "");
   }
   else
   {
      value &= (integer_cst_t(1) << bitsize) - 1;
   }
   return create_unique_const(STR(value), type);
}

tree_nodeRef tree_manager::CreateUniqueRealCst(long double value, const tree_nodeConstRef& type)
{
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

   return create_unique_const(ssX.str(), type);
}

bool tree_manager::is_CPP() const
{
   return Param->isOption(OPT_input_format) &&
          (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
           Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);
}

bool tree_manager::is_top_function(const function_decl* fd) const
{
   if(fd->name)
   {
      tree_nodeRef id_name = fd->name;
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
               const auto top_functions_names = Param->getOption<std::list<std::string>>(OPT_top_functions_names);
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
   tree_nodeRef fd_node = GetTreeNode(fun_id);
   auto* fd = GetPointer<function_decl>(fd_node);
   auto* sl = GetPointer<statement_list>(fd->body);

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
            tree_nodeRef tn = ssa.first;
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
                  if(use.first->index == statement_node->index)
                  {
                     found = true;
                  }
               }
               if(!found)
               {
                  std::cerr << "stmt: " << orig_string << " var: " << sn->ToString() << std::endl;
                  for(auto& stmt : sn->CGetUseStmts())
                  {
                     std::cerr << "stmt referred: " << stmt.first->index << std::endl;
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
