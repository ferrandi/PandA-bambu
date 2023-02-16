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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file NanoXploreWrapper.hpp
 * @brief Wrapper to synthesis tools by NanoXplore
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef _NANOXPLORE_WRAPPER_HPP_
#define _NANOXPLORE_WRAPPER_HPP_

/// superclass include
#include "SynthesisTool.hpp"

#include "refcount.hpp"
REF_FORWARD_DECL(target_device);

#define PARAM_sources_macro_list "sources_macro_list"

class NanoXploreWrapper : public SynthesisTool
{
 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    * @param tool_exec is the name of the executable
    * @param output_dir is the directory where to save all the results
    * @param default_output_dir is the default output directory
    */
   NanoXploreWrapper(const ParameterConstRef& Param, const std::string& tool_exec, const target_deviceRef& device,
                     const std::string& output_dir, const std::string& default_output_dir);

   /**
    * Destructor
    */
   ~NanoXploreWrapper() override;

   /**
    * Creates the proper configuration script
    */
   void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string toString(const xml_script_node_tRef node, const DesignParametersRef dp) const override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override = 0;
};
/// Refcount definition for the class
using NanoXploreWrapperRef = refcount<NanoXploreWrapper>;

#endif
