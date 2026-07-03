/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * This file is part of the PandA framework.
 *
 * Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "call_graph_builtin_call.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "var_pp_functor.hpp"
#include <string>

namespace
{
   ir_nodeConstRef getFunctionPointerType(const ir_nodeRef& tn)
   {
      THROW_ASSERT(tn->get_kind() == ssa_node_K, "Function pointer not in SSA-form");
      auto* sa = GetPointerS<ssa_node>(tn);
      const auto pt = ir_helper::CGetType(sa->var ? sa->var : tn);

      THROW_ASSERT(pt, "Declaration node has not information about pointer_ty_node");
      THROW_ASSERT(ir_helper::IsPointerType(pt), "Pointer type has not information about pointed function_ty_node");

      return ir_helper::CGetPointedType(pt);
   }
} // namespace

CallGraphBuiltinCall::CallGraphBuiltinCall(const application_managerRef AM, unsigned int functionId,
                                           const DesignFlowManager& DFM, const ParameterConstRef P)
    : FunctionFrontendFlowStep(AM, functionId, CALL_GRAPH_BUILTIN_CALL, DFM, P), modified(false)
{
   debug_level = P->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, CallGraphBuiltinCall::FunctionRelationship>>
CallGraphBuiltinCall::ComputeFrontendRelationships(DesignFlowStep::RelationshipType RT) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(RT)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("HWCallInjection::ComputeFrontendRelationships");
   }
   return relationships;
}

void CallGraphBuiltinCall::Initialize()
{
   const auto TM = AppM->get_ir_manager();
   auto allFunctions = AppM->GetCallGraphManager().GetReachedBodyFunctions();
   typeToDeclaration.clear();
   for(unsigned int idx : allFunctions)
   {
      const auto fnode = TM->GetIRNode(idx);
      const auto fname = ir_helper::GetFunctionName(fnode);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing function " + STR(idx) + " " + fname);
      const auto* fd = GetPointerS<function_val_node>(fnode);
      const auto type = ir_helper::PrintType(fd->type);
      if(fd->body)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---FunctionTypeString " + type);
      }
      typeToDeclaration[type].insert(idx);
   }
   already_visited.clear();
}

DesignFlowStep_Status CallGraphBuiltinCall::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->BuiltinWaitCall Analysis");
   ir_managerRef TM = AppM->get_ir_manager();
   auto* functionDecl = GetPointer<function_val_node>(TM->GetIRNode(function_id));
   auto* stmtList = GetPointer<statement_list_node>(functionDecl->body);

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      AppM->CGetCallGraphManager().GetCallGraph().writeDot(
          parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
          ("builtin-graph-pre" + STR(function_id) + ".dot"));
   }
   modified = false;

   for(const auto& block : stmtList->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         if(stmt->get_kind() == call_stmt_K || stmt->get_kind() == assign_stmt_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing stmt " + stmt->ToString());
            lookForBuiltinCall(stmt);
         }
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      AppM->CGetCallGraphManager().GetCallGraph().writeDot(
          parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
          ("builtin-graph-post" + STR(function_id) + ".dot"));
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--BuiltinWaitCall Analysis completed");
   // TODO: IR is not really modified, it is just the call graph being updated
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void CallGraphBuiltinCall::ExtendCallGraph(unsigned int callerIdx, const ir_nodeConstRef& ftype, unsigned int stmtIdx)
{
   const auto type = ir_helper::PrintType(ftype);
   for(unsigned int Itr : typeToDeclaration[type])
   {
      CallGraphManager::addCallPointAndExpand(already_visited, AppM, callerIdx, Itr, stmtIdx,
                                              FunctionEdgeInfo::CallType::indirect_call, debug_level);
      modified = true;
   }
}

void CallGraphBuiltinCall::lookForBuiltinCall(const ir_nodeRef& tn)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Update recursively node: " + STR(tn) + " id: " + STR(tn->index));

   const auto TM = AppM->get_ir_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + tn->get_kind_text());
   switch(tn->get_kind())
   {
      case call_node_K:
      {
         const auto* ce = GetPointer<call_node>(tn);

         const auto fn = GetPointer<addr_node>(ce->fn) ? GetPointer<addr_node>(ce->fn)->op : ce->fn;
         THROW_ASSERT(fn, "Address expression with null op");
         if(fn->get_kind() == function_val_node_K)
         {
            const auto funName = ir_helper::GetFunctionName(fn);
            if(funName == BUILTIN_WAIT_CALL)
            {
               const auto builtinArgZero =
                   GetPointer<addr_node>(ce->args[0]) ? GetPointer<addr_node>(ce->args[0])->op : ce->args[0];
               if(builtinArgZero->get_kind() == function_val_node_K)
               {
                  CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, builtinArgZero->index,
                                                          tn->index, FunctionEdgeInfo::CallType::indirect_call,
                                                          debug_level);
                  modified = true;
               }
               else if(builtinArgZero->get_kind() == ssa_node_K)
               {
                  const auto funPtrType = getFunctionPointerType(builtinArgZero);
                  ExtendCallGraph(function_id, funPtrType, ce->index);
               }
            }
         }
         break;
      }
      case call_stmt_K:
      {
         const auto* gc = GetPointerS<call_stmt>(tn);

         const auto fn = GetPointer<addr_node>(gc->fn) ? GetPointer<addr_node>(gc->fn)->op : gc->fn;
         THROW_ASSERT(fn, "Address expression with null op");
         if(fn->get_kind() == function_val_node_K)
         {
            const auto funName = ir_helper::GetFunctionName(fn);
            if(funName == BUILTIN_WAIT_CALL)
            {
               const auto builtinArgZero =
                   GetPointer<addr_node>(gc->args[0]) ? GetPointer<addr_node>(gc->args[0])->op : gc->args[0];
               if(builtinArgZero->get_kind() == function_val_node_K)
               {
                  CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, builtinArgZero->index,
                                                          tn->index, FunctionEdgeInfo::CallType::indirect_call,
                                                          debug_level);
                  modified = true;
               }
               else if(builtinArgZero->get_kind() == ssa_node_K)
               {
                  // Function pointers case.
                  const auto funPtrType = getFunctionPointerType(builtinArgZero);
                  ExtendCallGraph(function_id, funPtrType, gc->index);
               }
            }
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto* gm = GetPointerS<assign_stmt>(tn);
         if(GetPointer<call_node>(gm->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---call_node " + STR(gm->op1));
            lookForBuiltinCall(gm->op1);
         }
         break;
      }
      case constructor_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      case phi_stmt_K:
      case return_stmt_K:
      case identifier_node_K:
      case constant_int_val_node_K:
      case lut_node_K:
      case argument_val_node_K:
      case constant_fp_val_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case constant_vector_val_node_K:
      case CASE_BINARY_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively " + STR(tn->index));
}
