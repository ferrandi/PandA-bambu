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
 * @file check_system_type.hpp
 * @brief analyse srcp of variables and types to detect system ones; the identified one are flagged
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CHECK_SYSTEM_TYPE_HPP
#define CHECK_SYSTEM_TYPE_HPP

/// Autoheader include
#include "config_HAVE_LEON3.hpp"

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// STD include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <vector>

/// Utility include
#include "refcount.hpp"
#include "utility.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);

/**
 * Class which system_flag to tree_node of variables and types when necessary
 */
class CheckSystemType : public FunctionFrontendFlowStep
{
 private:
   /// The helper associated with the current function
   const BehavioralHelperConstRef behavioral_helper;

   /// The tree manager
   const tree_managerRef TM;

   /// Contains the list of the folders containing the system header files
   static std::vector<std::string> systemIncPath;

   /// Associates to each system header file its full path
   static CustomUnorderedMapUnstable<std::string, std::string> inclNameToPath;

   /// Associates a function to its original name
   static CustomUnorderedMapUnstable<std::string, std::string> rename_function;

   /// Associates a type to its original name
   static CustomUnorderedMapUnstable<std::string, std::string> rename_types;

   /// The set of functions which have to be considered library_system
   static CustomUnorderedSet<std::string> library_system_functions;

   /// The set of headers which contains function which have to be considered library_system
   static CustomUnorderedSet<std::string> library_system_includes;

#if HAVE_LEON3
   /// The set of system function not supported by bcc
   static CustomUnorderedSet<std::string> not_supported_leon3_functions;
#endif

   /// Map undefined library function to corresponding header
   static CustomUnorderedMapUnstable<std::string, std::string> undefined_library_function_include;

   /// Already executed
   bool already_executed;

   /**
    * Examinate recursively the tree to detect system types and system variables
    * @param tn is the root of the subtree to be examinated; it must be a tree_reindex
    */
   void recursive_examinate(const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_visited);

   /**
    * Examinate recursively the tree to detect system types and system variables
    * @param curr_tn is the root of the subtree to be examinated; it must not be a tree_reindex
    * @param index is the index of the tree_node
    */
   void recursive_examinate(const tree_nodeRef& curr_tn, const unsigned int index, CustomUnorderedSet<unsigned int>& already_visited);

   /**
    * Check if an header is a system header
    * @param include is the header file
    */
   bool is_system_include(std::string include) const;

   /**
    * Build the include map, the function rename map and library system sets
    */
   void build_include_structures();

   /**
    * Given the string stored in a srcp of the raw return the correct include name
    * @param include is the string of srcp to be checked
    * @param real_name is the string fixed
    */
   void getRealInclName(const std::string& include, std::string& real_name) const;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   CheckSystemType(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~CheckSystemType() override;

   /**
    * Adds the system_flag to the tree_node's of variables and types when necessary
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
