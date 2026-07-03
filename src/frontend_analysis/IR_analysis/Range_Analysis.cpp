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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file Range_Analysis.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "Range_Analysis.hpp"

#include "Bit_Value_opt.hpp"
#include "CousotConstraintGraph.hpp"
#include "CropDFSConstraintGraph.hpp"
#include "Parameter.hpp"
#include "PhiOpNode.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "bit_lattice.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "range_analysis_helper.hpp"
#include "string_manipulation.hpp"

#include "config_HAVE_ASSERTS.hpp"

#include <set>
#include <vector>

#define BITVALUE_UPDATE // Read/write bitvalue information during the analysis

#define RA_EXEC_NORMAL 0
#define RA_EXEC_READONLY 1
#define RA_EXEC_SKIP 2

#ifndef NDEBUG
extern bool _ra_enable_abs;
extern bool _ra_enable_negate;
extern bool _ra_enable_sext;
extern bool _ra_enable_zext;
extern bool _ra_enable_tnot;

extern bool _ra_enable_add;
extern bool _ra_enable_sub;
extern bool _ra_enable_mul;
extern bool _ra_enable_sdiv;
extern bool _ra_enable_udiv;
extern bool _ra_enable_srem;
extern bool _ra_enable_urem;
extern bool _ra_enable_shl;
extern bool _ra_enable_shr;
extern bool _ra_enable_and;
extern bool _ra_enable_or;
extern bool _ra_enable_xor;
extern bool _ra_enable_min;
extern bool _ra_enable_max;

extern bool _ra_enable_ternary;

extern bool _ra_enable_load;

#define OPERATION_OPTION(opts, X)                                                                          \
   if((opts).erase("no_" #X))                                                                              \
   {                                                                                                       \
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: " #X " operation disabled"); \
      _ra_enable_##X = false;                                                                              \
   }
#else
#define OPERATION_OPTION(opts, X) void(0)
#endif

namespace
{
   // ========================================================================== //
   // Static global functions and definitions
   // ========================================================================== //

   int updateIR(const VarNode* varNode, const ir_managerRef& TM,
                int
#ifndef NDEBUG
                    debug_level
#endif
                ,
                const application_managerRef& AppM)
   {
      const auto& V = varNode->getValue();
      const auto& interval = varNode->getRange();
      if(ir_helper::IsConstant(V) || interval.isUnknown() || VarNode::makeId(V, BB_ENTRY) != varNode->getId())
      {
         return ut_None;
      }
      THROW_ASSERT(V->get_kind() == ssa_node_K, "");
      auto* SSA = GetPointerS<ssa_node>(TM->GetIRNode(V->index));

      const bool isSigned = range_analysis::isSignedType(SSA->type);
      if(!SSA->range.isUnknown())
      {
         if(SSA->range.isSameRange(interval))
         {
            return ut_None;
         }
         if(!AppM->ApplyNewTransformation())
         {
            return ut_None;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Modified range " + SSA->range.ToString() + " to " + interval.ToString() + " for " +
                            SSA->ToString() + " " + SSA->type->get_kind_text());
      }
      else
      {
         auto newBW = interval.getBitWidth();
         if(interval.isFullSet())
         {
            return ut_None;
         }
         if(interval.isConstant())
         {
            newBW = 0U;
         }
         else
         {
            if(interval.isRegular())
            {
               newBW = isSigned ? Range::neededBits(interval.getSignedMin(), interval.getSignedMax(), true) :
                                  Range::neededBits(interval.getUnsignedMin(), interval.getUnsignedMax(), false);
               const auto currentBW = ir_helper::TypeSize(V);
               if(newBW >= currentBW)
               {
                  return ut_None;
               }
            }
            else if(interval.isAnti())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Anti range " + interval.ToString() + " not stored for " + SSA->ToString() + " " +
                                  SSA->type->get_kind_text());
               return ut_None;
            }
            else if(interval.isEmpty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Empty range not stored for " + SSA->ToString() + " " + SSA->type->get_kind_text());
               return ut_None;
            }
         }
         if(!AppM->ApplyNewTransformation())
         {
            return ut_None;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Added range " + interval.ToString() + "<" + STR(newBW) + "> for " + SSA->ToString() + " " +
                            SSA->type->get_kind_text());
      }

      int updateState = ut_None;
      auto bit_values = string_to_bitstring(SSA->bit_values);
      if(interval.isAnti() || interval.isEmpty())
      {
         updateState = ut_Range;
      }
      else if(interval.isFullSet())
      {
         updateState = ut_Range;
#ifdef BITVALUE_UPDATE
         bit_values = interval.getBitValues(isSigned);
#endif
      }
      else
      {
         updateState = ut_Range;

#ifdef BITVALUE_UPDATE
         if(!bit_values.empty())
         {
            const auto range_bv = interval.getBitValues(isSigned);
            const auto sup_bv =
                sup(bit_values, range_bv, interval.getBitWidth(), isSigned, interval.getBitWidth() == 1);
            if(bit_values != sup_bv)
            {
               bit_values = sup_bv;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Range bit_values: " + bitstring_to_string(sup_bv));
            }
         }
         else
         {
            bit_values = interval.getBitValues(isSigned);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Range bit_values: " + bitstring_to_string(bit_values));
         }
#endif
      }

      if(!AppM->ApplyNewTransformation())
      {
         return ut_None;
      }

#ifdef BITVALUE_UPDATE
      const auto curr_bv = bitstring_to_string(bit_values);
      if(isBetter(curr_bv, SSA->bit_values))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---BitValue updated for " + SSA->ToString() + " " + SSA->type->get_kind_text() + ": " +
                            SSA->bit_values + " -> " + curr_bv);
         SSA->bit_values = curr_bv;
         Bit_Value_opt::constrainSSA(SSA, TM);
         AppM->RegisterTransformation("RangeAnalysis", V);
         updateState |= ut_BitValue;
      }
#endif
      return updateState;
   }

} // namespace

static void ParmAndRetValPropagation(unsigned int function_id, const application_managerRef& AppM,
                                     const ConstraintGraphRef& CG,
                                     int
#ifndef NDEBUG
                                         debug_level
#endif
)
{
   const auto& CGM = AppM->CGetCallGraphManager();
   const auto call_graph = [&]() {
      const auto& reached_functions = CGM.GetReachedBodyFunctions();
      CustomUnorderedSet<CallGraph::vertex_descriptor> rbf;
      std::transform(reached_functions.begin(), reached_functions.end(), std::inserter(rbf, rbf.end()),
                     [&](unsigned int id) { return CGM.GetVertex(id); });
      return CGM.CGetCallSubGraph(rbf);
   }();
   const auto f_v = CGM.GetVertex(function_id);
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto FD = GetPointerS<const function_val_node>(fnode);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Parameters and return value propagation on function " + ir_helper::GetFunctionName(fnode));
   const auto is_called = [&]() {
      if(CGM.GetRootFunctions().count(function_id))
      {
         return false;
      }

      for(auto ie : call_graph.in_edges(f_v))
      {
         const auto& einfo = call_graph.CGetEdgeInfo(ie);
         if(einfo.direct_call_points.size())
         {
            return true;
         }
      }
      return false;
   }();
   if(!is_called)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---No call statements for this function, skipping...");
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   // Fetch the function arguments (formal parameters) into the data structure and generate PhiOp nodes for parameters
   // call values
   std::vector<ir_nodeConstRef> parameters;
   std::vector<PhiOpNode*> matchers;
#ifndef NDEBUG
   auto pindex = 0;
#endif
   for(const auto& pnode : FD->list_of_args)
   {
      const auto ssa_id = AppM->getSSAFromParm(function_id, pnode->index);
      const auto ssa_node = TM->GetIRNode(ssa_id);
      if(ssa_node && range_analysis::isValidType(ssa_node))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Parameter " + std::to_string(pindex) + " defined as " + ssa_node->ToString());
         // TODO: use_bbi should be the BBI where the variable is first used inside the function
         const auto sink = CG->addVarNode(ssa_node, function_id);

         // Check for pragma mask directives user defined range
         THROW_ASSERT(pnode->get_kind() == argument_val_node_K, "");
         const auto parm = GetPointerS<const argument_val_node>(pnode);
         auto phiOp = new PhiOpNode(sink, nullptr);
         if(!parm->range.isUnknown())
         {
            sink->setRange(parm->range);
            phiOp->setIntersect(parm->range);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Range hints found in parameter declaration: " + parm->range.ToString());
         }
         else
         {
            sink->setRange(sink->getMaxRange());
         }
         parameters.push_back(ssa_node);
         matchers.push_back(phiOp);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Parameter " + std::to_string(pindex) + " unused or with invalid type");
         parameters.push_back(nullptr);
         matchers.push_back(nullptr);
      }
#ifndef NDEBUG
      ++pindex;
#endif
   }

   // Check if the function returns a supported value type. If not, no return
   // value matching is done
   const auto ret_type = ir_helper::GetFunctionReturnType(fnode);
   auto hasReturn = ret_type && range_analysis::isValidType(ret_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Function has " + (hasReturn ? ("return type " + ret_type->get_kind_text()) : "no return type"));

   // Creates the data structure which receives the return values of the
   // function, if there is any
   std::vector<VarNode*> returnVars;
   if(hasReturn)
   {
      THROW_ASSERT(FD->body && FD->body->get_kind() == statement_list_node_K, "");
      const auto SL = GetPointerS<const statement_list_node>(FD->body);
      for(const auto& [idx, BB] : SL->list_of_bloc)
      {
         const auto& stmt_list = BB->CGetStmtList();
         if(stmt_list.size())
         {
            if(stmt_list.back()->get_kind() == return_stmt_K)
            {
               const auto gr = GetPointerS<const return_stmt>(stmt_list.back());
               if(gr->op) // Compiler defined return statements may be without argument
               {
                  returnVars.push_back(CG->addVarNode(gr->op, function_id));
               }
            }
         }
      }
   }
   if(returnVars.empty() && hasReturn)
   {
#ifndef NDEBUG
      if(range_analysis::isValidType(ret_type))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Function should return, but no return statement was found");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function return type not supported");
      }
#endif
      hasReturn = false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string("Function ") + (hasReturn ? "has explicit" : "has no") + " return statement" +
                      (returnVars.size() > 1 ? "s" : ""));

   for(const auto& ie : call_graph.in_edges(f_v))
   {
      const auto& einfo = call_graph.CGetEdgeInfo(ie);
      for(const auto call_id : einfo.direct_call_points)
      {
         const auto call_tn = TM->GetIRNode(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing call " + call_tn->ToString());
         THROW_ASSERT(GetPointer<const node_stmt>(call_tn), "");
         const auto gn = GetPointerS<const node_stmt>(call_tn);
         const auto caller_id = gn->parent->index;
         const std::vector<ir_nodeRef>* args = nullptr;
         ir_nodeConstRef ret_var = nullptr;
         if(call_tn->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointerS<const assign_stmt>(call_tn);
            THROW_ASSERT(GetPointer<const call_node>(ga->op1), "");
            const auto ce = GetPointerS<const call_node>(ga->op1);
            args = &ce->args;
            ret_var = ga->op0;
         }
         else if(call_tn->get_kind() == call_stmt_K)
         {
            const auto* gc = GetPointerS<const call_stmt>(call_tn);
            args = &gc->args;
         }
         else
         {
            THROW_UNREACHABLE("Call statement should be a assign_stmt or a call_stmt");
         }
         THROW_ASSERT(args->size() == parameters.size(),
                      "Function parameters and call arguments size mismatch (" + std::to_string(args->size()) +
                          " != " + std::to_string(parameters.size()) + "): " + call_tn->ToString());

         // Do the inter-procedural construction of CG
         VarNode* to = nullptr;
         VarNode* from = nullptr;

         // Match formal and real parameters
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         for(size_t i = 0; i < parameters.size(); ++i)
         {
            if(parameters[i] == nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Parameter " + STR(i) + " was constant, matching not necessary");
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           args->at(i)->ToString() + " bound to argument " + parameters[i]->ToString());
            // Add real parameter to the CG
            from = CG->addVarNode(args->at(i), caller_id);

            // Connect nodes
            matchers[i]->addSource(from);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

         // Match return values when return type is stored from caller
         if(hasReturn && call_tn->get_kind() != call_stmt_K)
         {
            // Add caller instruction to the CG (it receives the return value)
            to = CG->addVarNode(ret_var, caller_id);
            to->setRange(to->getMaxRange());

            auto phiOp = new PhiOpNode(to, nullptr);
            phiOp->setIntersect(to->getRange());
            for(auto var : returnVars)
            {
               phiOp->addSource(var);
            }
            CG->pushOperation(phiOp);

#ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
            {
               std::string phiString = "Return variable " + STR(phiOp->getSink()->getValue()) + " = PHI<";
               for(size_t i = 0; i < phiOp->getNumSources(); ++i)
               {
                  phiString += STR(phiOp->getSource(i)->getValue()) + ", ";
               }
               phiString.erase(phiString.size() - 2);
               phiString += ">";
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
            }
#endif
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   for(auto m : matchers)
   {
      if(m)
      {
         CG->pushOperation(m);
#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
         {
            std::string phiString = STR(m->getSink()->getValue()) + " = PHI<";
            for(size_t i = 0; i < m->getNumSources(); ++i)
            {
               phiString += STR(m->getSource(i)->getValue()) + ", ";
            }
            phiString.erase(phiString.size() - 2);
            phiString += ">";
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
         }
#endif
      }
   }
}

// ========================================================================== //
// RangeAnalysis
// ========================================================================== //
RangeAnalysis::RangeAnalysis(const application_managerRef AM, const DesignFlowManager& _design_flow_manager,
                             const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, _design_flow_manager, par),
      solverType(st_Cousot),
      computeESSA(true),
      execution_mode(RA_EXEC_NORMAL),
      last_ver_sum(0)
#ifndef NDEBUG
      ,
      graph_debug(DEBUG_LEVEL_NONE),
      iteration(0),
      stop_iteration(std::numeric_limits<decltype(stop_iteration)>::max()),
      stop_transformation(std::numeric_limits<decltype(stop_transformation)>::max())
#endif
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   const auto opts =
       string_to_container<std::vector<std::string>>(parameters->getOption<std::string>(OPT_range_analysis_mode), ",");
   CustomSet<std::string> ra_mode;
   for(const auto& opt : opts)
   {
      if(opt.size())
      {
         ra_mode.insert(opt);
      }
   }
   if(ra_mode.erase("crop"))
   {
      solverType = st_Crop;
   }
   if(ra_mode.erase("noESSA"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: no Extended SSA required");
      computeESSA = false;
   }
#ifndef NDEBUG
   if(ra_mode.erase("ro"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: read-only mode enabled");
      execution_mode = RA_EXEC_READONLY;
   }
#endif
   if(ra_mode.erase("skip"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: skip mode enabled");
      execution_mode = RA_EXEC_SKIP;
   }
#ifndef NDEBUG
   if(ra_mode.erase("debug_op"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: range operations debug");
      OpNode::debug_level = debug_level;
   }
   if(ra_mode.erase("debug_graph"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: graph debug");
      graph_debug = debug_level;
   }

   OPERATION_OPTION(ra_mode, abs);
   OPERATION_OPTION(ra_mode, negate);
   OPERATION_OPTION(ra_mode, sext);
   OPERATION_OPTION(ra_mode, zext);
   OPERATION_OPTION(ra_mode, tnot);
   OPERATION_OPTION(ra_mode, add);
   OPERATION_OPTION(ra_mode, sub);
   OPERATION_OPTION(ra_mode, mul);
   OPERATION_OPTION(ra_mode, sdiv);
   OPERATION_OPTION(ra_mode, udiv);
   OPERATION_OPTION(ra_mode, srem);
   OPERATION_OPTION(ra_mode, urem);
   OPERATION_OPTION(ra_mode, shl);
   OPERATION_OPTION(ra_mode, shr);
   OPERATION_OPTION(ra_mode, and);
   OPERATION_OPTION(ra_mode, or);
   OPERATION_OPTION(ra_mode, xor);
   OPERATION_OPTION(ra_mode, min);
   OPERATION_OPTION(ra_mode, max);
   OPERATION_OPTION(ra_mode, ternary);
   OPERATION_OPTION(ra_mode, load);
   if(ra_mode.size() && ra_mode.begin()->size())
   {
      THROW_ASSERT(ra_mode.size() <= 2, "Too many range analysis options left to parse");
      auto it = ra_mode.begin();
      if(ra_mode.size() == 2)
      {
         auto tr = ++ra_mode.begin();
         if(it->front() == 't')
         {
            it = ++ra_mode.begin();
            tr = ra_mode.begin();
         }
         THROW_ASSERT(tr->front() == 't', "Invalid range analysis option: " + *tr);
         stop_transformation = std::strtoull(tr->data() + sizeof(char), nullptr, 10);
         if(stop_transformation == 0)
         {
            THROW_ERROR("Invalid range analysis option: " + *tr);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Range analysis: only " + STR(stop_transformation) + " transformation" +
                            (stop_transformation > 1 ? "s" : "") + " will run on last iteration");
      }
      if(it->front() == 'i')
      {
         stop_iteration = std::strtoull(it->data() + sizeof(char), nullptr, 10);
      }
      else
      {
         stop_iteration = std::strtoull(it->data(), nullptr, 10);
      }
      if(stop_iteration == 0)
      {
         THROW_ERROR("Invalid range analysis option: " + *it);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Range analysis: only " + STR(stop_iteration) + " iteration" + (stop_iteration > 1 ? "s" : "") +
                         " will run");
   }
#else
   THROW_ASSERT(ra_mode.empty(), "Invalid range analysis mode falgs. (" + *ra_mode.begin() + ")");
#endif
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
RangeAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   if(execution_mode == RA_EXEC_SKIP)
   {
      return relationships;
   }
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BIT_VALUE_OPT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
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

void RangeAnalysis::ComputeRelationships(DesignFlowStepSet& relationships,
                                         const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
      for(const auto f_id : fun_id_to_restart)
      {
         const auto bv_signature = FunctionFrontendFlowStep::ComputeSignature(BIT_VALUE, f_id);
         const auto frontend_bv = design_flow_manager.GetDesignFlowStep(bv_signature);
         THROW_ASSERT(frontend_bv != DesignFlowGraph::null_vertex(), "step is not present");
         const auto bv = design_flow_graph->CGetNodeInfo(frontend_bv)->design_flow_step;
         relationships.insert(bv);
      }
      fun_id_to_restart.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool RangeAnalysis::HasToBeExecuted() const
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      return false;
   }

   const auto& CGM = AppM->CGetCallGraphManager();
   unsigned int curr_ver_sum = 0;
   for(const auto i : CGM.GetReachedBodyFunctions())
   {
      const auto FB = AppM->CGetFunctionBehavior(i);
      curr_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   return curr_ver_sum > last_ver_sum;
}

void RangeAnalysis::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range Analysis step");
   fun_id_to_restart.clear();
}

DesignFlowStep_Status RangeAnalysis::Exec()
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Range analysis no execution mode enabled");
      return DesignFlowStep_Status::SKIPPED;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   // Initialize constraint graph
   ConstraintGraphRef CG;
   switch(solverType)
   {
      case st_Cousot:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using jump-set abstract operators");
         CG.reset(new CousotConstraintGraph(AppM,
#ifndef NDEBUG
                                            debug_level, graph_debug));
#else
                                            DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      case st_Crop:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using standard abstract operators");
         CG.reset(new CropDFSConstraintGraph(AppM,
#ifndef NDEBUG
                                             debug_level, graph_debug));
#else
                                             DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      default:
         THROW_UNREACHABLE("Unknown solver type " + STR(solverType));
         break;
   }

   // Analyse only reached functions
   const auto rb_funcs = AppM->CGetCallGraphManager().GetReachedBodyFunctions();
   for(const auto& f : rb_funcs)
   {
      CG->buildGraph(f, computeESSA);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation...");
   for(const auto f : rb_funcs)
   {
      ParmAndRetValPropagation(f, AppM, CG, debug_level);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation completed");

#ifndef NDEBUG
   CG->findIntervals(parameters, GetName() + "(" + STR(iteration) + ")");
   ++iteration;
#else
   CG->findIntervals();
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#ifndef NDEBUG
   const auto modified = finalize(CG);
   if(stop_iteration != std::numeric_limits<decltype(stop_iteration)>::max())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Iteration " + STR(iteration) + "/" + STR(stop_iteration) + "completed (" +
                         STR(stop_iteration - iteration) + " to go)");
   }
   if(modified)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges updated");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges reached fixed point");
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#else
   return finalize(CG) ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#endif
}

bool RangeAnalysis::finalize(const ConstraintGraphRef& CG)
{
   THROW_ASSERT(CG, "");
   const auto& vars = CG->getVarNodes();
   CustomSet<unsigned int> modifiedFunctionsBit;

#ifndef NDEBUG
   if(execution_mode >= RA_EXEC_READONLY)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [key, node] : vars)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Range " + node->getRange().ToString() + " for " + node->getValue()->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "IR update not applied in read-only mode");
   }
   else
#endif
   {
      const auto TM = AppM->get_ir_manager();

#ifndef NDEBUG
      unsigned long long updated = 0;
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [key, node] : vars)
      {
#ifndef NDEBUG
         if(iteration == stop_iteration && updated >= stop_transformation)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Max required transformations performed. IR update aborted.");
            break;
         }
#endif
         if(const auto ut = updateIR(node, TM, debug_level, AppM))
         {
            if(ut & ut_BitValue)
            {
               const auto funID = node->getFunctionId();
               modifiedFunctionsBit.insert(funID);
#ifndef NDEBUG
               ++updated;
#endif
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Bounds updated for " + STR(updated) + "/" + STR(vars.size()) + " variables");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Modified BitValues " + STR(modifiedFunctionsBit.size()) + " functions:");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto fUT : modifiedFunctionsBit)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     AppM->CGetFunctionBehavior(fUT)->CGetBehavioralHelper()->GetFunctionName());
      fun_id_to_restart.insert(fUT);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   last_ver_sum = 0;
   const auto rbf = AppM->CGetCallGraphManager().GetReachedBodyFunctions();
   for(const auto f : rbf)
   {
      const auto FB = AppM->GetFunctionBehavior(f);
      const auto isInBit = fun_id_to_restart.count(f);
      last_ver_sum += FB->GetBBVersion() + (isInBit ? FB->UpdateBitValueVersion() : FB->GetBitValueVersion());
   }
   return !fun_id_to_restart.empty();
}
