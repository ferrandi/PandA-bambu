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
 * @file SimulationTool.cpp
 * @brief Implementation of some methods for the interface with simulation tools
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "SimulationTool.hpp"
#include "config_HAVE_ASSERTS.hpp" // for HAVE_ASSERTS

#include "ToolManager.hpp"

#include "ISE_isim_wrapper.hpp"
#include "IcarusWrapper.hpp"
#include "VIVADO_xsim_wrapper.hpp"
#include "VerilatorWrapper.hpp"
#include "modelsimWrapper.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for Trimspaces
#include <cmath>

/// STL include
#include <vector>

SimulationTool::SimulationTool(const ParameterConstRef& _Param) : Param(_Param), debug_level(Param->getOption<int>(OPT_debug_level)), output_level(Param->getOption<unsigned int>(OPT_output_level))
{
}

SimulationTool::~SimulationTool() = default;

SimulationToolRef SimulationTool::CreateSimulationTool(type_t type, const ParameterConstRef& _Param, const std::string& suffix)
{
   switch(type)
   {
      case UNKNOWN:
         THROW_ERROR("Simulation tool not specified");
         break;
      case MODELSIM:
         return SimulationToolRef(new modelsimWrapper(_Param, suffix));
         break;
      case ISIM:
         return SimulationToolRef(new ISE_isim_wrapper(_Param, suffix));
         break;
      case XSIM:
         return SimulationToolRef(new VIVADO_xsim_wrapper(_Param, suffix));
         break;
      case ICARUS:
         return SimulationToolRef(new IcarusWrapper(_Param, suffix));
         break;
      case VERILATOR:
         return SimulationToolRef(new VerilatorWrapper(_Param, suffix));
         break;
      default:
         THROW_ERROR("Simulation tool currently not supported");
   }
   /// this point should never be reached
   return SimulationToolRef();
}

void SimulationTool::CheckExecution()
{
   /// for default, nothing to do
}

unsigned long long int SimulationTool::Simulate(unsigned long long int& accum_cycles, unsigned int& n_testcases)
{
   if(generated_script.empty())
   {
      THROW_ERROR("Simulation script not yet generated");
   }

   /// remove previous simulation results
   auto result_file = Param->getOption<std::string>(OPT_simulation_output);
   if(boost::filesystem::exists(result_file))
   {
      boost::filesystem::remove_all(result_file);
   }
   auto profiling_result_file = Param->getOption<std::string>(OPT_profiling_output);
   if(boost::filesystem::exists(profiling_result_file))
   {
      boost::filesystem::remove_all(profiling_result_file);
   }
   ToolManagerRef tool(new ToolManager(Param));
   tool->configure(generated_script, "");
   std::vector<std::string> parameters, input_files, output_files;
   tool->execute(parameters, input_files, output_files, Param->getOption<std::string>(OPT_output_temporary_directory) + "/simulation_output");

   if(!log_file.empty() && output_level == OUTPUT_LEVEL_VERBOSE)
   {
      CopyStdout(log_file);
   }

   return DetermineCycles(accum_cycles, n_testcases);
}

unsigned long long int SimulationTool::DetermineCycles(unsigned long long int& accum_cycles, unsigned int& n_testcases)
{
   unsigned long long int num_cycles = 0;
   unsigned int i = 0;
   auto result_file = Param->getOption<std::string>(OPT_simulation_output);
   auto profiling_result_file = Param->getOption<std::string>(OPT_profiling_output);
   if(!boost::filesystem::exists(profiling_result_file))
   {
      if(!boost::filesystem::exists(result_file))
      {
         if(output_level != OUTPUT_LEVEL_VERBOSE)
         {
            CopyStdout(log_file);
         }
         THROW_ERROR("The simulation does not end correctly");
      }
      std::ifstream res_file(result_file.c_str());
      if(res_file.is_open())
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "File \"" + result_file + "\" opened");
         while(!res_file.eof())
         {
            std::string line;
            getline(res_file, line);
            if(line.empty())
            {
               continue;
            }
            line = TrimSpaces(line);
            std::vector<std::string> filevalues = SplitString(line, "\t ");
            if(filevalues[0] == "X")
            {
               CopyStdout(log_file);
               if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy))
               {
                  THROW_ERROR("Simulation not terminated!");
               }
               else
               {
                  break;
               }
            }
            else if(filevalues[0] == "0")
            {
               CopyStdout(log_file);
               if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy))
               {
                  THROW_ERROR("Simulation not correct!");
               }
               else
               {
                  break;
               }
            }
            else if(filevalues[0] == "-")
            {
               THROW_WARNING("Simulation completed but it is not possible to determine if it is correct!");
            }
            else if(filevalues[0] != "1")
            {
               CopyStdout(log_file);
               THROW_ERROR("String not valid: " + line);
            }
            auto sim_cycles = boost::lexical_cast<unsigned long long int>(filevalues[1]);
            if(filevalues.size() == 3)
            {
               if(filevalues[2] == "ns")
               {
                  sim_cycles = static_cast<unsigned long long int>(static_cast<long double>(sim_cycles) / Param->getOption<long double>(OPT_clock_period));
               }
               else if(filevalues[2] == "ps")
               {
                  sim_cycles = static_cast<unsigned long long int>(static_cast<long double>(sim_cycles) / 1000 / Param->getOption<long double>(OPT_clock_period));
               }
               else
               {
                  THROW_ERROR("Unexpected time unit: " + filevalues[2]);
               }
            }
            PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, (i + 1) << ". Simulation completed with SUCCESS; Execution time " << sim_cycles << " cycles;");
            num_cycles += sim_cycles;
            i++;
         }
      }
      else
      {
         CopyStdout(log_file);
         THROW_ERROR("Result file not correctly created");
      }
   }
   else
   {
      /// check for Not correct termination
      if(!boost::filesystem::exists(result_file))
      {
         if(output_level != OUTPUT_LEVEL_VERBOSE)
         {
            CopyStdout(log_file);
         }
         THROW_ERROR("The simulation does not end correctly");
      }
      std::ifstream res_file(result_file.c_str());
      if(res_file.is_open())
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "File \"" + result_file + "\" opened");
         while(!res_file.eof())
         {
            std::string line;
            getline(res_file, line);
            if(line.empty())
            {
               continue;
            }
            line = TrimSpaces(line);
            std::vector<std::string> filevalues = SplitString(line, "\t ");
            if(filevalues[0] == "X")
            {
               CopyStdout(log_file);
               if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy))
               {
                  THROW_ERROR("Simulation not terminated!");
               }
               else
               {
                  break;
               }
            }
            else if(filevalues[0] == "0")
            {
               CopyStdout(log_file);
               if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy))
               {
                  THROW_ERROR("Simulation not correct!");
               }
               else
               {
                  break;
               }
            }
            else if(filevalues[0] != "1")
            {
               CopyStdout(log_file);
               THROW_ERROR("String not valid: " + line);
            }
         }
      }
      else
      {
         CopyStdout(log_file);
         THROW_ERROR("Result file not correctly created");
      }
      std::ifstream profiling_res_file(profiling_result_file.c_str());
      if(profiling_res_file.is_open())
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "File \"" + profiling_result_file + "\" opened");
         double clock_period = Param->isOption(OPT_clock_period) ? Param->getOption<double>(OPT_clock_period) : 10;
         double time_stamp = 0.0;
#if HAVE_ASSERTS
         unsigned int prev_state = 3;
         bool first_iteration = true;
#endif
         while(!profiling_res_file.eof())
         {
            std::string line;
            getline(profiling_res_file, line);
            if(line.empty())
            {
               continue;
            }
            std::vector<std::string> filevalues = SplitString(line, "\t");
            boost::trim(filevalues[0]);
            boost::trim(filevalues[1]);
            if(filevalues[0] != "2" && filevalues[0] != "3")
            {
               CopyStdout(log_file);
               THROW_ERROR("String not valid: " + line);
            }
            if(filevalues[0] == "2")
            {
               THROW_ASSERT(prev_state == 3, "Something wrong happen during the reading of the profiling results");
#if HAVE_ASSERTS
               prev_state = 2;
#endif
               time_stamp = time_stamp - boost::lexical_cast<double>(filevalues[1]);
            }
            else
            {
               THROW_ASSERT(prev_state == 2 || first_iteration, "Something wrong happen during the reading of the profiling results");
#if HAVE_ASSERTS
               prev_state = 3;
#endif
               time_stamp = time_stamp + clock_period + boost::lexical_cast<double>(filevalues[1]);
            }
            i++;
#if HAVE_ASSERTS
            first_iteration = false;
#endif
         }
         num_cycles = static_cast<unsigned long long int>(std::round(time_stamp / clock_period));
         i = i / 2;
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Simulation completed with SUCCESS; Total Execution time " << num_cycles << " cycles; Number of executions " << i << ";");
      }
      else
      {
         CopyStdout(log_file);
         THROW_ERROR("Profiling result file not correctly created");
      }
   }

   if(i == 0)
   {
      if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy))
      {
         THROW_ERROR("Expected a number of cycles different from zero. Something wrong happened during the simulation!");
      }
      else
      {
         num_cycles = i = 1;
      }
   }
   accum_cycles = num_cycles;
   n_testcases = i;
   return num_cycles / i;
}

std::string SimulationTool::GenerateSimulationScript(const std::string& top_filename, const std::list<std::string>& file_list)
{
   std::ostringstream script;
   script << "#!/bin/bash" << std::endl;
   script << "##########################################################" << std::endl;
   script << "#     Automatically generated by the PandA framework     #" << std::endl;
   script << "##########################################################" << std::endl << std::endl;
   script << "# Simulation script for COMPONENT: " << top_filename << std::endl << std::endl;
   script << "cd " << GetCurrentPath() << std::endl;

   GenerateScript(script, top_filename, file_list);

   if(!log_file.empty() && output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      script << "cat " << log_file << std::endl << std::endl;
   }

   // Create the simulation script
   generated_script = GetPath("./" + std::string("simulate_") + top_filename + std::string(".sh"));
   std::ofstream file_stream(generated_script.c_str());
   file_stream << script.str() << std::endl;
   file_stream.close();

   ToolManagerRef tool(new ToolManager(Param));
   tool->configure("chmod", "");
   std::vector<std::string> parameters, input_files, output_files;
   parameters.emplace_back("+x");
   parameters.push_back(generated_script);
   input_files.push_back(generated_script);
   tool->execute(parameters, input_files, output_files, Param->getOption<std::string>(OPT_output_temporary_directory) + "/simulation_generation_scripts_output");

   return generated_script;
}

void SimulationTool::Clean() const
{
}
