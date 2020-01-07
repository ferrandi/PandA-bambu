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
 * @file nxpython_flow_wrapper.hpp
 * @brief Wrapper to invoke nxpython_flow tool by NANOXPLORE
 *
 * A object used to invoke nxpython_flow tool by NANOXPLORE
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef NANOXPLORE_NXPYTHON_FLOW_HPP
#define NANOXPLORE_NXPYTHON_FLOW_HPP

#include "NanoXploreWrapper.hpp"

#define NXPYTHON_FLOW_TOOL_ID "nxpython_flow"
#define NXPYTHON_FLOW_TOOL_EXEC "nxpython"

/**
 * @class nxpython_flow_wrapper
 * Main class for wrapping nxpython_flow tool by NANOXPLORE
 */
class nxpython_flow_wrapper : public NanoXploreWrapper
{
 protected:
   /**
    * Evaluates the design variables
    */
   void EvaluateVariables(const DesignParametersRef dp) override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   nxpython_flow_wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~nxpython_flow_wrapper() override;
};
/// Refcount definition for the class
typedef refcount<nxpython_flow_wrapper> nxpython_flow_wrapperRef;

#endif
