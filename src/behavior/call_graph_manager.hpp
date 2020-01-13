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
 * @file call_graph_manager.hpp
 * @brief Wrapper to call graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CALL_GRAPH_MANAGER_HPP
#define CALL_GRAPH_MANAGER_HPP

#include "call_graph.hpp" // for CallGraph (ptr o...
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"                          // for vertex, EdgeDesc...
#include "refcount.hpp"                       // for CONSTREF_FORWARD...
#include <boost/graph/depth_first_search.hpp> // for default_dfs_visitor

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(CallGraph);
REF_FORWARD_DECL(CallGraph);
REF_FORWARD_DECL(CallGraphsCollection);
CONSTREF_FORWARD_DECL(FunctionExpander);
REF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);

/**
 * This class manages the accesses to the CallGraph
 */
class CallGraphManager
{
 private:
   const CallGraphsCollectionRef call_graphs_collection;

   /// The view of call graph with all the edges
   const CallGraphRef call_graph;

   /// The tree manager
   const tree_managerConstRef tree_manager;

   /// put into relation function F_i and the list of functions called by F_i
   std::map<unsigned int, CustomOrderedSet<unsigned int>> called_by;

   /// put into relation function F_i and the vertex in the call graph representing it
   std::map<unsigned int, vertex> functionID_vertex_map;

   /// True if only
   const bool single_root_function;

   /// True if recursive calls are allowed
   const bool allow_recursive_functions;

   /// Root functions
   CustomOrderedSet<unsigned int> root_functions;

   /// source code functions directly or indirectly called by the root functions
   CustomOrderedSet<unsigned int> reached_body_functions;

   /// library functions directly or indirectly called by the root functions
   CustomOrderedSet<unsigned int> reached_library_functions;

   /// set of functions whose address is taken
   CustomOrderedSet<unsigned int> addressed_functions;

   /// set of input parameters
   const ParameterConstRef Param;

   /// the debug level
   const int debug_level;

   /**
    * Compute the root and reached functions, maintaining the internal data
    * structures coherent
    */
   void ComputeRootAndReachedFunctions();

 public:
   /// Functor used to check if the analysis should consider the body of a function
   const FunctionExpanderConstRef function_expander;

   /**
    * Constructor. The data structure is initialized.
    * @param function_expander is the functor used to determine if a function has to be considered during construction of call graph
    * @param single_root_function specifies if only one root function has to be considered
    * @param allow_recursive_functions specifies if recursive functions are allowed
    * @param tree_manager is the tree manager
    * @param Param is the set of input parameters
    */
   CallGraphManager(const FunctionExpanderConstRef function_expander, const bool single_root_function, const bool allow_recursive_functions, const tree_managerConstRef tree_manager, const ParameterConstRef Param);

   /**
    * Destructor
    */
   ~CallGraphManager();

   /**
    * Return the call graph
    */
   CallGraphRef GetCallGraph();

   /**
    * Return an acyclic version of the call graph
    */
   const CallGraphConstRef CGetAcyclicCallGraph() const;

   /**
    * Return the call graph
    */
   const CallGraphConstRef CGetCallGraph() const;

   /**
    * Return a subset of the call graph
    * @param vertices is the subset of vertices to be considered
    */
   const CallGraphConstRef CGetCallSubGraph(const CustomUnorderedSet<vertex>& vertices) const;

   /**
    * Returns the set of functions called by a function
    * @param index is the index of the caller function
    */
   const CustomOrderedSet<unsigned int> get_called_by(unsigned int index) const;

   /**
    * Returns the set of functions called by an operation vertex
    * @param cfg is the pointer to the graph which the operation belongs to
    * @param caller is the caller vertex
    */
   const CustomUnorderedSet<unsigned int> get_called_by(const OpGraphConstRef cfg, const vertex& caller) const;

   /**
    * Given a vertex of the call graph, this returns the index of the corresponding function
    * @param node is the vertex of the function
    * @return the index of the function
    */
   unsigned int get_function(vertex node) const;

   /**
    * Return the vertex given the function id
    * @param index is the function index
    * @return the corresponding vertex in the call graph
    */
   vertex GetVertex(const unsigned int index) const;

   /**
    * Returns the root functions (i.e., the functions that are not called by any other ones
    * @return the set of top function
    */
   const CustomOrderedSet<unsigned int> GetRootFunctions() const;

   /**
    * Returns the source code functions called by the root functions
    * @return the set of top function
    */
   CustomOrderedSet<unsigned int> GetReachedBodyFunctions() const;

   /**
    * compute the list of reached function starting from a given function
    * @param from_f is the starting function
    * @return the set of top function
    */
   CustomOrderedSet<unsigned int> GetReachedBodyFunctionsFrom(unsigned int from_f) const;

   /**
    * Returns the library functions called by the root functions
    * @return the set of library function (without implementation)
    */
   CustomOrderedSet<unsigned int> GetReachedLibraryFunctions() const;

   /**
    * return true in case the vertex has been already created
    * @param functionID is the function identifier
    */
   bool IsVertex(unsigned int functionID) const;

   /**
    * Returns true if the call point is present
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the tree node index of the call statement
    * @param call_type is the type of call
    */
   bool IsCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, enum FunctionEdgeInfo::CallType call_type) const;

   /**
    * @param new_function_id is the index of the function to add
    * @param fun_behavior is the corresponding function behavior
    */
   void AddFunction(unsigned int new_function_id, const FunctionBehaviorRef fun_behavior);

   /**
    * Creates a new call point
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the tree node index of the call statement
    * @param call_type is the type of call
    */
   void AddCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, enum FunctionEdgeInfo::CallType call_type);

   /**
    * Creates a new called function and directly adds the call to the call graph
    * @param caller_id is the function id of the caller
    * @param called_id is the function id of the called function
    * @param call_id is the tree node index of the call statement
    * @param called_function_behavior is the FunctionBehavior of the called function
    * @param call_type is the type of call
    */
   void AddFunctionAndCallPoint(unsigned int caller_id, unsigned int called_id, unsigned int call_id, const FunctionBehaviorRef called_function_behavior, enum FunctionEdgeInfo::CallType call_type);

   /**
    * Remove a function call, like RemoveCallPoint with a different API
    * @param caller_id is the index of the calling function
    * @param called_id is the index of the called function
    * @param call_id is the index of the statement containing the call
    */
   void RemoveCallPoint(const unsigned int caller_id, const unsigned int called_id, const unsigned int call_id);

   /**
    * Removes a call point.  * The edge is completely removed if necessary
    * @param e is the edge in the call graph
    * @param call_id is the call graph point to remove
    */
   void RemoveCallPoint(EdgeDescriptor e, const unsigned int call_id);

   /**
    * Replaces a call point.
    * @param e is the edge in the call graph
    * @param old_call_id is the old call tree node id
    * @param new_call_id is the new call tree node id
    */
   void ReplaceCallPoint(const EdgeDescriptor e, const unsigned int old_call_id, const unsigned int new_call_id);

   /**
    * Returns true is there is at least a reachable function that is
    * called through a function pointer or its address is taken
    */
   bool ExistsAddressedFunction() const;

   /**
    * Returns a set containing all the reachable addressed_functions
    */
   CustomOrderedSet<unsigned int> GetAddressedFunctions() const;
};
typedef refcount<CallGraphManager> CallGraphManagerRef;
typedef refcount<const CallGraphManager> CallGraphManagerConstRef;

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
   CustomOrderedSet<unsigned int>& body_functions;

   /// The list of encountered library functions
   CustomOrderedSet<unsigned int>& library_functions;

 public:
   /**
    * Constructor
    * @param allow_recursive_functions tells if recursive functions are allowed
    * @param call_graph_manager is the call graph manager
    * @param body_functions is where results will be stored
    * @param library_functions is where results will be stored
    */
   CalledFunctionsVisitor(const bool allow_recursive_functions, const CallGraphManager* call_graph_manager, CustomOrderedSet<unsigned int>& body_functions, CustomOrderedSet<unsigned int>& library_functions);

   void back_edge(const EdgeDescriptor& edge, const CallGraph& call_graph);

   /**
    * Function called when a vertex has been finished
    * @param u is the vertex
    * @param call_graph is the call graph
    */
   void finish_vertex(const vertex& u, const CallGraph& call_graph);
};
#endif
