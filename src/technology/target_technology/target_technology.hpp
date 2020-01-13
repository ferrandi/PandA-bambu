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
 * @file target_technology.hpp
 * @brief Class used to represent a generic target technology
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef _TARGET_TECHNOLOGY_HPP_
#define _TARGET_TECHNOLOGY_HPP_

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"

#include "custom_map.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"
#include <boost/lexical_cast.hpp>
#include <string>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(target_technology);
class xml_element;

class target_technology
{
 public:
   /// target technologies currently implemented
   typedef enum
   {
#if HAVE_CMOS_BUILT
      CMOS = 0, /// integrated circuits through CMOS technology
#endif
      FPGA = 1 /// FPGA devices
   } target_t;

 protected:
   /// Class containing all the parameters
   const ParameterConstRef Param;

   /// Technology type
   target_t type;

   /// Map containing all the parameters of the technology. All the values are stored as strings and they have correctly converted through the get_parameter template.
   std::map<std::string, std::string> parameters;

   /**
    * Method to read an XML file and identify the section containing technology-related parameters
    * @param node is the root node for the analysis
    */
   void xload(const xml_element* node);

   /**
    * Method to read the part of the XML file related to technology parameters
    * @param tech_xml is the node where the analysis will be performed
    */
   void xload_technology_parameters(const xml_element* tech_xml);

 public:
   /**
    * Constructor.
    */
   explicit target_technology(const ParameterConstRef& param);

   /**
    * Destructor.
    */
   virtual ~target_technology();

   /**
    * Initializes the target technology based on the given parameters
    */
   virtual void initialize() = 0;

   /**
    * Factory method. Creates the datastructure of the given type
    * @param type is the type of the target technology to be created
    * @param param is the datastructure containing all the parameters
    * @return a reference to the specialization of the target technology datastructure
    */
   static target_technologyRef create_technology(const target_t type, const ParameterConstRef& param);

   /**
    * Returns the type of the technology currently implemented.
    * @return the type of the technology
    */
   target_t get_type() const;

   /**
    * Returns the type of the technology currently implemented in a string format.
    * @return a string representing the type of the technology
    */
   virtual std::string get_string_type() const = 0;

   /**
    * Returns the value of the specified parameter, if any. Otherwise, it throws an exception.
    * @param key it is the string identifier of the parameter to be returned
    */
   template <typename G>
   G get_parameter(const std::string& key) const
   {
      if(parameters.find(key) == parameters.end())
         THROW_ERROR("Parameter \"" + key + "\" not found in target technology parameters' list");
      return boost::lexical_cast<G>(parameters.find(key)->second);
   }

   /**
    * Sets the value of the specified parameter
    */
   template <typename G>
   void set_parameter(const std::string& key, G value)
   {
      parameters[key] = boost::lexical_cast<std::string>(value);
   }

   /**
    * Returns true if there is a value associated with the specified string identifier, false otherwise.
    * @param key it is the string identifier of the parameter to be searched
    */
   bool is_parameter(const std::string& key) const
   {
      return parameters.find(key) != parameters.end();
   }
};

/// refcount definition for the class
typedef refcount<target_technology> target_technologyRef;

#endif
