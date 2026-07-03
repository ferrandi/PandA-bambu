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
 * @file application_manager.hpp
 * @brief Definition of the class representing a generic C application
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef _APPLICATION_MANAGER_HPP_
#define _APPLICATION_MANAGER_HPP_
#include "Parameter.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "ir_node.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

CONSTREF_FORWARD_DECL(ActorGraphManager);
REF_FORWARD_DECL(ActorGraphManager);
REF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(FunctionExpander);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(Discrepancy);
class CallGraphManager;

class application_manager
{
 private:
   /// The number of cfg transformations applied to this function
   size_t cfg_transformations;

#ifndef NDEBUG
   // The maximum number of cfg transformations specified by the user
   size_t cfg_max_transformations;
#endif

 protected:
   /// class representing the application information at low level
   const ir_managerRef TM;

   /// class representing the call graph of the application
   std::unique_ptr<CallGraphManager> call_graph_manager;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// set of global variables
   IRNodeConstSet global_variables;

   unsigned int address_bitsize;

   /// store memory objects which can be written
   CustomOrderedSet<unsigned int> written_objects;

   /// debugging level of the class
   const int debug_level;

   /// put into relation formal parameters and the associated ssa variables in a given function
   CustomMap<unsigned, CustomMap<unsigned, unsigned>> Parm2SSA_map;

   /**
    * Returns the values produced by a vertex (recursive version)
    */
   /// FIXME: to be remove after substitution with GetProducedValue
   unsigned int get_produced_value(const ir_nodeRef& tn) const;

   /**
    * Returns the values produced by a vertex (recursive version)
    */
   ir_nodeConstRef GetProducedValue(const ir_nodeConstRef& tn) const;

 public:
#if __cplusplus >= 201703L
   template <typename T>
   static std::string ParameterTypeName()
   {
      if constexpr(std::is_same_v<T, bool>)
      {
         return "bool";
      }
      else if constexpr(std::is_same_v<T, unsigned int>)
      {
         return "unsigned int";
      }
      else if constexpr(std::is_same_v<T, unsigned long>)
      {
         return "unsigned long";
      }
      else if constexpr(std::is_same_v<T, unsigned long long>)
      {
         return "unsigned long long";
      }
      else if constexpr(std::is_same_v<T, int>)
      {
         return "int";
      }
      else if constexpr(std::is_same_v<T, long>)
      {
         return "long";
      }
      else if constexpr(std::is_same_v<T, long long>)
      {
         return "long long";
      }
      else if constexpr(std::is_same_v<T, double>)
      {
         return "double";
      }
      else if constexpr(std::is_same_v<T, std::string>)
      {
         return "string";
      }
      else
      {
         return typeid(T).name();
      }
   }
#endif

#if HAVE_FROM_DISCREPANCY_BUILT
   /// The data for the discrepancy analysis
   DiscrepancyRef RDiscr;
#endif

   /// The original input file and the actual source code file to be elaborated
   std::vector<std::string> input_files;

   /**
    * Constructor
    * @param allow_recursive_functions specifies if recursive functions are allowed
    * @param _Param is the reference to the class containing all the parameters
    */
   application_manager(const bool allow_recursive_functions, const ParameterConstRef _Param);

   virtual ~application_manager() = default;

   /**
    * Returns the IR manager associated with the application
    */
   ir_managerRef get_ir_manager() const;

   /**
    * Returns the call graph associated with the application
    */
   CallGraphManager& GetCallGraphManager();

   /**
    * Returns the call graph associated with the application
    */
   const CallGraphManager& CGetCallGraphManager() const;

   /**
    * @brief Check for interface generation.
    *
    * The interface has to be created for the top function and
    * any additional top that is called through the bus.
    *
    * @param funId IR node index of a function.
    *
    * @returns true if is a top or is an additional top.
    */
   bool hasToBeInterfaced(unsigned int funId) const;

   /**
    * @brief isOmpLambdaFunction return true in case the function is an OMP lambda function
    * @param funId is the function id
    * @return true in case funId is an OMP lambda function
    */
   bool isOmpLambdaFunction(unsigned int funId) const;

   /**
    * @brief GetOMPThreadsCount return the number of threads asked for the lambda function
    * @param fun_id is the node id of the lambda function
    */
   unsigned GetOMPThreadsCount(unsigned int fun_id) const;

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
   FunctionBehaviorConstRef CGetFunctionBehavior(unsigned int index) const;

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
   void AddGlobalVariable(const ir_nodeConstRef& var);

   /**
    * Returns the set of original global variables
    * @return a set containing the identified of the global variables
    */
   const IRNodeConstSet& GetGlobalVariables() const;

   /**
    * Returns the value produced by a vertex
    */
   /// FIXME: to be remove after substitution with GetProducedValue
   unsigned int get_produced_value(unsigned int fun_id, gc_vertex_descriptor v) const;

   /**
    * Returns the value produced by a vertex
    */
   ir_nodeConstRef GetProducedValue(unsigned int fun_id, gc_vertex_descriptor v) const;

   /**
    * Returns the parameter data-structure
    */
   ParameterConstRef get_parameter() const;

   /**
    * Resolve a parameter with precedence: global parameter, then device parameter.
    * Returns true when a value is found.
    * Conversion/parsing errors are propagated to the caller.
    */
   template <typename T, typename DeviceRefT>
   bool TryGetParameterFromParameterOrDevice(const std::string& key, const DeviceRefT& device, T& value) const
   {
      if(Param && Param->IsParameter(key))
      {
         try
         {
            value = Param->GetParameter<T>(key);
            return true;
         }
         catch(const std::exception& e)
         {
            THROW_ERROR("Failed to parse parameter \"" + key + "\" from global parameters as type \"" +
                        ParameterTypeName<T>() + "\": " + e.what());
         }
         catch(...)
         {
            THROW_ERROR("Failed to parse parameter \"" + key + "\" from global parameters as type \"" +
                        ParameterTypeName<T>() + "\"");
         }
      }
      if(device && device->has_parameter(key))
      {
         try
         {
            value = device->template get_parameter<T>(key);
            return true;
         }
         catch(const std::exception& e)
         {
            THROW_ERROR("Failed to parse device parameter \"" + key + "\" as type \"" + ParameterTypeName<T>() +
                        "\": " + e.what());
         }
         catch(...)
         {
            THROW_ERROR("Failed to parse device parameter \"" + key + "\" as type \"" + ParameterTypeName<T>() + "\"");
         }
      }
      return false;
   }

   /**
    * Resolve a parameter with precedence: global parameter, then device parameter, then default value.
    * Conversion/parsing errors are propagated to the caller.
    */
   template <typename T, typename DeviceRefT>
   T GetParameterFromParameterOrDeviceOrDefault(const std::string& key, const DeviceRefT& device,
                                                const T& default_value) const
   {
      T value = default_value;
      if(TryGetParameterFromParameterOrDevice<T>(key, device, value))
      {
         return value;
      }
      return default_value;
   }

   /**
    * Resolve a required parameter with precedence: global parameter, then device parameter.
    * Throws if the parameter is not defined in either source.
    */
   template <typename T, typename DeviceRefT>
   T GetRequiredParameterFromParameterOrDevice(const std::string& key, const DeviceRefT& device) const
   {
      T value{};
      const bool found = TryGetParameterFromParameterOrDevice<T>(key, device, value);
      if(!found)
      {
         THROW_ERROR("Missing required parameter \"" + key + "\": define it in parameter or device parameter");
      }
      return value;
   }

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
    * @param new_tn is the IR node to be created
    */
#ifndef NDEBUG
   void RegisterTransformation(const std::string& step, const ir_nodeConstRef& new_tn);
#else
   inline void RegisterTransformation(const std::string&, const ir_nodeConstRef)
   {
   }
#endif

   /**
    * getSSAFromParm returns the ssa_node index associated with the argument_val_node index, 0 in case there is not an
    * associated index
    * @param functionID Id of the function to search the param in
    * @param parm_index is the argument_val_node index for which we look for the associated ssa_node index
    */
   unsigned getSSAFromParm(unsigned int functionID, unsigned parm_index) const;

   /**
    * @brief setSSAFromParm defines the argument_val_node versus ssa_node relation
    * @param functionID Id of the function containing the parameter
    * @param parm_index is the index of the argument_val_node
    * @param ssa_index is the index of the ssa_node
    */
   void setSSAFromParm(unsigned int functionID, unsigned int parm_index, unsigned ssa_index);

   /**
    * @brief clearParm2SSA cleans the map putting into relation argument_val_node and ssa_node
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
