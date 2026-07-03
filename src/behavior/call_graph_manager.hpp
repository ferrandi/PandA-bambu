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
 * @file call_graph_manager.hpp
 * @brief Wrapper to call graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef CALL_GRAPH_MANAGER_HPP
#define CALL_GRAPH_MANAGER_HPP

#include "call_graph.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"

class OpGraph;
CONSTREF_FORWARD_DECL(FunctionExpander);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * This class manages the accesses to the CallGraph
 */
class CallGraphManager : private CallGraphsCollection
{
   friend class call_graph_computation;

   /// The view of call graph with all the edges
   CallGraph call_graph;

   /// The IR manager
   const ir_managerConstRef ir_manager;

   /// put into relation function F_i and the list of functions called by F_i
   std::map<unsigned int, CustomSet<unsigned int>> called_by;

   /// put into relation function F_i and the vertex in the call graph representing it
   std::map<unsigned int, vertex_descriptor> functionID_vertex_map;

   /// True if recursive calls are allowed
   const bool allow_recursive_functions;

   /// Root functions
   CustomSet<unsigned int> root_functions;

   /// source code functions directly or indirectly called by the root functions
   CustomSet<unsigned int> reached_body_functions;

   /// library functions directly or indirectly called by the root functions
   CustomSet<unsigned int> reached_library_functions;

   /// set of functions whose address is taken
   CustomSet<unsigned int> addressed_functions;

   /// map omp lambda functions with the number of processes associated with them
   CustomOrderedMap<unsigned int, unsigned int> omp_lambda_functions;

   /// the debug level
   const int debug_level;

   /**
    * Creates a new call point
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the IR node index of the call statement
    * @param call_type is the type of call
    */
   void AddCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                     enum FunctionEdgeInfo::CallType call_type);

   /**
    * Returns true if the call point is present
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the IR node index of the call statement
    * @param call_type is the type of call
    */
   bool IsCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                    enum FunctionEdgeInfo::CallType call_type) const;

   /**
    * Compute the root and reached functions, maintaining the internal data
    * structures coherent
    */
   void UpdateReachedFunctions();

 public:
   /**
    * Constructor. The data structure is initialized.
    * @param allow_recursive_functions specifies if recursive functions are allowed
    * @param ir_manager is the IR manager
    * @param Param is the set of input parameters
    */
   CallGraphManager(const bool allow_recursive_functions, const ir_managerConstRef& ir_manager,
                    const ParameterConstRef& Param);

   CallGraphManager(const CallGraphManager&) = delete;

   /**
    * Return an acyclic version of the call graph
    */
   CallGraph CGetAcyclicCallGraph() const;

   /**
    * Return the call graph
    */
   const CallGraph& GetCallGraph() const;

   /**
    * Return a subset of the call graph
    * @param vertices is the subset of vertices to be considered
    */
   CallGraph CGetCallSubGraph(const CustomUnorderedSet<vertex_descriptor>& vertices) const;

   /**
    * Returns the set of functions called by a function
    * @param index is the index of the caller function
    */
   CustomSet<unsigned int> get_called_by(unsigned int index) const;

   /**
    * Returns the set of functions called by an operation vertex
    * @param cfg is the pointer to the graph which the operation belongs to
    * @param caller is the caller vertex
    */
   CustomSet<unsigned int> get_called_by(const OpGraph& cfg, vertex_descriptor caller) const;

   /**
    * Given a vertex of the call graph, this returns the index of the corresponding function
    * @param node is the vertex of the function
    * @return the index of the function
    */
   unsigned int get_function(vertex_descriptor node) const;

   /**
    * Return the vertex given the function id
    * @param index is the function index
    * @return the corresponding vertex in the call graph
    */
   vertex_descriptor GetVertex(unsigned int index) const;

   /**
    * @brief Set the root functions
    *
    * @param root_functions Set of root function ids
    */
   void SetRootFunctions(const CustomSet<unsigned int>& root_functions);

   /**
    * Returns the root functions (i.e., the functions that are not called by any other ones
    * @return the set of top function
    */
   const CustomSet<unsigned int>& GetRootFunctions() const;

   /**
    * Returns the source code functions called by the root functions
    * @return the set of top function
    */
   const CustomSet<unsigned int>& GetReachedBodyFunctions() const;

   /**
    * compute the list of reached function starting from a given function
    * @param from_f is the starting function
    * @param with_body consider only functions with body IR
    * @return the set of top function
    */
   CustomSet<unsigned int> GetReachedFunctionsFrom(unsigned int from_f, bool with_body = true) const;

   /**
    * @brief Get the parent root function
    *
    * @param fid Function id
    * @return unsigned int Parent root function id
    */
   unsigned int GetRootFunction(unsigned int fid) const;

   /**
    * Returns the library functions called by the root functions
    * @return the set of library function (without implementation)
    */
   CustomSet<unsigned int> GetReachedLibraryFunctions() const;

   /**
    * return true in case the vertex has been already created
    * @param functionID is the function identifier
    */
   bool IsVertex(unsigned int functionID) const;

   /**
    * @param new_function_id is the index of the function to add
    * @param fun_behavior is the corresponding function behavior
    */
   void AddFunction(unsigned int new_function_id, const FunctionBehaviorRef& fun_behavior);

   /**
    * Creates a new called function and directly adds the call to the call graph
    * @param AppM is the application manager
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the IR node index of the call statement
    * @param call_type is the type of call
    */
   void AddFunctionAndCallPoint(const application_managerRef& AppM, unsigned int caller_id, unsigned int called_id,
                                unsigned int call_id, enum FunctionEdgeInfo::CallType call_type);

   /**
    * Remove a function call, like RemoveCallPoint with a different API
    * @param caller_id is the index of the calling function
    * @param called_id is the index of the called function
    * @param call_id is the index of the statement containing the call
    */
   void RemoveCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id);

   /**
    * Removes a call point.
    * The edge is completely removed if necessary
    * @param e is the edge in the call graph
    * @param call_id is the call graph point to remove
    */
   void RemoveCallPoint(const edge_descriptor& e, unsigned int call_id);

   /**
    * Replaces a call point.
    * @param e is the edge in the call graph
    * @param old_call_id is the old call IR node id
    * @param new_call_id is the new call IR node id
    */
   void ReplaceCallPoint(const edge_descriptor& e, unsigned int old_call_id, unsigned int new_call_id);

   /**
    * Returns true is there is at least a reachable function that is
    * called through a function pointer or its address is taken
    */
   bool ExistsAddressedFunction() const;

   /**
    * Returns a set containing all the reachable addressed_functions
    */
   CustomSet<unsigned int> GetAddressedFunctions() const;

   /**
    * @brief SetOMPThreadsCount add a omp lambda function to the call graph manager
    * @param fun_id is the function id
    * @param nproc is the number of processes
    */
   void SetOMPThreadsCount(unsigned int fun_id, unsigned nproc);

   /**
    * Returns a set containing all the reachable addressed_functions
    */
   CustomOrderedSet<unsigned int> GetOMPLambdaFunctions() const;

   /**
    * @brief GetOMPThreadsCount return the number of threads asked for the lambda function
    * @param fun_id is the node id of the lambda function
    */
   unsigned GetOMPThreadsCount(unsigned int fun_id) const;

   /**
    * @brief IsOMPLambdaFunction returns true in case the function is a OMP lambda function
    * @param fun_id is the node id of the lambda function
    * @return return true in case fun_id is a lambda function
    */
   bool IsOMPLambdaFunction(unsigned int fun_id) const;

   static void expandCallGraphFromFunction(CustomUnorderedSet<unsigned int>& AV, const application_managerRef& AM,
                                           unsigned int f_id, int DL);

   static void addCallPointAndExpand(CustomUnorderedSet<unsigned int>& AV, const application_managerRef& AM,
                                     unsigned int caller_id, unsigned int called_id, unsigned int call_id,
                                     enum FunctionEdgeInfo::CallType call_type, int DL);
};

#endif
