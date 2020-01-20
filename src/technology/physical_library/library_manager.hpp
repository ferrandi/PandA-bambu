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
 * @file library_manager.hpp
 * @brief Class specification of the manager for each library.
 *
 * This class specifies the library_manager
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _LIBRARY_MANAGER_HPP
#define _LIBRARY_MANAGER_HPP

#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"
#include "config_HAVE_LIBRARY_COMPILER.hpp"

#include "boost/lexical_cast.hpp"
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(target_device);
/// RefCount type definition of the library_manager class structure
REF_FORWARD_DECL(library_manager);
/// RefCount type definition of the technology node datastructure
REF_FORWARD_DECL(technology_node);
/// RefCount type definition of the class containing all the parameters
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(attribute);
enum class TargetDevice_Type;
class xml_element;
//@}

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <string>
#include <vector>

struct attribute
{
 public:
   typedef enum
   {
      FLOAT64 = 0,
      BOOLEAN,
      INT32,
      STRING
   } value_t;

 private:
   std::string content;

   value_t value_type{FLOAT64};

   std::vector<attributeRef> content_list;

 public:
   attribute(const value_t _value_type, std::string _content);

   attribute(const std::string& _value_type, std::string _content);

   explicit attribute(const std::vector<attributeRef>& _content);

   std::string get_value_type_str() const;

   std::string get_content_str() const;

   bool has_list() const;

   unsigned int get_value_type() const;

   template <class G>
   G get_content() const
   {
      return boost::lexical_cast<G>(content);
   }

   const std::vector<attributeRef>& get_content_list() const
   {
      return content_list;
   }

   static void xload(const xml_element* node, std::vector<std::string>& ordered_attributes, std::map<std::string, attributeRef>& attributes);

   void xwrite(xml_element* xml_node, const std::string& name);
};
typedef refcount<attribute> attributeRef;

/**
 * This class manages the specific library structure.
 */
class library_manager
{
 public:
   /// typedef for the identification of the functional units contained into the library
   typedef std::map<std::string, technology_nodeRef> fu_map_type;

   /**
    * @name Library output formats
    */
   //@{
   /// available information for the library
   typedef enum
   {
      XML,
#if HAVE_FROM_LIBERTY
      LIBERTY,
#endif
#if HAVE_EXPERIMENTAL
      LEF,
#endif
#if HAVE_LIBRARY_COMPILER
      DB,
#endif
   } info_t;
   //@}

 private:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// string identifier of the library
   std::string name;

   /// datastructure to identify the units that are contained into the library
   fu_map_type fu_map;

   std::vector<std::string> ordered_attributes;

   /// attributes of the library
   std::map<std::string, attributeRef> attributes;

   /// files that provide information about the library
   std::map<unsigned int, std::string> info;

   /// flag to check if the library is standard (i.e., provided in input) or virtual
   bool is_std;

   /**
    * Set the default attributes for the library
    */
   void set_default_attributes();

   CustomOrderedSet<std::string> dont_use;

 public:
   /**
    * @name Constructors and destructors.
    */
   //@{
   /// Constructor.
   library_manager(ParameterConstRef Param, bool std = true);

   library_manager(std::string library_name, ParameterConstRef Param, bool std = true);

   /// Destructor.
   ~library_manager();
   //@}

   /**
    * Check if the library is virtual or not
    * @return true if the library is virtual, false otherwise (i.e., it is a standard library or, however, a library already characterized)
    */
   bool is_virtual() const;

   /**
    * Set a cell to be not used
    */
   void set_dont_use(const std::string& name);

   /**
    * Set a cell to be used
    */
   void remove_dont_use(const std::string& name);

   /**
    * Return the cells not to be used for the synthesis
    */
   CustomOrderedSet<std::string> get_dont_use_cells() const;

   size_t get_dont_use_num() const;

   void set_info(unsigned int type, const std::string& information);

   bool is_info(unsigned int type) const;

   std::string get_info(const info_t type, const TargetDevice_Type dv_type);

   void erase_info();

   static void xload(const xml_element* node, const library_managerRef& LM, const ParameterConstRef& Param, const target_deviceRef& device);

   void xwrite(xml_element* rootnode, TargetDevice_Type dv_type);

   std::string get_library_name() const;

   void add(const technology_nodeRef& node);

   void update(const technology_nodeRef& node);

   bool is_fu(const std::string& name) const;

   technology_nodeRef get_fu(const std::string& name) const;

   void remove_fu(const std::string& name);

   size_t get_gate_count() const;

   /**
    * Return the list of the resources contained into the given library
    * @return a datastructure that maps the name of the cells contained into the library with the related technology_node's
    */
   const fu_map_type& get_library_fu() const
   {
      return fu_map;
   }
};

typedef refcount<library_manager> library_managerRef;
typedef refcount<const library_manager> library_managerConstRef;

#endif
