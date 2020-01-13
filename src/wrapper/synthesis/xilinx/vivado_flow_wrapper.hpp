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
 * @file vivado_flow_wrapper.hpp
 * @brief Wrapper to invoke vivado_flow tool by XILINX
 *
 * A object used to invoke vivado_flow tool by XILINX
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XILINX_VIVADO_FLOW_HPP
#define XILINX_VIVADO_FLOW_HPP

#include "XilinxWrapper.hpp"

#define VIVADO_FLOW_TOOL_ID "vivado_flow"
#define VIVADO_FLOW_TOOL_EXEC "vivado"

/**
 * @class vivado_flow_wrapper
 * Main class for wrapping vivado_flow tool by XILINX
 */
class vivado_flow_wrapper : public XilinxWrapper
{
 private:
   void create_sdc(const DesignParametersRef& dp);

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
   vivado_flow_wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~vivado_flow_wrapper() override;
};
/// Refcount definition for the class
typedef refcount<vivado_flow_wrapper> vivado_flow_wrapperRef;

#endif
