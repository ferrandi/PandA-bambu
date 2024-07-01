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
 * @file application_manager.hpp
 * @brief Definition of the class representing a generic C application
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _APPLICATION_MANAGER_HPP_
#define _APPLICATION_MANAGER_HPP_
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"
#include "tree_node.hpp"

#include <cstddef>
#include <string>

#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

CONSTREF_FORWARD_DECL(ActorGraphManager);
REF_FORWARD_DECL(ActorGraphManager);
REF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(CallGraphManager);
REF_FORWARD_DECL(CallGraphManager);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(FunctionExpander);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(pragma_manager);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(Discrepancy);

class application_manager
{
 private:
   /// The number of cfg transformations applied to this function
   size_t cfg_transformations;

   // The maximum number of cfg transformations specified by the user
   size_t cfg_max_transformations;

 protected:
   /// class representing the application information at low level
   const tree_managerRef TM;

   /// class representing the call graph of the application
   const CallGraphManagerRef call_graph_manager;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// set of global variables
   TreeNodeConstSet global_variables;

   unsigned int address_bitsize;

   /// store memory objects which can be written
   CustomOrderedSet<unsigned int> written_objects;

#if HAVE_PRAGMA_BUILT
   /// class representing the source code pragmas
   pragma_managerRef PM;
#endif

   /// debugging level of the class
   const int debug_level;

   /// put into relation formal parameters and the associated ssa variables in a given function
   CustomMap<unsigned, CustomMap<unsigned, unsigned>> Parm2SSA_map;

   /**
    * Returns the values produced by a vertex (recursive version)
    */
   /// FIXME: to be remove after substitution with GetProducedValue
   unsigned int get_produced_value(const tree_nodeRef& tn) const;

   /**
    * Returns the values produced by a vertex (recursive version)
    */
   tree_nodeConstRef GetProducedValue(const tree_nodeConstRef& tn) const;

 public:
#if HAVE_FROM_DISCREPANCY_BUILT
   /// The data for the discrepancy analysis
   DiscrepancyRef RDiscr;
#endif

   /// The original input file and the actual source code file to be elaborated
   std::vector<std::string> input_files;

   /**
    * Constructor
    * @param function_expander is the expander used to determine if a called function has to be examinedi
    * @param allow_recursive_functions specifies if recursive functions are allowed
    * @param _Param is the reference to the class containing all the parameters
    */
   application_manager(const FunctionExpanderConstRef function_expander, const bool allow_recursive_functions,
                       const ParameterConstRef _Param);

   /**
    * Destructor
    */
   virtual ~application_manager();

   /**
    * Returns the tree manager associated with the application
    */
   const tree_managerRef get_tree_manager() const;

   /**
    * Returns the call graph associated with the application
    */
   CallGraphManagerRef GetCallGraphManager();

   /**
    * Returns the call graph associated with the application
    */
   const CallGraphManagerConstRef CGetCallGraphManager() const;

   /**
    * @brief Check for interface generation.
    *
    * The interface has to be created for the top function and
    * any additional top that is called through the bus.
    *
    * @param funId tree index of a function.
    *
    * @returns true if is a top or is an additional top.
    */
   bool hasToBeInterfaced(unsigned int funId) const;

   /**
    * Returns the data structure associated with the given identifier. This method returns an error if the function does
    * not exist.
    * @param index is the identified of the function to be returned
    * @return the FunctionBehavior associated with the given function
    */
   FunctionBehaviorRef GetFunctionBehavior(unsigned int index);

   /**
    * Returns the data-structure associated with the given identifier. This method returns an error if the function does
    * not exist.
    * @param index is the identified of the function to be returned
    * @return the FunctionBehavior associated with the given function
    */
   const FunctionBehaviorConstRef CGetFunctionBehavior(unsigned int index) const;

   /**
    * Returns the set of functions whose implementation is present in the parsed
    * input specification (i.e. which has a non empty graph)
    */
   CustomOrderedSet<unsigned int> get_functions_with_body() const;

   /**
    * Returns the set of functions whose implementation is not present in the parsed
    * input specification (i.e. the ones with an empty Control Flow Graph)
    */
   CustomOrderedSet<unsigned int> get_functions_without_body() const;

   /**
    * Adds a global variable
    * @param var is the global variable to be added
    */
   void AddGlobalVariable(const tree_nodeConstRef& var);

   /**
    * Returns the set of original global variables
    * @return a set containing the identified of the global variables
    */
   const TreeNodeConstSet& GetGlobalVariables() const;

#if HAVE_PRAGMA_BUILT
   /**
    * Returns the reference to the manager for the source code pragmas
    */
   const pragma_managerRef get_pragma_manager() const;
#endif

   /**
    * Returns the value produced by a vertex
    */
   /// FIXME: to be remove after substitution with GetProducedValue
   unsigned int get_produced_value(unsigned int fun_id, const vertex& v) const;

   /**
    * Returns the value produced by a vertex
    */
   tree_nodeConstRef GetProducedValue(unsigned int fun_id, const vertex& v) const;

   /**
    * Returns the parameter data-structure
    */
   const ParameterConstRef get_parameter() const;

   /**
    * Add the node_id to the set of object modified by a store
    * @param node_id is the object stored in memory
    */
   void add_written_object(unsigned int node_id);

   /**
    * Return the set of variables modified by a store
    */
   const CustomOrderedSet<unsigned int>& get_written_objects() const;

   /**
    * @brief clean_written_objects clean the written object data structure
    */
   void clean_written_objects();

   /**
    * set the value of the address bitsize
    * @param value is the new value
    */
   void set_address_bitsize(unsigned int value)
   {
      address_bitsize = value;
   }

   /**
    * return the address bitsize
    */
   unsigned int& Rget_address_bitsize()
   {
      return address_bitsize;
   }
   unsigned int get_address_bitsize() const
   {
      return address_bitsize;
   }

   /**
    * Return true if a new transformation can be applied
    */
   inline bool ApplyNewTransformation() const
   {
#ifndef NDEBUG
      return cfg_transformations < cfg_max_transformations;
#else
      return true;
#endif
   }

   /**
    * Register a transformation
    * @param step is the name of the step in which the transformation is applied
    * @param new_tn is the tree node to be created
    */
#ifndef NDEBUG
   void RegisterTransformation(const std::string& step, const tree_nodeConstRef& new_tn);
#else
   inline void RegisterTransformation(const std::string&, const tree_nodeConstRef)
   {
   }
#endif

   /**
    * getSSAFromParm returns the ssa_name index associated with the parm_decl index, 0 in case there is not an
    * associated index
    * @param functionID Id of the function to search the param in
    * @param parm_index is the parm_decl index for which we look for the associated ssa_name index
    */
   unsigned getSSAFromParm(unsigned int functionID, unsigned parm_index) const;

   /**
    * @brief setSSAFromParm defines the parm_decl versus ssa_name relation
    * @param parm_index is the index of the parm_decl
    * @param ssa_index is the index of the ssa_name
    */
   void setSSAFromParm(unsigned int functionID, unsigned int parm_index, unsigned ssa_index);

   /**
    * @brief clearParm2SSA cleans the map putting into relation parm_decl and ssa_name
    */
   void clearParm2SSA(unsigned int functionID);

   /**
    * return a copy of parameter to SSA map
    */
   CustomMap<unsigned, unsigned> getACopyParm2SSA(unsigned int functionID);
};
/// refcount definition of the class
using application_managerRef = refcount<application_manager>;
/// constant refcount definition of the class
using application_managerConstRef = refcount<const application_manager>;

#endif
