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
 * @file pragma_manager.hpp
 * @brief Manager for pragma annotations.
 *
 * A object for manage information about pragma directives in a C/C++ program.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PRAGMA_MANAGER_HPP
#define PRAGMA_MANAGER_HPP

/// Autoheader include
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_HAVE_TASK_GRAPHS_BUILT.hpp"

/// graph include
#include "graph.hpp"

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
CONSTREF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Loop);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(PartitioningManager);
REF_FORWARD_DECL(TaskGraphManager);
REF_FORWARD_DECL(TaskGraphEstimator);
REF_FORWARD_DECL(tree_manager);
//@}

/**
 * @class pragma_manager
 * This class is used to manage the pragma directives found in the source code.
 */
class pragma_manager
{
 protected:
   /// The application manager
   /// NOTE: this is equivalente to a weakrefcount since deleter should be null
   const application_managerRef application_manager;

   const tree_managerRef TM;

   CustomOrderedSet<std::string> BlackBoxFunctions;

   std::map<std::string, CustomUnorderedSet<std::string>> FunctionCallPragmas;

   /// Function defintion pragmas
   std::map<std::string, std::list<std::string>> function_definition_pragmas;

   std::map<unsigned int, std::string> GenericPragmas;

   /// Set of input parameters
   const ParameterConstRef param;

   /// The debug level
   int debug_level;

 public:
   /**
    * The possible openmp pragmas
    * Note that sections has to go before section but after parallel sections otherwise recognition would be wrong
    */
   enum OmpPragmaType
   {
      OMP_ATOMIC = 0,
      OMP_BARRIER,
      OMP_CRITICAL,
      OMP_DECLARE_SIMD,
      OMP_FOR,
      OMP_PARALLEL_FOR,
      OMP_PARALLEL_SECTIONS,
      OMP_PARALLEL,
      OMP_SECTIONS,
      OMP_SECTION,
      OMP_SIMD,
      OMP_TARGET,
      OMP_TASK,
      OMP_UNKNOWN
   };

   /// The list of omp directive keywords
   static const std::string omp_directive_keywords[OMP_UNKNOWN];

   /**
    * Constructor
    * @param application_manager is the application manager
    * @param param is the set of the input parameters
    */
   pragma_manager(const application_managerRef application_manager, const ParameterConstRef param);

   /**
    * Destructor
    */
   virtual ~pragma_manager();

   /**
    * Check if the datastructure information are compliant with the pragma reference manual
    * @return true if the datastructure is compliant, false otherwise
    */
   bool checkCompliant() const;

   bool isBlackBox(const std::string& name) const;

   /**
    * Return the pragmas associated with a function definition
    * @param function_name is the name of the function to be considered
    */
   const std::list<std::string> GetFunctionDefinitionPragmas(const std::string& function_name) const;

   CustomUnorderedSet<std::string> getFunctionCallPragmas(const std::string& Name) const;

   /**
    * Add a set of definition pragmas to a function
    * @param function_name is the name of the function
    * @param is the set of pragmas to be added
    */
   void AddFunctionDefinitionPragmas(const std::string& name, const CustomUnorderedSet<std::string>& pragmas);

   void addFunctionCallPragmas(const std::string& Name, const CustomUnorderedSet<std::string>& Pragmas);

   unsigned int addBlackBoxPragma(const std::string& function_name);

   void setGenericPragma(unsigned int number, const std::string& line);

   std::string getGenericPragma(unsigned int number) const;

   /**
    * Create a simd openmp pragma starting from the line containing it
    * @param line is the string containing the pragma
    * @return the index of the created pragma node
    */
   unsigned int AddOmpSimdPragma(const std::string& line) const;

   /**
    * Check if a omp for pragma is associated with the loop
    * @param app_man is the application manager
    * @param function_index is the index of the function
    * @param bb_vertex is the basic block to which for operation belongs
    * @return true if there is an associated pragma
    */
   bool CheckOmpFor(const application_managerConstRef app_man, const unsigned int function_index, const vertex bb_operation_vertex) const;

   /**
    * Check if a omp for pragma is associated with the loop; if yes, add the gimple_for
    * @param function_index is the index of the function
    * @param bb_vertex is the basic block to which for operation belongs
    */
   void CheckAddOmpFor(const unsigned int function_index, const vertex bb_operation_vertex);

   /**
    * Check if a omp simd pragma is associated with the loop; if yes, add information to the loop
    * @param function_index is the index of the function
    * @param bb_vertex is the basic block to which for operation belongs
    */
   void CheckAddOmpSimd(const unsigned int function_index, const vertex bb_operation_vertex);

   /**
    * Get mapping of a function given by the pragma
    * @param function_name is the name of the function
    */
   std::string get_mapping(const std::string& function_name) const;

   /**
    * Returns the identifier corresponding to an openmp directive
    * @param directive is the string to be considered
    * @return the corresponding identifier
    */
   static OmpPragmaType GetOmpPragmaType(const std::string& directive);

   /**
    * Extract clauses associated with a pragma
    * @param clauses_list is the string containing the clauses
    * @return the extracted clauses
    */
   CustomUnorderedMapUnstable<std::string, std::string> ExtractClauses(const std::string& clauses_list) const;
};

/// Refcount definition for the class
typedef refcount<const pragma_manager> pragma_managerConstRef;
typedef refcount<pragma_manager> pragma_managerRef;

#endif
