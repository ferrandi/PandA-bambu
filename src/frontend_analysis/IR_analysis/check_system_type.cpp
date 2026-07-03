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
 * @file check_system_type.cpp
 * @brief analyse loc_info of variables and types to detect system ones; the identified one are flagged
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "check_system_type.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "string_manipulation.hpp"

#include <filesystem>
#include <fstream>

#include "config_PANDA_ETC_BUILDDIR.hpp"
#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"

namespace
{
   const CustomMap<std::string, std::string> inclNameToPath = {{"i386-linux-gnu/bits/ipc.h", "sys/ipc.h"},
                                                               {"i386-linux-gnu/bits/sem.h", "sys/sem.h"},
                                                               {"i386-linux-gnu/bits/mathcalls.h", "math.h"},
                                                               {"i386-linux-gnu/bits/math-finite.h", "math.h"},
                                                               {"i386-linux-gnu/bits/types.h", "sys/types.h"},
                                                               {"i386-linux-gnu/bits/stat.h", "sys/stat.h"},
                                                               {"i386-linux-gnu/bits/mman.h", "sys/mman.h"},
                                                               {"i386-linux-gnu/bits/in.h", "netinet/in.h"},
                                                               {"i386-linux-gnu/bits/errno.h", "errno.h"},
                                                               {"i386-linux-gnu/bits/fcntl.h", "fcntl.h"},
                                                               {"i386-linux-gnu/bits/link.h", "link.h"},
                                                               {"i386-linux-gnu/bits/shm.h", "sys/shm.h"},
                                                               {"i386-linux-gnu/bits/stdio.h", "stdio.h"},
                                                               {"i386-linux-gnu/bits/resource.h", "sys/resource.h"},
                                                               {"i386-linux-gnu/bits/sigthread.h", "pthread.h"},
                                                               {"i386-linux-gnu/bits/string2.h", "string.h"},
                                                               {"i386-linux-gnu/bits/time.h", "sys/time.h"},
                                                               {"i386-linux-gnu/bits/pthreadtypes.h", "pthread.h"},
                                                               {"i386-linux-gnu/bits/sched.h", "sched.h"},
                                                               {"i386-linux-gnu/bits/stdio2.h", "stdio.h"},
                                                               {"bits/ipc.h", "sys/ipc.h"},
                                                               {"bits/sem.h", "sys/sem.h"},
                                                               {"bits/mathcalls.h", "math.h"},
                                                               {"bits/math-finite.", "math.h"},
                                                               {"bits/types.h", "sys/types.h"},
                                                               {"bits/stat.h", "sys/stat.h"},
                                                               {"bits/mman.h", "sys/mman.h"},
                                                               {"bits/in.h", "netinet/in.h"},
                                                               {"bits/errno.h", "errno.h"},
                                                               {"bits/fcntl.h", "fcntl.h"},
                                                               {"bits/link.h", "link.h"},
                                                               {"bits/shm.h", "sys/shm.h"},
                                                               {"bits/stdio.h", "stdio.h"},
                                                               {"bits/resource.h", "sys/resource.h"},
                                                               {"bits/sigthread.h", "pthread.h"},
                                                               {"bits/string2.h", "string.h"},
                                                               {"bits/time.h", "sys/time.h"},
                                                               {"bits/pthreadtypes.h", "pthread.h"},
                                                               {"bits/sched.h", "sched.h"},
                                                               {"bits/stdio2.h", "stdio.h"},
                                                               {"libio.h", "stdio.h"},
                                                               {"libm/hls/hlsmath.h", "math.h"},
                                                               {"libm/musl/musl_math.h", "math.h"},
                                                               {"libm/newlib/newlib_math.h", "math.h"}};

   const CustomMap<std::string, std::string> rename_function = {{"_IO_putc", "putc"}, {"_IO_getc", "getc"}};

   const CustomMap<std::string, std::string> rename_types = {{"__time_t", "long int"}, {"__suseconds_t", "long int"}};

   const CustomSet<std::string> library_system_functions = {
       {"__errno_location", "exit", "abort"},
   };

   const CustomSet<std::string> library_system_includes = {{"math.h"}};

   const CustomMap<std::string, std::string> undefined_library_function_include = {
       {"atof", "stdlib.h"},   {"atoi", "stdlib.h"},   {"srand48", "stdlib.h"}, {"va_start", "stdarg.h"},
       {"va_end", "stdarg.h"}, {"lgamma_r", "math.h"}, {"lgammaf_r", "math.h"}};

   std::vector<std::filesystem::path> system_includes;

   bool is_system_include(const std::filesystem::path& include)
   {
      if(include == "<built-in>")
      {
         return true;
      }
      const auto include_p = std::filesystem::weakly_canonical(include);
      return std::any_of(system_includes.begin(), system_includes.end(),
                         [&](const auto& system_path) { return is_subpath(include_p, system_path); });
   }

   std::string getRealInclName(const std::filesystem::path& include)
   {
      // Now I have to see if one of the elements in system_includes is the start of the include:
      // in case I eliminate it and lookup the remaining part of the string in the map
      const auto include_p = std::filesystem::weakly_canonical(include);
      for(const auto& system_path : system_includes)
      {
         if(is_subpath(include_p, system_path))
         {
            auto include_r = include_p.lexically_relative(system_path).string();
            if(inclNameToPath.find(include_r) != inclNameToPath.end())
            {
               return inclNameToPath.find(include_r)->second;
            }
            return include_r;
         }
      }

      // If, finally, the include is not a system one I simply print it back as it is
      return include.string();
   }
} // namespace

CheckSystemType::CheckSystemType(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CHECK_SYSTEM_TYPE, _design_flow_manager, _parameters),
      behavioral_helper(_AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()),
      TM(_AppM->get_ir_manager())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   if(system_includes.empty())
   {
      const CompilerWrapper compiler_wrapper(
          _parameters, _parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler),
          CompilerWrapper_OptimizationSet::O0);
      system_includes = compiler_wrapper.GetSystemIncludes();
   }
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CheckSystemType::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
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
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status CheckSystemType::InternalExec()
{
   const auto fnode = TM->GetIRNode(function_id);
   const auto* fd = GetPointerS<function_val_node>(fnode);
   const auto* sl = GetPointerS<statement_list_node>(fd->body);

   CustomUnorderedSet<unsigned int> already_visited;
   recursive_examinate(fnode, already_visited);

   for(const auto f : AppM->get_functions_without_body())
   {
      recursive_examinate(TM->GetIRNode(f), already_visited);
   }

   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      if(bb)
      {
         for(const auto& stmt : bb->CGetStmtList())
         {
            recursive_examinate(stmt, already_visited);
         }
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}

void CheckSystemType::recursive_examinate(const ir_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_visited) const
{
   THROW_ASSERT(tn, "Empty current IR node");
   if(!already_visited.insert(tn->index).second)
   {
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking @" + STR(tn->index));
   switch(tn->get_kind())
   {
      case CASE_DECL_NODES:
      {
         const auto dn = GetPointerS<decl_node>(tn);
         bool is_system;
         const auto [include, line, col] = ir_helper::GetSourcePath(tn, is_system);
         if(tn->get_kind() == function_val_node_K && library_system_functions.count(ir_helper::GetFunctionName(tn)))
         {
            dn->library_system_flag = true;
         }
         else if(!dn->operating_system_flag && !dn->library_system_flag && (is_system || is_system_include(include)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System declaration");
            const auto sr = GetPointer<IR_LocInfo>(tn);
            THROW_ASSERT(sr, "unexpected condition");
            if(!is_system && !sr->include_name.empty())
            {
               sr->include_name = getRealInclName(include);
            }
            if(tn->get_kind() == function_val_node_K && library_system_includes.count(sr->include_name))
            {
               dn->library_system_flag = true;
            }
            else
            {
               dn->operating_system_flag = true;
            }
         }
         else if(tn->get_kind() == function_val_node_K &&
                 undefined_library_function_include.count(ir_helper::GetFunctionName(tn)))
         {
            dn->library_system_flag = true;
            const auto sr = GetPointer<IR_LocInfo>(tn);
            sr->include_name = undefined_library_function_include.at(ir_helper::GetFunctionName(tn));
         }

         if(tn->get_kind() == function_val_node_K)
         {
            auto fd = GetPointerS<function_val_node>(tn);
            if(fd->name && fd->name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<identifier_node>(fd->name);
               if(rename_function.count(in->strg))
               {
                  in->strg = rename_function.at(in->strg);
               }
            }
         }

         if(starts_with(include, PANDA_ETC_BUILDDIR))
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
      case CASE_TYPE_NODES:
      {
         const auto ty = GetPointerS<type_node>(tn);

         switch(tn->get_kind())
         {
            case integer_ty_node_K:
            {
               break;
            }
            case vector_ty_node_K:
            {
               const auto vt = GetPointerS<vector_ty_node>(tn);
               recursive_examinate(vt->elts, already_visited);
               break;
            }
            case array_ty_node_K:
            {
               const auto at = GetPointerS<array_ty_node>(tn);
               recursive_examinate(at->elts, already_visited);
               break;
            }
            case struct_ty_node_K:
            {
               const auto rt = GetPointerS<struct_ty_node>(tn);
               if(rt->name)
               {
                  recursive_examinate(rt->name, already_visited);
               }
               for(const auto& it : rt->list_of_flds)
               {
                  recursive_examinate(it, already_visited);
                  if(!rt->libbambu_flag && ir_helper::IsInLibbambu(it))
                  {
                     rt->libbambu_flag = true;
                  }
               }
               break;
            }
            case pointer_ty_node_K:
            {
               const auto pt = GetPointerS<pointer_ty_node>(tn);
               recursive_examinate(pt->ptd, already_visited);
               break;
            }
            case function_ty_node_K:
            {
               const auto ft = GetPointerS<function_ty_node>(tn);
               if(ft->retn)
               {
                  recursive_examinate(ft->retn, already_visited);
               }
               for(const auto& p : ft->list_of_args_type)
               {
                  recursive_examinate(p, already_visited);
               }
               break;
            }
            case real_ty_node_K:
            case void_ty_node_K:
               break;
            case call_node_K:
            case constructor_node_K:
            case identifier_node_K:
            case lut_node_K:
            case ssa_node_K:
            case statement_list_node_K:
            case CASE_BINARY_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_TERNARY_NODES:
            case CASE_UNARY_NODES:
            default:
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + tn->get_kind_text());
         }
         bool is_system;
         const auto [include, line, col] = ir_helper::GetSourcePath(tn, is_system);
         if(!ty->libbambu_flag && (starts_with(include, relocate_install_path(PANDA_INCLUDE_INSTALLDIR).string()) ||
                                   starts_with(include, PANDA_ETC_BUILDDIR)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bambu type");
            ty->libbambu_flag = true;
         }
         if(!ty->system_flag && (is_system || is_system_include(include)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "System type");
            ty->system_flag = true;
            const auto sr = GetPointer<IR_LocInfo>(tn);
            if(!is_system && sr && sr->include_name != "")
            {
               sr->include_name = getRealInclName(include);
            }
         }
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(tn);
         recursive_examinate(ce->fn, already_visited);
         for(const auto& arg : ce->args)
         {
            recursive_examinate(arg, already_visited);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<call_stmt>(tn);
         recursive_examinate(ce->fn, already_visited);
         for(const auto& arg : ce->args)
         {
            recursive_examinate(arg, already_visited);
         }
         if(ce->predicate)
         {
            recursive_examinate(ce->predicate, already_visited);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(tn);
         recursive_examinate(gm->op0, already_visited);
         recursive_examinate(gm->op1, already_visited);
         if(gm->predicate)
         {
            recursive_examinate(gm->predicate, already_visited);
         }
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<ssa_node>(tn);
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
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(tn);
         if(ue->type)
         {
            recursive_examinate(ue->type, already_visited);
         }
         recursive_examinate(ue->op, already_visited);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(tn);
         if(be->type)
         {
            recursive_examinate(be->type, already_visited);
         }
         recursive_examinate(be->op0, already_visited);
         recursive_examinate(be->op1, already_visited);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(tn);
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
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(tn);
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
      case constructor_node_K:
      {
         const auto co = GetPointerS<constructor_node>(tn);
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
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               recursive_examinate(cond.first, already_visited);
            }
         }
         break;
      }
      case return_stmt_K:
      {
         const auto re = GetPointerS<return_stmt>(tn);
         if(re->op)
         {
            recursive_examinate(re->op, already_visited);
         }
         break;
      }
      case nop_stmt_K:
      case identifier_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
         break;
      case phi_stmt_K:
      case statement_list_node_K:
      case CASE_FAKE_NODES:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked @" + STR(tn->index));
}
