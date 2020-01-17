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
 * @file SimulationTool.hpp
 * @brief Abstract class for a generic simulation tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _SIMULATION_TOOL_HPP_
#define _SIMULATION_TOOL_HPP_

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(SimulationTool);

/// STD include
#include <string>

/// STL include
#include <list>

class SimulationTool
{
 public:
   /// supported synthesis tools
   typedef enum
   {
      UNKNOWN = 0,
      MODELSIM,
      ISIM,
      XSIM,
      ICARUS,
      VERILATOR
   } type_t;

 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debug level of the class
   int debug_level;

   /// verbosity level of the class
   unsigned int output_level;

   /// generated script
   std::string generated_script;

   /// log file
   std::string log_file;

   /**
    * Performs the actual writing
    */
   virtual void GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list) = 0;

 public:
   /**
    * Constructor
    */
   explicit SimulationTool(const ParameterConstRef& Param);

   /**
    * Destructor
    */
   virtual ~SimulationTool();

   /**
    * Factory method
    */
   static SimulationToolRef CreateSimulationTool(type_t type, const ParameterConstRef& Param, const std::string& suffix);

   /**
    * Checks if the current specification can be executed or not
    */
   virtual void CheckExecution();

   /**
    * Generates the proper simulation script
    */
   virtual std::string GenerateSimulationScript(const std::string& top_filename, const std::list<std::string>& file_list);

   /**
    * Performs the simulation and returns the number of cycles
    */
   virtual unsigned long long int Simulate(unsigned long long& accum_cycles, unsigned int& n_testcases);

   /**
    * Determines the average number of cycles for the simulation(s)
    * @param accum_cycles is the total number of accumulated cycles
    * @param n_testcases is the number of testcases simulated
    */
   unsigned long long int DetermineCycles(unsigned long long int& accum_cycles, unsigned int& n_testcases);

   /**
    * Remove files created during simulation
    * FIXME: this should become pure virtual
    */
   virtual void Clean() const;
};
/// refcount definition of the class
typedef refcount<SimulationTool> SimulationToolRef;
#endif
