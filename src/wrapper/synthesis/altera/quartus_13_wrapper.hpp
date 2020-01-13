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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file quartus_13_wrapper.hpp
 * @brief Wrapper to quartus 13.x and newer
 *
 * A object used to invoke synthesis tools by Altera
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef QUARTUS_13_WRAPPER_HPP
#define QUARTUS_13_WRAPPER_HPP

/// superclass include
#include "AlteraWrapper.hpp"

#define QUARTUS_13_SETUP_TOOL_ID "quartus_13_setup"
#define QUARTUS_13_FLOW_TOOL_ID "quartus_13_flow"
#define QUARTUS_13_FLOW_TOOL_EXEC "quartus_sh"

class Quartus13Wrapper : public AlteraWrapper
{
 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    * @param output_dir is the directory where to save all the results
    */
   Quartus13Wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~Quartus13Wrapper() override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;
};
#endif
