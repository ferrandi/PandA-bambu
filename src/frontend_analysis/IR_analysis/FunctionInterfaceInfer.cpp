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
 *              Copyright (C) 2018-2022 Politecnico di Milano
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
 * @file FunctionInterfaceInfer.cpp
 * @brief Restructure the top level function to adhere the specified interface.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

/// Header include
#include "FunctionInterfaceInfer.hpp"

///. include
#include "Parameter.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// parser/compiler include
#include "token_interface.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "area_model.hpp"
#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_model.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "constant_strings.hpp"
#include "copyrights_strings.hpp"

#include "language_writer.hpp"

#include "hls_step.hpp" // for HLSFlowStep_Type

#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/regex.hpp>

#define EPSILON 0.000000001
#define ENCODE_FDNAME(argName_string, MODE, interfaceType) \
   ((argName_string) + STR_CST_interface_parameter_keyword + (MODE) + (interfaceType))

FunctionInterfaceInfer::FunctionInterfaceInfer(const application_managerRef _AppM, unsigned int _function_id,
                                               const DesignFlowManagerConstRef _design_flow_manager,
                                               const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_INTERFACE_INFER, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

FunctionInterfaceInfer::~FunctionInterfaceInfer() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FunctionInterfaceInfer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, CALLED_FUNCTIONS));
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

void FunctionInterfaceInfer::ComputeRelationships(DesignFlowStepSet& relationship,
                                                  const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step ?
                 design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

void FunctionInterfaceInfer::Computepar2ssa(const statement_list* sl, std::map<unsigned, unsigned>& par2ssa)
{
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         TreeNodeMap<size_t> used_ssa = tree_helper::ComputeSsaUses(stmt);
         for(const auto& s : used_ssa)
         {
            const auto ssa_tn = GET_CONST_NODE(s.first);
            const auto ssa = GetPointerS<const ssa_name>(ssa_tn);
            if(ssa->var != nullptr && GET_CONST_NODE(ssa->var)->get_kind() == parm_decl_K)
            {
               const auto par_id = GET_INDEX_CONST_NODE(ssa->var);
               if(par2ssa.find(par_id) != par2ssa.end())
               {
                  THROW_ASSERT(par2ssa.at(par_id) == GET_INDEX_CONST_NODE(s.first), "unexpected condition");
               }
               else
               {
                  par2ssa[par_id] = GET_INDEX_CONST_NODE(s.first);
               }
            }
         }
      }
   }
}

void FunctionInterfaceInfer::classifyArgRecurse(CustomOrderedSet<unsigned>& Visited, const ssa_name* argSSA,
                                                const statement_list* sl, bool& isRead, bool& isWrite,
                                                bool& unkwown_pattern, std::list<tree_nodeRef>& writeStmt,
                                                std::list<tree_nodeRef>& readStmt)
{
   tree_nodeRef readType;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---SSA VARIABLE: " + argSSA->ToString() + " with " + STR(argSSA->CGetUseStmts().size()) +
                      " use statements");
   for(const auto& par_use : argSSA->CGetUseStmts())
   {
      const auto use_stmt = GET_CONST_NODE(par_use.first);
      if(!Visited.insert(GET_INDEX_CONST_NODE(par_use.first)).second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---SKIPPED STMT: " + use_stmt->ToString());
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---STMT: " + use_stmt->ToString());
      if(const auto ga = GetPointer<const gimple_assign>(use_stmt))
      {
         if(GET_CONST_NODE(ga->op0)->get_kind() == mem_ref_K)
         {
            if(GET_CONST_NODE(ga->op1)->get_kind() == mem_ref_K)
            {
               unkwown_pattern = true;
               THROW_WARNING("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
            }
            else
            {
               THROW_ASSERT(GET_CONST_NODE(ga->op1)->get_kind() == ssa_name_K ||
                                GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)),
                            "unexpected condition");
               if(GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)) ||
                  GetPointer<const ssa_name>(GET_CONST_NODE(ga->op1)) != argSSA)
               {
                  isWrite = true;
                  writeStmt.push_back(par_use.first);
               }
            }
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == mem_ref_K)
         {
            isRead = true;
            readType = GetPointerS<const mem_ref>(GET_CONST_NODE(ga->op1))->type;
            readStmt.push_back(par_use.first);
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == call_expr_K)
         {
            unkwown_pattern = true;
            THROW_WARNING("Pattern currently not supported: parameter passed as a parameter to another function " +
                          use_stmt->ToString());
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == nop_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == view_convert_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == ssa_name_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == pointer_plus_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == cond_expr_K)
         {
            const auto op0SSA = GetPointerS<const ssa_name>(GET_CONST_NODE(ga->op0));
            THROW_ASSERT(argSSA, "unexpected condition");
            classifyArgRecurse(Visited, op0SSA, sl, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
         }
         else
         {
            THROW_ERROR("Pattern currently not supported: parameter used in a non-supported statement " +
                        use_stmt->ToString() + ":" + GET_CONST_NODE(ga->op1)->get_kind_text());
         }
      }
      else if(const auto gc = GetPointer<const gimple_call>(use_stmt))
      {
         // look for the actual vs formal parameter binding
#if HAVE_ASSERTS
         bool found = false;
#endif
         unsigned par_index = 0;
         for(const auto& par : gc->args)
         {
            const auto par_node = GET_CONST_NODE(par);
            const auto ssaPar = GetPointerS<const ssa_name>(par_node);
            if(ssaPar)
            {
               if(argSSA == ssaPar)
               {
#if HAVE_ASSERTS
                  found = true;
#endif
                  break;
               }
            }
            else if(par_node)
            {
               THROW_ERROR("unexpected pattern: " + par_node->ToString());
            }
            ++par_index;
         }
         THROW_ASSERT(found, "found: " + argSSA->ToString() + " index=" + STR(par_index));
         THROW_ASSERT(gc->fn, "unexpected condition");
         const auto fn_node = GET_CONST_NODE(gc->fn);
         if(fn_node->get_kind() == addr_expr_K)
         {
            const auto ae = GetPointerS<const addr_expr>(fn_node);
            const auto ae_op = GET_CONST_NODE(ae->op);
            if(ae_op->get_kind() == function_decl_K)
            {
               const auto fd2 = GetPointerS<const function_decl>(ae_op);
               THROW_ASSERT(fd2->body, "unexpected condition");
               const auto sl2 = GetPointerS<const statement_list>(GET_CONST_NODE(fd2->body));
               std::map<unsigned, unsigned> par2ssa;
               Computepar2ssa(sl2, par2ssa);
               unsigned par2_index = 0;
               auto arg_id = 0u;
               std::string pdName_string2;
               for(const auto& arg : fd2->list_of_args)
               {
                  if(par2_index == par_index)
                  {
                     arg_id = GET_INDEX_CONST_NODE(arg);
                     const auto pd = GetPointerS<const parm_decl>(GET_CONST_NODE(arg));
                     const auto pdName = GET_CONST_NODE(pd->name);
                     THROW_ASSERT(GetPointer<const identifier_node>(pdName), "unexpected condition");
                     pdName_string2 = GetPointerS<const identifier_node>(pdName)->strg;
                     break;
                  }
                  ++par2_index;
               }
               THROW_ASSERT(arg_id, "unexpected condition");

               if(par2ssa.find(arg_id) != par2ssa.end())
               {
                  /// propagate design interfaces
                  const auto HLSMgr = GetPointer<HLS_manager>(AppM);
                  const auto par_node = GET_CONST_NODE(argSSA->var);
                  const auto pd = GetPointerS<const parm_decl>(par_node);
                  const auto pdName = GET_CONST_NODE(pd->name);
                  THROW_ASSERT(GetPointer<const identifier_node>(pdName), "unexpected condition");
                  const auto& pdName_string = GetPointerS<const identifier_node>(pdName)->strg;
                  const auto fun_node = GET_CONST_NODE(pd->scpe);
                  THROW_ASSERT(fun_node && fun_node->get_kind() == function_decl_K, "unexpected condition");
                  const auto fd = GetPointerS<const function_decl>(fun_node);
                  std::string fname;
                  tree_helper::get_mangled_fname(fd, fname);
                  if(HLSMgr->design_interface_arraysize.find(fname) != HLSMgr->design_interface_arraysize.end() &&
                     HLSMgr->design_interface_arraysize.at(fname).find(pdName_string) !=
                         HLSMgr->design_interface_arraysize.at(fname).end())
                  {
                     std::string fname2;
                     tree_helper::get_mangled_fname(fd2, fname2);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---fname2=" + fname2 + " pdName_string2=" + pdName_string2 + "\n");
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---fname=" + fname + " pdName_string=" + pdName_string + "\n");
                     HLSMgr->design_interface_arraysize[fname2][pdName_string2] =
                         HLSMgr->design_interface_arraysize.at(fname).at(pdName_string);
                  }
                  if(HLSMgr->design_interface_attribute2.find(fname) != HLSMgr->design_interface_attribute2.end() &&
                     HLSMgr->design_interface_attribute2.at(fname).find(pdName_string) !=
                         HLSMgr->design_interface_attribute2.at(fname).end())
                  {
                     std::string fname2;
                     tree_helper::get_mangled_fname(fd2, fname2);
                     HLSMgr->design_interface_attribute2[fname2][pdName_string2] =
                         HLSMgr->design_interface_attribute2.at(fname).at(pdName_string);
                  }
                  if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() &&
                     HLSMgr->design_interface_attribute3.at(fname).find(pdName_string) !=
                         HLSMgr->design_interface_attribute3.at(fname).end())
                  {
                     std::string fname2;
                     tree_helper::get_mangled_fname(fd2, fname2);
                     HLSMgr->design_interface_attribute3[fname2][pdName_string2] =
                         HLSMgr->design_interface_attribute3.at(fname).at(pdName_string);
                  }
                  const auto TM = AppM->get_tree_manager();
                  const auto argSSANode = TM->CGetTreeNode(par2ssa.find(arg_id)->second);
                  const auto argSSA2 = GetPointer<const ssa_name>(argSSANode);
                  THROW_ASSERT(argSSA2, "unexpected condition");
                  classifyArgRecurse(Visited, argSSA2, sl2, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Sub-function done\n");
               }
            }
            else
            {
               THROW_ERROR("unexpected pattern: " + ae_op->ToString());
            }
         }
         else if(fn_node)
         {
            THROW_ERROR("unexpected pattern: " + fn_node->ToString());
         }
         else
         {
            THROW_ERROR("unexpected pattern");
         }
      }
      else if(const auto gp = GetPointer<const gimple_phi>(use_stmt))
      {
         const auto op0SSA = GetPointerS<const ssa_name>(GET_CONST_NODE(gp->res));
         THROW_ASSERT(argSSA, "unexpected condition");
         THROW_ASSERT(!argSSA->virtual_flag, "unexpected condition");
         classifyArgRecurse(Visited, op0SSA, sl, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
      }
      else
      {
         unkwown_pattern = true;
         THROW_WARNING("USE PATTERN unexpected" + use_stmt->ToString());
      }
   }
}

void FunctionInterfaceInfer::classifyArg(statement_list* sl, tree_nodeRef argSSANode, bool& isRead, bool& isWrite,
                                         bool& unkwown_pattern, std::list<tree_nodeRef>& writeStmt,
                                         std::list<tree_nodeRef>& readStmt)
{
   const auto argSSA = GetPointer<const ssa_name>(GET_CONST_NODE(argSSANode));
   THROW_ASSERT(argSSA, "unexpected condition");
   CustomOrderedSet<unsigned> Visited;
   classifyArgRecurse(Visited, argSSA, sl, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
}

void FunctionInterfaceInfer::FixReadWriteCall(const gimple_assign* ga, gimple_node* newGN,
                                              const tree_manipulationRef tree_man, tree_nodeRef new_call,
                                              statement_list* sl, const tree_managerRef TM, tree_nodeRef origStmt,
                                              unsigned int destBB, const std::string& fname,
                                              const std::string& argName_string)
{
   newGN->memdef = ga->memdef;
   newGN->memuse = ga->memuse;
   if(ga->vdef)
   {
      auto ssaVDefVar = GetPointer<ssa_name>(GET_NODE(ga->vdef));
      THROW_ASSERT(ssaVDefVar, "unexpected condition");
      auto newSSAVdef =
          tree_man->create_ssa_name(ssaVDefVar->var, ssaVDefVar->type, tree_nodeRef(), tree_nodeRef(), false, true);
      newGN->vdef = newSSAVdef;
      GetPointerS<ssa_name>(GET_NODE(newGN->vdef))->SetDefStmt(new_call);
      const auto StmtVdefUses = ssaVDefVar->CGetUseStmts();
      for(const auto& used : StmtVdefUses)
      {
         TM->ReplaceTreeNode(used.first, ga->vdef, newSSAVdef);
      }
   }
   const auto StmtVusesUses = ga->vuses;
   for(const auto& vUse : StmtVusesUses)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(vUse));
      if(newGN->AddVuse(vUse))
      {
         sn->AddUseStmt(new_call);
      }
   }
   for(const auto& vOver : ga->vovers)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(vOver));
      if(newGN->AddVover(vOver))
      {
         sn->AddUseStmt(new_call);
      }
   }
   sl->list_of_bloc[destBB]->RemoveStmt(origStmt, AppM);
   GetPointer<HLS_manager>(AppM)->design_interface_loads[fname][destBB][argName_string].push_back(
       GET_INDEX_NODE(new_call));
}

void FunctionInterfaceInfer::create_Read_function(tree_nodeRef origStmt, const std::string& argName_string,
                                                  const std::string& fdName, tree_nodeRef aType, tree_nodeRef readType,
                                                  const tree_manipulationRef tree_man, const tree_managerRef TM,
                                                  bool commonRWSignature)
{
   THROW_ASSERT(origStmt, "expected a ref statement");
   THROW_ASSERT(GET_NODE(origStmt)->get_kind() == gimple_assign_K, "unexpected condition");
   const auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
   unsigned int destBB = ga->bb_index;
   THROW_ASSERT(ga->scpe, "expected a scope");
   THROW_ASSERT(GET_NODE(ga->scpe)->get_kind() == function_decl_K, "expected a function_decl");
   const auto fd = GetPointer<function_decl>(GET_NODE(ga->scpe));
   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   tree_nodeRef bit_size_type;
   tree_nodeRef boolean_type;
   if(commonRWSignature)
   {
      boolean_type = tree_man->GetBooleanType();
      bit_size_type = tree_man->GetUnsignedIntegerType();
      argsT.push_back(boolean_type);
      argsT.push_back(bit_size_type);
      argsT.push_back(readType);
   }
   argsT.push_back(aType);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   auto function_decl_node = tree_man->create_function_decl(fdName, fd->scpe, argsT, readType, srcp, false);
   std::vector<tree_nodeRef> args;
   if(commonRWSignature)
   {
      const auto sel_value = TM->CreateUniqueIntegerCst(0, boolean_type);
      args.push_back(sel_value);
      const auto size_value = TM->CreateUniqueIntegerCst(tree_helper::Size(readType), bit_size_type);
      args.push_back(size_value);
      tree_nodeRef data_value;
      if(GET_NODE(readType)->get_kind() == integer_type_K || GET_NODE(readType)->get_kind() == enumeral_type_K ||
         GET_NODE(readType)->get_kind() == pointer_type_K || GET_NODE(readType)->get_kind() == reference_type_K)
      {
         data_value = TM->CreateUniqueIntegerCst(0, readType);
      }
      else if(tree_helper::IsRealType(readType))
      {
         data_value = TM->CreateUniqueRealCst(0.l, readType);
      }
      else
      {
         THROW_ERROR("unexpected data type");
      }
      args.push_back(data_value);
   }

   THROW_ASSERT(GET_NODE(ga->op1)->get_kind() == mem_ref_K, "unexpected condition");
   const auto mr = GetPointer<mem_ref>(GET_NODE(ga->op1));
   args.push_back(mr->op0);

   const auto call_expr_node = tree_man->CreateCallExpr(function_decl_node, args, srcp);
   const auto new_call = tree_man->CreateGimpleAssign(readType, tree_nodeRef(), tree_nodeRef(), call_expr_node,
                                                      GET_INDEX_NODE(ga->scpe), destBB, srcp);
   sl->list_of_bloc.at(destBB)->PushBefore(new_call, origStmt, AppM);

   const auto newGN = GetPointer<gimple_assign>(GET_NODE(new_call));
   const tree_nodeRef temp_ssa_var = newGN->op0;
   const auto ssaDefVar = GetPointer<ssa_name>(GET_NODE(ga->op0));
   THROW_ASSERT(ssaDefVar, "unexpected condition");
   const auto StmtUses = ssaDefVar->CGetUseStmts();
   for(const auto& used : StmtUses)
   {
      TM->ReplaceTreeNode(used.first, ga->op0, temp_ssa_var);
   }

   FixReadWriteCall(ga, newGN, tree_man, new_call, sl, TM, origStmt, destBB, fname, argName_string);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LOAD STMT: " + new_call->ToString() + " in function " + fname);
}

void FunctionInterfaceInfer::create_Write_function(const std::string& argName_string, tree_nodeRef origStmt,
                                                   const std::string& fdName, tree_nodeRef writeValue,
                                                   tree_nodeRef aType, tree_nodeRef writeType,
                                                   const tree_manipulationRef tree_man, const tree_managerRef TM,
                                                   bool commonRWSignature)
{
   THROW_ASSERT(origStmt, "expected a ref statement");
   THROW_ASSERT(GET_NODE(origStmt)->get_kind() == gimple_assign_K, "unexpected condition");
   auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
   THROW_ASSERT(GET_NODE(ga->op0)->get_kind() == mem_ref_K, "unexpected condition");
   unsigned int destBB = ga->bb_index;
   THROW_ASSERT(ga->scpe, "expected a scope");
   THROW_ASSERT(GET_NODE(ga->scpe)->get_kind() == function_decl_K, "expected a function_decl");
   auto fd = GetPointer<function_decl>(GET_NODE(ga->scpe));
   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   tree_nodeRef boolean_type;
   const auto bit_size_type = tree_man->GetUnsignedIntegerType();
   const auto size_value = TM->CreateUniqueIntegerCst(tree_helper::Size(writeType), bit_size_type);

   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   if(commonRWSignature)
   {
      boolean_type = tree_man->GetBooleanType();
      argsT.push_back(boolean_type);
   }
   argsT.push_back(bit_size_type);
   if(tree_helper::IsSignedIntegerType(writeValue))
   {
      argsT.push_back(tree_man->CreateUnsigned(tree_helper::CGetType(writeValue)));
   }
   else
   {
      argsT.push_back(writeType);
   }
   argsT.push_back(aType);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   const auto function_decl_node =
       tree_man->create_function_decl(fdName, fd->scpe, argsT, tree_man->GetVoidType(), srcp, false);

   std::vector<tree_nodeRef> args;
   if(commonRWSignature)
   {
      const auto sel_value = TM->CreateUniqueIntegerCst(1, boolean_type);
      args.push_back(sel_value);
   }
   args.push_back(size_value);
   if(tree_helper::IsSignedIntegerType(writeValue))
   {
      const auto ga_nop =
          tree_man->CreateNopExpr(writeValue, tree_man->CreateUnsigned(tree_helper::CGetType(writeValue)), nullptr,
                                  nullptr, GET_INDEX_NODE(ga->scpe));
      sl->list_of_bloc.at(destBB)->PushBefore(ga_nop, origStmt, AppM);
      args.push_back(GetPointerS<gimple_assign>(GET_NODE(ga_nop))->op0);
   }
   else
   {
      args.push_back(writeValue);
   }
   const auto mr = GetPointerS<mem_ref>(GET_NODE(ga->op0));
   args.push_back(mr->op0);

   const auto new_call = tree_man->create_gimple_call(function_decl_node, args, GET_INDEX_NODE(ga->scpe), srcp, destBB);
   sl->list_of_bloc.at(destBB)->PushBefore(new_call, origStmt, AppM);

   const auto newGN = GetPointer<gimple_node>(GET_NODE(new_call));
   FixReadWriteCall(ga, newGN, tree_man, new_call, sl, TM, origStmt, destBB, fname, argName_string);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---STORE STMT: " + new_call->ToString() + " in function " + fname);
}

void FunctionInterfaceInfer::create_resource_Read_simple(const std::set<std::string>& operations,
                                                         const std::string& argName_string,
                                                         const std::string& interfaceType, unsigned int inputBitWidth,
                                                         bool IO_port, unsigned n_resources, unsigned rwBWsize)
{
   const std::string ResourceName = ENCODE_FDNAME(argName_string, "_Read_", interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);
      bool isMultipleResource = interfaceType == "acknowledge" || interfaceType == "valid" ||
                                interfaceType == "handshake" || interfaceType == "fifo" || interfaceType == "axis";
      if(isMultipleResource)
      {
         GetPointer<module>(interface_top)->set_multi_unit_multiplicity(n_resources);
      }

      unsigned int address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      structural_type_descriptorRef rwtype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", rwBWsize));
      if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);
      }
      structural_objectRef addrPort;
      if(isMultipleResource)
      {
         addrPort = CM->add_port_vector("in1", port_o::IN, n_resources, interface_top, addrType);
      }
      else
      {
         addrPort = CM->add_port("in1", port_o::IN, interface_top, addrType); // this port has a fixed name
      }
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector("out1", port_o::OUT, n_resources, interface_top, rwtype);
      }
      else
      {
         CM->add_port("out1", port_o::OUT, interface_top, rwtype);
      }

      std::string port_data_name;
      if(interfaceType == "axis")
      {
         port_data_name = "_s_axis_" + argName_string + "_TDATA";
      }
      else
      {
         port_data_name = "_" + argName_string + (interfaceType == "fifo" ? "_dout" : (IO_port ? "_i" : ""));
      }
      auto inPort = CM->add_port(port_data_name, port_o::IN, interface_top, dataType);
      GetPointer<port_o>(inPort)->set_port_interface((interfaceType == "axis" || interfaceType == "fifo") ?
                                                         port_o::port_interface::PI_FDOUT :
                                                         port_o::port_interface::PI_RNONE);
      if(interfaceType == "acknowledge" || interfaceType == "handshake")
      {
         auto inPort_o_ack =
             CM->add_port("_" + argName_string + (IO_port ? "_i" : "") + "_ack", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_RACK);
      }
      if(interfaceType == "valid" || interfaceType == "handshake")
      {
         auto inPort_o_vld =
             CM->add_port("_" + argName_string + (IO_port ? "_i" : "") + "_vld", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_RVALID);
      }
      if(interfaceType == "fifo")
      {
         auto inPort_empty_n = CM->add_port("_" + argName_string + "_empty_n", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_EMPTY_N);
         auto inPort_read = CM->add_port("_" + argName_string + "_read", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_READ);
      }
      if(interfaceType == "axis")
      {
         auto inPort_empty_n =
             CM->add_port("_s_axis_" + argName_string + "_TVALID", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_S_AXIS_TVALID);
         auto inPort_read =
             CM->add_port("_s_axis_" + argName_string + "_TREADY", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_S_AXIS_TREADY);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR, "Read_" + interfaceType + ".cpp");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      if(!isMultipleResource)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" ||
            interfaceType == "axis")
         {
            op->bounded = false;
            op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         }
         else
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 0);
            op->time_m->set_stage_period(0.0);
         }
         op->time_m->set_synthesis_dependent(true);
      }
      if(isMultipleResource)
      {
         HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
      }
      /// otherwise no constraints are required for this resource
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
}

void FunctionInterfaceInfer::create_resource_Write_simple(const std::set<std::string>& operations,
                                                          const std::string& argName_string,
                                                          const std::string& interfaceType, unsigned int inputBitWidth,
                                                          bool IO_port, bool isDiffSize, unsigned n_resources,
                                                          bool is_real, unsigned rwBWsize)
{
   const std::string ResourceName = ENCODE_FDNAME(argName_string, "_Write_", interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);
      bool isAVH = interfaceType == "acknowledge" || interfaceType == "valid" || interfaceType == "handshake" ||
                   interfaceType == "fifo" || interfaceType == "axis" || interfaceType == "none_registered";
      bool isMultipleResource = isDiffSize || isAVH;

      if(isMultipleResource)
      {
         GetPointer<module>(interface_top)->set_multi_unit_multiplicity(n_resources);
      }

      unsigned int address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
      if(is_real)
      {
         dataType->type = structural_type_descriptor::REAL;
      }
      auto nbitDataSize = 32u - static_cast<unsigned>(__builtin_clz(rwBWsize));
      structural_type_descriptorRef rwsize =
          structural_type_descriptorRef(new structural_type_descriptor("bool", nbitDataSize));
      structural_type_descriptorRef rwtype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", rwBWsize));
      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      if(interfaceType == "none_registered" || interfaceType == "acknowledge" || interfaceType == "handshake" ||
         interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);
      }
      structural_objectRef addrPort;
      if(isMultipleResource)
      {
         CM->add_port_vector("in1", port_o::IN, n_resources, interface_top, rwsize);
         CM->add_port_vector("in2", port_o::IN, n_resources, interface_top, rwtype);
         addrPort = CM->add_port_vector("in3", port_o::IN, n_resources, interface_top, addrType);
      }
      else
      {
         CM->add_port("in1", port_o::IN, interface_top, rwsize);
         CM->add_port("in2", port_o::IN, interface_top, rwtype);
         addrPort = CM->add_port("in3", port_o::IN, interface_top, addrType);
      }
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      if(interfaceType == "none_registered" || interfaceType == "acknowledge" || interfaceType == "handshake" ||
         interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      }
      std::string port_data_name;
      if(interfaceType == "axis")
      {
         port_data_name = "_m_axis_" + argName_string + "_TDATA";
      }
      else
      {
         port_data_name = "_" + argName_string + (interfaceType == "fifo" ? "_din" : (IO_port ? "_o" : ""));
      }
      auto inPort_o = CM->add_port(port_data_name, port_o::OUT, interface_top, dataType);
      GetPointer<port_o>(inPort_o)->set_port_interface((interfaceType == "axis" || interfaceType == "fifo") ?
                                                           port_o::port_interface::PI_FDIN :
                                                           port_o::port_interface::PI_WNONE);
      if(interfaceType == "acknowledge" || interfaceType == "handshake")
      {
         auto inPort_o_ack =
             CM->add_port("_" + argName_string + (IO_port ? "_o" : "") + "_ack", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_WACK);
      }
      if(interfaceType == "valid" || interfaceType == "handshake")
      {
         auto inPort_o_vld =
             CM->add_port("_" + argName_string + (IO_port ? "_o" : "") + "_vld", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_WVALID);
      }
      if(interfaceType == "fifo")
      {
         auto inPort_full_n = CM->add_port("_" + argName_string + "_full_n", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_FULL_N);
         auto inPort_read = CM->add_port("_" + argName_string + "_write", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_WRITE);
      }
      if(interfaceType == "axis")
      {
         auto inPort_full_n =
             CM->add_port("_m_axis_" + argName_string + "_TREADY", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_M_AXIS_TREADY);
         auto inPort_read =
             CM->add_port("_m_axis_" + argName_string + "_TVALID", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_M_AXIS_TVALID);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3");
      const auto writer = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
      if((interfaceType == "none" || interfaceType == "none_registered") && !(isDiffSize && !isAVH) &&
         writer == HDLWriter_Language::VHDL)
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VHDL_GENERATOR,
                                  "Write_" + interfaceType + ((isDiffSize && !isAVH) ? "DS" : "") + "_VHDL.cpp");
      }
      else
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                                  "Write_" + interfaceType + ((isDiffSize && !isAVH) ? "DS" : "") + ".cpp");
      }
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      if(!isMultipleResource)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(auto fdName : operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         if(interfaceType == "acknowledge" || interfaceType == "handshake" || interfaceType == "fifo" ||
            interfaceType == "axis")
         {
            op->bounded = false;
            op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         }
         else if(interfaceType == "none_registered")
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 2);
            op->time_m->set_stage_period(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON);
            const ControlStep ii_cs(1);
            op->time_m->set_initiation_time(ii_cs);
         }
         else
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 0);
            op->time_m->set_stage_period(0.0);
         }
         op->time_m->set_synthesis_dependent(true);
      }
      /// add constraint on resource
      HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
}

void FunctionInterfaceInfer::create_resource_array(const std::set<std::string>& operationsR,
                                                   const std::set<std::string>& operationsW,
                                                   const std::string& portNameSpecializer,
                                                   const std::string& interfaceType, unsigned int inputBitWidth,
                                                   unsigned int arraySize, unsigned n_resources, unsigned alignment,
                                                   bool is_real, unsigned rwBWsize)
{
   const auto n_channels = parameters->getOption<unsigned int>(OPT_channels_number);
   const auto isDP = inputBitWidth <= 64 && n_resources == 1 && n_channels == 2;
   const auto NResources = isDP ? 2 : n_resources;
   const auto read_write_string = (isDP ? std::string("ReadWriteDP_") : std::string("ReadWrite_"));
   const auto ResourceName = ENCODE_FDNAME(portNameSpecializer, "", "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto device_type = HLS_T->get_target_device()->get_type();
   const auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      const structural_managerRef CM(new structural_manager(parameters));
      const structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointerS<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(NResources);

      const auto nbitAddres = 32u - static_cast<unsigned>(__builtin_clz(arraySize * alignment - 1));
      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbit = 32u - static_cast<unsigned>(__builtin_clz(arraySize - 1));
      const auto nbitDataSize = 32u - static_cast<unsigned>(__builtin_clz(rwBWsize));
      const structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef address_interface_type(new structural_type_descriptor("bool", nbit));
      const structural_type_descriptorRef dataType(new structural_type_descriptor("bool", inputBitWidth));
      if(is_real)
      {
         dataType->type = structural_type_descriptor::REAL;
      }
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtype(new structural_type_descriptor("bool", rwBWsize));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, NResources, interface_top, bool_type);

      CM->add_port_vector("in1", port_o::IN, NResources, interface_top, size1); // when 0 is a read otherwise is a write
      CM->add_port_vector("in2", port_o::IN, NResources, interface_top,
                          rwsize); // bit-width size of the written or read data
      const auto dataPort = CM->add_port_vector("in3", port_o::IN, NResources, interface_top,
                                                rwtype); // value written when the first operand is 1, 0 otherwise
      const auto addrPort = CM->add_port_vector("in4", port_o::IN, NResources, interface_top, addrType); // address
      GetPointerS<port_o>(dataPort)->set_port_alignment(nbitAddres);

      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation

      CM->add_port_vector("out1", port_o::OUT, NResources, interface_top, rwtype);

      const auto inPort_address =
          CM->add_port("_" + portNameSpecializer + "_address0", port_o::OUT, interface_top, address_interface_type);
      GetPointerS<port_o>(inPort_address)->set_port_interface(port_o::port_interface::PI_ADDRESS);
      GetPointerS<port_o>(inPort_address)->set_port_alignment(alignment);
      if(isDP)
      {
         const auto inPort_address1 =
             CM->add_port("_" + portNameSpecializer + "_address1", port_o::OUT, interface_top, address_interface_type);
         GetPointerS<port_o>(inPort_address1)->set_port_interface(port_o::port_interface::PI_ADDRESS);
         GetPointerS<port_o>(inPort_address1)->set_port_alignment(alignment);
      }

      const auto inPort_ce = CM->add_port("_" + portNameSpecializer + "_ce0", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(inPort_ce)->set_port_interface(port_o::port_interface::PI_CHIPENABLE);
      if(isDP)
      {
         const auto inPort_ce1 =
             CM->add_port("_" + portNameSpecializer + "_ce1", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_ce1)->set_port_interface(port_o::port_interface::PI_CHIPENABLE);
      }

      if(!operationsW.empty())
      {
         const auto inPort_we = CM->add_port("_" + portNameSpecializer + "_we0", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_we)->set_port_interface(port_o::port_interface::PI_WRITEENABLE);
         if(isDP)
         {
            const auto inPort_we1 =
                CM->add_port("_" + portNameSpecializer + "_we1", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(inPort_we1)->set_port_interface(port_o::port_interface::PI_WRITEENABLE);
         }
      }
      if(!operationsR.empty())
      {
         const auto inPort_din = CM->add_port("_" + portNameSpecializer + "_q0", port_o::IN, interface_top, dataType);
         GetPointerS<port_o>(inPort_din)->set_port_interface(port_o::port_interface::PI_DIN);
         if(isDP)
         {
            const auto inPort_din1 =
                CM->add_port("_" + portNameSpecializer + "_q1", port_o::IN, interface_top, dataType);
            GetPointerS<port_o>(inPort_din1)->set_port_interface(port_o::port_interface::PI_DIN);
         }
      }
      if(!operationsW.empty())
      {
         const auto inPort_dout = CM->add_port("_" + portNameSpecializer + "_d0", port_o::OUT, interface_top, dataType);
         GetPointerS<port_o>(inPort_dout)->set_port_interface(port_o::port_interface::PI_DOUT);
         if(isDP)
         {
            const auto inPort_dout1 =
                CM->add_port("_" + portNameSpecializer + "_d1", port_o::OUT, interface_top, dataType);
            GetPointerS<port_o>(inPort_dout1)->set_port_interface(port_o::port_interface::PI_DOUT);
         }
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               read_write_string + interfaceType + ".cpp");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_model::create_model(device_type, parameters);
      fu->area_m->set_area_value(0);

      /// add constraint on resource
      HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName] = NResources;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
   for(const auto& fdName : operationsR)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }
   for(const auto& fdName : operationsW)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }
   const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
   const auto bram_f_unit = TechMan->get_fu(isDP ? ARRAY_1D_STD_BRAM_NN_SDS : ARRAY_1D_STD_BRAM_SDS, LIBRARY_STD_FU);
   const auto bram_fu = GetPointerS<functional_unit>(bram_f_unit);
   const auto load_op_node = bram_fu->get_operation("LOAD");
   const auto load_op = GetPointerS<operation>(load_op_node);
   const auto load_delay = load_op->time_m->get_execution_time();
   const auto load_cycles = load_op->time_m->get_cycles();
   const auto load_ii = load_op->time_m->get_initiation_time();
   const auto load_sp = load_op->time_m->get_stage_period();
   for(const auto& fdName : operationsR)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_model::create_model(device_type, parameters);
      op->bounded = true;
      op->time_m->set_execution_time(load_delay, load_cycles);
      op->time_m->set_initiation_time(load_ii);
      op->time_m->set_stage_period(load_sp);
      op->time_m->set_synthesis_dependent(true);
   }
   const auto store_op_node = bram_fu->get_operation("STORE");
   const auto store_op = GetPointerS<operation>(store_op_node);
   const auto store_delay = store_op->time_m->get_execution_time();
   const auto store_cycles = store_op->time_m->get_cycles();
   const auto store_ii = store_op->time_m->get_initiation_time();
   const auto store_sp = store_op->time_m->get_stage_period();
   for(const auto& fdName : operationsW)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_model::create_model(device_type, parameters);
      op->bounded = true;
      op->time_m->set_execution_time(store_delay, store_cycles);
      op->time_m->set_initiation_time(store_ii);
      op->time_m->set_stage_period(store_sp);
      op->time_m->set_synthesis_dependent(true);
   }
}

void FunctionInterfaceInfer::create_resource_m_axi(const std::set<std::string>& operationsR,
                                                   const std::set<std::string>& operationsW,
                                                   const std::string& argName_string,
                                                   const std::string& portNameSpecializer,
                                                   const std::string& interfaceType, unsigned int inputBitWidth,
                                                   unsigned n_resources, m_axi_type mat, unsigned rwBWsize)
{
   const auto ResourceName = ENCODE_FDNAME(portNameSpecializer, "", "");
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   const auto device_type = HLS_T->get_target_device()->get_type();
   const auto set_op_time = [&](operation* op) {
      op->time_m = time_model::create_model(device_type, parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   };
   auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      auto interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointer<module>(interface_top)->set_multi_unit_multiplicity(n_resources);

      unsigned int address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef address_interface_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef Intype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
      structural_type_descriptorRef size1 = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
      auto nbitDataSize = 32u - static_cast<unsigned>(__builtin_clz(rwBWsize));
      structural_type_descriptorRef rwsize =
          structural_type_descriptorRef(new structural_type_descriptor("bool", nbitDataSize));
      structural_type_descriptorRef rwtype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", rwBWsize));
      structural_type_descriptorRef idType = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
      structural_type_descriptorRef lenType = structural_type_descriptorRef(new structural_type_descriptor("bool", 8));
      structural_type_descriptorRef sizeType = structural_type_descriptorRef(new structural_type_descriptor("bool", 3));
      structural_type_descriptorRef burstType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 2));
      structural_type_descriptorRef lockType = structural_type_descriptorRef(new structural_type_descriptor("bool", 2));
      structural_type_descriptorRef cacheType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 4));
      structural_type_descriptorRef protType = structural_type_descriptorRef(new structural_type_descriptor("bool", 3));
      structural_type_descriptorRef qosType = structural_type_descriptorRef(new structural_type_descriptor("bool", 4));
      structural_type_descriptorRef regionType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 4));
      structural_type_descriptorRef userType = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
      structural_type_descriptorRef strbType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth / 8));
      structural_type_descriptorRef respType = structural_type_descriptorRef(new structural_type_descriptor("bool", 2));

      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);

      CM->add_port_vector("in1", port_o::IN, n_resources, interface_top,
                          size1); // when 0 is a read otherwise is a write
      CM->add_port_vector("in2", port_o::IN, n_resources, interface_top,
                          rwsize); // bit-width size of the written or read data
      CM->add_port_vector("in3", port_o::IN, n_resources, interface_top,
                          rwtype); // value written when the first operand is 1, 0 otherwise
      auto addrPort =
          CM->add_port_vector("in4", port_o::IN, n_resources, interface_top, address_interface_type); // address

      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation

      CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      CM->add_port_vector("out1", port_o::OUT, n_resources, interface_top, rwtype);

      auto Port_awvalid =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWVALID", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_awvalid)->set_port_interface(port_o::port_interface::M_AXI_AWVALID);

      auto Port_awready =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWREADY", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_awready)->set_port_interface(port_o::port_interface::M_AXI_AWREADY);

      auto Port_awaddr =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWADDR", port_o::OUT, interface_top, address_interface_type);
      GetPointer<port_o>(Port_awaddr)->set_port_interface(port_o::port_interface::M_AXI_AWADDR);

      auto Port_awid = CM->add_port("_m_axi_" + portNameSpecializer + "_AWID", port_o::OUT, interface_top, idType);
      GetPointer<port_o>(Port_awid)->set_port_interface(port_o::port_interface::M_AXI_AWID);

      auto Port_awlen = CM->add_port("_m_axi_" + portNameSpecializer + "_AWLEN", port_o::OUT, interface_top, lenType);
      GetPointer<port_o>(Port_awlen)->set_port_interface(port_o::port_interface::M_AXI_AWLEN);

      auto Port_awsize =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWSIZE", port_o::OUT, interface_top, sizeType);
      GetPointer<port_o>(Port_awsize)->set_port_interface(port_o::port_interface::M_AXI_AWSIZE);

      auto Port_awburst =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWBURST", port_o::OUT, interface_top, burstType);
      GetPointer<port_o>(Port_awburst)->set_port_interface(port_o::port_interface::M_AXI_AWBURST);

      auto Port_awlock =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWLOCK", port_o::OUT, interface_top, lockType);
      GetPointer<port_o>(Port_awlock)->set_port_interface(port_o::port_interface::M_AXI_AWLOCK);

      auto Port_awcache =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWCACHE", port_o::OUT, interface_top, cacheType);
      GetPointer<port_o>(Port_awcache)->set_port_interface(port_o::port_interface::M_AXI_AWCACHE);

      auto Port_awprot =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWPROT", port_o::OUT, interface_top, protType);
      GetPointer<port_o>(Port_awprot)->set_port_interface(port_o::port_interface::M_AXI_AWPROT);

      auto Port_awqos = CM->add_port("_m_axi_" + portNameSpecializer + "_AWQOS", port_o::OUT, interface_top, qosType);
      GetPointer<port_o>(Port_awqos)->set_port_interface(port_o::port_interface::M_AXI_AWQOS);

      auto Port_awregion =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWREGION", port_o::OUT, interface_top, regionType);
      GetPointer<port_o>(Port_awregion)->set_port_interface(port_o::port_interface::M_AXI_AWREGION);

      auto Port_awuser =
          CM->add_port("_m_axi_" + portNameSpecializer + "_AWUSER", port_o::OUT, interface_top, userType);
      GetPointer<port_o>(Port_awuser)->set_port_interface(port_o::port_interface::M_AXI_AWUSER);

      auto Port_wvalid =
          CM->add_port("_m_axi_" + portNameSpecializer + "_WVALID", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_wvalid)->set_port_interface(port_o::port_interface::M_AXI_WVALID);

      auto Port_wready =
          CM->add_port("_m_axi_" + portNameSpecializer + "_WREADY", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_wready)->set_port_interface(port_o::port_interface::M_AXI_WREADY);

      auto Port_wdata = CM->add_port("_m_axi_" + portNameSpecializer + "_WDATA", port_o::OUT, interface_top, Intype);
      GetPointer<port_o>(Port_wdata)->set_port_interface(port_o::port_interface::M_AXI_WDATA);

      auto Port_wstrb = CM->add_port("_m_axi_" + portNameSpecializer + "_WSTRB", port_o::OUT, interface_top, strbType);
      GetPointer<port_o>(Port_wstrb)->set_port_interface(port_o::port_interface::M_AXI_WSTRB);

      auto Port_wlast = CM->add_port("_m_axi_" + portNameSpecializer + "_WLAST", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_wlast)->set_port_interface(port_o::port_interface::M_AXI_WLAST);

      auto Port_wid = CM->add_port("_m_axi_" + portNameSpecializer + "_WID", port_o::OUT, interface_top, idType);
      GetPointer<port_o>(Port_wid)->set_port_interface(port_o::port_interface::M_AXI_WID);

      auto Port_wuser = CM->add_port("_m_axi_" + portNameSpecializer + "_WUSER", port_o::OUT, interface_top, userType);
      GetPointer<port_o>(Port_wuser)->set_port_interface(port_o::port_interface::M_AXI_WUSER);

      auto Port_arvalid =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARVALID", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_arvalid)->set_port_interface(port_o::port_interface::M_AXI_ARVALID);

      auto Port_arready =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARREADY", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_arready)->set_port_interface(port_o::port_interface::M_AXI_ARREADY);

      auto Port_araddr =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARADDR", port_o::OUT, interface_top, address_interface_type);
      GetPointer<port_o>(Port_araddr)->set_port_interface(port_o::port_interface::M_AXI_ARADDR);

      auto Port_arid = CM->add_port("_m_axi_" + portNameSpecializer + "_ARID", port_o::OUT, interface_top, idType);
      GetPointer<port_o>(Port_arid)->set_port_interface(port_o::port_interface::M_AXI_ARID);

      auto Port_arlen = CM->add_port("_m_axi_" + portNameSpecializer + "_ARLEN", port_o::OUT, interface_top, lenType);
      GetPointer<port_o>(Port_arlen)->set_port_interface(port_o::port_interface::M_AXI_ARLEN);

      auto Port_arsize =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARSIZE", port_o::OUT, interface_top, sizeType);
      GetPointer<port_o>(Port_arsize)->set_port_interface(port_o::port_interface::M_AXI_ARSIZE);

      auto Port_arburst =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARBURST", port_o::OUT, interface_top, burstType);
      GetPointer<port_o>(Port_arburst)->set_port_interface(port_o::port_interface::M_AXI_ARBURST);

      auto Port_arlock =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARLOCK", port_o::OUT, interface_top, lockType);
      GetPointer<port_o>(Port_arlock)->set_port_interface(port_o::port_interface::M_AXI_ARLOCK);

      auto Port_arcache =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARCACHE", port_o::OUT, interface_top, cacheType);
      GetPointer<port_o>(Port_arcache)->set_port_interface(port_o::port_interface::M_AXI_ARCACHE);

      auto Port_arprot =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARPROT", port_o::OUT, interface_top, protType);
      GetPointer<port_o>(Port_arprot)->set_port_interface(port_o::port_interface::M_AXI_ARPROT);

      auto Port_arqos = CM->add_port("_m_axi_" + portNameSpecializer + "_ARQOS", port_o::OUT, interface_top, qosType);
      GetPointer<port_o>(Port_arqos)->set_port_interface(port_o::port_interface::M_AXI_ARQOS);

      CM->add_port("_m_axi_" + portNameSpecializer + "_ARREGION", port_o::OUT, interface_top, regionType);
      GetPointer<port_o>(Port_awlock)->set_port_interface(port_o::port_interface::M_AXI_ARREGION);

      auto Port_aruser =
          CM->add_port("_m_axi_" + portNameSpecializer + "_ARUSER", port_o::OUT, interface_top, userType);
      GetPointer<port_o>(Port_aruser)->set_port_interface(port_o::port_interface::M_AXI_ARUSER);

      auto Port_rvalid =
          CM->add_port("_m_axi_" + portNameSpecializer + "_RVALID", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_rvalid)->set_port_interface(port_o::port_interface::M_AXI_RVALID);

      auto Port_rready =
          CM->add_port("_m_axi_" + portNameSpecializer + "_RREADY", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_rready)->set_port_interface(port_o::port_interface::M_AXI_RREADY);

      auto Port_rdata = CM->add_port("_m_axi_" + portNameSpecializer + "_RDATA", port_o::IN, interface_top, Intype);
      GetPointer<port_o>(Port_rdata)->set_port_interface(port_o::port_interface::M_AXI_RDATA);

      auto Port_rlast = CM->add_port("_m_axi_" + portNameSpecializer + "_RLAST", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_rlast)->set_port_interface(port_o::port_interface::M_AXI_RLAST);

      auto Port_rid = CM->add_port("_m_axi_" + portNameSpecializer + "_RID", port_o::IN, interface_top, idType);
      GetPointer<port_o>(Port_rid)->set_port_interface(port_o::port_interface::M_AXI_RID);

      auto Port_ruser = CM->add_port("_m_axi_" + portNameSpecializer + "_RUSER", port_o::IN, interface_top, userType);
      GetPointer<port_o>(Port_ruser)->set_port_interface(port_o::port_interface::M_AXI_RUSER);

      auto Port_rresp = CM->add_port("_m_axi_" + portNameSpecializer + "_RRESP", port_o::IN, interface_top, respType);
      GetPointer<port_o>(Port_rresp)->set_port_interface(port_o::port_interface::M_AXI_RRESP);

      auto Port_bvalid =
          CM->add_port("_m_axi_" + portNameSpecializer + "_BVALID", port_o::IN, interface_top, bool_type);
      GetPointer<port_o>(Port_bvalid)->set_port_interface(port_o::port_interface::M_AXI_BVALID);

      auto Port_bready =
          CM->add_port("_m_axi_" + portNameSpecializer + "_BREADY", port_o::OUT, interface_top, bool_type);
      GetPointer<port_o>(Port_bready)->set_port_interface(port_o::port_interface::M_AXI_BREADY);

      auto Port_bresp = CM->add_port("_m_axi_" + portNameSpecializer + "_BRESP", port_o::IN, interface_top, respType);
      GetPointer<port_o>(Port_bresp)->set_port_interface(port_o::port_interface::M_AXI_BRESP);

      auto Port_bid = CM->add_port("_m_axi_" + portNameSpecializer + "_BID", port_o::IN, interface_top, idType);
      GetPointer<port_o>(Port_bid)->set_port_interface(port_o::port_interface::M_AXI_BID);

      auto Port_buser = CM->add_port("_m_axi_" + portNameSpecializer + "_BUSER", port_o::OUT, interface_top, userType);
      GetPointer<port_o>(Port_buser)->set_port_interface(port_o::port_interface::M_AXI_BUSER);

      if(mat == m_axi_type::axi_slave)
      {
         auto Port_LSawvalid = CM->add_port("_s_axi_AXILiteS_AWVALID", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWVALID);
         CM->add_port("_s_axi_AXILiteS_AWREADY", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWREADY);
         CM->add_port("_s_axi_AXILiteS_AWADDR", port_o::IN, interface_top, address_interface_type);
         GetPointer<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWADDR);
         CM->add_port("_s_axi_AXILiteS_WVALID", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_WREADY", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_WDATA", port_o::IN, interface_top, Intype);
         CM->add_port("_s_axi_AXILiteS_WSTRB", port_o::IN, interface_top, strbType);
         CM->add_port("_s_axi_AXILiteS_ARVALID", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_ARREADY", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_ARADDR", port_o::IN, interface_top, address_interface_type);
         CM->add_port("_s_axi_AXILiteS_RVALID", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_RREADY", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_RDATA", port_o::OUT, interface_top, Intype);
         CM->add_port("_s_axi_AXILiteS_RRESP", port_o::OUT, interface_top, respType);
         CM->add_port("_s_axi_AXILiteS_BVALID", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_BREADY", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_BRESP", port_o::OUT, interface_top, respType);
      }

      auto inPort_m_axi = CM->add_port("_" + argName_string, port_o::IN, interface_top, address_interface_type);
      if(mat == m_axi_type::none || mat == m_axi_type::axi_slave)
      {
         GetPointer<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_OFF);
      }
      else
      {
         GetPointer<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_DIRECT);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "ReadWrite_" + interfaceType + ".cpp");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operationsR)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      for(const auto& fdName : operationsW)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_model::create_model(device_type, parameters);
      fu->area_m->set_area_value(0);

      for(const auto& fdName : operationsR)
      {
         auto op = GetPointerS<operation>(fu->get_operation(fdName));
         set_op_time(op);
      }
      for(const auto& fdName : operationsW)
      {
         auto op = GetPointerS<operation>(fu->get_operation(fdName));
         set_op_time(op);
      }
      /// add constraint on resource
      HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
   else
   {
      for(const auto& fdName : operationsR)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      for(const auto& fdName : operationsW)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));

      for(const auto& fdName : operationsR)
      {
         auto op = GetPointerS<operation>(fu->get_operation(fdName));
         set_op_time(op);
      }
      for(const auto& fdName : operationsW)
      {
         auto op = GetPointerS<operation>(fu->get_operation(fdName));
         set_op_time(op);
      }
      const auto address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef address_interface_type(new structural_type_descriptor("bool", address_bitsize));
      const auto interface_top = fu->CM->get_circ();
      const auto inPort_m_axi =
          fu->CM->add_port("_" + argName_string, port_o::IN, interface_top, address_interface_type);
      if(mat == m_axi_type::none || mat == m_axi_type::axi_slave)
      {
         GetPointerS<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_OFF);
      }
      else
      {
         GetPointerS<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_DIRECT);
      }
   }
}

void FunctionInterfaceInfer::ComputeResourcesAlignment(unsigned& n_resources, unsigned& alignment,
                                                       unsigned int inputBitWidth, bool is_acType, bool is_signed,
                                                       bool is_fixed)
{
   n_resources = 1;
   if(inputBitWidth > 64 && inputBitWidth <= 128)
   {
      n_resources = 2;
   }
   else if(inputBitWidth > 128)
   {
      n_resources = inputBitWidth / 32 + (inputBitWidth % 32 ? 1 : 0);
      if(!is_signed && inputBitWidth % 32 == 0 && !is_fixed)
      {
         ++n_resources;
      }
   }
   /// compute alignment
   alignment = 1;
   if(inputBitWidth <= 8)
   {
      alignment = !is_acType ? 1 : 4;
   }
   else if(inputBitWidth <= 16)
   {
      alignment = !is_acType ? 2 : 4;
   }
   else if(inputBitWidth <= 32)
   {
      alignment = 4;
   }
   else if(inputBitWidth <= 64)
   {
      alignment = 8;
   }
   else if(inputBitWidth <= 128)
   {
      alignment = 16;
   }
   else
   {
      alignment = (inputBitWidth / 32) + 4 * (inputBitWidth % 32 ? 1 : 0);
      if(!is_signed && inputBitWidth % 32 == 0 && !is_fixed)
      {
         alignment += 4;
      }
   }
}

void FunctionInterfaceInfer::create_resource(const std::set<std::string>& operationsR,
                                             const std::set<std::string>& operationsW,
                                             const std::string& argName_string, const std::string& interfaceType,
                                             unsigned int inputBitWidth, bool isDiffSize, const std::string& fname,
                                             unsigned n_resources, unsigned alignment, bool isReal, unsigned rwBWsize)
{
   if(interfaceType == "none" || interfaceType == "none_registered" || interfaceType == "acknowledge" ||
      interfaceType == "valid" || interfaceType == "ovalid" || interfaceType == "handshake" ||
      interfaceType == "fifo" || interfaceType == "axis")
   {
      THROW_ASSERT(!operationsR.empty() || !operationsW.empty(), "unexpected condition");
      bool IO_P = !operationsR.empty() && !operationsW.empty();
      if(!operationsR.empty())
      {
         create_resource_Read_simple(operationsR, argName_string, (interfaceType == "ovalid" ? "none" : interfaceType),
                                     inputBitWidth, IO_P, n_resources, rwBWsize);
      }
      if(!operationsW.empty())
      {
         create_resource_Write_simple(operationsW, argName_string,
                                      (interfaceType == "ovalid" ? "valid" : interfaceType), inputBitWidth, IO_P,
                                      isDiffSize, n_resources, isReal, rwBWsize);
      }
   }
   else if(interfaceType == "array")
   {
      const auto HLSMgr = GetPointer<HLS_manager>(AppM);
      THROW_ASSERT(HLSMgr->design_interface_arraysize.find(fname) != HLSMgr->design_interface_arraysize.end() &&
                       HLSMgr->design_interface_arraysize.find(fname)->second.find(argName_string) !=
                           HLSMgr->design_interface_arraysize.find(fname)->second.end(),
                   "unexpected condition");
      const auto arraySizeSTR = HLSMgr->design_interface_arraysize.at(fname).at(argName_string);
      const auto arraySize = boost::lexical_cast<unsigned>(arraySizeSTR);
      if(arraySize == 0)
      {
         THROW_ERROR("array size equal to zero");
      }

      auto portNameSpecializer = argName_string;
      if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() &&
         HLSMgr->design_interface_attribute3.at(fname).find(argName_string) !=
             HLSMgr->design_interface_attribute3.at(fname).end())
      {
         portNameSpecializer = HLSMgr->design_interface_attribute3.at(fname).at(argName_string);
      }

      create_resource_array(operationsR, operationsW, portNameSpecializer, interfaceType, inputBitWidth, arraySize,
                            n_resources, alignment, isReal, rwBWsize);
   }
   else if(interfaceType == "m_axi")
   {
      auto mat = m_axi_type::none;
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      auto portNameSpecializer = argName_string;

      if(HLSMgr->design_interface_attribute2.find(fname) != HLSMgr->design_interface_attribute2.end() &&
         HLSMgr->design_interface_attribute2.at(fname).find(argName_string) !=
             HLSMgr->design_interface_attribute2.at(fname).end())
      {
         const auto& matString = HLSMgr->design_interface_attribute2.at(fname).at(argName_string);
         if(matString == "none")
         {
            mat = m_axi_type::none;
         }
         else if(matString == "direct")
         {
            mat = m_axi_type::direct;
            portNameSpecializer = "gmem";
         }
         else if(matString == "axi_slave")
         {
            mat = m_axi_type::axi_slave;
            portNameSpecializer = "gmem";
         }
         else
         {
            THROW_ERROR("non-supported m_axi attribute or malformed pragma");
         }
      }
      if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() &&
         HLSMgr->design_interface_attribute3.at(fname).find(argName_string) !=
             HLSMgr->design_interface_attribute3.at(fname).end())
      {
         portNameSpecializer = HLSMgr->design_interface_attribute3.at(fname).at(argName_string);
      }
      create_resource_m_axi(operationsR, operationsW, argName_string, portNameSpecializer, interfaceType, inputBitWidth,
                            n_resources, mat, rwBWsize);
   }
   else
   {
      THROW_ERROR("interface not supported: " + interfaceType);
   }
}

static const boost::regex signature_param_typename("((?:\\w+\\s*)+(?:<[^>]*>)?\\s*[\\*&]?\\s*)");

bool FunctionInterfaceInfer::HasToBeExecuted() const
{
   return bb_version == 0;
}

DesignFlowStep_Status FunctionInterfaceInfer::InternalExec()
{
   const auto top_functions = AppM->CGetCallGraphManager()->GetRootFunctions();
   const auto is_top = top_functions.find(function_id) != top_functions.end();
   if(GetPointer<const HLS_manager>(AppM) && is_top)
   {
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      const auto TM = AppM->get_tree_manager();
      const auto fnode = TM->CGetTreeNode(function_id);
      const auto fd = GetPointer<const function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      if(HLSMgr->design_interface_typename.find(fname) == HLSMgr->design_interface_typename.end())
      {
         const auto dfname = string_demangle(fname);
         if(!dfname.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extracting interface from signature " + fname);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Demangled as " + dfname);
            boost::sregex_token_iterator typename_it(dfname.begin(), dfname.end(), signature_param_typename, 0), end;
            ++typename_it; // First match is the function name
            auto& top_design_interface_typename = HLSMgr->design_interface_typename[fname];
            auto& top_design_interface_typename_signature = HLSMgr->design_interface_typename_signature[fname];
            auto& top_design_interface_typename_orig_signature =
                HLSMgr->design_interface_typename_orig_signature[fname];
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Iterating arguments: ");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            for(const auto& arg : fd->list_of_args)
            {
               THROW_ASSERT(typename_it != end, "");
               std::stringstream pname;
               pname << arg;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Argument " + pname.str());
               const std::string tname(*typename_it);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Typename " + tname);
               top_design_interface_typename[pname.str()] = tname;
               top_design_interface_typename_signature.push_back(tname);
               top_design_interface_typename_orig_signature.push_back(tname);
               if(tname.find("fixed<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname.str()] =
                      std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_fixed.h");
               }
               if(tname.find("int<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname.str()] =
                      std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_int.h");
               }
               ++typename_it;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }

      if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
      {
         bool modified = false;
         if(HLSMgr->design_interface.find(fname) != HLSMgr->design_interface.end())
         {
            const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
            /// pre-process the list of statements to bind parm_decl and ssa variables
            std::map<unsigned, unsigned> par2ssa;
            const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
            Computepar2ssa(sl, par2ssa);

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function " + fname);
            auto& DesignInterfaceArgs = HLSMgr->design_interface.at(fname);
            auto& DesignInterfaceTypenameArgs = HLSMgr->design_interface_typename.at(fname);
            for(const auto& arg : fd->list_of_args)
            {
               const auto arg_id = GET_INDEX_NODE(arg);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---parm_decl id: " + STR(arg_id));
               const auto a = GetPointer<parm_decl>(GET_NODE(arg));
               const auto& aType = a->type;
               const auto argName = GET_NODE(a->name);
               THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
               const auto& argName_string = GetPointer<identifier_node>(argName)->strg;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---parm_decl name: " + argName_string);
               THROW_ASSERT(DesignInterfaceArgs.count(argName_string), "Not matched parameter name: " + argName_string);
               auto interfaceType = DesignInterfaceArgs.at(argName_string);
               const auto interfaceTypename = DesignInterfaceTypenameArgs.at(argName_string);
               if(interfaceType != "default")
               {
                  if(par2ssa.find(arg_id) == par2ssa.end())
                  {
                     THROW_WARNING("parameter not used by any statement");
                     if(tree_helper::IsPointerType(aType))
                     {
                        DesignInterfaceArgs[argName_string] = "none";
                     }
                     else
                     {
                        THROW_ERROR("parameter not used: specified interface does not make sense - " + interfaceType);
                     }
                     continue;
                  }
                  if(interfaceType == "bus") /// TO BE FIXED
                  {
                     DesignInterfaceArgs[argName_string] = "default";
                     continue;
                  }
                  if(tree_helper::IsPointerType(aType))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is a pointer\n");
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---list of statement that use this parameter\n");
                     auto ptd_type = tree_helper::CGetPointedType(aType);
                     unsigned inputBitWidth;
                     if(tree_helper::IsArrayType(ptd_type))
                     {
                        inputBitWidth = tree_helper::GetArrayElementSize(ptd_type);
                     }
                     else
                     {
                        inputBitWidth = tree_helper::Size(ptd_type);
                     }
                     bool is_signed;
                     bool is_fixed;
                     auto acTypeBw = ac_type_bitwidth(interfaceTypename, is_signed, is_fixed);
                     bool is_acType = false;
                     if(acTypeBw)
                     {
                        is_acType = true;
                        inputBitWidth = acTypeBw;
                     }
                     unsigned n_resources;
                     unsigned alignment;
                     ComputeResourcesAlignment(n_resources, alignment, inputBitWidth, is_acType, is_signed, is_fixed);
                     THROW_ASSERT(inputBitWidth, "unexpected condition");

                     auto argSSANode = TM->GetTreeReindex(par2ssa.find(arg_id)->second);
                     bool isRead = false;
                     bool isWrite = false;
                     bool unkwown_pattern = false;
                     std::list<tree_nodeRef> writeStmt;
                     std::list<tree_nodeRef> readStmt;
                     const auto commonRWSignature = interfaceType == "array" || interfaceType == "m_axi";

                     classifyArg(sl, argSSANode, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);

                     if(unkwown_pattern)
                     {
                        THROW_ERROR("unknown pattern identified");
                     }
                     if(writeStmt.empty() && readStmt.empty())
                     {
                        THROW_ERROR("parameter " + argName_string + " cannot have interface " + interfaceType +
                                    " (no load or write is associated with it)");
                     }

                     if(isRead && isWrite)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---IO arg\n");
                        if(interfaceType == "ptrdefault")
                        {
                           if(parameters->IsParameter("none-ptrdefault") &&
                              parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[argName_string] = "none";
                              interfaceType = "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[argName_string] = "none_registered";
                              interfaceType = "none_registered";
                           }
                           else
                           {
                              DesignInterfaceArgs[argName_string] = "ovalid";
                              interfaceType = "ovalid";
                           }
                        }
                        else if(interfaceType == "fifo" || interfaceType == "axis")
                        {
                           THROW_ERROR("parameter " + argName_string + " cannot have interface " + interfaceType +
                                       " because it cannot be read and write at the same time");
                        }
                     }
                     else if(isRead)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---I arg\n");
                        if(interfaceType == "ptrdefault")
                        {
                           DesignInterfaceArgs[argName_string] = "none";
                           interfaceType = "none";
                        }
                        else if(interfaceType == "ovalid")
                        {
                           THROW_ERROR("parameter " + argName_string + " cannot have interface " + interfaceType +
                                       " because it is read only");
                        }
                     }
                     else if(isWrite)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---O arg\n");
                        if(interfaceType == "ptrdefault")
                        {
                           if(parameters->IsParameter("none-ptrdefault") &&
                              parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[argName_string] = "none";
                              interfaceType = "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[argName_string] = "none_registered";
                              interfaceType = "none_registered";
                           }
                           else
                           {
                              DesignInterfaceArgs[argName_string] = "valid";
                              interfaceType = "valid";
                           }
                        }
                     }
                     else
                     {
                        THROW_ERROR("pattern not yet supported: unused arg");
                     }
                     if(isRead && !isWrite)
                     {
                        std::set<std::string> operationsR, operationsW;
                        unsigned int loadIdIndex = 0;
                        bool is_real = false;
                        unsigned rwsize = 1;
                        for(const auto& rs : readStmt)
                        {
                           const auto instanceFname = ENCODE_FDNAME(
                               argName_string, "_Read_" + (n_resources == 1 ? "" : (STR(loadIdIndex) + "_")),
                               interfaceType);
                           operationsR.insert(instanceFname);
                           if(n_resources != 1)
                           {
                              ++loadIdIndex;
                           }
                           const auto rs_node = GET_NODE(rs);
                           const auto rs_ga = GetPointer<gimple_assign>(rs_node);
                           const auto readType = GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type;
                           is_real = is_real || tree_helper::IsRealType(readType);
                           create_Read_function(rs, argName_string, instanceFname, aType, readType, tree_man, TM,
                                                commonRWSignature);
                           rwsize = std::max(rwsize, tree_helper::Size(readType));
                        }
                        create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth, false,
                                        fname, n_resources, alignment, is_real, rwsize);
                        modified = true;
                     }
                     else if(isRead && isWrite)
                     {
                        std::set<std::string> operationsR, operationsW;
                        unsigned int IdIndex = 0;
                        bool is_real = false;
                        unsigned rwsize = 1;
                        for(const auto& rs : readStmt)
                        {
                           const auto instanceFname =
                               ENCODE_FDNAME(argName_string, "_Read_" + (n_resources == 1 ? "" : (STR(IdIndex) + "_")),
                                             (interfaceType == "ovalid" ? "none" : interfaceType));
                           operationsR.insert(instanceFname);
                           if(n_resources != 1)
                           {
                              ++IdIndex;
                           }
                           const auto rs_node = GET_NODE(rs);
                           const auto rs_ga = GetPointer<gimple_assign>(rs_node);
                           const auto readType = GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type;
                           is_real = is_real || tree_helper::IsRealType(readType);
                           create_Read_function(rs, argName_string, instanceFname, aType, readType, tree_man, TM,
                                                commonRWSignature);
                           rwsize = std::max(rwsize, tree_helper::Size(readType));
                        }
                        IdIndex = 0;
                        bool isDiffSize = false;
                        unsigned WrittenSize = 0;
                        for(const auto& ws : writeStmt)
                        {
                           const auto instanceFname =
                               ENCODE_FDNAME(argName_string, "_Write_" + (n_resources == 1 ? "" : (STR(IdIndex) + "_")),
                                             (interfaceType == "ovalid" ? "valid" : interfaceType));
                           operationsW.insert(instanceFname);
                           if(n_resources != 1)
                           {
                              ++IdIndex;
                           }

                           const auto ws_node = GET_NODE(ws);
                           const auto ws_ga = GetPointer<gimple_assign>(ws_node);
                           if(WrittenSize == 0)
                           {
                              WrittenSize = tree_helper::Size(ws_ga->op1);
                              if(WrittenSize < inputBitWidth)
                              {
                                 isDiffSize = true;
                              }
                           }
                           else if(WrittenSize != tree_helper::Size(ws_ga->op1) || WrittenSize < inputBitWidth)
                           {
                              isDiffSize = true;
                           }
                           rwsize = std::max(rwsize, tree_helper::Size(ws_ga->op1));
                           create_Write_function(argName_string, ws, instanceFname, ws_ga->op1, aType,
                                                 GetPointerS<mem_ref>(GET_NODE(ws_ga->op0))->type, tree_man, TM,
                                                 commonRWSignature);
                        }
                        create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth,
                                        isDiffSize, fname, n_resources, alignment, is_real, rwsize);
                        modified = true;
                     }
                     else if(!isRead && isWrite)
                     {
                        std::set<std::string> operationsR, operationsW;
                        unsigned int IdIndex = 0;
                        bool isDiffSize = false;
                        unsigned WrittenSize = 0;
                        bool is_real = false;
                        unsigned rwsize = 1;
                        for(const auto& ws : writeStmt)
                        {
                           const auto instanceFname =
                               ENCODE_FDNAME(argName_string, "_Write_" + (n_resources == 1 ? "" : (STR(IdIndex) + "_")),
                                             (interfaceType == "ovalid" ? "valid" : interfaceType));
                           operationsW.insert(instanceFname);
                           if(n_resources != 1)
                           {
                              ++IdIndex;
                           }

                           const auto ws_node = GET_NODE(ws);
                           const auto ws_ga = GetPointer<gimple_assign>(ws_node);
                           if(WrittenSize == 0)
                           {
                              WrittenSize = tree_helper::Size(ws_ga->op1);
                              if(WrittenSize < inputBitWidth)
                              {
                                 isDiffSize = true;
                              }
                           }
                           else if(WrittenSize != tree_helper::Size(ws_ga->op1) || WrittenSize < inputBitWidth)
                           {
                              isDiffSize = true;
                           }
                           rwsize = std::max(rwsize, tree_helper::Size(ws_ga->op1));
                           const auto writeType = GetPointerS<mem_ref>(GET_NODE(ws_ga->op0))->type;
                           is_real = is_real || tree_helper::IsRealType(writeType);
                           create_Write_function(argName_string, ws, instanceFname, ws_ga->op1, aType, writeType,
                                                 tree_man, TM, commonRWSignature);
                        }
                        create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth,
                                        isDiffSize, fname, n_resources, alignment, is_real, rwsize);
                        modified = true;
                     }
                     else
                     {
                        THROW_ERROR("pattern not yet supported");
                     }
                  }
                  else if(interfaceType == "none")
                  {
                     THROW_ERROR("unexpected interface (" + interfaceType + ") for parameter " + argName_string);
                  }
                  else
                  {
                     THROW_ERROR("not yet supported interface (" + interfaceType + ") for parameter " + argName_string);
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed function " + fname);
         }
         if(modified)
         {
            function_behavior->UpdateBBVersion();
         }
         return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
      }
   }
   return DesignFlowStep_Status::UNCHANGED;
}
