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
 * @file ISE_isim_wrapper.hpp
 * @brief Wrapper to ISIM by XILINX
 *
 * A object used to invoke XILINX ISIM tool
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XILINX_ISE_ISIM_WRAPPER_HPP
#define XILINX_ISE_ISIM_WRAPPER_HPP

#include "SimulationTool.hpp"
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
class xml_element;

#include "custom_map.hpp"
#include <string>
#include <vector>

#define DEFAULT_INTSTYLE INTSTYLE_SILENT
#define INTSTYLE_ISE "ise"
#define INTSTYLE_SILENT "silent"
#define INTSTYLE_XFLOW "xflow"

#define ISIM_SUBDIR (Param->getOption<std::string>(OPT_output_directory) + std::string("/isim"))

/**
 * @class ISE_isim_wrapper
 * Main class for wrapping ISE ISIM Xilinx
 */
class ISE_isim_wrapper : public SimulationTool
{
 private:
   /// suffix added to the ISIM dir
   std::string suffix;

   /**
    * Creates the project file for ISIM
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
   ISE_isim_wrapper(const ParameterConstRef& Param, std::string suffix);

   /**
    * Destructor
    */
   ~ISE_isim_wrapper() override;

   /**
    * Checks if the current specification can be executed or not
    */
   void CheckExecution() override;
};
/// Refcount definition for the class
typedef refcount<ISE_isim_wrapper> ISE_isim_wrapperRef;

#endif
