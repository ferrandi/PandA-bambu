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

/// Utility include
#include <filesystem>

/// Wrapper include
#include "compiler_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define FILENAME_NORM(name) (std::filesystem::path(name).lexically_normal().string())

std::vector<std::string> CheckSystemType::systemIncPath;

const CustomUnorderedMap<std::string, std::string> CheckSystemType::inclNameToPath = {
    {FILENAME_NORM("i386-linux-gnu/bits/ipc.h"), FILENAME_NORM("sys/ipc.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/sem.h"), FILENAME_NORM("sys/sem.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/mathcalls.h"), FILENAME_NORM("math.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/math-finite.h"), FILENAME_NORM("math.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/types.h"), FILENAME_NORM("sys/types.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/stat.h"), FILENAME_NORM("sys/stat.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/mman.h"), FILENAME_NORM("sys/mman.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/in.h"), FILENAME_NORM("netinet/in.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/errno.h"), FILENAME_NORM("errno.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/fcntl.h"), FILENAME_NORM("fcntl.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/link.h"), FILENAME_NORM("link.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/shm.h"), FILENAME_NORM("sys/shm.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/stdio.h"), FILENAME_NORM("stdio.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/resource.h"), FILENAME_NORM("sys/resource.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/sigthread.h"), FILENAME_NORM("pthread.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/string2.h"), FILENAME_NORM("string.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/time.h"), FILENAME_NORM("sys/time.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/pthreadtypes.h"), FILENAME_NORM("pthread.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/sched.h"), FILENAME_NORM("sched.h")},
    {FILENAME_NORM("i386-linux-gnu/bits/stdio2.h"), FILENAME_NORM("stdio.h")},
    {FILENAME_NORM("bits/ipc.h"), FILENAME_NORM("sys/ipc.h")},
    {FILENAME_NORM("bits/sem.h"), FILENAME_NORM("sys/sem.h")},
    {FILENAME_NORM("bits/mathcalls.h"), FILENAME_NORM("math.h")},
    {FILENAME_NORM("bits/math-finite."), FILENAME_NORM("math.h")},
    {FILENAME_NORM("bits/types.h"), FILENAME_NORM("sys/types.h")},
    {FILENAME_NORM("bits/stat.h"), FILENAME_NORM("sys/stat.h")},
    {FILENAME_NORM("bits/mman.h"), FILENAME_NORM("sys/mman.h")},
    {FILENAME_NORM("bits/in.h"), FILENAME_NORM("netinet/in.h")},
    {FILENAME_NORM("bits/errno.h"), FILENAME_NORM("errno.h")},
    {FILENAME_NORM("bits/fcntl.h"), FILENAME_NORM("fcntl.h")},
    {FILENAME_NORM("bits/link.h"), FILENAME_NORM("link.h")},
    {FILENAME_NORM("bits/shm.h"), FILENAME_NORM("sys/shm.h")},
    {FILENAME_NORM("bits/stdio.h"), FILENAME_NORM("stdio.h")},
    {FILENAME_NORM("bits/resource.h"), FILENAME_NORM("sys/resource.h")},
    {FILENAME_NORM("bits/sigthread.h"), FILENAME_NORM("pthread.h")},
    {FILENAME_NORM("bits/string2.h"), FILENAME_NORM("string.h")},
    {FILENAME_NORM("bits/time.h"), FILENAME_NORM("sys/time.h")},
    {FILENAME_NORM("bits/pthreadtypes.h"), FILENAME_NORM("pthread.h")},
    {FILENAME_NORM("bits/sched.h"), FILENAME_NORM("sched.h")},
    {FILENAME_NORM("bits/stdio2.h"), FILENAME_NORM("stdio.h")},
    {FILENAME_NORM("libio.h"), FILENAME_NORM("stdio.h")},
    {FILENAME_NORM("libm/math_private_kernels.h"), FILENAME_NORM("math.h")},
    {FILENAME_NORM("libm/math_private.h"), FILENAME_NORM("math.h")},
    {FILENAME_NORM("libm/math_privatef.h"), FILENAME_NORM("math.h")}};

const CustomUnorderedMap<std::string, std::string> CheckSystemType::rename_function = {{"_IO_putc", "putc"},
                                                                                       {"_IO_getc", "getc"}};

const CustomUnorderedMap<std::string, std::string> CheckSystemType::rename_types = {{"__time_t", "long int"},
                                                                                    {"__suseconds_t", "long int"}};

const CustomUnorderedSet<std::string> CheckSystemType::library_system_functions = {
    {"__errno_location", "exit", "abort"},
};

const CustomUnorderedSet<std::string> CheckSystemType::library_system_includes = {{"math.h"}};

const CustomUnorderedMap<std::string, std::string> CheckSystemType::undefined_library_function_include = {
    {"atof", "stdlib.h"},     {"atoi", "stdlib.h"},   {"srand48", "stdlib.h"},
    {"va_start", "stdarg.h"}, {"va_end", "stdarg.h"}, {"lgamma", "math.h"},
    {"lgammaf", "math.h"},    {"lgamma_r", "math.h"}, {"lgammaf_r", "math.h"}};

CheckSystemType::CheckSystemType(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CHECK_SYSTEM_TYPE, _design_flow_manager, _parameters),
      behavioral_helper(AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()),
      TM(AppM->get_tree_manager()),
      already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   if(systemIncPath.size() == 0)
   {
      build_include_structures(parameters);
   }
}

CheckSystemType::~CheckSystemType() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CheckSystemType::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         break;
      }
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         break;
      }
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

bool CheckSystemType::HasToBeExecuted() const
{
   return !already_executed;
}

DesignFlowStep_Status CheckSystemType::InternalExec()
{
   const auto curr_tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<function_decl>(curr_tn);
   const auto sl = GetPointerS<statement_list>(fd->body);

   CustomUnorderedSet<unsigned int> already_visited;
   recursive_examinate(curr_tn, function_id, already_visited);

   for(const auto f : AppM->get_functions_without_body())
   {
      recursive_examinate(TM->GetTreeNode(f), already_visited);
   }

   for(const auto& bbi_bb : sl->list_of_bloc)
   {
      const auto& bb = bbi_bb.second;
      if(bb)
      {
         for(const auto& stmt : bb->CGetStmtList())
         {
            recursive_examinate(stmt, already_visited);
         }
      }
   }

   already_executed = true;
   return DesignFlowStep_Status::SUCCESS;
}

void CheckSystemType::recursive_examinate(const tree_nodeRef& tn,
                                          CustomUnorderedSet<unsigned int>& already_visited) const
{
   recursive_examinate(tn, tn->index, already_visited);
}

void CheckSystemType::recursive_examinate(const tree_nodeRef& curr_tn, const unsigned int index,
                                          CustomUnorderedSet<unsigned int>& already_visited) const
{
   THROW_ASSERT(curr_tn, "Empty current tree node");
   if(already_visited.count(index))
   {
      return;
   }
   already_visited.insert(index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking @" + STR(index));
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<call_expr>(curr_tn);
         recursive_examinate(ce->fn, already_visited);
         for(const auto& arg : ce->args)
         {
            recursive_examinate(arg, already_visited);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<gimple_call>(curr_tn);
         recursive_examinate(ce->fn, already_visited);
         for(const auto& arg : ce->args)
         {
            recursive_examinate(arg, already_visited);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<gimple_assign>(curr_tn);
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
         const auto dn = GetPointerS<decl_node>(curr_tn);
         auto fd = GetPointer<function_decl>(curr_tn);
         bool is_system;
         const auto include = std::get<0>(behavioral_helper->get_definition(index, is_system));
         if(fd && library_system_functions.count(tree_helper::print_function_name(TM, fd)))
         {
            dn->library_system_flag = true;
         }
         else if(!dn->operating_system_flag && !dn->library_system_flag && (is_system || is_system_include(include)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System declaration");
            const auto sr = GetPointer<srcp>(curr_tn);
            if(!is_system && sr && sr->include_name != "")
            {
               sr->include_name = getRealInclName(include);
            }
            fd = GetPointer<function_decl>(curr_tn);
            if(fd)
            {
               if(sr)
               {
                  if(library_system_includes.count(sr->include_name))
                  {
                     dn->library_system_flag = true;
                  }
                  else
                  {
                     dn->operating_system_flag = true;
                  }
               }
               else
               {
                  dn->operating_system_flag = true;
               }
            }
            else
            {
               dn->operating_system_flag = true;
            }
         }
         else if(fd && undefined_library_function_include.count(tree_helper::print_function_name(TM, fd)))
         {
            dn->library_system_flag = true;
            const auto sr = GetPointer<srcp>(curr_tn);
            sr->include_name = undefined_library_function_include.at(tree_helper::print_function_name(TM, fd));
         }
         // Checking for implicit declaration
         if(curr_tn->get_kind() == function_decl_K)
         {
            fd = GetPointerS<function_decl>(curr_tn);
            const auto sr = GetPointer<srcp>(curr_tn);
            if(fd->type && fd->undefined_flag && !fd->operating_system_flag && !fd->library_system_flag &&
               sr->include_name != "<built-in>")
            {
               const auto ft = GetPointerS<function_type>(fd->type);
               if(!ft->prms && ft->retn && GetPointer<integer_type>(ft->retn))
               {
                  const auto it = GetPointerS<integer_type>(ft->retn);
                  if(it->name && GetPointer<type_decl>(it->name))
                  {
                     const auto td = GetPointerS<type_decl>(it->name);
                     if(td->name && GetPointer<identifier_node>(td->name))
                     {
                        const auto in = GetPointerS<identifier_node>(td->name);
                        if(in->strg == "int")
                        {
                           fd->include_name = "<built-in>";
                        }
                     }
                  }
               }
            }
            if(fd->name && fd->name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<identifier_node>(fd->name);
               if(rename_function.count(in->strg))
               {
                  in->strg = rename_function.at(in->strg);
               }
            }
         }
         // Checking for type
         if(curr_tn->get_kind() == type_decl_K)
         {
            const auto td = GetPointerS<type_decl>(curr_tn);
            if(td->name && td->name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<identifier_node>(td->name);
               if(rename_types.count(in->strg))
               {
                  in->strg = rename_types.at(in->strg);
               }
            }
         }
         if(curr_tn->get_kind() == var_decl_K && dn->artificial_flag &&
            (behavioral_helper->PrintVariable(dn->index) == "__FUNCTION__" or
             behavioral_helper->PrintVariable(dn->index) == "__PRETTY_FUNCTION__"))
         {
            dn->library_system_flag = true;
         }
         if(include.find("etc/libbambu") != std::string::npos)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---In libbambu");
            dn->libbambu_flag = true;
         }
         else if(dn->libbambu_flag)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---In libbambu");
         }
         recursive_examinate(dn->type, already_visited);
         break;
      }
      case ssa_name_K:
      {
         const auto sn = GetPointerS<ssa_name>(curr_tn);
         if(sn->type)
         {
            recursive_examinate(sn->type, already_visited);
         }
         else
         {
            recursive_examinate(sn->var, already_visited);
         }
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = curr_tn;
         while(current)
         {
            if(GetPointerS<tree_list>(current)->purp)
            {
               recursive_examinate(GetPointerS<tree_list>(current)->purp, already_visited);
            }
            recursive_examinate(GetPointerS<tree_list>(current)->valu, already_visited);
            current = GetPointerS<tree_list>(current)->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<unary_expr>(curr_tn);
         if(ue->type)
         {
            recursive_examinate(ue->type, already_visited);
         }
         recursive_examinate(ue->op, already_visited);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(curr_tn);
         if(be->type)
         {
            recursive_examinate(be->type, already_visited);
         }
         recursive_examinate(be->op0, already_visited);
         recursive_examinate(be->op1, already_visited);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<ternary_expr>(curr_tn);
         if(te->type)
         {
            recursive_examinate(te->type, already_visited);
         }
         recursive_examinate(te->op0, already_visited);
         if(te->op1)
         {
            recursive_examinate(te->op1, already_visited);
         }
         if(te->op2)
         {
            recursive_examinate(te->op2, already_visited);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<quaternary_expr>(curr_tn);
         if(qe->type)
         {
            recursive_examinate(qe->type, already_visited);
         }
         recursive_examinate(qe->op0, already_visited);
         if(qe->op1)
         {
            recursive_examinate(qe->op1, already_visited);
         }
         if(qe->op2)
         {
            recursive_examinate(qe->op2, already_visited);
         }
         if(qe->op3)
         {
            recursive_examinate(qe->op3, already_visited);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<lut_expr>(curr_tn);
         recursive_examinate(le->op0, already_visited);
         recursive_examinate(le->op1, already_visited);
         if(le->op2)
         {
            recursive_examinate(le->op2, already_visited);
         }
         if(le->op3)
         {
            recursive_examinate(le->op3, already_visited);
         }
         if(le->op4)
         {
            recursive_examinate(le->op4, already_visited);
         }
         if(le->op5)
         {
            recursive_examinate(le->op5, already_visited);
         }
         if(le->op6)
         {
            recursive_examinate(le->op6, already_visited);
         }
         if(le->op7)
         {
            recursive_examinate(le->op7, already_visited);
         }
         if(le->op8)
         {
            recursive_examinate(le->op8, already_visited);
         }
         break;
      }
      case constructor_K:
      {
         const auto co = GetPointerS<constructor>(curr_tn);
         if(co->type)
         {
            recursive_examinate(co->type, already_visited);
         }
         else
         {
            for(const auto& it : co->list_of_idx_valu)
            {
               recursive_examinate(it.second, already_visited);
            }
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<gimple_cond>(curr_tn);
         recursive_examinate(gc->op0, already_visited);
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<gimple_switch>(curr_tn);
         recursive_examinate(se->op0, already_visited);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               recursive_examinate(cond.first, already_visited);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         const auto re = GetPointerS<gimple_return>(curr_tn);
         if(re->op)
         {
            recursive_examinate(re->op, already_visited);
         }
         break;
      }
      case gimple_for_K:
      {
         const auto fe = GetPointerS<gimple_for>(curr_tn);
         recursive_examinate(fe->op0, already_visited);
         recursive_examinate(fe->op1, already_visited);
         recursive_examinate(fe->op2, already_visited);
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointerS<gimple_while>(curr_tn);
         recursive_examinate(we->op0, already_visited);
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointerS<gimple_goto>(curr_tn);
         recursive_examinate(ge->op, already_visited);
         break;
      }
      case CASE_TYPE_NODES:
      {
         const auto ty = GetPointerS<type_node>(curr_tn);
         THROW_ASSERT(ty, "expected a name");
         if(ty->name)
         {
            recursive_examinate(ty->name, already_visited);
         }
         switch(curr_tn->get_kind())
         {
            case boolean_type_K:
            {
               //_Bool is C99: we replace it for MAGIC compatibility
               const auto bt = GetPointerS<boolean_type>(curr_tn);
               if(bt->name && GetPointer<type_decl>(bt->name))
               {
                  const auto td = GetPointerS<type_decl>(bt->name);
                  if(td->name && GetPointer<identifier_node>(td->name))
                  {
                     const auto in = GetPointerS<identifier_node>(td->name);
                     if(parameters->isOption(OPT_gcc_standard) &&
                        parameters->getOption<std::string>(OPT_gcc_standard) == "c99" && in->strg == "_Bool")
                     {
                        in->strg = "int";
                     }
                  }
               }
               break;
            }
            case vector_type_K:
            {
               const auto vt = GetPointerS<vector_type>(curr_tn);
               recursive_examinate(vt->elts, already_visited);
               break;
            }
            case array_type_K:
            {
               const auto at = GetPointerS<array_type>(curr_tn);
               recursive_examinate(at->elts, already_visited);
               break;
            }
            case record_type_K:
            {
               const auto rt = GetPointerS<record_type>(curr_tn);
               for(const auto& it : rt->list_of_flds)
               {
                  recursive_examinate(it, already_visited);
                  if(!rt->libbambu_flag && tree_helper::IsInLibbambu(TM, it->index))
                  {
                     rt->libbambu_flag = true;
                  }
               }
               for(const auto& it : rt->list_of_fncs)
               {
                  recursive_examinate(it, already_visited);
               }
               break;
            }
            case union_type_K:
            {
               const auto ut = GetPointerS<union_type>(curr_tn);
               for(const auto& it : ut->list_of_flds)
               {
                  recursive_examinate(it, already_visited);
                  if(!ut->libbambu_flag && tree_helper::IsInLibbambu(TM, it->index))
                  {
                     ut->libbambu_flag = true;
                  }
               }
               for(const auto& it : ut->list_of_fncs)
               {
                  recursive_examinate(it, already_visited);
               }
               break;
            }
            case pointer_type_K:
            {
               const auto pt = GetPointerS<pointer_type>(curr_tn);
               recursive_examinate(pt->ptd, already_visited);
               break;
            }
            case reference_type_K:
            {
               const auto rt = GetPointerS<reference_type>(curr_tn);
               if(rt->refd)
               {
                  recursive_examinate(rt->refd, already_visited);
               }
               break;
            }
            case function_type_K:
            {
               const auto ft = GetPointerS<function_type>(curr_tn);
               if(ft->retn)
               {
                  recursive_examinate(ft->retn, already_visited);
               }
               if(ft->prms)
               {
                  recursive_examinate(ft->prms, already_visited);
               }
               break;
            }
            case method_type_K:
            {
               const auto mt = GetPointerS<method_type>(curr_tn);
               if(mt->retn)
               {
                  recursive_examinate(mt->retn, already_visited);
               }
               if(mt->prms)
               {
                  recursive_examinate(mt->prms, already_visited);
               }
               if(mt->clas)
               {
                  recursive_examinate(mt->clas, already_visited);
               }
               break;
            }
            case integer_type_K:
            {
               const auto it = GetPointerS<integer_type>(curr_tn);
               if(it->name && GetPointer<identifier_node>(it->name))
               {
                  const auto in = GetPointerS<identifier_node>(it->name);
                  if(in->strg == "sizetype")
                  {
                     in->strg = "unsigned long";
                  }
                  else if(in->strg == "ssizetype")
                  {
                     in->strg = "long";
                  }
                  else if(in->strg == "bitsizetype" || in->strg == "bit_size_type")
                  {
                     in->strg = "unsigned long long int";
                  }
               }
               break;
            }
            case template_type_parm_K:
            {
               const auto ttp = GetPointerS<template_type_parm>(curr_tn);
               recursive_examinate(ttp->name, already_visited);
               break;
            }
            case typename_type_K:
            {
               const auto tt = GetPointerS<typename_type>(curr_tn);
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
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + curr_tn->get_kind_text());
         }
         bool is_system;
         const auto include = std::get<0>(behavioral_helper->get_definition(index, is_system));
         if((include.find("etc/libbambu") != std::string::npos) ||
            (include.find(PANDA_DATA_INSTALLDIR "/panda/ac_types/include") != std::string::npos) ||
            (include.find(PANDA_DATA_INSTALLDIR "/panda/ac_math/include") != std::string::npos) ||
            (ty->name && GetPointer<const type_decl>(ty->name) &&
             GetPointerS<const type_decl>(ty->name)->libbambu_flag))
         {
            ty->libbambu_flag = true;
         }
         if(!ty->system_flag && (is_system || is_system_include(include)))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System type");
            ty->system_flag = true;
            const auto sr = GetPointer<srcp>(curr_tn);
            if(!is_system && sr && sr->include_name != "")
            {
               sr->include_name = getRealInclName(include);
            }
         }
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<target_mem_ref>(curr_tn);
         if(tmr->symbol)
         {
            recursive_examinate(tmr->symbol, already_visited);
         }
         if(tmr->base)
         {
            recursive_examinate(tmr->base, already_visited);
         }
         if(tmr->idx)
         {
            recursive_examinate(tmr->idx, already_visited);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            recursive_examinate(tmr->base, already_visited);
         }
         if(tmr->idx)
         {
            recursive_examinate(tmr->idx, already_visited);
         }
         if(tmr->idx2)
         {
            recursive_examinate(tmr->idx2, already_visited);
         }
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
         const auto ce = GetPointerS<cast_expr>(curr_tn);
         if(ce->op)
         {
            recursive_examinate(ce->op, already_visited);
         }
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
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + curr_tn->get_kind_text());
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
   {
      return true;
   }
   std::string include_p(FILENAME_NORM(include));
   for(const auto& i : systemIncPath)
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

void CheckSystemType::build_include_structures(ParameterConstRef parameters)
{
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(
       parameters, CompilerWrapper_CompilerTarget::CT_NO_COMPILER, CompilerWrapper_OptimizationSet::O0));
   std::vector<std::string> Splitted;
   compiler_wrapper->GetSystemIncludes(Splitted);

   for(const auto& tok_iter : Splitted)
   {
      if(tok_iter != "")
      {
         std::string temp;
         std::error_code ec;
         if(getenv("MINGW_INST_DIR"))
         {
            std::string mingw_prefix = getenv("MINGW_INST_DIR");
            temp = tok_iter;
            if(starts_with(temp, "z:/mingw"))
            {
               temp =
                   temp.replace(0, 8, FILENAME_NORM(mingw_prefix)); /// replace z:/mingw at the beginning of the string
            }
            temp = FILENAME_NORM(temp);
         }
         else if(getenv("APPDIR"))
         {
            const std::string app_prefix = getenv("APPDIR");
            temp = FILENAME_NORM(tok_iter);
            const auto canon_temp = std::filesystem::weakly_canonical(temp, ec);
            if(!ec)
            {
               systemIncPath.push_back(canon_temp.string());
               if(temp.find(app_prefix) != 0)
               {
                  temp = app_prefix + "/" + FILENAME_NORM(tok_iter);
               }
               else
               {
                  temp = temp.substr(app_prefix.size());
               }
            }
         }
         else
         {
            temp = FILENAME_NORM(tok_iter);
         }
         const auto canon_temp = std::filesystem::weakly_canonical(temp, ec);
         if(!ec)
         {
            systemIncPath.push_back(canon_temp.string());
         }
      }
   }
   systemIncPath.push_back("/usr/local/share/hframework/include");
   if(!parameters->isOption(OPT_pretty_print))
   {
      systemIncPath.push_back(LIBBAMBU_SRCDIR);
   }
}

std::string CheckSystemType::getRealInclName(const std::string& include)
{
   // Now I have to see if one of the elements in systemIncPath is the start of the include:
   // in case I eliminate it && look the remaining part of the string in the map
   std::string include_p(FILENAME_NORM(include));
   for(const auto& i : systemIncPath)
   {
      if(include_p.compare(0, i.size() + 1, i + "/") == 0)
      {
         const auto trimmed = include_p.substr(i.size() + 1);
         if(inclNameToPath.find(trimmed) != inclNameToPath.end())
         {
            return inclNameToPath.find(trimmed)->second;
         }
         else if(LIBBAMBU_SRCDIR == i && starts_with(trimmed, "libm/"))
         {
            return FILENAME_NORM("math.h");
         }
         else
         {
            return trimmed;
         }
      }
   }

   // If, finally, the include is not a system one I simply print it back as it is
   return include;
}
