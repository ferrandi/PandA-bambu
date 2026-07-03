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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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

   std::map<std::string, std::string> backends;

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
   virtual ~generic_device() = default;

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

   bool has_backend(const std::string& backend_id) const
   {
      return backends.find(backend_id) != backends.end();
   }

   const std::string& get_backend(const std::string& backend_id) const
   {
      return backends.at(backend_id);
   }

   /**
    * Returns the technology manager
    */
   technology_managerRef get_technology_manager() const;

   /**
    * Factory method.
    * @param Param is the global parameter class
    * @param TM is technology manager
    */
   static generic_deviceRef factory(const ParameterConstRef& Param, const technology_managerRef& TM);
};
/// refcount definition for the class
using generic_deviceRef = refcount<generic_device>;

#endif
