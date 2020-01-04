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
 * @file quartus_report_wrapper.hpp
 * @brief Wrapper to invoke quartus_report tool by Altera
 *
 * A object used to invoke quartus_report tool by Altera
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef QUARTUS_REPORT_WRAPPER_HPP
#define QUARTUS_REPORT_WRAPPER_HPP

/// Superclass include
#include "AlteraWrapper.hpp"

#define QUARTUS_REPORT_TOOL_ID "quartus_sta"
#define QUARTUS_REPORT_TOOL_EXEC "quartus_sta"

/**
 * @class QuartusReportWrapper
 * Main class for wrapping quartus_report tool by Altera
 */
class QuartusReportWrapper : public AlteraWrapper
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
   QuartusReportWrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~QuartusReportWrapper() override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;
};
#endif
