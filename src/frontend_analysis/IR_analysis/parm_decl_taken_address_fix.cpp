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
 * @file parm_decl_taken_address_fix.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "parm_decl_taken_address_fix.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

parm_decl_taken_address_fix::parm_decl_taken_address_fix(const ParameterConstRef params, const application_managerRef AM, unsigned int fun_id, const DesignFlowManagerConstRef dfm) : FunctionFrontendFlowStep(AM, fun_id, PARM_DECL_TAKEN_ADDRESS, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

parm_decl_taken_address_fix::~parm_decl_taken_address_fix() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> parm_decl_taken_address_fix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
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

DesignFlowStep_Status parm_decl_taken_address_fix::InternalExec()
{
   bool changed = false;
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_manipulationRef IRman = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd and fd->body, "Node " + STR(tn) + "is not a function_decl or has no body");
   const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   const std::string fu_name = tree_helper::name_function(TM, function_id);
   THROW_ASSERT(not GetPointer<const function_type>(tree_helper::CGetType(tn))->varargs_flag, "function " + fu_name + " is varargs");
   // compute the set of parm_decl for which an address is taken
   CustomOrderedSet<unsigned int> parm_decl_addr;
   std::map<unsigned int, tree_nodeRef> parm_decl_var_decl_rel;
   for(auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
         {
            const auto* ga = GetPointer<const gimple_assign>(GET_NODE(stmt));
            if(GET_NODE(ga->op1)->get_kind() == addr_expr_K)
            {
               auto* ae = GetPointer<addr_expr>(GET_NODE(ga->op1));
               if(GET_NODE(ae->op)->get_kind() == parm_decl_K)
                  parm_decl_addr.insert(GET_INDEX_NODE(ae->op));
            }
         }
      }
   }
   for(auto par_index : parm_decl_addr)
   {
      auto par = TM->CGetTreeReindex(par_index);
      const auto* pd = GetPointer<const parm_decl>(GET_NODE(par));
      THROW_ASSERT(pd, "unexpected condition");
      const tree_nodeRef p_type = pd->type;
      const std::string srcp = pd->include_name + ":" + STR(pd->line_number) + ":" + STR(pd->column_number);
      const std::string original_param_name = pd->name ? GetPointer<const identifier_node>(GET_NODE(pd->name))->strg : STR(par_index);
      const std::string local_var_name = "bambu_artificial_local_parameter_copy_" + original_param_name;
      const auto local_var_identifier = IRman->create_identifier_node(local_var_name);
      const auto new_local_var_decl = IRman->create_var_decl(local_var_identifier, p_type, pd->scpe, pd->size, tree_nodeRef(), tree_nodeRef(), srcp, GetPointer<const type_node>(GET_NODE(p_type))->algn, pd->used);
      parm_decl_var_decl_rel[par_index] = new_local_var_decl;

      for(auto& block : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
         for(const auto& stmt : block.second->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
            TM->ReplaceTreeNode(stmt, par, new_local_var_decl);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
      }
   }
   if(!parm_decl_addr.empty())
   {
      /// select the first basic block
      const auto entry_block = sl->list_of_bloc.at(BB_ENTRY);
      const auto succ_blocks = entry_block->list_of_succ;
      THROW_ASSERT(succ_blocks.size() == 1, "entry basic block of function " + fu_name + " has " + STR(succ_blocks.size()) + " successors");
      auto bb_index = *succ_blocks.begin();
      const auto first_block = sl->list_of_bloc.at(bb_index);
      for(auto par_index : parm_decl_addr)
      {
         auto par = TM->CGetTreeReindex(par_index);
         auto vd = parm_decl_var_decl_rel.at(par_index);
         const auto* pd = GetPointer<const parm_decl>(GET_NODE(par));
         THROW_ASSERT(pd, "unexpected condition");
         const std::string srcp_default = pd->include_name + ":" + STR(pd->line_number) + ":" + STR(pd->column_number);
         auto new_ga_expr = IRman->CreateGimpleAssignAddrExpr(GET_NODE(vd), bb_index, srcp_default);
         first_block->PushFront(new_ga_expr);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement statement " + GET_NODE(new_ga_expr)->ToString());
         auto* nge = GetPointer<gimple_assign>(GET_NODE(new_ga_expr));
         nge->temporary_address = true;
         tree_nodeRef ssa_addr = nge->op0;
         auto* sa = GetPointer<ssa_name>(GET_NODE(ssa_addr));
         tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(sa->type));

         const tree_nodeRef p_type = pd->type;
         auto mr = IRman->create_binary_operation(p_type, ssa_addr, offset, srcp_default, mem_ref_K);
         tree_nodeRef ssa_par = IRman->create_ssa_name(par, p_type, tree_nodeRef(), tree_nodeRef());
         tree_nodeRef ga = IRman->create_gimple_modify_stmt(mr, ssa_par, srcp_default, bb_index);
         first_block->PushAfter(ga, new_ga_expr);
         GetPointer<gimple_node>(GET_NODE(ga))->artificial = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement statement " + GET_NODE(ga)->ToString());
      }
      changed = true;
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
