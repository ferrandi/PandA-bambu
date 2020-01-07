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
 * @file VIVADO_xsim_wrapper.hpp
 * @brief Wrapper to XSIM by XILINX VIVADO
 *
 * A object used to invoke XILINX XSIM tool
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XILINX_VIVADO_XSIM_WRAPPER_HPP
#define XILINX_VIVADO_XSIM_WRAPPER_HPP

#include "SimulationTool.hpp"
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
class xml_element;

#include "custom_map.hpp"
#include <string>
#include <vector>

#define XSIM_SUBDIR (Param->getOption<std::string>(OPT_output_directory) + std::string("/xsim"))

/**
 * @class VIVADO_xsim_wrapper
 * Main class for wrapping VIVADO XSIM Xilinx
 */
class VIVADO_xsim_wrapper : public SimulationTool
{
 private:
   /// suffix added to the XSIM dir
   std::string suffix;

   /**
    * @brief create the project file for VIVADO xelab
    * @param top_filename top entity/filename
    * @param file_list list of files to be simulated
    * @return name of the project file
    */
   std::string create_project_script(const std::string& top_filename, const std::list<std::string>& file_list);

   /**
    * Generates the proper simulation script
    */
   void GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list) override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   VIVADO_xsim_wrapper(const ParameterConstRef& Param, std::string suffix);

   /**
    * Destructor
    */
   ~VIVADO_xsim_wrapper() override;

   /**
    * Checks if the current specification can be executed or not
    */
   void CheckExecution() override;

   /**
    * Remove files created during simulation
    */
   void Clean() const override;
};
/// Refcount definition for the class
typedef refcount<VIVADO_xsim_wrapper> VIVADO_xsim_wrapperRef;

#endif
