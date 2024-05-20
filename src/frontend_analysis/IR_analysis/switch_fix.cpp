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
 * @file switch_fix.cpp
 * @brief Analysis step that modifies the control flow graph to fix switches
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "switch_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

SwitchFix::SwitchFix(const application_managerRef _AppM, unsigned int _function_id,
                     const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, SWITCH_FIX, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

SwitchFix::~SwitchFix() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
SwitchFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

DesignFlowStep_Status SwitchFix::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();

   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   tree_nodeRef temp = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(fd->body);
   auto& list_of_block = sl->list_of_bloc;

   /// Fix switch statements
   for(const auto& [bbi, bb] : list_of_block)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(bbi));
      const auto list_of_stmt = bb->CGetStmtList();
      // Checking for switch
      if(!list_of_stmt.empty() && list_of_stmt.back()->get_kind() == gimple_switch_K)
      {
         const auto gs_node = list_of_stmt.back();
         const auto gs = GetPointer<const gimple_switch>(gs_node);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->BB" + STR(bbi) + " ends with a switch");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Checking if some successor has more than two gimple_label");
         /// The set of basic blocks which contain more than a label; this fix has to be performed before
         /// multiple_pred_switch check
         CustomUnorderedSet<unsigned int> multiple_labels_blocks;
         for(const auto succ : bb->list_of_succ)
         {
            const auto succ_list_of_stmt = list_of_block.at(succ)->CGetStmtList();
            auto next = succ_list_of_stmt.begin();
            next = succ_list_of_stmt.size() > 1 ? ++next : next;
            if(succ_list_of_stmt.size() > 1 and (*next)->get_kind() == gimple_label_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(succ));
               multiple_labels_blocks.insert(succ);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

         CustomUnorderedSet<unsigned int>::iterator multiple_labels_block,
             multiple_labels_block_end = multiple_labels_blocks.end();
         for(multiple_labels_block = multiple_labels_blocks.begin(); multiple_labels_block != multiple_labels_block_end;
             ++multiple_labels_block)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Splitting BB" + STR(*multiple_labels_block));
            /// Compute the case labels of the switch
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computing case labels");
            CustomUnorderedSet<tree_nodeRef> cases;
            const tree_vec* tv = GetPointer<tree_vec>(gs->op1);
            std::vector<tree_nodeRef>::const_iterator it, it_end = tv->list_of_op.end();
            for(it = tv->list_of_op.begin(); it != it_end; ++it)
            {
               cases.insert(GetPointer<case_label_expr>(*it)->got);
            }
            /// First check that the first label is a case
            blocRef current_block = list_of_block.at(*multiple_labels_block);
            const auto current_list_of_stmt = list_of_block.at(*multiple_labels_block)->CGetStmtList();
            auto current_tree_node = current_list_of_stmt.front();
            tree_nodeRef ld = GetPointer<gimple_label>(current_tree_node)->op;
            if(cases.find(ld) == cases.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->First label is not a case");
               size_t stmt_i = 1;
               auto stmt_index = current_list_of_stmt.begin();
               THROW_ASSERT(current_list_of_stmt.size() > 1, "expected more than one statement");
               ++stmt_index;
               while(stmt_index != current_list_of_stmt.end())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining operation " + STR(stmt_i));
                  current_tree_node = *stmt_index;
                  THROW_ASSERT(current_tree_node->get_kind() == gimple_label_K,
                               "An artificial label_decl has not been found at the beginning of the basic block");
                  ld = GetPointer<gimple_label>(current_tree_node)->op;
                  if(cases.find(ld) != cases.end())
                  {
                     break;
                  }
                  ++stmt_index;
                  ++stmt_i;
               }
               THROW_ASSERT(stmt_i < current_list_of_stmt.size(),
                            "An artificial label_decl has not been found at the beginning of the basic block");
               const auto temp_label = *stmt_index;
               list_of_block.at(*multiple_labels_block)->RemoveStmt(temp_label, AppM);
               list_of_block.at(*multiple_labels_block)->PushFront(temp_label, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Labels inverted");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Splitting first label");

            current_tree_node = current_list_of_stmt.front();
            list_of_block.at(*multiple_labels_block)->RemoveStmt(current_tree_node, AppM);
            /// Each label has to be put into a different basic block; first label is treated in a different way since
            /// it can have multiple predecessor Create new basic block
            blocRef new_bb(new bloc(list_of_block.rbegin()->first + 1));
            new_bb->loop_id = current_block->loop_id;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(new_bb->number));
            list_of_block[new_bb->number] = new_bb;
            new_bb->list_of_pred = current_block->list_of_pred;
            /// Updating successors of the predecessor of the current block when they are not the switch
            for(auto predecessor : current_block->list_of_pred)
            {
               if(predecessor != bbi)
               {
                  auto& successors = list_of_block.at(predecessor)->list_of_succ;
                  successors.erase(std::find(successors.begin(), successors.end(), *multiple_labels_block));
                  successors.push_back(new_bb->number);
               }
            }
            new_bb->PushFront(current_tree_node, AppM);
            bb->list_of_succ.push_back(new_bb->number);
            blocRef previous_block = new_bb;
            auto next = current_list_of_stmt.begin();
            next = current_list_of_stmt.size() > 1 ? ++next : next;
            while(current_list_of_stmt.size() > 1 and (*next)->get_kind() == gimple_label_K)
            {
               ++next;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Splitting next label");
               /// This is an intermediate label
               current_tree_node = current_list_of_stmt.front();
               list_of_block.at(*multiple_labels_block)->RemoveStmt(current_tree_node, AppM);
               /// Create new basic block
               new_bb = blocRef(new bloc(list_of_block.rbegin()->first + 1));
               new_bb->loop_id = previous_block->loop_id;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(new_bb->number));
               list_of_block[new_bb->number] = new_bb;
               new_bb->list_of_pred.push_back(previous_block->number);
               previous_block->list_of_succ.push_back(new_bb->number);
               new_bb->PushFront(current_tree_node, AppM);
               previous_block = new_bb;

               ld = GetPointer<gimple_label>(current_tree_node)->op;
               if(cases.find(ld) != cases.end())
               {
                  new_bb->list_of_pred.push_back(bbi);
                  bb->list_of_succ.push_back(new_bb->number);
               }
            }
            /// Predecessor of current node has to be fixed
            current_block->list_of_pred.clear();
            current_block->list_of_pred.push_back(previous_block->number);
            previous_block->list_of_succ.push_back(*multiple_labels_block);

            /// Check if this is a case : and not a label : - if so remove  this from successor of switch, otherwise add
            /// switch to predecessor of this
            current_tree_node = current_list_of_stmt.front();

            ld = GetPointer<gimple_label>(current_tree_node)->op;
            if(cases.find(ld) == cases.end())
            {
               std::vector<unsigned int>& switch_list_of_succ = bb->list_of_succ;

               THROW_ASSERT(std::find(switch_list_of_succ.begin(), switch_list_of_succ.end(), current_block->number) !=
                                switch_list_of_succ.end(),
                            "current block not found between successors of current one");

               switch_list_of_succ.erase(
                   std::find(switch_list_of_succ.begin(), switch_list_of_succ.end(), current_block->number));
            }
            else
            {
               current_block->list_of_pred.push_back(bbi);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Split BB" + STR(*multiple_labels_block));
         }
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            PrintTreeManager(false);
         }
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            WriteBBGraphDot("BB_After_" + GetName() + "_BB" + STR(bbi) + ".dot");
         }
         CustomUnorderedSet<unsigned int> to_be_fixed;
         for(const auto succ : bb->list_of_succ)
         {
            if(list_of_block.at(succ)->list_of_pred.size() > 1)
            {
               to_be_fixed.insert(succ);
            }
         }
         CustomUnorderedSet<unsigned int>::const_iterator t, t_end = to_be_fixed.end();
         for(t = to_be_fixed.begin(); t != t_end; ++t)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing BB" + STR(*t));
            // Creating new basic block
            blocRef new_bb(new bloc(list_of_block.rbegin()->first + 1));
            new_bb->loop_id = bb->loop_id;
            list_of_block[new_bb->number] = new_bb;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding BB" + STR(new_bb->number));
            new_bb->list_of_pred.push_back(bbi);
            new_bb->list_of_succ.push_back(*t);
            size_t i, i_end;
            i_end = bb->list_of_succ.size();
            for(i = 0; i != i_end; i++)
            {
               if(bb->list_of_succ[i] == *t)
               {
                  bb->list_of_succ[i] = new_bb->number;
                  break;
               }
            }
            // if this flag true, there are more than one predecessor of *t basic block which end with a switch
            bool multiple_pred_switch = false;
            i_end = list_of_block.at(*t)->list_of_pred.size();
            for(i = 0; i != i_end; i++)
            {
               if(list_of_block.at(*t)->list_of_pred[i] == bbi)
               {
                  list_of_block.at(*t)->list_of_pred[i] = new_bb->number;
                  for(const auto& phi : list_of_block.at(*t)->CGetPhiList())
                  {
                     auto* current_phi = GetPointer<gimple_phi>(phi);
                     for(const auto& def_edge : current_phi->CGetDefEdgesList())
                     {
                        if(def_edge.second == bbi)
                        {
                           current_phi->ReplaceDefEdge(TM, def_edge,
                                                       gimple_phi::DefEdge(def_edge.first, new_bb->number));
                        }
                     }
                  }
               }
               else
               {
                  if(list_of_block.at(list_of_block.at(*t)->list_of_pred[i]) and
                     list_of_block.at(list_of_block.at(*t)->list_of_pred[i])->CGetStmtList().size())
                  {
                     if(list_of_block.at(list_of_block.at(*t)->list_of_pred[i])->CGetStmtList().back()->get_kind() ==
                        gimple_switch_K)
                     {
                        multiple_pred_switch = true;
                     }
                  }
               }
            }
            // moving label expr
            THROW_ASSERT(list_of_block.at(*t)->CGetStmtList().front()->get_kind() == gimple_label_K,
                         "BB" + STR(*t) + " follows switch " + STR(bbi) + " but it does not start with a label");
            if(multiple_pred_switch)
            {
               auto label_expr_node = list_of_block.at(*t)->CGetStmtList().front();
               auto* le = GetPointer<gimple_label>(label_expr_node);
               auto* ld = GetPointer<label_decl>(le->op);
               unsigned int new_label_decl_id = TM->new_tree_node_id();
               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
               IR_schema[TOK(TOK_TYPE)] = STR(ld->type->index);
               IR_schema[TOK(TOK_SCPE)] = STR(ld->scpe->index);
               IR_schema[TOK(TOK_SRCP)] = ld->include_name + ":" + STR(ld->line_number) + ":" + STR(ld->column_number);
               IR_schema[TOK(TOK_ARTIFICIAL)] = STR(ld->artificial_flag);
               const auto new_label_decl = TM->create_tree_node(new_label_decl_id, label_decl_K, IR_schema);
               unsigned int new_label_expr_id = TM->new_tree_node_id();
               IR_schema.clear();
               IR_schema[TOK(TOK_SCPE)] = STR(ld->scpe->index);
               IR_schema[TOK(TOK_OP)] = STR(new_label_decl->index);
               IR_schema[TOK(TOK_SRCP)] = le->include_name + ":" + STR(le->line_number) + ":" + STR(ld->column_number);
               const auto new_label = TM->create_tree_node(new_label_expr_id, gimple_label_K, IR_schema);
               auto* te = GetPointer<tree_vec>(gs->op1);
               std::vector<tree_nodeRef>& list_of_op = te->list_of_op;
               for(auto& ind : list_of_op)
               {
                  auto* cl = GetPointer<case_label_expr>(ind);
                  if(cl->got->index == le->op->index)
                  {
                     cl->got = new_label_decl;
                  }
               }
               new_bb->PushFront(new_label, AppM);
            }
            else
            {
               auto label = list_of_block.at(*t)->CGetStmtList().front();
               list_of_block.at(*t)->RemoveStmt(label, AppM);
               new_bb->PushFront(label, AppM);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed BB" + STR(*t));
         }
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            PrintTreeManager(false);
         }
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            WriteBBGraphDot("BB_After_" + GetName() + "_BB" + STR(bbi) + ".dot");
         }
         if(AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing switch");
            /// create the gimple_multi_way_if node
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
            unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
            IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
            IR_schema[TOK(TOK_SCPE)] = STR(function_id);
            const auto new_gwi_node = TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
            auto new_gwi = GetPointer<gimple_multi_way_if>(new_gwi_node);

            /// Map between label decl index and corresponding case value
            CustomUnorderedMap<unsigned int, TreeNodeConstSet> case_labels;
            const auto gotos = GetPointer<const tree_vec>(gs->op1);
            for(const auto& goto_ : gotos->list_of_op)
            {
               const auto cle = GetPointer<const case_label_expr>(goto_);
               THROW_ASSERT(cle, STR(goto_));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Destination: " + STR(cle->got->index) + " CLE " + STR(cle->index));
               case_labels[cle->got->index].insert(goto_);
            }

            /// The destination of the default
            unsigned int default_bb = 0;

            /// Building list of conds
            for(auto succ : bb->list_of_succ)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successor BB" + STR(succ));
               const auto label = list_of_block.at(succ)->CGetStmtList().front();
               const auto gl = GetPointer<const gimple_label>(label);
               THROW_ASSERT(gl, STR(label));
               tree_nodeRef cond = nullptr;
               for(const auto& case_label : case_labels.at(gl->op->index))
               {
                  const auto cle = GetPointer<const case_label_expr>(case_label);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering case " + cle->ToString());
                  if(cle->op0)
                  {
                     const auto eq = tree_man->CreateEqExpr(gs->op0, cle->op0, bb, function_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Equality is " + GetPointer<const ssa_name>(eq)->CGetDefStmt()->ToString());
                     cond = cond ? tree_man->CreateOrExpr(cond, eq, bb, function_id) : eq;
                     if(cle->op1)
                     {
                        const auto low_value = tree_helper::GetConstValue(cle->op0);
                        const auto high_value = tree_helper::GetConstValue(cle->op1);
                        for(integer_cst_t index = low_value + 1; index <= high_value; index++)
                        {
                           cond = tree_man->CreateOrExpr(
                               cond,
                               tree_man->CreateEqExpr(
                                   gs->op0, TM->CreateUniqueIntegerCst(index, tree_helper::CGetType(cle->op0)), bb,
                                   function_id),
                               bb, function_id);
                        }
                     }
                  }
                  /// Default
                  else
                  {
                     default_bb = succ;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
               if(cond)
               {
                  new_gwi->add_cond(cond, succ);
               }
               list_of_block.at(succ)->RemoveStmt(label, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered successor BB" + STR(succ));
            }
            new_gwi->add_cond(tree_nodeRef(), default_bb);
            std::cout << "Removing2: " << gs_node << "\n"
                      << "Before2  : " << bb->CGetStmtList().back() << "\n";
            bb->RemoveStmt(gs_node, AppM);
            bb->PushBack(new_gwi_node, AppM);
            AppM->RegisterTransformation(GetName(), new_gwi_node);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            PrintTreeManager(false);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(bbi));
   }

   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
