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
 * @file SynopsysWrapper.hpp
 * @brief Wrapper to synthesis tools by Synopsys
 *
 * A object used to invoke synthesis tools by Synopsys
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _SYNOPSYS_WRAPPER_HPP_
#define _SYNOPSYS_WRAPPER_HPP_

/// superclass include
#include "SynthesisTool.hpp"

class SynopsysWrapper : public SynthesisTool
{
 public:
   /// implemented wrappers
   typedef enum
   {
      UNDEFINED = 0,
#if HAVE_EXPERIMENTAL
      PRIME_TIME,
      FORMALITY,
      LIBRARY_COMPILER,
#endif
      DESIGN_COMPILER
   } wrapper_t;

   /**
    * Constructor
    * @param Param is the set of parameters
    * @param tool_exec is the name of the executable
    * @param output_dir is the directory where to save all the results
    * @param default_output_dir is the default output directory
    */
   SynopsysWrapper(const ParameterConstRef& Param, const std::string& tool_exec, const target_deviceRef& device, const std::string& output_dir, const std::string& default_output_dir);

   /**
    * Destructor
    */
   ~SynopsysWrapper() override;

   /**
    * Creates the proper configuration script
    */
   void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) override;

   /**
    * Returns the tool command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string toString(const xml_script_node_tRef node, const DesignParametersRef dp) const override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const override;

   /**
    * Factory method
    */
   static SynthesisToolRef CreateWrapper(wrapper_t type, const ParameterConstRef& Param, const target_deviceRef& _device, const std::string& output_dir);
};
/// Refcount definition for the class
typedef refcount<SynopsysWrapper> SynopsysWrapperRef;

#endif
