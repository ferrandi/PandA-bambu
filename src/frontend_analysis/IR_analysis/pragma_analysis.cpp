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
 * @file pragma_analysis.cpp
 * @brief Pre-analysis step fixing pragma definitions
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "pragma_analysis.hpp"

#include "Parameter.hpp"
#include "PragmaParser.hpp"
#include "application_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "pragma_constants.hpp"
#include "pragma_manager.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <fstream>
#include <string>

PragmaAnalysis::PragmaAnalysis(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, PRAGMA_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
PragmaAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
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

std::list<tree_nodeRef> OpenParallelSections;

std::string PragmaAnalysis::get_call_parameter(const tree_nodeRef& tn, unsigned int idx) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Asking parameter " + std::to_string(idx) + " " + STR(tn->index));
   const auto ce = GetPointer<gimple_call>(tn);
   if(idx >= ce->args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not found");
      return "";
   }
   const auto ae = GetPointer<const addr_expr>(ce->args.at(idx));
   THROW_ASSERT(ae, "Argument of call is not addr_expr: " + ce->args.at(idx)->get_kind_text());
   std::string string_arg;
   if(ae->op->get_kind() == var_decl_K)
   {
      auto vd = GetPointer<const var_decl>(ae->op);
      THROW_ASSERT(vd, "unexpected condition");
      THROW_ASSERT(vd->init, "unexpected condition");
      auto vd_init = vd->init;
      if(vd_init->get_kind() == constructor_K)
      {
         const auto co = GetPointer<const constructor>(vd_init);
         for(const auto& idx_valu : co->list_of_idx_valu)
         {
            THROW_ASSERT(idx_valu.second->get_kind() == integer_cst_K, "unexpected condition");
            const auto cst_val = tree_helper::GetConstValue(idx_valu.second);
            char val = static_cast<char>(cst_val);
            if(!val)
            {
               break;
            }
            string_arg.push_back(val);
         }
      }
      else
      {
         THROW_ERROR("unexpected condition");
      }
   }
   else
   {
      const auto sc = GetPointer<const string_cst>(
          ae->op->get_kind() == string_cst_K ? ae->op : GetPointer<const array_ref>(ae->op)->op0);
      string_arg = sc->strg;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parameter is " + string_arg);
   return string_arg;
}

tree_nodeRef PragmaAnalysis::create_omp_pragma(const tree_nodeRef& tn) const
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const pragma_managerRef PM = AppM->get_pragma_manager();
   const auto gc = GetPointer<const gimple_call>(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating openmp pragma starting from " + tn->ToString());
   const auto fn = GetPointer<addr_expr>(gc->fn)->op;
   const auto name = GetPointer<function_decl>(fn)->name;
   const auto function_name = GetPointer<identifier_node>(name)->strg;

   const pragma_manager::OmpPragmaType directive = pragma_manager::GetOmpPragmaType(get_call_parameter(tn, 1));

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   tree_node_schema[TOK(TOK_SRCP)] =
       gc->include_name + ":" + std::to_string(gc->line_number) + ":" + std::to_string(gc->column_number);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tn_schema;
   tree_nodeRef d_node = nullptr;
   const auto s_node = TM->create_tree_node(TM->new_tree_node_id(), omp_pragma_K, local_tn_schema);

   bool is_block = false;
   bool is_opening = false;
   if(function_name == STR_CST_pragma_function_start)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---pragma opens a block");
      is_block = true;
      is_opening = true;
      switch(directive)
      {
         case(pragma_manager::OMP_BARRIER):
         {
            THROW_ERROR("Unsupported omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         case(pragma_manager::OMP_ATOMIC):
         case(pragma_manager::OMP_DECLARE_SIMD):
         case(pragma_manager::OMP_FOR):
         case(pragma_manager::OMP_PARALLEL_FOR):
         case(pragma_manager::OMP_SIMD):
         case(pragma_manager::OMP_TARGET):
         case(pragma_manager::OMP_UNKNOWN):
         {
            THROW_UNREACHABLE("Unexpected omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         case(pragma_manager::OMP_CRITICAL):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_critical_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_critical_pragma>(d_node);
            pn->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_parallel_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_parallel_pragma>(d_node);
            pn->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL_SECTIONS):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tn_schema1;
            local_tn_schema1[TOK(TOK_PRAGMA_OMP_SHORTCUT)] = STR(true);
            unsigned int node_parallel = TM->new_tree_node_id();
            const auto pn_node = TM->create_tree_node(node_parallel, omp_parallel_pragma_K, local_tn_schema1);
            auto pn = GetPointer<omp_parallel_pragma>(pn_node);
            pn->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));

            unsigned int node_sections = TM->new_tree_node_id();
            TM->create_tree_node(node_sections, omp_sections_pragma_K, local_tn_schema1);

            tree_node_schema[TOK(TOK_OP0)] = std::to_string(node_parallel);
            tree_node_schema[TOK(TOK_OP1)] = std::to_string(node_sections);

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_parallel_sections_pragma_K, tree_node_schema);

            OpenParallelSections.push_back(d_node);
            break;
         }
         case(pragma_manager::OMP_SECTION):
         {
            TM->create_tree_node(TM->new_tree_node_id(), omp_section_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_SECTIONS):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_sections_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_TASK):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_task_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_task_pragma>(d_node);
            pn->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
   else if(function_name == STR_CST_pragma_function_end)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---pragma closes a block");
      is_block = true;
      is_opening = false;
      switch(directive)
      {
         case(pragma_manager::OMP_BARRIER):
         {
            THROW_ERROR("Unsupported omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         case(pragma_manager::OMP_ATOMIC):
         case(pragma_manager::OMP_DECLARE_SIMD):
         case(pragma_manager::OMP_FOR):
         case(pragma_manager::OMP_PARALLEL_FOR):
         case(pragma_manager::OMP_SIMD):
         case(pragma_manager::OMP_TARGET):
         case(pragma_manager::OMP_UNKNOWN):
         {
            THROW_UNREACHABLE("Unexpected omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         case(pragma_manager::OMP_CRITICAL):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_critical_pragma_K, local_tree_node_schema);
            auto* pn = GetPointer<omp_critical_pragma>(d_node);
            pn->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_parallel_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_PARALLEL_SECTIONS):
         {
            const auto pn = OpenParallelSections.back();
            local_tn_schema[TOK(TOK_OP0)] = std::to_string(GetPointer<omp_parallel_sections_pragma>(pn)->op0->index);
            local_tn_schema[TOK(TOK_OP1)] = std::to_string(GetPointer<omp_parallel_sections_pragma>(pn)->op1->index);

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_parallel_sections_pragma_K, local_tn_schema);

            OpenParallelSections.pop_back();
            break;
         }
         case(pragma_manager::OMP_SECTION):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_section_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_SECTIONS):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_sections_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_TASK):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;

            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_task_pragma_K, local_tree_node_schema);
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
   else if(function_name == STR_CST_pragma_function_single_line_one_argument)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---single line pragma");
      switch(directive)
      {
         case(pragma_manager::OMP_BARRIER):
         {
            THROW_ERROR("Unsupported omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         case(pragma_manager::OMP_ATOMIC):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_atomic_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_FOR):
         case(pragma_manager::OMP_PARALLEL_FOR):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_for_pragma_K, tree_node_schema);
            auto* fp = GetPointer<omp_for_pragma>(d_node);
            fp->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_DECLARE_SIMD):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_declare_simd_pragma_K, tree_node_schema);
            auto* sp = GetPointer<omp_declare_simd_pragma>(d_node);
            sp->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_SIMD):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_simd_pragma_K, tree_node_schema);
            auto* sp = GetPointer<omp_simd_pragma>(d_node);
            sp->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_TARGET):
         {
            d_node = TM->create_tree_node(TM->new_tree_node_id(), omp_target_pragma_K, tree_node_schema);
            auto* tp = GetPointer<omp_target_pragma>(d_node);
            tp->clauses = PM->ExtractClauses(get_call_parameter(tn, 2));
            break;
         }
         case(pragma_manager::OMP_CRITICAL):
         case(pragma_manager::OMP_PARALLEL):
         case(pragma_manager::OMP_PARALLEL_SECTIONS):
         case(pragma_manager::OMP_SECTION):
         case(pragma_manager::OMP_SECTIONS):
         case(pragma_manager::OMP_TASK):
         case(pragma_manager::OMP_UNKNOWN):
         {
            THROW_UNREACHABLE("Unexpected omp directive " + pragma_manager::omp_directive_keywords[directive]);
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }

   tree_node_schema.clear();
   tree_node_schema[TOK(TOK_SRCP)] =
       gc->include_name + ":" + std::to_string(gc->line_number) + ":" + std::to_string(gc->column_number);
   tree_node_schema[TOK(TOK_SCPE)] = STR(gc->scpe->index);
   tree_node_schema[TOK(TOK_IS_BLOCK)] = STR(is_block);
   tree_node_schema[TOK(TOK_OPEN)] = STR(is_opening);
   tree_node_schema[TOK(TOK_PRAGMA_SCOPE)] = std::to_string(s_node->index);
   tree_node_schema[TOK(TOK_PRAGMA_DIRECTIVE)] = std::to_string(d_node->index);
   const auto gp_node = TM->create_tree_node(TM->new_tree_node_id(), gimple_pragma_K, tree_node_schema);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created openmp pragma: " + gp_node->ToString());
   return gp_node;
}

tree_nodeRef PragmaAnalysis::create_map_pragma(const tree_nodeRef& tn) const
{
   const auto TM = AppM->get_tree_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Creating mapping pragma starting from tree node " + std::to_string(tn->index));
   const auto gc = GetPointer<const gimple_call>(tn);
#if HAVE_ASSERTS
   const auto fn = GetPointer<addr_expr>(gc->fn)->op;
   const auto name = GetPointer<function_decl>(fn)->name;
   const auto function_name = GetPointer<identifier_node>(name)->strg;
#endif

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> scope_tn_schema;
   const auto s_node = TM->create_tree_node(TM->new_tree_node_id(), map_pragma_K, scope_tn_schema);

   THROW_ASSERT(function_name == STR_CST_pragma_function_single_line_two_arguments,
                "Error in map pragma replacing function " + function_name);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> directive_tn_schema;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Component is " + get_call_parameter(tn, 2));
   directive_tn_schema[TOK(TOK_HW_COMPONENT)] = get_call_parameter(tn, 2);

   const std::string fourth_parameter = get_call_parameter(tn, 3);
   if(fourth_parameter != "")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fourth parameter is " + fourth_parameter);
      if(fourth_parameter != STR_CST_pragma_keyword_recursive)
      {
         THROW_ERROR("Error in pragma syntax: second parameter of mapping pragma is " + fourth_parameter);
      }
      directive_tn_schema[TOK(TOK_RECURSIVE)] = STR(true);
   }
   const auto d_node = TM->create_tree_node(TM->new_tree_node_id(), call_point_hw_pragma_K, directive_tn_schema);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   tree_node_schema[TOK(TOK_SRCP)] =
       gc->include_name + ":" + std::to_string(gc->line_number) + ":" + std::to_string(gc->column_number);
   tree_node_schema[TOK(TOK_SCPE)] = STR(gc->scpe->index);
   tree_node_schema[TOK(TOK_IS_BLOCK)] = STR(false);
   tree_node_schema[TOK(TOK_OPEN)] = STR(false);
   tree_node_schema[TOK(TOK_PRAGMA_SCOPE)] = std::to_string(s_node->index);
   tree_node_schema[TOK(TOK_PRAGMA_DIRECTIVE)] = std::to_string(d_node->index);
   tree_node_schema[TOK(TOK_BB_INDEX)] = std::to_string(gc->bb_index);
   const auto gp_node = TM->create_tree_node(TM->new_tree_node_id(), gimple_pragma_K, tree_node_schema);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created mapping pragma: " + gp_node->ToString());
   return gp_node;
}

DesignFlowStep_Status PragmaAnalysis::Exec()
{
   const auto TM = AppM->get_tree_manager();
   const auto PM = AppM->get_pragma_manager();

   const auto functions = TM->GetAllFunctions();
   for(const auto function : functions)
   {
      const auto curr_tn = TM->GetTreeNode(function);
      auto fd = GetPointer<function_decl>(curr_tn);
      if(!fd->body)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining function " + STR(function));
      auto sl = GetPointer<statement_list>(fd->body);
      for(const auto& [bbi, bb] : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + std::to_string(bbi));
         const auto list_of_stmt = bb->CGetStmtList();
         auto it2 = list_of_stmt.begin();
         while(it2 != list_of_stmt.end())
         {
            const auto TN = *it2;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + TN->ToString());
            if(TN->get_kind() == gimple_call_K)
            {
               const auto gc = GetPointer<gimple_call>(TN);
               if(gc->fn && gc->fn->get_kind() == addr_expr_K)
               {
                  const auto fn = GetPointer<addr_expr>(gc->fn)->op;
                  if(fn)
                  {
                     const auto name = GetPointer<function_decl>(fn)->name;
                     const auto function_name = GetPointer<identifier_node>(name)->strg;
                     if(!starts_with(function_name, STR_CST_pragma_prefix))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "<--Skip statement " + std::to_string(TN->index));
                        it2++;
                        continue;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a pragma");
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
                     tree_node_schema[TOK(TOK_SRCP)] = gc->include_name + ":" + std::to_string(gc->line_number) + ":" +
                                                       std::to_string(gc->column_number);
                     tree_node_schema[TOK(TOK_SCPE)] = STR(function);

                     // unsigned int scope, directive;
                     if(!starts_with(function_name, STR_CST_pragma_function_generic))
                     {
                        const auto scope = get_call_parameter(TN, 0);
                        tree_nodeRef gp_node = nullptr;
                        if(scope == STR_CST_pragma_keyword_omp)
                        {
                           gp_node = create_omp_pragma(TN);
                        }
                        else if(scope == STR_CST_pragma_keyword_map)
                        {
                           gp_node = create_map_pragma(TN);
                        }
                        if(gp_node)
                        {
                           // NOTE: application manager is not passed as argument since pragma analysis is performed
                           // before call graph computation
                           bb->Replace(TN, gp_node, true, nullptr);
                        }
                     }
                     else
                     {
                        const auto d_node =
                            TM->create_tree_node(TM->new_tree_node_id(), gimple_pragma_K, tree_node_schema);
                        auto num = function_name.substr(10, function_name.size());
                        num = num.substr(0, num.find('_'));
                        std::string string_base = PM->getGenericPragma(static_cast<unsigned>(std::stoul(num)));
                        string_base = string_base.substr(string_base.find("#pragma") + 8, string_base.size());
                        auto el = GetPointer<gimple_pragma>(d_node);
                        el->line = string_base;
                        auto next = it2;
                        for(++next; next != list_of_stmt.end(); ++next)
                        {
                           auto en = GetPointer<gimple_node>(*next);
                           if(en)
                           {
                              en->pragmas.push_back(d_node);
                              /// Erasing first element
                              // NOTE: application manager is not passed as argument since pragma analysis is performed
                              // before call graph computation
                              bb->RemoveStmt(TN, nullptr);
                              break;
                           }
                        }
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Examined statement " + std::to_string(TN->index));
            it2++;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + std::to_string(bbi));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined function " + STR(function));
   }

   return DesignFlowStep_Status::SUCCESS;
}
