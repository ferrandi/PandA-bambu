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
 * @file technology_manager.hpp
 * @brief Class specification of the manager of the technology library data structures.
 *
 * This class specifies the technology_manager node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TECHNOLOGY_MANAGER_HPP
#define TECHNOLOGY_MANAGER_HPP
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"
#include "strong_typedef.hpp"

#include <ostream>
#include <string>
#include <vector>

#include "config_HAVE_CIRCUIT_BUILT.hpp"

/// working library.
#define WORK_LIBRARY std::string("work")
/// OpenMP library
#define OPENMP_LIBRARY std::string("OpenMP_library")
/// proxy library
#define PROXY_LIBRARY std::string("proxy_library")
/// interface library
#define INTERFACE_LIBRARY std::string("interface_library")
/// standard library where all built-in ports are defined.
#define LIBRARY_STD std::string("STD")
/// standard library where all standard HLS resources are defined
#define LIBRARY_STD_FU std::string("STD_FU")
/// standard library for parallel controller
#define LIBRARY_PC std::string("STD_PC")
/// standard library for dataflow interface modules
#define LIBRARY_STD_DATAFLOW std::string("STD_DATAFLOW")

/**
 * @name Forward declarations.
 */
//@{
/// RefCount type definition of the Parameter class structure
CONSTREF_FORWARD_DECL(Parameter);
/// RefCount type definition of the technology_manager class structure
CONSTREF_FORWARD_DECL(technology_manager);
/// RefCount type definition of the library_manager class structure for representing libraries
REF_FORWARD_DECL(library_manager);
using fileIO_istreamRef = refcount<std::istream>;
/// RefCount type definition of the structural_manager class structure
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(simple_indent);
/// forward decl of xml Element
class xml_element;
class allocation;
class mixed_hls;
struct TimeStamp;
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);
class functional_unit;

//@}

/**
 * This class manages the technology library structures.
 */
class technology_manager
{
 public:
   /**
    * @name Library output formats
    */
   //@{
   const static unsigned int XML;
   //@}

   /// definition of the type for identifying the libraries
   using library_map_type = CustomUnorderedMap<std::string, library_managerRef>;

 private:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// The debug level for this class
   int debug_level;

   /// map between library name and the corresponding data structure
   library_map_type library_map;

   ///(reverse) ordered list of libraries; it gives a priority ordering for searching the nodes
   std::vector<std::string> libraries;

   /// The builtin components
   CustomSet<std::string> builtins;

   /// Map function names to hardware module used for implementation
   CustomUnorderedMap<std::string, technology_nodeRef> function_fu;

   /**
    * Return the functional unit used to compute the setup hold time
    * @return the functional unit used to compute the setup hold time
    */
   const functional_unit* CGetSetupHoldFU() const;

 public:
   /**
    * @name Constructors and Destructors.
    */
   //@{
   /// Constructor.
   explicit technology_manager(const ParameterConstRef Param);

   /// Destructor.
   ~technology_manager();
   //@}

 private:
   /// friend definition for class allocation
   friend class allocation;

 public:
   /**
    * @name Factory functions. Used to build and fill the technology data structures
    */
   //@{
   /**
    * Add the given functional_unit to the specified library.
    * @param curr is the added element
    */
   void add(const technology_nodeRef curr, const std::string& Library);

#if HAVE_CIRCUIT_BUILT
   /**
    * Build a resource based on the given characteristics and structural representation.
    * @param is_builtin specifies if the resource is builtin or not
    */
   void add_resource(const std::string& Library, const std::string& fu_name,
                     const structural_managerRef CM = structural_managerRef(), const bool is_builtin = false);
#endif

   /**
    * Add an operation to the specified functional unit
    */
   technology_nodeRef add_operation(const std::string& Library, const std::string& fu_name,
                                    const std::string& operation_name);

   //@}

   /**
    * @name Query functions. Used to extract some information from the technology data structure.
    */
   //@{

   /**
    * Return the list of the libraries
    * @return a vector containing the identifiers of the libraries contained into the technology manager
    */
   const std::vector<std::string>& get_library_list() const
   {
      return libraries;
   }

   /**
    * Check if an operation can be implemented by a given component in a given library.
    * @param fu_name is the name of the component.
    * @param op_name is the name of the operation.
    * @param Library is library name where the unit is stored
    * @return true when the operation can be implemented, false otherwise.
    */
   bool can_implement(const std::string& fu_name, const std::string& op_name, const std::string& Library) const;

   /**
    * Return the reference to a component given its name.
    * @param fu_name is the name of the component.
    * @param Library is library name where the unit is stored
    * @return the reference to a component if found, else nullptr
    */
   technology_nodeRef get_fu(const std::string& fu_name, const std::string& Library) const;

   /**
    * Return the reference to a component given its name.
    * @param fu_name is the name of the component.
    * @param Library is library name where the unit is stored if found
    * @return the reference to a component if found, else nullptr
    */
   technology_nodeRef get_fu(const std::string& fu_name, std::string* Library = nullptr) const;

   /**
    * Return the higher priority library where the given component is stored
    * @param Name is the name of the component.
    * @return the identifier of the library where the unit is contained
    */
   std::string get_library(const std::string& Name) const;

   /**
    * Release the given library
    * @param Name is the name of the library to be erased.
    */
   void erase_library(const std::string& Name);

   /**
    * Check if a library is contained into the data structure
    * @param Name is the name of the library.
    * @return true if there is a data structure corresponding to the library name, false otherwise
    */
   bool is_library_manager(const std::string& Name) const;

   /**
    * Return the library data structure corresponding to the given library id
    * @param Name is the name of the library.
    * @return the reference to the data structure of the library
    */
   library_managerRef get_library_manager(const std::string& Name) const;

   /**
    * Return the initiation time for a given operation type and a given component.
    * @param fu_name is the name of the component.
    * @param op_name is the name of the operation.
    * @param res is true when there is the pair (fu_name, op_name), false otherwise.
    * @param Library is library name where the unit is stored
    * @return the initiation_time for (fu_name, op_name).
    */
   ControlStep get_initiation_time(const std::string& fu_name, const std::string& op_name,
                                   const std::string& Library) const;

   /**
    * Return the execution time for a given operation type and a given component.
    * @param fu_name is the name of the component.
    * @param op_name is the name of the operation.
    * @param res is true when there is the pair (fu_name, op_name), false otherwise.
    * @param Library is library name where the unit is stored
    * @return the execution time for (fu_name, op_name)
    */
   double get_execution_time(const std::string& fu_name, const std::string& op_name, const std::string& Library) const;

   /**
    * Return the area for a given component.
    * @param fu_name is the name of the component.
    * @param Library is library name where the unit is stored
    * @return the area for the library component
    */
   double get_area(const std::string& fu_name, const std::string& Library) const;

   /**
    * Return true if a component is builtin
    * @param component_name is the name of the component
    * @return true if the component is builtin
    */
   bool IsBuiltin(const std::string& component_name) const;

   //@}

   /**
    * @name XML functions.
    */
   //@{
   /**
    * Load a technology manager from an xml file.
    * @param node is a node of the xml tree.
    */
   void xload(const xml_element* node);

   /**
    * add library elements operation node to an xml tree.
    * @param rootnode is the root node at which the xml representation of the operation is attached.
    */
   void xwrite(xml_element* rootnode, const CustomOrderedSet<std::string>& libraries = CustomOrderedSet<std::string>());

   /**
    * @name Print functions.
    */
   //@{

   /**
    * Function that prints the class technology_manager.
    */
   void print(std::ostream& os) const;
   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const technology_manager& s)
   {
      s.print(os);
      return os;
   }
   /**
    * Friend definition of the << operator. Pointer version.
    */
   friend std::ostream& operator<<(std::ostream& os, const technology_managerRef& s)
   {
      if(s)
      {
         s->print(os);
      }
      return os;
   }
   //@}

   /**
    * Return the setup hold time
    * @return the setup hold time
    */
   double CGetSetupHoldTime() const;

   /**
    * Return the characterization timestamp of the setup hold time
    * @return the characterization timestamp of the setup hold time
    */
   TimeStamp CGetSetupHoldTimeStamp() const;

   /**
    * Return FU used to implement given function if any
    * @param fname function name
    * @return technology_nodeRef Functional unit with fname in the supported operations' set or nullptr
    */
   technology_nodeRef GetFunctionFU(const std::string& fname) const;
};

using technology_managerRef = refcount<technology_manager>;

#endif
