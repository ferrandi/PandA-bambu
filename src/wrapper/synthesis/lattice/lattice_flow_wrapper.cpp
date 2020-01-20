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
 * @file lattice_flow_wrapper.cpp
 * @brief Implementation of the wrapper to lattice_flow
 *
 * Implementation of the methods to wrap lattice_flow by Lattice
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "lattice_flow_wrapper.hpp"

#include "ToolManager.hpp"
#include "xml_script_command.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

// constructor
lattice_flow_wrapper::lattice_flow_wrapper(const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device) : LatticeWrapper(_Param, LATTICE_FLOW_TOOL_EXEC, _device, _output_dir, LATTICE_FLOW_TOOL_ID)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the LATTICE_FLOW wrapper...");
}

// destructor
lattice_flow_wrapper::~lattice_flow_wrapper() = default;

void lattice_flow_wrapper::EvaluateVariables(const DesignParametersRef dp)
{
   dp->assign(PARAM_lattice_outdir, output_dir, false);
   std::string top_id = dp->component_name;
   dp->parameter_values[PARAM_lattice_report] = output_dir + "/" + top_id + "_report.xml";
}
