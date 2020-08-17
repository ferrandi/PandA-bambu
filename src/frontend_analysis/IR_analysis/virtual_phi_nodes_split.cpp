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
 * @file virtual_phi_nodes_split.cpp
 * @brief Insert a temporary assignment for each phi use. This code has been derived from the split_phi_nodes.[hc]pp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "virtual_phi_nodes_split.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <fstream>

/// tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

bool virtual_phi_nodes_split::tree_dumped = false;

virtual_phi_nodes_split::virtual_phi_nodes_split(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, VIRTUAL_PHI_NODES_SPLIT, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

virtual_phi_nodes_split::~virtual_phi_nodes_split() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> virtual_phi_nodes_split::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
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

DesignFlowStep_Status virtual_phi_nodes_split::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   if(debug_level >= DEBUG_LEVEL_PEDANTIC && !tree_dumped)
   {
      tree_dumped = true;
      std::string raw_file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + "before_virtual_phi_nodes_split.raw";
      std::ofstream raw_file(raw_file_name.c_str());
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Tree-Manager dumped for debug purpose");
      raw_file << TM;
      raw_file.close();
   }
   /// Basic block splitting information: if replace[source, target] exists, then between source and target replace[source, target] has been inserted
   std::map<std::pair<unsigned int, unsigned int>, unsigned int> replace;

   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;

   std::map<unsigned int, blocRef>::iterator iit, iit_end = list_of_bloc.end();
   for(iit = list_of_bloc.begin(); iit != iit_end; ++iit)
   {
      blocRef& bb_block = iit->second;
      for(const auto& phi : bb_block->CGetPhiList())
      {
         virtual_split_phi(phi, bb_block, list_of_bloc, TM, replace);
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      std::string raw_file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + "after_virtual_phi_nodes_split.raw";
      std::ofstream raw_file(raw_file_name.c_str());
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Tree-Manager dumped for debug purpose");
      raw_file << TM;
      raw_file.close();
   }
   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}

void virtual_phi_nodes_split::virtual_split_phi(tree_nodeRef tree_phi, blocRef& bb_block, std::map<unsigned int, blocRef>& list_of_bloc, const tree_managerRef TM, std::map<std::pair<unsigned int, unsigned int>, unsigned int>& replace)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Splitting phi node " + boost::lexical_cast<std::string>(GET_INDEX_NODE(tree_phi)));
   auto* phi = GetPointer<gimple_phi>(GET_NODE(tree_phi));
   THROW_ASSERT(phi, "A non-phi node is stored in the phi_list");
   // std::cout << "Analyzing phi-node: @" << GET_INDEX_NODE(tree_phi) << std::endl;
   if(phi->virtual_flag)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Phi node is virtual so no splitting is performed");
      return;
   }
   for(const auto& def_edge : phi->CGetDefEdgesList())
   {
      tree_nodeRef def = def_edge.first;

      /// create the new ssa
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_IR_schema;
      auto* ssa_var = GetPointer<ssa_name>(GET_NODE(phi->res));
      THROW_ASSERT(ssa_var, "unexpected condition " + STR(GET_INDEX_NODE(phi->res)));
      unsigned int type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(phi->res));
      unsigned int new_ssa_id = TM->new_tree_node_id();
      unsigned int vers = TM->get_next_vers();
      bool volatile_flag = false;
      bool virtual_flag = false;
      ssa_IR_schema[TOK(TOK_TYPE)] = STR(type_index);
      if(ssa_var->var)
         ssa_IR_schema[TOK(TOK_VAR)] = STR(GET_INDEX_NODE(ssa_var->var));
      ssa_IR_schema[TOK(TOK_VERS)] = STR(vers);
      ssa_IR_schema[TOK(TOK_VOLATILE)] = STR(volatile_flag);
      ssa_IR_schema[TOK(TOK_VIRTUAL)] = STR(virtual_flag);
      TM->create_tree_node(new_ssa_id, ssa_name_K, ssa_IR_schema);

      tree_nodeRef res = TM->GetTreeReindex(new_ssa_id);

      /// substitute the def with the new ssa
      phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(res, def_edge.second));
      unsigned int bb_source = def_edge.second;
      blocRef source_bb;
      std::map<unsigned int, blocRef>::iterator it, it_end = list_of_bloc.end();
      for(it = list_of_bloc.begin(); it != it_end; ++it)
      {
         source_bb = it->second;
         if(source_bb->number != bb_source)
            continue;
         if(replace.find(std::pair<unsigned int, unsigned int>(bb_source, bb_block->number)) != replace.end())
         {
            bb_source = replace[std::pair<unsigned int, unsigned int>(bb_source, bb_block->number)];
            source_bb = list_of_bloc.find(bb_source)->second;
            phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, bb_source));
         }
         if(bb_source == bloc::ENTRY_BLOCK_ID)
         {
            // New block has not been created
            if(!next_entry)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Predecessor is entry: creating new basic block");
               THROW_ASSERT(source_bb->list_of_succ.size() == 1, "ENTRY BB has more than one successor");
               unsigned int this_bb_index = source_bb->list_of_succ.front();
               // Creating new basic block
               blocRef new_bb = blocRef(new bloc(list_of_bloc.rbegin()->first + 1));
               new_bb->list_of_pred.push_back(bloc::ENTRY_BLOCK_ID);
               new_bb->list_of_succ.push_back(this_bb_index);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding edge from " + boost::lexical_cast<std::string>(bloc::ENTRY_BLOCK_ID) + " to " + boost::lexical_cast<std::string>(this_bb_index));

               // Updating entry
               source_bb->list_of_succ.clear();
               source_bb->list_of_succ.push_back(new_bb->number);

               // Updating this
               blocRef this_bb;
               unsigned int index;
               for(index = 0; index < list_of_bloc.size(); index++)
               {
                  if(list_of_bloc[index]->number == this_bb_index)
                  {
                     this_bb = list_of_bloc[index];
                     break;
                  }
               }
               THROW_ASSERT(index < list_of_bloc.size(), "Current basic block not found");
               for(index = 0; index < this_bb->list_of_pred.size(); index++)
               {
                  if(this_bb->list_of_pred[index] == bloc::ENTRY_BLOCK_ID)
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding edge from " + boost::lexical_cast<std::string>(new_bb->number) + " to " + boost::lexical_cast<std::string>(this_bb->list_of_pred[index]));
                     this_bb->list_of_pred[index] = new_bb->number;
                     break;
                  }
               }
               THROW_ASSERT(index < this_bb->list_of_pred.size(), "Entry not found in predecessors of current basic block");

               list_of_bloc[new_bb->number] = new_bb;
               source_bb = new_bb;
               next_entry = new_bb;
            }
            else
            {
               source_bb = next_entry;
            }
         }
         const auto list_of_stmt = source_bb->CGetStmtList();

         unsigned int gimple_stmt_id = TM->new_tree_node_id();
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
         IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         IR_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(res));
         IR_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(def));
         IR_schema[TOK(TOK_ORIG)] = boost::lexical_cast<std::string>(GET_INDEX_NODE(tree_phi));
         TM->create_tree_node(gimple_stmt_id, gimple_assign_K, IR_schema);
         tree_nodeRef created_stmt = TM->GetTreeReindex(gimple_stmt_id);
         phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, source_bb->number));
         if(list_of_stmt.size() and
            (GetPointer<gimple_goto>(GET_NODE(list_of_stmt.back())) || GetPointer<gimple_while>(GET_NODE(list_of_stmt.back())) || GetPointer<gimple_for>(GET_NODE(list_of_stmt.back())) || GetPointer<gimple_switch>(GET_NODE(list_of_stmt.back()))))
         {
            source_bb->PushBack(created_stmt);
            /// update bb_index associated with the statement
         }
         else if(list_of_stmt.size() && GetPointer<gimple_cond>(GET_NODE(list_of_stmt.back())))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "One of the predecessors ends with an if");
            bool true_case = source_bb->true_edge == bb_block->number;
            blocRef new_bb = blocRef(new bloc(list_of_bloc.rbegin()->first + 1));
            if(true_case)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "This is true edge");
               // Creating new basic block
               new_bb->list_of_pred.push_back(source_bb->true_edge);
               new_bb->list_of_succ.push_back(bb_block->number);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created new basic block " + boost::lexical_cast<std::string>(new_bb->number));

               // Updating predecessor
               source_bb->list_of_succ.erase(std::find(source_bb->list_of_succ.begin(), source_bb->list_of_succ.end(), bb_block->number));
               source_bb->list_of_succ.push_back(new_bb->number);
               source_bb->true_edge = new_bb->number;

               // Updating this
               bb_block->list_of_pred.erase(std::find(bb_block->list_of_pred.begin(), bb_block->list_of_pred.end(), source_bb->number));
               bb_block->list_of_pred.push_back(new_bb->number);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding edge from " + boost::lexical_cast<std::string>(source_bb->number) + " to " + boost::lexical_cast<std::string>(new_bb->number));
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "This is false edge");
               // Creating new basic block
               new_bb->list_of_pred.push_back(source_bb->false_edge);
               new_bb->list_of_succ.push_back(bb_block->number);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created new basic block " + boost::lexical_cast<std::string>(new_bb->number));

               // Updating predecessor
               source_bb->list_of_succ.erase(std::find(source_bb->list_of_succ.begin(), source_bb->list_of_succ.end(), bb_block->number));
               source_bb->list_of_succ.push_back(new_bb->number);
               source_bb->false_edge = new_bb->number;
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Updated predecessor");

               // Updating this
               bb_block->list_of_pred.erase(std::find(bb_block->list_of_pred.begin(), bb_block->list_of_pred.end(), source_bb->number));
               bb_block->list_of_pred.push_back(new_bb->number);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding edge from " + boost::lexical_cast<std::string>(source_bb->number) + " to " + boost::lexical_cast<std::string>(new_bb->number));
            }
            list_of_bloc[new_bb->number] = new_bb;
            replace[std::pair<unsigned int, unsigned int>(source_bb->number, bb_block->number)] = new_bb->number;
            phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, new_bb->number));

            // Inserting phi
            new_bb->PushBack(created_stmt);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Inserted new gimple " + boost::lexical_cast<std::string>(gimple_stmt_id));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Finished creation of new basic block");
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Inserted new gimple " + boost::lexical_cast<std::string>(gimple_stmt_id));
            source_bb->PushBack(created_stmt);
         }
         break;
      }
   }
   return;
}
