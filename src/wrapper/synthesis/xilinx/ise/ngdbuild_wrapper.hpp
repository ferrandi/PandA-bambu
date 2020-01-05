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
 * @file ngdbuild_wrapper.hpp
 * @brief Wrapper to ngdbuild by XILINX
 *
 * A object used to invoke XILINX ngd creation tool
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _XILINX_NGDBUILD_WRAPPER_HPP_
#define _XILINX_NGDBUILD_WRAPPER_HPP_

#include "XilinxWrapper.hpp"

#define NGDBUILD_TOOL_ID "ngdbuild"

#define PARAM_ngdbuild_tmpdir "ngdbuild_tmpdir"

/**
 * @class ngdbuild_wrapper
 * Main class for wrapping ISE tools by Xilinx
 */
class ngdbuild_wrapper : public XilinxWrapper
{
 protected:
   /**
    * Initializes the reserved variables
    */
   void init_reserved_vars() override;

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
   ngdbuild_wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~ngdbuild_wrapper() override;
};
/// Refcount definition for the class
typedef refcount<ngdbuild_wrapper> ngdbuild_wrapperRef;

#endif
