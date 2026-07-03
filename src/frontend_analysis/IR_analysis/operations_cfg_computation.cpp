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
 * @file operations_cfg_computation.cpp
 * @brief Analysis step creating the control flow graph of the operations
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "operations_cfg_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

operations_cfg_computation::operations_cfg_computation(const ParameterConstRef _parameters,
                                                       const application_managerRef _AppM, unsigned int _function_id,
                                                       const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OPERATIONS_CFG_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
operations_cfg_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         relationships.insert(std::make_pair(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
         relationships.insert(std::make_pair(SELECT_TREE_BALANCING, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void operations_cfg_computation::Initialize()
{
   if(bb_version != 0)
   {
      function_behavior->ogc->Clear();
      auto basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
      for(const auto& basic_block : basic_block_graph.vertices())
      {
         basic_block_graph.GetNodeInfo(basic_block).statements_list.clear();
      }
   }
}

DesignFlowStep_Status operations_cfg_computation::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto helper = function_behavior->CGetBehavioralHelper();
   const auto& ogc = function_behavior->ogc;
   const auto& bbgc = function_behavior->bbgc;
   const auto& root_functions = AppM->CGetCallGraphManager().GetRootFunctions();

   /// entry and exit computation
   clean_start_nodes();
   ogc->AddOperation(TM, ENTRY, ENTRY, BB_ENTRY, 0);
   bbgc->add_operation_to_bb(ogc->getIndex(ENTRY), BB_ENTRY);
   ogc->add_type(ENTRY, TYPE_ENTRY);

   ogc->AddOperation(TM, EXIT, EXIT, BB_EXIT, 0);
   bbgc->add_operation_to_bb(ogc->getIndex(EXIT), BB_EXIT);
   ogc->add_type(EXIT, TYPE_EXIT);

   std::string res;
   std::string f_name = helper->GetFunctionName() + "_" + STR(function_id);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing label map");
   for(const auto& v : fbb.vertices())
   {
      THROW_ASSERT(fbb.CGetGraphInfo().exit_vertex, "Exit basic block not set");
      if(v == fbb.CGetGraphInfo().exit_vertex)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping exit vertex");
         continue;
      }
      const auto& bb_node_info = fbb.CGetNodeInfo(v);
      const auto block = bb_node_info.block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining BB" + STR(block->number));
      if(block->CGetStmtList().empty() && v != fbb.CGetGraphInfo().entry_vertex && v != fbb.CGetGraphInfo().exit_vertex)
      {
         ir_manager::IRSchema nop_stmt_schema;
         nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
         nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
         auto nop_tn = TM->create_ir_node(nop_stmt_K, nop_stmt_schema);
         GetPointerS<nop_stmt>(nop_tn)->bb_index = block->number;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created nop_stmt " + nop_tn->ToString());
         block->PushBack(nop_tn, AppM);
      }
      if(block->CGetStmtList().size())
      {
         auto front = block->CGetStmtList().front();
         if(block->CGetPhiList().empty())
         {
            res = get_first_node(front, f_name);
         }
         else
         {
            res = get_first_node(block->CGetPhiList().front(), f_name);
         }
      }
      else if(block->CGetPhiList().size())
      {
         res = get_first_node(block->CGetPhiList().front(), f_name);
      }
      else if(v == fbb.CGetGraphInfo().entry_vertex)
      {
         res = ENTRY;
         first_statement[block->number] = res;
         continue;
      }
      else /*empty basic block*/
      {
         THROW_UNREACHABLE("");
      }
      first_statement[block->number] = res;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed label map");
   /// second cycle doing the real job
   ir_nodeRef last_instruction;
   for(const auto& v : fbb.vertices())
   {
      if(/* v == fbb.CGetGraphInfo().entry_vertex || */ v == fbb.CGetGraphInfo().exit_vertex)
      {
         continue;
      }
      const auto& bb_node_info = fbb.CGetNodeInfo(v);
      const auto block = bb_node_info.block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Building operation of basic block BB" + STR(block->number));
      for(const auto& phi : block->CGetPhiList())
      {
         actual_name = get_first_node(phi, f_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phi " + actual_name);
         if(!empty_start_nodes())
         {
            connect_start_nodes(ogc, actual_name);
         }
         init_start_nodes(actual_name);
         build_operation_recursive(TM, ogc, phi, f_name, block->number);
         bbgc->add_operation_to_bb(ogc->getIndex(actual_name), block->number);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phi " + actual_name);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---List of operations size " + STR(block->CGetStmtList().size()));
      auto s_end = block->CGetStmtList().end();
      auto s = block->CGetStmtList().begin();

      for(; s != s_end; s++)
      {
         actual_name = get_first_node(*s, f_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing operation " + actual_name);
         if(!empty_start_nodes())
         {
            connect_start_nodes(ogc, actual_name);
         }
         init_start_nodes(actual_name);
         build_operation_recursive(TM, ogc, *s, f_name, block->number);
         bbgc->add_operation_to_bb(ogc->getIndex(actual_name), block->number);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed operation " + actual_name);
      }
      clean_start_nodes();
      if(block->CGetStmtList().empty())
      {
         if(!block->CGetPhiList().empty())
         {
            last_instruction = block->CGetPhiList().back();
         }
         else
         {
            last_instruction = ir_nodeRef();
         }
      }
      else
      {
         last_instruction = block->CGetStmtList().back();
      }
      if(last_instruction)
      {
         init_start_nodes(get_first_node(last_instruction, f_name));
      }
      else
      {
         init_start_nodes(first_statement[block->number]);
      }
      if(block->list_of_succ.size() == 0 and root_functions.find(function_id) != root_functions.end())
      {
         std::list<std::string>::iterator operation, operation_end = start_nodes.end();
         for(operation = start_nodes.begin(); operation != operation_end; ++operation)
         {
            ogc->add_type(*operation, TYPE_LAST_OP);
         }
      }
      else
      {
         for(const auto successor : block->list_of_succ)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successor BB" + STR(successor));
            if(successor == bloc::EXIT_BLOCK_ID)
            {
               connect_start_nodes(ogc, EXIT);
               if(root_functions.find(function_id) != root_functions.end())
               {
                  std::list<std::string>::iterator operation, operation_end = start_nodes.end();
                  for(operation = start_nodes.begin(); operation != operation_end; ++operation)
                  {
                     ogc->add_type(*operation, TYPE_LAST_OP);
                  }
               }
            }
            else
            {
               connect_start_nodes(ogc, first_statement[successor]);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered successor BB" + STR(successor));
         }
      }
      clean_start_nodes();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   first_statement.clear();
   clean_start_nodes();

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetOpGraph(FunctionBehavior::CFG)
          .writeDot(function_behavior->GetDotPath() / "OP_CFG_Cleaned.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}

std::string operations_cfg_computation::get_first_node(const ir_nodeRef& tn, const std::string& f_name) const
{
   auto ind = tn->index;
   std::string src;
   src = f_name + "_" + STR(ind);

   switch(tn->get_kind())
   {
      case CASE_NODE_STMTS:
         return src;
      case constructor_node_K:
      case call_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      case lut_node_K:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                          std::string("Node not supported (") + STR(ind) + std::string("): ") + tn->get_kind_text());
   }
   return "";
}

void operations_cfg_computation::insert_start_node(const std::string& start_node)
{
   start_nodes.push_back(start_node);
}

void operations_cfg_computation::clean_start_nodes()
{
   start_nodes.clear();
}

bool operations_cfg_computation::empty_start_nodes() const
{
   return start_nodes.empty();
}

void operations_cfg_computation::init_start_nodes(const std::string& start_node)
{
   start_nodes.clear();
   start_nodes.push_back(start_node);
}

void operations_cfg_computation::connect_start_nodes(const std::unique_ptr<operations_graph_constructor>& ogc,
                                                     const std::string& next)
{
   const auto& root_functions = AppM->CGetCallGraphManager().GetRootFunctions();
   for(const auto& Start_node : start_nodes)
   {
      /// Mark first operation of the application
      if(root_functions.count(function_id) && Start_node == ENTRY)
      {
         ogc->add_type(next, TYPE_FIRST_OP);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge from " + Start_node + " to " + next);
      ogc->AddEdge(ogc->getIndex(Start_node), ogc->getIndex(next), CFG_SELECTOR);
   }
}

void operations_cfg_computation::build_operation_recursive(const ir_managerRef TM,
                                                           const std::unique_ptr<operations_graph_constructor>& ogc,
                                                           const ir_nodeRef tn, const std::string& f_name,
                                                           unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Building CFG of node " + STR(tn->index) + " of type " + tn->get_kind_text());
   switch(tn->get_kind())
   {
      case return_stmt_K:
      {
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, tn->index);
         ogc->add_type(actual_name, TYPE_RET);
         break;
      }
      case assign_stmt_K:
      {
         const auto me = GetPointerS<const assign_stmt>(tn);
         const auto op0_kind = me->op0->get_kind();
         const auto op1_kind = me->op1->get_kind();
         const auto op0_type = ir_helper::CGetType(me->op0);
         const auto op1_type = ir_helper::CGetType(me->op1);
         const auto& fun_mem_data = function_behavior->get_function_mem();

         const auto load_candidate = ir_helper::IsLoad(tn, fun_mem_data);
         const auto store_candidate = ir_helper::IsStore(tn, fun_mem_data);

         if(!ir_helper::IsVectorType(me->op0) &&
            ((((ir_helper::IsArrayEquivType(me->op0) && !ir_helper::IsPointerType(me->op0))) ||
              op1_kind == constructor_node_K)))
         {
            if(!ir_helper::IsArrayEquivType(me->op0) ||
               (((op1_kind == constructor_node_K ||
                  (op1_kind == variable_val_node_K && GetPointerS<const variable_val_node>(me->op1)->init))) &&
                (GetPointer<const decl_node>(me->op0) || op0_kind == ssa_node_K)))
            {
               function_behavior->GetBehavioralHelper()->add_initialization(me->op0->index, me->op1->index);
               ogc->add_type(actual_name, TYPE_INIT);
            }
         }

         if(op1_kind == itofp_node_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as itofp_node_xx_to_xxx operation");
            const auto fe = GetPointerS<const itofp_node>(me->op1);
            auto size_dest = ir_helper::Size(fe->type);
            auto size_from = ir_helper::Size(fe->op);
            if(size_from < 32)
            {
               size_from = 32;
            }
            else if(size_from > 32 && size_from < 64)
            {
               size_from = 64;
            }
            ogc->AddOperation(TM, actual_name, ITOFP_NODE + STR("_") + STR(size_from) + "_to_" + STR(size_dest),
                              bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(op1_kind == fptoi_node_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as fptoi_node_xx_to_xxx operation");
            const auto fte = GetPointerS<const fptoi_node>(me->op1);
            auto size_dest = ir_helper::Size(fte->type);
            const auto is_unsigned = ir_helper::IsUnsignedIntegerType(fte->type);
            const auto size_from = ir_helper::Size(fte->op);
            if(size_dest < 32)
            {
               size_dest = 32;
            }
            else if(size_dest > 32 && size_dest < 64)
            {
               size_dest = 64;
            }
            ogc->AddOperation(TM, actual_name,
                              FPTOI_NODE + STR("_") + STR(size_from) + "_to_" + (is_unsigned ? "u" : "") +
                                  STR(size_dest),
                              bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(ir_helper::IsVectorType(me->op0) && op1_kind == constructor_node_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as VECTOR CONCATENATION operation");
            const auto co = GetPointerS<const constructor_node>(me->op1);
            ogc->AddOperation(TM, actual_name, VECT_CONCATENATION "_" + STR(co->list_of_idx_valu.size()), bb_index,
                              me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(op0_type && op1_type && me->op1->get_kind() != insertvalue_node_K &&
                 me->op1->get_kind() != extractvalue_node_K &&
                 ((op0_type->get_kind() == struct_ty_node_K && op1_type->get_kind() == struct_ty_node_K &&
                   op1_kind != bitcast_node_K) ||
                  (op0_type->get_kind() == array_ty_node_K) ||
                  (fun_mem_data.count(me->op0->index) && fun_mem_data.count(me->op1->index)) ||
                  (fun_mem_data.count(me->op0->index) && load_candidate) ||
                  (store_candidate && fun_mem_data.count(me->op1->index))))
         {
            if(op1_kind == constructor_node_K && GetPointer<const constructor_node>(me->op1) &&
               GetPointer<const constructor_node>(me->op1)->list_of_idx_valu.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set as MEMSET operation");
               ogc->AddOperation(TM, actual_name, MEMSET, bb_index, me->index);
               ogc->add_type(actual_name, TYPE_GENERIC);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set as MEMCPY operation");
               ogc->AddOperation(TM, actual_name, MEMCPY, bb_index, me->index);
               ogc->add_type(actual_name, TYPE_MEMCPY);
            }
         }
         else if(store_candidate || fun_mem_data.count(me->op0->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set as STORE operation");
            ogc->AddOperation(TM, actual_name, "STORE", bb_index, me->index);
            ogc->add_type(actual_name, TYPE_STORE);
         }
         else if(load_candidate || fun_mem_data.count(me->op1->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set as LOAD operation");
            ogc->AddOperation(TM, actual_name, "LOAD", bb_index, me->index);
            ogc->add_type(actual_name, TYPE_LOAD);
         }
         else
         {
            ogc->AddOperation(TM, actual_name, ASSIGN, bb_index, me->index);
            ogc->add_type(actual_name, TYPE_ASSIGN);
            build_operation_recursive(TM, ogc, me->op1, f_name, bb_index);
         }
         break;
      }
      case nop_stmt_K:
      {
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, tn->index);
         ogc->add_type(actual_name, TYPE_NOP);
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<const call_node>(tn);
         ir_nodeRef fnode{nullptr};
         if(ce->fn->get_kind() == addr_node_K)
         {
            fnode = GetPointerS<const unary_node>(ce->fn)->op;
         }
         if(fnode && fnode->get_kind() == function_val_node_K)
         {
            const auto fd = GetPointerS<const function_val_node>(fnode);
            const auto fun_name = ir_helper::GetFunctionName(fnode);
            // const std::string builtin_prefix("__builtin_");
            // if(fun_name.find(builtin_prefix) == 0)
            //   fun_name = fun_name.substr(builtin_prefix.size());

            // Creating node of call
            ogc->AddOperation(TM, actual_name, fun_name, bb_index, 0);
            unsigned type_external = TYPE_EXTERNAL;
            if(fd->writing_memory || fd->reading_memory)
            {
               type_external = type_external | TYPE_RW;
            }
            ogc->add_type(actual_name, type_external);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set as TYPE_EXTERNAL operation");
            if(fun_name == "exit" || fun_name == "abort" || fun_name == "__builtin_exit" ||
               fun_name == "__builtin_abort")
            {
               ogc->add_type(actual_name, TYPE_LAST_OP);
            }
            ogc->add_called_function(actual_name, fd->index);
         }
         else
         {
            // Call of not an usual function decl
            // This call is needed to set the basic block index of this operation. Otherwise
            // vertex is created but basic block index is not setted
            ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, 0);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<const call_stmt>(tn);
         ir_nodeRef fnode{nullptr};
         if(ce->fn->get_kind() == addr_node_K)
         {
            fnode = GetPointerS<const unary_node>(ce->fn)->op;
         }
         if(fnode && fnode->get_kind() == function_val_node_K)
         {
            const auto fd = GetPointerS<const function_val_node>(fnode);
            if(ir_helper::is_a_nop_function_decl(fd))
            {
               ogc->AddOperation(TM, actual_name, NOP, bb_index, ce->index);
               ogc->add_type(actual_name, TYPE_NOP);
            }
            else
            {
               const auto fun_name = ir_helper::GetFunctionName(fnode);
               // const std::string builtin_prefix("__builtin_");
               // if(fun_name.find(builtin_prefix) == 0)
               //   fun_name = fun_name.substr(builtin_prefix.size());

               // Creating node of call
               ogc->AddOperation(TM, actual_name, fun_name, bb_index, ce->index);
               unsigned int type_external = TYPE_EXTERNAL;
               if(fd->writing_memory || fd->reading_memory)
               {
                  type_external = type_external | TYPE_RW;
               }
               ogc->add_type(actual_name, type_external);
               if(fun_name == "exit" || fun_name == "abort" || fun_name == "__builtin_exit" ||
                  fun_name == "__builtin_abort")
               {
                  ogc->add_type(actual_name, TYPE_LAST_OP);
               }
               ogc->add_called_function(actual_name, fd->index);
            }
         }
         else
         {
            // Call of not an usual function decl
            // This call is needed to set the basic block index of this operation. Otherwise
            // vertex is created but basic block index is not setted
            ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, ce->index);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         ogc->AddOperation(TM, actual_name, MULTI_READ_COND, bb_index, tn->index);
         ogc->add_type(actual_name, TYPE_MULTIIF);
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<const unary_node>(tn);
         build_operation_recursive(TM, ogc, ue->op, f_name, bb_index);
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<const binary_node>(tn);
         build_operation_recursive(TM, ogc, be->op0, f_name, bb_index);
         build_operation_recursive(TM, ogc, be->op1, f_name, bb_index);
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<const ternary_node>(tn);
         build_operation_recursive(TM, ogc, te->op0, f_name, bb_index);
         if(te->op1)
         {
            build_operation_recursive(TM, ogc, te->op1, f_name, bb_index);
         }
         if(te->op2)
         {
            build_operation_recursive(TM, ogc, te->op2, f_name, bb_index);
         }
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<const lut_node>(tn);
         build_operation_recursive(TM, ogc, le->op0, f_name, bb_index);
         build_operation_recursive(TM, ogc, le->op1, f_name, bb_index);
         if(le->op2)
         {
            build_operation_recursive(TM, ogc, le->op2, f_name, bb_index);
         }
         if(le->op3)
         {
            build_operation_recursive(TM, ogc, le->op3, f_name, bb_index);
         }
         if(le->op4)
         {
            build_operation_recursive(TM, ogc, le->op4, f_name, bb_index);
         }
         if(le->op5)
         {
            build_operation_recursive(TM, ogc, le->op5, f_name, bb_index);
         }
         if(le->op6)
         {
            build_operation_recursive(TM, ogc, le->op6, f_name, bb_index);
         }
         if(le->op7)
         {
            build_operation_recursive(TM, ogc, le->op7, f_name, bb_index);
         }
         if(le->op8)
         {
            build_operation_recursive(TM, ogc, le->op8, f_name, bb_index);
         }
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case ssa_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case constructor_node_K:
      case CASE_DECL_NODES:
      {
         break;
      }
      case phi_stmt_K:
      {
         const auto phi = GetPointerS<const phi_stmt>(tn);
         ogc->AddOperation(TM, actual_name, tn->get_kind_text(), bb_index, tn->index);
         ogc->add_type(actual_name, phi->virtual_flag ? TYPE_VPHI : TYPE_PHI);
         break;
      }
      case identifier_node_K:
      case statement_list_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built CFG of node " + STR(tn->index));
}
