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
 * @file var_computation.cpp
 * @brief Analyzes operations and creates the sets of read and written variables.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "var_computation.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp"

#include <fstream>

#define TOSTRING(id) std::to_string(id)

VarComputation::VarComputation(const ParameterConstRef _parameters, const application_managerRef _AppM,
                               unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, VAR_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
VarComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void VarComputation::Initialize()
{
   if(bb_version != 0 && bb_version != function_behavior->GetBBVersion())
   {
      auto mod_cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
      if(mod_cfg.num_vertices() != 0)
      {
         for(const auto op : mod_cfg.vertices())
         {
            auto& op_node_info = mod_cfg.GetNodeInfo(op);
            op_node_info.cited_variables.clear();
            op_node_info.actual_parameters.clear();
            op_node_info.Initialize();
         }
      }
   }
}

DesignFlowStep_Status VarComputation::InternalExec()
{
   const auto cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
   const auto& ogc = function_behavior->ogc;
   std::list<OpGraph::vertex_descriptor> Vertices;
   for(const auto& v : cfg.vertices())
   {
      Vertices.push_back(v);
   }
   std::list<OpGraph::vertex_descriptor> PhiNodes;
   for(auto Ver = Vertices.begin(); Ver != Vertices.end();)
   {
      auto curr_Ver = Ver;
      ++Ver;
      if(cfg.CGetNodeInfo(*curr_Ver).node_type == TYPE_VPHI)
      {
         PhiNodes.push_back(*curr_Ver);
         Vertices.erase(curr_Ver);
      }
   }
   for(const auto Ver : Vertices)
   {
      const auto& node = cfg.CGetNodeInfo(Ver).node;
      if(node)
      {
         RecursivelyAnalyze(Ver, node, VariableAccessType::UNKNOWN, ogc);
      }
   }
   for(const auto& PhiNode : PhiNodes)
   {
      const auto& node = cfg.CGetNodeInfo(PhiNode).node;
      if(node)
      {
         RecursivelyAnalyze(PhiNode, node, VariableAccessType::UNKNOWN, ogc);
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      cfg.writeDot(function_behavior->GetDotPath() / "OP_Variables.dot", 1);
   }
   return DesignFlowStep_Status::SUCCESS;
}

void VarComputation::RecursivelyAnalyze(OpGraph::vertex_descriptor op_vertex, const ir_nodeConstRef& _ir_node,
                                        const VariableAccessType access_type,
                                        const std::unique_ptr<operations_graph_constructor>& ogc) const
{
   const auto ir_node = _ir_node;
   const auto node_kind = ir_node->get_kind();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing node " + STR(ir_node) + "(" + ir_node::GetString(node_kind) + ")");

   const auto gn = GetPointer<const node_stmt>(ir_node);

   if(GetPointer<const node_stmt>(ir_node) && (gn->vuses.size() || gn->vdef))
   {
      AnalyzeVops(op_vertex, GetPointer<const node_stmt>(ir_node), ogc);
   }

   switch(node_kind)
   {
      case nop_stmt_K:
         break;
      case assign_stmt_K:
      {
         const auto* ga = GetPointerS<const assign_stmt>(ir_node);
         RecursivelyAnalyze(op_vertex, ga->op0, VariableAccessType::DEFINITION, ogc);
         RecursivelyAnalyze(op_vertex, ga->op1, VariableAccessType::USE, ogc);
         if(ga->predicate)
         {
            RecursivelyAnalyze(op_vertex, ga->predicate, VariableAccessType::USE, ogc);
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto* pn = GetPointerS<const phi_stmt>(ir_node);
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            RecursivelyAnalyze(op_vertex, def_edge.first, VariableAccessType::USE, ogc);
         }
         RecursivelyAnalyze(op_vertex, pn->res, VariableAccessType::DEFINITION, ogc);
         break;
      }
      case return_stmt_K:
      {
         const auto* gr = GetPointerS<const return_stmt>(ir_node);
         const auto& op = gr->op;
         if(op)
         {
            RecursivelyAnalyze(op_vertex, op, VariableAccessType::USE, ogc);
         }
         break;
      }
      case call_node_K:
      {
         const auto* ce = GetPointerS<const call_node>(ir_node);
         /// Needed to correctly support function pointers
         if(ce->fn->get_kind() == ssa_node_K)
         {
            RecursivelyAnalyze(op_vertex, ce->fn, VariableAccessType::USE, ogc);
         }
         for(const auto& arg : ce->args)
         {
            /// add parameter to the vertex
            ogc->add_parameter(op_vertex, arg->index);
            RecursivelyAnalyze(op_vertex, arg, VariableAccessType::ARG, ogc);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto* gc = GetPointer<const call_stmt>(ir_node);
         /// Needed to correctly support function pointers
         if(gc->fn->get_kind() == ssa_node_K)
         {
            RecursivelyAnalyze(op_vertex, gc->fn, VariableAccessType::USE, ogc);
         }
         for(const auto& arg : gc->args)
         {
            /// add parameter to the vertex
            ogc->add_parameter(op_vertex, arg->index);
            RecursivelyAnalyze(op_vertex, arg, VariableAccessType::ARG, ogc);
         }
         if(gc->predicate)
         {
            RecursivelyAnalyze(op_vertex, gc->predicate, VariableAccessType::USE, ogc);
         }

         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto* gmwi = GetPointerS<const multi_way_if_stmt>(ir_node);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursivelyAnalyze(op_vertex, cond.first, VariableAccessType::USE, ogc);
            }
         }
         break;
      }
      case function_val_node_K:
      {
         break;
      }
      case variable_val_node_K:
      {
         ogc->AddSourceCodeVariable(op_vertex, ir_node->index);
         const auto* vd = GetPointer<const variable_val_node>(ir_node);
         if(vd && (!vd->parent || vd->parent->get_kind() == module_unit_node_K))
         {
            AppM->AddGlobalVariable(_ir_node);
         }
         break;
      }
      case argument_val_node_K:
      {
         ogc->AddVariable(function_behavior->GetOpGraph(FunctionBehavior::CFG).CGetGraphInfo().entry_vertex,
                          ir_node->index, VariableType::SCALAR, access_type);
         ogc->AddSourceCodeVariable(op_vertex, ir_node->index);
         break;
      }
      case ssa_node_K:
      {
         const auto* sn = GetPointer<const ssa_node>(ir_node);
         if(sn->virtual_flag)
         {
            switch(access_type)
            {
               case(VariableAccessType::USE):
               case(VariableAccessType::DEFINITION):
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::VIRTUAL, access_type);
                  break;
               case(VariableAccessType::ARG):
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::VIRTUAL, VariableAccessType::USE);
                  break;
               case(VariableAccessType::ADDRESS):
               case(VariableAccessType::OVER):
                  THROW_UNREACHABLE("Address expresion of a virtual variable");
                  break;
               case(VariableAccessType::UNKNOWN):
               default:
                  THROW_UNREACHABLE("");
            }
         }
         else
         {
            if(sn->GetDefStmt()->get_kind() == nop_stmt_K && sn->var)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Adding " + sn->ToString() + " to defs of Entry");
               ogc->AddVariable(function_behavior->GetOpGraph(FunctionBehavior::CFG).CGetGraphInfo().entry_vertex,
                                ir_node->index, VariableType::SCALAR, VariableAccessType::DEFINITION);
            }
            ogc->AddSourceCodeVariable(op_vertex, ir_node->index);
            switch(access_type)
            {
               case(VariableAccessType::USE):
               case(VariableAccessType::DEFINITION):
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::SCALAR, access_type);
                  break;
               case(VariableAccessType::ARG):
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::SCALAR, VariableAccessType::USE);
                  break;
               case(VariableAccessType::ADDRESS):
                  break;
               case(VariableAccessType::OVER):
               {
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::SCALAR, VariableAccessType::USE);
                  ogc->AddVariable(op_vertex, ir_node->index, VariableType::SCALAR, VariableAccessType::DEFINITION);
                  break;
               }
               case(VariableAccessType::UNKNOWN):
               default:
                  THROW_UNREACHABLE("");
            }
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto* ue = GetPointerS<const unary_node>(ir_node);
         if(ue->get_kind() == addr_node_K)
         {
            RecursivelyAnalyze(op_vertex, ue->op, VariableAccessType::ADDRESS, ogc);
         }
         else
         {
            RecursivelyAnalyze(op_vertex, ue->op, VariableAccessType::USE, ogc);
         }
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto* be = GetPointerS<const binary_node>(ir_node);
         RecursivelyAnalyze(op_vertex, be->op0, VariableAccessType::USE, ogc);
         RecursivelyAnalyze(op_vertex, be->op1, VariableAccessType::USE, ogc);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto* te = GetPointerS<const ternary_node>(ir_node);
         RecursivelyAnalyze(op_vertex, te->op0, VariableAccessType::USE, ogc);
         RecursivelyAnalyze(op_vertex, te->op1, VariableAccessType::USE, ogc);
         if(te->op2)
         {
            RecursivelyAnalyze(op_vertex, te->op2, VariableAccessType::USE, ogc);
         }
         break;
      }
      case lut_node_K:
      {
         auto* le = GetPointerS<const lut_node>(ir_node);
         RecursivelyAnalyze(op_vertex, le->op0, VariableAccessType::USE, ogc);
         RecursivelyAnalyze(op_vertex, le->op1, VariableAccessType::USE, ogc);
         if(le->op2)
         {
            RecursivelyAnalyze(op_vertex, le->op2, VariableAccessType::USE, ogc);
         }
         if(le->op3)
         {
            RecursivelyAnalyze(op_vertex, le->op3, VariableAccessType::USE, ogc);
         }
         if(le->op4)
         {
            RecursivelyAnalyze(op_vertex, le->op4, VariableAccessType::USE, ogc);
         }
         if(le->op5)
         {
            RecursivelyAnalyze(op_vertex, le->op5, VariableAccessType::USE, ogc);
         }
         if(le->op6)
         {
            RecursivelyAnalyze(op_vertex, le->op6, VariableAccessType::USE, ogc);
         }
         if(le->op7)
         {
            RecursivelyAnalyze(op_vertex, le->op7, VariableAccessType::USE, ogc);
         }
         if(le->op8)
         {
            RecursivelyAnalyze(op_vertex, le->op8, VariableAccessType::USE, ogc);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto* constr = GetPointerS<const constructor_node>(ir_node);
         for(const auto& valu : constr->list_of_idx_valu)
         {
            RecursivelyAnalyze(op_vertex, valu.second, VariableAccessType::USE, ogc);
         }
         break;
      }
      case field_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      {
         break;
      }
      case identifier_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case statement_list_node_K:
      case CASE_TYPE_NODES:
      {
         THROW_UNREACHABLE("Unexpected IR node: " + ir_node::GetString(node_kind));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Analyzed node " + STR(ir_node) + "(" + ir_node::GetString(node_kind) + ")");
   return;
}

void VarComputation::AnalyzeVops(OpGraph::vertex_descriptor op_vertex, const node_stmt* vop,
                                 const std::unique_ptr<operations_graph_constructor>& ogc) const
{
   for(const auto& vuse : vop->vuses)
   {
      ogc->AddVariable(op_vertex, vuse->index, VariableType::VIRTUAL, VariableAccessType::USE);
   }
   if(vop->vdef)
   {
      ogc->AddVariable(op_vertex, vop->vdef->index, VariableType::VIRTUAL, VariableAccessType::DEFINITION);
   }
   for(auto const& vover : vop->vovers)
   {
      ogc->AddVariable(op_vertex, vover->index, VariableType::VIRTUAL, VariableAccessType::OVER);
   }
}
