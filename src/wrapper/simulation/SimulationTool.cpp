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
#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"

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

#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/regex.hpp>
#include <cmath>
#include <string>
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
   const auto profiling_enabled = boost::filesystem::exists(profiling_result_file);
   if(!boost::filesystem::exists(result_file))
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
                       i << ". Simulation completed with SUCCESS; Execution time " << sim_cycles << " cycles;");
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
                       i << ". Simulation completed with SUCCESS; Profiled execution time " << sim_cycles
                         << " cycles;");
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
      cosim_retval = boost::lexical_cast<int>(values);
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

std::string SimulationTool::GenerateSimulationScript(const std::string& top_filename,
                                                     const std::list<std::string>& file_list)
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

   GenerateScript(script, top_filename, file_list);

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

std::string SimulationTool::GenerateLibraryBuildScript(std::ostringstream& script, const std::string& output_dir,
                                                       std::string& cflags) const
{
   const auto default_compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   const auto opt_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_gcc_optimization_set);
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(Param, default_compiler, opt_set));

   const auto extra_compiler_flags = [&]() {
      std::string flags = cflags +
                          " -fwrapv -ffloat-store -flax-vector-conversions -msse2 -mfpmath=sse -fno-strict-aliasing "
                          "-D__builtin_bambu_time_start()= -D__builtin_bambu_time_stop()= -D__BAMBU_SIM__";
      flags += " -I" + relocate_compiler_path(PANDA_INCLUDE_INSTALLDIR);
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
   cflags = compiler_wrapper->GetCompilerParameters(extra_compiler_flags);
   boost::cmatch what;
   std::string kill_printf;
   if(boost::regex_search(cflags.c_str(), what, boost::regex("\\s*(\\-D'?printf[^=]*='?)'*")))
   {
      kill_printf.append(what[1].first, what[1].second);
      cflags.erase(static_cast<size_t>(what[0].first - cflags.c_str()),
                   static_cast<size_t>(what[0].second - what[0].first));
   }

   const auto top_dfname = string_demangle(top_fname);
   const auto add_fname_prefix = [&](const std::string& prefix) {
      if(top_dfname.size() && top_fname != top_dfname)
      {
         const auto fname = top_dfname.substr(0, top_dfname.find('('));
         return boost::replace_first_copy(top_fname, STR(fname.size()) + fname,
                                          STR(fname.size() + prefix.size()) + prefix + fname);
      }
      return prefix + top_fname;
   };
   const auto tb_srcs = [&]() {
      CustomSet<std::string> srcs;
      if(Param->isOption(OPT_testbench_input_file))
      {
         const auto tb_files = Param->getOption<const CustomSet<std::string>>(OPT_testbench_input_file);
         srcs.insert(tb_files.begin(), tb_files.end());
      }
      if(Param->isOption(OPT_no_parse_files))
      {
         const auto no_parse_files = Param->getOption<const CustomSet<std::string>>(OPT_no_parse_files);
         srcs.insert(no_parse_files.begin(), no_parse_files.end());
      }
      return srcs;
   }();

   script << "cosim_argv_h=\"" << Param->getOption<std::string>(OPT_output_directory) << "/simulation/cosim_argv.h\"\n"
          << "argv=(\"$@\")\n"
          << "cat > ${cosim_argv_h} << EOF\n"
          << "#ifndef M_COSIM_ARGV_H\n"
          << "//////////////////////////////////////////////////////////\n"
          << "//    Automatically generated by the PandA framework    //\n"
          << "//////////////////////////////////////////////////////////\n"
          << "\n"
          << "static const char* __m_cosim_argv[] = {\n"
          << "  \"m_cosim_main\"$(printf ',\\n  \"%s\"' \"${argv[@]}\")\n"
          << "  };\n"
          << "#endif // M_COSIM_ARGV_H\n"
          << "EOF\n";

   auto compiler_env = boost::regex_replace("\n" + compiler_wrapper->GetCompiler().gcc,
                                            boost::regex("([\\w\\d]+=(\".*\"|[^\\s]+))\\s*"), "export $1\n");
   boost::replace_last(compiler_env, "\n", "\nexport CC=\"");
   compiler_env += "\"";
   script << compiler_env << "\n"
          << "export CFLAGS=\"" << cflags << "\"\n\n";

   if(!Param->isOption(OPT_input_format) ||
      Param->getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_RAW)
   {
      const auto input_files = Param->getOption<const CustomSet<std::string>>(OPT_input_file);
      const auto m_top_fname = add_fname_prefix("__m_");
      script << "srcs=(\n";
      for(const auto& src : input_files)
      {
         script << "  \"" << src << "\"\n";
      }
      script << ")\n"
             << "objs=()\n"
             << "for src in \"${srcs[@]}\"\n"
             << "do\n"
             << "  obj=\"$(basename ${src})\"\n"
             << "  case \"${obj}\" in\n"
             << "  *.gimplePSSA)\n"
             << "    continue\n"
             << "    ;;\n"
             << "  *)\n"
             << "    obj=\"" << output_dir << "/${obj%.*}.o\"\n"
             << "    ${CC} -c ${CFLAGS} " << kill_printf << " -fPIC -o ${obj} ${src}\n"
             << "    objcopy --weaken --redefine-sym " << top_fname << "=" << m_top_fname << " ${obj}\n"
             << "    objs+=(\"${obj}\")\n"
             << "    ;;\n"
             << "  esac\n"
             << "done\n\n";
   }

   if(Param->isOption(OPT_pretty_print))
   {
      const auto m_pp_top_fname = add_fname_prefix("__m_pp_");
      const auto pp_file = boost::filesystem::path(Param->getOption<std::string>(OPT_pretty_print));
      const auto pp_fileo = output_dir + "/" + pp_file.stem().string() + ".o";
      script << "${CC} -c ${CFLAGS} -fno-strict-aliasing -fPIC";
      if(CompilerWrapper::isClangCheck(default_compiler))
      {
         script << " -fbracket-depth=1024";
      }
      script
          << " -o " << pp_fileo << " " << pp_file.string() << "\n"
          << "objcopy --keep-global-symbol " << top_fname << " $(nm " << pp_fileo
          << " | grep -o '[^[:space:]]*get_pc_thunk[^[:space:]]*' | sed 's/^/--keep-global-symbol /' | tr '\\n' ' ') "
          << pp_fileo << "\n"
          << "objcopy --redefine-sym " << top_fname << "=" << m_pp_top_fname << " " << pp_fileo << "\n"
          << "objs+=(\"" << pp_fileo << "\")\n\n";
   }

   const auto dpi_cwrapper_file =
       Param->getOption<std::string>(OPT_output_directory) + "/simulation/" STR_CST_testbench_generation_basename ".c";
   script << "${CC} -c ${CFLAGS}";
   if(Param->isOption(OPT_pretty_print) && top_fname != "main")
   {
      script << " -DPP_VERIFICATION";
   }
   script << " -fPIC -o " << output_dir << "/m_wrapper.o " << dpi_cwrapper_file << "\n"
          << "objs+=(\"" << output_dir << "/m_wrapper.o\")\n\n";

   if(tb_srcs.size())
   {
      script << "tb_srcs=(\n";
      for(const auto& src : tb_srcs)
      {
         if(!boost::ends_with(src, ".xml"))
         {
            script << "  \"" << src << "\"\n";
         }
      }
      script << ")\n"
             << "TB_CFLAGS=\""
             << (Param->isOption(OPT_testbench_extra_gcc_flags) ?
                     Param->getOption<std::string>(OPT_testbench_extra_gcc_flags) :
                     "")
             << "\"\n"
             << "for src in \"${tb_srcs[@]}\"\n"
             << "do\n"
             << "  obj=\"$(basename ${src})\"\n"
             << "  case \"${obj}\" in\n"
             << "  *.c)  ;&\n"
             << "  *.cc) ;&\n"
             << "  *.cpp)\n"
             << "    obj=\"" << output_dir << "/${obj%.*}.tb.o\"\n"
             << "    ${CC} -c ${CFLAGS} ${TB_CFLAGS} -fPIC -o ${obj} ${src}\n"
             << "    objcopy -W " << top_fname << " ${obj}\n"
             << "    objs+=(\"${obj}\")\n"
             << "    ;;\n"
             << "  *)\n"
             << "    objs+=(\"${src}\")\n"
             << "    ;;\n"
             << "  esac\n"
             << "done\n\n";
   }

   script << "CFLAGS+=\" -D__M_OUT_LVL=" << Param->getOption<int>(OPT_output_level)
          << " -D__M_COSIM_ARGV=\\\"${cosim_argv_h}\\\"\"\n";

   const auto libtb_filename = output_dir + "/libtb.so";
   script << "bash " << relocate_compiler_path(PANDA_INCLUDE_INSTALLDIR "/mdpi/build.sh") << " ${objs[*]} -o "
          << libtb_filename << "\n\n";
   return libtb_filename;
}

void SimulationTool::Clean() const
{
}
