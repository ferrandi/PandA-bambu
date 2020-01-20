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
 * @file IcarusWrapper.hpp
 * @brief Wrapper to Icarus.
 *
 * A object used to invoke the Icarus compiler on verilog files
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ICARUSWRAPPER_HPP
#define ICARUSWRAPPER_HPP

#include "config_HAVE_EXPERIMENTAL.hpp"

#include "SimulationTool.hpp"
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);

#include <string>

/**
 * @class IcarusWrapper
 * Main class for wrapping the Icarus verilog compiler.
 */
class IcarusWrapper : public SimulationTool
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
   IcarusWrapper(const ParameterConstRef& Param, std::string suffix);

   /**
    * Destructor
    */
   ~IcarusWrapper() override;

   /**
    * Checks if the current specification can be executed or not
    */
   void CheckExecution() override;

#if HAVE_EXPERIMENTAL
   /**
    * Compiles the specified verilog to check the execution
    */
   unsigned int compile_verilog(const std::string& FileName);

   /**
    * Parses a verilog file and converts it into the related XML description
    */
   unsigned int convert_to_xml(const std::string& SourceFileName, const std::string& LibraryName, const std::string& TargetFileName);
#endif
};
/// Refcount definition for the IcarusWrapper class
typedef refcount<IcarusWrapper> IcarusWrapperRef;

#endif
