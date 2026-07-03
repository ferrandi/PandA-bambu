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
 *              Copyright (C) 2021-2026 Politecnico di Milano
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
 * @file FunctionCallOpt.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "FunctionCallOpt.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <functional>

#define PARAMETER_INLINE_MAX_COST "inline-max-cost"
#define DEAFULT_MAX_INLINE_CONST 60

CustomSet<unsigned int> FunctionCallOpt::always_inline;
CustomSet<unsigned int> FunctionCallOpt::never_inline;
CustomMap<unsigned int, CustomSet<std::tuple<unsigned int, FunctionOptType>>> FunctionCallOpt::opt_call;
size_t FunctionCallOpt::inline_max_cost = DEAFULT_MAX_INLINE_CONST;

static const std::set<std::string> inlinedFunctionByDefault{"__mul32", "__umul32", "__mul64", "__umul64"};

static CustomUnorderedMap<kind, size_t> op_costs = {
    {call_node_K, 8}, {mul_node_K, 3},        {widen_mul_node_K, 3},  {idiv_node_K, 3},
    {irem_node_K, 3}, {bitcast_node_K, 0},    {nop_node_K, 0},        {ssa_node_K, 0},
    {addr_node_K, 0}, {concat_bit_node_K, 0}, {extract_bit_node_K, 0}};

/**
 * Generate unique id from call argument list
 * @param tns list of arguments
 * @return std::string unique id for the given list of arguments
 */
static std::string __arg_suffix(const std::vector<ir_nodeRef>& tns)
{
   std::stringstream suffix;
   for(const auto& tn : tns)
   {
      if(ir_helper::IsConstant(tn))
      {
         suffix << tn;
      }
      else
      {
         suffix << "_" << STR(tn->index);
      }
   }
   const auto hash = std::hash<std::string>{}(suffix.str());
   suffix.str(std::string());
   suffix << "_" << std::hex << hash;
   return suffix.str();
}

/**
 * Check if given call statement performs a call with all constant arguments
 * @param call_tn considered call statement IR node
 * @return true If all arguments of the call are constants
 * @return false If any argument is not constant
 */
static bool __has_constant_args(const ir_nodeConstRef& call_tn)
{
   const auto all_constant = [](const std::vector<ir_nodeRef>& tns) {
      for(const auto& tn : tns)
      {
         if(!ir_helper::IsConstant(tn))
         {
            return false;
         }
      }
      return true;
   };
   const auto call_kind = call_tn->get_kind();
   if(call_kind == call_stmt_K)
   {
      const auto gc = GetPointerS<const call_stmt>(call_tn);
      return all_constant(gc->args);
   }
   else if(call_kind == assign_stmt_K)
   {
      const auto ga = GetPointerS<const assign_stmt>(call_tn);
      THROW_ASSERT(ga->op1->get_kind() == call_node_K, "");
      const auto ce = GetPointerS<const call_node>(ga->op1);
      return all_constant(ce->args);
   }
   THROW_UNREACHABLE("Expected call statement: " + ir_node::GetString(call_kind) + " - " + STR(call_tn));
   return false;
}

/**
 * Check if a statement has virtual SSA variables
 *
 * @param gn node_stmt to check
 * @return true if vdef/vuse/vovers are found
 * @return false if no vdef/vuse/vovers are found
 */
static bool __has_virtuals(const node_stmt* gn)
{
   return gn->vdef || gn->vuses.size() || gn->vovers.size();
}

FunctionCallOpt::FunctionCallOpt(const ParameterConstRef Param, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_OPT, _design_flow_manager, Param), caller_bb()
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FunctionCallOpt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FUNCTION_CALL_OPT, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, CALLED_FUNCTIONS));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
            relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool FunctionCallOpt::HasToBeExecuted() const
{
   for(const auto& id_bb : caller_bb)
   {
      if(AppM->CGetFunctionBehavior(id_bb.first)->GetBBVersion() != id_bb.second)
      {
         return true;
      }
   }
   return FunctionFrontendFlowStep::HasToBeExecuted();
}

void FunctionCallOpt::Initialize()
{
   if(never_inline.empty())
   {
      const auto TM = AppM->get_ir_manager();
      if(parameters->IsParameter(PARAMETER_INLINE_MAX_COST))
      {
         inline_max_cost = parameters->GetParameter<size_t>(PARAMETER_INLINE_MAX_COST);
      }
      if(parameters->isOption(OPT_inline_functions))
      {
         const auto fnames = string_to_container<std::vector<std::string>>(
             parameters->getOption<std::string>(OPT_inline_functions), ",");
         for(const auto& fname_cond : fnames)
         {
            const auto toks = string_to_container<std::vector<std::string>>(fname_cond, "=");
            const auto& fname = toks.at(0);
            const auto fnode = TM->GetFunction(fname);
            if(fnode)
            {
               auto& inline_set = (!(toks.size() > 1) || toks.at(1) == "1") ? always_inline : never_inline;
               inline_set.insert(fnode->index);
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                              "Required " + STR((!(toks.size() > 1) || toks.at(1) == "1") ? "always" : "never") +
                                  " inline for function " + fname);
            }
            else
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Required inline function not found: " + fname);
            }
         }
      }
      for(const auto& defaultInlineFunction : inlinedFunctionByDefault)
      {
         const auto fnode = TM->GetFunction(defaultInlineFunction);
         if(fnode)
         {
            always_inline.insert(fnode->index);
         }
      }
      const auto HLSMgr = GetPointer<HLS_manager>(AppM);
      for(const auto& [symbol, arch] : *HLSMgr->module_arch)
      {
         THROW_ASSERT(arch, "Expected initialized architecture for function " + symbol);
         const auto inline_attr = arch->attrs.find(FunctionArchitecture::func_inline);
         if(inline_attr != arch->attrs.end())
         {
            const auto fnode = TM->GetFunction(symbol);
            if(fnode)
            {
               if(inline_attr->second == "self")
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Required always inline for function " + symbol);
                  always_inline.insert(fnode->index);
               }
               else if(inline_attr->second == "off")
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Required never inline for function " + symbol);
                  never_inline.insert(fnode->index);
               }
               else if(inline_attr->second == "recursive")
               {
                  THROW_WARNING("Recursive inline not yet supported.");
               }
            }
            else
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                              "Required inline (" + inline_attr->second + ") function not found: " + symbol);
            }
         }
      }
      never_inline.insert(0);
   }
   caller_bb.clear();
}

DesignFlowStep_Status FunctionCallOpt::InternalExec()
{
   THROW_ASSERT(!parameters->IsParameter("function-opt") || parameters->GetParameter<bool>("function-opt"),
                "Function call optimization should not be executed");
   if(GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) &&
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Function already scheduled, inlining is not possible any more");
      return DesignFlowStep_Status::UNCHANGED;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto fd = GetPointerS<function_val_node>(fnode);
   THROW_ASSERT(fd->body, "");
   const auto sl = GetPointerS<statement_list_node>(fd->body);
   bool modified = false;
   const auto opt_stmts = opt_call.find(function_id);
   if(opt_stmts != opt_call.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Performing call optimization:");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      ir_manipulationRef ir_man(new ir_manipulation(TM, parameters, AppM));
      for(const auto& stmt_opt : opt_stmts->second)
      {
         const auto stmt_id = std::get<0>(stmt_opt);
         const auto opt_type = std::get<1>(stmt_opt);
         const auto stmt = TM->GetIRNode(stmt_id);
         if(stmt)
         {
            const auto gn = GetPointerS<const node_stmt>(stmt);
            const auto bb_it = sl->list_of_bloc.find(gn->bb_index);
            if(gn->bb_index != 0 && bb_it != sl->list_of_bloc.end())
            {
               const auto& bb = bb_it->second;
               if(std::find_if(bb->CGetStmtList().cbegin(), bb->CGetStmtList().cend(),
                               [&](const ir_nodeRef& tn) { return tn->index == stmt_id; }) != bb->CGetStmtList().cend())
               {
                  if(!AppM->ApplyNewTransformation())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Max transformations reached, skipping function inlining...");
                     break;
                  }
                  if(opt_type == FunctionOptType::INLINE)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Inlining required for call statement " + stmt->ToString());
                     ir_man->InlineFunctionCall(stmt, fnode);
                     AppM->RegisterTransformation(GetName(), nullptr);
                     modified = true;
                  }
                  else if(opt_type == FunctionOptType::VERSION)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Versioning required for call statement " + stmt->ToString());
                     const auto version_suffix = [&]() -> std::string {
                        const auto call_stmt_tn = stmt;
                        if(call_stmt_tn->get_kind() == call_stmt_K)
                        {
                           const auto gc = GetPointerS<const call_stmt>(call_stmt_tn);
                           return __arg_suffix(gc->args);
                        }
                        else if(call_stmt_tn->get_kind() == assign_stmt_K)
                        {
                           const auto ga = GetPointerS<const assign_stmt>(call_stmt_tn);
                           const auto ce = GetPointer<const call_node>(ga->op1);
                           return __arg_suffix(ce->args);
                        }
                        THROW_UNREACHABLE("Unsupported call statement: " + call_stmt_tn->get_kind_text() + " " +
                                          STR(stmt));

                        return "";
                     }();
                     const auto _modified = ir_man->VersionFunctionCall(stmt, fnode, version_suffix);
                     if(_modified)
                     {
                        AppM->RegisterTransformation(GetName(), stmt);
                        modified = true;
                     }
                  }
                  else
                  {
                     THROW_UNREACHABLE("Unknown function call optimization type: " + STR(opt_type));
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Statement " + STR(stmt_id) + " was not present in BB" + STR(bb->number));
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "BB" + STR(gn->bb_index) + " was not found in current function");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Statement " + STR(stmt_id) + " was not in IR manager");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Performed call inlining");
      opt_call.erase(function_id);
   }

   if(parameters->isOption(OPT_top_design_name) &&
      TM->GetFunction(parameters->getOption<std::string>(OPT_top_design_name))->index == function_id)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Current function is marked as top RTL design, skipping...");
   }
   else if(never_inline.count(function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Current function is marked as never inline, skipping...");
   }
   else if(AppM->isOmpLambdaFunction(function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Current function is marked as OpenMP lambda, skipping...");
   }
   else
   {
      const auto& CGM = AppM->CGetCallGraphManager();
      const auto& CG = CGM.GetCallGraph();
      const auto function_v = CGM.GetVertex(function_id);
      const auto call_count = [&]() -> size_t {
         size_t call_points = 0u;
         for(const auto& ie : CG.in_edges(function_v))
         {
            const auto caller_info = CG.CGetEdgeInfo(ie);
            call_points += caller_info.direct_call_points.size();
            call_points += caller_info.indirect_call_points.size();
            call_points += caller_info.function_addresses.size();
         }
         return call_points;
      }();
      if(call_count != 0)
      {
         const auto loop_count = detect_loops(sl);
         const auto body_cost = compute_cost(sl);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Current function information:");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Internal loops : " + STR(loop_count));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call points    : " + STR(call_count));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Body cost      : " + STR(body_cost));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Force inline   : " + STR(always_inline.count(function_id) ? "yes" : "no"));
         const bool inline_funciton = always_inline.count(function_id) || ((body_cost * call_count) <= inline_max_cost);

         for(const auto& ie : CG.in_edges(function_v))
         {
            const auto caller_id = CGM.get_function(ie.m_source);
            caller_bb.insert(std::make_pair(caller_id, AppM->CGetFunctionBehavior(caller_id)->GetBBVersion()));
            const auto caller_node = TM->GetIRNode(caller_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Analysing call points from " + ir_helper::GetFunctionName(caller_node));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            THROW_ASSERT(caller_node->get_kind() == function_val_node_K, "");
            const auto caller_fd = GetPointerS<const function_val_node>(caller_node);
            THROW_ASSERT(caller_fd->body, "");
            const auto caller_sl = GetPointerS<const statement_list_node>(caller_fd->body);
            const auto is_unique_bb_call = [&](const node_stmt* gn) -> bool {
               THROW_ASSERT(caller_sl->list_of_bloc.count(gn->bb_index), "BB" + STR(gn->bb_index) +
                                                                             " not found in function " +
                                                                             ir_helper::GetFunctionName(caller_node));
               const auto& bb = caller_sl->list_of_bloc.at(gn->bb_index);
               for(const auto& tn : bb->CGetStmtList())
               {
                  if(tn->index != gn->index && (tn->get_kind() == call_stmt_K ||
                                                (tn->get_kind() == assign_stmt_K &&
                                                 GetPointerS<const assign_stmt>(tn)->op1->get_kind() == call_node_K)))
                  {
                     return false;
                  }
               }
               return true;
            };

            const auto& caller_info = CG.CGetEdgeInfo(ie);
            bool all_inlined = true;
            for(const auto& call_id : caller_info.direct_call_points)
            {
               const auto call_tn = TM->GetIRNode(call_id);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing direct call point " + STR(call_tn));
               const auto call_gn = GetPointerS<const node_stmt>(call_tn);
               const auto may_resolve_to_const =
                   __has_constant_args(call_tn) && loop_count == 0 && body_cost <= inline_max_cost;
               if(__has_virtuals(call_gn))
               {
                  // TODO: alias analysis is not able to handle inlining yet
                  all_inlined = false;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Virtual SSA handling not yet implemented, skipping...");
                  continue;
               }
               if(inline_funciton || may_resolve_to_const)
               {
                  if(loop_count == 0 || is_unique_bb_call(call_gn))
                  {
                     opt_call[caller_id].insert(std::make_pair(call_id, FunctionOptType::INLINE));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" +
                                        STR(always_inline.count(function_id) ? "Always inline function" :
                                                                               "Low body cost function") +
                                        ", inlining required");
                     continue;
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---BB" + STR(call_gn->bb_index) + " from function " +
                                        ir_helper::GetFunctionName(caller_node) +
                                        " has multiple call points, skipping...");
                  }
               }
               if(may_resolve_to_const)
               {
                  opt_call[caller_id].insert(std::make_pair(call_id, FunctionOptType::VERSION));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Current call has all constant arguments, versioning required");
                  continue;
               }
               all_inlined = false;
            }
            if(all_inlined)
            {
               caller_bb.erase(caller_id);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Analysed call points from " + ir_helper::GetFunctionName(caller_node));
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Current function has zero call points, skipping analysis...");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(modified)
   {
      function_behavior->UpdateBBVersion();
      function_behavior->UpdateBitValueVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

void FunctionCallOpt::RequestCallOpt(const ir_nodeConstRef& call_tn, unsigned int caller_id, FunctionOptType opt)
{
#if HAVE_ASSERTS
   const auto stmt_kind = call_tn->get_kind();
   bool is_call = stmt_kind == call_stmt_K;
   if(stmt_kind == assign_stmt_K)
   {
      const auto rhs = GetPointerS<const assign_stmt>(call_tn)->op1;
      const auto rhs_kind = rhs->get_kind();
      is_call |= rhs_kind == call_node_K;
   }
#endif
   THROW_ASSERT(is_call, "Statement does not contain a call");
   opt_call[caller_id].insert(std::make_pair(call_tn->index, opt));
}

size_t FunctionCallOpt::compute_cost(const statement_list_node* body)
{
   size_t total_cost = 0;
   for(const auto& bb : body->list_of_bloc)
   {
      for(const auto& stmt_rdx : bb.second->CGetStmtList())
      {
         const auto stmt = stmt_rdx;
         if(stmt->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointerS<const assign_stmt>(stmt);
            const auto op_kind = ga->op1->get_kind();
            const auto op_cost = op_costs.find(op_kind);
            if(op_cost != op_costs.end())
            {
               total_cost += op_cost->second;
            }
            else
            {
               if(op_kind == shl_node_K || op_kind == shr_node_K || op_kind == and_node_K || op_kind == or_node_K)
               {
                  total_cost += !ir_helper::IsConstant(GetPointerS<const binary_node>(ga->op1)->op1);
               }
               else
               {
                  total_cost += 1;
               }
            }
         }
         else if(stmt->get_kind() == call_stmt_K)
         {
            total_cost += 8;
         }
         else if(stmt->get_kind() == multi_way_if_stmt_K)
         {
            const auto mw = GetPointerS<const multi_way_if_stmt>(stmt);
            total_cost += mw->list_of_cond.size() * 8ULL;
         }
      }
   }
   return total_cost;
}

size_t FunctionCallOpt::detect_loops(const statement_list_node* body) const
{
   /// store the IR BB graph ala boost::graph
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   for(const auto& block : body->list_of_bloc)
   {
      inverse_vertex_map.insert(std::make_pair(block.first, bb_graphs_collection.AddVertex(BBNodeInfo(block.second))));
   }

   /// add edges
   for(const auto& curr_bb_pair : body->list_of_bloc)
   {
      const auto& curr_bbi = curr_bb_pair.first;
      const auto& curr_bb = curr_bb_pair.second;
      for(const auto& lop : curr_bb->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)),
                      "BB" + STR(lop) + " (successor of BB" + STR(curr_bbi) + ") does not exist");
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bbi), CFG_SELECTOR);
      }

      for(const auto& los : curr_bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(curr_bb->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                      CFG_SELECTOR);
      }
   }
   bb_graphs_collection.AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                CFG_SELECTOR);
   const auto sccs = cfg.GetStronglyConnectedComponents();
   size_t loop_count = 0;
   for(const auto& scc : sccs)
   {
      if(scc.size() > 1)
      {
         loop_count++;
         continue;
      }
      THROW_ASSERT(scc.size() == 1, "");
      const auto bb_v = scc.front();
      const auto& bb = cfg.CGetNodeInfo(bb_v).block;
      if(std::find(bb->list_of_succ.begin(), bb->list_of_succ.end(), bb->number) != bb->list_of_succ.end())
      {
         loop_count++;
      }
   }
   return loop_count;
}
