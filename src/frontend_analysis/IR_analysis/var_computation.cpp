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
 * @file var_computation.cpp
 * @brief Analyzes operations and creates the sets of read and written variables.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_I386_GCC45_COMPILER.hpp"

/// Header include
#include "var_computation.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "boost/lexical_cast.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define TOSTRING(id) boost::lexical_cast<std::string>(id)

VarComputation::VarComputation(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, VAR_ANALYSIS, _design_flow_manager, _parameters), ogc(function_behavior->ogc), cfg(function_behavior->CGetOpGraph(FunctionBehavior::CFG)), behavioral_helper(function_behavior->GetBehavioralHelper())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

VarComputation::~VarComputation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> VarComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
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
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphRef mod_cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
      if(boost::num_vertices(*mod_cfg) != 0)
      {
         VertexIterator op, op_end;
         for(boost::tie(op, op_end) = boost::vertices(*mod_cfg); op != op_end; op++)
         {
            const OpNodeInfoRef op_node_info = mod_cfg->GetOpNodeInfo(*op);
            op_node_info->cited_variables.clear();
            op_node_info->variables.clear();
#if HAVE_EXPERIMENTAL
            op_node_info->dynamic_memory_locations.clear();
#endif
            op_node_info->actual_parameters.clear();
            op_node_info->Initialize();
         }
      }
   }
}

DesignFlowStep_Status VarComputation::InternalExec()
{
   const tree_managerConstRef tree_manager = AppM->get_tree_manager();

   VertexIterator VerIt, VerItEnd;
   std::list<vertex> Vertices;
   for(boost::tie(VerIt, VerItEnd) = boost::vertices(*cfg); VerIt != VerItEnd; VerIt++)
   {
      Vertices.push_back(*VerIt);
   }
   std::list<vertex> PhiNodes;
   for(auto Ver = Vertices.begin(); Ver != Vertices.end();)
   {
      auto curr_Ver = Ver;
      ++Ver;
      if(GET_TYPE(cfg, *curr_Ver) == TYPE_VPHI)
      {
         PhiNodes.push_back(*curr_Ver);
         Vertices.erase(curr_Ver);
      }
   }
   for(auto& Vertice : Vertices)
   {
      const unsigned int node_id = cfg->CGetOpNodeInfo(Vertice)->GetNodeId();
      if(node_id != ENTRY_ID and node_id != EXIT_ID)
      {
         RecursivelyAnalyze(Vertice, tree_manager->CGetTreeNode(node_id), FunctionBehavior_VariableAccessType::UNKNOWN);
      }
   }
   for(auto& PhiNode : PhiNodes)
   {
      const unsigned int node_id = cfg->CGetOpNodeInfo(PhiNode)->GetNodeId();
      if(node_id != ENTRY_ID and node_id != EXIT_ID)
      {
         RecursivelyAnalyze(PhiNode, tree_manager->CGetTreeNode(node_id), FunctionBehavior_VariableAccessType::UNKNOWN);
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::CFG)->WriteDot("OP_Variables.dot", 1);
   }
   return DesignFlowStep_Status::SUCCESS;
}

void VarComputation::RecursivelyAnalyze(const vertex op_vertex, const tree_nodeConstRef tree_node, const FunctionBehavior_VariableAccessType access_type) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + tree_node->ToString() + "(" + tree_node->get_kind_text() + ")");

   const auto gn = GetPointer<const gimple_node>(tree_node);

   if(GetPointer<const gimple_node>(tree_node) and (gn->memuse or gn->memdef or gn->vuses.size() or gn->vdef))
   {
      AnalyzeVops(op_vertex, GetPointer<const gimple_node>(tree_node));
   }

   switch(tree_node->get_kind())
   {
      case gimple_nop_K:
      case gimple_pragma_K:
         break;
      case gimple_assign_K:
      {
         const auto* ga = GetPointer<const gimple_assign>(tree_node);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ga->op0), FunctionBehavior_VariableAccessType::DEFINITION);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ga->op1), FunctionBehavior_VariableAccessType::USE);
         if(ga->predicate)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ga->predicate), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_phi_K:
      {
         const auto* pn = GetPointer<const gimple_phi>(tree_node);
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(def_edge.first), FunctionBehavior_VariableAccessType::USE);
         }
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(pn->res), FunctionBehavior_VariableAccessType::DEFINITION);
         break;
      }
      case gimple_return_K:
      {
         const auto* gr = GetPointer<const gimple_return>(tree_node);
         const tree_nodeConstRef op = gr->op;
         if(op)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(op), FunctionBehavior_VariableAccessType::USE);
         }
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         const auto* ce = GetPointer<const call_expr>(tree_node);
         /// Needed to correctly support function pointers
         if(GET_CONST_NODE(ce->fn)->get_kind() == ssa_name_K)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ce->fn), FunctionBehavior_VariableAccessType::USE);
         }
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            /// add parameter to the vertex
            ogc->add_parameter(op_vertex, GET_CONST_NODE(*arg)->index);
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(*arg), FunctionBehavior_VariableAccessType::ARG);
         }

         break;
      }

      case gimple_call_K:
      {
         const auto* gc = GetPointer<const gimple_call>(tree_node);
         /// Needed to correctly support function pointers
         if(GET_CONST_NODE(gc->fn)->get_kind() == ssa_name_K)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(gc->fn), FunctionBehavior_VariableAccessType::USE);
         }
         const std::vector<tree_nodeRef>& args = gc->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            /// add parameter to the vertex
            ogc->add_parameter(op_vertex, GET_CONST_NODE(*arg)->index);
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(*arg), FunctionBehavior_VariableAccessType::ARG);
         }

         break;
      }
      case gimple_cond_K:
      {
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(GetPointer<const gimple_cond>(tree_node)->op0), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_while_K:
      {
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(GetPointer<const gimple_while>(tree_node)->op0), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_for_K:
      {
         const auto* fe = GetPointer<const gimple_for>(tree_node);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(fe->op0), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(fe->op1), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(fe->op2), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto* gmwi = GetPointer<const gimple_multi_way_if>(tree_node);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursivelyAnalyze(op_vertex, GET_CONST_NODE(cond.first), FunctionBehavior_VariableAccessType::USE);
            }
         }
         break;
      }
      case gimple_switch_K:
      {
         const auto* gs = GetPointer<const gimple_switch>(tree_node);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(gs->op0), FunctionBehavior_VariableAccessType::USE);
         if(gs->op1)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(gs->op1), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_label_K:
      {
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(GetPointer<const gimple_label>(tree_node)->op), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_goto_K:
      {
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(GetPointer<const gimple_goto>(tree_node)->op), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case gimple_asm_K:
      {
         const auto* asme = GetPointer<const gimple_asm>(tree_node);
         if(asme->out)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(asme->out), FunctionBehavior_VariableAccessType::DEFINITION);
         if(asme->in)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(asme->in), FunctionBehavior_VariableAccessType::USE);
         if(asme->clob)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(asme->clob), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case function_decl_K:
      {
         break;
      }
      case var_decl_K:
      {
#if HAVE_I386_GCC45_COMPILER
         if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC45)
         {
            if(tree_helper::is_volatile(AppM->get_tree_manager(), tree_node->index))
            {
               THROW_ERROR("Volatile variables or types not supported with gcc 4.5");
            }
         }
#endif
         ogc->AddSourceCodeVariable(op_vertex, tree_node->index);
         const auto* vd = GetPointer<const var_decl>(tree_node);
         if(vd and (not vd->scpe or GET_CONST_NODE(vd->scpe)->get_kind() == translation_unit_decl_K))
            AppM->add_global_variable(vd->index);
         break;
      }
      case parm_decl_K:
      {
         ogc->AddVariable(cfg->CGetOpGraphInfo()->entry_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, access_type);
         ogc->AddSourceCodeVariable(op_vertex, tree_node->index);
         break;
      }
      case result_decl_K:
      {
         ogc->AddSourceCodeVariable(op_vertex, tree_node->index);
         break;
      }
      case ssa_name_K:
      {
         const auto* sn = GetPointer<const ssa_name>(tree_node);
         if(sn->virtual_flag)
         {
            switch(access_type)
            {
               case(FunctionBehavior_VariableAccessType::USE):
               case(FunctionBehavior_VariableAccessType::DEFINITION):
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::VIRTUAL, access_type);
                  break;
               case(FunctionBehavior_VariableAccessType::ARG):
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::VIRTUAL, FunctionBehavior_VariableAccessType::USE);
                  break;
               case(FunctionBehavior_VariableAccessType::ADDRESS):
               case(FunctionBehavior_VariableAccessType::OVER):
                  THROW_UNREACHABLE("Address expresion of a virtual variable");
                  break;
               case(FunctionBehavior_VariableAccessType::UNKNOWN):
               default:
                  THROW_UNREACHABLE("");
            }
         }
         else
         {
            if((sn->volatile_flag or (sn->CGetDefStmts().size() == 1 and GET_NODE(sn->CGetDefStmt())->get_kind() == gimple_nop_K)) and sn->var)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + sn->ToString() + " to defs of Entry");
               ogc->AddVariable(cfg->CGetOpGraphInfo()->entry_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
            }
            ogc->AddSourceCodeVariable(op_vertex, tree_node->index);
            switch(access_type)
            {
               case(FunctionBehavior_VariableAccessType::USE):
               case(FunctionBehavior_VariableAccessType::DEFINITION):
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, access_type);
                  break;
               case(FunctionBehavior_VariableAccessType::ARG):
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
                  break;
               case(FunctionBehavior_VariableAccessType::ADDRESS):
                  break;
               case(FunctionBehavior_VariableAccessType::OVER):
               {
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
                  ogc->AddVariable(op_vertex, tree_node->index, FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
                  break;
               }
               case(FunctionBehavior_VariableAccessType::UNKNOWN):
               default:
                  THROW_UNREACHABLE("");
            }
         }
         break;
      }
      case tree_list_K:
      {
         const auto* tl = GetPointer<const tree_list>(tree_node);
         if(tl->purp)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tl->purp), FunctionBehavior_VariableAccessType::USE);
         tree_nodeConstRef current_args = tree_node;
         while(current_args)
         {
            const auto* current_tree_list = GetPointer<const tree_list>(current_args);
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(current_tree_list->valu), access_type);
            if(current_tree_list->chan)
               current_args = GET_CONST_NODE(current_tree_list->chan);
            else
               break;
         }
         break;
      }
      case tree_vec_K:
      {
         const auto* tv = GetPointer<const tree_vec>(tree_node);
         const std::vector<tree_nodeRef>& list_of_op = tv->list_of_op;
         const std::vector<tree_nodeRef>::const_iterator op_end = list_of_op.end();
         for(auto op = list_of_op.begin(); op != op_end; ++op)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(*op), access_type);
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto* ue = GetPointer<const unary_expr>(tree_node);
         if(ue->get_kind() == addr_expr_K)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ue->op), FunctionBehavior_VariableAccessType::ADDRESS);
         }
         else
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(ue->op), FunctionBehavior_VariableAccessType::USE);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto* be = GetPointer<const binary_expr>(tree_node);
         if(be->get_kind() == postincrement_expr_K or be->get_kind() == postdecrement_expr_K)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(be->op0), FunctionBehavior_VariableAccessType::DEFINITION);
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(be->op0), FunctionBehavior_VariableAccessType::OVER);
         }
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(be->op0), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(be->op1), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto* te = GetPointer<const ternary_expr>(tree_node);
         /// GCC 4.5 plugin does not writer vuse for component ref of volatile variable
#if HAVE_I386_GCC45_COMPILER
         if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC45)
         {
            if(te->get_kind() == component_ref_K)
            {
               if(tree_helper::is_volatile(AppM->get_tree_manager(), te->op0->index))
               {
                  THROW_ERROR("Accessing field of volatile union/struct not supported with gcc 4.5");
               }
            }
         }
#endif

         if(te->get_kind() == component_ref_K or te->get_kind() == bit_field_ref_K)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(te->op0), access_type);
         else
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(te->op0), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(te->op1), FunctionBehavior_VariableAccessType::USE);
         if(te->op2)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(te->op2), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto* qe = GetPointer<const quaternary_expr>(tree_node);
         const tree_nodeConstRef first_operand = GET_CONST_NODE(qe->op0);
         if((qe->get_kind() == array_ref_K or qe->get_kind() == array_range_ref_K) and tree_helper::CGetType(first_operand)->get_kind() == array_type_K)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(qe->op0), access_type);
         else
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(qe->op0), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(qe->op1), FunctionBehavior_VariableAccessType::USE);
         if(qe->op2)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(qe->op2), FunctionBehavior_VariableAccessType::USE);
         if(qe->op3)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(qe->op3), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<const lut_expr>(tree_node);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op0), FunctionBehavior_VariableAccessType::USE);
         RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op1), FunctionBehavior_VariableAccessType::USE);
         if(le->op2)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op2), FunctionBehavior_VariableAccessType::USE);
         if(le->op3)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op3), FunctionBehavior_VariableAccessType::USE);
         if(le->op4)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op4), FunctionBehavior_VariableAccessType::USE);
         if(le->op5)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op5), FunctionBehavior_VariableAccessType::USE);
         if(le->op6)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op6), FunctionBehavior_VariableAccessType::USE);
         if(le->op7)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op7), FunctionBehavior_VariableAccessType::USE);
         if(le->op8)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(le->op8), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case target_mem_ref_K:
      {
         const auto* tm = GetPointer<const target_mem_ref>(tree_node);
         if(tm->symbol)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->symbol), access_type);
         if(tm->base)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->base), FunctionBehavior_VariableAccessType::USE);
         if(tm->idx)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->idx), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case target_mem_ref461_K:
      {
         const auto* tm = GetPointer<const target_mem_ref461>(tree_node);
         if(tm->base)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->base), FunctionBehavior_VariableAccessType::USE);
         if(tm->idx)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->idx), FunctionBehavior_VariableAccessType::USE);
         if(tm->idx2)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(tm->idx2), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case constructor_K:
      {
         const auto* constr = GetPointer<const constructor>(tree_node);
         const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = constr->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator valu, valu_end = list_of_idx_valu.end();
         for(valu = list_of_idx_valu.begin(); valu != valu_end; ++valu)
         {
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(valu->second), FunctionBehavior_VariableAccessType::USE);
         }
         break;
      }
      case case_label_expr_K:
      {
         const auto* cle = GetPointer<const case_label_expr>(tree_node);
         if(cle->op0)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(cle->op0), FunctionBehavior_VariableAccessType::USE);
         if(cle->op1)
            RecursivelyAnalyze(op_vertex, GET_CONST_NODE(cle->op1), FunctionBehavior_VariableAccessType::USE);
         break;
      }
      case complex_cst_K:
      case const_decl_K:
      case field_decl_K:
      case integer_cst_K:
      case label_decl_K:
      case namespace_decl_K:
      case template_decl_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case type_decl_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case issue_pragma_K:
      case blackbox_pragma_K:
      case profiling_pragma_K:
      case statistical_profiling_K:
      case map_pragma_K:
      case call_hw_pragma_K:
      case call_point_hw_pragma_K:
      case omp_pragma_K:
      case null_node_K:
      case omp_atomic_pragma_K:
      case omp_critical_pragma_K:
      case omp_declare_simd_pragma_K:
      case omp_for_pragma_K:
      case omp_parallel_pragma_K:
      case omp_sections_pragma_K:
      case omp_simd_pragma_K:
      case omp_parallel_sections_pragma_K:
      case omp_section_pragma_K:
      case omp_task_pragma_K:
      case omp_target_pragma_K:
      case statement_list_K:
      case CASE_TYPE_NODES:
      case target_expr_K:
      {
         THROW_UNREACHABLE("Unexpected tree node: " + tree_node->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + tree_node->ToString() + "(" + tree_node->get_kind_text() + ")");
   return;
}

void VarComputation::AnalyzeVops(const vertex op_vertex, const gimple_node* vop) const
{
   if(vop->memuse)
   {
      ogc->AddVariable(op_vertex, GET_INDEX_NODE(vop->memuse), FunctionBehavior_VariableType::MEMORY, FunctionBehavior_VariableAccessType::USE);
   }
   if(vop->memdef)
   {
      ogc->AddVariable(op_vertex, GET_INDEX_NODE(vop->memdef), FunctionBehavior_VariableType::MEMORY, FunctionBehavior_VariableAccessType::DEFINITION);
      if(vop->memuse)
      {
         ogc->AddVariable(op_vertex, GET_INDEX_NODE(vop->memuse), FunctionBehavior_VariableType::MEMORY, FunctionBehavior_VariableAccessType::OVER);
      }
   }
   if(vop->vuses.size())
   {
      for(auto vuse : vop->vuses)
      {
         ogc->AddVariable(op_vertex, GET_INDEX_NODE(vuse), FunctionBehavior_VariableType::VIRTUAL, FunctionBehavior_VariableAccessType::USE);
      }
   }
   if(vop->vdef)
   {
      ogc->AddVariable(op_vertex, GET_INDEX_NODE(vop->vdef), FunctionBehavior_VariableType::VIRTUAL, FunctionBehavior_VariableAccessType::DEFINITION);
   }
   if(vop->vovers.size())
   {
      for(auto const& vover : vop->vovers)
      {
         ogc->AddVariable(op_vertex, GET_INDEX_NODE(vover), FunctionBehavior_VariableType::VIRTUAL, FunctionBehavior_VariableAccessType::OVER);
      }
   }
}
