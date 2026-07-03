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
 * @file ir_manager.cpp
 * @brief Class implementation of the manager of the IR structures extracted from the raw file.
 *
 * This file implements some of the ir_manager member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "ir_manager.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "compiler_constants.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"
#include "ir_node_factory.hpp"
#include "ir_node_finder.hpp"
#include "ir_nodes_merger.hpp"
#include "ir_reindex.hpp"
#include "ir_reindex_remove.hpp"
#include "raw_writer.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
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

ir_manager::ir_manager(const ParameterConstRef& _Param)
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

void ir_manager::add_reserve(size_t n)
{
   ir_nodes.reserve(ir_nodes.size() + n);
}

void ir_manager::FixIRReindex()
{
   ir_reindex_remove trr(*this);
   for(auto& tn : ir_nodes)
   {
      if(tn)
      {
         trr(tn);
      }
   }
}

void ir_manager::AddIRNode(const ir_nodeRef& curr)
{
   THROW_ASSERT(curr && curr->get_kind() != ir_reindex_K,
                "Invalid IR node: " + (curr ? curr->get_kind_text() : "nullptr"));
   THROW_ASSERT(curr->index > 0, "Expected a positive index.");
   if(curr->index >= last_node_id)
   {
      last_node_id = curr->index + 1;
      ir_nodes.resize(last_node_id);
   }
   ir_nodes[curr->index] = curr;
   add_to_partition(curr->get_kind(), curr->index, curr);
}

ir_nodeRef ir_manager::GetIRReindex(unsigned int index)
{
   THROW_ASSERT(index > 0, "Expected a positive index (" + STR(index) + ")");
   if(index < last_node_id && ir_nodes.at(index))
   {
      return ir_nodes.at(index);
   }
   if(index >= last_node_id)
   {
      last_node_id = index + 1;
      ir_nodes.resize(last_node_id);
   }
   return ir_nodeRef(new ir_reindex(index, ir_nodes.at(index)));
}

ir_nodeRef ir_manager::GetIRNode(const unsigned int i) const
{
   THROW_ASSERT(i < last_node_id, "Out of range index index (" + STR(i) + ")");
   THROW_ASSERT(ir_nodes.at(i), "IR node with index " + STR(i) + " not found");
   return ir_nodes.at(i);
}

// *****************************************************************************************

unsigned int ir_manager::find_sc_main_node() const
{
   return function_index("sc_main");
}

unsigned int ir_manager::function_index(const std::string& function_name) const
{
   const auto tn = GetFunction(function_name);
   return tn ? tn->index : 0;
}

ir_nodeRef ir_manager::GetFunction(const std::string& function_name) const
{
   null_deleter null_del;
   ir_managerConstRef TM(this, null_del);
   if(partitionMap.find(function_val_node_K) != partitionMap.end())
   {
      for(const auto& [idx, fdecl] : partitionMap.at(function_val_node_K))
      {
         const auto fd = GetPointerS<const function_val_node>(fdecl);
         std::string simple_name, mangled_name;
         if(fd->name->get_kind() == identifier_node_K)
         {
            const auto in = GetPointerS<const identifier_node>(fd->name);
            simple_name = in->strg;
         }
         if(fd->mngl)
         {
            if(fd->mngl->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<identifier_node>(fd->mngl);
               mangled_name = in->strg;
            }
         }
         const auto name = [&](const ir_nodeConstRef& fd0) {
            const auto fname = ir_helper::GetFunctionName(fd0);
            if(TM->is_CPP())
            {
               const auto demangled_name = cxa_demangle(fname);
               if(!demangled_name.empty())
               {
                  return demangled_name.substr(0, demangled_name.find('('));
               }
            }
            return fname;
         }(fdecl);
         if(name == function_name || function_name == std::string("-") ||
            (!simple_name.empty() && function_name == simple_name) ||
            (!mangled_name.empty() && mangled_name == function_name))
         {
            return fdecl;
         }
      }
   }
   return nullptr;
}

unsigned int ir_manager::function_index_mngl(const std::string& function_name) const
{
   null_deleter null_del;
   ir_managerConstRef TM(this, null_del);
   unsigned int function_id = 0;
   if(partitionMap.find(function_val_node_K) != partitionMap.end())
   {
      for(const auto& [f_id, fnode] : partitionMap.at(function_val_node_K))
      {
         const auto fd = GetPointerS<function_val_node>(fnode);
         std::string simple_name, mangled_name;
         if(fd->name->get_kind() == identifier_node_K)
         {
            const auto in = GetPointerS<identifier_node>(fd->name);
            simple_name = in->strg;
         }
         if(fd->mngl && fd->mngl->get_kind() == identifier_node_K)
         {
            const auto in = GetPointerS<identifier_node>(fd->mngl);
            mangled_name = in->strg;
         }
         const auto name = ir_helper::GetFunctionName(fnode);
         if(name == function_name || function_name == std::string("-") ||
            (!simple_name.empty() && function_name == simple_name) ||
            (!mangled_name.empty() && mangled_name == function_name))
         {
            function_id = f_id;
         }
      }
   }
   return function_id;
}

void ir_manager::print(std::ostream& os) const
{
   raw_writer RW(os);
   auto node_count_str = std::to_string(ir_nodes.size());
   node_count_str = std::string(10 - node_count_str.size(), ' ') + node_count_str;

   os << CompilerWrapper::bambu_ir_info << "NODE_COUNT: " << node_count_str << "\n";
   for(unsigned int idx = 0; idx < ir_nodes.size(); ++idx)
   {
      const auto& tn = ir_nodes.at(idx);
      if(tn)
      {
         os << "@" << idx << " ";
         tn->visit(&RW);
         os << std::endl;
      }
   }
}

void ir_manager::PrintBambuLLVM(std::ostream& os) const
{
   if(partitionMap.find(variable_val_node_K) != partitionMap.end())
   {
      for(const auto& [idx, vdecl] : partitionMap.at(variable_val_node_K))
      {
         auto vd = GetPointerS<variable_val_node>(vdecl);
         if(vd->parent->get_kind() == module_unit_node_K)
         {
            os << vd->ToStringDecl() << "\n";
         }
      }
   }
   if(partitionMap.find(function_val_node_K) != partitionMap.end())
   {
      for(const auto& [idx, fdecl] : partitionMap.at(function_val_node_K))
      {
         auto fd = GetPointerS<function_val_node>(fdecl);
         if(GetPointerS<function_val_node>(fdecl)->body)
         {
            os << fd->ToStringDef();
         }
         else
         {
            os << fd->ToStringDecl();
         }
      }
   }
}

ir_nodeRef ir_manager::create_ir_node(const unsigned int node_id, enum kind ir_node_type,
                                      const IRSchema& ir_node_schema)
{
   ir_node_factory TNF(ir_node_schema, *this);
   const auto tn = TNF.create_ir_node(node_id, ir_node_type);
   // INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param),
   //                "---Created IR node " + STR(node_id) + ": " + STR(tn));
   return tn;
}

ir_nodeRef ir_manager::create_ir_node(enum kind ir_node_type, const IRSchema& ir_node_schema)
{
   return create_ir_node(new_ir_node_id(), ir_node_type, ir_node_schema);
}

unsigned int ir_manager::new_ir_node_id()
{
   unsigned int temp = last_node_id;
   ++last_node_id;
   ir_nodes.resize(last_node_id);
   return temp;
}

unsigned int ir_manager::get_next_available_ir_node_id() const
{
   return last_node_id;
}

void ir_manager::add_to_partition(enum kind tag, unsigned int index, ir_nodeRef curr)
{
   partitionMap[tag][index] = curr;
}

unsigned int ir_manager::find0(enum kind ir_node_type, const IRSchema& ir_node_schema)
{
   if(ir_node_type == identifier_node_K)
   {
      std::string id;
      if(ir_node_schema.find(TOK(TOK_STRG)) != ir_node_schema.end())
      {
         id = ir_node_schema.find(TOK(TOK_STRG))->second;
      }
      else
      {
         THROW_ERROR("Incorrect schema for identifier_node: no TOK_STRG");
      }
      return find_identifier_nodeID(id);
   }
   std::string key = ir_node::GetString(ir_node_type);
   const auto tns_end = ir_node_schema.end();
   for(auto tns = ir_node_schema.begin(); tns != tns_end; ++tns)
   {
      key += ";" + STOK2(tns->first) + "=" + tns->second;
   }
   // std::cout << "KEY: " + key + "[" << (find_cache.find(key) != find_cache.end() ? find_cache.at(key) : 0)
   // << "]" << std::endl;
   if(find_cache.find(key) != find_cache.end())
   {
      return find_cache.at(key);
   }
   ir_node_finder TNF(ir_node_schema);
   if(partitionMap.find(ir_node_type) != partitionMap.end())
   {
      for(const auto& [idx, fdecl] : partitionMap.at(ir_node_type))
      {
         if(TNF.check(fdecl))
         {
            find_cache[key] = idx;
            return idx;
         }
      }
   }
   return 0;
}

#ifndef NDEBUG
static unsigned int __replace_ir_node_debug_level = DEBUG_LEVEL_NONE;
#endif

void ir_manager::ReplaceIRNode(const ir_nodeRef& stmt, const ir_nodeRef& old_node, const ir_nodeRef& new_node,
                               bool use_counting)
{
   THROW_ASSERT(GetPointer<const node_stmt>(stmt), "Replacing ssa name starting from " + stmt->ToString());
   THROW_ASSERT(!GetPointer<const node_stmt>(new_node), "new node cannot be a node_stmt");
   THROW_ASSERT(!GetPointer<const node_stmt>(old_node), "old node cannot be a node_stmt: " + STR(old_node));
   /// Temporary variable used to pass first argument of RecursiveReplaceIRNode by reference. Since it is a
   /// node_stmt it has not to be replaced
   ir_nodeRef temp = stmt;
#ifndef NDEBUG
   // __replace_ir_node_debug_level = GET_FUNCTION_DEBUG_LEVEL(Param);
#endif
   RecursiveReplaceIRNode(temp, old_node, new_node, stmt, false, use_counting);
#ifndef NDEBUG
   // __replace_ir_node_debug_level = DEBUG_LEVEL_NONE;
#endif
}

void ir_manager::RecursiveReplaceIRNode(ir_nodeRef& tn, const ir_nodeRef old_node, const ir_nodeRef& new_node,
                                        const ir_nodeRef& stmt, const bool definition, const bool use_counting)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level,
                  "-->Replacing " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                      new_node->ToString() + "(" + new_node->get_kind_text() +
                      ") starting from node: " + tn->ToString() + "(" + tn->get_kind_text() + ")");
   if(tn->index == old_node->index)
   {
      /// Check if we need to update uses or definitions
      const auto gn = GetPointer<const node_stmt>(stmt);
      if(gn)
      {
         if(definition)
         {
            if(gn->vdef && gn->vdef->index == old_node->index)
            {
               const auto vssa = GetPointerS<ssa_node>(new_node);
               vssa->SetDefStmt(stmt);
            }
            if(gn->memdef && gn->memdef->index == old_node->index)
            {
               const auto vssa = GetPointerS<ssa_node>(new_node);
               vssa->SetDefStmt(stmt);
            }
            const auto ga = GetPointer<const assign_stmt>(stmt);
            if(ga && ga->op0->index == old_node->index && old_node->get_kind() == ssa_node_K)
            {
               GetPointerS<ssa_node>(new_node)->SetDefStmt(stmt);
            }
            const auto gp = GetPointer<const phi_stmt>(stmt);
            if(gp && gp->res->index == old_node->index && !GetPointer<cst_node>(new_node))
            {
               THROW_ASSERT(new_node->get_kind() == ssa_node_K, new_node->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level,
                              "---Setting " + STR(stmt) + " as new define statement of " + STR(new_node));
               GetPointerS<ssa_node>(new_node)->SetDefStmt(stmt);
            }
         }
         else if(use_counting)
         {
            THROW_ASSERT(gn->bb_index, "Statement is not in a basic block: " + stmt->ToString());

            const auto used_ssas = ir_helper::ComputeSsaUses(old_node);
            for(const auto& [node, uses] : used_ssas)
            {
               for(auto counter = uses; counter; --counter)
               {
                  GetPointerS<ssa_node>(node)->RemoveUse(stmt);
               }
            }
            tn = new_node;
            const auto new_used_ssas = ir_helper::ComputeSsaUses(new_node);
            for(const auto& [node, uses] : new_used_ssas)
            {
               for(auto counter = uses; counter; --counter)
               {
                  GetPointerS<ssa_node>(node)->AddUseStmt(stmt);
               }
            }
         }
      }
      THROW_ASSERT(!definition || GetPointer<cst_node>(new_node) || new_node->get_kind() == ssa_node_K,
                   "unexpected node");
      if(!definition || new_node->get_kind() == ssa_node_K)
      {
         tn = new_node;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level,
                     "<--Replaced " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                         new_node->ToString() + "(" + new_node->get_kind_text() + ") New statement: " + tn->ToString());
      return;
   }

   const auto replaceVirtuals = [&](node_stmt* gn) {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level, "-->Checking virtuals");
      if(gn->memdef)
      {
         RecursiveReplaceIRNode(gn->memdef, old_node, new_node, stmt, true, use_counting);
      }
      if(gn->memuse)
      {
         RecursiveReplaceIRNode(gn->memuse, old_node, new_node, stmt, false, use_counting);
      }
      if(gn->vdef)
      {
         RecursiveReplaceIRNode(gn->vdef, old_node, new_node, stmt, true, use_counting);
      }
      const auto vuse_it = gn->vuses.find(old_node);
      if(vuse_it != gn->vuses.end())
      {
         gn->vuses.erase(vuse_it);
         const auto old_vssa = GetPointerS<ssa_node>(old_node);
         if(use_counting)
         {
            old_vssa->RemoveUse(stmt);
         }
         if(gn->AddVuse(new_node))
         {
            if(use_counting)
            {
               const auto new_vssa = GetPointerS<ssa_node>(new_node);
               new_vssa->AddUseStmt(stmt);
            }
         }
      }
      const auto vover_it = gn->vovers.find(old_node);
      if(vover_it != gn->vovers.end())
      {
         gn->vovers.erase(vover_it);
         const auto old_vssa = GetPointerS<ssa_node>(old_node);
         if(use_counting)
         {
            old_vssa->RemoveUse(stmt);
         }
         if(gn->AddVover(new_node))
         {
            if(use_counting)
            {
               const auto new_vssa = GetPointerS<ssa_node>(new_node);
               new_vssa->AddUseStmt(stmt);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level, "<--Checked virtuals");
   };

   switch(tn->get_kind())
   {
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(tn);
         replaceVirtuals(gm);
         RecursiveReplaceIRNode(gm->op0, old_node, new_node, stmt, new_node->get_kind() == ssa_node_K, use_counting);
         RecursiveReplaceIRNode(gm->op1, old_node, new_node, stmt, false, use_counting);
         if(gm->predicate)
         {
            RecursiveReplaceIRNode(gm->predicate, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(tn);
         replaceVirtuals(gmwi);
         for(auto& [cond, edge] : gmwi->list_of_cond)
         {
            if(cond)
            {
               RecursiveReplaceIRNode(cond, old_node, new_node, stmt, false, use_counting);
            }
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<phi_stmt>(tn);
         replaceVirtuals(gp);
         for(auto& [def, edge] : gp->list_of_def_edge)
         {
            RecursiveReplaceIRNode(def, old_node, new_node, stmt, false, use_counting);
         }
         RecursiveReplaceIRNode(gp->res, old_node, new_node, stmt, true, use_counting);
         break;
      }
      case call_stmt_K:
      {
         const auto gc = GetPointerS<call_stmt>(tn);
         replaceVirtuals(gc);
         if(gc->predicate)
         {
            RecursiveReplaceIRNode(gc->predicate, old_node, new_node, stmt, false, use_counting);
         }
         for(auto& arg : gc->args)
         {
            RecursiveReplaceIRNode(arg, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case return_stmt_K:
      {
         const auto gr = GetPointerS<return_stmt>(tn);
         replaceVirtuals(gr);
         if(gr->op)
         {
            RecursiveReplaceIRNode(gr->op, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(tn);
         RecursiveReplaceIRNode(ue->op, old_node, new_node, stmt, false, use_counting);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(tn);
         RecursiveReplaceIRNode(be->op0, old_node, new_node, stmt, false, use_counting);
         RecursiveReplaceIRNode(be->op1, old_node, new_node, stmt, false, use_counting);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(tn);
         RecursiveReplaceIRNode(te->op0, old_node, new_node, stmt, false, use_counting);
         RecursiveReplaceIRNode(te->op1, old_node, new_node, stmt, false, use_counting);
         if(te->op2)
         {
            RecursiveReplaceIRNode(te->op2, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(tn);
         RecursiveReplaceIRNode(le->op0, old_node, new_node, stmt, false, use_counting);
         RecursiveReplaceIRNode(le->op1, old_node, new_node, stmt, false, use_counting);
         if(le->op2)
         {
            RecursiveReplaceIRNode(le->op2, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op3)
         {
            RecursiveReplaceIRNode(le->op3, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op4)
         {
            RecursiveReplaceIRNode(le->op4, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op5)
         {
            RecursiveReplaceIRNode(le->op5, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op6)
         {
            RecursiveReplaceIRNode(le->op6, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op7)
         {
            RecursiveReplaceIRNode(le->op7, old_node, new_node, stmt, false, use_counting);
         }
         if(le->op8)
         {
            RecursiveReplaceIRNode(le->op8, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(tn);
         for(auto& arg : ce->args)
         {
            RecursiveReplaceIRNode(arg, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto constr = GetPointerS<constructor_node>(tn);
         for(auto& idx_value : constr->list_of_idx_valu)
         {
            RecursiveReplaceIRNode(idx_value.second, old_node, new_node, stmt, false, use_counting);
         }
         break;
      }
      case field_val_node_K:
      case function_val_node_K:
      case nop_stmt_K:
      case constant_int_val_node_K:
      case argument_val_node_K:
      case constant_fp_val_node_K:
      case ssa_node_K:
      case variable_val_node_K:
      case constant_vector_val_node_K:
      {
         break;
      }
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") + tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, __replace_ir_node_debug_level,
                  "<--Replaced " + old_node->ToString() + " (" + old_node->get_kind_text() + ") with " +
                      new_node->ToString() + "(" + new_node->get_kind_text() + ") New statement: " + tn->ToString());
}

void ir_manager::erase_usage_info(const ir_nodeRef& tn, const ir_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Erase usage into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
   switch(tn->get_kind())
   {
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(tn);
         erase_usage_info(gm->op1, stmt);
         if(gm->predicate)
         {
            erase_usage_info(gm->predicate, stmt);
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(tn);
         erase_usage_info(ue->op, stmt);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(tn);
         erase_usage_info(be->op0, stmt);
         erase_usage_info(be->op1, stmt);
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               erase_usage_info(cond.first, stmt);
            }
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(tn);
         erase_usage_info(te->op0, stmt);
         erase_usage_info(te->op1, stmt);
         if(te->op2)
         {
            erase_usage_info(te->op2, stmt);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(tn);
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
      case variable_val_node_K:
      case argument_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      case phi_stmt_K:
      case function_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<ssa_node>(tn);
         for(const auto& [ssa, use_count] : sn->CGetUseStmts())
         {
            if(ssa->index == stmt->index)
            {
               for(size_t repetition = 0; repetition < use_count; repetition++)
               {
                  sn->RemoveUse(ssa);
               }
            }
         }
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(tn);
         for(const auto& arg : ce->args)
         {
            erase_usage_info(arg, stmt);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<call_stmt>(tn);
         if(ce->predicate)
         {
            erase_usage_info(ce->predicate, stmt);
         }
         for(const auto& arg : ce->args)
         {
            erase_usage_info(arg, stmt);
         }
         break;
      }
      case constructor_node_K:
      case nop_stmt_K:
      case return_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") + tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "<--Erased usage into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
}

void ir_manager::insert_usage_info(const ir_nodeRef& tn, const ir_nodeRef& stmt)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Insert usage info into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
   switch(tn->get_kind())
   {
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(tn);
         insert_usage_info(gm->op1, stmt);
         if(gm->predicate)
         {
            insert_usage_info(gm->predicate, stmt);
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(tn);
         insert_usage_info(ue->op, stmt);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(tn);
         insert_usage_info(be->op0, stmt);
         insert_usage_info(be->op1, stmt);
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               insert_usage_info(cond.first, stmt);
            }
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(tn);
         insert_usage_info(te->op0, stmt);
         insert_usage_info(te->op1, stmt);
         if(te->op2)
         {
            insert_usage_info(te->op2, stmt);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(tn);
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
      case variable_val_node_K:
      case argument_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      case phi_stmt_K:
      case function_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<ssa_node>(tn);
         sn->AddUseStmt(stmt);
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(tn);
         for(const auto& arg : ce->args)
         {
            insert_usage_info(arg, stmt);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<call_stmt>(tn);
         if(ce->predicate)
         {
            insert_usage_info(ce->predicate, stmt);
         }
         for(const auto& arg : ce->args)
         {
            insert_usage_info(arg, stmt);
         }
         break;
      }
      case constructor_node_K:
      case nop_stmt_K:
      case return_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") + tn->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "<--Inserted usage info into node " + STR(tn->index) + " (" + tn->get_kind_text() +
                      "). Statement: " + STR(stmt->index) + " (" + stmt->get_kind_text() + ")");
}

#define IS_DECL_NODE(tn)                                                           \
   (tn->get_kind() == field_val_node_K || tn->get_kind() == function_val_node_K || \
    tn->get_kind() == argument_val_node_K || tn->get_kind() == variable_val_node_K)

#define IS_TYPE_NODE(tn)                                                                                               \
   (tn->get_kind() == integer_ty_node_K || tn->get_kind() == pointer_ty_node_K || tn->get_kind() == array_ty_node_K || \
    tn->get_kind() == function_ty_node_K || tn->get_kind() == real_ty_node_K || tn->get_kind() == struct_ty_node_K ||  \
    tn->get_kind() == vector_ty_node_K)

void ir_manager::merge_ir_managers(const ir_managerRef& source_ir_manager)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Starting merging of new ir_manager");
   /// a declaration is uniquely identified by the name and by the scope
   /// in case the decl_node has a mangle the associated identifier_node is unique
   /// a decl_node without name is not added to the symbol table
   /// a decl_node local to a function_val_node (parent is a function_val_node) is not added to the symbol table
   /// a decl_node local to a type_node without name is not added to the symbol table
   /// a static decl_node is not added to the symbol table
   /// argument_val_node are not added to the symbol table
   /// declaration with type_node local to a function are not considered
   /// the key of the declaration symbol table is structured as "name--scope"
   /// the value of the declaration symbol table is the nodeID of the ir_node in the ir_manager
   CustomUnorderedMapUnstable<std::string, unsigned int> global_decl_symbol_table;

   /// a type_node without name is not added to the symbol table
   /// a type_node local to a function_val_node is not added to the symbol table
   /// the key of the type symbol table is structured as "name"
   /// the value of the type symbol table is the nodeID of the ir_node in the ir_manager
   CustomUnorderedMapUnstable<std::string, unsigned int> global_type_symbol_table;

   /// this table is used to give a name to unqualified record or union
   CustomUnorderedMap<unsigned int, std::string> global_type_unql_symbol_table;
   /// global static variable and function become global so we need some sort of uniquification
   CustomUnorderedSet<std::string> static_symbol_table;
   CustomUnorderedSet<std::string> static_function_header_symbol_table;
   const ir_managerRef TM_this(this, null_deleter{});

   // TODO: this call may be useless since FixIRReindex is called on every ir_manager after parsing is completed,
   // thus no "un-fixed" ir_manager should be able to call merge_ir_managers
   FixIRReindex();

#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "/before_ir_merge_" +
                                  STR(get_next_available_ir_node_id()) + STR_CST_bambu_ir_suffix;
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }
#endif

   /// build the symbol tables of ir_node inheriting from type_node and then from decl_node; decl_nodes have to be
   /// examinated later since they have to be examinated after the record/union types???
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Building the global symbol table of this IR manager");
   std::string symbol_name;
   std::string symbol_scope;
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Checking types");
   for(unsigned int idx = 0; idx < ir_nodes.size(); ++idx)
   {
      const auto& tn = ir_nodes.at(idx);
      if(tn && !IS_DECL_NODE(tn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(idx));
         if(check_for_type(tn, symbol_name, symbol_scope, global_type_symbol_table, idx))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         THROW_ASSERT(global_type_symbol_table.find(symbol_name) == global_type_symbol_table.end(),
                      "duplicated symbol in global_type_symbol_table: " + symbol_name + " " +
                          STR(global_type_symbol_table.at(symbol_name)) + " " + STR(idx));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "---Is INSERTED in the symbol table " + symbol_name + " --> " + STR(idx));
         global_type_symbol_table[symbol_name] = idx;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Checked types");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Checking declarations");
   for(unsigned int idx = 0; idx < ir_nodes.size(); ++idx)
   {
      const auto& tn = ir_nodes.at(idx);
      /// check for decl_node
      if(tn && IS_DECL_NODE(tn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(idx));
         if(check_for_decl(tn, symbol_name, symbol_scope, idx, global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Not skipped");
            /// check for static
            if((tn->get_kind() == function_val_node_K && GetPointerS<function_val_node>(tn)->static_flag) ||
               (tn->get_kind() == variable_val_node_K && GetPointerS<variable_val_node>(tn)->static_flag))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Static declaration");
               THROW_ASSERT(
                   (tn->get_kind() == function_val_node_K && GetPointerS<function_val_node>(tn)->body) ||
                       (tn->get_kind() == variable_val_node_K &&
                        static_symbol_table.find(symbol_name + "-" + symbol_scope) == static_symbol_table.end()) ||
                       (tn->get_kind() == function_val_node_K && !GetPointerS<function_val_node>(tn)->body &&
                        static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) ==
                            static_function_header_symbol_table.end()),
                   "duplicated static symbol in the current ir_manager: " + symbol_name + "-" + symbol_scope + " " +
                       STR(idx));
               if(tn->get_kind() == function_val_node_K && !GetPointerS<function_val_node>(tn)->body)
               {
                  static_function_header_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               else
               {
                  static_symbol_table.insert(symbol_name + "-" + symbol_scope);
               }
               continue;
            }
            /// check for function_val_node undefined
            if(tn->get_kind() == function_val_node_K &&
               global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(GetPointerS<function_val_node>(
                      ir_nodes.at(global_decl_symbol_table.at(symbol_name + "-" + symbol_scope)))
                      ->body)
               {
                  continue;
               }
               if(!GetPointerS<function_val_node>(tn)->body)
               {
                  continue;
               }
               // else do overwrite
            }
            else if(tn->get_kind() == variable_val_node_K &&
                    global_decl_symbol_table.find(symbol_name + "-" + symbol_scope) != global_decl_symbol_table.end())
            {
               if(!GetPointerS<variable_val_node>(
                       ir_nodes.at(global_decl_symbol_table.at(symbol_name + "-" + symbol_scope)))
                       ->extern_flag)
               {
                  continue;
               }
               if(GetPointerS<variable_val_node>(tn)->extern_flag)
               {
                  continue;
               }
               // else do overwrite
            }
            else if(tn->get_kind() != function_val_node_K && tn->get_kind() != variable_val_node_K)
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

#ifndef NDEBUG
   for(const auto& gtust_i : global_type_unql_symbol_table)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gtust_i.first) + "-unqualified_type>" + gtust_i.second);
   }
   for(const auto& gst_i : global_decl_symbol_table)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gst_i.second) + "-decl>" + gst_i.first);
   }
   for(const auto& gst_i : global_type_symbol_table)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(gst_i.second) + "-type>" + gst_i.first);
   }
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Built table for this ir_manager");
   /// source static function declaration
   /// used to correctly rename declaration and definition of a function
   CustomUnorderedMapUnstable<std::string, std::string> source_static_symbol_table;
   /// the key is the old index while the values is the new index
   CustomUnorderedMapUnstable<unsigned int, unsigned int> remap;

   /// At the moment the reverse of remap; it is filled only with function_val_node without body
   CustomUnorderedMapUnstable<unsigned int, unsigned int> reverse_remap;

   /// For each static function in source_ir_manager the index of the IR node (in source_ir_manager) of its
   /// forward declaration
   CustomUnorderedMapUnstable<std::string, unsigned int> static_forward_declaration_functions;

   /// For each static function in source the index of the IR node (in source_ir_manager) of its implementation
   CustomUnorderedMapUnstable<std::string, unsigned int> static_implementation_functions;

   /// set of nodes that will be added to the current IR manager (this)
   OrderedSetStd<unsigned int> not_yet_remapped;
   OrderedSetStd<unsigned int> to_be_visited;

   auto& source_ir_nodes = source_ir_manager->ir_nodes;
   ir_nodes.reserve((ir_nodes.size() + source_ir_manager->ir_nodes.size()) * 9UL / 8UL);

   /// remap ir_node from source_ir_manager to this ir_manager
   /// first remap the types and then decl
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Analyzing " + STR(source_ir_nodes.size()) + " IR nodes of second IR manager");
   for(unsigned int idx = 0; idx < source_ir_nodes.size(); ++idx)
   {
      const auto& tn = source_ir_nodes.at(idx);
      /// check for decl_node
      if(tn && !IS_DECL_NODE(tn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Checking " + STR(idx));
         if(tn->get_kind() == identifier_node_K)
         {
            const auto id = GetPointerS<identifier_node>(tn);
            unsigned int node_id = find_identifier_nodeID(id->strg);
            if(node_id)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Identifier FOUND: remapped!");
               remap[idx] = node_id;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Identifier NOT found: skipped!");
               continue;
            }
         }
         else if(check_for_type(tn, symbol_name, symbol_scope, global_type_symbol_table, idx))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Is NOT inserted in the symbol table");
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(idx) + "-ntype>" + symbol_name + "-" + symbol_scope);
            auto gst_it = global_type_symbol_table.find(symbol_name);
            if(gst_it == global_type_symbol_table.end())
            {
               if(remap.find(idx) == remap.end())
               {
                  unsigned int new_ir_index = new_ir_node_id();
                  global_type_symbol_table[symbol_name] = remap[idx] = new_ir_index;
                  not_yet_remapped.insert(idx);
                  to_be_visited.insert(idx);
               }
               continue;
            }
            else
            {
               // record type
               if(tn->get_kind() == struct_ty_node_K)
               {
                  // present in this ir_manager
                  if(gst_it->second < ir_nodes.size() && ir_nodes.at(gst_it->second))
                  {
                     const auto& curr_tn = ir_nodes.at(gst_it->second);
                     if(curr_tn->get_kind() == struct_ty_node_K &&
                        GetPointerS<struct_ty_node>(curr_tn)->list_of_flds.empty() &&
                        !GetPointerS<struct_ty_node>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(idx); /// overwrite gst_it->second
                        to_be_visited.insert(idx);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "RECORD TYPE: " + gst_it->first + " " + STR(idx));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(gst_it->second < source_ir_nodes.size() && source_ir_nodes.at(gst_it->second),
                                  "There is a symbol which is not present in this nor ir_manager nor the other");
                     if(GetPointerS<struct_ty_node>(source_ir_nodes.at(gst_it->second))->list_of_flds.empty() and
                        !GetPointerS<struct_ty_node>(tn)->list_of_flds.empty())
                     {
                        not_yet_remapped.insert(idx); /// overwrite gst_it->second
                        to_be_visited.insert(idx);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "RECORD TYPE: " + gst_it->first + " " + STR(idx));
                     }
                  }
               }
               remap[idx] = gst_it->second;
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Analyzed IR nodes of second IR manager");

   /// remap ir_node from source_ir_manager to this ir_manager
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Remapping declaration nodes");
   for(unsigned int idx = 0; idx < source_ir_nodes.size(); ++idx)
   {
      const auto tn = source_ir_nodes.at(idx);
      /// check for decl_node
      if(tn && IS_DECL_NODE(tn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "-->Examining declaration node " + STR(idx) + " of second IR manager: " +
                            (GetPointerS<const decl_node>(tn)->mngl ?
                                 STR(GetPointerS<const decl_node>(tn)->mngl) :
                                 (GetPointerS<const decl_node>(tn)->name ? STR(GetPointerS<const decl_node>(tn)->name) :
                                                                           "internal")));
         if(check_for_decl(tn, symbol_name, symbol_scope, idx, global_type_unql_symbol_table))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Not suitable for symbol table");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, STR(idx) + "-ndecl>" + symbol_name + "-" + symbol_scope);
         /// check for static
         if((tn->get_kind() == function_val_node_K && GetPointerS<function_val_node>(tn)->static_flag) ||
            (tn->get_kind() == variable_val_node_K && GetPointerS<variable_val_node>(tn)->static_flag))
         {
            /// Management of forward declaration of static function
            if(tn->get_kind() == function_val_node_K)
            {
               /// Implementation node
               if(GetPointerS<function_val_node>(tn)->body)
               {
                  /// If this is the implementation already remaps so that also forward declaration can be remapped on
                  /// the same node
                  const unsigned int new_index = new_ir_node_id();
                  remap[idx] = new_index;
                  not_yet_remapped.insert(idx);
                  to_be_visited.insert(idx);

                  static_implementation_functions[symbol_name] = idx;

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
                  static_forward_declaration_functions[symbol_name] = idx;
                  /// Check if we have already encountered function implementation, remap this on the same node
                  if(static_implementation_functions.find(symbol_name) != static_implementation_functions.end())
                  {
                     remap[idx] = remap[static_implementation_functions[symbol_name]];
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Static");
            if(source_static_symbol_table.find(symbol_name) != source_static_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Already present in source static symbol table");
               unsigned int node_id_source =
                   source_ir_manager->find_identifier_nodeID(source_static_symbol_table.at(symbol_name));

               GetPointerS<decl_node>(tn)->name = source_ir_manager->GetIRNode(node_id_source);
            }
            else if(static_symbol_table.find(symbol_name + "-" + symbol_scope) != static_symbol_table.end() or
                    static_function_header_symbol_table.find(symbol_name + "-" + symbol_scope) !=
                        static_function_header_symbol_table.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Already present in destination static symbol table");
               /// static function or variable_val_node with a conflicting name
               /// so we fix the name...
               /// and we fix in the source ir_manager
               std::string tgt_symbol;
               unsigned int counter = 0;
               unsigned int node_id_this, node_id_source;
               do
               {
                  tgt_symbol = symbol_name + STR(counter++);
                  node_id_this = find_identifier_nodeID(tgt_symbol);
                  node_id_source = source_ir_manager->find_identifier_nodeID(tgt_symbol);
               } while((node_id_this > 0 || node_id_source > 0));
               IRSchema ir_node_schema;
               ir_node_schema[TOK(TOK_STRG)] = tgt_symbol;
               node_id_source = source_ir_manager->new_ir_node_id();
               source_ir_manager->create_ir_node(node_id_source, identifier_node_K, ir_node_schema);
               GetPointerS<decl_node>(tn)->name = source_ir_manager->GetIRNode(node_id_source);
               source_static_symbol_table[symbol_name] = tgt_symbol;
            }
            else
            {
               if(tn->get_kind() != function_val_node_K)
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
               new_index = new_ir_node_id();
               global_decl_symbol_table[symbol_name + "-" + symbol_scope] = remap[idx] = new_index;
               if(tn->get_kind() == function_val_node_K && !GetPointerS<const function_val_node>(tn)->body)
               {
                  reverse_remap[new_index] = idx;
               }
               if(tn->get_kind() == variable_val_node_K && GetPointerS<const variable_val_node>(tn)->extern_flag)
               {
                  reverse_remap[new_index] = idx;
               }
               not_yet_remapped.insert(idx);
               to_be_visited.insert(idx);
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
               continue;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Already in global decl symbol table");
               if((tn->get_kind() == function_val_node_K && GetPointerS<function_val_node>(tn)->body) ||
                  // In the first IR manager there is extern type variable, in the second there is the definition
                  (tn->get_kind() == variable_val_node_K &&
                   (!GetPointerS<variable_val_node>(ir_nodes.at(gst_it->second)) ||
                    GetPointerS<variable_val_node>(ir_nodes.at(gst_it->second))->extern_flag) and
                   !GetPointerS<variable_val_node>(tn)->extern_flag) ||
                  // In the first IR manager there is a function_val_node without loc_info, in the second there is a
                  // funcion_decl with loc_info
                  (tn->get_kind() == function_val_node_K &&
                   GetPointerS<function_val_node>(ir_nodes.at(gst_it->second)) &&
                   GetPointerS<function_val_node>(ir_nodes.at(gst_it->second))->include_name == "<built-in>" &&
                   !GetPointerS<function_val_node>(tn)->include_name.empty() &&
                   GetPointerS<function_val_node>(tn)->include_name != "<built-in>"))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---but... forced overwriting");
                  /// The following statements force idx to overwrite gst_it->second
                  not_yet_remapped.insert(idx);
                  to_be_visited.insert(idx);
                  remap[idx] = gst_it->second;
                  /// If the symbol is in reverse_remap, it means that is a function_val_node without body coming from
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
                  remap[idx] = gst_it->second;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Remapped declaration nodes");
   /// IR node visitor
   ir_node_reached TNR(remap, not_yet_remapped, TM_this);
   for(const auto idx : to_be_visited)
   {
      source_ir_manager->GetIRNode(idx)->visit(&TNR);
   }

   /// compute the vertexes reached from all function_val_node of source_ir_manager
   if(source_ir_manager->partitionMap.find(function_val_node_K) != source_ir_manager->partitionMap.end())
   {
      for(auto& [idx, fnode] : source_ir_manager->partitionMap.at(function_val_node_K))
      {
         if(remap.find(idx) == remap.end())
         {
            remap[idx] = new_ir_node_id();
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "Function decl: old " + STR(idx) + " - new " + STR(remap[idx]));
            GetPointerS<function_val_node>(fnode)->visit(&TNR);
            not_yet_remapped.insert(idx);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Already remapped ir_node");
#ifndef NDEBUG
   for(auto& [old_idx, new_idx] : remap)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Original " + STR(old_idx) + " New " + STR(new_idx));
   }
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Starting remapping remaining nodes");
   ir_node_index_factory TNIF(remap, TM_this);
   for(const auto idx : not_yet_remapped)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Original " + STR(idx) + " New " + STR(remap.at(idx)));
      TNIF.create_ir_node(remap.at(idx), source_ir_manager->GetIRNode(idx));

      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "Type is " + source_ir_manager->GetIRNode(idx)->get_kind_text());
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "DONE");
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      std::string raw_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "/after_" +
                                  STR(get_next_available_ir_node_id()) + STR_CST_bambu_ir_suffix;
      std::cerr << raw_file_name << std::endl;
      std::ofstream raw_file(raw_file_name.c_str());
      raw_file << TM_this;
      raw_file.close();
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Fix IR reindex nodes");
   FixIRReindex();
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended merging of new ir_manager");
}

static std::string get_type_name(const ir_nodeRef& _tn)
{
   const auto type = ir_helper::CGetType(_tn);
   THROW_ASSERT(type, "Node type not type_node");
   return "Internal_" + STR(type->index);
}

bool ir_manager::check_for_decl(const ir_nodeRef& tn, std::string& symbol_name, std::string& symbol_scope,
                                unsigned int ASSERT_PARAMETER(node_id),
                                const CustomUnorderedMap<unsigned int, std::string>& global_type_unql_symbol_table)
{
   THROW_ASSERT(IS_DECL_NODE(tn), "Node should be a declaration node");
   const auto dn = GetPointerS<const decl_node>(tn);
   symbol_name = symbol_scope = "";
   /// check for name
   if(!dn->name)
   {
      return true;
   }
   if(dn->name->get_kind() == identifier_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "---check_for_decl is considering: " + GetPointerS<const identifier_node>(dn->name)->strg + ":" +
                         dn->include_name);
   }
   /// check for argument_val_node
   if(dn->get_kind() == argument_val_node_K)
   {
      return true;
   }
   /// check for scope
   if(dn->parent && dn->parent->get_kind() == function_val_node_K)
   {
      return true;
   }
   if(tn->get_kind() != function_val_node_K && dn->parent && dn->parent->get_kind() == struct_ty_node_K &&
      (!GetPointerS<const struct_ty_node>(dn->parent)->name &&
       global_type_unql_symbol_table.find(dn->parent->index) == global_type_unql_symbol_table.end()))
   {
      return true;
   }
   THROW_ASSERT(dn->name->get_kind() == identifier_node_K,
                "expected an identifier_node: " + STR(dn->name->get_kind_text()) + " " + STR(node_id) + " " +
                    STR(dn->name->index));
   if(dn->mngl)
   {
      THROW_ASSERT(dn->mngl->get_kind() == identifier_node_K,
                   "expected an identifier_node: " + STR(dn->mngl->get_kind_text()));
      if(tn->get_kind() == function_val_node_K && GetPointerS<const function_val_node>(tn)->builtin_flag)
      {
         symbol_name = GetPointerS<const identifier_node>(dn->name)->strg;
         if(starts_with(symbol_name, "__builtin_"))
         {
            symbol_name = symbol_name.substr(sizeof("__builtin_") - 1U);
         }
      }
      else
      {
         symbol_name = GetPointerS<const identifier_node>(dn->mngl)->strg;
      }
   }
   else
   {
      symbol_name = GetPointerS<const identifier_node>(dn->name)->strg;
      if(starts_with(symbol_name, "__builtin_"))
      {
         symbol_name = symbol_name.substr(strlen("__builtin_"));
      }
   }
   if(dn->parent && dn->parent->get_kind() == struct_ty_node_K && GetPointerS<const struct_ty_node>(dn->parent)->name)
   {
      const auto* type = GetPointerS<const struct_ty_node>(dn->parent);
      THROW_ASSERT(type, "expected a type_node: " + dn->parent->get_kind_text());
      std::string type_name;
      if(type->name->get_kind() == identifier_node_K)
      {
         type_name = GetPointerS<identifier_node>(type->name)->strg;
      }
      else
      {
         type_name = get_type_name(type->name);
      }
      if(type->name)
      {
         symbol_scope = "struct " + type_name;
      }
      if(tn->get_kind() == function_val_node_K)
      {
         symbol_scope = symbol_scope + "#F";
      }
   }
   else if(dn->parent && dn->parent->get_kind() == struct_ty_node_K &&
           !GetPointerS<const struct_ty_node>(dn->parent)->name &&
           global_type_unql_symbol_table.find(dn->parent->index) != global_type_unql_symbol_table.end())
   {
      symbol_scope = global_type_unql_symbol_table.find(dn->parent->index)->second;
   }
   else if(tn->get_kind() == variable_val_node_K)
   {
      const auto* vd = GetPointerS<const variable_val_node>(tn);
      if(vd->parent && vd->parent->get_kind() != module_unit_node_K && vd->static_flag)
      {
         symbol_scope = "#V:" + STR(vd->parent->index);
      }
      else
      {
         symbol_scope = "#V";
      }
   }
   else if(tn->get_kind() == function_val_node_K)
   {
      const auto* fd = GetPointerS<const function_val_node>(tn);
      if(fd->parent && fd->parent->get_kind() != module_unit_node_K && fd->static_flag)
      {
         symbol_scope = "#F:" + STR(fd->parent->index);
      }
      else
      {
         symbol_scope = "#F";
      }
   }
   return false;
}

bool ir_manager::check_for_type(const ir_nodeRef& tn, std::string& symbol_name, std::string& symbol_scope,
                                const CustomUnorderedMapUnstable<std::string, unsigned int>& global_type_symbol_table,
                                unsigned int DEBUG_PARAMETER(node_id))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Checking for type " + STR(node_id));
   symbol_name = symbol_scope = "";
   if(tn->get_kind() != struct_ty_node_K)
   {
      return true;
   }
   const auto type = GetPointerS<struct_ty_node>(tn);
   if(!type->name)
   { /// integer_ty_node and real_ty_node have some duplication
      return true;
   }
   std::string type_name;
   if(type->name->get_kind() == identifier_node_K)
   {
      type_name = GetPointerS<identifier_node>(type->name)->strg;
   }
   else
   {
      type_name = get_type_name(type->name);
   }
   if(tn->get_kind() == struct_ty_node_K)
   {
      symbol_name = "struct " + type_name;
   }
   else
   {
      symbol_name = type_name;
   }

   return global_type_symbol_table.find(symbol_name) != global_type_symbol_table.end();
}

unsigned int ir_manager::find_identifier_nodeID(const std::string& str) const
{
   auto it = identifiers_unique_table.find(str);
   if(it != identifiers_unique_table.end())
   {
      return it->second;
   }
   return 0;
}

const CustomOrderedSet<unsigned int> ir_manager::GetAllFunctions() const
{
   CustomOrderedSet<unsigned int> functions;
   if(partitionMap.find(function_val_node_K) != partitionMap.end())
   {
      std::transform(partitionMap.at(function_val_node_K).begin(), partitionMap.at(function_val_node_K).end(),
                     std::inserter(functions, functions.end()),
                     [](const std::pair<const unsigned int, ir_nodeRef>& i) { return i.first; });
   }
   return functions;
}

unsigned int ir_manager::get_next_vers()
{
   if(next_vers == 0)
   {
      for(const auto& tn : ir_nodes)
      {
         if(tn && tn->get_kind() == ssa_node_K)
         {
            const auto sn = GetPointerS<ssa_node>(tn);
            if(sn->vers > next_vers)
            {
               next_vers = sn->vers;
            }
         }
      }
   }
   // INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, GET_FUNCTION_DEBUG_LEVEL(Param), "---Created ssa " + STR(next_vers));
   return ++next_vers;
}

void ir_manager::add_goto()
{
   added_goto++;
}

unsigned int ir_manager::get_added_goto() const
{
   return added_goto;
}

void ir_manager::increment_removed_pointer_plus()
{
   removed_pointer_plus++;
}

unsigned int ir_manager::get_removed_pointer_plus() const
{
   return removed_pointer_plus;
}

void ir_manager::increment_removable_pointer_plus()
{
   removable_pointer_plus++;
}

unsigned int ir_manager::get_removable_pointer_plus() const
{
   return removable_pointer_plus;
}

void ir_manager::increment_unremoved_pointer_plus()
{
   unremoved_pointer_plus++;
}

unsigned int ir_manager::get_unremoved_pointer_plus() const
{
   return unremoved_pointer_plus;
}

ir_nodeRef ir_manager::create_unique_const(const std::string& val, const ir_nodeConstRef& type)
{
   const auto key = std::make_pair(val, type->index);
   const auto unique_cst = unique_cst_map.find(key);
   if(unique_cst != unique_cst_map.end())
   {
      return unique_cst->second;
   }

   ir_nodeRef cst_node;
   IRSchema IR_schema;
   IR_schema[TOK(TOK_TYPE)] = STR(type->index);
   if(ir_helper::IsRealType(type))
   {
      IR_schema[TOK(TOK_VALR)] = val;
      IR_schema[TOK(TOK_VALX)] = val;
      cst_node = create_ir_node(constant_fp_val_node_K, IR_schema);
   }
   else
   {
      IR_schema[TOK(TOK_VALUE)] = val;
      cst_node = create_ir_node(constant_int_val_node_K, IR_schema);
   }
   unique_cst_map[key] = cst_node;
   return cst_node;
}

ir_nodeRef ir_manager::CreateUniqueIntegerCst(integer_cst_t value, const ir_nodeConstRef& type)
{
   const auto bitsize = ir_helper::Size(type);
   if(ir_helper::IsSignedIntegerType(type) && ((value >> (bitsize - 1)) & 1))
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

ir_nodeRef ir_manager::CreateUniqueRealCst(long double value, const ir_nodeConstRef& type)
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

bool ir_manager::is_CPP() const
{
   return Param->isOption(OPT_input_format) &&
          (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
           Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);
}

bool ir_manager::is_top_function(const function_val_node* fd) const
{
   if(fd->name && fd->name->get_kind() == identifier_node_K)
   {
      const auto in = GetPointerS<identifier_node>(fd->name);
      if(Param->isOption(OPT_top_functions_names))
      {
         const auto top_functions_names = Param->getOption<std::list<std::string>>(OPT_top_functions_names);
         for(const auto& top_function_name : top_functions_names)
         {
            if(in->strg == top_function_name)
            {
               return true;
            }
         }
      }
      if(Param->isOption(OPT_top_design_name))
      {
         const auto top_rtldesign_function = Param->getOption<std::string>(OPT_top_design_name);
         if(in->strg == top_rtldesign_function)
         {
            return true;
         }
      }
   }
   return false;
}
