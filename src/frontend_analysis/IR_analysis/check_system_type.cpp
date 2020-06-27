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
 * @file check_system_type.cpp
 * @brief analyse srcp of variables and types to detect system ones; the identified one are flagged
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_LEON3.hpp"
#include "config_LIBBAMBU_SRCDIR.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"

/// Header include
#include "check_system_type.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

/// Wrapper include
#include "gcc_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define FILENAME_NORM(name) ((boost::filesystem::path(name)).normalize().string())

std::vector<std::string> CheckSystemType::systemIncPath;

CustomUnorderedMapUnstable<std::string, std::string> CheckSystemType::inclNameToPath;

CustomUnorderedMapUnstable<std::string, std::string> CheckSystemType::rename_function;

CustomUnorderedMapUnstable<std::string, std::string> CheckSystemType::rename_types;

CustomUnorderedSet<std::string> CheckSystemType::library_system_functions;

CustomUnorderedSet<std::string> CheckSystemType::library_system_includes;

#if HAVE_LEON3
CustomUnorderedSet<std::string> CheckSystemType::not_supported_leon3_functions;
#endif

CustomUnorderedMapUnstable<std::string, std::string> CheckSystemType::undefined_library_function_include;

CheckSystemType::CheckSystemType(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CHECK_SYSTEM_TYPE, _design_flow_manager, _parameters), behavioral_helper(AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()), TM(AppM->get_tree_manager()), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   if(inclNameToPath.size() == 0)
      build_include_structures();
}

CheckSystemType::~CheckSystemType() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> CheckSystemType::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_BAMBU_BUILT
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
#endif
         break;
      }
      case(DEPENDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status CheckSystemType::InternalExec()
{
   CustomUnorderedSet<unsigned int> already_visited;

   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   recursive_examinate(curr_tn, function_id, already_visited);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   for(const auto f : AppM->get_functions_without_body())
   {
      recursive_examinate(TM->get_tree_node_const(f), f, already_visited);
   }

   std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it, it_end;
   it_end = blocks.end();
   for(it = blocks.begin(); it != it_end; ++it)
   {
      if(it->second)
      {
         for(const auto& stmt : it->second->CGetStmtList())
         {
            recursive_examinate(stmt, already_visited);
         }
      }
   }

   already_executed = true;
   return DesignFlowStep_Status::SUCCESS;
}

void CheckSystemType::recursive_examinate(const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_visited)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Not Passed a tree_reindex");
   recursive_examinate(GET_NODE(tn), GET_INDEX_NODE(tn), already_visited);
}

void CheckSystemType::recursive_examinate(const tree_nodeRef& curr_tn, const unsigned int index, CustomUnorderedSet<unsigned int>& already_visited)
{
   THROW_ASSERT(curr_tn, "Empty current tree node");
   if(already_visited.find(index) != already_visited.end())
   {
      return;
   }
   already_visited.insert(index);
   THROW_ASSERT(curr_tn->get_kind() != tree_reindex_K, "Passed tree_reindex instead of real node");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking @" + STR(index));
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr* ce = GetPointer<call_expr>(curr_tn);
         recursive_examinate(ce->fn, already_visited);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, already_visited);
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* ce = GetPointer<gimple_call>(curr_tn);
         recursive_examinate(ce->fn, already_visited);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, already_visited);
         }
         break;
      }
      case gimple_assign_K:
      {
         const gimple_assign* gm = GetPointer<gimple_assign>(curr_tn);
         recursive_examinate(gm->op0, already_visited);
         recursive_examinate(gm->op1, already_visited);

         if(gm->predicate)
         {
            recursive_examinate(gm->predicate, already_visited);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case CASE_DECL_NODES:
      {
         auto* dn = GetPointer<decl_node>(curr_tn);
         auto* fd = GetPointer<function_decl>(curr_tn);
         bool is_system;
         std::string include = std::get<0>(behavioral_helper->get_definition(index, is_system));
         if(fd and (library_system_functions.find(tree_helper::print_function_name(TM, fd)) != library_system_functions.end()))
         {
            dn->library_system_flag = true;
         }
         else if(!dn->operating_system_flag and !dn->library_system_flag and (is_system || is_system_include(include)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System declaration");
            auto* sr = GetPointer<srcp>(curr_tn);
            if(!is_system && sr && sr->include_name != "")
            {
               std::string new_include;
               getRealInclName(include, new_include);
               sr->include_name = new_include;
            }
            fd = GetPointer<function_decl>(curr_tn);
            if(fd)
            {
               if(sr)
               {
                  std::string include_name = sr->include_name;
                  if(library_system_includes.find(include_name) != library_system_includes.end())
                     dn->library_system_flag = true;
                  else
                     dn->operating_system_flag = true;
               }
               else
                  dn->operating_system_flag = true;
            }
            else
               dn->operating_system_flag = true;
         }
         else if(fd and undefined_library_function_include.count(tree_helper::print_function_name(TM, fd)))
         {
            dn->library_system_flag = true;
            auto* sr = GetPointer<srcp>(curr_tn);
            sr->include_name = undefined_library_function_include[tree_helper::print_function_name(TM, fd)];
         }
         // Checking for implicit declaration
         if(curr_tn->get_kind() == function_decl_K)
         {
            fd = GetPointer<function_decl>(curr_tn);
            auto* sr = GetPointer<srcp>(curr_tn);
            if(fd->type and fd->undefined_flag and !fd->operating_system_flag and !fd->library_system_flag and sr->include_name != "<built-in>")
            {
               auto* ft = GetPointer<function_type>(GET_NODE(fd->type));
               if(!ft->prms and ft->retn and GetPointer<integer_type>(GET_NODE(ft->retn)))
               {
                  auto* it = GetPointer<integer_type>(GET_NODE(ft->retn));
                  if(it->name and GetPointer<type_decl>(GET_NODE(it->name)))
                  {
                     auto* td = GetPointer<type_decl>(GET_NODE(it->name));
                     if(td->name and GetPointer<identifier_node>(GET_NODE(td->name)))
                     {
                        auto* in = GetPointer<identifier_node>(GET_NODE(td->name));
                        if(in->strg == "int")
                           fd->include_name = "<built-in>";
                     }
                  }
               }
            }
            if(fd->name and GET_NODE(fd->name)->get_kind() == identifier_node_K)
            {
               auto* in = GetPointer<identifier_node>(GET_NODE(fd->name));
#if HAVE_LEON3
               if(not_supported_leon3_functions.find(in->strg) != not_supported_leon3_functions.end())
               {
                  if(parameters->getOption<bool>(OPT_without_operating_system))
                  {
                     THROW_ERROR("Leon3 without operating system does not support function " + in->strg);
                  }
               }
#endif
               if(rename_function.find(in->strg) != rename_function.end())
                  in->strg = rename_function.find(in->strg)->second;
            }
         }
         // Checking for type
         if(curr_tn->get_kind() == type_decl_K)
         {
            auto* td = GetPointer<type_decl>(curr_tn);
            if(td->name and GET_NODE(td->name)->get_kind() == identifier_node_K)
            {
               auto* in = GetPointer<identifier_node>(GET_NODE(td->name));
               if(rename_types.find(in->strg) != rename_types.end())
                  in->strg = rename_types.find(in->strg)->second;
            }
         }
         if(curr_tn->get_kind() == var_decl_K and dn->artificial_flag and (behavioral_helper->PrintVariable(dn->index) == "__FUNCTION__" or behavioral_helper->PrintVariable(dn->index) == "__PRETTY_FUNCTION__"))
         {
            dn->library_system_flag = true;
         }
#if HAVE_BAMBU_BUILT
         if(include.find("etc/libbambu") != std::string::npos)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---In libbambu");
            dn->libbambu_flag = true;
         }
         else if(dn->libbambu_flag)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---In libbambu");
         }
#endif

         recursive_examinate(dn->type, already_visited);
         break;
      }
      case ssa_name_K:
      {
         const ssa_name* sn = GetPointer<ssa_name>(curr_tn);
         if(sn->type)
            recursive_examinate(sn->type, already_visited);
         else
            recursive_examinate(sn->var, already_visited);
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = curr_tn;
         while(current)
         {
            if(GetPointer<tree_list>(current)->purp)
               recursive_examinate(GetPointer<tree_list>(current)->purp, already_visited);
            recursive_examinate(GetPointer<tree_list>(current)->valu, already_visited);
            current = GetPointer<tree_list>(current)->chan;
            if(current)
               current = GET_NODE(current);
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const unary_expr* ue = GetPointer<unary_expr>(curr_tn);
         if(ue->type)
            recursive_examinate(ue->type, already_visited);
         recursive_examinate(ue->op, already_visited);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const binary_expr* be = GetPointer<binary_expr>(curr_tn);
         if(be->type)
            recursive_examinate(be->type, already_visited);
         recursive_examinate(be->op0, already_visited);
         recursive_examinate(be->op1, already_visited);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         if(te->type)
            recursive_examinate(te->type, already_visited);
         recursive_examinate(te->op0, already_visited);
         if(te->op1)
            recursive_examinate(te->op1, already_visited);
         if(te->op2)
            recursive_examinate(te->op2, already_visited);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         if(qe->type)
            recursive_examinate(qe->type, already_visited);
         recursive_examinate(qe->op0, already_visited);
         if(qe->op1)
            recursive_examinate(qe->op1, already_visited);
         if(qe->op2)
            recursive_examinate(qe->op2, already_visited);
         if(qe->op3)
            recursive_examinate(qe->op3, already_visited);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         recursive_examinate(le->op0, already_visited);
         recursive_examinate(le->op1, already_visited);
         if(le->op2)
            recursive_examinate(le->op2, already_visited);
         if(le->op3)
            recursive_examinate(le->op3, already_visited);
         if(le->op4)
            recursive_examinate(le->op4, already_visited);
         if(le->op5)
            recursive_examinate(le->op5, already_visited);
         if(le->op6)
            recursive_examinate(le->op6, already_visited);
         if(le->op7)
            recursive_examinate(le->op7, already_visited);
         if(le->op8)
            recursive_examinate(le->op8, already_visited);
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         if(co->type)
            recursive_examinate(co->type, already_visited);
         else
         {
            const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
            std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator it, it_end = list_of_idx_valu.end();
            for(it = list_of_idx_valu.begin(); it != it_end; ++it)
            {
               recursive_examinate(it->second, already_visited);
            }
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond* gc = GetPointer<gimple_cond>(curr_tn);
         recursive_examinate(gc->op0, already_visited);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch* se = GetPointer<gimple_switch>(curr_tn);
         recursive_examinate(se->op0, already_visited);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               recursive_examinate(cond.first, already_visited);
         break;
      }
      case gimple_return_K:
      {
         const gimple_return* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            recursive_examinate(re->op, already_visited);
         break;
      }
      case gimple_for_K:
      {
         const gimple_for* fe = GetPointer<gimple_for>(curr_tn);
         recursive_examinate(fe->op0, already_visited);
         recursive_examinate(fe->op1, already_visited);
         recursive_examinate(fe->op2, already_visited);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* we = GetPointer<gimple_while>(curr_tn);
         recursive_examinate(we->op0, already_visited);
         break;
      }
      case gimple_goto_K:
      {
         const gimple_goto* ge = GetPointer<gimple_goto>(curr_tn);
         recursive_examinate(ge->op, already_visited);
         break;
      }
      case CASE_TYPE_NODES:
      {
         auto* ty = GetPointer<type_node>(curr_tn);
         THROW_ASSERT(ty, "expected a name");
         if(ty->name)
            recursive_examinate(ty->name, already_visited);
         switch(curr_tn->get_kind())
         {
            case boolean_type_K:
            {
               //_Bool is C99: we replace it for MAGIC compatibility
               auto* bt = GetPointer<boolean_type>(curr_tn);
               if(bt->name and GetPointer<type_decl>(GET_NODE(bt->name)))
               {
                  auto* td = GetPointer<type_decl>(GET_NODE(bt->name));
                  if(td->name and GetPointer<identifier_node>(GET_NODE(td->name)))
                  {
                     auto* in = GetPointer<identifier_node>(GET_NODE(td->name));
                     if(parameters->isOption(OPT_gcc_standard) and parameters->getOption<std::string>(OPT_gcc_standard) == "c99" and in->strg == "_Bool")
                     {
                        const std::string INT = "int";
                        in->strg = INT;
                     }
                  }
               }
               break;
            }
            case vector_type_K:
            {
               const vector_type* vt = GetPointer<vector_type>(curr_tn);
               recursive_examinate(vt->elts, already_visited);
               break;
            }
            case array_type_K:
            {
               const array_type* at = GetPointer<array_type>(curr_tn);
               recursive_examinate(at->elts, already_visited);
               break;
            }
            case record_type_K:
            {
               auto* rt = GetPointer<record_type>(curr_tn);
#if HAVE_BAMBU_BUILT
               const std::vector<tree_nodeRef>& list_of_flds = rt->list_of_flds;
               std::vector<tree_nodeRef>::const_iterator it, it_end = list_of_flds.end();
               for(it = list_of_flds.begin(); it != it_end; ++it)
               {
                  recursive_examinate(*it, already_visited);
                  if(not rt->libbambu_flag and tree_helper::IsInLibbambu(TM, (*it)->index))
                  {
                     rt->libbambu_flag = true;
                  }
               }
#endif
               const std::vector<tree_nodeRef>& list_of_fncs = rt->list_of_fncs;
               std::vector<tree_nodeRef>::const_iterator it_f, it_f_end = list_of_fncs.end();
               for(it_f = list_of_fncs.begin(); it_f != it_f_end; ++it_f)
                  recursive_examinate(*it_f, already_visited);
               break;
            }
            case union_type_K:
            {
               auto* ut = GetPointer<union_type>(curr_tn);
#if HAVE_BAMBU_BUILT
               const std::vector<tree_nodeRef>& list_of_flds = ut->list_of_flds;
               std::vector<tree_nodeRef>::const_iterator it, it_end = list_of_flds.end();
               for(it = list_of_flds.begin(); it != it_end; ++it)
               {
                  recursive_examinate(*it, already_visited);
                  if(not ut->libbambu_flag and tree_helper::IsInLibbambu(TM, (*it)->index))
                  {
                     ut->libbambu_flag = true;
                  }
               }
#endif
               const std::vector<tree_nodeRef>& list_of_fncs = ut->list_of_fncs;
               std::vector<tree_nodeRef>::const_iterator it_f, it_f_end = list_of_fncs.end();
               for(it_f = list_of_fncs.begin(); it_f != it_f_end; ++it_f)
                  recursive_examinate(*it_f, already_visited);
               break;
            }
            case pointer_type_K:
            {
               const pointer_type* pt = GetPointer<pointer_type>(curr_tn);
               recursive_examinate(pt->ptd, already_visited);
               break;
            }
            case reference_type_K:
            {
               const reference_type* rt = GetPointer<reference_type>(curr_tn);
               if(rt->refd)
                  recursive_examinate(rt->refd, already_visited);
               break;
            }
            case function_type_K:
            {
               const function_type* ft = GetPointer<function_type>(curr_tn);
               if(ft->retn)
                  recursive_examinate(ft->retn, already_visited);
               if(ft->prms)
                  recursive_examinate(ft->prms, already_visited);
               break;
            }
            case method_type_K:
            {
               const method_type* mt = GetPointer<method_type>(curr_tn);
               if(mt->retn)
                  recursive_examinate(mt->retn, already_visited);
               if(mt->prms)
                  recursive_examinate(mt->prms, already_visited);
               if(mt->clas)
                  recursive_examinate(mt->clas, already_visited);
               break;
            }
            case integer_type_K:
            {
               auto* it = GetPointer<integer_type>(curr_tn);
               if(it->name and GetPointer<identifier_node>(GET_NODE(it->name)))
               {
                  auto* in = GetPointer<identifier_node>(GET_NODE(it->name));
                  if(in->strg == "sizetype")
                  {
                     const std::string INT = "unsigned long";
                     in->strg = INT;
                  }
                  if(in->strg == "ssizetype")
                  {
                     const std::string INT = "long";
                     in->strg = INT;
                  }
                  else if(in->strg == "bitsizetype")
                  {
                     const std::string INT = "unsigned long long int";
                     in->strg = INT;
                  }
                  else if(in->strg == "bit_size_type")
                  {
                     const std::string INT = "unsigned long long int";
                     in->strg = INT;
                  }
               }
               break;
            }
            case template_type_parm_K:
            {
               const template_type_parm* ttp = GetPointer<template_type_parm>(curr_tn);
               recursive_examinate(ttp->name, already_visited);
               break;
            }
            case typename_type_K:
            {
               const typename_type* tt = GetPointer<typename_type>(curr_tn);
               recursive_examinate(tt->name, already_visited);
               break;
            }

            case enumeral_type_K:
            case CharType_K:
            case nullptr_type_K:
            case type_pack_expansion_K:
            case type_argument_pack_K:
            case complex_type_K:
            case real_type_K:
            case void_type_K:
               break;
            case binfo_K:
            case block_K:
            case constructor_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case identifier_node_K:
            case lang_type_K:
            case offset_type_K:
            case qual_union_type_K:
            case set_type_K:
            case ssa_name_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case error_mark_K:
            case lut_expr_K:
            case CASE_CPP_NODES:
            case CASE_BINARY_EXPRESSION:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TERNARY_EXPRESSION:
            case CASE_UNARY_EXPRESSION:
            default:
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         }
         bool is_system;
         std::string include = std::get<0>(behavioral_helper->get_definition(index, is_system));
#if HAVE_BAMBU_BUILT
         if(include.find("etc/libbambu") != std::string::npos or include.find(std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include")) != std::string::npos or
            include.find(std::string(PANDA_DATA_INSTALLDIR "/panda/ac_math/include")) != std::string::npos or (ty->name and GetPointer<const type_decl>(GET_CONST_NODE(ty->name)) and GetPointer<const type_decl>(GET_CONST_NODE(ty->name))->libbambu_flag))
         {
            ty->libbambu_flag = true;
         }
#endif
         if(!ty->system_flag && (is_system || is_system_include(include)))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System type");
            ty->system_flag = true;
            auto* sr = GetPointer<srcp>(curr_tn);
            if(!is_system && sr && sr->include_name != "")
            {
               std::string new_include;
               getRealInclName(include, new_include);
               sr->include_name = new_include;
            }
         }
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            recursive_examinate(tmr->symbol, already_visited);
         if(tmr->base)
            recursive_examinate(tmr->base, already_visited);
         if(tmr->idx)
            recursive_examinate(tmr->idx, already_visited);
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            recursive_examinate(tmr->base, already_visited);
         if(tmr->idx)
            recursive_examinate(tmr->idx, already_visited);
         if(tmr->idx2)
            recursive_examinate(tmr->idx2, already_visited);
         break;
      }
      case real_cst_K:
      case complex_cst_K:
      case string_cst_K:
      case integer_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case tree_vec_K:
      case case_label_expr_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case identifier_node_K:
      case CASE_PRAGMA_NODES:
         break;
      case cast_expr_K:
      {
         const cast_expr* ce = GetPointer<cast_expr>(curr_tn);
         if(ce->op)
            recursive_examinate(ce->op, already_visited);
         break;
      }
      case binfo_K:
      case block_K:
      case baselink_K:
      case ctor_initializer_K:
      case do_stmt_K:
      case expr_stmt_K:
      case if_stmt_K:
      case for_stmt_K:
      case handler_K:
      case modop_expr_K:
      case new_expr_K:
      case overload_K:
      case return_stmt_K:
      case scope_ref_K:
      case template_id_expr_K:
      case template_parm_index_K:
      case trait_expr_K:
      case try_block_K:
      case vec_new_expr_K:
      case while_stmt_K:
      case nontype_argument_pack_K:
      case expr_pack_expansion_K:
      case CASE_FAKE_NODES:
      case gimple_bind_K:
      case gimple_phi_K:
      case gimple_resx_K:
      case statement_list_K:
      case target_expr_K:
      case error_mark_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked @" + STR(index));
   return;
}

bool CheckSystemType::is_system_include(std::string include) const
{
   if(include == "<built-in>")
      return true;
   std::string include_p(FILENAME_NORM(include));
   for(auto& i : systemIncPath)
   {
      if(include_p.compare(0, i.size() + 1, i + "/") == 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Include is " + include + ": system include");
         return true;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Include is " + include + ": non-system include");
   return false;
}

void CheckSystemType::build_include_structures()
{
   // Ok, now I put here the includes which must be modified
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/ipc.h")] = FILENAME_NORM("sys/ipc.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/sem.h")] = FILENAME_NORM("sys/sem.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/mathcalls.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/math-finite.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/types.h")] = FILENAME_NORM("sys/types.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/stat.h")] = FILENAME_NORM("sys/stat.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/mman.h")] = FILENAME_NORM("sys/mman.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/in.h")] = FILENAME_NORM("netinet/in.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/errno.h")] = FILENAME_NORM("errno.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/fcntl.h")] = FILENAME_NORM("fcntl.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/link.h")] = FILENAME_NORM("link.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/shm.h")] = FILENAME_NORM("sys/shm.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/stdio.h")] = FILENAME_NORM("stdio.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/resource.h")] = FILENAME_NORM("sys/resource.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/sigthread.h")] = FILENAME_NORM("pthread.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/string2.h")] = FILENAME_NORM("string.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/time.h")] = FILENAME_NORM("sys/time.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/pthreadtypes.h")] = FILENAME_NORM("pthread.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/sched.h")] = FILENAME_NORM("sched.h");
   inclNameToPath[FILENAME_NORM("i386-linux-gnu/bits/stdio2.h")] = FILENAME_NORM("stdio.h");
   inclNameToPath[FILENAME_NORM("bits/ipc.h")] = FILENAME_NORM("sys/ipc.h");
   inclNameToPath[FILENAME_NORM("bits/sem.h")] = FILENAME_NORM("sys/sem.h");
   inclNameToPath[FILENAME_NORM("bits/mathcalls.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("bits/math-finite.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("bits/types.h")] = FILENAME_NORM("sys/types.h");
   inclNameToPath[FILENAME_NORM("bits/stat.h")] = FILENAME_NORM("sys/stat.h");
   inclNameToPath[FILENAME_NORM("bits/mman.h")] = FILENAME_NORM("sys/mman.h");
   inclNameToPath[FILENAME_NORM("bits/in.h")] = FILENAME_NORM("netinet/in.h");
   inclNameToPath[FILENAME_NORM("bits/errno.h")] = FILENAME_NORM("errno.h");
   inclNameToPath[FILENAME_NORM("bits/fcntl.h")] = FILENAME_NORM("fcntl.h");
   inclNameToPath[FILENAME_NORM("bits/link.h")] = FILENAME_NORM("link.h");
   inclNameToPath[FILENAME_NORM("bits/shm.h")] = FILENAME_NORM("sys/shm.h");
   inclNameToPath[FILENAME_NORM("bits/stdio.h")] = FILENAME_NORM("stdio.h");
   inclNameToPath[FILENAME_NORM("bits/resource.h")] = FILENAME_NORM("sys/resource.h");
   inclNameToPath[FILENAME_NORM("bits/sigthread.h")] = FILENAME_NORM("pthread.h");
   inclNameToPath[FILENAME_NORM("bits/string2.h")] = FILENAME_NORM("string.h");
   inclNameToPath[FILENAME_NORM("bits/time.h")] = FILENAME_NORM("sys/time.h");
   inclNameToPath[FILENAME_NORM("bits/pthreadtypes.h")] = FILENAME_NORM("pthread.h");
   inclNameToPath[FILENAME_NORM("bits/sched.h")] = FILENAME_NORM("sched.h");
   inclNameToPath[FILENAME_NORM("bits/stdio2.h")] = FILENAME_NORM("stdio.h");
   inclNameToPath[FILENAME_NORM("libio.h")] = FILENAME_NORM("stdio.h");

   /// libbambu/libm
   inclNameToPath[FILENAME_NORM("libm/math_private_kernels.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("libm/math_private.h")] = FILENAME_NORM("math.h");
   inclNameToPath[FILENAME_NORM("libm/math_privatef.h")] = FILENAME_NORM("math.h");
   //   inclNameToPath[FILENAME_NORM("softfloat/bambu.h")] = FILENAME_NORM("stdio.h");
   //   inclNameToPath[FILENAME_NORM("softfloat/softfloat.h")] = FILENAME_NORM("stdio.h");

   /// libbambu/soft-fp
   //   inclNameToPath[FILENAME_NORM("soft-fp/single.h")] = FILENAME_NORM("math.h");
   //   inclNameToPath[FILENAME_NORM("soft-fp/soft-fp.h")] = FILENAME_NORM("math.h");
   //   inclNameToPath[FILENAME_NORM("soft-fp/bambu-arch.h")] = FILENAME_NORM("stdlib.h");

   std::vector<std::string> Splitted;
   const GccWrapperConstRef gcc_wrapper(new GccWrapper(parameters, GccWrapper_CompilerTarget::CT_NO_GCC, GccWrapper_OptimizationSet::O0));
   gcc_wrapper->GetSystemIncludes(Splitted);

   for(auto& tok_iter : Splitted)
   {
      if(tok_iter != "")
      {
         std::string temp;
         if(getenv("MINGW_INST_DIR"))
         {
            std::string mingw_prefix = getenv("MINGW_INST_DIR");
            temp = tok_iter;
            if(boost::algorithm::starts_with(temp, "z:/mingw"))
               temp = temp.replace(0, 8, FILENAME_NORM(mingw_prefix)); /// replace z:/mingw at the beginning of the string
            temp = FILENAME_NORM(temp);
            systemIncPath.push_back(temp);
         }
         else if(getenv("APPDIR"))
         {
            std::string app_prefix = getenv("APPDIR");
            temp = app_prefix + "/" + FILENAME_NORM(tok_iter);
            systemIncPath.push_back(temp);
         }
         else
         {
            temp = FILENAME_NORM(tok_iter);
            systemIncPath.push_back(temp);
         }
      }
   }
   systemIncPath.push_back("/usr/local/share/hframework/include");
#if HAVE_BAMBU_BUILT
   if(not parameters->isOption(OPT_pretty_print))
      systemIncPath.push_back(LIBBAMBU_SRCDIR);
#endif

   /// Building the rename function map
   rename_function["_IO_putc"] = "putc";
   rename_function["_IO_getc"] = "getc";

   /// Building the rename type map
   rename_types["__time_t"] = "long int";
   rename_types["__suseconds_t"] = "long int";

   /// Building library system function
   library_system_includes.insert("math.h");
   library_system_functions.insert("__errno_location");
   library_system_functions.insert("exit");
   library_system_functions.insert("abort");

#if HAVE_LEON3
   /// Building not supported function
   not_supported_leon3_functions.insert("fopen");
#endif

   undefined_library_function_include["atof"] = "stdlib.h";
   undefined_library_function_include["atoi"] = "stdlib.h";
   undefined_library_function_include["srand48"] = "stdlib.h";
   undefined_library_function_include["va_start"] = "stdarg.h";
   undefined_library_function_include["va_end"] = "stdarg.h";
   undefined_library_function_include["lgamma"] = "math.h";
   undefined_library_function_include["lgammaf"] = "math.h";
   undefined_library_function_include["lgamma_r"] = "math.h";
   undefined_library_function_include["lgammaf_r"] = "math.h";
}

void CheckSystemType::getRealInclName(const std::string& include, std::string& real_name) const
{
   // Now I have to see if one of the elements in systemIncPath is the start of the include:
   // in case I eliminate it and look the remaining part of the string in the map
   std::string include_p(FILENAME_NORM(include));
   for(auto& i : systemIncPath)
   {
      if(include_p.compare(0, i.size() + 1, i + "/") == 0)
      {
         std::string trimmed = include_p.substr(i.size() + 1);
         if(inclNameToPath.find(trimmed) != inclNameToPath.end())
            real_name = inclNameToPath.find(trimmed)->second;
#if HAVE_BAMBU_BUILT
         else if(LIBBAMBU_SRCDIR == i && boost::algorithm::starts_with(trimmed, "libm/"))
            real_name = FILENAME_NORM("math.h");
#endif
         else
            real_name = trimmed;
         return;
      }
   }

   // If, finally, the include is not a system one I simply print it back as it is
   real_name = include;
}

bool CheckSystemType::HasToBeExecuted() const
{
   return not already_executed;
}
