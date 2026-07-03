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
 * @file library_manager.hpp
 * @brief Class specification of the manager for each library.
 *
 * This class specifies the library_manager
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef _LIBRARY_MANAGER_HPP
#define _LIBRARY_MANAGER_HPP

#include "refcount.hpp"

#include <boost/lexical_cast.hpp>

REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(technology_node);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(attribute);
class xml_element;

#include "custom_set.hpp"
#include <map>
#include <string>
#include <vector>

struct attribute
{
 public:
   using value_t = enum { FLOAT64 = 0, BOOLEAN, INT32, STRING };

 private:
   std::string content{};

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

   static void xload(const xml_element* EnodeC, std::vector<std::string>& ordered_attributes,
                     std::map<std::string, attributeRef>& attributes);

   void xwrite(xml_element* xml_node, const std::string& name);
};
using attributeRef = refcount<attribute>;

/**
 * This class manages the specific library structure.
 */
class library_manager
{
 public:
   /// typedef for the identification of the functional units contained into the library
   using fu_map_type = std::map<std::string, technology_nodeRef>;

   /// available information for the library
   using info_t = enum {
      XML,
   };

 private:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// string identifier of the library
   std::string name;

   /// data-structure to identify the units that are contained into the library
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
   library_manager(ParameterConstRef Param, bool std = true);

   library_manager(std::string library_name, ParameterConstRef Param, bool std = true);

   /**
    * Check if the library is virtual or not
    * @return true if the library is virtual, false otherwise (i.e., it is a standard library or, however, a library
    * already characterized)
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

   void erase_info();

   static void xload(const xml_element* node, const library_managerRef& LM, const ParameterConstRef& Param);

   void xwrite(xml_element* rootnode);

   std::string get_library_name() const;

   void add(const technology_nodeRef& node);

   void update(const technology_nodeRef& node);

   bool is_fu(const std::string& name) const;

   technology_nodeRef get_fu(const std::string& name) const;

   void remove_fu(const std::string& name);

   size_t get_gate_count() const;

   /**
    * Return the list of the resources contained into the given library
    * @return a data structure that maps the name of the cells contained into the library with the related
    * technology_node's
    */
   const fu_map_type& get_library_fu() const
   {
      return fu_map;
   }
};

using library_managerRef = refcount<library_manager>;
using library_managerConstRef = refcount<const library_manager>;

#endif
