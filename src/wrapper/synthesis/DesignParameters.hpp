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
 * @file DesignParameters.hpp
 * @brief This file contains the definition of the parameters for the synthesis tools
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _DESIGN_PARAMETERS_HPP_
#define _DESIGN_PARAMETERS_HPP_

/// Autoheader include
#include "config_HAVE_DESIGN_COMPILER.hpp"
#include "config_HAVE_FORMALITY.hpp"
#include "config_HAVE_IPXACT_BUILT.hpp"
#include "config_HAVE_LIBRARY_COMPILER.hpp"
#include "config_HAVE_LIBRARY_CREATOR.hpp"

#include "custom_map.hpp"
#include <string>

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(DesignParameters);

#include "exceptions.hpp"
#define SCRIPT_FILENAME "__script_filename__"

struct DesignParameters
{
   /// Name of the component
   std::string component_name;

   /// Name of the flow
   std::string chain_name;

   /// Parameters map type
   typedef std::map<std::string, std::string> map_t;

   /// Map between the name of the parameter and the corresponding string-based value
   map_t parameter_values;

#if HAVE_IPXACT_BUILT
   /**
    * Parses an XML-based design configuration in IP-XACT format
    */
   void xload_design_configuration(const ParameterConstRef Param, const std::string& xml_file);
#endif

   /**
    * Returns a clone of the current parameter configuration
    */
   inline DesignParametersRef clone() const
   {
      DesignParametersRef params(new DesignParameters);
      params->component_name = this->component_name;
      params->chain_name = this->chain_name;
      params->parameter_values = this->parameter_values;
      return params;
   }

   /**
    * Assigns a value to a saved parameter. In case the parameter has not been defined
    * before, it creates a new parameter, or if checking only for existing values, an
    * exception is thrown.
    *
    * @param name Parameter name.
    * @param value Parameter value.
    * @param checkExisting Check if the parameter name is already defined.
    */
   inline void assign(const std::string& name, const std::string& value, bool checkExisting)
   {
      if(checkExisting && parameter_values.find(name) == parameter_values.end())
      {
         THROW_ERROR("Parameter \"" + name + "\" not yet defined");
      }
      else
         parameter_values[name] = value;
   }

   /**
    * Returns the value associated with a parameter name
    *
    * @param name Parameter name.
    * @return a string representing the parameter value.
    */
   inline std::string get_value(const std::string& name) const
   {
      if(parameter_values.find(name) == parameter_values.end())
      {
         THROW_ERROR("Parameter \"" + name + "\" not yet defined");
      }
      return parameter_values.find(name)->second;
   }
};
/// refcount definition of the class
typedef refcount<DesignParameters> DesignParametersRef;

#endif
