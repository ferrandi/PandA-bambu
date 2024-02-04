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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file generic_device.hpp
 * @brief Generic device description
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef GENERIC_DEVICE_HPP
#define GENERIC_DEVICE_HPP

#include "exceptions.hpp"
#include "refcount.hpp"
#include <boost/lexical_cast.hpp>
#include <map>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(BackendFlow);
class xml_element;

/// generic device description
class generic_device
{
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// technology manager
   const technology_managerRef TM;

   /// Map of the technology parameter
   std::map<std::string, std::string> parameters;

   /// map between bash variables and values
   std::map<std::string, std::string> vars;

   /// The debug level
   int debug_level;

   /**
    * XML load of device parameters
    * @param dev_xml is the root xml node
    */
   void xload_device_parameters(const xml_element* dev_xml);

 public:
   /**
    * Constructor of the class
    * @param Param is the reference to the class that contains all the parameters
    * @param TM is the reference to the current technology library
    */
   generic_device(const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Destructor of the class
    */
   virtual ~generic_device();

   /**
    * XML load specialization
    * @param node is the device root node
    */
   void xload(const xml_element* node);

   /**
    * XML write specialization
    * @param node is the device root node
    */
   void xwrite(xml_element* node);

   /**
    * Load device characteristics
    */
   void load_devices();

   /**
    * Factory method.
    * @param Param is the global parameter class
    * @param TM is technology manager
    */
   static generic_deviceRef factory(const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Returns a parameter by key.
    * @param key is the parameter ID
    */
   template <typename G>
   G get_parameter(const std::string& key) const
   {
      if(parameters.find(key) == parameters.end())
      {
         THROW_ERROR("Parameter \"" + key + "\" not found in target device parameters' list");
      }
      return boost::lexical_cast<G>(parameters.find(key)->second);
   }

   /**
    * Sets the value of the parameter
    */
   template <typename G>
   void set_parameter(const std::string& key, G value)
   {
      parameters[key] = std::to_string(value);
   }

   /**
    * Check if parameter exist
    * @param key is the parameter ID
    */
   bool has_parameter(const std::string& key) const
   {
      return parameters.find(key) != parameters.end();
   }

   const std::map<std::string, std::string>& get_device_bash_vars() const
   {
      return vars;
   }

   /**
    * Returns the technology manager
    */
   technology_managerRef get_technology_manager() const;
};
/// refcount definition for the class
using generic_deviceRef = refcount<generic_device>;

#endif
