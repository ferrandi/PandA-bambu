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
 * @file modelsimWrapper.hpp
 * @brief Wrapper to modelsim.
 *
 * A object used to invoke the modelsim compiler and simulator
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef MODELSIMWRAPPER_HPP
#define MODELSIMWRAPPER_HPP

#include "SimulationTool.hpp"
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);

/// STD include
#include <string>

/// STL include
#include <list>

/**
 * @class modelsimWrapper
 * Main class for wrapping the modesilm compiler and simulator.
 */
class modelsimWrapper : public SimulationTool
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
   modelsimWrapper(const ParameterConstRef& Param, std::string suffix);

   /**
    * Destructor
    */
   ~modelsimWrapper() override;

   /**
    * Checks if the current specification can be executed or not
    */
   void CheckExecution() override;

   /**
    * Remove files created during simulation
    */
   void Clean() const override;
};
/// Refcount definition for the modelsimWrapper class
typedef refcount<modelsimWrapper> modelsimWrapperRef;

#endif
