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
 * @file lattice_flow_wrapper.hpp
 * @brief Wrapper to invoke lattice_flow tool by Lattice
 *
 * A object used to invoke lattice_flow tool by Lattice
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef LATTICE_LATTICE_FLOW_HPP
#define LATTICE_LATTICE_FLOW_HPP

#include "LatticeWrapper.hpp"

#define PARAM_lattice_outdir "lattice_outdir"
#define LATTICE_FLOW_TOOL_ID "lattice_flow"
#define LATTICE_FLOW_TOOL_EXEC "diamondc"

/**
 * @class lattice_flow_wrapper
 * Main class for wrapping lattice_flow tool by Lattice
 */
class lattice_flow_wrapper : public LatticeWrapper
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
   lattice_flow_wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~lattice_flow_wrapper() override;
};
/// Refcount definition for the class
typedef refcount<lattice_flow_wrapper> lattice_flow_wrapperRef;

#endif
