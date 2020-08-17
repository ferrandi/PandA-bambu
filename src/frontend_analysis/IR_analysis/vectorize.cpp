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
 * @file vectorize.cpp
 * @brief This class contains the methods for vectorize loop or whole function
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "vectorize.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/loops_detection
#include "loop.hpp"
#include "loops.hpp"

/// behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
/// Skipping warnings due to operator() redefinition
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "tree_node_dup.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/algorithm/string/replace.hpp>
std::string ToString(Transformation transformation)
{
   switch(transformation)
   {
      case(NONE):
         return "None";
      case(COND_CON):
         return "Convergent Cond";
      case(COND_DIV):
         return "Divergent Cond";
      case(INC):
         return "Inc";
      case(INIT):
         return "Init";
      case(SCALAR):
         return "Scalar";
      case(SIMD):
         return "Simd";
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return "";
}

Vectorize::Vectorize(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, VECTORIZE, _design_flow_manager, _parameters), TM(_AppM->get_tree_manager()), tree_man(new tree_manipulation(TM, _parameters))
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

Vectorize::~Vectorize() = default;

void Vectorize::ClassifyLoop(const LoopConstRef loop, const size_t parallel_degree)
{
   const auto loop_id = loop->GetId();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR(loop_id));
   const auto cdg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);

   /// FIXME: the parallel degree is set to default for doall loop and not read from pragma
   const auto potential_parallel_degree = parameters->getOption<size_t>(OPT_gcc_openmp_simd);
   const size_t current_parallel_degree = parallel_degree != 0 ? parallel_degree : (loop->loop_type & DOALL_LOOP) ? potential_parallel_degree : 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop parallel degree: " + STR(current_parallel_degree));

   if(loop->loop_type & DOALL_LOOP)
   {
      const long long int iteration_number = (loop->loop_type & COUNTABLE_LOOP) ? ((loop->upper_bound + (loop->close_interval ? 1 : 0) - loop->lower_bound) / loop->increment) : 0;
      /// The number of iterations can be not multiple of the parallel degree of outer loop
      if(loop->loop_type & COUNTABLE_LOOP and iteration_number % static_cast<long long int>(potential_parallel_degree) == 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loop is parallelizable, countable and the iterations number is multiple of parallel degree");
         /// The header cannot diverge
         basic_block_divergence[loop_id] = false;

         /// The control condition which enables execution of header
         THROW_ASSERT(boost::in_degree(loop->GetHeader(), *cdg_bb_graph) == 1, "");
         InEdgeIterator ie, ie_end;
         boost::tie(ie, ie_end) = boost::in_edges(loop->GetHeader(), *cdg_bb_graph);
         auto edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
         auto source = boost::source(*ie, *cdg_bb_graph);
         THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
         const auto header_controller = std::pair<vertex, unsigned int>(source, edge_labels.size() ? *(edge_labels.begin()) : 0);

         /// Classify the remaining basic blocks
         const auto blocks = loop->get_blocks();
         for(const auto block : blocks)
         {
            boost::tie(ie, ie_end) = boost::in_edges(block, *cdg_bb_graph);
            edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
            source = boost::source(*ie, *cdg_bb_graph);
            THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
            const auto current_controller = std::pair<vertex, unsigned int>(source, edge_labels.size() ? *(edge_labels.begin()) : 0);
            const auto bb_index = cdg_bb_graph->CGetBBNodeInfo(block)->block->number;
            if(current_controller == header_controller)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(bb_index) + " does not diverge");
               basic_block_divergence[bb_index] = false;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(bb_index) + " diverges");
               basic_block_divergence[bb_index] = true;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Loop is parallelizable but not countable or iterations number is not multiple of parallel degree");
         const auto blocks = loop->get_blocks();
         for(const auto block : blocks)
         {
            basic_block_divergence[cdg_bb_graph->CGetBBNodeInfo(block)->block->number] = true;
         }
      }
   }
   else if(current_parallel_degree != 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loop is an inner simd");
      /// The control condition which enables execution of header
      THROW_ASSERT(boost::in_degree(loop->GetHeader(), *cdg_bb_graph) == 1, "");
      InEdgeIterator ie, ie_end;
      boost::tie(ie, ie_end) = boost::in_edges(loop->GetHeader(), *cdg_bb_graph);
      auto edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
      auto source = boost::source(*ie, *cdg_bb_graph);
      THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
      const auto header_controller = std::pair<vertex, unsigned int>(source, edge_labels.size() ? *(edge_labels.begin()) : 0);
      if(!basic_block_divergence[cdg_bb_graph->CGetBBNodeInfo(header_controller.first)->block->number] and (loop->loop_type & COUNTABLE_LOOP) and not basic_block_divergence.find(loop->Parent()->GetId())->second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Header does not diverge");
         basic_block_divergence[loop_id] = false;
         /// Classify the remaining basic blocks
         const auto blocks = loop->get_blocks();
         for(const auto block : blocks)
         {
            boost::tie(ie, ie_end) = boost::in_edges(block, *cdg_bb_graph);
            edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
            source = boost::source(*ie, *cdg_bb_graph);
            THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
            const auto current_controller = std::pair<vertex, unsigned int>(source, edge_labels.size() ? *(edge_labels.begin()) : 0);
            const auto bb_index = cdg_bb_graph->CGetBBNodeInfo(block)->block->number;
            if(current_controller == header_controller)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(bb_index) + " does not diverge");
               basic_block_divergence[bb_index] = false;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(bb_index) + " diverges");
               basic_block_divergence[bb_index] = true;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Header diverges, so all the basic blocks of the loop diverge");
         const auto blocks = loop->get_blocks();
         for(const auto block : blocks)
         {
            basic_block_divergence[cdg_bb_graph->CGetBBNodeInfo(block)->block->number] = true;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   loop_parallel_degree[loop_id] = current_parallel_degree;

   /// If the current parallel degree is different from the parallel loop of the outer loop, this is an outer simd loop
   if(current_parallel_degree != parallel_degree)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop " + STR(loop_id) + ": outer simd");
      simd_loop_type[loop_id] = SIMD_OUTER;
   }
   else if(current_parallel_degree != 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop " + STR(loop_id) + ": inner simd");
      simd_loop_type[loop_id] = SIMD_INNER;
   }
   else
   {
      simd_loop_type[loop_id] = SIMD_NONE;
   }

   for(auto nested_loop : loop->GetChildren())
   {
      ClassifyLoop(nested_loop, current_parallel_degree);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR(loop_id));
}

void Vectorize::ClassifyTreeNode(const unsigned int loop_id, const tree_nodeConstRef tree_node)
{
   if(transformations.find(tree_node->index) != transformations.end())
   {
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Classifying " + tree_node->get_kind_text() + " " + STR(tree_node->index) + ": " + tree_node->ToString());
   const LoopConstRef loop = function_behavior->CGetLoops()->CGetLoop(loop_id);
   switch(tree_node->get_kind())
   {
      case gimple_assign_K:
      {
         const auto* ga = GetPointer<const gimple_assign>(tree_node);
         const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
         ClassifyTreeNode(loop_id, GET_NODE(ga->op0));
         ClassifyTreeNode(loop_id, GET_NODE(ga->op1));
         if(ga->vdef)
            ClassifyTreeNode(loop_id, GET_NODE(ga->vdef));

         /// Check if the statement is the increment operation of an outer loop
         if(ga->predicate)
         {
            transformations[tree_node->index] = SCALAR;
         }
         else if(loop->inc_id == tree_node->index and simd_loop_type[loop->GetId()] == SIMD_OUTER)
         {
            transformations[tree_node->index] = INC;
         }
         /// This gimple assign could already be analyzed because of induction variable gimple phi
         else if(transformations.find(ga->index) == transformations.end())
         {
            if((transformations.find(ga->op1->index) != transformations.end() and transformations.find(ga->op1->index)->second == SCALAR) or
               (transformations.find(ga->op0->index) != transformations.end() and transformations.find(ga->op0->index)->second == SCALAR))
            {
               transformations[tree_node->index] = SCALAR;
            }
            else
            {
               transformations[tree_node->index] = SIMD;
            }
         }
         break;
      }
      case ssa_name_K:
      {
         const auto* sa = GetPointer<const ssa_name>(tree_node);
         THROW_ASSERT(sa->CGetDefStmts().size() == 1, "Not in ssa form: " + STR(sa->CGetDefStmts().size()) + " definitions");
         const unsigned int bb_index = GetPointer<const gimple_node>(GET_NODE(sa->CGetDefStmt()))->bb_index;
         const BBGraphConstRef bb_fcfg = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
         const unsigned int loop_index = bb_fcfg->CGetBBNodeInfo(bb_fcfg->CGetBBGraphInfo()->bb_index_map.find(bb_index)->second)->loop_id;
         if(GetPointer<const gimple_pragma>(GET_CONST_NODE(sa->CGetDefStmt())))
         {
            // SKIP
         }
         else if(simd_loop_type.find(loop_index)->second == SIMD_NONE)
         {
            transformations[tree_node->index] = NONE;
         }
         else if(tree_helper::CGetType(tree_node)->get_kind() == pointer_type_K or sa->virtual_flag)
         {
            transformations[tree_node->index] = SCALAR;
         }
         else
         {
            transformations[tree_node->index] = SIMD;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto* ue = GetPointer<const unary_expr>(tree_node);
         ClassifyTreeNode(loop_id, GET_NODE(ue->op));
         if(tree_node->get_kind() == nop_expr_K or tree_node->get_kind() == convert_expr_K or tree_node->get_kind() == abs_expr_K)
         {
            /// FIXME: allocation does not support this
            transformations[tree_node->index] = SCALAR;
         }
         else
         {
            transformations[tree_node->index] = transformations[ue->op->index];
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto* be = GetPointer<const binary_expr>(tree_node);
         if(tree_node->get_kind() == mem_ref_K or tree_node->get_kind() == trunc_div_expr_K or tree_node->get_kind() == trunc_mod_expr_K or tree_node->get_kind() == widen_mult_expr_K or tree_node->get_kind() == mult_expr_K or
            tree_node->get_kind() == lut_expr_K or tree_node->get_kind() == extract_bit_expr_K)
         {
            transformations[tree_node->index] = SCALAR;
         }
         /// FIXME: C backend for vec_lshift_expr is wrong
         else if(tree_node->get_kind() == rshift_expr_K or tree_node->get_kind() == lshift_expr_K or tree_node->get_kind() == le_expr_K /*) and (GET_NODE(be->op0)->get_kind() == integer_cst_K)*/)
         {
            transformations[tree_node->index] = SCALAR;
         }
#if 0
            else if(tree_node->get_kind() == plus_expr_K)
            {
               ClassifyTreeNode(loop_id, GET_NODE(be->op0));
               ClassifyTreeNode(loop_id, GET_NODE(be->op1));
               transformations[tree_node->index] = SIMD;
            }
#endif
         else
         {
            transformations[tree_node->index] = SCALAR;
            ClassifyTreeNode(loop_id, GET_NODE(be->op0));
            ClassifyTreeNode(loop_id, GET_NODE(be->op1));
            transformations[tree_node->index] = SIMD;
         }
         break;
      }
      case CASE_CST_NODES:
      {
         break;
      }
      case gimple_phi_K:
      {
         const auto* gp = GetPointer<const gimple_phi>(tree_node);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            ClassifyTreeNode(loop_id, GET_NODE(def_edge.first));
            THROW_ASSERT(GET_NODE(def_edge.first)->get_kind() != var_decl_K, "Unsupported pattern - Phi is" + STR(tree_node));
            if(def_edge.first->get_kind() == var_decl_K)
            {
               THROW_ERROR("Unsupported pattern");
            }
            transformations[tree_node->index] = SIMD;
         }
         ClassifyTreeNode(loop_id, GET_NODE(gp->res));
         /// Check if the phi is the init operation of an outer loop
         if(loop->init_gimple_id == tree_node->index and simd_loop_type[loop->GetId()] == SIMD_OUTER)
         {
            iv_increment[gp->res->index] = function_behavior->CGetLoops()->CGetLoop(loop_id)->increment;
            transformations[tree_node->index] = INIT;
         }
         /// Check if the phi is in the header of an outer loop
         else if(loop->GetId() == gp->bb_index and simd_loop_type[loop->GetId()] == SIMD_OUTER)
         {
            if(gp->virtual_flag or tree_helper::CGetType(GET_NODE(gp->res))->get_kind() == boolean_type_K)
            {
               transformations[gp->index] = SIMD;
            }
            else
            {
               const BBGraphConstRef bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);
               const vertex header = bb_graph->CGetBBGraphInfo()->bb_index_map.find(loop_id)->second;
               if(boost::in_degree(header, *bb_graph) != 1)
               {
                  THROW_ASSERT(false, "Header loop has more than non-feedback incoming edge");
                  THROW_ERROR("Unsupported pattern");
               }
               InEdgeIterator ie, ie_end;
               boost::tie(ie, ie_end) = boost::in_edges(header, *bb_graph);
               const vertex previous = boost::source(*ie, *bb_graph);
               const unsigned int previous_id = bb_graph->CGetBBNodeInfo(previous)->block->number;
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  if(def_edge.second != previous_id)
                  {
                     const tree_nodeRef loop_ssa = GET_NODE(def_edge.first);
                     if(loop_ssa->get_kind() != ssa_name_K)
                     {
                        THROW_ASSERT(false, "Unsupported pattern - Operand of ssa is " + loop_ssa->ToString());
                        THROW_ERROR("Unsupported pattern");
                     }
                     const auto* sn = GetPointer<const ssa_name>(loop_ssa);
                     if(sn->CGetDefStmts().size() != 1)
                     {
                        THROW_ASSERT(false, sn->ToString() + " has not a single definition");
                        THROW_ERROR("Unsupported pattern");
                     }
                     const auto* ga = GetPointer<const gimple_assign>(GET_NODE(sn->CGetDefStmt()));
                     if(not ga)
                     {
                        THROW_ASSERT(false, sn->ToString() + " is not defined in a gimple assignment but in " + sn->CGetDefStmt()->ToString());
                        THROW_ERROR("Unsupported pattern");
                     }
                     const auto* pe = GetPointer<const plus_expr>(GET_NODE(ga->op1));
                     if(not pe)
                     {
                        THROW_ASSERT(false, "Unexpected pattern: " + ga->op1->ToString());
                        THROW_ERROR("Unsupported pattern");
                     }
                     if(pe->op0->index != gp->res->index)
                     {
                        THROW_ASSERT(false, "Unexpected pattern: " + pe->op0->ToString());
                        THROW_ERROR("Unsupported pattern");
                     }
                     const auto* ic = GetPointer<const integer_cst>(GET_NODE(pe->op1));
                     if(not ic)
                     {
                        THROW_ASSERT(false, "Unexpected pattern: " + pe->op1->ToString());
                        THROW_ERROR("Unsupported pattern");
                     }
                     iv_increment[gp->res->index] = tree_helper::get_integer_cst_value(ic);
                     transformations[gp->index] = INIT;
                     transformations[ga->index] = INC;
                     break;
                  }
               }
            }
         }
         else
         {
            transformations[tree_node->index] = SIMD;
         }

         break;
      }
      case target_mem_ref461_K:
      {
         const auto* tmr = GetPointer<const target_mem_ref461>(tree_node);
         if(tmr->base)
         {
            ClassifyTreeNode(loop_id, GET_NODE(tmr->base));
         }
         if(tmr->offset)
         {
            ClassifyTreeNode(loop_id, GET_NODE(tmr->offset));
         }
         if(tmr->idx)
         {
            ClassifyTreeNode(loop_id, GET_NODE(tmr->idx));
         }
         if(tmr->step)
         {
            ClassifyTreeNode(loop_id, GET_NODE(tmr->step));
         }
         if(tmr->idx2)
         {
            ClassifyTreeNode(loop_id, GET_NODE(tmr->idx2));
         }
         transformations[tree_node->index] = SCALAR;
         break;
      }
      case var_decl_K:
      {
         transformations[tree_node->index] = SCALAR;
         break;
      }
      case gimple_cond_K:
      {
         const auto* gc = GetPointer<const gimple_cond>(tree_node);
         /// Check if the gimple cond belongs to an exit of the loop
         const auto basic_block_index = gc->bb_index;
         const BBGraphConstRef fbb = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
         const auto basic_block = fbb->CGetBBGraphInfo()->bb_index_map.find(basic_block_index)->second;
         ClassifyTreeNode(loop_id, GET_NODE(gc->op0));
         bool divergent = false;
         if(std::find(loop->exit_block_iter_begin(), loop->exit_block_iter_end(), basic_block) == loop->exit_block_iter_end())
         {
            divergent = true;
         }
         else
         {
            divergent = basic_block_divergence.find(basic_block_index)->second;
            /// Check that one of the outgoing edge is a feedback edge
            OutEdgeIterator oe, oe_end;
            for(boost::tie(oe, oe_end) = boost::out_edges(basic_block, *fbb); oe != oe_end; oe++)
            {
               const auto target_basic_block = boost::target(*oe, *fbb);
               const auto target_basic_block_index = fbb->CGetBBNodeInfo(target_basic_block)->block->number;
               if(basic_block_divergence.find(target_basic_block_index) == basic_block_divergence.end() or not basic_block_divergence.find(target_basic_block_index)->second)
               {
               }
               else
               {
                  divergent = true;
               }
            }
         }
         transformations[tree_node->index] = divergent ? COND_DIV : COND_CON;
         break;
      }
      case cond_expr_K:
      {
         const auto* ce = GetPointer<const cond_expr>(tree_node);
         ClassifyTreeNode(loop_id, GET_NODE(ce->op0));
         ClassifyTreeNode(loop_id, GET_NODE(ce->op1));
         ClassifyTreeNode(loop_id, GET_NODE(ce->op2));
         transformations[tree_node->index] = SIMD;
         break;
      }
      case array_ref_K:
      {
         const auto* ar = GetPointer<const array_ref>(tree_node);
         ClassifyTreeNode(loop_id, GET_NODE(ar->op0));
         ClassifyTreeNode(loop_id, GET_NODE(ar->op1));
         if(ar->op2)
            ClassifyTreeNode(loop_id, GET_NODE(ar->op2));
         if(ar->op3)
            ClassifyTreeNode(loop_id, GET_NODE(ar->op3));
         transformations[tree_node->index] = SCALAR;
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto* ce = GetPointer<const call_expr>(tree_node);
         const auto* ae = GetPointer<const addr_expr>(GET_NODE(ce->fn));
         const auto* fd = GetPointer<const function_decl>(GET_NODE(ae->op));
         const auto* in = GetPointer<const identifier_node>(GET_NODE(fd->name));
         const std::string function_name = in->strg;
         transformations[tree_node->index] = TM->function_index("parallel_" + function_name) ? SIMD : SCALAR;
         break;
      }
      case bit_ior_concat_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      {
         const auto te = GetPointer<const ternary_expr>(tree_node);
         ClassifyTreeNode(loop_id, GET_NODE(te->op0));
         ClassifyTreeNode(loop_id, GET_NODE(te->op1));
         ClassifyTreeNode(loop_id, GET_NODE(te->op2));
         transformations[tree_node->index] = SCALAR;
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<const lut_expr>(tree_node);
         ClassifyTreeNode(loop_id, GET_NODE(le->op0));
         ClassifyTreeNode(loop_id, GET_NODE(le->op1));
         if(le->op2)
            ClassifyTreeNode(loop_id, GET_NODE(le->op2));
         if(le->op3)
            ClassifyTreeNode(loop_id, GET_NODE(le->op3));
         if(le->op4)
            ClassifyTreeNode(loop_id, GET_NODE(le->op4));
         if(le->op5)
            ClassifyTreeNode(loop_id, GET_NODE(le->op5));
         if(le->op6)
            ClassifyTreeNode(loop_id, GET_NODE(le->op6));
         if(le->op7)
            ClassifyTreeNode(loop_id, GET_NODE(le->op7));
         if(le->op8)
            ClassifyTreeNode(loop_id, le->op8);
         transformations[tree_node->index] = SCALAR;
         break;
      }
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case error_mark_K:
      case using_decl_K:
      case type_decl_K:
      case target_mem_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_call_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_while_K:
      case CASE_PRAGMA_NODES:
      case array_range_ref_K:
      case target_expr_K:
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case CASE_TYPE_NODES:
      {
         THROW_UNREACHABLE("Not supported tree node " + tree_node->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--" + (transformations.find(tree_node->index) != transformations.end() ? ToString(transformations[tree_node->index]) : "Unclassified") + " - " + STR(tree_node->index) + ": " + tree_node->ToString());
}

unsigned int Vectorize::DuplicateIncrement(const unsigned int loop_id, const tree_nodeRef statement)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Duplicating " + statement->ToString());
   auto* ga = GetPointer<gimple_assign>(statement);

   /// First reclassify the original increment as simd
   transformations[statement->index] = SIMD;

   /// First create the new ssa_name
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_tree_node_schema;
   const auto* sa = GetPointer<const ssa_name>(GET_NODE(ga->op0));
   if(sa->type)
      ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
   if(sa->var)
      ssa_tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
   ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
   if(sa->volatile_flag)
      ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
   if(sa->virtual_flag)
      ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
   if(sa->max)
      ssa_tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
   if(sa->min)
      ssa_tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);
   unsigned int ssa_tree_node_index = TM->new_tree_node_id();
   TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);

   CustomUnorderedMapStable<unsigned int, unsigned int> remapping;
   tree_node_dup tnd(remapping, TM);
   remapping[ga->op0->index] = ssa_tree_node_index;

   /// Duplicate increment
   const unsigned int new_gimple = tnd.create_tree_node(statement);
#ifndef NDEBUG
   auto* new_ga = GetPointer<gimple_assign>(TM->get_tree_node_const(new_gimple));
#endif
   const BBGraphRef bb_graph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   LoopRef loop = function_behavior->GetLoops()->GetLoop(loop_id);
   THROW_ASSERT(loop->IsReducible(), "Loop is not reducible");
   /// Replace uses
   for(const auto& phi : bb_graph->GetBBNodeInfo(loop->GetHeader())->block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + ga->op0->ToString() + " with " + TM->get_tree_node_const(ssa_tree_node_index)->ToString() + " in " + phi->ToString());
      TM->ReplaceTreeNode(phi, ga->op0, TM->GetTreeReindex(ssa_tree_node_index));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Result: " + phi->ToString());
   }
   THROW_ASSERT(loop->num_exits() == 1, "Loop with not a single exit");
   const auto stmt_list = bb_graph->GetBBNodeInfo(*(loop->exit_block_iter_begin()))->block->CGetStmtList();
   THROW_ASSERT(stmt_list.size(), "Empty loop exit");
   THROW_ASSERT(stmt_list.size() and GetPointer<gimple_cond>(GET_NODE(stmt_list.back())), "Loop exit is not a cond_expr: " + stmt_list.back()->ToString());
   const auto gimple_cond_stmt = stmt_list.back();
   auto* gc = GetPointer<gimple_cond>(GET_NODE(gimple_cond_stmt));
   const auto condition_to_be_duplicated = gc->op0;
   auto* cond_op = GetPointer<ssa_name>(GET_CONST_NODE(condition_to_be_duplicated));
   THROW_ASSERT(cond_op, "Cond expression operand is " + STR(condition_to_be_duplicated));
   THROW_ASSERT(cond_op->CGetDefStmts().size() == 1, "Cond argument is not defined in a single assignment");
   auto def_statement = cond_op->CGetDefStmt();
   const auto use_stmts = cond_op->CGetUseStmts();
   if(use_stmts.size() > 1)
   {
      /// First create the new ssa_name
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> dup_op_cond_tn_schema;
      if(cond_op->type)
         dup_op_cond_tn_schema[TOK(TOK_TYPE)] = STR(cond_op->type->index);
      if(cond_op->var)
         dup_op_cond_tn_schema[TOK(TOK_VAR)] = STR(cond_op->var->index);
      dup_op_cond_tn_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
      if(cond_op->volatile_flag)
         dup_op_cond_tn_schema[TOK(TOK_VOLATILE)] = STR(cond_op->volatile_flag);
      if(cond_op->virtual_flag)
         dup_op_cond_tn_schema[TOK(TOK_VIRTUAL)] = STR(cond_op->virtual_flag);
      if(cond_op->max)
         dup_op_cond_tn_schema[TOK(TOK_MAX)] = STR(cond_op->max->index);
      if(cond_op->min)
         dup_op_cond_tn_schema[TOK(TOK_MIN)] = STR(cond_op->min->index);
      unsigned int dup_op_cond_index = TM->new_tree_node_id();
      // cppcheck-suppress unreadVariable
      remapping[cond_op->index] = dup_op_cond_index;
      TM->create_tree_node(dup_op_cond_index, ssa_name_K, dup_op_cond_tn_schema);
      /// Duplicate computation of condition
      const unsigned int new_cond_computation = tnd.create_tree_node(GET_NODE(def_statement));

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(TM->CGetTreeNode(new_cond_computation)));
      const auto new_gc_cond = GetPointer<const gimple_assign>(TM->CGetTreeNode(new_cond_computation));
      TM->ReplaceTreeNode(gimple_cond_stmt, condition_to_be_duplicated, new_gc_cond->op0);

      /// Look for phi computing the guard of header
      THROW_ASSERT(guards.find(loop_id) != guards.end(), "");
      const auto header_guard_tn = guards.find(loop_id)->second;
      if(GET_CONST_NODE(header_guard_tn)->get_kind() != integer_cst_K)
      {
         const auto header_guard_sn = GetPointer<const ssa_name>(GET_CONST_NODE(header_guard_tn));
         THROW_ASSERT(header_guard_sn, STR(header_guard_tn));
         const auto header_guard_stmt_tn = GET_CONST_NODE(header_guard_sn->CGetDefStmt());
         const auto header_guard_gp = GetPointer<const gimple_phi>(header_guard_stmt_tn);
         THROW_ASSERT(header_guard_gp, STR(header_guard_stmt_tn));
         const auto feedback_condition = [&]() -> tree_nodeRef {
            for(const auto& def_edge : header_guard_gp->CGetDefEdgesList())
            {
               if(def_edge.second == gc->bb_index)
               {
                  return def_edge.first;
               }
            }
            THROW_UNREACHABLE("");
            return tree_nodeRef();
         }();
         const auto feedback_condition_sn = GetPointer<const ssa_name>(GET_CONST_NODE(feedback_condition));
         THROW_ASSERT(feedback_condition_sn, "");
         const auto feedback_condition_def_stmt = GET_CONST_NODE(feedback_condition_sn->CGetDefStmt());
#if HAVE_ASSERTS
         const auto feedback_condition_def_ga = GetPointer<const gimple_assign>(feedback_condition_def_stmt);
         THROW_ASSERT(feedback_condition_def_ga, STR(feedback_condition_def_stmt));
         const auto feedback_condition_right_ta = GetPointer<const truth_and_expr>(GET_CONST_NODE(feedback_condition_def_ga->op1));
         THROW_ASSERT(feedback_condition_right_ta, GET_CONST_NODE(feedback_condition_def_ga->op1)->get_kind_text() + ": " + STR(feedback_condition_def_ga->op1));
         THROW_ASSERT(feedback_condition_right_ta->op0->index == condition_to_be_duplicated->index or feedback_condition_right_ta->op1->index == condition_to_be_duplicated->index, STR(feedback_condition_def_ga->op1));
#endif
         TM->ReplaceTreeNode(feedback_condition_sn->CGetDefStmt(), condition_to_be_duplicated, new_gc_cond->op0);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced condition in feedback guard computation " + STR(feedback_condition_def_stmt));
         bb_graph->GetBBNodeInfo(*(loop->exit_block_iter_begin()))->block->PushBefore(TM->GetTreeReindex(new_cond_computation), feedback_condition_sn->CGetDefStmt());
      }
      else
      {
         bb_graph->GetBBNodeInfo(*(loop->exit_block_iter_begin()))->block->PushBack(TM->GetTreeReindex(new_cond_computation));
      }
      ClassifyTreeNode(loop_id, TM->get_tree_node_const(new_cond_computation));
   }
   else
   {
      /// We create a local variable since first argument of replace_ssa_name is passed by reference.
      /// Statement root will not be modified by replace_ssa_name but only their successors
      TM->ReplaceTreeNode(def_statement, ga->op0, TM->GetTreeReindex(ssa_tree_node_index));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created statement " + new_ga->ToString());
   return new_gimple;
}

bool Vectorize::LookForScalar(const tree_nodeConstRef tree_node)
{
   switch(tree_node->get_kind())
   {
      case target_mem_ref461_K:
      case ssa_name_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      {
         THROW_UNREACHABLE("Not supported tree node " + tree_node->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return false;
}
#if HAVE_UNORDERED && NO_ABSEIL_HASH
/**
 * Definition of hash function for std::tyuple<unsigned int, vertex, unsigned int>
 */
namespace std
{
   template <>
   struct hash<std::tuple<unsigned int, vertex, unsigned int>> : public unary_function<std::tuple<unsigned int, vertex, unsigned int>, size_t>
   {
      size_t operator()(std::tuple<unsigned int, vertex, unsigned int> value) const
      {
         std::size_t ret = 0;
         hash<unsigned int> hasher;
         boost::hash_combine(ret, hasher(std::get<0>(value)));
         boost::hash_combine(ret, std::get<1>(value));
         boost::hash_combine(ret, hasher(std::get<2>(value)));
         return ret;
      }
   };
} // namespace std
#endif

void Vectorize::AddGuards()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing guards");
   /// The guards for combination of basic_block condition
   CustomMap<std::tuple<unsigned int, vertex, unsigned int>, tree_nodeRef> condition_guards;
   const auto true_value_id = TM->new_tree_node_id();
   const auto boolean_type = tree_man->create_boolean_type();
   const auto true_value = tree_man->CreateIntegerCst(boolean_type, 1, true_value_id);
   const auto cdg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);
   const auto cfg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);
   const auto fcfg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   std::list<vertex> basic_blocks;
   cfg_bb_graph->TopologicalSort(basic_blocks);
   for(const auto basic_block : basic_blocks)
   {
      const auto bb_node_info = cfg_bb_graph->CGetBBNodeInfo(basic_block);
      const auto basic_block_id = bb_node_info->block->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(basic_block_id));
      const auto loop_id = bb_node_info->loop_id;
      if(simd_loop_type.find(loop_id)->second == SIMD_NONE)
      {
         guards[basic_block_id] = true_value;
      }
      else if(!basic_block_divergence.find(basic_block_id)->second)
      {
         guards[basic_block_id] = true_value;
      }
      /// Endif
      else if(boost::in_degree(basic_block, *cfg_bb_graph) > 1 and boost::in_degree(basic_block, *cfg_bb_graph) == boost::in_degree(basic_block, *fcfg_bb_graph))
      {
         THROW_ASSERT(boost::in_degree(basic_block, *cdg_bb_graph) == 1, "");
         InEdgeIterator ie, ie_end;
         boost::tie(ie, ie_end) = boost::in_edges(basic_block, *cdg_bb_graph);
         auto edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
         auto source = boost::source(*ie, *cdg_bb_graph);
         THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
         const auto controller = std::tuple<unsigned int, vertex, unsigned int>(bb_node_info->loop_id, source, edge_labels.size() ? *(edge_labels.begin()) : 0);
         THROW_ASSERT(condition_guards.find(controller) != condition_guards.end(), "");
         guards[basic_block_id] = condition_guards.find(controller)->second;
      }
      /// Header
      else if(boost::in_degree(basic_block, *fcfg_bb_graph) > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding phi in header BB" + STR(basic_block_id));
         /// Create the ssa defined by phi
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema;
         auto ssa_vers = TM->get_next_vers();
         auto ssa_node_nid = TM->new_tree_node_id();
         ssa_schema[TOK(TOK_TYPE)] = STR(boolean_type->index);
         ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
         ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
         ssa_schema[TOK(TOK_VIRTUAL)] = STR(false);
         TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(TM->CGetTreeNode(ssa_node_nid)));

         /// Create the phi
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_phi_schema;
         const auto gimple_phi_id = TM->new_tree_node_id();
         gimple_phi_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_phi_schema[TOK(TOK_TYPE)] = STR(boolean_type->index);
         gimple_phi_schema[TOK(TOK_RES)] = STR(ssa_node_nid);
         gimple_phi_schema[TOK(TOK_SCPE)] = STR(function_id);
         TM->create_tree_node(gimple_phi_id, gimple_phi_K, gimple_phi_schema);
         auto gp = GetPointer<gimple_phi>(TM->get_tree_node_const(gimple_phi_id));
         gp->SetSSAUsesComputed();
         THROW_ASSERT(gp, "");
         InEdgeIterator ie, ie_end;
         for(boost::tie(ie, ie_end) = boost::in_edges(basic_block, *cfg_bb_graph); ie != ie_end; ie++)
         {
            const auto source = boost::source(*ie, *cfg_bb_graph);
            THROW_ASSERT(cfg_bb_graph->CGetBBNodeInfo(source)->block, "");
            const auto source_id = source != cfg_bb_graph->CGetBBGraphInfo()->entry_vertex ? cfg_bb_graph->CGetBBNodeInfo(source)->block->number : BB_ENTRY;
            THROW_ASSERT(guards.find(source_id) != guards.end(), "");
            gp->AddDefEdge(TM, gimple_phi::DefEdge(guards.find(source_id)->second, source_id));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(TM->CGetTreeNode(gimple_phi_id)));
         bb_node_info->block->AddPhi(TM->GetTreeReindex(gimple_phi_id));
         boost::tie(ie, ie_end) = boost::in_edges(basic_block, *cdg_bb_graph);
         auto edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
         auto source = boost::source(*ie, *cdg_bb_graph);
         THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == source, "");
         const auto controller = std::tuple<unsigned int, vertex, unsigned int>(bb_node_info->loop_id, source, edge_labels.size() ? *(edge_labels.begin()) : 0);
         condition_guards[controller] = gp->res;
         guards[basic_block_id] = gp->res;
      }
      else
      {
         InEdgeIterator ie, ie_end;
         boost::tie(ie, ie_end) = boost::in_edges(basic_block, *cfg_bb_graph);
         THROW_ASSERT(boost::in_degree(basic_block, *cdg_bb_graph) == 1, "");
         boost::tie(ie, ie_end) = boost::in_edges(basic_block, *cdg_bb_graph);
         const auto control = boost::source(*ie, *cdg_bb_graph);
         auto edge_labels = cdg_bb_graph->CGetBBEdgeInfo(*ie)->get_labels(CDG_SELECTOR);
         THROW_ASSERT(edge_labels.size() == 1 or cdg_bb_graph->CGetBBGraphInfo()->entry_vertex == control, "");
         const auto controller = std::tuple<unsigned int, vertex, unsigned int>(bb_node_info->loop_id, control, edge_labels.size() ? *(edge_labels.begin()) : 0);
         if(condition_guards.find(controller) != condition_guards.end())
         {
            const auto combined_condition = condition_guards.find(controller)->second;
            guards[basic_block_id] = combined_condition;
         }
         else
         {
            const auto control_bb_node_info = cfg_bb_graph->CGetBBNodeInfo(control)->block;
            const auto control_id = control_bb_node_info->number;
            const auto control_list_of_stmt = control_bb_node_info->CGetStmtList();
            THROW_ASSERT(control_list_of_stmt.size(), "");
            const auto gc = GetPointer<gimple_cond>(GET_NODE(control_list_of_stmt.back()));
            THROW_ASSERT(gc, "");
            const auto control_condition = guards.find(control_id)->second;
            const auto edge_condition = gc->op0;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Edge condition is " + STR(edge_condition));
            /// We cannot use CreateAndExpr to add the created statement since we need to add at the beginning of the current basic block instad of the end
            const auto combined_condition = control_condition->index != true_value->index ? tree_man->CreateAndExpr(control_condition, edge_condition, blocRef()) : edge_condition;
            if(control_condition->index != true_value->index)
            {
               bb_node_info->block->PushFront(GetPointer<const ssa_name>(GET_NODE(combined_condition))->CGetDefStmt());
            }
            if(edge_condition != gc->op0)
            {
               bb_node_info->block->PushFront(GetPointer<const ssa_name>(GET_NODE(edge_condition))->CGetDefStmt());
            }
            condition_guards[controller] = combined_condition;
            guards[basic_block_id] = combined_condition;
         }
      }
      /// Fix phi
      if(simd_loop_type.find(loop_id)->second != SIMD_NONE)
      {
         if(boost::out_degree(basic_block, *cfg_bb_graph) != boost::out_degree(basic_block, *fcfg_bb_graph))
         {
            if(boost::out_degree(basic_block, *fcfg_bb_graph) == 1)
            {
               OutEdgeIterator oe, oe_end;
               boost::tie(oe, oe_end) = boost::out_edges(basic_block, *fcfg_bb_graph);
               const auto target = boost::target(*oe, *fcfg_bb_graph);
               const auto target_id = cfg_bb_graph->CGetBBNodeInfo(target)->block->number;
               if(basic_block_divergence.find(target_id)->second)
               {
                  THROW_ASSERT(guards.find(target_id) != guards.end(), "");
                  const auto guard = guards.find(target_id)->second;
                  const auto sn = GetPointer<ssa_name>(GET_NODE(guard));
                  THROW_ASSERT(sn, "");
                  auto gp = GetPointer<gimple_phi>(GET_NODE(sn->CGetDefStmt()));
                  THROW_ASSERT(gp, "");
                  gp->AddDefEdge(TM, gimple_phi::DefEdge(guards.find(basic_block_id)->second, basic_block_id));
               }
            }
            else if(boost::out_degree(basic_block, *fcfg_bb_graph) == 2)
            {
               OutEdgeIterator oe, oe_end;
               for(boost::tie(oe, oe_end) = boost::out_edges(basic_block, *fcfg_bb_graph); oe != oe_end; oe++)
               {
                  if(fcfg_bb_graph->GetSelector(*oe) & FB_CFG_SELECTOR)
                  {
                     const auto target = boost::target(*oe, *fcfg_bb_graph);
                     const auto target_id = cfg_bb_graph->CGetBBNodeInfo(target)->block->number;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering feedback edge BB" + STR(basic_block_id) + "-->" + STR(target_id));
                     THROW_ASSERT(target_id == bb_node_info->block->true_edge, "");
                     if(basic_block_divergence.find(target_id)->second)
                     {
                        THROW_ASSERT(guards.find(target_id) != guards.end(), "");
                        const auto guard = guards.find(target_id)->second;
                        const auto sn = GetPointer<ssa_name>(GET_NODE(guard));
                        THROW_ASSERT(sn, "");
                        auto gp = GetPointer<gimple_phi>(GET_NODE(sn->CGetDefStmt()));
                        THROW_ASSERT(gp, "");
                        const auto list_of_stmt = bb_node_info->block->CGetStmtList();
                        THROW_ASSERT(list_of_stmt.size(), "");
                        const auto gc = GetPointer<gimple_cond>(GET_NODE(list_of_stmt.back()));
                        THROW_ASSERT(gc, "");
                        const auto current_condition = guards.find(basic_block_id)->second;
                        const auto feedback_condition = gc->op0;
                        const auto combined_condition = current_condition->index != true_value->index ? tree_man->CreateAndExpr(current_condition, feedback_condition, bb_node_info->block) : gc->op0;
                        gp->AddDefEdge(TM, gimple_phi::DefEdge(combined_condition, basic_block_id));
                     }
                  }
               }
            }
            else
            {
               THROW_UNREACHABLE("");
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(basic_block_id));
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      for(const auto& guard : guards)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Guard of BB" + STR(guard.first) + " is " + STR(guard.second));
      }
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed guards");
}

void Vectorize::FixPhis()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing phis");
   const auto dom_graph = function_behavior->CGetBBGraph(FunctionBehavior::DOM_TREE);
   const auto fcfg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const auto cdg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);
   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*fcfg_bb_graph); basic_block != basic_block_end; basic_block++)
   {
#ifndef NDEBUG
      const auto basic_block_id = fcfg_bb_graph->CGetBBNodeInfo(*basic_block)->block->number;
#endif
      if(boost::in_degree(*basic_block, *fcfg_bb_graph) <= 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + STR(basic_block_id) + " because has not multiple incoming edges");
         continue;
      }
      const auto bb_node_info = fcfg_bb_graph->CGetBBNodeInfo(*basic_block);
      if(simd_loop_type.find(bb_node_info->loop_id)->second == SIMD_NONE)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + STR(basic_block_id) + " because is outside parallelized loops");
         continue;
      }
      if(bb_node_info->loop_id == bb_node_info->block->number)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + STR(basic_block_id) + " because is an header");
         continue;
      }
      /// Check if the input basic block diverge
      bool diverge = false;
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(*basic_block, *fcfg_bb_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *fcfg_bb_graph);
         const auto source_id = fcfg_bb_graph->CGetBBNodeInfo(source)->block->number;
         if(basic_block_divergence.find(source_id)->second)
         {
            diverge = true;
            break;
         }
      }
      if(not diverge)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + STR(basic_block_id) + " because is not a convergence point");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing phis in BB" + STR(basic_block_id));
      THROW_ASSERT(boost::in_degree(*basic_block, *dom_graph) == 1, "");
      boost::tie(ie, ie_end) = boost::in_edges(*basic_block, *dom_graph);
      vertex whole_dominator = boost::source(*ie, *dom_graph);
      THROW_ASSERT(fcfg_bb_graph->ExistsEdge(whole_dominator, *basic_block), "");
      const std::map<vertex, unsigned int>& bb_map_levels = function_behavior->get_bb_map_levels();
      const bb_vertex_order_by_map comp_i(bb_map_levels);
      std::set<vertex, bb_vertex_order_by_map> phi_inputs(comp_i);
      for(boost::tie(ie, ie_end) = boost::in_edges(*basic_block, *fcfg_bb_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *fcfg_bb_graph);
         if(source != whole_dominator)
            phi_inputs.insert(source);
      }
      for(const auto& phi : bb_node_info->block->CGetPhiList())
      {
         CustomMap<unsigned int, tree_nodeRef> new_defs;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing phi " + STR(phi));
         auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
         const auto type = tree_helper::CGetType(GET_NODE(gp->res));
         THROW_ASSERT(type, "");
         for(const auto source : phi_inputs)
         {
            const auto source_id = fcfg_bb_graph->CGetBBNodeInfo(source)->block->number;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing value coming from BB" + STR(source_id));
            InEdgeIterator ie_local, ie_end_local;
            THROW_ASSERT(boost::in_degree(source, *cdg_bb_graph) == 1, "");
            boost::tie(ie_local, ie_end_local) = boost::in_edges(source, *cdg_bb_graph);
            vertex local_dominator = boost::source(*ie_local, *cdg_bb_graph);
            const auto local_dominator_index = dom_graph->CGetBBNodeInfo(local_dominator)->block->number;
            const auto from_dominator = [&]() -> tree_nodeRef {
               if(local_dominator == whole_dominator)
               {
                  for(const auto& def_edge : gp->CGetDefEdgesList())
                  {
                     if(def_edge.second == local_dominator_index)
                     {
                        return def_edge.first;
                     }
                  }
                  THROW_UNREACHABLE("");
                  return tree_nodeRef();
               }
               else
               {
                  THROW_ASSERT(new_defs.find(local_dominator_index) != new_defs.end(), "");
                  return new_defs.find(local_dominator_index)->second;
               }
            }();
            const auto from_source = [&]() -> gimple_phi::DefEdge {
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  if(def_edge.second == source_id)
                  {
                     return def_edge;
                  }
               }
               THROW_UNREACHABLE("");
               return gimple_phi::DefEdge();
            }();
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema, ssa_schema;
            const auto cond_expr_id = TM->new_tree_node_id();
            cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            cond_expr_schema[TOK(TOK_TYPE)] = STR(type->index);
            THROW_ASSERT(guards.find(source_id) != guards.end(), "");
            cond_expr_schema[TOK(TOK_OP0)] = STR(guards.find(source_id)->second->index);
            cond_expr_schema[TOK(TOK_OP1)] = STR(from_source.first->index);
            cond_expr_schema[TOK(TOK_OP2)] = STR(from_dominator->index);
            TM->create_tree_node(cond_expr_id, cond_expr_K, cond_expr_schema);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created cond_expr " + TM->get_tree_node_const(cond_expr_id)->ToString());

            /// Create the ssa with the new input of the phi
            auto ssa_vers = TM->get_next_vers();
            auto ssa_node_nid = TM->new_tree_node_id();
            ssa_schema[TOK(TOK_TYPE)] = STR(type->index);
            ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
            ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
            ssa_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
            if(GET_NODE(from_source.first)->get_kind() == ssa_name_K and GET_NODE(from_dominator)->get_kind() == ssa_name_K)
            {
               const auto sn1 = GetPointer<const ssa_name>(GET_NODE(from_source.first));
               const auto sn2 = GetPointer<const ssa_name>(GET_NODE(from_dominator));
               if(sn1->var and sn2->var and sn1->var->index == sn2->var->index)
               {
                  ssa_schema[TOK(TOK_VAR)] = STR(sn1->var->index);
               }
            }
            TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created ssa_name " + TM->get_tree_node_const(ssa_node_nid)->ToString());

            /// Create the assign
            const auto gimple_node_id = TM->new_tree_node_id();
            gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            gimple_assign_schema[TOK(TOK_TYPE)] = STR(type->index);
            gimple_assign_schema[TOK(TOK_OP0)] = STR(ssa_node_nid);
            gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
            TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
            fcfg_bb_graph->CGetBBNodeInfo(source)->block->PushBack(TM->GetTreeReindex(gimple_node_id));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created gimple_assign " + TM->get_tree_node_const(gimple_node_id)->ToString());
            new_defs[source_id] = TM->GetTreeReindex(ssa_node_nid);
            gp->ReplaceDefEdge(TM, from_source, gimple_phi::DefEdge(TM->GetTreeReindex(ssa_node_nid), from_source.second));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed value coming from BB" + STR(source_id));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phi " + STR(phi));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phis in BB" + STR(basic_block_id));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phis");
}

void Vectorize::SetPredication()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating predicated statements");
   const auto bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   VertexIterator bb, bb_end;
   for(boost::tie(bb, bb_end) = boost::vertices(*bb_graph); bb != bb_end; bb++)
   {
      const BBNodeInfoConstRef bb_node_info = bb_graph->CGetBBNodeInfo(*bb);
      const auto basic_block_index = bb_node_info->block->number;
      if(basic_block_divergence.find(basic_block_index) != basic_block_divergence.end() and basic_block_divergence.find(basic_block_index)->second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering statements of BB" + STR(basic_block_index));
         for(const auto& stmt : bb_node_info->block->CGetStmtList())
         {
            auto ga = GetPointer<gimple_assign>(GET_NODE(stmt));
            if(ga and ga->predicate)
            {
               ga->predicate = guards.find(basic_block_index)->second;
               auto sn = GetPointer<ssa_name>(GET_NODE(ga->predicate));
               if(sn)
               {
                  sn->AddUseStmt(stmt);
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered statements of BB" + STR(basic_block_index));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created predicated statements");
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> Vectorize::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_CONTROL_DEPENDENCE_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_ANALYSIS_BAMBU, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_ORDER_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PREDICATE_STATEMENTS, SAME_FUNCTION));
         const auto is_simd = [&]() -> bool {
            const auto sl = GetPointer<const statement_list>(GET_NODE(GetPointer<const function_decl>(TM->CGetTreeNode(function_id))->body));
            THROW_ASSERT(sl, "");
            for(const auto& block : sl->list_of_bloc)
            {
               for(const auto& stmt : block.second->CGetStmtList())
               {
                  const auto gp = GetPointer<const gimple_pragma>(GET_NODE(stmt));
                  if(gp and gp->scope and GetPointer<const omp_pragma>(GET_NODE(gp->scope)))
                  {
                     const auto* sp = GetPointer<const omp_simd_pragma>(GET_NODE(gp->directive));
                     if(sp)
                     {
                        return true;
                     }
                  }
               }
            }
            return false;
         }();
         if(is_simd)
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SERIALIZE_MUTUAL_EXCLUSIONS, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SIMPLE_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status Vectorize::InternalExec()
{
   if(parameters->IsParameter("vectorize") and parameters->GetParameter<std::string>("vectorize") == "disable")
      return DesignFlowStep_Status::UNCHANGED;

   /// Classify loop
   ClassifyLoop(function_behavior->GetLoops()->GetLoop(0), 0);

   /// Add the guards
   AddGuards();
#ifndef NDEBUG
   if(debug_level > DEBUG_LEVEL_VERY_PEDANTIC)
   {
      WriteBBGraphDot("BB_Inside_" + GetName() + "_Guards.dot");
   }
#endif

   /// Fix the phi
   FixPhis();
#ifndef NDEBUG
   if(debug_level > DEBUG_LEVEL_VERY_PEDANTIC)
   {
      WriteBBGraphDot("BB_Inside_" + GetName() + "_FixPhis.dot");
   }
#endif
   /// Predicate instructions which cannot be speculated
   SetPredication();
#ifndef NDEBUG
   if(debug_level > DEBUG_LEVEL_VERY_PEDANTIC)
   {
      WriteBBGraphDot("BB_Inside_" + GetName() + "_Predicated.dot");
   }
#endif

   /// Classify statement
   const BBGraphRef bb_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
   VertexIterator bb, bb_end;
   for(boost::tie(bb, bb_end) = boost::vertices(*bb_graph); bb != bb_end; bb++)
   {
      const BBNodeInfoConstRef bb_node_info = bb_graph->CGetBBNodeInfo(*bb);
      if(simd_loop_type[bb_node_info->loop_id] != SIMD_NONE)
      {
         const blocRef block = bb_node_info->block;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Classifying statement of BB" + STR(block->number));
         for(const auto& statement : block->CGetStmtList())
         {
            ClassifyTreeNode(bb_node_info->loop_id, GET_CONST_NODE(statement));
         }
         for(const auto& phi : block->CGetPhiList())
         {
            ClassifyTreeNode(bb_node_info->loop_id, GET_CONST_NODE(phi));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Classified statement of BB" + STR(block->number));
      }
   }

   /// Duplicate the increment operation when necessary
   for(boost::tie(bb, bb_end) = boost::vertices(*bb_graph); bb != bb_end; bb++)
   {
      const BBNodeInfoConstRef bb_node_info = bb_graph->CGetBBNodeInfo(*bb);
      if(simd_loop_type[bb_node_info->loop_id] != SIMD_NONE)
      {
         const blocRef block = bb_node_info->block;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Transforming increment statement of BB" + STR(block->number));
         for(const auto& statement : block->CGetStmtList())
         {
            if(transformations.find(statement->index)->second == INC)
            {
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               {
                  const std::string file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + "before_" + STR(statement->index) + "_expansion.gimple";
                  std::ofstream gimple_file(file_name.c_str());
                  TM->PrintGimple(gimple_file, false);
                  gimple_file.close();
               }
               const auto new_statement = DuplicateIncrement(bb_node_info->loop_id, GET_NODE(statement));
               block->PushBefore(TM->GetTreeReindex(new_statement), statement);
               ClassifyTreeNode(bb_node_info->loop_id, TM->get_tree_node_const(new_statement));
               transformations[new_statement] = INC;
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               {
                  const std::string file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + "after_" + STR(statement->index) + "_expansion.gimple";
                  std::ofstream gimple_file(file_name.c_str());
                  TM->PrintGimple(gimple_file, false);
                  gimple_file.close();
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed increment statement of BB" + STR(block->number));
      }
   }
#ifndef NDEBUG
   if(debug_level > DEBUG_LEVEL_VERY_PEDANTIC)
   {
      WriteBBGraphDot("BB_Inside_" + GetName() + "_Duplicated.dot");
   }
#endif

   /// Perform the transformation
   for(boost::tie(bb, bb_end) = boost::vertices(*bb_graph); bb != bb_end; bb++)
   {
      const BBNodeInfoConstRef bb_node_info = bb_graph->CGetBBNodeInfo(*bb);
      if(simd_loop_type[bb_node_info->loop_id] != SIMD_NONE)
      {
         const blocRef block = bb_node_info->block;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Transforming statement of BB" + STR(block->number) + " - Loop " + STR(bb_node_info->loop_id) + " - Parallel degree " + STR(loop_parallel_degree[bb_node_info->loop_id]));
         std::list<tree_nodeRef> new_statement_list;
         std::vector<tree_nodeRef> new_phi_list;
         for(const auto& statement : block->CGetStmtList())
         {
            TM->GetTreeReindex(Transform(statement->index, loop_parallel_degree[bb_node_info->loop_id], 0, new_statement_list, new_phi_list));
         }
         for(const auto& phi : block->CGetPhiList())
         {
            TM->GetTreeReindex(Transform(phi->index, loop_parallel_degree[bb_node_info->loop_id], 0, new_statement_list, new_phi_list));
         }
         /// Remove old statements
         const auto& old_statement_list = block->CGetStmtList();
         while(old_statement_list.size())
            block->RemoveStmt(old_statement_list.front());
         /// Remove old phis
         const auto& old_phi_list = block->CGetPhiList();
         while(old_phi_list.size())
            block->RemovePhi(old_phi_list.front());
         /// Add new statements
         for(const auto& new_stmt : new_statement_list)
         {
            block->PushBack(new_stmt);
         }
         /// Add new phis
         for(const auto& new_phi : new_phi_list)
         {
            block->AddPhi(new_phi);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed statement of BB" + STR(block->number));
      }
   }

   iv_increment.clear();
   simd_loop_type.clear();
   loop_parallel_degree.clear();
   basic_block_divergence.clear();
   transformations.clear();
   function_behavior->UpdateBBVersion();
   guards.clear();
   return DesignFlowStep_Status::SUCCESS;
}

unsigned int Vectorize::Transform(const unsigned int tree_node_index, const size_t parallel_degree, const size_t scalar_index, std::list<tree_nodeRef>& new_stmt_list, std::vector<tree_nodeRef>& new_phi_list)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Transforming " + TM->get_tree_node_const(tree_node_index)->get_kind_text() + " " + STR(tree_node_index) +
                      ((TM->get_tree_node_const(tree_node_index)->get_kind() != function_decl_K) ? ": " + TM->get_tree_node_const(tree_node_index)->ToString() : ""));
   const tree_nodeConstRef tn = TM->get_tree_node_const(tree_node_index);
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   unsigned int return_value = 0;
   const auto transformation = [&]() -> Transformation {
      if(transformations.find(tree_node_index) != transformations.end() and transformations.find(tree_node_index)->second == NONE)
      {
         return NONE;
      }
      if(scalar_index != 0)
      {
         return SCALAR;
      }
      if(transformations.find(tree_node_index) != transformations.end())
      {
         return transformations.find(tree_node_index)->second;
      }
      return SIMD;
   }();
   switch(transformation)
   {
      case(NONE):
      {
         return_value = tree_node_index;
         break;
      }
      case(COND_CON):
      {
         THROW_ASSERT(tn->get_kind() == gimple_cond_K, "Condition is " + tn->get_kind_text());
         const auto* gc = GetPointer<const gimple_cond>(tn);
         std::string include_name = GetPointer<const srcp>(tn)->include_name;
         unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
         unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
         tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
         tree_node_schema[TOK(TOK_OP0)] = STR(Transform(gc->op0->index, parallel_degree, 1, new_stmt_list, new_phi_list));
         unsigned int new_tree_node_index = TM->new_tree_node_id();
         TM->create_tree_node(new_tree_node_index, gimple_cond_K, tree_node_schema);
         auto* new_gc = GetPointer<gimple_cond>(TM->get_tree_node_const(new_tree_node_index));
         new_gc->memuse = gc->memuse;
         new_gc->memdef = gc->memdef;
         for(const auto& vuse : gc->vuses)
         {
            new_gc->vuses.insert(TM->GetTreeReindex(Transform(vuse->index, parallel_degree, 1, new_stmt_list, new_phi_list)));
         }
         if(gc->vdef)
         {
            new_gc->vdef = TM->GetTreeReindex(Transform(gc->vdef->index, parallel_degree, 1, new_stmt_list, new_phi_list));
         }
         new_gc->vovers = gc->vovers;
         new_gc->pragmas = gc->pragmas;
         new_gc->use_set = gc->use_set;
         new_gc->clobbered_set = gc->clobbered_set;
         return_value = new_tree_node_index;
         new_stmt_list.push_back(TM->GetTreeReindex(new_tree_node_index));
         break;
      }
      case(COND_DIV):
      {
         std::list<tree_nodeRef> conditions;
         const auto* gc = GetPointer<const gimple_cond>(tn);
         for(size_t parallel_index = 1; parallel_index <= parallel_degree; parallel_index++)
         {
            conditions.push_back(TM->GetTreeReindex(Transform(gc->op0->index, parallel_degree, parallel_index, new_stmt_list, new_phi_list)));
         }
         while(conditions.size() > 1)
         {
            const auto first_condition = conditions.front();
            conditions.pop_front();
            const auto second_condition = conditions.front();
            conditions.pop_front();
            const auto new_cond = tree_man->CreateOrExpr(first_condition, second_condition, blocRef());
            new_stmt_list.push_back(GetPointer<const ssa_name>(GET_NODE(new_cond))->CGetDefStmt());
            conditions.push_back(new_cond);
         }
         std::string include_name = GetPointer<const srcp>(tn)->include_name;
         unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
         unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
         tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
         tree_node_schema[TOK(TOK_OP0)] = STR(conditions.front()->index);
         unsigned int new_tree_node_index = TM->new_tree_node_id();
         TM->create_tree_node(new_tree_node_index, gimple_cond_K, tree_node_schema);
         return_value = new_tree_node_index;
         new_stmt_list.push_back(TM->GetTreeReindex(new_tree_node_index));
         break;
      }
      case(INIT):
      {
         THROW_ASSERT(tn->get_kind() == gimple_phi_K, "Init is " + tn->get_kind_text());
         const auto* gp = GetPointer<const gimple_phi>(tn);
         tree_node_schema[TOK(TOK_SCPE)] = STR(Transform(gp->scpe->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         tree_node_schema[TOK(TOK_RES)] = STR(Transform(gp->res->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         tree_node_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
         std::string include_name = gp->include_name;
         unsigned int line_number = gp->line_number;
         unsigned int column_number = gp->column_number;
         tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
         unsigned int new_tree_node_id = TM->new_tree_node_id();
         TM->create_tree_node(new_tree_node_id, gimple_phi_K, tree_node_schema);
         auto* new_gp = GetPointer<gimple_phi>(TM->get_tree_node_const(new_tree_node_id));
         InEdgeIterator ie, ie_end;
         const BBGraphRef bb_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
         const vertex header = bb_graph->CGetBBGraphInfo()->bb_index_map.find(gp->bb_index)->second;
         boost::tie(ie, ie_end) = boost::in_edges(header, *bb_graph);
         const vertex previous = boost::source(*ie, *bb_graph);
         const unsigned int previous_id = bb_graph->CGetBBNodeInfo(previous)->block->number;
         const auto previous_block = bb_graph->GetBBNodeInfo(previous)->block;
         const auto previous_list_of_stmt = previous_block->CGetStmtList();
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            if(def_edge.second == previous_id)
            {
               if(GET_NODE(def_edge.first)->get_kind() == integer_cst_K)
               {
                  const auto* ic = GetPointer<const integer_cst>(GET_NODE(def_edge.first));
                  tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(ic->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  /// The current init
                  const long long int original_init = tree_helper::get_integer_cst_value(ic);
                  unsigned int new_init_tree_node_id = TM->new_tree_node_id();
                  TM->create_tree_node(new_init_tree_node_id, vector_cst_K, tree_node_schema);
                  auto* new_tn = GetPointer<vector_cst>(TM->get_tree_node_const(new_init_tree_node_id));
                  THROW_ASSERT(iv_increment.find(gp->res->index) != iv_increment.end(), "Increment variable of " + gp->res->ToString() + " is unknown");
                  const long long int increment = iv_increment.find(gp->res->index)->second;
                  for(size_t i = 0; i < parallel_degree; i++)
                  {
                     const long long int local_init = original_init + increment * static_cast<long long int>(i);
                     const tree_nodeRef new_ic = tree_man->CreateIntegerCst(ic->type, local_init, TM->new_tree_node_id());
                     new_tn->list_of_valu.push_back(TM->GetTreeReindex(new_ic->index));
                  }
                  new_gp->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeReindex(new_tn->index), def_edge.second));
               }
               else if(GET_NODE(def_edge.first)->get_kind() == ssa_name_K)
               {
                  CustomMap<size_t, unsigned int> version_to_ssa;
                  for(size_t i = 1; i <= parallel_degree; i++)
                  {
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_tree_node_schema, plus_tree_node_schema, ssa_tree_node_schema;

                     plus_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                     plus_tree_node_schema[TOK(TOK_OP0)] = STR(def_edge.first->index);
                     plus_tree_node_schema[TOK(TOK_TYPE)] = STR(tree_helper::CGetType((GET_NODE(def_edge.first)))->index);
                     THROW_ASSERT(iv_increment.find(gp->res->index) != iv_increment.end(), "Increment variable of " + gp->res->ToString() + " is unknown");
                     const long long int increment = iv_increment.find(gp->res->index)->second * static_cast<long long int>(i - 1);
                     const tree_nodeRef new_ic = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), increment, TM->new_tree_node_id());
                     plus_tree_node_schema[TOK(TOK_OP1)] = STR(new_ic->index);
                     unsigned int plus_tree_node_index = TM->new_tree_node_id();
                     TM->create_tree_node(plus_tree_node_index, plus_expr_K, plus_tree_node_schema);

                     THROW_ASSERT(GET_NODE(def_edge.first)->get_kind() == ssa_name_K, "");
                     const auto* sa = GetPointer<const ssa_name>(GET_NODE(def_edge.first));
                     if(sa->type)
                        ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
                     if(sa->var)
                        ssa_tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
                     ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
                     ssa_tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
                     if(sa->volatile_flag)
                        ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
                     if(sa->virtual_flag)
                        ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
                     if(sa->max)
                        ssa_tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
                     if(sa->min)
                        ssa_tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);
                     unsigned int ssa_tree_node_index = TM->new_tree_node_id();
                     TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);
                     version_to_ssa[i] = ssa_tree_node_index;

                     gimple_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                     gimple_tree_node_schema[TOK(TOK_OP1)] = STR(plus_tree_node_index);
                     gimple_tree_node_schema[TOK(TOK_OP0)] = STR(ssa_tree_node_index);
                     unsigned int gimple_tree_node_index = TM->new_tree_node_id();
                     TM->create_tree_node(gimple_tree_node_index, gimple_assign_K, gimple_tree_node_schema);

                     THROW_ASSERT(previous_list_of_stmt.empty() or GET_NODE((*(previous_list_of_stmt.begin())))->get_kind() != gimple_cond_K, "");
                     previous_block->PushBack(TM->GetTreeReindex(gimple_tree_node_index));
                  }
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema, gimple_tree_node_schema, ssa_tree_node_schema;
                  constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(tree_helper::CGetType(GET_NODE(gp->res))->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int constructor_index = TM->new_tree_node_id();
                  TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
                  auto* constr = GetPointer<constructor>(TM->get_tree_node_const(constructor_index));
                  for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
                  {
                     const tree_nodeRef new_ic = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(scalar - 1), TM->new_tree_node_id());
                     constr->add_idx_valu(new_ic, TM->GetTreeReindex(version_to_ssa[scalar]));
                  }

                  const auto* sa = GetPointer<const ssa_name>(GET_NODE(def_edge.first));
                  if(sa->type)
                     ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(sa->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  if(sa->var)
                     ssa_tree_node_schema[TOK(TOK_VAR)] = STR(Transform(sa->var->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
                  if(sa->volatile_flag)
                     ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
                  if(sa->virtual_flag)
                     ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
                  if(sa->max)
                     ssa_tree_node_schema[TOK(TOK_MAX)] = STR(Transform(sa->max->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  if(sa->min)
                     ssa_tree_node_schema[TOK(TOK_MIN)] = STR(Transform(sa->min->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int ssa_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);

                  gimple_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  gimple_tree_node_schema[TOK(TOK_OP1)] = STR(constructor_index);
                  gimple_tree_node_schema[TOK(TOK_OP0)] = STR(ssa_tree_node_index);
                  unsigned int gimple_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(gimple_tree_node_index, gimple_assign_K, gimple_tree_node_schema);
                  previous_block->PushBack(TM->GetTreeReindex(gimple_tree_node_index));
                  new_gp->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeReindex(ssa_tree_node_index), def_edge.second));
               }
               else
               {
                  THROW_UNREACHABLE(def_edge.first->ToString());
               }
            }
            else
            {
               THROW_ASSERT(transformations.find(def_edge.first->index) == transformations.end() or transformations.find(def_edge.first->index)->second == SIMD, "");
               new_gp->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeReindex(Transform(def_edge.first->index, parallel_degree, 0, new_stmt_list, new_phi_list)), def_edge.second));
            }
         }
         return_value = new_tree_node_id;
         new_phi_list.push_back(TM->GetTreeReindex(return_value));
         /// Creating scalar from simd
         for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
         {
            /// Build bit_field_ref to extract the scalar
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> bit_field_ref_tree_node_schema, ssa_tree_node_schema, gimple_assign_tree_node_schema;
            unsigned int bit_field_ref_index = TM->new_tree_node_id();
            const auto element_type = tree_helper::CGetType(GET_NODE(gp->res));
            /// vector of boolean types are mapped on vector of unsigned integer
            const unsigned int bit_size = element_type->get_kind() != boolean_type_K ? tree_helper::Size(element_type) : 32;
            const tree_nodeRef offset = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>((scalar - 1) * bit_size), TM->new_tree_node_id());
            const tree_nodeRef size = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(bit_size), TM->new_tree_node_id());
            bit_field_ref_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
            bit_field_ref_tree_node_schema[TOK(TOK_TYPE)] = STR(tree_helper::CGetType(GET_NODE(gp->res))->index);
            bit_field_ref_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(gp->res->index, parallel_degree, 0, new_stmt_list, new_phi_list));
            bit_field_ref_tree_node_schema[TOK(TOK_OP1)] = STR(size->index);
            bit_field_ref_tree_node_schema[TOK(TOK_OP2)] = STR(offset->index);
            TM->create_tree_node(bit_field_ref_index, bit_field_ref_K, bit_field_ref_tree_node_schema);

            const auto* sa = GetPointer<const ssa_name>(GET_NODE(gp->res));
            if(sa->type)
               ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
            if(sa->var)
               ssa_tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
            ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
            ssa_tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
            if(sa->volatile_flag)
               ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
            if(sa->virtual_flag)
               ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
            if(sa->max)
               ssa_tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
            if(sa->min)
               ssa_tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);

            unsigned int ssa_tree_node_index = TM->new_tree_node_id();
            TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);

            gimple_assign_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
            gimple_assign_tree_node_schema[TOK(TOK_OP1)] = STR(bit_field_ref_index);
            gimple_assign_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(gp->res->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
            unsigned int gimple_new_tree_node_index = TM->new_tree_node_id();
            TM->create_tree_node(gimple_new_tree_node_index, gimple_assign_K, gimple_assign_tree_node_schema);
            /// Split of phi node goes to the beginning of the list of statement
            new_stmt_list.push_front(TM->GetTreeReindex(gimple_new_tree_node_index));
         }
         break;
      }
      case(INC):
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inc");
         THROW_ASSERT(tn->get_kind() == gimple_assign_K, "Increment is " + tn->get_kind_text());
         const auto* ga = GetPointer<const gimple_assign>(tn);
         const long long int increment = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(GetPointer<binary_expr>(GET_NODE(ga->op1))->op1)));
         std::string include_name = GetPointer<const srcp>(tn)->include_name;
         unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
         unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
         tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
         if(ga->orig)
            tree_node_schema[TOK(TOK_ORIG)] = STR(ga->orig->index);
         tree_node_schema[TOK(TOK_CLOBBER)] = STR(ga->clobber);
         tree_node_schema[TOK(TOK_INIT)] = STR(ga->init_assignment);
         tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         tree_node_schema[TOK(TOK_OP1)] = STR(Transform(ga->op1->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         unsigned int new_tree_node_index = TM->new_tree_node_id();
         TM->create_tree_node(new_tree_node_index, gimple_assign_K, tree_node_schema);
         auto* new_ga = GetPointer<gimple_assign>(TM->get_tree_node_const(new_tree_node_index));
         THROW_ASSERT(GET_NODE(new_ga->op1)->get_kind() == plus_expr_K or GET_NODE(new_ga->op1)->get_kind() == minus_expr_K, "Loop increment operation is not a plus expression nor a minus expression");
         auto* be = GetPointer<binary_expr>(GET_NODE(new_ga->op1));
         THROW_ASSERT(GET_NODE(be->op1)->get_kind() == vector_cst_K, "Increment is not constant");
         const tree_nodeConstRef type = GetPointer<const vector_type>(tree_helper::CGetType(GET_NODE(be->op1)))->elts;
         const long long int new_increment = increment * static_cast<long long int>(parallel_degree);
         const tree_nodeRef new_ic = tree_man->CreateIntegerCst(type, new_increment, TM->new_tree_node_id());
         be->op1 = TM->GetTreeReindex(Transform(new_ic->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         new_ga->memuse = ga->memuse;
         new_ga->memdef = ga->memdef;
         for(const auto& vuse : ga->vuses)
         {
            new_ga->vuses.insert(TM->GetTreeReindex(Transform(vuse->index, parallel_degree, 0, new_stmt_list, new_phi_list)));
         }
         if(ga->vdef)
         {
            new_ga->vdef = TM->GetTreeReindex(Transform(ga->vdef->index, parallel_degree, 0, new_stmt_list, new_phi_list));
         }
         new_ga->vovers = ga->vovers;
         new_ga->pragmas = ga->pragmas;
         new_ga->use_set = ga->use_set;
         new_ga->clobbered_set = ga->clobbered_set;
         return_value = new_tree_node_index;
         new_stmt_list.push_back(TM->GetTreeReindex(new_tree_node_index));

         /// Creating scalar from simd

         for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
         {
            /// Build bit_field_ref to extract the scalar
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> bit_field_ref_tree_node_schema, ssa_tree_node_schema, gimple_assign_tree_node_schema;
            unsigned int bit_field_ref_index = TM->new_tree_node_id();
            const auto element_type = tree_helper::CGetType(GET_NODE(ga->op0));
            /// vector of boolean types are mapped on vector of integer
            const unsigned int bit_size = element_type->get_kind() != boolean_type_K ? tree_helper::Size(element_type) : 32;
            const tree_nodeRef offset = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>((scalar - 1) * bit_size), TM->new_tree_node_id());
            const tree_nodeRef size = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(bit_size), TM->new_tree_node_id());
            bit_field_ref_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
            bit_field_ref_tree_node_schema[TOK(TOK_TYPE)] = STR(tree_helper::CGetType(tn)->index);
            bit_field_ref_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
            bit_field_ref_tree_node_schema[TOK(TOK_OP1)] = STR(size->index);
            bit_field_ref_tree_node_schema[TOK(TOK_OP2)] = STR(offset->index);
            TM->create_tree_node(bit_field_ref_index, bit_field_ref_K, bit_field_ref_tree_node_schema);

            const auto* sa = GetPointer<const ssa_name>(GET_NODE(ga->op0));
            if(sa->type)
               ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
            if(sa->var)
               ssa_tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
            ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
            ssa_tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
            if(sa->volatile_flag)
               ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
            if(sa->virtual_flag)
               ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
            if(sa->max)
               ssa_tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
            if(sa->min)
               ssa_tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);

            unsigned int ssa_tree_node_index = TM->new_tree_node_id();
            TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);

            gimple_assign_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
            gimple_assign_tree_node_schema[TOK(TOK_OP1)] = STR(bit_field_ref_index);
            gimple_assign_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
            unsigned int gimple_new_tree_node_index = TM->new_tree_node_id();
            TM->create_tree_node(gimple_new_tree_node_index, gimple_assign_K, gimple_assign_tree_node_schema);
            new_stmt_list.push_back(TM->GetTreeReindex(gimple_new_tree_node_index));
            scalar_to_scalar[tree_node_index][scalar] = ssa_tree_node_index;
         }
         break;
      }
      case(SCALAR):
      {
         if(scalar_index != 0 and scalar_to_scalar.find(tn->index) != scalar_to_scalar.end() and scalar_to_scalar.find(tn->index)->second.find(scalar_index) != scalar_to_scalar.find(tn->index)->second.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Already transformed " + STR(tree_node_index) +
                               ((TM->get_tree_node_const(tree_node_index)->get_kind() != function_decl_K) ? ": " + TM->get_tree_node_const(scalar_to_scalar.find(tn->index)->second.find(scalar_index)->second)->ToString() : ""));
            return scalar_to_scalar.find(tn->index)->second.find(scalar_index)->second;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Scalar");
         switch(tn->get_kind())
         {
            case gimple_assign_K:
            {
               const auto* ga = GetPointer<const gimple_assign>(tn);
               if(GET_CONST_NODE(ga->op1)->get_kind() == call_expr_K || GET_CONST_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
               {
                  const auto ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
                  THROW_ASSERT(GET_CONST_NODE(ce->fn)->get_kind() == addr_expr_K, STR(ce->fn));
                  const auto ae = GetPointer<const addr_expr>(GET_CONST_NODE(ce->fn));
                  THROW_ASSERT(GET_CONST_NODE(ae->op)->get_kind() == function_decl_K, STR(ae->op));
                  AppM->GetCallGraphManager()->RemoveCallPoint(function_id, ae->op->index, tn->index);
               }
               CustomUnorderedMap<size_t, unsigned int> scalar_to_ssa;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating replicas of scalar");
               for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
               {
                  std::string include_name = GetPointer<const srcp>(tn)->include_name;
                  unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
                  unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
                  tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  if(ga->orig)
                     tree_node_schema[TOK(TOK_ORIG)] = STR(ga->orig->index);
                  tree_node_schema[TOK(TOK_CLOBBER)] = STR(ga->clobber);
                  tree_node_schema[TOK(TOK_ADDR)] = STR(ga->temporary_address);
                  tree_node_schema[TOK(TOK_INIT)] = STR(ga->init_assignment);
                  tree_node_schema[TOK(TOK_OP1)] = STR(Transform(ga->op1->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
                  tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
                  if(ga->predicate)
                     tree_node_schema[TOK(TOK_PREDICATE)] = STR(Transform(ga->predicate->index, parallel_degree, scalar, new_stmt_list, new_phi_list));

                  unsigned int new_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(new_tree_node_index, gimple_assign_K, tree_node_schema);
                  auto* new_ga = GetPointer<gimple_assign>(TM->get_tree_node_const(new_tree_node_index));
                  new_ga->memuse = ga->memuse;
                  new_ga->memdef = ga->memdef;
                  for(const auto& vuse : ga->vuses)
                  {
                     new_ga->vuses.insert(TM->GetTreeReindex(Transform(vuse->index, parallel_degree, scalar, new_stmt_list, new_phi_list)));
                  }
                  if(ga->vdef)
                  {
                     new_ga->vdef = TM->GetTreeReindex(Transform(ga->vdef->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
                     const auto old_vdef = GetPointer<const ssa_name>(GET_NODE(ga->vdef));
                     for(const auto& use_stmt : old_vdef->CGetUseStmts())
                     {
                        const auto stmt = use_stmt.first;
                        if(transformations.find(stmt->index) == transformations.end())
                        {
                           GetPointer<gimple_node>(GET_NODE(stmt))->vuses.erase(ga->vdef);
                           GetPointer<gimple_node>(GET_NODE(stmt))->vuses.insert(new_ga->vdef);
                        }
                     }
                  }
                  new_ga->vovers = ga->vovers;
                  new_ga->pragmas = ga->pragmas;
                  new_ga->use_set = ga->use_set;
                  new_ga->clobbered_set = ga->clobbered_set;
                  new_stmt_list.push_back(TM->GetTreeReindex(new_tree_node_index));
                  scalar_to_ssa[scalar] = new_ga->op0->index;
                  return_value = new_ga->index;
                  if(GET_CONST_NODE(ga->op1)->get_kind() == call_expr_K || GET_CONST_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
                  {
                     const auto ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
                     THROW_ASSERT(GET_CONST_NODE(ce->fn)->get_kind() == addr_expr_K, STR(ce->fn));
                     const auto ae = GetPointer<const addr_expr>(GET_CONST_NODE(ce->fn));
                     THROW_ASSERT(GET_CONST_NODE(ae->op)->get_kind() == function_decl_K, STR(ae->op));
                     AppM->GetCallGraphManager()->AddCallPoint(function_id, ae->op->index, new_ga->index, FunctionEdgeInfo::CallType::direct_call);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_ga->ToString());
               }
               tree_node_schema.clear();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created replicas of scalar");
               /// Build simd from scalar
               if(transformations[ga->op0->index] == SIMD)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Building constructor for left part");

                  /// Build constructor for right part
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema;
                  constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(tree_helper::CGetType(GET_NODE(ga->op0))->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int constructor_index = TM->new_tree_node_id();
                  TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
                  auto* constr = GetPointer<constructor>(TM->get_tree_node_const(constructor_index));
                  for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
                  {
                     const tree_nodeRef new_ic = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(scalar - 1), TM->new_tree_node_id());
                     constr->add_idx_valu(new_ic, TM->GetTreeReindex(scalar_to_ssa[scalar]));
                  }

                  std::string include_name = GetPointer<const srcp>(tn)->include_name;
                  unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
                  unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
                  tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  if(ga->orig)
                     tree_node_schema[TOK(TOK_ORIG)] = STR(ga->orig->index);
                  tree_node_schema[TOK(TOK_CLOBBER)] = STR(ga->clobber);
                  tree_node_schema[TOK(TOK_INIT)] = STR(ga->init_assignment);
                  tree_node_schema[TOK(TOK_OP1)] = STR(constr->index);
                  tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int new_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(new_tree_node_index, gimple_assign_K, tree_node_schema);
                  new_stmt_list.push_back(TM->GetTreeReindex(new_tree_node_index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built constructor for left part");
               }
               break;
            }
            case CASE_UNARY_EXPRESSION:
            {
               const auto* ue = GetPointer<const unary_expr>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(ue->type->index);
               tree_node_schema[TOK(TOK_OP)] = STR(Transform(ue->op->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               std::string include_name = ue->include_name;
               unsigned int line_number = ue->line_number;
               unsigned int column_number = ue->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case CASE_BINARY_EXPRESSION:
            {
               const auto* be = GetPointer<const binary_expr>(tn);
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(be->op0->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               std::string include_name = be->include_name;
               unsigned int line_number = be->line_number;
               unsigned int column_number = be->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               tree_node_schema[TOK(TOK_TYPE)] = STR(be->type->index);
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(be->op1->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case CASE_TERNARY_EXPRESSION:
            {
               const auto* te = GetPointer<const ternary_expr>(tn);
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(te->op0->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               std::string include_name = te->include_name;
               unsigned int line_number = te->line_number;
               unsigned int column_number = te->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               tree_node_schema[TOK(TOK_TYPE)] = STR(te->type->index);
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(te->op1->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP2)] = STR(Transform(te->op2->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case lut_expr_K:
            {
               const auto* le = GetPointer<const lut_expr>(tn);
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(le->op0->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               std::string include_name = le->include_name;
               unsigned int line_number = le->line_number;
               unsigned int column_number = le->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               tree_node_schema[TOK(TOK_TYPE)] = STR(le->type->index);
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(le->op1->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op2)
                  tree_node_schema[TOK(TOK_OP2)] = STR(Transform(le->op2->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op3)
                  tree_node_schema[TOK(TOK_OP3)] = STR(Transform(le->op3->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op4)
                  tree_node_schema[TOK(TOK_OP4)] = STR(Transform(le->op4->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op5)
                  tree_node_schema[TOK(TOK_OP5)] = STR(Transform(le->op5->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op6)
                  tree_node_schema[TOK(TOK_OP6)] = STR(Transform(le->op6->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op7)
                  tree_node_schema[TOK(TOK_OP7)] = STR(Transform(le->op7->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(le->op8)
                  tree_node_schema[TOK(TOK_OP8)] = STR(Transform(le->op8->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case array_ref_K:
            {
               const auto* qe = GetPointer<const quaternary_expr>(tn);
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(qe->op0->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               std::string include_name = qe->include_name;
               unsigned int line_number = qe->line_number;
               unsigned int column_number = qe->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               tree_node_schema[TOK(TOK_TYPE)] = STR(qe->type->index);
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(qe->op0->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(qe->op1->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(qe->op2)
                  tree_node_schema[TOK(TOK_OP2)] = STR(Transform(qe->op2->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               if(qe->op3)
                  tree_node_schema[TOK(TOK_OP3)] = STR(Transform(qe->op3->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }

            case target_mem_ref461_K:
            {
               const auto* tmr = GetPointer<const target_mem_ref461>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(tmr->type->index);
               if(tmr->base)
               {
                  tree_node_schema[TOK(TOK_BASE)] = STR(Transform(tmr->base->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               }
               if(tmr->offset)
               {
                  tree_node_schema[TOK(TOK_OFFSET)] = STR(Transform(tmr->offset->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               }
               if(tmr->idx)
               {
                  tree_node_schema[TOK(TOK_IDX)] = STR(Transform(tmr->idx->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               }
               if(tmr->step)
               {
                  tree_node_schema[TOK(TOK_STEP)] = STR(Transform(tmr->step->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               }
               if(tmr->idx2)
               {
                  tree_node_schema[TOK(TOK_IDX2)] = STR(Transform(tmr->idx2->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list));
               }
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, target_mem_ref461_K, tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case CASE_CST_NODES:
            {
               return_value = tree_node_index;
               break;
            }
            case var_decl_K:
            {
               return_value = tree_node_index;
               break;
            }
            case ssa_name_K:
            {
               if(transformations.find(tn->index) != transformations.end())
               {
                  const auto* sa = GetPointer<const ssa_name>(tn);
                  if(sa->type)
                     tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
                  if(sa->var)
                     tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
                  tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
                  tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
                  if(sa->volatile_flag)
                     tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
                  if(sa->virtual_flag)
                     tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
                  if(sa->max)
                     tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
                  if(sa->min)
                     tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);

                  unsigned int ssa_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(ssa_tree_node_index, ssa_name_K, tree_node_schema);
                  return_value = ssa_tree_node_index;
               }
               else
               {
                  return_value = tree_node_index;
               }
               break;
            }
            case call_expr_K:
            case aggr_init_expr_K:
            {
               const auto* ce = GetPointer<const call_expr>(tn);
               std::string include_name = ce->include_name;
               unsigned int line_number = ce->line_number;
               unsigned int column_number = ce->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               tree_node_schema[TOK(TOK_TYPE)] = STR(ce->type->index);
               tree_node_schema[TOK(TOK_FN)] = STR(ce->fn->index);
               unsigned int call_expr_tree_node_index = TM->new_tree_node_id();
               TM->create_tree_node(call_expr_tree_node_index, call_expr_K, tree_node_schema);
               auto* new_ce = GetPointer<call_expr>(TM->get_tree_node_const(call_expr_tree_node_index));
               for(const auto& arg : ce->args)
               {
                  new_ce->AddArg(TM->GetTreeReindex(Transform(arg->index, parallel_degree, scalar_index, new_stmt_list, new_phi_list)));
               }
               return_value = call_expr_tree_node_index;
               break;
            }
            case const_decl_K:
            case field_decl_K:
            case function_decl_K:
            case label_decl_K:
            case namespace_decl_K:
            case parm_decl_K:
            case result_decl_K:
            case translation_unit_decl_K:
            case template_decl_K:
            case error_mark_K:
            case using_decl_K:
            case type_decl_K:
            case binfo_K:
            case block_K:
            case case_label_expr_K:
            case constructor_K:
            case identifier_node_K:
            case statement_list_K:
            case target_mem_ref_K:
            case tree_list_K:
            case tree_vec_K:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case gimple_asm_K:
            case gimple_bind_K:
            case gimple_call_K:
            case gimple_cond_K:
            case gimple_for_K:
            case gimple_goto_K:
            case gimple_label_K:
            case gimple_multi_way_if_K:
            case gimple_nop_K:
            case gimple_phi_K:
            case gimple_pragma_K:
            case gimple_predict_K:
            case gimple_resx_K:
            case gimple_return_K:
            case gimple_switch_K:
            case gimple_while_K:
            case CASE_PRAGMA_NODES:
            case array_range_ref_K:
            case target_expr_K:
            case CASE_TYPE_NODES:
            {
               THROW_UNREACHABLE("Not supported tree node " + tn->get_kind_text());
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         scalar_to_scalar[tree_node_index][scalar_index] = return_value;
         break;
      }
      case(SIMD):
      {
         if(scalar_to_vector.find(tree_node_index) != scalar_to_vector.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Already transformed " + STR(tree_node_index) + ((TM->get_tree_node_const(tree_node_index)->get_kind() != function_decl_K) ? ": " + TM->get_tree_node_const(tree_node_index)->ToString() : ""));
            return scalar_to_vector.find(tree_node_index)->second;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simd");
         switch(tn->get_kind())
         {
            case gimple_assign_K:
            {
               const auto* ga = GetPointer<const gimple_assign>(tn);
               std::string include_name = GetPointer<const srcp>(tn)->include_name;
               unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
               unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               if(ga->orig)
                  tree_node_schema[TOK(TOK_ORIG)] = STR(ga->orig->index);
               tree_node_schema[TOK(TOK_CLOBBER)] = STR(ga->clobber);
               tree_node_schema[TOK(TOK_ADDR)] = STR(ga->temporary_address);
               tree_node_schema[TOK(TOK_INIT)] = STR(ga->init_assignment);
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(ga->op1->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               THROW_ASSERT(transformations.find(ga->op1->index) == transformations.end() or transformations.find(ga->op1->index)->second == SIMD, "Composition not yet implemented " + STR(tn));
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_index = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_index, gimple_assign_K, tree_node_schema);
               auto* new_ga = GetPointer<gimple_assign>(TM->get_tree_node_const(new_tree_node_index));
               new_ga->memuse = ga->memuse;
               new_ga->memdef = ga->memdef;
               for(const auto& vuse : ga->vuses)
               {
                  new_ga->vuses.insert(TM->GetTreeReindex(Transform(vuse->index, parallel_degree, 0, new_stmt_list, new_phi_list)));
               }
               if(new_ga->vdef)
               {
                  new_ga->vdef = TM->GetTreeReindex(Transform(ga->vdef->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               }
               new_ga->vovers = ga->vovers;
               new_ga->pragmas = ga->pragmas;
               new_ga->use_set = ga->use_set;
               new_ga->clobbered_set = ga->clobbered_set;
               return_value = new_tree_node_index;
               THROW_ASSERT(GET_NODE(new_ga->op0)->get_kind() == ssa_name_K, "Left operand is not ssa");
               new_stmt_list.push_back(TM->GetTreeReindex(return_value));
               for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
               {
                  /// Build bit_field_ref to extract the scalar
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> bit_field_ref_tree_node_schema, gimple_assign_tree_node_schema;
                  unsigned int bit_field_ref_index = TM->new_tree_node_id();
                  const auto element_type = tree_helper::CGetType(GET_NODE(ga->op0));
                  /// vector of boolean types are mapped on vector of integer
                  const unsigned int bit_size = element_type->get_kind() != boolean_type_K ? tree_helper::Size(element_type) : 32;
                  const tree_nodeRef offset = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>((scalar - 1) * bit_size), TM->new_tree_node_id());
                  const tree_nodeRef size = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(bit_size), TM->new_tree_node_id());
                  bit_field_ref_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  bit_field_ref_tree_node_schema[TOK(TOK_TYPE)] = STR(tree_helper::CGetType(tn)->index);
                  bit_field_ref_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  bit_field_ref_tree_node_schema[TOK(TOK_OP1)] = STR(size->index);
                  bit_field_ref_tree_node_schema[TOK(TOK_OP2)] = STR(offset->index);
                  TM->create_tree_node(bit_field_ref_index, bit_field_ref_K, bit_field_ref_tree_node_schema);

                  gimple_assign_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  gimple_assign_tree_node_schema[TOK(TOK_OP1)] = STR(bit_field_ref_index);
                  gimple_assign_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(ga->op0->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
                  unsigned int gimple_new_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(gimple_new_tree_node_index, gimple_assign_K, gimple_assign_tree_node_schema);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + TM->get_tree_node_const(gimple_new_tree_node_index)->ToString());
                  new_stmt_list.push_back(TM->GetTreeReindex(gimple_new_tree_node_index));
                  scalar_to_scalar[tree_node_index][scalar] = return_value;
               }
               break;
            }
            case ssa_name_K:
            {
               const auto* sa = GetPointer<const ssa_name>(tn);
               if(sa->type)
                  tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(sa->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(sa->var)
                  tree_node_schema[TOK(TOK_VAR)] = STR(Transform(sa->var->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
               tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
               if(sa->volatile_flag)
                  tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
               if(sa->virtual_flag)
                  tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
               if(sa->max)
                  tree_node_schema[TOK(TOK_MAX)] = STR(Transform(sa->max->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(sa->min)
                  tree_node_schema[TOK(TOK_MIN)] = STR(Transform(sa->min->index, parallel_degree, 0, new_stmt_list, new_phi_list));

               unsigned int new_tree_node_index = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_index, ssa_name_K, tree_node_schema);
               return_value = new_tree_node_index;
               break;
            }
            case CASE_TYPE_NODES:
            {
               const auto* type = GetPointer<const type_node>(tn);
               tree_node_schema[TOK(TOK_QUAL)] = STR(static_cast<unsigned int>(type->qual));
               if(type->unql)
                  tree_node_schema[TOK(TOK_UNQL)] = STR(Transform(type->unql->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(type->scpe)
                  tree_node_schema[TOK(TOK_SCPE)] = STR(Transform(type->scpe->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(type->packed_flag)
                  tree_node_schema[TOK(TOK_PACKED)] = STR(type->packed_flag);
               tree_node_schema[TOK(TOK_SYSTEM)] = STR(false);
               if(type->algn)
                  tree_node_schema[TOK(TOK_ALGN)] = STR(type->algn);
               switch(type->get_kind())
               {
                  case pointer_type_K:
#if 0
                           {
                              const pointer_type * pt = GetPointer<const pointer_type>(tn);
                              tree_node_schema[TOK(TOK_PTD)] = STR(Transform(pt->ptd->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                              if(type->size)
                                 tree_node_schema[TOK(TOK_SIZE)] = STR(type->size->index);
                              if(type->name)
                                 tree_node_schema[TOK(TOK_NAME)] = STR(Transform(type->name->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                              unsigned int new_tree_node_id = TM->new_tree_node_id();
                              TM->create_tree_node(new_tree_node_id, pointer_type_K, tree_node_schema);
                              return_value = new_tree_node_id;
                              break;
                           }
#endif
                  case boolean_type_K:
                  case integer_type_K:
                  case void_type_K:
                  {
                     tree_node_schema[TOK(TOK_ELTS)] = type->get_kind() != boolean_type_K ? STR(type->index) : STR(tree_man->create_default_unsigned_integer_type()->index);
                     if(type->size)
                     {
                        const unsigned int element_size = type->get_kind() != boolean_type_K ? tree_helper::Size(tn) : tree_helper::Size(GET_NODE(tree_man->create_default_unsigned_integer_type()));
                        const unsigned int bit_size = element_size * static_cast<unsigned int>(parallel_degree);
                        const tree_nodeRef size = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(bit_size), TM->new_tree_node_id());
                        tree_node_schema[TOK(TOK_SIZE)] = STR(size->index);
                        tree_node_schema[TOK(TOK_ALGN)] = STR(bit_size);
                     }
                     unsigned int new_tree_node_id = TM->new_tree_node_id();
                     TM->create_tree_node(new_tree_node_id, vector_type_K, tree_node_schema);
                     return_value = new_tree_node_id;
                     /// Workaround due to cycle in the tree between void_type and type_decl
                     auto* new_type_node = GetPointer<type_node>(TM->get_tree_node_const(new_tree_node_id));
                     if(new_type_node->name and GET_NODE(new_type_node->name)->get_kind() == type_decl_K)
                     {
                        auto* td = GetPointer<type_decl>(GET_NODE(new_type_node->name));
                        td->type = TM->GetTreeReindex(new_tree_node_id);
                     }
                     break;
                  }
                  case array_type_K:
                  case CharType_K:
                  case nullptr_type_K:
                  case type_pack_expansion_K:
                  case complex_type_K:
                  case enumeral_type_K:
                  case function_type_K:
                  case lang_type_K:
                  case method_type_K:
                  case offset_type_K:
                  case qual_union_type_K:
                  case real_type_K:
                  case record_type_K:
                  case reference_type_K:
                  case set_type_K:
                  case template_type_parm_K:
                  case typename_type_K:
                  case type_argument_pack_K:
                  case union_type_K:
                  case vector_type_K:
                  case binfo_K:
                  case block_K:
                  case call_expr_K:
                  case aggr_init_expr_K:
                  case case_label_expr_K:
                  case constructor_K:
                  case identifier_node_K:
                  case ssa_name_K:
                  case statement_list_K:
                  case target_expr_K:
                  case target_mem_ref_K:
                  case target_mem_ref461_K:
                  case tree_list_K:
                  case tree_vec_K:
                  case error_mark_K:
                  case lut_expr_K:
                  case CASE_BINARY_EXPRESSION:
                  case CASE_CPP_NODES:
                  case CASE_CST_NODES:
                  case CASE_DECL_NODES:
                  case CASE_FAKE_NODES:
                  case CASE_GIMPLE_NODES:
                  case CASE_PRAGMA_NODES:
                  case CASE_QUATERNARY_EXPRESSION:
                  case CASE_TERNARY_EXPRESSION:
                  case CASE_UNARY_EXPRESSION:
                  {
                     THROW_UNREACHABLE("Not supported tree node " + tn->get_kind_text());
                     break;
                  }
                  default:
                  {
                     THROW_UNREACHABLE("");
                  }
               }
               break;
            }
            case var_decl_K:
            case type_decl_K:
            {
               const auto* dn = GetPointer<const decl_node>(tn);
               if(dn->name)
                  tree_node_schema[TOK(TOK_NAME)] = STR(Transform(dn->name->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(dn->mngl)
                  tree_node_schema[TOK(TOK_MNGL)] = STR(Transform(dn->mngl->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(dn->orig)
                  tree_node_schema[TOK(TOK_ORIG)] = STR(dn->orig->index);
               if(dn->type)
               {
                  if(dn->get_kind() != type_decl_K)
                  {
                     tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(dn->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  }
               }
               if(dn->scpe)
                  tree_node_schema[TOK(TOK_SCPE)] = STR(Transform(dn->scpe->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(dn->attributes)
                  tree_node_schema[TOK(TOK_ATTRIBUTES)] = STR(dn->attributes->index);
               if(dn->chan)
                  tree_node_schema[TOK(TOK_CHAN)] = STR(Transform(dn->chan->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(dn->artificial_flag)
                  tree_node_schema[TOK(TOK_ARTIFICIAL)] = STR(dn->artificial_flag);
               if(dn->packed_flag)
                  tree_node_schema[TOK(TOK_PACKED)] = STR(dn->packed_flag);
               if(dn->operating_system_flag)
                  tree_node_schema[TOK(TOK_OPERATING_SYSTEM)] = STR(dn->operating_system_flag);
               if(dn->library_system_flag)
                  tree_node_schema[TOK(TOK_LIBRARY_SYSTEM)] = STR(dn->library_system_flag);
               if(dn->libbambu_flag)
                  tree_node_schema[TOK(TOK_LIBBAMBU)] = STR(dn->libbambu_flag);
               if(dn->C_flag)
                  tree_node_schema[TOK(TOK_C)] = STR(dn->C_flag);
               std::string include_name = dn->get_kind() == type_decl_K and dn->include_name == "<built-in>" ? "<new>" : dn->include_name;
               unsigned int line_number = dn->line_number;
               unsigned int column_number = dn->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               switch(tn->get_kind())
               {
                  case(type_decl_K):
                  {
                     const auto* td = GetPointer<const type_decl>(tn);
                     if(td->tmpl_parms)
                        tree_node_schema[TOK(TOK_TMPL_PARMS)] = STR(Transform(td->tmpl_parms->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                     if(td->tmpl_args)
                        tree_node_schema[TOK(TOK_TMPL_ARGS)] = STR(Transform(td->tmpl_args->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                     unsigned int new_tree_node_id = TM->new_tree_node_id();
                     TM->create_tree_node(new_tree_node_id, type_decl_K, tree_node_schema);
                     return_value = new_tree_node_id;
                     break;
                  }
                  case var_decl_K:
                  {
                     const auto* vd = GetPointer<const var_decl>(tn);
                     if(vd->use_tmpl)
                        tree_node_schema[TOK(TOK_USE_TMPL)] = STR(vd->use_tmpl);
                     if(vd->static_static_flag)
                        tree_node_schema[TOK(TOK_STATIC_STATIC)] = STR(vd->static_static_flag);
                     if(vd->extern_flag)
                        tree_node_schema[TOK(TOK_EXTERN)] = STR(vd->extern_flag);
                     if(vd->static_flag)
                        tree_node_schema[TOK(TOK_STATIC)] = STR(vd->static_flag);
                     if(vd->init)
                        tree_node_schema[TOK(TOK_INIT)] = STR(Transform(vd->init->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                     if(vd->size)
                     {
                        /// Size of new type
                        const auto* type = GetPointer<const type_node>(TM->get_tree_node_const(Transform(dn->type->index, parallel_degree, 0, new_stmt_list, new_phi_list)));
                        tree_node_schema[TOK(TOK_SIZE)] = STR(type->size->index);
                     }
                     if(vd->algn)
                        tree_node_schema[TOK(TOK_ALGN)] = STR(vd->algn);
                     if(vd->used)
                        tree_node_schema[TOK(TOK_USED)] = STR(vd->used);
                     if(vd->register_flag)
                        tree_node_schema[TOK(TOK_REGISTER)] = STR(vd->register_flag);
                     if(vd->smt_ann)
                        tree_node_schema[TOK(TOK_SMT_ANN)] = STR(Transform(vd->smt_ann->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                     unsigned int new_tree_node_id = TM->new_tree_node_id();
                     TM->create_tree_node(new_tree_node_id, var_decl_K, tree_node_schema);
                     return_value = new_tree_node_id;
                     break;
                  }
                  case const_decl_K:
                  case field_decl_K:
                  case function_decl_K:
                  case label_decl_K:
                  case namespace_decl_K:
                  case parm_decl_K:
                  case result_decl_K:
                  case translation_unit_decl_K:
                  case error_mark_K:
                  case using_decl_K:
                  case template_decl_K:
                  case binfo_K:
                  case block_K:
                  case call_expr_K:
                  case aggr_init_expr_K:
                  case case_label_expr_K:
                  case constructor_K:
                  case identifier_node_K:
                  case ssa_name_K:
                  case statement_list_K:
                  case target_expr_K:
                  case target_mem_ref_K:
                  case target_mem_ref461_K:
                  case tree_list_K:
                  case tree_vec_K:
                  case lut_expr_K:
                  case CASE_BINARY_EXPRESSION:
                  case CASE_CPP_NODES:
                  case CASE_CST_NODES:
                  case CASE_FAKE_NODES:
                  case CASE_GIMPLE_NODES:
                  case CASE_PRAGMA_NODES:
                  case CASE_QUATERNARY_EXPRESSION:
                  case CASE_TERNARY_EXPRESSION:
                  case CASE_TYPE_NODES:
                  case CASE_UNARY_EXPRESSION:
                  {
                     THROW_UNREACHABLE("Not supported tree node " + tn->get_kind_text());
                     break;
                  }
                  default:
                  {
                     THROW_UNREACHABLE("");
                  }
               }
               break;
            }
            case using_decl_K:
            case translation_unit_decl_K:
            {
               return_value = tree_node_index;
               break;
            }
            case identifier_node_K:
            {
               const auto* in = GetPointer<const identifier_node>(tn);
               if(in->operator_flag)
                  tree_node_schema[TOK(TOK_OPERATOR)] = STR(in->operator_flag);
               else
                  tree_node_schema[TOK(TOK_STRG)] = "vector_" + boost::replace_all_copy(in->strg, " ", "_");
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, identifier_node_K, tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case function_decl_K:
            {
               /// FIXME: check if the function has to parallelized
               return_value = tree_node_index;
               break;
            }
            case CASE_UNARY_EXPRESSION:
            {
               const auto* ue = GetPointer<const unary_expr>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(ue->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP)] = STR(Transform(ue->op->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               THROW_ASSERT(transformations.find(ue->op->index) == transformations.end() or transformations.find(ue->op->index)->second != SCALAR, "");
               std::string include_name = ue->include_name;
               unsigned int line_number = ue->line_number;
               unsigned int column_number = ue->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case CASE_BINARY_EXPRESSION:
            {
               const auto* be = GetPointer<const binary_expr>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(be->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(be->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               if(tn->get_kind() == lshift_expr_K or tn->get_kind() == rshift_expr_K)
               {
                  tree_node_schema[TOK(TOK_OP1)] = STR(be->op1->index);
               }
               else if(transformations.find(be->op1->index) == transformations.end() or transformations.find(be->op1->index)->second != SCALAR)
               {
                  tree_node_schema[TOK(TOK_OP1)] = STR(Transform(be->op1->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Building constructor for second operand");

                  /// Build constructor
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema, temp_tree_node_schema, ssa_tree_node_schema;
                  constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(be->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int constructor_index = TM->new_tree_node_id();
                  TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
                  auto* constr = GetPointer<constructor>(TM->get_tree_node_const(constructor_index));
                  for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
                  {
                     const tree_nodeRef new_ic = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(scalar - 1), TM->new_tree_node_id());
                     constr->add_idx_valu(new_ic, TM->GetTreeReindex(be->op1->index));
                  }

                  std::string include_name = GetPointer<const srcp>(tn)->include_name;
                  unsigned int line_number = GetPointer<const srcp>(tn)->line_number;
                  unsigned int column_number = GetPointer<const srcp>(tn)->column_number;
                  temp_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                  temp_tree_node_schema[TOK(TOK_OP1)] = STR(constr->index);
                  THROW_ASSERT(GET_NODE(be->op1)->get_kind() == ssa_name_K, "Unexpected operand " + GET_NODE(be->op1)->get_kind_text());
                  const auto* sa = GetPointer<const ssa_name>(GET_NODE(be->op1));
                  if(sa->type)
                     ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(sa->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  if(sa->var)
                     ssa_tree_node_schema[TOK(TOK_VAR)] = STR(Transform(sa->var->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
                  ssa_tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
                  if(sa->volatile_flag)
                     ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
                  if(sa->virtual_flag)
                     ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
                  if(sa->max)
                     ssa_tree_node_schema[TOK(TOK_MAX)] = STR(Transform(sa->max->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  if(sa->min)
                     ssa_tree_node_schema[TOK(TOK_MIN)] = STR(Transform(sa->min->index, parallel_degree, 0, new_stmt_list, new_phi_list));

                  unsigned int ssa_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);
                  temp_tree_node_schema[TOK(TOK_OP0)] = STR(ssa_tree_node_index);
                  temp_tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(tree_helper::CGetType(GET_NODE(be->op1))->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                  unsigned int temp_tree_node_index = TM->new_tree_node_id();
                  TM->create_tree_node(temp_tree_node_index, gimple_assign_K, temp_tree_node_schema);
                  auto* new_ga = GetPointer<gimple_assign>(TM->get_tree_node_const(temp_tree_node_index));
                  new_stmt_list.push_back(TM->GetTreeReindex(temp_tree_node_index));
                  tree_node_schema[TOK(TOK_OP1)] = STR(new_ga->op0->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built constructor for second operand");
               }
               THROW_ASSERT(transformations.find(be->op0->index) == transformations.end() or transformations.find(be->op0->index)->second != SCALAR, "");
               std::string include_name = be->include_name;
               unsigned int line_number = be->line_number;
               unsigned int column_number = be->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               if(tn->get_kind() == truth_or_expr_K)
                  TM->create_tree_node(new_tree_node_id, bit_ior_expr_K, tree_node_schema);
               else if(tn->get_kind() == truth_and_expr_K)
                  TM->create_tree_node(new_tree_node_id, bit_and_expr_K, tree_node_schema);
               else if(tn->get_kind() == lshift_expr_K)
                  TM->create_tree_node(new_tree_node_id, vec_lshift_expr_K, tree_node_schema);
               else if(tn->get_kind() == rshift_expr_K)
                  TM->create_tree_node(new_tree_node_id, vec_rshift_expr_K, tree_node_schema);
               else
                  TM->create_tree_node(new_tree_node_id, tn->get_kind(), tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case CASE_CST_NODES:
            {
               const auto* cn = GetPointer<const cst_node>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(cn->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, vector_cst_K, tree_node_schema);
               auto* new_tn = GetPointer<vector_cst>(TM->get_tree_node_const(new_tree_node_id));
               for(size_t i = 0; i < parallel_degree; i++)
                  new_tn->list_of_valu.push_back(TM->GetTreeReindex(tn->index));
               return_value = new_tree_node_id;
               break;
            }
            case gimple_phi_K:
            {
               const auto* gp = GetPointer<const gimple_phi>(tn);
               tree_node_schema[TOK(TOK_SCPE)] = STR(Transform(gp->scpe->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_RES)] = STR(Transform(gp->res->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
               std::string include_name = gp->include_name;
               unsigned int line_number = gp->line_number;
               unsigned int column_number = gp->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, gimple_phi_K, tree_node_schema);
               auto* new_gp = GetPointer<gimple_phi>(TM->get_tree_node_const(new_tree_node_id));
               new_gp->SetSSAUsesComputed();
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  THROW_ASSERT(gp->virtual_flag or transformations.find(def_edge.first->index) == transformations.end() or transformations.find(def_edge.first->index)->second != SCALAR, "");
                  if(transformations.find(def_edge.first->index) == transformations.end() or transformations.find(def_edge.first->index)->second == SIMD)
                     new_gp->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeReindex(Transform(def_edge.first->index, parallel_degree, 0, new_stmt_list, new_phi_list)), def_edge.second));
                  else
                     new_gp->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeReindex(def_edge.first->index), def_edge.second));
               }
               return_value = new_tree_node_id;
               new_phi_list.push_back(TM->GetTreeReindex(return_value));
               if(not gp->virtual_flag)
               {
                  /// Creating scalar from simd

                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating scalar from simd");
                  for(size_t scalar = 1; scalar <= parallel_degree; scalar++)
                  {
                     /// Build bit_field_ref to extract the scalar
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> bit_field_ref_tree_node_schema, ssa_tree_node_schema, gimple_assign_tree_node_schema;
                     unsigned int bit_field_ref_index = TM->new_tree_node_id();
                     const auto element_type = tree_helper::CGetType(GET_NODE(gp->res));
                     /// vector of boolean types are mapped on vector of integer
                     const unsigned int bit_size = element_type->get_kind() != boolean_type_K ? tree_helper::Size(element_type) : 32;
                     const tree_nodeRef offset = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>((scalar - 1) * bit_size), TM->new_tree_node_id());
                     const tree_nodeRef size = tree_man->CreateIntegerCst(tree_man->create_default_unsigned_integer_type(), static_cast<long long int>(bit_size), TM->new_tree_node_id());
                     bit_field_ref_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                     bit_field_ref_tree_node_schema[TOK(TOK_TYPE)] = STR(tree_helper::CGetType(GET_NODE(gp->res))->index);
                     bit_field_ref_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(gp->res->index, parallel_degree, 0, new_stmt_list, new_phi_list));
                     bit_field_ref_tree_node_schema[TOK(TOK_OP1)] = STR(size->index);
                     bit_field_ref_tree_node_schema[TOK(TOK_OP2)] = STR(offset->index);
                     TM->create_tree_node(bit_field_ref_index, bit_field_ref_K, bit_field_ref_tree_node_schema);

                     const auto* sa = GetPointer<const ssa_name>(GET_NODE(gp->res));
                     if(sa->type)
                        ssa_tree_node_schema[TOK(TOK_TYPE)] = STR(sa->type->index);
                     if(sa->var)
                        ssa_tree_node_schema[TOK(TOK_VAR)] = STR(sa->var->index);
                     ssa_tree_node_schema[TOK(TOK_VERS)] = STR(TM->get_next_vers());
                     ssa_tree_node_schema[TOK(TOK_ORIG_VERS)] = STR(sa->orig_vers);
                     if(sa->volatile_flag)
                        ssa_tree_node_schema[TOK(TOK_VOLATILE)] = STR(sa->volatile_flag);
                     if(sa->virtual_flag)
                        ssa_tree_node_schema[TOK(TOK_VIRTUAL)] = STR(sa->virtual_flag);
                     if(sa->max)
                        ssa_tree_node_schema[TOK(TOK_MAX)] = STR(sa->max->index);
                     if(sa->min)
                        ssa_tree_node_schema[TOK(TOK_MIN)] = STR(sa->min->index);

                     unsigned int ssa_tree_node_index = TM->new_tree_node_id();
                     TM->create_tree_node(ssa_tree_node_index, ssa_name_K, ssa_tree_node_schema);

                     gimple_assign_tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
                     gimple_assign_tree_node_schema[TOK(TOK_OP1)] = STR(bit_field_ref_index);
                     gimple_assign_tree_node_schema[TOK(TOK_OP0)] = STR(Transform(gp->res->index, parallel_degree, scalar, new_stmt_list, new_phi_list));
                     unsigned int gimple_new_tree_node_index = TM->new_tree_node_id();
                     TM->create_tree_node(gimple_new_tree_node_index, gimple_assign_K, gimple_assign_tree_node_schema);
                     /// Split of phi node goes to the beginning of the list of statement
                     new_stmt_list.push_front(TM->GetTreeReindex(gimple_new_tree_node_index));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created scalar from simd");
               }
               break;
            }
            case cond_expr_K:
            {
               const auto* te = GetPointer<const ternary_expr>(tn);
               tree_node_schema[TOK(TOK_TYPE)] = STR(Transform(te->type->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP2)] = STR(Transform(te->op2->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP1)] = STR(Transform(te->op1->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               tree_node_schema[TOK(TOK_OP0)] = STR(Transform(te->op0->index, parallel_degree, 0, new_stmt_list, new_phi_list));
               THROW_ASSERT(transformations.find(te->op2->index) == transformations.end() or transformations.find(te->op2->index)->second != SCALAR, "");
               THROW_ASSERT(transformations.find(te->op1->index) == transformations.end() or transformations.find(te->op1->index)->second != SCALAR, "");
               THROW_ASSERT(transformations.find(te->op0->index) == transformations.end() or transformations.find(te->op0->index)->second != SCALAR, "");
               std::string include_name = te->include_name;
               unsigned int line_number = te->line_number;
               unsigned int column_number = te->column_number;
               tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);
               unsigned int new_tree_node_id = TM->new_tree_node_id();
               TM->create_tree_node(new_tree_node_id, vec_cond_expr_K, tree_node_schema);
               return_value = new_tree_node_id;
               break;
            }
            case const_decl_K:
            case field_decl_K:
            case label_decl_K:
            case namespace_decl_K:
            case parm_decl_K:
            case result_decl_K:
            case template_decl_K:
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case constructor_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case gimple_asm_K:
            case gimple_bind_K:
            case gimple_call_K:
            case gimple_cond_K:
            case gimple_for_K:
            case gimple_goto_K:
            case gimple_label_K:
            case gimple_multi_way_if_K:
            case gimple_nop_K:
            case gimple_pragma_K:
            case gimple_predict_K:
            case gimple_resx_K:
            case gimple_return_K:
            case gimple_switch_K:
            case gimple_while_K:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case component_ref_K:
            case bit_field_ref_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case obj_type_ref_K:
            case save_expr_K:
            case vec_cond_expr_K:
            case vec_perm_expr_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case lut_expr_K:
            case bit_ior_concat_expr_K:
            case error_mark_K:
            {
               if(debug_level >= DEBUG_LEVEL_PEDANTIC)
               {
                  PrintTreeManager(false);
               }
               THROW_UNREACHABLE("Not supported tree node " + tn->get_kind_text());
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         scalar_to_vector[tree_node_index] = return_value;
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Transformed " + TM->get_tree_node_const(tree_node_index)->get_kind_text() + " " + STR(tree_node_index) +
                      ((TM->get_tree_node_const(return_value)->get_kind() != function_decl_K) ? ": " + TM->get_tree_node_const(return_value)->ToString() : ""));
   return return_value;
}

bool Vectorize::HasToBeExecuted() const
{
   if(bb_version != 0)
   {
      return false;
   }
   if(function_behavior->CGetLoops())
   {
      for(const auto& loop : function_behavior->CGetLoops()->GetList())
      {
         if(loop->loop_type & DOALL_LOOP)
         {
            return true;
         }
      }
   }
   return false;
}
