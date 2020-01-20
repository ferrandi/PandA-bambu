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
 * @file VerilatorWrapper.hpp
 * @brief Wrapper to Verilator simulator
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VERILATOR_WRAPPER_HPP
#define VERILATOR_WRAPPER_HPP

#include "SimulationTool.hpp"
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);

#include <string>

/**
 * @class VerilatorWrapper
 * Main class for wrapping the Icarus verilog compiler.
 */
class VerilatorWrapper : public SimulationTool
{
   /// suffix added to the SIM dir
   std::string suffix;

   /**
    * Generates the proper simulation script
    */
   void GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list) override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   VerilatorWrapper(const ParameterConstRef& Param, std::string suffix);

   /**
    * Destructor
    */
   ~VerilatorWrapper() override;

   /**
    * Checks if the current specification can be executed or not
    */
   void CheckExecution() override;

   /**
    * Remove files created during simulation
    */
   void Clean() const override;
};
/// Refcount definition for the VerilatorWrapper class
typedef refcount<VerilatorWrapper> VerilatorWrapperRef;

#endif
