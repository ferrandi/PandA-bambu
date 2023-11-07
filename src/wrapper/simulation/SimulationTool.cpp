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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "SimulationTool.hpp"

#include "config_HAVE_ASSERTS.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "ISE_isim_wrapper.hpp"
#include "IcarusWrapper.hpp"
#include "Parameter.hpp"
#include "ToolManager.hpp"
#include "VIVADO_xsim_wrapper.hpp"
#include "VerilatorWrapper.hpp"
#include "compiler_wrapper.hpp"
#include "constants.hpp"
#include "custom_set.hpp"
#include "fileIO.hpp"
#include "modelsimWrapper.hpp"
#include "string_manipulation.hpp"
#include "testbench_generation_constants.hpp"
#include "utility.hpp"

#include <cmath>
#include <regex>
#include <string>
#include <thread>
#include <vector>

SimulationTool::type_t SimulationTool::to_sim_type(const std::string& str)
{
   if(str == "MODELSIM")
   {
      return SimulationTool::MODELSIM;
   }
   else if(str == "ISIM")
   {
      return SimulationTool::ISIM;
   }
   else if(str == "XSIM")
   {
      return SimulationTool::XSIM;
   }
   else if(str == "ICARUS")
   {
      return SimulationTool::ICARUS;
   }
   else if(str == "VERILATOR")
   {
      return SimulationTool::VERILATOR;
   }
   else
   {
      THROW_ERROR("Unknown simulator: " + str);
   }
   return SimulationTool::UNKNOWN;
}

SimulationTool::SimulationTool(const ParameterConstRef& _Param, const std::string& _top_fname)
    : Param(_Param),
      debug_level(Param->getOption<int>(OPT_debug_level)),
      output_level(Param->getOption<unsigned int>(OPT_output_level)),
      top_fname(_top_fname)
{
}

SimulationTool::~SimulationTool() = default;

SimulationToolRef SimulationTool::CreateSimulationTool(type_t type, const ParameterConstRef& _Param,
                                                       const std::string& suffix, const std::string& top_fname)
{
   switch(type)
   {
      case UNKNOWN:
         THROW_ERROR("Simulation tool not specified");
         break;
      case MODELSIM:
         return SimulationToolRef(new modelsimWrapper(_Param, suffix, top_fname));
         break;
      case ISIM:
         return SimulationToolRef(new ISE_isim_wrapper(_Param, suffix, top_fname));
         break;
      case XSIM:
         return SimulationToolRef(new VIVADO_xsim_wrapper(_Param, suffix, top_fname));
         break;
      case ICARUS:
         return SimulationToolRef(new IcarusWrapper(_Param, suffix, top_fname));
         break;
      case VERILATOR:
         return SimulationToolRef(new VerilatorWrapper(_Param, suffix, top_fname));
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

void SimulationTool::Simulate(unsigned long long int& accum_cycles, unsigned long long& n_testcases)
{
   if(generated_script.empty())
   {
      THROW_ERROR("Simulation script not yet generated");
   }

   /// remove previous simulation results
   auto result_file = Param->getOption<std::string>(OPT_simulation_output);
   if(std::filesystem::exists(result_file))
   {
      std::filesystem::remove_all(result_file);
   }
   auto profiling_result_file = Param->getOption<std::string>(OPT_profiling_output);
   if(std::filesystem::exists(profiling_result_file))
   {
      std::filesystem::remove_all(profiling_result_file);
   }
   ToolManagerRef tool(new ToolManager(Param));
   tool->configure(generated_script, "");
   std::vector<std::string> parameters, input_files, output_files;
   if(Param->isOption(OPT_testbench_argv))
   {
      const auto tb_argv = Param->getOption<std::string>(OPT_testbench_argv);
      parameters.push_back(tb_argv);
   }
   tool->execute(parameters, input_files, output_files,
                 Param->getOption<std::string>(OPT_output_temporary_directory) + "/simulation_output");

   DetermineCycles(accum_cycles, n_testcases);
}

void SimulationTool::DetermineCycles(unsigned long long int& accum_cycles, unsigned long long& n_testcases)
{
   unsigned long long int num_cycles = 0;
   unsigned long long i = 0;
   const auto sim_period = 2.0l;
   const auto result_file = Param->getOption<std::string>(OPT_simulation_output);
   const auto profiling_result_file = Param->getOption<std::string>(OPT_profiling_output);
   const auto discrepancy_enabled = Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy);
   const auto profiling_enabled = std::filesystem::exists(profiling_result_file);
   if(!std::filesystem::exists(result_file))
   {
      THROW_ERROR("The simulation does not end correctly");
   }

   std::ifstream res_file(result_file);
   if(!res_file.is_open())
   {
      THROW_ERROR("Unable to open results file");
   }
   PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "File \"" + result_file + "\" opened");
   std::string values;
   std::getline(res_file, values);
   if(values.empty())
   {
      THROW_ERROR("Result file was empty");
   }
   const auto sim_times = convert_string_to_vector<std::string>(values, ",");
   for(const auto& start_end : sim_times)
   {
      if(start_end.back() == 'X')
      {
         if(discrepancy_enabled)
         {
            num_cycles = i = 1;
            break;
         }
         THROW_ERROR("Simulation not terminated!");
      }
      else if(start_end.back() == 'A')
      {
         THROW_ERROR("Simulation terminated with abort call!");
      }
      if(!profiling_enabled)
      {
         const auto times = SplitString(start_end, "|");
         THROW_ASSERT(times.size() == 2, "Unexpected simulation time format");
         unsigned long long start_time = 0, end_time = 0;
         if(!boost::conversion::try_lexical_convert<unsigned long long>(times.at(0), start_time) ||
            !boost::conversion::try_lexical_convert<unsigned long long>(times.at(1), end_time))
         {
            THROW_ERROR("Unable to parse simulation time report: check simulator output for errors.");
         }
         THROW_ASSERT(end_time >= start_time, "Simulation went back in time");
         const auto sim_time = static_cast<long double>(end_time - start_time);
         const auto sim_cycles = static_cast<unsigned long long int>(std::ceil(sim_time / sim_period));
         num_cycles += sim_cycles;
         ++i;
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                       "Run " << i << " execution time " << sim_cycles << " cycles;");
      }
   }

   if(profiling_enabled)
   {
      std::ifstream profiling_res_file(profiling_result_file.c_str());
      if(!profiling_res_file.is_open())
      {
         THROW_ERROR("Profiling result file not correctly created");
      }

      PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "File \"" + profiling_result_file + "\" opened");
      std::getline(profiling_res_file, values);
      const auto profile_times = convert_string_to_vector<std::string>(values, ",");
      for(const auto& start_end : profile_times)
      {
         const auto times = SplitString(start_end, "|");
         THROW_ASSERT(times.size() == 2, "Unexpected simulation time format");
         unsigned long long start_time = 0, end_time = 0;
         if(!boost::conversion::try_lexical_convert<unsigned long long>(times.at(0), start_time) ||
            !boost::conversion::try_lexical_convert<unsigned long long>(times.at(1), end_time))
         {
            THROW_ERROR("Unable to parse simulation time report: check simulator output for errors.");
         }
         THROW_ASSERT(end_time >= start_time, "Profiling went back in time");
         const auto sim_time = static_cast<long double>(end_time - start_time);
         const auto sim_cycles = static_cast<unsigned long long int>(std::ceil(sim_time / sim_period));
         num_cycles += sim_cycles;
         ++i;
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                       "Run " << i << " profiled execution time " << sim_cycles << " cycles;");
      }
   }

   std::getline(res_file, values);
   if(values.back() == 'A')
   {
      THROW_ERROR("Co-simulation main aborted");
   }
   int cosim_retval;
   try
   {
      cosim_retval = std::stoi(values);
   }
   catch(...)
   {
      THROW_ERROR("Co-simulation completed unexpectedly.");
   }

   if(cosim_retval)
   {
      THROW_ERROR("Co-simulation main returned non-zero value: " + values);
   }

   if(i == 0)
   {
      THROW_ERROR("Expected a number of cycles different from zero. Something wrong happened during the simulation!");
   }
   accum_cycles = num_cycles;
   n_testcases = i;
}

std::string SimulationTool::GenerateSimulationScript(const std::string& top_filename, std::list<std::string> file_list)
{
   std::ostringstream script;
   script << "#!/bin/bash\n"
          << "##########################################################\n"
          << "#     Automatically generated by the PandA framework     #\n"
          << "##########################################################\n"
          << "# Simulation script for COMPONENT: " << top_filename << "\n"
          << "set -e\n"
          << "cd " << GetCurrentPath() << "\n"
          << "if [ ! -z \"$APPDIR\" ]; then LD_LIBRARY_PATH=\"\"; fi\n";

   file_list.push_back(relocate_compiler_path(PANDA_DATA_INSTALLDIR) + "/panda/libmdpi/mdpi.c");

   const auto default_compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   const auto opt_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_gcc_optimization_set);
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(Param, default_compiler, opt_set));
   const auto sim_dir = Param->getOption<std::string>(OPT_output_directory) + "/simulation/";
   auto compiler_env = std::regex_replace("\n" + compiler_wrapper->GetCompiler().gcc,
                                          std::regex("([\\w\\d]+=(\".*\"|[^\\s]+))\\s*"), "export $1\n");
   boost::replace_last(compiler_env, "\n", "\nCC=\"");
   compiler_env += "\"";

   script << compiler_env << "\n";

   if(Param->isOption(OPT_testbench_input_file) &&
      starts_with(Param->getOption<std::string>(OPT_testbench_input_file), "elf:"))
   {
      script << "SYS_ELF=\"" << Param->getOption<std::string>(OPT_testbench_input_file).substr(4) << "\"\n";
   }
   else
   {
      script << "SYS_ELF=\"" << sim_dir << "/testbench\"\n";
   }

   script << "SIM_DIR=\"" << sim_dir << "\"\n"
          << "OUT_LVL=\"" << Param->getOption<int>(OPT_output_level) << "\"\n\n"
          << "### Do not edit below\n\n";

   script << "function cleanup { if [ ${__sys_elf_pid} -ne 0 ]; then kill ${__sys_elf_pid}; fi }\n"
          << "trap cleanup EXIT\n\n";

   GenerateScript(script, top_filename, file_list);

   script << "wait ${__sys_elf_pid}\n"
          << "__sys_elf_pid=0\n";

   // Create the simulation script
   generated_script = GetPath("./" + std::string("simulate_") + top_filename + std::string(".sh"));
   std::ofstream file_stream(generated_script.c_str());
   file_stream << script.str() << "\n";
   file_stream.close();

   ToolManagerRef tool(new ToolManager(Param));
   tool->configure("chmod", "");
   std::vector<std::string> parameters, input_files, output_files;
   parameters.emplace_back("+x");
   parameters.push_back(generated_script);
   input_files.push_back(generated_script);
   tool->execute(parameters, input_files, output_files,
                 Param->getOption<std::string>(OPT_output_temporary_directory) +
                     "/simulation_generation_scripts_output");

   return generated_script;
}

std::string SimulationTool::GenerateLibraryBuildScript(std::ostringstream& script, const std::string& beh_dir,
                                                       std::string& beh_cflags) const
{
   const auto default_compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   const auto opt_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_gcc_optimization_set);
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(Param, default_compiler, opt_set));

   const auto extra_compiler_flags = [&]() {
      std::string flags = " -fwrapv -ffloat-store -flax-vector-conversions -msse2 -mfpmath=sse -fno-strict-aliasing "
                          "-D__builtin_bambu_time_start\\(\\)= -D__builtin_bambu_time_stop\\(\\)= -D__BAMBU_SIM__";
      flags += " -I" + relocate_compiler_path(PANDA_DATA_INSTALLDIR) + "/panda/libmdpi/include";
      if(!Param->isOption(OPT_input_format) ||
         Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_C)
      {
         flags += " -fexcess-precision=standard";
      }
      if(Param->isOption(OPT_gcc_optimizations))
      {
         const auto gcc_parameters = Param->getOption<const CustomSet<std::string>>(OPT_gcc_optimizations);
         if(gcc_parameters.find("tree-vectorize") != gcc_parameters.end())
         {
            boost::replace_all(flags, "-msse2", "");
            flags += " -m32";
         }
      }
      return flags;
   }();
   auto cflags = compiler_wrapper->GetCompilerParameters(extra_compiler_flags);
   std::cmatch what;
   std::string kill_printf;
   if(std::regex_search(cflags.c_str(), what, std::regex("\\s*(\\-D'?printf[^=]*='?)'*")))
   {
      kill_printf.append(what[1].first, what[1].second);
      cflags.erase(static_cast<size_t>(what[0].first - cflags.c_str()),
                   static_cast<size_t>(what[0].second - what[0].first));
   }
   beh_cflags += " -I" + relocate_compiler_path(PANDA_DATA_INSTALLDIR) + "/panda/libmdpi/include";
   beh_cflags += " -D__M_IPC_FILENAME=\\\\\\\"${SIM_DIR}/panda_ipc_mmap\\\\\\\"";
   beh_cflags += " -D__M_OUT_LVL=${OUT_LVL}";
   if(cflags.find("-m32") != std::string::npos)
   {
      beh_cflags += " -D__M32";
   }
   else if(cflags.find("-mx32") != std::string::npos)
   {
      beh_cflags += " -D__MX32";
   }
   else if(cflags.find("-m64") != std::string::npos)
   {
      beh_cflags += " -D__M64";
   }
   beh_cflags += " -O2";

   const auto m_top_fname = cxa_prefix_mangled(top_fname, "__m_");
   const auto m_pp_top_fname = cxa_prefix_mangled(top_fname, "__m_pp_");
   const auto srcs =
       Param->getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_RAW ?
           boost::replace_all_copy(Param->getOption<std::string>(OPT_input_file), STR_CST_string_separator, " ") :
           "";
   const std::string cosim_src = "${SIM_DIR}/" STR_CST_testbench_generation_basename ".c";
   const auto pp_srcs = Param->isOption(OPT_pretty_print) ? Param->getOption<std::string>(OPT_pretty_print) : "";
   const auto tb_srcs = [&]() {
      std::string files;
      if(Param->isOption(OPT_testbench_input_file))
      {
         const auto tb_files = Param->getOption<const CustomSet<std::string>>(OPT_testbench_input_file);
         if(starts_with(*tb_files.begin(), "elf:"))
         {
            return files;
         }
         for(const auto& filename : tb_files)
         {
            if(ends_with(filename, ".xml"))
            {
               files += cosim_src + " ";
            }
            else
            {
               files += filename + " ";
            }
         }
      }
      else if(Param->isOption(OPT_testbench_input_string))
      {
         files += cosim_src + " ";
      }
      if(Param->isOption(OPT_no_parse_files))
      {
         files +=
             boost::replace_all_copy(Param->getOption<std::string>(OPT_no_parse_files), STR_CST_string_separator, " ") +
             " ";
      }
      boost::trim(files);
      return files;
   }();
   const auto tb_extra_cflags =
       Param->isOption(OPT_tb_extra_gcc_options) ? Param->getOption<std::string>(OPT_tb_extra_gcc_options) : "";

   script << "make -C " << relocate_compiler_path(PANDA_DATA_INSTALLDIR) << "/panda/libmdpi \\\n"
          << "  SIM_DIR=\"${SIM_DIR}\" BEH_DIR=\"" << beh_dir << "\" \\\n"
          << "  TOP_FNAME=\"" << top_fname << "\" \\\n"
          << "  MTOP_FNAME=\"" << m_top_fname << "\" \\\n"
          << "  MPPTOP_FNAME=\"" << m_pp_top_fname << "\" \\\n"
          << "  CC=\"${CC}\" \\\n"
          << "  BEH_CC=\"${BEH_CC}\" \\\n"
          << "  CFLAGS=\"" << cflags << "\" \\\n"
          << "  BEH_CFLAGS=\"" << beh_cflags << "\" \\\n"
          << "  TB_CFLAGS=\"" << tb_extra_cflags << "\" \\\n"
          << "  SRCS=\"" << srcs << "\" \\\n"
          << "  COSIM_SRC=\"" << cosim_src << "\" \\\n"
          << "  PP_SRC=\"" << pp_srcs << "\" \\\n"
          << "  TB_SRCS=\"" << tb_srcs << "\" \\\n"
          << "  -j " << std::thread::hardware_concurrency() << " -f Makefile.mk\n\n";

   script
       << "if [ -f ${SYS_ELF} ]; then\n"
       << "  function get_class { readelf -h $1 | grep Class: | sed -E 's/.*Class:\\s*(\\w+)/\\1/'; }\n"
       << "  sys_elf_class=\"$(get_class ${SYS_ELF})\"\n"
       << "  driver_elf_class=\"$(get_class ${SIM_DIR}/libmdpi_driver.so)\"\n"
       << "  if [ \"${sys_elf_class}\" != \"${driver_elf_class}\" ]; then\n"
       << "    echo \"ERROR: Wrong system application ELF class: ${sys_elf_class} != ${driver_elf_class}\"; exit 1;\n"
       << "  fi\n"
       << "  LD_PRELOAD=${SIM_DIR}/libmdpi_driver.so ${SYS_ELF} \"$@\" 2>&1 | tee ${SIM_DIR}/$(basename "
          "${SYS_ELF}).log &\n"
       << "  __sys_elf_pid=$!\n"
       << "  echo \"Launched user testbench (PID ${__sys_elf_pid}) with args: $@\"\n"
       << "fi\n\n";

   return cflags;
}

void SimulationTool::Clean() const
{
}
