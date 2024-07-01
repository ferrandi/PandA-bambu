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
 * @file operations_cfg_computation.cpp
 * @brief Analysis step creating the control flow graph of the operations
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "operations_cfg_computation.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

operations_cfg_computation::operations_cfg_computation(const ParameterConstRef _parameters,
                                                       const application_managerRef _AppM, unsigned int _function_id,
                                                       const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OPERATIONS_CFG_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

operations_cfg_computation::~operations_cfg_computation() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
operations_cfg_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            relationships.insert(std::make_pair(EXTRACT_OMP_ATOMIC, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(LUT_TRANSFORMATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
         relationships.insert(std::make_pair(COND_EXPR_RESTRUCTURING, SAME_FUNCTION));
         relationships.insert(std::make_pair(VECTORIZE, SAME_FUNCTION));
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
      const BBGraphRef basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
      if(boost::num_vertices(*basic_block_graph) != 0)
      {
         VertexIterator basic_block, basic_block_end;
         for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph);
             basic_block != basic_block_end; basic_block++)
         {
            basic_block_graph->GetBBNodeInfo(*basic_block)->statements_list.clear();
         }
      }
   }
}

DesignFlowStep_Status operations_cfg_computation::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const BBGraphRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   VertexIterator v_iter, v_iter_end;
   const BehavioralHelperConstRef helper = function_behavior->CGetBehavioralHelper();
   const operations_graph_constructorRef& ogc = function_behavior->ogc;
   const BasicBlocksGraphConstructorRef bbgc = function_behavior->bbgc;
   const auto& root_functions = AppM->CGetCallGraphManager()->GetRootFunctions();

   /// entry and exit computation
   clean_start_nodes();
   ogc->AddOperation(TM, ENTRY, ENTRY, BB_ENTRY, 0);
   bbgc->add_operation_to_bb(ogc->getIndex(ENTRY), BB_ENTRY);
   ogc->add_type(ENTRY, TYPE_ENTRY);

   ogc->AddOperation(TM, EXIT, EXIT, BB_EXIT, 0);
   bbgc->add_operation_to_bb(ogc->getIndex(EXIT), BB_EXIT);
   ogc->add_type(EXIT, TYPE_EXIT);

   std::string res;
   std::string f_name = helper->get_function_name() + "_" + STR(function_id);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing label map");
   /// first cycle to compute the label_decl_map and the first_statement maps.
   for(boost::tie(v_iter, v_iter_end) = boost::vertices(*fbb); v_iter != v_iter_end; ++v_iter)
   {
      THROW_ASSERT(fbb->CGetBBGraphInfo()->exit_vertex, "Exit basic block not set");
      if(*v_iter == fbb->CGetBBGraphInfo()->exit_vertex)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping exit vertex");
         continue;
      }
      const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(*v_iter);
      const auto block = bb_node_info->block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining BB" + STR(block->number));
      if(block->CGetStmtList().empty() and *v_iter != fbb->CGetBBGraphInfo()->entry_vertex and
         *v_iter != fbb->CGetBBGraphInfo()->exit_vertex)
      {
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
         const auto new_tree_node_id = TM->new_tree_node_id();
         gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
         gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
         TM->create_tree_node(new_tree_node_id, gimple_nop_K, gimple_nop_schema);
         auto gn = GetPointer<gimple_nop>(TM->GetTreeNode(new_tree_node_id));
         gn->bb_index = block->number;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Created gimple_nop " + TM->GetTreeNode(new_tree_node_id)->ToString());
         block->PushBack(TM->GetTreeNode(new_tree_node_id), AppM);
      }
      if(block->CGetStmtList().size())
      {
         auto front = block->CGetStmtList().front();
         if(front->get_kind() == gimple_label_K)
         {
            auto* le = GetPointer<gimple_label>(front);
            THROW_ASSERT(le->op, "Label expr without object");
            res = get_first_node(front, f_name);
            label_decl_map[le->op->index] = res;
         }
         else if(block->CGetPhiList().empty())
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
      else if(*v_iter == fbb->CGetBBGraphInfo()->entry_vertex)
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
   tree_nodeRef last_instruction;
   for(boost::tie(v_iter, v_iter_end) = boost::vertices(*fbb); v_iter != v_iter_end; ++v_iter)
   {
      if(/* *v_iter == fbb->CGetBBGraphInfo()->entry_vertex || */ *v_iter == fbb->CGetBBGraphInfo()->exit_vertex)
      {
         continue;
      }
      const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(*v_iter);
      const auto block = bb_node_info->block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Building operation of basic block BB" + STR(block->number));
      bool skip_first_stmt = false;
      if(block->CGetStmtList().size())
      {
         const auto front = block->CGetStmtList().front();
         if(front->get_kind() == gimple_label_K)
         {
            actual_name = get_first_node(front, f_name);
            THROW_ASSERT(actual_name == first_statement[block->number],
                         "the name of the first vertice has to be the same of the label expression vertex");
            if(!empty_start_nodes())
            {
               connect_start_nodes(ogc, actual_name);
            }
            init_start_nodes(actual_name);
            build_operation_recursive(TM, ogc, front, f_name, block->number);
            bbgc->add_operation_to_bb(ogc->getIndex(actual_name), block->number);
            skip_first_stmt = true;
         }
      }
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
      if(skip_first_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped first statement");
         s++;
      }
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
            last_instruction = tree_nodeRef();
         }
      }
      else if(block->CGetStmtList().size() == 1 && skip_first_stmt)
      {
         if(block->CGetPhiList().size())
         {
            last_instruction = block->CGetPhiList().back();
         }
         else
         {
            last_instruction = block->CGetStmtList().back();
         }
      }
      else
      {
         last_instruction = block->CGetStmtList().back();
      }
      if(last_instruction && last_instruction->get_kind() != gimple_switch_K)
      {
         init_start_nodes(get_first_node(last_instruction, f_name));
      }
      else if(!last_instruction)
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
               if(successor == block->true_edge)
               {
                  connect_start_nodes(ogc, first_statement[successor], true, false);
               }
               else if(successor == block->false_edge)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Successor is on the false edge");
                  THROW_ASSERT(first_statement.find(successor) != first_statement.end(),
                               "First statement of successor BB" + STR(successor) + " not found");
                  connect_start_nodes(ogc, first_statement[successor], false, true);
               }
               else if(last_instruction && GetPointer<gimple_goto>(last_instruction) && block->list_of_succ.size() > 1)
               {
                  connect_start_nodes(ogc, first_statement[successor], true, true, successor);
               }
               else
               {
                  connect_start_nodes(ogc, first_statement[successor]);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered successor BB" + STR(successor));
         }
      }
      clean_start_nodes();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   label_decl_map.clear();
   first_statement.clear();
   clean_start_nodes();

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::CFG)->WriteDot("OP_CFG_Cleaned.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}

std::string operations_cfg_computation::get_first_node(const tree_nodeRef& tn, const std::string& f_name) const
{
   auto ind = tn->index;
   std::string src;
   src = f_name + "_" + STR(ind);

   switch(tn->get_kind())
   {
      case CASE_GIMPLE_NODES:
         return src;
      case case_label_expr_K:
      {
         auto* cle = GetPointer<case_label_expr>(tn);
         ind = cle->got->index;
         THROW_ASSERT(label_decl_map.find(ind) != label_decl_map.end(), "Label " + STR(ind) + " doesn't exist");
         return label_decl_map.find(ind)->second;
      }
      case binfo_K:
      case block_K:
      case constructor_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      case lut_expr_K:
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

void operations_cfg_computation::connect_start_nodes(const operations_graph_constructorRef ogc, const std::string& next,
                                                     bool true_edge, bool false_edge, unsigned int nodeid)
{
   const auto& root_functions = AppM->CGetCallGraphManager()->GetRootFunctions();
   for(const auto& Start_node : start_nodes)
   {
      /// Mark first operation of the application
      if(root_functions.count(function_id) && Start_node == ENTRY)
      {
         ogc->add_type(next, TYPE_FIRST_OP);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge from " + Start_node + " to " + next);
      ogc->AddEdge(ogc->getIndex(Start_node), ogc->getIndex(next), CFG_SELECTOR);
      if(true_edge && false_edge)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding label " + STR(nodeid));
         ogc->add_edge_info(ogc->getIndex(Start_node), ogc->getIndex(next), CFG_SELECTOR, nodeid);
      }
      if(true_edge && !false_edge)
      {
         ogc->add_edge_info(ogc->getIndex(Start_node), ogc->getIndex(next), CFG_SELECTOR, T_COND);
      }
      if(false_edge && !true_edge)
      {
         ogc->add_edge_info(ogc->getIndex(Start_node), ogc->getIndex(next), CFG_SELECTOR, F_COND);
      }
   }
}

void operations_cfg_computation::build_operation_recursive(const tree_managerRef TM,
                                                           const operations_graph_constructorRef ogc,
                                                           const tree_nodeRef tn, const std::string& f_name,
                                                           unsigned int bb_index)
{
   const auto curr_tn = tn;
   const auto ind = tn->index;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Building CFG of node " + STR(ind) + " of type " + curr_tn->get_kind_text());
   switch(curr_tn->get_kind())
   {
      case gimple_return_K:
      {
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_RET);
         break;
      }
      case gimple_assign_K:
      {
         const auto me = GetPointerS<const gimple_assign>(curr_tn);
         const auto op0_kind = me->op0->get_kind();
         const auto op1_kind = me->op1->get_kind();
         const auto op0_type = tree_helper::CGetType(me->op0);
         const auto op1_type = tree_helper::CGetType(me->op1);
         const auto& fun_mem_data = function_behavior->get_function_mem();

         const auto load_candidate = tree_helper::IsLoad(tn, fun_mem_data);
         const auto store_candidate = tree_helper::IsStore(tn, fun_mem_data);

         if(!me->clobber && !tree_helper::IsVectorType(me->op0) &&
            ((((tree_helper::IsArrayEquivType(me->op0) && !tree_helper::IsPointerType(me->op0))) ||
              op1_kind == constructor_K)))
         {
            if(!tree_helper::IsArrayEquivType(me->op0) ||
               (((op1_kind == constructor_K || (op1_kind == var_decl_K && GetPointerS<const var_decl>(me->op1)->init) ||
                  op1_kind == string_cst_K)) &&
                (GetPointer<const decl_node>(me->op0) || op0_kind == ssa_name_K)))
            {
               function_behavior->GetBehavioralHelper()->add_initialization(me->op0->index, me->op1->index);
               ogc->add_type(actual_name, TYPE_INIT);
            }
         }

         if(me->init_assignment || me->clobber)
         {
            ogc->AddOperation(TM, actual_name, NOP, bb_index, me->index);
            ogc->add_type(actual_name, TYPE_NOP);
         }
         else if(op1_kind == float_expr_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as float_expr_xx_to_xxx operation");
            const auto fe = GetPointerS<const float_expr>(me->op1);
            auto size_dest = tree_helper::Size(fe->type);
            auto size_from = tree_helper::Size(fe->op);
            if(size_from < 32)
            {
               size_from = 32;
            }
            else if(size_from > 32 && size_from < 64)
            {
               size_from = 64;
            }
            ogc->AddOperation(TM, actual_name, FLOAT_EXPR + STR("_") + STR(size_from) + "_to_" + STR(size_dest),
                              bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(op1_kind == fix_trunc_expr_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as fix_trunc_expr_xx_to_xxx operation");
            const auto fte = GetPointerS<const fix_trunc_expr>(me->op1);
            auto size_dest = tree_helper::Size(fte->type);
            const auto is_unsigned = tree_helper::IsUnsignedIntegerType(fte->type);
            const auto size_from = tree_helper::Size(fte->op);
            if(size_dest < 32)
            {
               size_dest = 32;
            }
            else if(size_dest > 32 && size_dest < 64)
            {
               size_dest = 64;
            }
            ogc->AddOperation(TM, actual_name,
                              FIX_TRUNC_EXPR + STR("_") + STR(size_from) + "_to_" + (is_unsigned ? "u" : "") +
                                  STR(size_dest),
                              bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(tree_helper::IsVectorType(me->op0) && op1_kind == constructor_K)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - set as VECTOR CONCATENATION operation");
            const auto co = GetPointerS<const constructor>(me->op1);
            const auto size_obj = tree_helper::SizeAlloc(co->type);
            const auto n_byte = size_obj / 8;
            ogc->AddOperation(TM, actual_name,
                              VECT_CONCATENATION + STR("_") + STR(n_byte) + "_" + STR(co->list_of_idx_valu.size()),
                              bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(op1_kind == complex_expr_K)
         {
            ogc->AddOperation(TM, actual_name, tree_node::GetString(op1_kind), bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(!store_candidate && (op0_kind == realpart_expr_K || op0_kind == imagpart_expr_K))
         {
            ogc->AddOperation(TM, actual_name, tree_node::GetString(op0_kind) + "_write", bb_index, me->index);
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else if(op0_type && op1_type && me->op1->get_kind() != insertvalue_expr_K &&
                 me->op1->get_kind() != extractvalue_expr_K &&
                 ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K &&
                   op1_kind != view_convert_expr_K) ||
                  (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K &&
                   op1_kind != view_convert_expr_K) ||
                  (op0_type->get_kind() == array_type_K) ||
                  (fun_mem_data.count(me->op0->index) && fun_mem_data.count(me->op1->index)) ||
                  (fun_mem_data.count(me->op0->index) && load_candidate) ||
                  (store_candidate && fun_mem_data.count(me->op1->index))))
         {
            if(op1_kind == constructor_K && GetPointer<const constructor>(me->op1) &&
               GetPointer<const constructor>(me->op1)->list_of_idx_valu.empty())
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
         if(me->predicate)
         {
            ogc->add_type(actual_name, TYPE_PREDICATED);
         }
         break;
      }
      case gimple_pragma_K:
      case gimple_nop_K:
      {
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_NOP);
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(curr_tn);
         const function_decl* fd = nullptr;

         if(ce->fn->get_kind() == addr_expr_K)
         {
            const auto ue = GetPointerS<const unary_expr>(ce->fn);
            fd = GetPointerS<const function_decl>(ue->op);
         }
         else if(ce->fn->get_kind() == obj_type_ref_K)
         {
            const auto fu_node = tree_helper::find_obj_type_ref_function(ce->fn);
            fd = GetPointerS<const function_decl>(fu_node);
         }
         if(fd)
         {
            std::string fun_name = tree_helper::print_function_name(TM, fd);
            fun_name = tree_helper::NormalizeTypename(fun_name);
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
#if HAVE_FROM_PRAGMA_BUILT
            if(fd->omp_atomic)
            {
               ogc->add_type(actual_name, TYPE_ATOMIC);
            }
#endif
            ogc->add_called_function(actual_name, fd->index);
         }
         else
         {
            // Call of not an usual function decl (for example array_ref of an array of function pointer)
            // This call is needed to set the basic block index of this operation. Otherwise
            // vertex is created but basic block index is not setted
            ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(curr_tn);
         const function_decl* fd = nullptr;

         if(ce->fn->get_kind() == addr_expr_K)
         {
            const auto ue = GetPointerS<const unary_expr>(ce->fn);
            fd = GetPointerS<const function_decl>(ue->op);
         }
         else if(ce->fn->get_kind() == obj_type_ref_K)
         {
            const auto fu_node = tree_helper::find_obj_type_ref_function(ce->fn);
            fd = GetPointerS<const function_decl>(fu_node);
         }
         if(fd)
         {
            if(tree_helper::is_a_nop_function_decl(fd))
            {
               ogc->AddOperation(TM, actual_name, NOP, bb_index, ce->index);
               ogc->add_type(actual_name, TYPE_NOP);
            }
            else
            {
               std::string fun_name = tree_helper::print_function_name(TM, fd);
               fun_name = tree_helper::NormalizeTypename(fun_name);
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
#if HAVE_FROM_PRAGMA_BUILT
               if(fd->omp_atomic)
               {
                  ogc->add_type(actual_name, TYPE_ATOMIC);
               }
#endif
               ogc->add_called_function(actual_name, fd->index);
            }
         }
         else
         {
            // Call of not an usual function decl (for example array_ref of an array of function pointer)
            // This call is needed to set the basic block index of this operation. Otherwise
            // vertex is created but basic block index is not setted
            ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, ce->index);
         }
         break;
      }
      case gimple_goto_K:
      {
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_GOTO);
         break;
      }
      case gimple_label_K:
      {
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         const auto le = GetPointerS<const gimple_label>(curr_tn);
         THROW_ASSERT(le->op, "expected a gimple_label operand");
         const auto ld = GetPointerS<const label_decl>(le->op);
         if(ld->artificial_flag)
         {
            ogc->add_type(actual_name, TYPE_NOP);
         }
         else
         {
            ogc->add_type(actual_name, TYPE_LABEL);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(curr_tn);
         ogc->AddOperation(TM, actual_name, READ_COND, bb_index, gc->index);
         build_operation_recursive(TM, ogc, gc->op0, f_name, bb_index);
         ogc->add_type(actual_name, TYPE_IF);
         break;
      }
      case gimple_multi_way_if_K:
      {
         ogc->AddOperation(TM, actual_name, MULTI_READ_COND, bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_MULTIIF);
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointerS<const gimple_while>(curr_tn);
         ogc->AddOperation(TM, actual_name, READ_COND, bb_index, curr_tn->index);
         build_operation_recursive(TM, ogc, we->op0, f_name, bb_index);
         ogc->add_type(actual_name, TYPE_WHILE);
         break;
      }
      case gimple_for_K:
      {
         ogc->AddOperation(TM, actual_name, READ_COND, bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_FOR);
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(curr_tn);

         ogc->AddOperation(TM, actual_name, SWITCH_COND, bb_index, curr_tn->index);
         build_operation_recursive(TM, ogc, se->op0, actual_name, bb_index);
         ogc->add_type(actual_name, TYPE_SWITCH);

         tree_nodeConstRef case_label_exprs;
         if(se->op1)
         {
            case_label_exprs = se->op1;
         }
         else
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                             std::string("op2 in gimple_switch not yet supported (") + STR(ind) + std::string(")"));
         }
         if(case_label_exprs->get_kind() == tree_vec_K)
         {
            const auto tv = GetPointerS<const tree_vec>(case_label_exprs);
            for(const auto& i : tv->list_of_op)
            {
               const auto res = get_first_node(i, f_name);
               THROW_ASSERT(res != "", "Impossible to find first operation of case " + STR(i->index));
               if(i->get_kind() == case_label_expr_K)
               {
                  const auto cl = GetPointerS<const case_label_expr>(i);
                  if(cl->default_flag)
                  {
                     connect_start_nodes(ogc, res, true, true, default_COND);
                  }
                  else
                  {
                     connect_start_nodes(ogc, res, true, true, i->index);
                  }
               }
               else
               {
                  connect_start_nodes(ogc, res, true, true, i->index);
               }
            }
         }
         else
         {
            THROW_ERROR(std::string("expected tree_vec in op1 in gimple_switch (") + STR(ind) + std::string(")"));
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(curr_tn);
         build_operation_recursive(TM, ogc, ue->op, f_name, bb_index);
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<const binary_expr>(curr_tn);
         build_operation_recursive(TM, ogc, be->op0, f_name, bb_index);
         build_operation_recursive(TM, ogc, be->op1, f_name, bb_index);
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<const ternary_expr>(curr_tn);
         build_operation_recursive(TM, ogc, te->op0, f_name, bb_index);
         if(te->op1)
         {
            build_operation_recursive(TM, ogc, te->op1, f_name, bb_index);
         }
         if(te->op2)
         {
            build_operation_recursive(TM, ogc, te->op2, f_name, bb_index);
         }
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<const quaternary_expr>(curr_tn);
         build_operation_recursive(TM, ogc, qe->op0, f_name, bb_index);
         if(qe->op1)
         {
            build_operation_recursive(TM, ogc, qe->op1, f_name, bb_index);
         }
         if(qe->op2)
         {
            build_operation_recursive(TM, ogc, qe->op2, f_name, bb_index);
         }
         if(qe->op3)
         {
            build_operation_recursive(TM, ogc, qe->op3, f_name, bb_index);
         }
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<const lut_expr>(curr_tn);
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
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, 0);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case ssa_name_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case constructor_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case CASE_DECL_NODES:
      {
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointerS<const gimple_asm>(curr_tn);
         if(!ae->volatile_flag && (ae->in && ae->out))
         {
            ogc->add_type(actual_name, TYPE_GENERIC);
         }
         else
         {
            ogc->add_type(actual_name, TYPE_GENERIC | TYPE_OPAQUE);
         }
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         break;
      }
      case gimple_phi_K:
      {
         const auto phi = GetPointerS<const gimple_phi>(curr_tn);
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         ogc->add_type(actual_name, phi->virtual_flag ? TYPE_VPHI : TYPE_PHI);
         break;
      }
      case CASE_CPP_NODES:

      {
         ogc->AddOperation(TM, actual_name, curr_tn->get_kind_text(), bb_index, curr_tn->index);
         ogc->add_type(actual_name, TYPE_GENERIC);
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      case target_expr_K:
      case error_mark_K:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built CFG of node " + STR(ind));
}
