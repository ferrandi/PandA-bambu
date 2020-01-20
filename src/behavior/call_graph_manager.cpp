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
 * @file call_graph.cpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "call_graph_manager.hpp"
#include "config_HAVE_ASSERTS.hpp" // for HAVE_ASSERTS

#include "Parameter.hpp"           // for Parameter, OPT_top_...
#include "behavioral_helper.hpp"   // for BehavioralHelper
#include "call_graph.hpp"          // for CallGraph, Function...
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_VERY_PE...
#include "exceptions.hpp"          // for THROW_ASSERT, THROW...
#include "function_behavior.hpp"   // for BehavioralHelperCon...
#include "graph.hpp"               // for SelectEdge, vertex
#include "loops.hpp"               // for FunctionBehaviorRef
#include "op_graph.hpp"            // for OpGraphConstRef
#include "string_manipulation.hpp" // for STR GET_CLASS
#include "tree_manager.hpp"        // for tree_manager, Param...
#include <algorithm>               // for set_intersection
#include <boost/tuple/tuple.hpp>   // for tie
#include <iterator>                // for insert_iterator
#include <list>                    // for list
#include <string>                  // for operator+, char_traits
#include <utility>                 // for pair
#include <vector>                  // for vector

/**
 * Helper macro adding a call point to an edge of the call graph
 * @param g is the graph
 * @param e is the edge
 * @param newstmt is the call point to be added
 */
#define ADD_CALL_POINT(g, e, newstmt) get_edge_info<function_graph_edge_info>(e, *(g))->call_points.insert(newstmt)

/**
 * @name function graph selector
 */
//@{
/// Data line selector
#define STD_SELECTOR 1 << 0
/// Clock line selector
#define FEEDBACK_SELECTOR 1 << 1
//@}

CallGraphManager::CallGraphManager(const FunctionExpanderConstRef _function_expander, const bool _single_root_function, const bool _allow_recursive_functions, const tree_managerConstRef _tree_manager, const ParameterConstRef _Param)
    : call_graphs_collection(new CallGraphsCollection(CallGraphInfoRef(new CallGraphInfo()), _Param)),
      call_graph(new CallGraph(call_graphs_collection, STD_SELECTOR | FEEDBACK_SELECTOR)),
      tree_manager(_tree_manager),
      single_root_function(_single_root_function),
      allow_recursive_functions(_allow_recursive_functions),
      Param(_Param),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this))),
      function_expander(_function_expander)
{
}

CallGraphManager::~CallGraphManager() = default;

void CallGraphManager::AddFunction(unsigned int new_function_id, const FunctionBehaviorRef fun_behavior)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding function: " + fun_behavior->CGetBehavioralHelper()->get_function_name() + " id: " + STR(new_function_id));
   if(not IsVertex(new_function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new vertex");
      vertex v = call_graphs_collection->AddVertex(NodeInfoRef(new FunctionInfo()));
      GET_NODE_INFO(call_graphs_collection.get(), FunctionInfo, v)->nodeID = new_function_id;
      functionID_vertex_map[new_function_id] = v;
      called_by[new_function_id] = CustomOrderedSet<unsigned int>();
      call_graph->GetCallGraphInfo()->behaviors[new_function_id] = fun_behavior;
      ComputeRootAndReachedFunctions();
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---vertex already present");
      THROW_ASSERT(call_graph->GetCallGraphInfo()->behaviors.at(new_function_id) == fun_behavior,
                   "adding a different behavior for " + STR(new_function_id) + "prev: " + STR(call_graph->GetCallGraphInfo()->behaviors.at(new_function_id)) + "new: " + STR(fun_behavior));
   }
}

void CallGraphManager::AddCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, enum FunctionEdgeInfo::CallType call_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call with id: " + STR(call_id) + " from fun id: " + STR(caller_id) + " to fun id: " + STR(called_id));
   if(IsCallPoint(caller_id, called_id, call_id, call_type))
      return;
   THROW_ASSERT(not IsCallPoint(caller_id, called_id, call_id, FunctionEdgeInfo::CallType::call_any),
                "call id " + STR(call_id) + " from function " + STR(caller_id) + " function " + STR(called_id) + " was already in the call graph with the same call type");
   THROW_ASSERT(IsVertex(caller_id), "caller function should be already added to the call_graph");
   THROW_ASSERT(IsVertex(called_id), "called function should be already added to the call_graph");
   const vertex src = GetVertex(caller_id);
   const vertex tgt = GetVertex(called_id);
   if(called_by.at(caller_id).find(called_id) == called_by.at(caller_id).end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---No previous call from fun id: " + STR(caller_id) + " to fun id: " + STR(called_id));
      called_by.at(caller_id).insert(called_id);
      call_graphs_collection->AddEdge(src, tgt, STD_SELECTOR);
      try
      {
         std::list<vertex> topological_sort;
         CallGraph(call_graphs_collection, STD_SELECTOR).TopologicalSort(topological_sort);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Sorted call graph");
      }
      catch(std::exception& e)
      {
         call_graphs_collection->RemoveSelector(src, tgt, STD_SELECTOR);
         call_graphs_collection->AddSelector(src, tgt, FEEDBACK_SELECTOR);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Something wrong in call insertion");
      }
   }

   EdgeDescriptor e;
   bool found;
   boost::tie(e, found) = boost::edge(src, tgt, *CGetCallGraph());
   THROW_ASSERT(found, "call id " + STR(call_id) + " from function " + STR(caller_id) + " function " + STR(called_id) + " was not in the call graph");

   auto* functionEdgeInfo = get_edge_info<FunctionEdgeInfo, CallGraph>(e, *call_graph);
   THROW_ASSERT(call_id, "");

   switch(call_type)
   {
      case FunctionEdgeInfo::CallType::direct_call:
         functionEdgeInfo->direct_call_points.insert(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added direct call");
         break;
      case FunctionEdgeInfo::CallType::indirect_call:
         functionEdgeInfo->indirect_call_points.insert(call_id);
         addressed_functions.insert(called_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added indirect call");
         break;
      case FunctionEdgeInfo::CallType::function_address:
         functionEdgeInfo->function_addresses.insert(call_id);
         addressed_functions.insert(called_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "added taken address");
         break;
      case FunctionEdgeInfo::CallType::call_any:
      default:
         THROW_UNREACHABLE("unexpected call type");
   }
   ComputeRootAndReachedFunctions();
}

bool CallGraphManager::IsCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, enum FunctionEdgeInfo::CallType call_type) const
{
   if(not IsVertex(caller_id) or not IsVertex(called_id))
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
   const vertex src = GetVertex(caller_id);
   const vertex tgt = GetVertex(called_id);

   EdgeDescriptor e;
   bool found;
   boost::tie(e, found) = boost::edge(src, tgt, *CGetCallGraph());
   THROW_ASSERT(found, "call id " + STR(call_id) + " from function " + STR(caller_id) + " function " + STR(called_id) + " was not in the call graph");

   auto* functionEdgeInfo = get_edge_info<FunctionEdgeInfo, CallGraph>(e, *call_graph);

   bool res = false;
   switch(call_type)
   {
      case FunctionEdgeInfo::CallType::direct_call:
         res = functionEdgeInfo->direct_call_points.find(call_id) != functionEdgeInfo->direct_call_points.end();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "direct call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::indirect_call:
         res = functionEdgeInfo->indirect_call_points.find(call_id) != functionEdgeInfo->indirect_call_points.end();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "indirect call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::function_address:
         res = functionEdgeInfo->function_addresses.find(call_id) != functionEdgeInfo->function_addresses.end();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function_address! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::call_any:
         res = functionEdgeInfo->direct_call_points.find(call_id) != functionEdgeInfo->direct_call_points.end() or functionEdgeInfo->indirect_call_points.find(call_id) != functionEdgeInfo->indirect_call_points.end() or
               functionEdgeInfo->function_addresses.find(call_id) != functionEdgeInfo->function_addresses.end();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "any call! present? " + STR(res));
         break;
      default:
         THROW_UNREACHABLE("unexpected call type");
   }
   return res;
}

void CallGraphManager::AddFunctionAndCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, const FunctionBehaviorRef called_function_behavior, enum FunctionEdgeInfo::CallType call_type)
{
   AddFunction(called_id, called_function_behavior);
   AddCallPoint(caller_id, called_id, call_id, call_type);
}

void CallGraphManager::RemoveCallPoint(EdgeDescriptor e, const unsigned int callid)
{
   const unsigned int caller_id = Cget_node_info<FunctionInfo, CallGraph>(boost::source(e, *call_graph), *call_graph)->nodeID;
   const unsigned int called_id = Cget_node_info<FunctionInfo, CallGraph>(boost::target(e, *call_graph), *call_graph)->nodeID;

   auto* edge_info = get_edge_info<FunctionEdgeInfo, CallGraph>(e, *call_graph);
   auto& direct_calls = edge_info->direct_call_points;
   auto& indirect_calls = edge_info->indirect_call_points;
   auto& function_addresses = edge_info->function_addresses;

#if HAVE_ASSERTS
   int found_calls = 0;
#endif
   const auto dir_it = direct_calls.find(callid);
   if(dir_it != direct_calls.end())
   {
      direct_calls.erase(callid);
#if HAVE_ASSERTS
      found_calls++;
#endif
   }
   const auto indir_it = indirect_calls.find(callid);
   if(indir_it != indirect_calls.end())
   {
      indirect_calls.erase(callid);
#if HAVE_ASSERTS
      found_calls++;
#endif
   }
   const auto addr_it = function_addresses.find(callid);
   if(addr_it != function_addresses.end())
   {
      function_addresses.erase(callid);
#if HAVE_ASSERTS
      found_calls++;
#endif
   }

   THROW_ASSERT(found_calls, "call id " + STR(callid) + " is not a call point in function " + STR(caller_id) + " for function " + STR(called_id));
   THROW_ASSERT(found_calls == 1, "call id " + STR(callid) + " is a multiple call point in function " + STR(caller_id) + " for function " + STR(called_id));

   if(direct_calls.empty() and indirect_calls.empty() and function_addresses.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed function call edge: " + STR(caller_id) + " -> " + STR(called_id));
      boost::remove_edge(boost::source(e, *call_graphs_collection), boost::target(e, *call_graphs_collection), *call_graphs_collection);
      called_by.at(caller_id).erase(called_id);
      call_graphs_collection->RemoveSelector(e);
      ComputeRootAndReachedFunctions();
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are still " + STR(direct_calls.size()) + " direct calls, " + STR(indirect_calls.size()) + " indirect calls, and " + STR(function_addresses.size()) + " places where the address is taken");
   }
   if(indirect_calls.empty() and function_addresses.empty())
      addressed_functions.erase(called_id);
}

void CallGraphManager::RemoveCallPoint(const unsigned int caller_id, const unsigned int called_id, const unsigned int call_id)
{
   const auto caller_vertex = GetVertex(caller_id);
   const auto called_vertex = GetVertex(called_id);
   EdgeDescriptor e;
   bool found;
   boost::tie(e, found) = boost::edge(caller_vertex, called_vertex, *CGetCallGraph());
   THROW_ASSERT(found, "call id " + STR(call_id) + " is not a call point in function " + STR(caller_id) + " for function " + STR(called_id));
   RemoveCallPoint(e, call_id);
}

void CallGraphManager::ReplaceCallPoint(const EdgeDescriptor e, const unsigned int orig, const unsigned int repl)
{
   THROW_ASSERT(orig != repl, "old call point is replaced with itself");
   const unsigned int caller_id = Cget_node_info<FunctionInfo, CallGraph>(boost::source(e, *call_graph), *call_graph)->nodeID;
   const unsigned int called_id = Cget_node_info<FunctionInfo, CallGraph>(boost::target(e, *call_graph), *call_graph)->nodeID;

   enum FunctionEdgeInfo::CallType old_call_type = FunctionEdgeInfo::CallType::direct_call;
   const auto edge_info = get_edge_info<FunctionEdgeInfo, CallGraph>(e, *call_graph);
   const auto& direct_calls = edge_info->direct_call_points;
   const auto& indirect_calls = edge_info->indirect_call_points;
   const auto& function_addresses = edge_info->function_addresses;
   const auto dir_it = direct_calls.find(orig);
   if(dir_it != direct_calls.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::direct_call;
   }
   const auto indir_it = indirect_calls.find(orig);
   if(indir_it != indirect_calls.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::indirect_call;
   }
   const auto addr_it = function_addresses.find(orig);
   if(addr_it != function_addresses.end())
   {
      old_call_type = FunctionEdgeInfo::CallType::function_address;
   }
   // add goes before remove because it avoids clearing the edge
   AddCallPoint(caller_id, called_id, repl, old_call_type);
   RemoveCallPoint(e, orig);
}

bool CallGraphManager::ExistsAddressedFunction() const
{
   for(const auto i : addressed_functions)
      if(reached_body_functions.find(i) != reached_body_functions.end())
         return true;
   return false;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetAddressedFunctions() const
{
   CustomOrderedSet<unsigned int> reachable_addressed_fun_ids;
   std::set_intersection(reached_body_functions.cbegin(), reached_body_functions.cend(), addressed_functions.cbegin(), addressed_functions.cend(), std::inserter(reachable_addressed_fun_ids, reachable_addressed_fun_ids.begin()));
   return reachable_addressed_fun_ids;
}

const CallGraphConstRef CallGraphManager::CGetAcyclicCallGraph() const
{
   return CallGraphRef(new CallGraph(call_graphs_collection, STD_SELECTOR));
}

const CallGraphConstRef CallGraphManager::CGetCallGraph() const
{
   return call_graph;
}

const CallGraphConstRef CallGraphManager::CGetCallSubGraph(const CustomUnorderedSet<vertex>& vertices) const
{
   return CallGraphConstRef(new CallGraph(call_graphs_collection, STD_SELECTOR | FEEDBACK_SELECTOR, vertices));
}

vertex CallGraphManager::GetVertex(const unsigned int index) const
{
   THROW_ASSERT(functionID_vertex_map.find(index) != functionID_vertex_map.end(), "this vertex does not exist " + STR(index));
   return functionID_vertex_map.at(index);
}

bool CallGraphManager::IsVertex(unsigned int functionID) const
{
   return functionID_vertex_map.find(functionID) != functionID_vertex_map.end();
}

unsigned int CallGraphManager::get_function(vertex node) const
{
   const auto end = functionID_vertex_map.cend();
   for(auto i = functionID_vertex_map.cbegin(); i != end; i++)
      if(i->second == node)
         return i->first;
   return 0;
}

const CustomOrderedSet<unsigned int> CallGraphManager::get_called_by(unsigned int index) const
{
   if(called_by.find(index) != called_by.end())
      return called_by.at(index);
   else
      return CustomOrderedSet<unsigned int>();
}

const CustomUnorderedSet<unsigned int> CallGraphManager::get_called_by(const OpGraphConstRef cfg, const vertex& caller) const
{
   return cfg->CGetOpNodeInfo(caller)->called;
}

void CallGraphManager::ComputeRootAndReachedFunctions()
{
   root_functions.clear();
   const unsigned int main_index = tree_manager->function_index("main");
   /// If top function option has been passed
   if(Param->isOption(OPT_top_functions_names))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Top functions passed by user");
      const auto top_functions_names = Param->getOption<const std::list<std::string>>(OPT_top_functions_names);
      for(const auto& top_function_name : top_functions_names)
      {
         const unsigned int top_function_index = tree_manager->function_index(top_function_name);
         if(top_function_index == 0)
         {
            THROW_ERROR("Function " + top_function_name + " not found");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Root function " + STR(top_function_index));
            root_functions.insert(top_function_index);
         }
      }
   }
   /// If not -c option has been passed we assume that whole program has been passed, so the main must be present
   else if(not Param->getOption<bool>(OPT_gcc_c))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Expected main");
      /// Main not found
      if(not main_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main not found");
         if(tree_manager->get_next_available_tree_node_id() != 1)
         {
            THROW_ERROR("No main function found, but -c option not passed");
         }
         /// Main not found, but call graph not yet built
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---But call graph not built");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main found");
         root_functions.insert(main_index);
      }
   }
   /// If there is the main, we return it
   else if(main_index)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main was not expected but is present: " + STR(main_index));
      root_functions.insert(main_index);
   }
   /// Return all the functions not called by any other function
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main was not expected and is not present");
      VertexIterator function, function_end;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Looking for root functions");
      for(boost::tie(function, function_end) = boost::vertices(*call_graph); function != function_end; function++)
      {
         unsigned int fun_id = get_function(*function);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function" + STR(fun_id));
         THROW_ASSERT(fun_id > 0, "expected a meaningful function id");
         const std::map<unsigned int, FunctionBehaviorRef>& behaviors = call_graph->CGetCallGraphInfo()->behaviors;
         if(boost::in_degree(*function, *call_graph) == 0 and behaviors.find(fun_id) != behaviors.end() and behaviors.find(fun_id)->second->CGetBehavioralHelper()->has_implementation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Has body");
            if(single_root_function)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single root function");
               if(root_functions.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---No root functions found yet");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added root_function" + STR(fun_id));
                  root_functions.insert(fun_id);
               }
               else if(fun_id > *(root_functions.begin()))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New root_function" + STR(fun_id) + " with higher id than previous " + STR(*root_functions.begin()));
                  root_functions.clear();
                  root_functions.insert(fun_id);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multiple root functions");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added root_function" + STR(fun_id));
               root_functions.insert(fun_id);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed function" + STR(fun_id));
      }
   }
   reached_body_functions.clear();
   reached_library_functions.clear();
   CalledFunctionsVisitor vis(allow_recursive_functions, this, reached_body_functions, reached_library_functions);
   std::vector<boost::default_color_type> color_vec(boost::num_vertices(*call_graph));
   for(auto root_fun_id : root_functions)
   {
      if(IsVertex(root_fun_id))
      {
         const vertex top_vertex = GetVertex(root_fun_id);
         boost::depth_first_visit(*call_graph, top_vertex, vis, boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), *call_graph), boost::white_color));
      }
   }
}

const CustomOrderedSet<unsigned int> CallGraphManager::GetRootFunctions() const
{
   THROW_ASSERT(boost::num_vertices(*call_graph) == 0 or root_functions.size(), "Root functions have not yet been computed");
   return root_functions;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetReachedBodyFunctions() const
{
   return reached_body_functions;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetReachedBodyFunctionsFrom(unsigned int from) const
{
   CustomOrderedSet<unsigned int> dummy;
   CustomOrderedSet<unsigned int> f_list;

   CalledFunctionsVisitor vis(allow_recursive_functions, this, f_list, dummy);
   std::vector<boost::default_color_type> color_vec(boost::num_vertices(*call_graph));
   const vertex top_vertex = GetVertex(from);
   boost::depth_first_visit(*call_graph, top_vertex, vis, boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), *call_graph), boost::white_color));
   return f_list;
}

CustomOrderedSet<unsigned int> CallGraphManager::GetReachedLibraryFunctions() const
{
   return reached_library_functions;
}

CalledFunctionsVisitor::CalledFunctionsVisitor(const bool _allow_recursive_functions, const CallGraphManager* _call_graph_manager, CustomOrderedSet<unsigned int>& _body_functions, CustomOrderedSet<unsigned int>& _library_functions)
    : allow_recursive_functions(_allow_recursive_functions), call_graph_manager(_call_graph_manager), body_functions(_body_functions), library_functions(_library_functions)
{
}

void CalledFunctionsVisitor::back_edge(const EdgeDescriptor& e, const CallGraph& g)
{
   if(not allow_recursive_functions)
   {
      const std::map<unsigned int, FunctionBehaviorRef>& behaviors = g.CGetCallGraphInfo()->behaviors;
      vertex source = boost::source(e, g);
      vertex target = boost::target(e, g);
      THROW_ERROR("Recursive functions not yet supported: " + behaviors.find(call_graph_manager->get_function(source))->second->CGetBehavioralHelper()->get_function_name() + "-->" +
                  behaviors.find(call_graph_manager->get_function(target))->second->CGetBehavioralHelper()->get_function_name());
   }
}

void CalledFunctionsVisitor::finish_vertex(const vertex& u, const CallGraph& g)
{
   unsigned int function_id = Cget_node_info<FunctionInfo, graph>(u, g)->nodeID;
   if(g.CGetCallGraphInfo()->behaviors.find(function_id)->second->CGetBehavioralHelper()->has_implementation())
      body_functions.insert(function_id);
   else
      library_functions.insert(function_id);
}
