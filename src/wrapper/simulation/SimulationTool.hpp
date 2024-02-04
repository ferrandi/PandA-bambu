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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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

#include <list>
#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(SimulationTool);
class SimulationTool
{
 public:
   /// supported synthesis tools
   using type_t = enum { UNKNOWN = 0, MODELSIM, XSIM, VERILATOR };

   static type_t to_sim_type(const std::string& str);

 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debug level of the class
   int debug_level;

   /// verbosity level of the class
   unsigned int output_level;

   const std::string top_fname;

   /// generated script
   std::string generated_script;

   /// log file
   std::string log_file;

   /// comma separated list of include dirs
   const std::string inc_dirs;

   /**
    * Performs the actual writing
    */
   virtual std::string GenerateScript(std::ostream& script, const std::string& top_filename,
                                      const std::list<std::string>& file_list) = 0;

   std::string GenerateLibraryBuildScript(std::ostream& script, const std::string& libtb_filename,
                                          std::string& beh_cflags) const;

 public:
   /**
    * Constructor
    */
   explicit SimulationTool(const ParameterConstRef& Param, const std::string& top_fname, const std::string& inc_dirs);

   /**
    * Destructor
    */
   virtual ~SimulationTool();

   /**
    * Factory method
    */
   static SimulationToolRef CreateSimulationTool(type_t type, const ParameterConstRef& Param, const std::string& suffix,
                                                 const std::string& top_fname, const std::string& inc_dirs);

   /**
    * Checks if the current specification can be executed or not
    */
   virtual void CheckExecution();

   /**
    * Generates the proper simulation script
    */
   virtual std::string GenerateSimulationScript(const std::string& top_filename, std::list<std::string> file_list);

   /**
    * Performs the simulation and returns the number of cycles
    */
   virtual void Simulate(unsigned long long& accum_cycles, unsigned long long& n_testcases);

   /**
    * Determines the average number of cycles for the simulation(s)
    * @param accum_cycles is the total number of accumulated cycles
    * @param n_testcases is the number of testcases simulated
    */
   void DetermineCycles(unsigned long long int& accum_cycles, unsigned long long& n_testcases);

   /**
    * Remove files created during simulation
    * FIXME: this should become pure virtual
    */
   virtual void Clean() const;
};
/// refcount definition of the class
using SimulationToolRef = refcount<SimulationTool>;
#endif
