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
 * @file target_device.hpp
 * @brief This (abstract) class represents a generic target device for the synthesis process
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef _TARGET_DEVICE_HPP_
#define _TARGET_DEVICE_HPP_

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"

#include "custom_map.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"
#include <boost/lexical_cast.hpp>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(target_technology);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(BackendFlow);
class xml_element;

/// definition of the supported types for target devices
enum class TargetDevice_Type
{
#if HAVE_CMOS_BUILT
   IC = 0, //! the target device is an integrated circuit
#endif
   FPGA //! the target device is a reconfigurable component (e.g., an FPGA)
};

class target_device
{
 protected:
   /// define the type of the target device
   TargetDevice_Type device_type;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// class containing all the elements of the technology library
   const technology_managerRef TM;

   /// Reference to the target technology which will be used in the target device
   target_technologyRef target;

   /// Map containing all the parameters of the technology. All the values are stored as strings and they have correctly converted through the get_parameter template.
   std::map<std::string, std::string> parameters;

   /// Height of the core area dedicated for the design (in um).
   double core_height;

   /// Width of the core area dedicated for the design (in um).
   double core_width;

   /// The debug level
   int debug_level;

   /**
    * Method to read the part of the XML file related to technology parameters
    * @param dev_xml is the node where the analysis will be performed
    */
   void xload_device_parameters(const xml_element* dev_xml);

 public:
   /**
    * Constructor of the class
    * @param Param is the reference to the class that contains all the parameters
    * @param TM is the reference to the current technology library
    * @param target is the reference to the current target technology
    */
   target_device(const ParameterConstRef& Param, const technology_managerRef& TM, const TargetDevice_Type type);

   /**
    * Destructor of the class
    */
   virtual ~target_device();

   /**
    * Method to read an XML file and identify the section containing device-related parameters
    * @param node is the root node for the analysis
    */
   void xload(const target_deviceRef& device, const xml_element* node);

   /**
    * Method to write an XML node
    * @param node is the node for writing the information
    */
   virtual void xwrite(xml_element* node) = 0;

   /**
    * Load all data specific for a given technology
    */
   virtual void load_devices(const target_deviceRef device) = 0;

   /**
    * Factory method. It creates the proper specialization based on the given type
    * @param type is the type of the target device that has to be created
    * @param Param is the class containing all the parameters
    * @param TM is the reference to the current technology library
    * @param target is the reference to the current target technology
    */
   static target_deviceRef create_device(const TargetDevice_Type type, const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Returns the value of the specified parameter, if any. Otherwise, it throws an exception.
    * @param key it is the string identifier of the parameter to be returned
    */
   template <typename G>
   G get_parameter(const std::string& key) const
   {
      if(parameters.find(key) == parameters.end())
         THROW_ERROR("Parameter \"" + key + "\" not found in target device parameters' list");
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
   bool has_parameter(const std::string& key) const
   {
      return parameters.find(key) != parameters.end();
   }

   /**
    * Returns the height of the area dedicated for the implementation
    */
   double get_core_height() const;

   /**
    * Returns the width of the area dedicated for the implementation
    */
   double get_core_width() const;

   /**
    * Initializes the target device based on the given parameters
    */
   virtual void initialize() = 0;

   /**
    * Returns the target technology datastructure
    */
   target_technologyRef get_target_technology() const;

   /**
    * Returns the technology manager
    */
   technology_managerRef get_technology_manager() const;

   /**
    * return the type of target device
    */
   TargetDevice_Type get_type() const
   {
      return device_type;
   }
};
/// refcount definition for the class
typedef refcount<target_device> target_deviceRef;

#endif
