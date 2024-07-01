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
 * @file pragma_manager.cpp
 * @brief Implementation for methods used to manage pragma annotations.
 *
 * Implementation of methods used to manage information about pragma directives in a C/C++ program.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "pragma_manager.hpp"

#include "Parameter.hpp"
#include "PragmaParser.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "pragma_constants.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "var_pp_functor.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>

#include <list>
#include <regex>
#include <string>
#include <vector>

#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_NPROFILE.hpp"

const std::string pragma_manager::omp_directive_keywords[pragma_manager::OMP_UNKNOWN] = {
    "atomic",   "barrier",  "critical", "declare simd", "for",    "parallel for", "parallel sections",
    "parallel", "sections", "section",  "simd",         "target", "task",
};

unsigned int num_task = 0;

pragma_manager::pragma_manager(const application_managerRef _application_manager, const ParameterConstRef _param)
    : application_manager(_application_manager),
      TM(_application_manager->get_tree_manager()),
      param(_param),
      debug_level(_param->get_class_debug_level(GET_CLASS(*this)))
{
   if(param->isOption(OPT_blackbox))
   {
      const auto black_box_functions = param->getOption<CustomSet<std::string>>(OPT_blackbox);
      for(const auto& black_box_function : black_box_functions)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, param->getOption<int>(OPT_output_level),
                       "Function \"" + black_box_function + "\" is a blackbox");
         BlackBoxFunctions.insert(black_box_function);
         // addBlackBoxPragma(black_box_function);
      }
   }
}

pragma_manager::~pragma_manager() = default;

bool pragma_manager::checkCompliant() const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Checking source code compliance...");

   // here check for reference manual compliance

   return true;
}

bool pragma_manager::isBlackBox(const std::string& name) const
{
   return BlackBoxFunctions.find(name) != BlackBoxFunctions.end();
}

const std::list<std::string> pragma_manager::GetFunctionDefinitionPragmas(const std::string& function_name) const
{
   if(function_definition_pragmas.find(function_name) != function_definition_pragmas.end())
   {
      return function_definition_pragmas.find(function_name)->second;
   }
   return std::list<std::string>();
}

CustomUnorderedSet<std::string> pragma_manager::getFunctionCallPragmas(const std::string& Name) const
{
   if(FunctionCallPragmas.find(Name) != FunctionCallPragmas.end())
   {
      return FunctionCallPragmas.find(Name)->second;
   }
   else
   {
      return CustomUnorderedSet<std::string>();
   }
}

void pragma_manager::AddFunctionDefinitionPragmas(const std::string& function_name,
                                                  const CustomUnorderedSet<std::string>& pragmas)
{
   for(const auto& pragma : pragmas)
   {
      std::match_results<std::string::const_iterator> what;
      std::regex expr;
      if(pragma.find("#pragma omp declare simd") == 0)
      {
         function_definition_pragmas[function_name].push_back(pragma);
         continue;
      }
      THROW_ERROR("Unsupported function definition pragma associated with function " + function_name + ": " + pragma);
   }
}

void pragma_manager::addFunctionCallPragmas(const std::string& Name, const CustomUnorderedSet<std::string>& Pragmas)
{
   CustomUnorderedSet<std::string>::const_iterator k, k_end = Pragmas.end();
   for(k = Pragmas.begin(); k != k_end; ++k)
   {
      FunctionCallPragmas[Name].insert(*k);
   }
}

unsigned int pragma_manager::addBlackBoxPragma(const std::string& function_name, unsigned int function_id)
{
   unsigned int scope = TM->new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
   TM->create_tree_node(scope, issue_pragma_K, tree_node_schema);

   unsigned int directive = TM->new_tree_node_id();
   TM->create_tree_node(directive, blackbox_pragma_K, tree_node_schema);

   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> schema;
   schema[TOK(TOK_SRCP)] = ":0:0";
   schema[TOK(TOK_SCPE)] = STR(function_id);
   schema[TOK(TOK_IS_BLOCK)] = STR(false);
   schema[TOK(TOK_OPEN)] = STR(false);
   schema[TOK(TOK_PRAGMA_SCOPE)] = std::to_string(scope);
   schema[TOK(TOK_PRAGMA_DIRECTIVE)] = std::to_string(directive);
   unsigned int final_id = TM->new_tree_node_id();
   TM->create_tree_node(final_id, gimple_pragma_K, schema);

   BlackBoxFunctions.insert(function_name);

   return final_id;
}

unsigned int pragma_manager::AddOmpSimdPragma(const std::string& line, unsigned int function_id) const
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> simd_tree_node_schema, omp_pragma_tree_node_schema,
       tree_node_schema;
   unsigned int scope_id = TM->new_tree_node_id();
   TM->create_tree_node(scope_id, omp_pragma_K, omp_pragma_tree_node_schema);

   unsigned int simd_id = TM->new_tree_node_id();
   TM->create_tree_node(simd_id, omp_simd_pragma_K, simd_tree_node_schema);
   auto* osp = GetPointer<omp_simd_pragma>(TM->GetTreeNode(simd_id));
   if(line != "#pragma omp declare simd")
   {
      osp->clauses = ExtractClauses(line.substr(line.find("#pragma omp declare simd ")));
   }

   tree_node_schema[TOK(TOK_IS_BLOCK)] = STR(true);
   tree_node_schema[TOK(TOK_OPEN)] = STR(false);
   tree_node_schema[TOK(TOK_PRAGMA_SCOPE)] = std::to_string(scope_id);
   tree_node_schema[TOK(TOK_PRAGMA_DIRECTIVE)] = std::to_string(simd_id);
   tree_node_schema[TOK(TOK_BB_INDEX)] = std::to_string(0);
   tree_node_schema[TOK(TOK_SRCP)] = ":0:0";
   tree_node_schema[TOK(TOK_SCPE)] = STR(function_id);
   unsigned int pragma_id = TM->new_tree_node_id();
   TM->create_tree_node(pragma_id, gimple_pragma_K, tree_node_schema);
   return pragma_id;
}

CustomUnorderedMapUnstable<std::string, std::string>
pragma_manager::ExtractClauses(const std::string& clauses_list) const
{
   CustomUnorderedMapUnstable<std::string, std::string> clauses_map;
   if(!clauses_list.size())
   {
      return clauses_map;
   }

   std::string trimmed_clauses = clauses_list;
   bool inside_parentheses = false;
   /// Trim blanks inside parentheses
   for(size_t index = clauses_list.size(); index > 0; index--)
   {
      if(trimmed_clauses[index - 1] == ')')
      {
         inside_parentheses = true;
      }
      else if(trimmed_clauses[index - 1] == '(')
      {
         inside_parentheses = false;
         index--;
         if(index == 0)
         {
            break;
         }
         for(; index > 0 && trimmed_clauses[index - 1] == ' '; index--)
         {
            trimmed_clauses.erase(index - 1, 1);
         }
      }
      else if(trimmed_clauses[index - 1] == ' ' && inside_parentheses)
      {
         trimmed_clauses.erase(index - 1, 1);
      }
   }

   std::vector<std::string> splitted;
   boost::algorithm::split(splitted, trimmed_clauses, boost::algorithm::is_any_of(" \t\n"));

   for(const auto& clause : splitted)
   {
      if(clause.find('(') != std::string::npos)
      {
         const std::string key = clause.substr(0, clause.find('('));
         const std::string value = clause.substr(clause.find('(') + 1, clause.size() - clause.find('(') - 2);
         clauses_map[key] = value;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Clause " + key + ": " + value);
      }
      else
      {
         clauses_map[clause] = "";
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Empty clause " + clause);
      }
   }
   return clauses_map;
}

void pragma_manager::setGenericPragma(unsigned int number, const std::string& line)
{
   GenericPragmas[number] = line;
}

std::string pragma_manager::getGenericPragma(unsigned int number) const
{
   THROW_ASSERT(GenericPragmas.count(number), "Wrong generic pragma");
   return GenericPragmas.find(number)->second;
}

bool pragma_manager::CheckOmpFor(const application_managerConstRef app_man, const unsigned int function_index,
                                 const vertex bb_operation_vertex) const
{
   const BBGraphConstRef bb_cfg = app_man->CGetFunctionBehavior(function_index)->CGetBBGraph(FunctionBehavior::BB);
   vertex current = bb_operation_vertex;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Looking for an openmp associated with loop " +
                      std::to_string(bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->block->number));
   while(boost::in_degree(current, *bb_cfg) == 1)
   {
      const BBNodeInfoConstRef info = bb_cfg->CGetBBNodeInfo(current);
      for(const auto& stmt : info->block->CGetStmtList())
      {
         if(stmt->get_kind() == gimple_pragma_K)
         {
            auto* pn = GetPointer<gimple_pragma>(stmt);
            if(pn->scope && GetPointer<omp_pragma>(pn->scope))
            {
               auto* fp = GetPointer<omp_for_pragma>(pn->directive);
               if(fp)
               {
                  return true;
               }
            }
         }
      }
      InEdgeIterator ei, ei_end;
      boost::tie(ei, ei_end) = boost::in_edges(current, *bb_cfg);
      current = boost::source(*ei, *bb_cfg);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return false;
}

void pragma_manager::CheckAddOmpFor(const unsigned int function_index, const vertex bb_operation_vertex,
                                    const application_managerRef AppM)
{
   const auto bb_cfg = application_manager->CGetFunctionBehavior(function_index)->CGetBBGraph(FunctionBehavior::BB);
   vertex current = bb_operation_vertex;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Looking for an openmp associated with loop " +
                      std::to_string(bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->block->number));
   while(boost::in_degree(current, *bb_cfg) == 1)
   {
      const auto info = bb_cfg->CGetBBNodeInfo(current);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing BB" + STR(info->block->number));
      for(const auto& stmt : info->block->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analzying " + STR(stmt));
         if(stmt->get_kind() == gimple_pragma_K)
         {
            const auto pn = GetPointerS<gimple_pragma>(stmt);
            if(pn->scope && GetPointer<omp_pragma>(pn->scope))
            {
               const auto fp = GetPointer<omp_for_pragma>(pn->directive);
               if(fp)
               {
                  info->block->RemoveStmt(stmt, AppM);
                  const auto FB = application_manager->GetFunctionBehavior(function_index);
                  FB->GetLoops()->GetLoop(bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->block->number)->loop_type |=
                      DOALL_LOOP;
                  // FB->UpdateBBVersion();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Found");
                  return;
               }
            }
         }
      }
      InEdgeIterator ei, ei_end;
      boost::tie(ei, ei_end) = boost::in_edges(current, *bb_cfg);
      current = boost::source(*ei, *bb_cfg);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not found");
}

void pragma_manager::CheckAddOmpSimd(const unsigned int function_index, const vertex bb_operation_vertex,
                                     const application_managerRef AppM)
{
   const auto bb_cfg = application_manager->GetFunctionBehavior(function_index)->GetBBGraph(FunctionBehavior::BB);
   const auto current_loop_id = bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->loop_id;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Looking for an openmp simd associated with loop " +
                      std::to_string(bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->block->number));
   if(boost::in_degree(bb_operation_vertex, *bb_cfg) != 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not found");
      return;
   }
   InEdgeIterator ei, ei_end;
   boost::tie(ei, ei_end) = boost::in_edges(bb_operation_vertex, *bb_cfg);
   vertex current = boost::source(*ei, *bb_cfg);
   while(boost::in_degree(current, *bb_cfg) == 1)
   {
      const auto info = bb_cfg->GetBBNodeInfo(current);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing BB" + STR(info->block->number));
      for(const auto& stmt : info->block->CGetStmtList())
      {
         if(stmt->get_kind() == gimple_pragma_K)
         {
            const auto pn = GetPointerS<gimple_pragma>(stmt);
            if(pn->scope && GetPointer<omp_pragma>(pn->scope))
            {
               const auto sp = GetPointer<omp_simd_pragma>(pn->directive);
               if(sp)
               {
                  info->block->RemoveStmt(stmt, AppM);
                  if(pn->vdef)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing vdef " + STR(pn->vdef));
                     const auto ssa_vdef = GetPointerS<ssa_name>(pn->vdef);
                     const auto vdef_uses = ssa_vdef->CGetUseStmts();
                     for(const auto& stmt_uses : vdef_uses)
                     {
                        const auto gn = GetPointerS<gimple_node>(stmt_uses.first);
                        if(gn->memuse && gn->memuse->index == pn->vdef->index)
                        {
                           ssa_vdef->RemoveUse(stmt_uses.first);
                           gn->memuse = nullptr;
                        }
                        if(gn->vuses.erase(pn->vdef))
                        {
                           ssa_vdef->RemoveUse(stmt_uses.first);
                        }
                        if(gn->vovers.erase(pn->vdef))
                        {
                           ssa_vdef->RemoveUse(stmt_uses.first);
                        }
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed vdef");
                  }
                  const auto FB = application_manager->GetFunctionBehavior(function_index);
                  FB->GetLoops()->GetLoop(bb_cfg->CGetBBNodeInfo(bb_operation_vertex)->block->number)->loop_type |=
                      DOALL_LOOP;
                  // FB->UpdateBBVersion();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Found");
                  return;
               }
            }
         }
      }
      boost::tie(ei, ei_end) = boost::in_edges(current, *bb_cfg);
      current = boost::source(*ei, *bb_cfg);
      if(boost::out_degree(current, *bb_cfg) != 1 || bb_cfg->CGetBBNodeInfo(current)->loop_id != current_loop_id)
      {
         break;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not found");
}

pragma_manager::OmpPragmaType pragma_manager::GetOmpPragmaType(const std::string& directive)
{
   OmpPragmaType omp_pragma_type = OMP_UNKNOWN;
   size_t index = 0;
   for(index = 0; index < static_cast<size_t>(pragma_manager::OMP_UNKNOWN); index++)
   {
      if(directive.find(pragma_manager::omp_directive_keywords[index]) != std::string::npos)
      {
         omp_pragma_type = static_cast<pragma_manager::OmpPragmaType>(index);
         break;
      }
   }
   return omp_pragma_type;
}
