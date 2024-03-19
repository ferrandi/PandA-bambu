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
 * @file VerilatorWrapper.cpp
 * @brief Wrapper to Verilator simulator.
 *
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "VerilatorWrapper.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "file_IO_constants.hpp"
#include "simple_indent.hpp"
#include "structural_objects.hpp"
#include "utility.hpp"

#include <fstream>
#include <utility>

// constructor
VerilatorWrapper::VerilatorWrapper(const ParameterConstRef& _Param, const std::string& _top_fname,
                                   const std::string& _inc_dirs)
    : SimulationTool(_Param, _top_fname, _inc_dirs)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the VERILATOR wrapper...");
}

void VerilatorWrapper::GenerateVerilatorMain(const std::filesystem::path& filename) const
{
   std::ofstream os(filename, std::ios::out);
   simple_indent PP('{', '}', 3);

   PP(os, "#include <memory>\n");
   PP(os, "\n");
   PP(os, "#include <verilated.h>\n");
   PP(os, "\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "# include <verilated_vcd_c.h>\n");
   PP(os, "#endif\n");
   PP(os, "\n");
   PP(os, "#include \"Vbambu_testbench.h\"\n");
   PP(os, "\n");
   PP(os, "\n");
   PP(os, "static vluint64_t CLOCK_PERIOD = 2;\n");
   PP(os, "static vluint64_t HALF_CLOCK_PERIOD = CLOCK_PERIOD/2;\n");
   PP(os, "\n");
   PP(os, "vluint64_t main_time = 0;\n");
   PP(os, "\n");
   PP(os, "double sc_time_stamp ()  {return main_time;}\n");
   PP(os, "\n");
   PP(os, "int main (int argc, char **argv, char **env)\n");
   PP(os, "{\n");
   PP(os, "Verilated::commandArgs(argc, argv);\n");
   PP(os, "Verilated::debug(0);\n");
   PP(os, "const std::unique_ptr<Vbambu_testbench> top{new Vbambu_testbench{\"clocked_bambu_testbench\"}};");
   PP(os, "\n");
   PP(os, "\n");
   PP(os, "main_time=0;\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "Verilated::traceEverOn(true);\n");
   PP(os, "const std::unique_ptr<VerilatedVcdC> tfp{new VerilatedVcdC};\n");
   PP(os, "top->trace (tfp.get(), 99);\n");
   PP(os, "tfp->set_time_unit(\"p\");\n");
   PP(os, "tfp->set_time_resolution(\"p\");\n");
   PP(os, "tfp->open (\"" + beh_dir.string() + "/test.vcd\");\n");
   PP(os, "#endif\n");
   PP(os, "top->" CLOCK_PORT_NAME " = 1;\n");
   PP(os, "while (!Verilated::gotFinish())\n");
   PP(os, "{\n");
   PP(os, "top->" CLOCK_PORT_NAME " = !top->" CLOCK_PORT_NAME ";\n");
   PP(os, "top->eval();\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "tfp->dump (main_time);\n");
   PP(os, "#endif\n");
   PP(os, "main_time += HALF_CLOCK_PERIOD;\n");
   PP(os, "}\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "tfp->dump (main_time);\n");
   PP(os, "tfp->close();\n");
   PP(os, "#endif\n");
   PP(os, "top->final();\n");
   PP(os, "\n");
   PP(os, "return 0;\n");
   PP(os, "}");
}

std::string VerilatorWrapper::GenerateScript(std::ostream& script, const std::string& top_filename,
                                             const std::list<std::string>& file_list)
{
   for(const auto& file : file_list)
   {
      if(file.find(".vhd") != std::string::npos)
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Mixed simulation not supported by Verilator");
      }
   }
   const auto generate_vcd_output = (Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd)) ||
                                    (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                                    (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
   const auto main_filename = beh_dir / "sim_main.cpp";
   GenerateVerilatorMain(main_filename);
   log_file = "${BEH_DIR}/" + top_filename + "_verilator.log";
   script << "export VM_PARALLEL_BUILDS=1\n"
          << "BEH_CC=\"${CC}\"\n"
          << "obj_dir=\"${BEH_DIR}/verilator_obj\"\n\n";
   std::string beh_cflags = "-DVERILATOR -isystem $(dirname $(which verilator))/../share/verilator/include/vltstd";
   const auto cflags = GenerateLibraryBuildScript(script, beh_cflags);
   const auto vflags = [&]() {
      std::string flags;
      if(cflags.find("-m32") != std::string::npos)
      {
         flags += " +define+__M32";
      }
      else if(cflags.find("-mx32") != std::string::npos)
      {
         flags += " +define+__MX32";
      }
      else if(cflags.find("-m64") != std::string::npos)
      {
         flags += " +define+__M64";
      }
      const auto inc_dir_list = string_to_container<std::vector<std::string>>(inc_dirs, ";");
      for(const auto& inc : inc_dir_list)
      {
         flags += " +incdir+" + inc;
      }
      flags += " +define+__BAMBU_SIM__";
      return flags;
   }();

#ifdef _WIN32
   /// this removes the dependency from perl on MinGW32
   script << "verilator_bin"
#else
   script << "verilator"
#endif
          << " --cc --exe --Mdir ${obj_dir} -Wno-fatal -Wno-lint -sv " << vflags
          << " ${BEH_DIR}/libmdpi.so -O3 --unroll-count 10000 --output-split-cfuncs 3000  --output-split-ctrace 3000";
   if(!generate_vcd_output)
   {
      script << " --x-assign fast --x-initial fast --noassert";
   }

   auto nThreadsVerilator = 1;
   if(Param->isOption(OPT_verilator_parallel) && Param->getOption<int>(OPT_verilator_parallel) > 1)
   {
      const auto thread_support =
          system("bash -c \"if [ $(verilator --version | grep Verilator | sed -E 's/Verilator ([0-9]+).*/\1/') -ge 4 "
                 "]; then exit 0; else exit 1; fi\" > /dev/null 2>&1") == 0;
      THROW_WARNING("Installed version of Verilator does not support multi-threading.");
      if(thread_support)
      {
         nThreadsVerilator = Param->getOption<int>(OPT_verilator_parallel);
      }
   }

   if(nThreadsVerilator > 1)
   {
      script << " --threads " << nThreadsVerilator;
   }
   if(generate_vcd_output)
   {
      script << " --trace --trace-underscore"; // --trace-params
      auto is_verilator_l2_name =
          system("bash -c \"if [[ \\\"x$(verilator --l2-name v 2>&1 | head -n1 | grep -i 'Invalid Option')\\\" = "
                 "\\\"x\\\" ]]; then exit 0; else exit 1; fi\" > /dev/null 2>&1") == 0;
      if(is_verilator_l2_name)
      {
         script << " --l2-name bambu_testbench";
      }
   }
   for(const auto& file : file_list)
   {
      if(ends_with(file, "mdpi.c"))
      {
         script << " ${BEH_DIR}/libmdpi.so";
      }
      else
      {
         script << " " << file;
      }
   }
   script << " " << main_filename.string() << " --top-module bambu_testbench\n"
          << "if [ $? -ne 0 ]; then exit 1; fi\n\n"
          << "ln -sf ../../ ${obj_dir}/" << Param->getOption<std::string>(OPT_output_directory) << " || true\n";

   const auto nThreadsMake =
       Param->isOption(OPT_verilator_parallel) ? Param->getOption<int>(OPT_verilator_parallel) : 1;
   script << "make -C ${obj_dir}  -j " << nThreadsMake
          << " OPT=\"-fstrict-aliasing\" -f Vbambu_testbench.mk Vbambu_testbench";
#ifdef _WIN32
   /// VM_PARALLEL_BUILDS=1 removes the dependency from perl
   script << " VM_PARALLEL_BUILDS=1 CFG_CXXFLAGS_NO_UNUSED=\"\"";
#endif
   script << "\n\n";

   return "${obj_dir}/Vbambu_testbench 2>&1 | tee " + log_file;
}
