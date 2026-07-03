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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file call_graph.cpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "call_graph_manager.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include <algorithm>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "config_HAVE_ASSERTS.hpp"

/**
 * Visitor to identify the list of called functions
 */
struct CalledFunctionsVisitor : public boost::default_dfs_visitor
{
 private:
   /// True if recursive calls are allowed
   const bool allow_recursive_functions;

   /// The call graph manager
   const CallGraphManager& CGM;

   /// The list of encountered body functions
   CustomSet<unsigned int>& body_functions;

   /// The list of encountered library functions
   CustomSet<unsigned int>& library_functions;

 public:
   /**
    * Constructor
    * @param _allow_recursive_functions tells if recursive functions are allowed
    * @param call_graph_manager is the call graph manager
    * @param _body_functions is where functions with an implementation will be stored
    * @param _library_functions is where functions without an implementation will be stored
    */
   CalledFunctionsVisitor(const bool _allow_recursive_functions, const CallGraphManager& call_graph_manager,
                          CustomSet<unsigned int>& _body_functions, CustomSet<unsigned int>& _library_functions)
       : allow_recursive_functions(_allow_recursive_functions),
         CGM(call_graph_manager),
         body_functions(_body_functions),
         library_functions(_library_functions)
   {
   }

   void back_edge(const CallGraph::edge_descriptor& e, const CallGraph& g)
   {
      if(!allow_recursive_functions)
      {
         const auto& behaviors = g.CGetGraphInfo().behaviors;
         const auto source = g.source(e);
         const auto target = g.target(e);
         THROW_ERROR("Recursive functions not yet supported: " +
                     behaviors.at(CGM.get_function(source))->CGetBehavioralHelper()->GetFunctionName() + "-->" +
                     behaviors.at(CGM.get_function(target))->CGetBehavioralHelper()->GetFunctionName());
      }
   }

   /**
    * Function called when a vertex has been finished
    * @param u is the vertex
    * @param g is the call graph
    */
   void finish_vertex(CallGraph::vertex_descriptor u, const CallGraph& g)
   {
      const auto f_id = g.CGetNodeInfo(u).nodeID;
      if(g.CGetGraphInfo().behaviors.at(f_id)->CGetBehavioralHelper()->has_implementation())
      {
         body_functions.insert(f_id);
      }
      else
      {
         library_functions.insert(f_id);
      }
   }
};

CallGraphManager::CallGraphManager(const bool _allow_recursive_functions, const ir_managerConstRef& _ir_manager,
                                   const ParameterConstRef& _Param)
    : call_graph(*this, STD_SELECTOR | FEEDBACK_SELECTOR),
      ir_manager(_ir_manager),
      allow_recursive_functions(_allow_recursive_functions),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

void CallGraphManager::AddFunction(unsigned int new_function_id, const FunctionBehaviorRef& fun_behavior)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Adding function: " + fun_behavior->CGetBehavioralHelper()->GetFunctionName() +
                      " id: " + STR(new_function_id));
   if(!IsVertex(new_function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new vertex");
      auto v = AddVertex();
      operator[](v).nodeID = new_function_id;
      functionID_vertex_map[new_function_id] = v;
      called_by[new_function_id] = CustomOrderedSet<unsigned int>();
      call_graph.GetGraphInfo().behaviors[new_function_id] = fun_behavior;
      UpdateReachedFunctions();
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---vertex already present");
      THROW_ASSERT(call_graph.GetGraphInfo().behaviors.at(new_function_id) == fun_behavior,
                   "adding a different behavior for " + STR(new_function_id) + " prev: " +
                       STR(call_graph.GetGraphInfo().behaviors.at(new_function_id)) + " new: " + STR(fun_behavior));
   }
}

void CallGraphManager::AddCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                    enum FunctionEdgeInfo::CallType call_type)
{
   THROW_ASSERT(call_id, "");
#if !defined(NDEBUG) || HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " + ir_helper::GetFunctionName(ir_manager->GetIRNode(caller_id));
   const auto called_name = "(" + STR(called_id) + ") " + ir_helper::GetFunctionName(ir_manager->GetIRNode(caller_id));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Adding call with id: " + STR(call_id) + " from " + caller_name + " to " + called_name);
   if(IsCallPoint(caller_id, called_id, call_id, call_type))
   {
      return;
   }
   THROW_ASSERT(!IsCallPoint(caller_id, called_id, call_id, FunctionEdgeInfo::CallType::call_any),
                "call id " + STR(call_id) + " from " + caller_name + " to " + called_name +
                    " was already in the call graph with the same call type");
   THROW_ASSERT(IsVertex(caller_id), "caller function should be already added to the call_graph");
   THROW_ASSERT(IsVertex(called_id), "called function should be already added to the call_graph");
   const auto src = GetVertex(caller_id);
   const auto tgt = GetVertex(called_id);
   if(called_by.at(caller_id).find(called_id) == called_by.at(caller_id).end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---No previous call from " + caller_name + " to " + called_name);
      called_by.at(caller_id).insert(called_id);
      AddEdge(src, tgt, STD_SELECTOR);
      try
      {
         std::list<vertex_descriptor> topological_sort;
         CallGraph(*this, STD_SELECTOR).TopologicalSort(topological_sort);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Sorted call graph");
      }
      catch(std::exception& e)
      {
         RemoveSelector(src, tgt, STD_SELECTOR);
         AddSelector(src, tgt, FEEDBACK_SELECTOR);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Something wrong in call insertion");
      }
   }

   const auto [e, found] = boost::edge(src, tgt, call_graph);
   THROW_ASSERT(found, "call id " + STR(call_id) + " from " + caller_name + " to " + called_name +
                           " was not in the call graph");

   auto& functionEdgeInfo = call_graph.GetEdgeInfo(e);

   switch(call_type)
   {
      case FunctionEdgeInfo::CallType::direct_call:
         functionEdgeInfo.direct_call_points.insert(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added direct call");
         break;
      case FunctionEdgeInfo::CallType::indirect_call:
         functionEdgeInfo.indirect_call_points.insert(call_id);
         addressed_functions.insert(called_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added indirect call");
         break;
      case FunctionEdgeInfo::CallType::function_address:
         functionEdgeInfo.function_addresses.insert(call_id);
         addressed_functions.insert(called_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added taken address");
         break;
      case FunctionEdgeInfo::CallType::call_any:
      default:
         THROW_UNREACHABLE("unexpected call type");
   }
   UpdateReachedFunctions();
}

bool CallGraphManager::IsCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                   enum FunctionEdgeInfo::CallType call_type) const
{
   if(!IsVertex(caller_id) || !IsVertex(called_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Missing vertex");
      return false;
   }

   if(called_by.at(caller_id).find(called_id) == called_by.at(caller_id).end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Missing call");
      return false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Call is present");
   const auto src = GetVertex(caller_id);
   const auto tgt = GetVertex(called_id);

   const auto [e, found] = boost::edge(src, tgt, call_graph);
#if HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " + ir_helper::GetFunctionName(ir_manager->GetIRNode(caller_id));
   const auto called_name = "(" + STR(called_id) + ") " + ir_helper::GetFunctionName(ir_manager->GetIRNode(called_id));
#endif
   THROW_ASSERT(found, "call id " + STR(call_id) + " from " + caller_name + " to " + called_name +
                           " was not in the call graph");

   const auto& functionEdgeInfo = call_graph.CGetEdgeInfo(e);

   bool res = false;
   switch(call_type)
   {
      case FunctionEdgeInfo::CallType::direct_call:
         res = functionEdgeInfo.direct_call_points.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "direct call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::indirect_call:
         res = functionEdgeInfo.indirect_call_points.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "indirect call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::function_address:
         res = functionEdgeInfo.function_addresses.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function_address! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::call_any:
         res = functionEdgeInfo.direct_call_points.count(call_id) ||
               functionEdgeInfo.indirect_call_points.count(call_id) ||
               functionEdgeInfo.function_addresses.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "any call! present? " + STR(res));
         break;
      default:
         THROW_UNREACHABLE("unexpected call type");
   }
   return res;
}

void CallGraphManager::AddFunctionAndCallPoint(const application_managerRef& AppM, unsigned int caller_id,
                                               unsigned int called_id, unsigned int call_id,
                                               enum FunctionEdgeInfo::CallType call_type)
{
   const auto fnode = ir_manager->GetIRNode(called_id);
   if(ir_helper::GetFunctionName(fnode) != BUILTIN_WAIT_CALL)
   {
      if(!IsVertex(called_id))
      {
         const auto helper = std::make_shared<BehavioralHelper>(AppM, called_id, AppM->get_parameter());
         const auto FB = std::make_shared<FunctionBehavior>(AppM, helper, AppM->get_parameter());
         AddFunction(called_id, FB);
      }
      AddCallPoint(caller_id, called_id, call_id, call_type);
   }
}

void CallGraphManager::RemoveCallPoint(const edge_descriptor& e, unsigned int call_id)
{
   const auto called_id = call_graph.CGetNodeInfo(call_graph.target(e)).nodeID;
   const auto called_name = ir_helper::GetFunctionName(ir_manager->GetIRNode(called_id));
   if(called_name == BUILTIN_WAIT_CALL)
   {
      return;
   }
   const auto caller_id = call_graph.CGetNodeInfo(call_graph.source(e)).nodeID;
   auto& edge_info = call_graph.GetEdgeInfo(e);
#if HAVE_ASSERTS
   auto found_calls =
#endif
       edge_info.direct_call_points.erase(call_id);
#if HAVE_ASSERTS
   found_calls +=
#endif
       edge_info.indirect_call_points.erase(call_id);
#if HAVE_ASSERTS
   found_calls +=
#endif
       edge_info.function_addresses.erase(call_id);

#if !defined(NDEBUG) || HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " + ir_helper::GetFunctionName(ir_manager->GetIRNode(caller_id));
#endif
   THROW_ASSERT(found_calls, "call id " + STR(call_id) + " is not a call point in function " + caller_name +
                                 " for function (" + STR(called_id) + ") " + called_name);
   THROW_ASSERT(found_calls == 1, "call id " + STR(call_id) + " is a multiple call point in function " + caller_name +
                                      " for function (" + STR(called_id) + ") " + called_name);
   if(edge_info.indirect_call_points.empty() && edge_info.function_addresses.empty())
   {
      addressed_functions.erase(called_id);
      if(edge_info.direct_call_points.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Removed function call edge: " + caller_name + " -> (" + STR(called_id) + ") " + called_name);
         boost::remove_edge(e, *this);
         called_by.at(caller_id).erase(called_id);
         // const auto called_v = target(e);
         // if(call_graph.in_degree(called_v) == 0)
         // {
         //    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
         //                   "Removed dangling function vertex: (" + STR(called_id) + ") " + called_name);
         //    RemoveVertex(called_v);
         // }
         return UpdateReachedFunctions();
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "There are still " + STR(edge_info.direct_call_points.size()) + " direct calls, " +
                      STR(edge_info.indirect_call_points.size()) + " indirect calls, and " +
                      STR(edge_info.function_addresses.size()) + " places where the address is taken");
}

void CallGraphManager::RemoveCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id)
{
   const auto called_name = ir_helper::GetFunctionName(ir_manager->GetIRNode(called_id));
   if(called_name == BUILTIN_WAIT_CALL)
   {
      return;
   }
   const auto caller_vertex = GetVertex(caller_id);
   const auto called_vertex = GetVertex(called_id);
   const auto [e, found] = boost::edge(caller_vertex, called_vertex, call_graph);
   THROW_ASSERT(found, "call id " + STR(call_id) + " is not a call point in function (" + STR(caller_id) + ") " +
                           ir_helper::GetFunctionName(ir_manager->GetIRNode(caller_id)) + " for function (" +
                           STR(called_id) + ") " + called_name);
   RemoveCallPoint(e, call_id);
}

void CallGraphManager::ReplaceCallPoint(const edge_descriptor& e, unsigned int old_call_id, unsigned int new_call_id)
{
   THROW_ASSERT(old_call_id != new_call_id, "old call point is replaced with itself");
   const auto caller_id = call_graph.CGetNodeInfo(call_graph.source(e)).nodeID;
   const auto called_id = call_graph.CGetNodeInfo(call_graph.target(e)).nodeID;

   auto old_call_type = FunctionEdgeInfo::CallType::direct_call;
   const auto& edge_info = call_graph.CGetEdgeInfo(e);
   const auto& direct_calls = edge_info.direct_call_points;
   const auto& indirect_calls = edge_info.indirect_call_points;
   const auto& function_addresses = edge_info.function_addresses;
   const auto dir_it = direct_calls.find(old_call_id);
   if(dir_it != direct_calls.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::direct_call;
   }
   const auto indir_it = indirect_calls.find(old_call_id);
   if(indir_it != indirect_calls.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::indirect_call;
   }
   const auto addr_it = function_addresses.find(old_call_id);
   if(addr_it != function_addresses.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::function_address;
   }
   // add goes before remove because it avoids clearing the edge
   AddCallPoint(caller_id, called_id, new_call_id, old_call_type);
   RemoveCallPoint(e, old_call_id);
}

bool CallGraphManager::ExistsAddressedFunction() const
{
   for(const auto i : addressed_functions)
   {
      if(reached_body_functions.find(i) != reached_body_functions.end())
      {
         return true;
      }
   }
   return false;
}

CustomSet<unsigned int> CallGraphManager::GetAddressedFunctions() const
{
   CustomSet<unsigned int> reachable_addressed_fun_ids;
   std::set_intersection(reached_body_functions.cbegin(), reached_body_functions.cend(), addressed_functions.cbegin(),
                         addressed_functions.cend(),
                         std::inserter(reachable_addressed_fun_ids, reachable_addressed_fun_ids.begin()));
   return reachable_addressed_fun_ids;
}

CallGraph CallGraphManager::CGetAcyclicCallGraph() const
{
   return CallGraph(*this, STD_SELECTOR);
}

const CallGraph& CallGraphManager::GetCallGraph() const
{
   return call_graph;
}

CallGraph CallGraphManager::CGetCallSubGraph(const CustomUnorderedSet<vertex_descriptor>& vertices) const
{
   return CallGraph(*this, STD_SELECTOR | FEEDBACK_SELECTOR, vertices);
}

CallGraphManager::vertex_descriptor CallGraphManager::GetVertex(unsigned int index) const
{
   THROW_ASSERT(functionID_vertex_map.find(index) != functionID_vertex_map.end(),
                "this vertex does not exist " + STR(index));
   return functionID_vertex_map.at(index);
}

bool CallGraphManager::IsVertex(unsigned int functionID) const
{
   return functionID_vertex_map.find(functionID) != functionID_vertex_map.end();
}

unsigned int CallGraphManager::get_function(vertex_descriptor node) const
{
   return call_graph.CGetNodeInfo(node).nodeID;
}

CustomSet<unsigned int> CallGraphManager::get_called_by(unsigned int index) const
{
   const auto it = called_by.find(index);
   if(it != called_by.end())
   {
      return it->second;
   }
   return CustomSet<unsigned int>();
}

CustomSet<unsigned int> CallGraphManager::get_called_by(const OpGraph& cfg, vertex_descriptor caller) const
{
   return cfg.CGetNodeInfo(caller).called;
}

void CallGraphManager::UpdateReachedFunctions()
{
   reached_body_functions.clear();
   reached_library_functions.clear();
   CalledFunctionsVisitor vis(allow_recursive_functions, *this, reached_body_functions, reached_library_functions);
   for(const auto root_id : root_functions)
   {
      if(IsVertex(root_id))
      {
         std::vector<boost::default_color_type> color_vec(call_graph.num_vertices());
         const auto top_vertex = GetVertex(root_id);
         boost::depth_first_visit(call_graph, top_vertex, vis,
                                  boost::make_iterator_property_map(color_vec.begin(),
                                                                    boost::get(boost::vertex_index_t(), call_graph),
                                                                    boost::white_color),
                                  [&](vertex_descriptor u, const CallGraph& g) {
                                     const auto u_id = g.CGetNodeInfo(u).nodeID;
                                     return u_id != root_id && root_functions.count(u_id);
                                  });
      }
   }
}

void CallGraphManager::SetRootFunctions(const CustomSet<unsigned int>& _root_functions)
{
   root_functions = _root_functions;
}

const CustomSet<unsigned int>& CallGraphManager::GetRootFunctions() const
{
   THROW_ASSERT(call_graph.num_vertices() == 0 || root_functions.size(), "Root functions have not yet been computed");
   return root_functions;
}

const CustomOrderedSet<unsigned int>& CallGraphManager::GetReachedBodyFunctions() const
{
   return reached_body_functions;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetReachedFunctionsFrom(unsigned int from, bool with_body) const
{
   CustomOrderedSet<unsigned int> dummy;
   CustomOrderedSet<unsigned int> f_list;

   const auto top_vertex = GetVertex(from);
   CalledFunctionsVisitor vis(allow_recursive_functions, *this, f_list, with_body ? dummy : f_list);
   std::vector<boost::default_color_type> color_vec(call_graph.num_vertices());
   boost::depth_first_visit(call_graph, top_vertex, vis,
                            boost::make_iterator_property_map(
                                color_vec.begin(), boost::get(boost::vertex_index_t(), call_graph), boost::white_color),
                            [&](vertex_descriptor u, const CallGraph& g) {
                               const auto u_id = g.CGetNodeInfo(u).nodeID;
                               return u_id != from && root_functions.count(u_id);
                            });
   return f_list;
}

unsigned int CallGraphManager::GetRootFunction(unsigned int fid) const
{
   if(root_functions.count(fid))
   {
      return fid;
   }

   unsigned int parent_fid = 0;
   const auto top_vertex = GetVertex(fid);
   for(auto root_fid : root_functions)
   {
      CustomOrderedSet<unsigned int> f_list;
      CalledFunctionsVisitor vis(allow_recursive_functions, *this, f_list, f_list);
      std::vector<boost::default_color_type> color_vec(call_graph.num_vertices());
      boost::depth_first_visit(
          call_graph, top_vertex, vis,
          boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), call_graph),
                                            boost::white_color),
          [&](vertex_descriptor u, const CallGraph& g) { return root_functions.count(g.CGetNodeInfo(u).nodeID); });
      if(f_list.count(fid))
      {
         THROW_ASSERT(parent_fid == 0, "Expected single parent root functions.");
#if HAVE_ASSERTS
         parent_fid = root_fid;
#else
         return root_fid;
#endif
      }
   }
   return parent_fid;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetReachedLibraryFunctions() const
{
   return reached_library_functions;
}

void CallGraphManager::SetOMPThreadsCount(unsigned int fun_id, unsigned nproc)
{
   omp_lambda_functions[fun_id] = nproc;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetOMPLambdaFunctions() const
{
   CustomOrderedSet<unsigned int> reachable_addressed_fun_ids;
   CustomOrderedSet<unsigned int> omp_lambda_functions_ids;
   for(auto funid_pair : omp_lambda_functions)
   {
      omp_lambda_functions_ids.insert(funid_pair.first);
   }
   std::set_intersection(reached_body_functions.cbegin(), reached_body_functions.cend(),
                         omp_lambda_functions_ids.cbegin(), omp_lambda_functions_ids.cend(),
                         std::inserter(reachable_addressed_fun_ids, reachable_addressed_fun_ids.begin()));
   return reachable_addressed_fun_ids;
}

unsigned CallGraphManager::GetOMPThreadsCount(unsigned int fun_id) const
{
   THROW_ASSERT(IsOMPLambdaFunction(fun_id), "unexpected condition");
   return omp_lambda_functions.at(fun_id);
}

bool CallGraphManager::IsOMPLambdaFunction(unsigned int fun_id) const
{
   return omp_lambda_functions.count(fun_id);
}

static void call_graph_computation_recursive(CustomUnorderedSet<unsigned int>& AV, const application_managerRef& AppM,
                                             unsigned int current, const ir_nodeRef& tn, unsigned int node_stmt,
                                             enum FunctionEdgeInfo::CallType call_type, int DL)
{
   if(tn->get_kind() != function_val_node_K)
   {
      if(!AV.insert(tn->index).second)
      {
         return;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL,
                  "-->Recursive analysis of " + STR(tn->index) + " of type " + tn->get_kind_text() + "(statement is " +
                      tn->ToString() + ")");

   switch(tn->get_kind())
   {
      case function_val_node_K:
      {
         const auto* fd = GetPointerS<const function_val_node>(tn);
         /// check for nested function
         if(fd->parent && fd->parent->get_kind() == function_val_node_K)
         {
            THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(tn->index));
            THROW_ERROR("Nested functions not yet supported " + STR(tn->index));
         }
         AppM->GetCallGraphManager().AddFunctionAndCallPoint(AppM, current, tn->index, node_stmt, call_type);
         if(!AV.insert(tn->index).second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "<--");
            return;
         }
         CallGraphManager::expandCallGraphFromFunction(AV, AppM, tn->index, DL);
         break;
      }
      case return_stmt_K:
      {
         auto* re = GetPointerS<return_stmt>(tn);
         if(re->op)
         {
            call_graph_computation_recursive(AV, AppM, current, re->op, node_stmt, call_type, DL);
         }
         break;
      }
      case assign_stmt_K:
      {
         auto* me = GetPointerS<assign_stmt>(tn);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzing left part");
         call_graph_computation_recursive(AV, AppM, current, me->op0, node_stmt, call_type, DL);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzed left part - Analyzing right part");
         call_graph_computation_recursive(AV, AppM, current, me->op1, node_stmt, call_type, DL);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzed right part");
         if(me->predicate)
         {
            call_graph_computation_recursive(AV, AppM, current, me->predicate, node_stmt, call_type, DL);
         }
         break;
      }
      case nop_stmt_K:
      {
         break;
      }
      case call_node_K:
      {
         auto* ce = GetPointerS<call_node>(tn);
         ir_nodeRef fun_node = ce->fn;
         if(fun_node->get_kind() == addr_node_K)
         {
            auto* ue = GetPointerS<unary_node>(fun_node);
            fun_node = ue->op;
         }
         call_graph_computation_recursive(AV, AppM, current, fun_node, node_stmt,
                                          FunctionEdgeInfo::CallType::direct_call, DL);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(AV, AppM, current, arg, node_stmt, call_type, DL);
         }
         break;
      }
      case call_stmt_K:
      {
         auto* ce = GetPointerS<call_stmt>(tn);
         ir_nodeRef fun_node = ce->fn;
         if(fun_node->get_kind() == addr_node_K)
         {
            auto* ue = GetPointerS<unary_node>(fun_node);
            fun_node = ue->op;
         }
         call_graph_computation_recursive(AV, AppM, current, fun_node, node_stmt,
                                          FunctionEdgeInfo::CallType::direct_call, DL);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(AV, AppM, current, arg, node_stmt, call_type, DL);
         }
         break;
      }
      case select_node_K:
      {
         auto* ce = GetPointerS<select_node>(tn);
         call_graph_computation_recursive(AV, AppM, current, ce->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AppM, current, ce->op1, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AppM, current, ce->op2, node_stmt, call_type, DL);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_NODES:
      {
         auto* ue = GetPointerS<unary_node>(tn);
         call_graph_computation_recursive(AV, AppM, current, ue->op, node_stmt, call_type, DL);
         break;
      }
      case CASE_BINARY_NODES:
      {
         auto* be = GetPointerS<binary_node>(tn);
         call_graph_computation_recursive(AV, AppM, current, be->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AppM, current, be->op1, node_stmt, call_type, DL);
         break;
      }
      case multi_way_if_stmt_K:
      {
         auto* gmwi = GetPointerS<multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               call_graph_computation_recursive(AV, AppM, current, cond.first, node_stmt, call_type, DL);
            }
         }
         break;
      }
      /*ternary expressions*/
      case shufflevector_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case insertvalue_node_K:
      case insertelement_node_K:
      case concat_bit_node_K:
      {
         auto* te = GetPointerS<ternary_node>(tn);
         call_graph_computation_recursive(AV, AppM, current, te->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AppM, current, te->op1, node_stmt, call_type, DL);
         if(te->op2)
         {
            call_graph_computation_recursive(AV, AppM, current, te->op2, node_stmt, call_type, DL);
         }
         break;
      }
      case lut_node_K:
      {
         auto* le = GetPointerS<lut_node>(tn);
         call_graph_computation_recursive(AV, AppM, current, le->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AppM, current, le->op1, node_stmt, call_type, DL);
         if(le->op2)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op2, node_stmt, call_type, DL);
         }
         if(le->op3)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op3, node_stmt, call_type, DL);
         }
         if(le->op4)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op4, node_stmt, call_type, DL);
         }
         if(le->op5)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op5, node_stmt, call_type, DL);
         }
         if(le->op6)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op6, node_stmt, call_type, DL);
         }
         if(le->op7)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op7, node_stmt, call_type, DL);
         }
         if(le->op8)
         {
            call_graph_computation_recursive(AV, AppM, current, le->op8, node_stmt, call_type, DL);
         }
         break;
      }
      case constructor_node_K:
      {
         auto* c = GetPointerS<constructor_node>(tn);
         for(const auto& i : c->list_of_idx_valu)
         {
            call_graph_computation_recursive(AV, AppM, current, i.second, node_stmt, call_type, DL);
         }
         break;
      }
      case variable_val_node_K:
      {
         /// var decl performs an assignment when init is not null
         auto* vd = GetPointerS<variable_val_node>(tn);
         if(vd->init)
         {
            call_graph_computation_recursive(AV, AppM, current, vd->init, node_stmt, call_type, DL);
         }
      }
      case argument_val_node_K:
      case ssa_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      case phi_stmt_K:
      {
         break;
      }
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR(std::string("Node not supported (") + STR(tn->index) + std::string("): ") + tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "<--Completed the recursive analysis of node " + STR(tn->index));
}

void CallGraphManager::expandCallGraphFromFunction(CustomUnorderedSet<unsigned int>& AV,
                                                   const application_managerRef& AppM, unsigned int f_id, int DL)
{
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(f_id);
   if(ir_helper::IsFunctionImplemented(fnode))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyze body of " + ir_helper::GetFunctionName(fnode));
      const auto* fd = GetPointerS<const function_val_node>(fnode);
      const auto* sl = GetPointerS<const statement_list_node>(fd->body);
      if(sl->list_of_bloc.empty())
      {
         THROW_ERROR("We can only work on CFG provided by Clang/LLVM");
      }
      else
      {
         for(const auto& [bbi, bb] : sl->list_of_bloc)
         {
            for(const auto& stmt : bb->CGetStmtList())
            {
               call_graph_computation_recursive(AV, AppM, f_id, stmt, stmt->index,
                                                FunctionEdgeInfo::CallType::function_address, DL);
            }
         }
      }
   }
}

void CallGraphManager::addCallPointAndExpand(CustomUnorderedSet<unsigned int>& AV, const application_managerRef& AppM,
                                             unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                             enum FunctionEdgeInfo::CallType call_type, int DL)
{
   auto& CGM = AppM->GetCallGraphManager();
   if(!CGM.IsVertex(called_id))
   {
      CGM.AddFunctionAndCallPoint(AppM, caller_id, called_id, call_id, call_type);
      expandCallGraphFromFunction(AV, AppM, called_id, DL);
   }
   else
   {
      CGM.AddCallPoint(caller_id, called_id, call_id, call_type);
   }
}
