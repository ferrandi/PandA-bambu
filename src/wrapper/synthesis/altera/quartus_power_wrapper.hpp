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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#ifndef QUARTUS_POWER_WRAPPER_HPP
#define QUARTUS_POWER_WRAPPER_HPP

/// Superclass include
#include "AlteraWrapper.hpp"

#define QUARTUS_POWER_TOOL_ID "quartus_pow"
#define QUARTUS_POWER_TOOL_EXEC "quartus_pow"

/**
 * @class QuartusPowerWrapper
 * Main class for wrapping quartus_pow tool by Altera
 */
class QuartusPowerWrapper : public AlteraWrapper
{
 protected:
   /**
    * Evaluates the design variables
    */
   void EvaluateVariables(const DesignParametersRef dp) override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   QuartusPowerWrapper(const ParameterConstRef Param, const std::string& _output_dir, const target_deviceRef _device);

   /**
    * Destructor
    */
   ~QuartusPowerWrapper() override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;

   /**
    * Creates the proper configuration script
    */
   void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) override;
};
#endif
