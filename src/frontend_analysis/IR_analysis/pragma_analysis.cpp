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

/// Header include
#include "pragma_analysis.hpp"

/// Codesign include
#include "application_manager.hpp"

/// Constants include
#include "pragma_constants.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// Pragma include
#include "PragmaParser.hpp"

/// STL include
#include "custom_map.hpp"
#include <string>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "ext_tree_node.hpp"
#include "pragma_manager.hpp"

/// Utility include
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <fstream>

PragmaAnalysis::PragmaAnalysis(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : ApplicationFrontendFlowStep(_AppM, PRAGMA_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

PragmaAnalysis::~PragmaAnalysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> PragmaAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
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

std::string PragmaAnalysis::get_call_parameter(unsigned int tree_node, unsigned int idx) const
{
   const tree_managerRef TM = AppM->get_tree_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Asking parameter " + boost::lexical_cast<std::string>(idx) + " " + STR(tree_node));
   auto tn = TM->get_tree_node_const(tree_node);
   const gimple_call* ce = GetPointer<gimple_call>(tn);
   if(idx >= ce->args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not found");
      return "";
   }
   const tree_nodeConstRef arg = GET_NODE(ce->args[idx]);
   const auto* ae = GetPointer<const addr_expr>(arg);
   THROW_ASSERT(ae, "Argument of call is not addr_expr: " + arg->get_kind_text());
   const tree_nodeConstRef ae_arg = GET_NODE(ae->op);
   std::string string_arg;
   if(ae_arg->get_kind() == var_decl_K)
   {
      auto vd = GetPointer<const var_decl>(ae_arg);
      THROW_ASSERT(vd, "unexpected condition");
      THROW_ASSERT(vd->init, "unexpected condition");
      auto vd_init = GET_NODE(vd->init);
      if(vd_init->get_kind() == constructor_K)
      {
         const constructor* co = GetPointer<const constructor>(vd_init);
         for(auto idx_valu : co->list_of_idx_valu)
         {
            auto valu = GET_NODE(idx_valu.second);
            THROW_ASSERT(valu->get_kind() == integer_cst_K, "unexpected condition");
            auto ic = GetPointer<const integer_cst>(valu);
            char val = static_cast<char>(ic->value);
            if(!val)
               break;
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
      const auto* sc = GetPointer<const string_cst>(ae_arg->get_kind() == string_cst_K ? ae_arg : GET_NODE(GetPointer<const array_ref>(ae_arg)->op0));
      string_arg = sc->strg;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parameter is " + string_arg);
   return string_arg;
}

void PragmaAnalysis::create_omp_pragma(const unsigned int tree_node) const
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const pragma_managerRef PM = AppM->get_pragma_manager();
   const tree_nodeRef curr_tn = TM->get_tree_node_const(tree_node);
   const auto* gc = GetPointer<const gimple_call>(curr_tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating openmp pragma starting from " + curr_tn->ToString());
   const tree_nodeRef& tn = GET_NODE(gc->fn);
   const tree_nodeRef& fn = GET_NODE(GetPointer<addr_expr>(tn)->op);
   const tree_nodeRef& name = GET_NODE(GetPointer<function_decl>(fn)->name);
   const std::string& function_name = GetPointer<identifier_node>(name)->strg;

   const pragma_manager::OmpPragmaType directive = pragma_manager::GetOmpPragmaType(get_call_parameter(tree_node, 1));

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   std::string include_name = GetPointer<srcp>(TM->get_tree_node_const(tree_node))->include_name;
   unsigned int line_number = GetPointer<srcp>(TM->get_tree_node_const(tree_node))->line_number;
   unsigned int column_number = GetPointer<srcp>(TM->get_tree_node_const(tree_node))->column_number;
   tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tn_schema;
   unsigned int directive_idx = 0, scope_idx = TM->new_tree_node_id();
   TM->create_tree_node(scope_idx, omp_pragma_K, local_tn_schema);

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
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_critical_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_critical_pragma>(TM->get_tree_node_const(directive_idx));
            pn->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_parallel_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_parallel_pragma>(TM->get_tree_node_const(directive_idx));
            pn->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL_SECTIONS):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tn_schema1;
            local_tn_schema1[TOK(TOK_PRAGMA_OMP_SHORTCUT)] = boost::lexical_cast<std::string>(true);
            unsigned int node_parallel = TM->new_tree_node_id();
            TM->create_tree_node(node_parallel, omp_parallel_pragma_K, local_tn_schema1);
            auto* pn = GetPointer<omp_parallel_pragma>(TM->get_tree_node_const(node_parallel));
            pn->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));

            unsigned int node_sections = TM->new_tree_node_id();
            TM->create_tree_node(node_sections, omp_sections_pragma_K, local_tn_schema1);

            tree_node_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(node_parallel);
            tree_node_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(node_sections);
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_parallel_sections_pragma_K, tree_node_schema);

            OpenParallelSections.push_back(TM->get_tree_node_const(directive_idx));
            break;
         }
         case(pragma_manager::OMP_SECTION):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_section_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_SECTIONS):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_sections_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_TASK):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_task_pragma_K, tree_node_schema);
            auto* pn = GetPointer<omp_task_pragma>(TM->get_tree_node_const(directive_idx));
            pn->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
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
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_critical_pragma_K, local_tree_node_schema);
            auto* pn = GetPointer<omp_critical_pragma>(TM->get_tree_node_const(directive_idx));
            pn->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_PARALLEL):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_parallel_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_PARALLEL_SECTIONS):
         {
            const tree_nodeRef& pn = OpenParallelSections.back();
            local_tn_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(GetPointer<omp_parallel_sections_pragma>(pn)->op0));
            local_tn_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(GetPointer<omp_parallel_sections_pragma>(pn)->op1));
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_parallel_sections_pragma_K, local_tn_schema);

            OpenParallelSections.pop_back();
            break;
         }
         case(pragma_manager::OMP_SECTION):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_section_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_SECTIONS):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_sections_pragma_K, local_tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_TASK):
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> local_tree_node_schema;
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_task_pragma_K, local_tree_node_schema);
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
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_atomic_pragma_K, tree_node_schema);
            break;
         }
         case(pragma_manager::OMP_FOR):
         case(pragma_manager::OMP_PARALLEL_FOR):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_for_pragma_K, tree_node_schema);
            auto* fp = GetPointer<omp_for_pragma>(TM->get_tree_node_const(directive_idx));
            fp->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_DECLARE_SIMD):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_declare_simd_pragma_K, tree_node_schema);
            auto* sp = GetPointer<omp_declare_simd_pragma>(TM->get_tree_node_const(directive_idx));
            sp->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_SIMD):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_simd_pragma_K, tree_node_schema);
            auto* sp = GetPointer<omp_simd_pragma>(TM->get_tree_node_const(directive_idx));
            sp->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
            break;
         }
         case(pragma_manager::OMP_TARGET):
         {
            directive_idx = TM->new_tree_node_id();
            TM->create_tree_node(directive_idx, omp_target_pragma_K, tree_node_schema);
            auto* tp = GetPointer<omp_target_pragma>(TM->get_tree_node_const(directive_idx));
            tp->clauses = PM->ExtractClauses(get_call_parameter(tree_node, 2));
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

   tree_node_schema[TOK(TOK_IS_BLOCK)] = boost::lexical_cast<std::string>(is_block);
   tree_node_schema[TOK(TOK_OPEN)] = boost::lexical_cast<std::string>(is_opening);
   tree_node_schema[TOK(TOK_PRAGMA_SCOPE)] = boost::lexical_cast<std::string>(scope_idx);
   tree_node_schema[TOK(TOK_PRAGMA_DIRECTIVE)] = boost::lexical_cast<std::string>(directive_idx);
   tree_node_schema[TOK(TOK_BB_INDEX)] = boost::lexical_cast<std::string>(gc->bb_index);
   if(gc->memuse)
      tree_node_schema[TOK(TOK_MEMUSE)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(gc->memuse));
   if(gc->memdef)
      tree_node_schema[TOK(TOK_MEMDEF)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(gc->memdef));
   TM->create_tree_node(tree_node, gimple_pragma_K, tree_node_schema);
   GetPointer<gimple_pragma>(TM->get_tree_node_const(tree_node))->vuses = gc->vuses;
   GetPointer<gimple_pragma>(TM->get_tree_node_const(tree_node))->vdef = gc->vdef;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created openmp pragma");
}

void PragmaAnalysis::create_map_pragma(const unsigned int node_id) const
{
   const tree_managerRef TM = AppM->get_tree_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating mapping pragma starting from tree node " + boost::lexical_cast<std::string>(node_id));
   const tree_nodeRef curr_tn = TM->get_tree_node_const(node_id);
   const auto* gc = GetPointer<const gimple_call>(curr_tn);
#if HAVE_ASSERTS
   const tree_nodeRef& tn = GET_NODE(gc->fn);
   const tree_nodeRef& fn = GET_NODE(GetPointer<addr_expr>(tn)->op);
   const tree_nodeRef& name = GET_NODE(GetPointer<function_decl>(fn)->name);
   const std::string& function_name = GetPointer<identifier_node>(name)->strg;
#endif

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   std::string include_name = GetPointer<srcp>(TM->get_tree_node_const(node_id))->include_name;
   unsigned int line_number = GetPointer<srcp>(TM->get_tree_node_const(node_id))->line_number;
   unsigned int column_number = GetPointer<srcp>(TM->get_tree_node_const(node_id))->column_number;
   tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> scope_tn_schema;
   unsigned int directive_idx = TM->new_tree_node_id(), scope_idx = TM->new_tree_node_id();

   TM->create_tree_node(scope_idx, map_pragma_K, scope_tn_schema);

   THROW_ASSERT(function_name == STR_CST_pragma_function_single_line_two_arguments, "Error in map pragma replacing function " + function_name);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> directive_tn_schema;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Component is " + get_call_parameter(node_id, 2));
   directive_tn_schema[TOK(TOK_HW_COMPONENT)] = get_call_parameter(node_id, 2);

   const std::string fourth_parameter = get_call_parameter(node_id, 3);
   if(fourth_parameter != "")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fourth parameter is " + fourth_parameter);
      if(fourth_parameter != STR_CST_pragma_keyword_recursive)
      {
         THROW_ERROR("Error in pragma syntax: second parameter of mapping pragma is " + fourth_parameter);
      }
      directive_tn_schema[TOK(TOK_RECURSIVE)] = boost::lexical_cast<std::string>(true);
   }
   TM->create_tree_node(directive_idx, call_point_hw_pragma_K, directive_tn_schema);

   tree_node_schema[TOK(TOK_IS_BLOCK)] = boost::lexical_cast<std::string>(false);
   tree_node_schema[TOK(TOK_OPEN)] = boost::lexical_cast<std::string>(false);
   tree_node_schema[TOK(TOK_PRAGMA_SCOPE)] = boost::lexical_cast<std::string>(scope_idx);
   tree_node_schema[TOK(TOK_PRAGMA_DIRECTIVE)] = boost::lexical_cast<std::string>(directive_idx);
   tree_node_schema[TOK(TOK_BB_INDEX)] = boost::lexical_cast<std::string>(gc->bb_index);
   TM->create_tree_node(node_id, gimple_pragma_K, tree_node_schema);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created mapping pragma starting from tree node " + boost::lexical_cast<std::string>(node_id));
}

DesignFlowStep_Status PragmaAnalysis::Exec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const pragma_managerRef PM = AppM->get_pragma_manager();

   const CustomUnorderedSet<unsigned int>& functions = TM->GetAllFunctions();
   for(const auto function : functions)
   {
      const tree_nodeRef curr_tn = TM->get_tree_node_const(function);
      auto* fd = GetPointer<function_decl>(curr_tn);
      if(not fd->body)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining function " + STR(function));
      auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
      std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
      std::map<unsigned int, blocRef>::iterator it, it_end;
      it_end = blocks.end();
      for(it = blocks.begin(); it != it_end; ++it)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + boost::lexical_cast<std::string>(it->first));
         const auto list_of_stmt = it->second->CGetStmtList();
         auto it2 = list_of_stmt.begin();
         while(it2 != list_of_stmt.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + boost::lexical_cast<std::string>(GET_INDEX_NODE(*it2)));
            const tree_nodeRef& TN = GET_NODE(*it2);
            if(TN->get_kind() == gimple_call_K)
            {
               const tree_nodeRef& tn = GET_NODE(GetPointer<gimple_call>(TN)->fn);
               if(tn and tn->get_kind() == addr_expr_K)
               {
                  const tree_nodeRef& fn = GET_NODE(GetPointer<addr_expr>(tn)->op);
                  if(fn)
                  {
                     const tree_nodeRef& name = GET_NODE(GetPointer<function_decl>(fn)->name);
                     const std::string& function_name = GetPointer<identifier_node>(name)->strg;
                     if(function_name.find(STR_CST_pragma_prefix) == std::string::npos)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skip statement " + boost::lexical_cast<std::string>(GET_INDEX_NODE(*it2)));
                        it2++;
                        continue;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Find a pragma");
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
                     std::string include_name = GetPointer<srcp>(TN)->include_name;
                     unsigned int line_number = GetPointer<srcp>(TN)->line_number;
                     unsigned int column_number = GetPointer<srcp>(TN)->column_number;
                     tree_node_schema[TOK(TOK_SRCP)] = include_name + ":" + boost::lexical_cast<std::string>(line_number) + ":" + boost::lexical_cast<std::string>(column_number);

                     // unsigned int scope, directive;
                     if(!boost::algorithm::starts_with(function_name, STR_CST_pragma_function_generic))
                     {
                        std::string scope = get_call_parameter(GET_INDEX_NODE(*it2), 0);
                        if(scope == STR_CST_pragma_keyword_omp)
                           create_omp_pragma(GET_INDEX_NODE(*it2));
                        else if(scope == STR_CST_pragma_keyword_map)
                           create_map_pragma(GET_INDEX_NODE(*it2));
                     }
                     else
                     {
                        TM->create_tree_node(GET_INDEX_NODE(*it2), gimple_pragma_K, tree_node_schema);
                        std::string num = function_name;
                        num = num.substr(10, num.size());
                        num = num.substr(0, num.find("_"));
                        std::string string_base = PM->getGenericPragma(boost::lexical_cast<unsigned int>(num));
                        string_base = string_base.substr(string_base.find("#pragma") + 8, string_base.size());
                        auto* el = GetPointer<gimple_pragma>(TM->get_tree_node_const(GET_INDEX_NODE(*it2)));
                        el->line = string_base;
                        decltype(it2) next;
                        for(next = it2; next != list_of_stmt.end(); next++)
                        {
                           auto* en = GetPointer<gimple_node>(GET_NODE(*next));
                           if(en)
                           {
                              en->pragmas.push_back(*it2);
                              /// Erasing first element
                              if(it2 == list_of_stmt.begin())
                              {
                                 it->second->RemoveStmt(*it2);
                                 it2 = list_of_stmt.begin();
                              }
                              /// Erasing not the first element
                              else
                              {
                                 next = it2;
                                 next++;
                                 it->second->RemoveStmt(*it2);
                                 next--;
                                 it2 = next;
                              }
                              break;
                           }
                        }
                        /// No more gimple_node - Finished for this block
                        if(next == list_of_stmt.end())
                        {
                           break;
                        }
                        continue;
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + boost::lexical_cast<std::string>(GET_INDEX_NODE(*it2)));
            it2++;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + boost::lexical_cast<std::string>(it->first));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined function " + STR(function));
   }

   return DesignFlowStep_Status::SUCCESS;
}
