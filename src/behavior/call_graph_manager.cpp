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

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/tuple/tuple.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "config_HAVE_ASSERTS.hpp"

/**
 * Helper macro adding a call point to an edge of the call graph
 * @param g is the graph
 * @param e is the edge
 * @param newstmt is the call point to be added
 */
#define ADD_CALL_POINT(g, e, newstmt) get_edge_info<function_graph_edge_info>(e, *(g))->call_points.insert(newstmt)

/**
 * Visitor to identify the list of called functions
 */
struct CalledFunctionsVisitor : public boost::default_dfs_visitor
{
 private:
   /// True if recursive calls are allowed
   const bool allow_recursive_functions;

   /// The call graph manager
   const CallGraphManager* call_graph_manager;

   /// The list of encountered body functions
   CustomSet<unsigned int>& body_functions;

   /// The list of encountered library functions
   CustomSet<unsigned int>& library_functions;

 public:
   /**
    * Constructor
    * @param allow_recursive_functions tells if recursive functions are allowed
    * @param call_graph_manager is the call graph manager
    * @param body_functions is where results will be stored
    * @param library_functions is where results will be stored
    */
   CalledFunctionsVisitor(const bool _allow_recursive_functions, const CallGraphManager* _call_graph_manager,
                          CustomSet<unsigned int>& _body_functions, CustomSet<unsigned int>& _library_functions)
       : allow_recursive_functions(_allow_recursive_functions),
         call_graph_manager(_call_graph_manager),
         body_functions(_body_functions),
         library_functions(_library_functions)
   {
   }

   void back_edge(const EdgeDescriptor& e, const CallGraph& g)
   {
      if(!allow_recursive_functions)
      {
         const auto& behaviors = g.CGetCallGraphInfo()->behaviors;
         const auto source = boost::source(e, g);
         const auto target = boost::target(e, g);
         THROW_ERROR(
             "Recursive functions not yet supported: " +
             behaviors.at(call_graph_manager->get_function(source))->CGetBehavioralHelper()->get_function_name() +
             "-->" +
             behaviors.at(call_graph_manager->get_function(target))->CGetBehavioralHelper()->get_function_name());
      }
   }

   /**
    * Function called when a vertex has been finished
    * @param u is the vertex
    * @param call_graph is the call graph
    */
   void finish_vertex(const vertex& u, const CallGraph& g)
   {
      const auto f_id = g.CGetFunctionNodeInfo(u)->nodeID;
      if(g.CGetCallGraphInfo()->behaviors.at(f_id)->CGetBehavioralHelper()->has_implementation())
      {
         body_functions.insert(f_id);
      }
      else
      {
         library_functions.insert(f_id);
      }
   }
};

CallGraphManager::CallGraphManager(const FunctionExpanderConstRef _function_expander,
                                   const bool _allow_recursive_functions, const tree_managerConstRef _tree_manager,
                                   const ParameterConstRef _Param)
    : call_graphs_collection(new CallGraphsCollection(CallGraphInfoRef(new CallGraphInfo()), _Param)),
      call_graph(new CallGraph(call_graphs_collection, STD_SELECTOR | FEEDBACK_SELECTOR)),
      tree_manager(_tree_manager),
      allow_recursive_functions(_allow_recursive_functions),
      Param(_Param),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this))),
      function_expander(_function_expander)
{
}

CallGraphManager::~CallGraphManager() = default;

void CallGraphManager::AddFunction(unsigned int new_function_id, const FunctionBehaviorRef fun_behavior)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Adding function: " + fun_behavior->CGetBehavioralHelper()->get_function_name() +
                      " id: " + STR(new_function_id));
   if(!IsVertex(new_function_id))
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
                   "adding a different behavior for " + STR(new_function_id) +
                       " prev: " + STR(call_graph->GetCallGraphInfo()->behaviors.at(new_function_id)) +
                       " new: " + STR(fun_behavior));
   }
}

void CallGraphManager::AddCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                    enum FunctionEdgeInfo::CallType call_type)
{
#if !defined(NDEBUG) || HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " +
                            tree_helper::print_function_name(
                                tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(caller_id)));
   const auto called_name = "(" + STR(called_id) + ") " +
                            tree_helper::print_function_name(
                                tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(called_id)));
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
   THROW_ASSERT(found, "call id " + STR(call_id) + " from " + caller_name + " to " + called_name +
                           " was not in the call graph");

   const auto functionEdgeInfo = call_graph->GetFunctionEdgeInfo(e);
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

   EdgeDescriptor e;
   bool found;
   boost::tie(e, found) = boost::edge(src, tgt, *CGetCallGraph());
#if HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " +
                            tree_helper::print_function_name(
                                tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(caller_id)));
   const auto called_name = "(" + STR(called_id) + ") " +
                            tree_helper::print_function_name(
                                tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(called_id)));
#endif
   THROW_ASSERT(found, "call id " + STR(call_id) + " from " + caller_name + " to " + called_name +
                           " was not in the call graph");

   const auto functionEdgeInfo = call_graph->CGetFunctionEdgeInfo(e);

   bool res = false;
   switch(call_type)
   {
      case FunctionEdgeInfo::CallType::direct_call:
         res = functionEdgeInfo->direct_call_points.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "direct call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::indirect_call:
         res = functionEdgeInfo->indirect_call_points.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "indirect call! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::function_address:
         res = functionEdgeInfo->function_addresses.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function_address! present? " + STR(res));
         break;
      case FunctionEdgeInfo::CallType::call_any:
         res = functionEdgeInfo->direct_call_points.count(call_id) ||
               functionEdgeInfo->indirect_call_points.count(call_id) ||
               functionEdgeInfo->function_addresses.count(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "any call! present? " + STR(res));
         break;
      default:
         THROW_UNREACHABLE("unexpected call type");
   }
   return res;
}

void CallGraphManager::AddFunctionAndCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                               const FunctionBehaviorRef called_function_behavior,
                                               enum FunctionEdgeInfo::CallType call_type)
{
   AddFunction(called_id, called_function_behavior);
   AddCallPoint(caller_id, called_id, call_id, call_type);
}

void CallGraphManager::AddFunctionAndCallPoint(const application_managerRef AppM, unsigned int caller_id,
                                               unsigned int called_id, unsigned int call_id,
                                               enum FunctionEdgeInfo::CallType call_type)
{
   if(tree_helper::GetFunctionName(tree_manager, tree_manager->GetTreeNode(called_id)) != BUILTIN_WAIT_CALL)
   {
      if(!IsVertex(called_id))
      {
         bool has_body = tree_manager->get_implementation_node(called_id) != 0;
         BehavioralHelperRef helper =
             BehavioralHelperRef(new BehavioralHelper(AppM, called_id, has_body, AppM->get_parameter()));
         FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, AppM->get_parameter()));
         AddFunctionAndCallPoint(caller_id, called_id, call_id, FB, call_type);
      }
      else
      {
         AddCallPoint(caller_id, called_id, call_id, call_type);
      }
   }
}

void CallGraphManager::RemoveCallPoint(EdgeDescriptor e, const unsigned int callid)
{
   const auto called_id = call_graph->CGetFunctionNodeInfo(boost::target(e, *call_graph))->nodeID;
   const auto called_name = tree_helper::print_function_name(
       tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(called_id)));
   if(called_name == BUILTIN_WAIT_CALL)
   {
      return;
   }
   const auto caller_id = call_graph->CGetFunctionNodeInfo(boost::source(e, *call_graph))->nodeID;

   const auto edge_info = call_graph->GetFunctionEdgeInfo(e);
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

#if !defined(NDEBUG) || HAVE_ASSERTS
   const auto caller_name = "(" + STR(caller_id) + ") " +
                            tree_helper::print_function_name(
                                tree_manager, GetPointerS<const function_decl>(tree_manager->GetTreeNode(caller_id)));
#endif
   THROW_ASSERT(found_calls, "call id " + STR(callid) + " is not a call point in function " + caller_name +
                                 " for function (" + STR(called_id) + ") " + called_name);
   THROW_ASSERT(found_calls == 1, "call id " + STR(callid) + " is a multiple call point in function " + caller_name +
                                      " for function (" + STR(called_id) + ") " + called_name);

   if(direct_calls.empty() && indirect_calls.empty() && function_addresses.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Removed function call edge: " + caller_name + " -> (" + STR(called_id) + ") " + called_name);
      const auto called_v = boost::target(e, *call_graphs_collection);
      boost::remove_edge(boost::source(e, *call_graphs_collection), called_v, *call_graphs_collection);
      called_by.at(caller_id).erase(called_id);
      call_graphs_collection->RemoveSelector(e);
      ComputeRootAndReachedFunctions();
      // InEdgeIterator it, it_end;
      // boost::tie(it, it_end) = boost::in_edges(called_v, *call_graph);
      // if(it == it_end)
      // {
      //    call_graphs_collection->RemoveVertex(called_v);
      //    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed dangling function vertex: (" +
      //    STR(called_id) + ") " + called_name);
      // }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "There are still " + STR(direct_calls.size()) + " direct calls, " + STR(indirect_calls.size()) +
                         " indirect calls, and " + STR(function_addresses.size()) +
                         " places where the address is taken");
   }
   if(indirect_calls.empty() && function_addresses.empty())
   {
      addressed_functions.erase(called_id);
   }
}

void CallGraphManager::RemoveCallPoint(const unsigned int caller_id, const unsigned int called_id,
                                       const unsigned int call_id)
{
   const auto called_name = tree_helper::GetFunctionName(tree_manager, tree_manager->GetTreeNode(called_id));
   if(called_name == BUILTIN_WAIT_CALL)
   {
      return;
   }
   const auto caller_vertex = GetVertex(caller_id);
   const auto called_vertex = GetVertex(called_id);
   EdgeDescriptor e;
   bool found;
   boost::tie(e, found) = boost::edge(caller_vertex, called_vertex, *CGetCallGraph());
   THROW_ASSERT(found, "call id " + STR(call_id) + " is not a call point in function (" + STR(caller_id) + ") " +
                           tree_helper::GetFunctionName(tree_manager, tree_manager->GetTreeNode(caller_id)) +
                           " for function (" + STR(called_id) + ") " + called_name);
   RemoveCallPoint(e, call_id);
}

void CallGraphManager::ReplaceCallPoint(const EdgeDescriptor e, const unsigned int orig, const unsigned int repl)
{
   THROW_ASSERT(orig != repl, "old call point is replaced with itself");
   const auto caller_id = call_graph->CGetFunctionNodeInfo(boost::source(e, *call_graph))->nodeID;
   const auto called_id = call_graph->CGetFunctionNodeInfo(boost::target(e, *call_graph))->nodeID;

   auto old_call_type = FunctionEdgeInfo::CallType::direct_call;
   const auto edge_info = call_graph->CGetFunctionEdgeInfo(e);
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

CallGraphConstRef CallGraphManager::CGetAcyclicCallGraph() const
{
   return CallGraphRef(new CallGraph(call_graphs_collection, STD_SELECTOR));
}

CallGraphConstRef CallGraphManager::CGetCallGraph() const
{
   return call_graph;
}

CallGraphConstRef CallGraphManager::CGetCallSubGraph(const CustomUnorderedSet<vertex>& vertices) const
{
   return CallGraphConstRef(new CallGraph(call_graphs_collection, STD_SELECTOR | FEEDBACK_SELECTOR, vertices));
}

vertex CallGraphManager::GetVertex(const unsigned int index) const
{
   THROW_ASSERT(functionID_vertex_map.find(index) != functionID_vertex_map.end(),
                "this vertex does not exist " + STR(index));
   return functionID_vertex_map.at(index);
}

bool CallGraphManager::IsVertex(unsigned int functionID) const
{
   return functionID_vertex_map.find(functionID) != functionID_vertex_map.end();
}

unsigned int CallGraphManager::get_function(vertex node) const
{
   return call_graph->CGetFunctionNodeInfo(node)->nodeID;
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

CustomSet<unsigned int> CallGraphManager::get_called_by(const OpGraphConstRef cfg, const vertex& caller) const
{
   return cfg->CGetOpNodeInfo(caller)->called;
}

void CallGraphManager::ComputeRootAndReachedFunctions()
{
   reached_body_functions.clear();
   reached_library_functions.clear();
   CalledFunctionsVisitor vis(allow_recursive_functions, this, reached_body_functions, reached_library_functions);
   for(const auto root_id : root_functions)
   {
      if(IsVertex(root_id))
      {
         std::vector<boost::default_color_type> color_vec(boost::num_vertices(*call_graph));
         const auto top_vertex = GetVertex(root_id);
         boost::depth_first_visit(*call_graph, top_vertex, vis,
                                  boost::make_iterator_property_map(color_vec.begin(),
                                                                    boost::get(boost::vertex_index_t(), *call_graph),
                                                                    boost::white_color),
                                  [&](vertex u, const CallGraph& g) {
                                     const auto u_id = g.CGetFunctionNodeInfo(u)->nodeID;
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
   THROW_ASSERT(boost::num_vertices(*call_graph) == 0 || root_functions.size(),
                "Root functions have not yet been computed");
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
   CalledFunctionsVisitor vis(allow_recursive_functions, this, f_list, with_body ? dummy : f_list);
   std::vector<boost::default_color_type> color_vec(boost::num_vertices(*call_graph));
   boost::depth_first_visit(*call_graph, top_vertex, vis,
                            boost::make_iterator_property_map(color_vec.begin(),
                                                              boost::get(boost::vertex_index_t(), *call_graph),
                                                              boost::white_color),
                            [&](vertex u, const CallGraph& g) {
                               const auto u_id = g.CGetFunctionNodeInfo(u)->nodeID;
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
      CalledFunctionsVisitor vis(allow_recursive_functions, this, f_list, f_list);
      std::vector<boost::default_color_type> color_vec(boost::num_vertices(*call_graph));
      boost::depth_first_visit(
          *call_graph, top_vertex, vis,
          boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), *call_graph),
                                            boost::white_color),
          [&](vertex u, const CallGraph& g) { return root_functions.count(g.CGetFunctionNodeInfo(u)->nodeID); });
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

void CallGraphManager::expandCallGraphFromFunction(CustomUnorderedSet<unsigned int>& AV,
                                                   const application_managerRef AM, unsigned int f_id, int DL)
{
   const auto TM = AM->get_tree_manager();
   const auto has_body = TM->get_implementation_node(f_id) != 0;
   if(has_body)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyze body of " + tree_helper::name_function(TM, f_id));
      const auto fun = TM->GetTreeNode(f_id);
      const auto fd = GetPointerS<const function_decl>(fun);
      const auto sl = GetPointerS<const statement_list>(fd->body);
      if(sl->list_of_bloc.empty())
      {
         THROW_ERROR("We can only work on CFG provided by GCC/CLANG");
      }
      else
      {
         for(const auto& b : sl->list_of_bloc)
         {
            for(const auto& stmt : b.second->CGetStmtList())
            {
               call_graph_computation_recursive(AV, AM, f_id, TM, stmt, stmt->index,
                                                FunctionEdgeInfo::CallType::function_address, DL);
            }
         }
      }
   }
}
void CallGraphManager::addCallPointAndExpand(CustomUnorderedSet<unsigned int>& AV, const application_managerRef AM,
                                             unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                             enum FunctionEdgeInfo::CallType call_type, int DL)
{
   bool has_been_previously_added = AM->GetCallGraphManager()->IsVertex(called_id);
   AM->GetCallGraphManager()->AddFunctionAndCallPoint(AM, caller_id, called_id, call_id, call_type);
   if(!has_been_previously_added)
   {
      expandCallGraphFromFunction(AV, AM, called_id, DL);
   }
}

void CallGraphManager::call_graph_computation_recursive(CustomUnorderedSet<unsigned int>& AV,
                                                        const application_managerRef AM, unsigned int current,
                                                        const tree_managerRef& TM, const tree_nodeRef& tn,
                                                        unsigned int node_stmt,
                                                        enum FunctionEdgeInfo::CallType call_type, int DL)
{
   unsigned int ind = tn->index;
   if(tn->get_kind() != function_decl_K)
   {
      if(AV.find(ind) != AV.end())
      {
         return;
      }
      else
      {
         AV.insert(ind);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL,
                  "-->Recursive analysis of " + STR(ind) + " of type " + tn->get_kind_text() + "(statement is " +
                      tn->ToString() + ")");

   switch(tn->get_kind())
   {
      case function_decl_K:
      {
         unsigned int impl = TM->get_implementation_node(ind);
         if(impl)
         {
            ind = impl;
         }
         /// check for nested function
         const tree_nodeRef fun = TM->GetTreeNode(ind);
         const auto* fd = GetPointerS<const function_decl>(fun);
         if(fd->scpe && fd->scpe->get_kind() == function_decl_K)
         {
            THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(ind));
            THROW_ERROR("Nested functions not yet supported " + STR(ind));
         }
         AM->GetCallGraphManager()->AddFunctionAndCallPoint(AM, current, ind, node_stmt, call_type);
         if(AV.find(ind) != AV.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "<--");
            return;
         }
         else
         {
            AV.insert(ind);
         }
         expandCallGraphFromFunction(AV, AM, ind, DL);
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointerS<gimple_return>(tn);
         if(re->op)
         {
            call_graph_computation_recursive(AV, AM, current, TM, re->op, node_stmt, call_type, DL);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* me = GetPointerS<gimple_assign>(tn);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzing left part");
         call_graph_computation_recursive(AV, AM, current, TM, me->op0, node_stmt, call_type, DL);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzed left part - Analyzing right part");
         call_graph_computation_recursive(AV, AM, current, TM, me->op1, node_stmt, call_type, DL);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "---Analyzed right part");
         if(me->predicate)
         {
            call_graph_computation_recursive(AV, AM, current, TM, me->predicate, node_stmt, call_type, DL);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         auto* ce = GetPointerS<call_expr>(tn);
         tree_nodeRef fun_node = ce->fn;
         if(fun_node->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointerS<unary_expr>(fun_node);
            fun_node = ue->op;
         }
         else if(fun_node->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(ce->fn);
         }

         call_graph_computation_recursive(AV, AM, current, TM, fun_node, node_stmt,
                                          FunctionEdgeInfo::CallType::direct_call, DL);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(AV, AM, current, TM, arg, node_stmt, call_type, DL);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointerS<gimple_call>(tn);
         tree_nodeRef fun_node = ce->fn;
         if(fun_node->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointerS<unary_expr>(fun_node);
            fun_node = ue->op;
         }
         else if(fun_node->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(ce->fn);
         }
         call_graph_computation_recursive(AV, AM, current, TM, fun_node, node_stmt,
                                          FunctionEdgeInfo::CallType::direct_call, DL);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(AV, AM, current, TM, arg, node_stmt, call_type, DL);
         }
         break;
      }
      case cond_expr_K:
      {
         auto* ce = GetPointerS<cond_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, ce->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, ce->op1, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, ce->op2, node_stmt, call_type, DL);
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointerS<gimple_cond>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, gc->op0, node_stmt, call_type, DL);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointerS<unary_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, ue->op, node_stmt, call_type, DL);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointerS<binary_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, be->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, be->op1, node_stmt, call_type, DL);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         auto* se = GetPointerS<gimple_switch>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, se->op0, node_stmt, call_type, DL);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointerS<gimple_multi_way_if>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               call_graph_computation_recursive(AV, AM, current, TM, cond.first, node_stmt, call_type, DL);
            }
         }
         break;
      }
      case obj_type_ref_K:
      {
         tree_nodeRef fun = tree_helper::find_obj_type_ref_function(tn);
         call_graph_computation_recursive(AV, AM, current, TM, fun, node_stmt, call_type, DL);
         break;
      }
      case save_expr_K:
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case insertvalue_expr_K:
      case insertelement_expr_K:
      case bit_ior_concat_expr_K:
      {
         auto* te = GetPointerS<ternary_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, te->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, te->op1, node_stmt, call_type, DL);
         if(te->op2)
         {
            call_graph_computation_recursive(AV, AM, current, TM, te->op2, node_stmt, call_type, DL);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointerS<quaternary_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, qe->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, qe->op1, node_stmt, call_type, DL);
         if(qe->op2)
         {
            call_graph_computation_recursive(AV, AM, current, TM, qe->op2, node_stmt, call_type, DL);
         }
         if(qe->op3)
         {
            call_graph_computation_recursive(AV, AM, current, TM, qe->op3, node_stmt, call_type, DL);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointerS<lut_expr>(tn);
         call_graph_computation_recursive(AV, AM, current, TM, le->op0, node_stmt, call_type, DL);
         call_graph_computation_recursive(AV, AM, current, TM, le->op1, node_stmt, call_type, DL);
         if(le->op2)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op2, node_stmt, call_type, DL);
         }
         if(le->op3)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op3, node_stmt, call_type, DL);
         }
         if(le->op4)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op4, node_stmt, call_type, DL);
         }
         if(le->op5)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op5, node_stmt, call_type, DL);
         }
         if(le->op6)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op6, node_stmt, call_type, DL);
         }
         if(le->op7)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op7, node_stmt, call_type, DL);
         }
         if(le->op8)
         {
            call_graph_computation_recursive(AV, AM, current, TM, le->op8, node_stmt, call_type, DL);
         }
         break;
      }
      case constructor_K:
      {
         auto* c = GetPointerS<constructor>(tn);
         for(const auto& i : c->list_of_idx_valu)
         {
            call_graph_computation_recursive(AV, AM, current, TM, i.second, node_stmt, call_type, DL);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         auto* vd = GetPointerS<var_decl>(tn);
         if(vd->init)
         {
            call_graph_computation_recursive(AV, AM, current, TM, vd->init, node_stmt, call_type, DL);
         }
      }
      case result_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case template_decl_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_asm_K:
      case gimple_phi_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case CASE_PRAGMA_NODES:
      case gimple_pragma_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case CASE_CPP_NODES:
      case case_label_expr_K:
      case CASE_FAKE_NODES:
      case const_decl_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case target_expr_K:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR(std::string("Node not supported (") + STR(ind) + std::string("): ") + tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, DL, "<--Completed the recursive analysis of node " + STR(ind));
}
