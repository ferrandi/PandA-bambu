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
 * @file DesignCompilerWrapper.cpp
 * @brief Implementation of the wrapper to Design Compiler
 *
 * Implementation of the methods to wrap Design Compiler by Synopsys
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Includes the class definition
#include "DesignCompilerWrapper.hpp"
#include "DesignParameters.hpp"
#include "ToolManager.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_LOGIC_SYNTHESIS_FLOW_BUILT.hpp"
#include "xml_script_command.hpp"

/// Constants include
#include "constant_strings.hpp"

#include "area_model.hpp"
#include "cell_model.hpp"
#include "library_manager.hpp"
#include "structural_manager.hpp"
#include "target_device.hpp"
#include "technology_manager.hpp"
#include "time_model.hpp"

#if HAVE_EXPERIMENTAL
#include "Design.hpp"
#include "Design_manager.hpp"
#include "design_compiler_constants.hpp"
#endif

#include "HDL_manager.hpp"
#include "language_writer.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "utility.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <cpu_time.hpp>
#include <utility.hpp>

#include "string_manipulation.hpp" // for GET_CLASS
#include <fstream>

// Fixed design parameter names
#define dc_compile_effort_high "dc_compile_effort_high"         // enable high effort compilation switch
#define dc_compile_effort_medium "dc_compile_effort_medium"     // enable medium effort compilation switch
#define dc_compile_effort_ultra "dc_compile_effort_ultra"       // enable ultra effort compilation switch
#define dc_constraint_file "dc_constraint_file"                 // constraint file path
#define dc_dont_use "dc_dont_use"                               // nets not to be used for optimization
#define dc_HDL_file "dc_HDL_file"                               // HDL file set
#define dc_link_library "dc_link_library"                       // link library file set
#define dc_max_area "dc_max_area"                               // maximum synthesizable area contraint
#define dc_max_delay "dc_max_delay"                             // maximum synthesizable delay constraint
#define dc_report_power "dc_report_power"                       // power report generation
#define dc_search_path "dc_search_path"                         // library search path
#define dc_synthesize "dc_synthesize"                           // synthesize the design (FIXME: Not needed anymore)
#define dc_target "dc_target"                                   // target component file path
#define dc_target_library "dc_target_library"                   // target library name
#define dc_top "dc_top"                                         // top component name
#define dc_top_filetype "dc_top_filetype"                       // top component file type belonging to: {vhdl, verilog}
#define dc_zero_interconnect_delay "dc_zero_interconnect_delay" // enable zero interconnection delay
#define dc_output_dir "dc_output_dir"                           // output directory

// constructor
DesignCompilerWrapper::DesignCompilerWrapper(const ParameterConstRef _Param, const target_deviceRef _device, const std::string& _output_dir)
    : SynopsysWrapper(_Param, DESIGN_COMPILER_TOOL_ID, _device, _output_dir, "DC"), max_area(0.0), max_delay(0.0), synthesis_result(false)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating and configuring the Design Compiler wrapper...");
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));

   synthetic_libs.push_back("dw_foundation.sldb");
   synthetic_libs.push_back("standard.sldb");
   link_libs.push_back("*");
   link_libs.push_back("dw_foundation.sldb");
   link_libs.push_back("standard.sldb");
   search_path.push_back(".");

   init_reserved_vars();
}

void DesignCompilerWrapper::EvaluateVariables(const DesignParametersRef dp)
{
   // Detect the top module name
   std::string top_name = dp->component_name;
   xml_set_variable_tRef var_top = get_reserved_parameter(dc_top);
   var_top->clean();
   var_top->singleValue = new std::string(top_name);
   dp->parameter_values[dc_top] = top_name;

   if(dp->parameter_values.find(PARAM_sdc_file) != dp->parameter_values.end())
   {
      xml_set_variable_tRef var_sdc = get_reserved_parameter(dc_constraint_file);
      var_sdc->clean();
      var_sdc->singleValue = new std::string(dp->parameter_values[PARAM_sdc_file]);
   }

   set_link_libraries(dp);
   set_target_libraries(dp);

   std::string HDL_files = dp->parameter_values[PARAM_HDL_files];
   std::vector<std::string> files = convert_string_to_vector<std::string>(HDL_files, ";");
   std::string target_file = import_input_design(dp, files);

   perform_optimization(dp);

   save_design(dp, target_file);

   for(auto& xml_script_node : xml_script_nodes)
   {
      if(xml_script_node->nodeType == NODE_COMMAND && *(GetPointer<xml_command_t>(xml_script_node)->name) == "report_area")
      {
         if(!GetPointer<xml_command_t>(xml_script_node)->output)
            THROW_ERROR("output file not specified for command \"report_area\"");
         report_files[REPORT_AREA] = *(GetPointer<xml_command_t>(xml_script_node)->output);
         replace_parameters(dp, report_files[REPORT_AREA]);
      }
      if(xml_script_node->nodeType == NODE_COMMAND && *(GetPointer<xml_command_t>(xml_script_node)->name) == "report_timing")
      {
         if(!GetPointer<xml_command_t>(xml_script_node)->output)
            THROW_ERROR("output file not specified for command \"report_timing\"");
         report_files[REPORT_TIME] = *(GetPointer<xml_command_t>(xml_script_node)->output);
         replace_parameters(dp, report_files[REPORT_TIME]);
      }
   }
}

void DesignCompilerWrapper::init_reserved_vars()
{
   SynthesisTool::init_reserved_vars();

   // Initializing the reserved variables
   ADD_RES_VAR(dc_compile_effort_high);
   ADD_RES_VAR(dc_compile_effort_medium);
   ADD_RES_VAR(dc_compile_effort_ultra);
   ADD_RES_VAR(dc_constraint_file);
   ADD_RES_VAR(dc_dont_use);
   ADD_RES_VAR(dc_HDL_file);
   ADD_RES_VAR(dc_link_library);
   ADD_RES_VAR(dc_max_area);
   ADD_RES_VAR(dc_max_delay);
   ADD_RES_VAR(dc_report_power);
   ADD_RES_VAR(dc_search_path);
   ADD_RES_VAR(dc_synthesize);
   ADD_RES_VAR(dc_target);
   ADD_RES_VAR(dc_target_library);
   ADD_RES_VAR(dc_top);
   ADD_RES_VAR(dc_top_filetype);
   ADD_RES_VAR(dc_zero_interconnect_delay);
}

void DesignCompilerWrapper::set_top_module(const std::string& top)
{
   top_module = top;
}

void DesignCompilerWrapper::set_constraint_file(const std::string& path)
{
   constraint_file = path;
}

// destructor
DesignCompilerWrapper::~DesignCompilerWrapper()
{
}

void DesignCompilerWrapper::add_link_library(const std::vector<std::string>& link_library)
{
   for(const auto& l : link_library)
      add_link_library(l);
}

void DesignCompilerWrapper::add_link_library(const std::string& link_library)
{
   if(std::find(link_libs.begin(), link_libs.end(), link_library) == link_libs.end())
      link_libs.push_back(link_library);
}

void DesignCompilerWrapper::add_target_library(const std::vector<std::string>& target_library)
{
   for(const auto& l : target_library)
      add_target_library(l);
}

void DesignCompilerWrapper::add_target_library(const std::string& target_library)
{
   if(std::find(target_libs.begin(), target_libs.end(), target_library) == target_libs.end())
      target_libs.push_back(target_library);
}

void DesignCompilerWrapper::add_dont_use_cells(const std::string& local_top_module, const std::string& dont_use_cells)
{
   dont_use_map[local_top_module] = dont_use_cells;
}

void DesignCompilerWrapper::set_search_path(const std::string& path)
{
   search_path.push_back(path);
}

void DesignCompilerWrapper::set_search_path(const DesignParametersRef)
{
   xml_set_variable_tRef var_search_path = get_reserved_parameter(dc_search_path);
   var_search_path->clean();
   for(auto& l : search_path)
   {
      xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(l, nullptr));
      var_search_path->multiValues.push_back(entry);
   }
}

void DesignCompilerWrapper::set_link_libraries(const DesignParametersRef)
{
   xml_set_variable_tRef var_link_library = get_reserved_parameter(dc_link_library);
   var_link_library->clean();
   for(auto& link_lib : link_libs)
   {
      xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(link_lib, nullptr));
      var_link_library->multiValues.push_back(entry);
   }
}

void DesignCompilerWrapper::set_target_libraries(const DesignParametersRef)
{
   if(device->has_parameter("target_library"))
   {
      std::string target_library = device->get_parameter<std::string>("target_library");
      target_libs = convert_string_to_vector<std::string>(target_library, ";");
   }

   xml_set_variable_tRef var_target_library = get_reserved_parameter(dc_target_library);
   var_target_library->clean();
   for(auto& target_lib : target_libs)
   {
      xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(target_lib, nullptr));
      var_target_library->multiValues.push_back(entry);
   }
}

std::string DesignCompilerWrapper::import_input_design(const DesignParametersRef dp, const std::vector<std::string>& file_list)
{
   std::string top = dp->component_name;

   HDLWriter_Language language = HDLWriter_Language::VERILOG;
   std::string target = top + "_synth.v";
   xml_set_variable_tRef var_file_set = get_reserved_parameter(dc_HDL_file);
   var_file_set->clean();
   for(const auto& v : file_list)
   {
      xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(v, nullptr));
      var_file_set->multiValues.push_back(entry);

      boost::filesystem::path verilog(v);
      std::string extension = boost::filesystem::extension(verilog);
      if(extension == ".v" || extension == ".verilog")
         language = HDLWriter_Language::VERILOG;
      else if(extension == ".vhd" || extension == ".vhdl")
         language = HDLWriter_Language::VHDL;
      else
         THROW_ERROR("Format of file \"" + v + "\" is not compliant with Design Compiler's wrapper");
      if(top.size() == 0)
      {
         std::string name_v = GetLeafFileName(verilog);
         std::string base_name = name_v.substr(0, name_v.find_last_of("."));
         target = base_name + "_synth.v";
      }
   }

   xml_set_variable_tRef var_top_type = get_reserved_parameter(dc_top_filetype);
   var_top_type->clean();
   if(language == HDLWriter_Language::VERILOG)
      var_top_type->singleValue = new std::string("verilog");
   else if(language == HDLWriter_Language::VHDL)
      var_top_type->singleValue = new std::string("vhdl");
   else
      THROW_ERROR("Input language not supported!");

   return output_dir + "/output/" + target;
}

void DesignCompilerWrapper::set_constraints(const DesignParametersRef dp)
{
   tool = ToolManagerRef(new ToolManager(Param));
   tool->configure(DESIGN_COMPILER_TOOL_ID, "");

   std::string top = dp->component_name;
   if(!top.length() && this->top_module.length())
      top = this->top_module;

   xml_set_variable_tRef var_constraint_file = get_reserved_parameter(dc_constraint_file);
   var_constraint_file->clean();
   if(constraint_file.size())
   {
      std::string relative_file = tool->determine_paths(constraint_file);
      var_constraint_file->singleValue = new std::string(relative_file);
   }

   xml_set_variable_tRef var_max_area = get_reserved_parameter(dc_max_area);
   var_max_area->clean();
   var_max_area->singleValue = new std::string(boost::lexical_cast<std::string>(max_area));
   xml_set_variable_tRef var_zero_int_delay = get_reserved_parameter(dc_zero_interconnect_delay);
   var_zero_int_delay->clean();
   if(Param->getOption<bool>("compound-gates") && !Param->getOption<bool>(OPT_has_complete_characterization))
      var_zero_int_delay->singleValue = new std::string("1");
   else
      var_zero_int_delay->singleValue = new std::string("0");

   xml_set_variable_tRef var_dont_use = get_reserved_parameter(dc_dont_use);
   var_dont_use->clean();
   if(dont_use_map.find(top) != dont_use_map.end() || dont_use_map.find("*") != dont_use_map.end())
   {
      if(dont_use_map.find(top) != dont_use_map.end())
      {
         xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(dont_use_map[top_module], nullptr));
         var_dont_use->multiValues.push_back(entry);
      }
      if(dont_use_map.find("*") != dont_use_map.end()) ///'*' represents a wildcard for all benchmarks
      {
         xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(dont_use_map["*"], nullptr));
         var_dont_use->multiValues.push_back(entry);
      }
   }

   xml_set_variable_tRef var_max_delay = get_reserved_parameter(dc_max_delay);
   var_max_delay->clean();
   var_max_delay->singleValue = new std::string(boost::lexical_cast<std::string>(max_delay));
}

void DesignCompilerWrapper::perform_optimization(const DesignParametersRef)
{
   /// compile the current design
   xml_set_variable_tRef var_synthesize = get_reserved_parameter(dc_synthesize);
   xml_set_variable_tRef var_c_medium = get_reserved_parameter(dc_compile_effort_medium);
   xml_set_variable_tRef var_c_high = get_reserved_parameter(dc_compile_effort_high);
   xml_set_variable_tRef var_c_ultra = get_reserved_parameter(dc_compile_effort_ultra);
   var_synthesize->clean();
   var_c_medium->clean();
   var_c_high->clean();
   var_c_ultra->clean();

   if(Param->getOption<bool>(OPT_rtl))
   {
      unsigned int effort = MEDIUM;
      if(Param->isOption(OPT_design_compiler_effort))
         effort = Param->getOption<unsigned int>(OPT_design_compiler_effort);
      switch(effort)
      {
         case MEDIUM:
            var_c_medium->singleValue = new std::string("1");
            break;
         case HIGH:
            var_c_high->singleValue = new std::string("1");
            break;
         case ULTRA:
            var_c_ultra->singleValue = new std::string("1");
            break;
         default:
            THROW_UNREACHABLE("Unknown design effort: " + boost::lexical_cast<std::string>(effort));
      }
      var_synthesize->singleValue = new std::string("1");
   }
   else
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "No synthesis is performed, only evaluation");
      var_synthesize->singleValue = new std::string("0");
   }
}

void DesignCompilerWrapper::save_design(const DesignParametersRef dp, const std::string& target_file)
{
   dp->parameter_values[dc_output_dir] = output_dir + "/output";

   report_files[SYNTHESIS_RESULT] = target_file;

   xml_set_variable_tRef var_c_target = get_reserved_parameter(dc_target);
   var_c_target->clean();
   var_c_target->singleValue = new std::string(target_file);
}

void DesignCompilerWrapper::write_reports(const DesignParametersRef)
{
   report_files[REPORT_AREA] = "area_report.log";
   report_files[REPORT_CELL] = "cell_report.log";

   if(Param->getOption<bool>(OPT_has_complete_characterization))
   {
      report_files[REPORT_TIME] = "time_report.log";
      report_files[REPORT_POWER] = "power_report.log";
   }
}

void DesignCompilerWrapper::parse_cell_reports()
{
   if(report_files.find(REPORT_CELL) == report_files.end() || !boost::filesystem::exists(report_files[REPORT_CELL]))
      return;
   std::string time_report = report_files[REPORT_CELL];
   std::ifstream output_file(time_report.c_str());
   if(output_file.is_open())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "File \"" + time_report + "\" opened");
      bool reading = false;
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, line);
         if(line.size() and line.find("----") != std::string::npos)
         {
            reading = !reading;
            continue;
         }
         if(reading and line.size() and !boost::algorithm::starts_with(line, " "))
         {
            std::string tk = line.substr(line.find_first_of(' '), line.size());
            boost::trim(tk);
            tk = tk.substr(0, tk.find_first_of(' '));
            if(cell_frequency.find(tk) == cell_frequency.end())
               cell_frequency[tk] = 0;
            cell_frequency[tk]++;
         }
      }
   }
   if(cell_frequency.size())
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "** Cell Report **");
      unsigned int cell_count = 0;
      for(std::map<std::string, unsigned int>::iterator c = cell_frequency.begin(); c != cell_frequency.end(); ++c)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "  * Cell: " << c->first << " - Frequency: " << c->second);
         cell_count += c->second;
      }
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "- Total: " << cell_count);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*****************");
   }
}

time_modelRef DesignCompilerWrapper::parse_time_reports()
{
   if(report_files.find(REPORT_TIME) == report_files.end() || !boost::filesystem::exists(report_files[REPORT_TIME]))
   {
      return time_modelRef();
   }

   std::string time_report = report_files[REPORT_TIME];
   std::ifstream output_file(time_report.c_str());
   if(output_file.is_open())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "File \"" + time_report + "\" opened");
      bool is_path_element = false;
      unsigned int l = 0;
      std::vector<std::string> timing_path, critical_cell;
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, line);
         if(line.size() and line.find("Design :") != std::string::npos)
         {
            std::string token("Design :");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
         }
         if(line.size() and line.find("data arrival time") != std::string::npos)
         {
            std::string token("data arrival time");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            double a_time = boost::lexical_cast<double>(tk);
            is_path_element = false;
            if(a_time < 0)
               a_time = -a_time;
            arrival_time.push_back(a_time);
            if(critical_cell.size())
            {
               critical_paths.push_back(timing_path);
               critical_cells.push_back(critical_cell);
               timing_path.clear();
               critical_cell.clear();
            }
         }
         else if(line.size() and line.find("----") != std::string::npos)
         {
            if(l == 0)
            {
               is_path_element = true;
               l++;
               continue;
            }
         }
         else if(line.size() and line.find("slack") != std::string::npos)
         {
            l = 0;
         }
         else if(line.size() and (is_path_element || line.find(" (in)") != std::string::npos))
         {
            is_path_element = true;
            if(line.find(" (in)") != std::string::npos || line.find(" (ideal)") != std::string::npos || line.find(" (rise edge)") != std::string::npos)
            {
               critical_cell.clear();
               timing_path.clear();
               continue;
            }
            std::string el1 = line.substr(0, line.find(" ("));
            boost::trim(el1);
            timing_path.push_back(el1);
            if(line.find(" (out)") == std::string::npos)
            {
               std::string el = line.substr(line.find_first_of("(") + 1, line.size());
               boost::trim(el);
               el = el.substr(0, el.find_first_of(")"));
               critical_cell.push_back(el);
            }
            else
            {
               is_path_element = false;
               critical_paths.push_back(timing_path);
               critical_cells.push_back(critical_cell);
               timing_path.clear();
               critical_cell.clear();
            }
         }
      }
   }

   time_modelRef time_m;
   if(arrival_time.size())
   {
      double max_arrival_time = 0;
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "** Time Report **");
      for(unsigned int a = 0; a < arrival_time.size(); a++)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "* " << a << ". Arrival time          : " << arrival_time[a]);
         if(arrival_time[a] > max_arrival_time)
            max_arrival_time = arrival_time[a];
      }
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*****************");

      /// saving the value in the time model
      if(max_arrival_time != 0.0)
      {
         time_m = time_model::create_model(device->get_type(), Param);
         time_m->set_execution_time(max_arrival_time, 1);
      }
   }
   return time_m;
}

void DesignCompilerWrapper::parse_synthesis_reports()
{
   if(report_files.find(SYNTHESIS_LOG) == report_files.end() || !boost::filesystem::exists(report_files[SYNTHESIS_LOG]))
   {
      synthesis_time = 0;
      return;
   }
   std::string synthesis_report = report_files[SYNTHESIS_LOG];

   std::ifstream output_file(synthesis_report.c_str());
   if(output_file.is_open())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "File \"" + synthesis_report + "\" opened");
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, line);
         if(line.size() and line.find("microseconds per iteration") != std::string::npos and line.find("echo") == std::string::npos)
         {
            std::string token("microseconds per iteration");
            std::string elapsed_time_string = line.substr(0, line.find(token));
            boost::trim(elapsed_time_string);
            synthesis_time = boost::lexical_cast<double>(elapsed_time_string) / 1000.0;
         }
         if(line.size() and line.find("RESULT SYNTHESIS:") != std::string::npos and line.find("echo") == std::string::npos)
         {
            std::string token("RESULT SYNTHESIS:");
            std::string result_string = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(result_string);
            synthesis_result = boost::lexical_cast<bool>(result_string);
         }
      }
   }

   if(synthesis_time > 0)
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "** Synthesis Report **");
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "* Elapsed synthesis time    : " << print_cpu_time(static_cast<long int>(synthesis_time)) << " seconds");
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*****************");
   }
}

area_modelRef DesignCompilerWrapper::parse_area_reports()
{
   if(report_files.find(REPORT_AREA) == report_files.end() || !boost::filesystem::exists(report_files[REPORT_AREA]))
   {
      return area_modelRef();
   }

   std::string area_report = report_files[REPORT_AREA];
   std::ifstream output_file(area_report.c_str());
   if(output_file.is_open())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "File \"" + area_report + "\" opened");
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, line);
         if(line.size() and line.find("Combinational area:") != std::string::npos)
         {
            std::string token("Combinational area:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            area[area_model::COMBINATIONAL_AREA] = boost::lexical_cast<double>(tk);
         }
         if(line.size() and line.find("Noncombinational area:") != std::string::npos)
         {
            std::string token("Noncombinational area:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            area[area_model::NONCOMBINATIONAL_AREA] = boost::lexical_cast<double>(tk);
         }
         if(line.size() and line.find("Total cell area:") != std::string::npos)
         {
            std::string token("Total cell area:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            area[area_model::CELL_AREA] = boost::lexical_cast<double>(tk);
         }
         if(line.size() and line.find("Total area:") != std::string::npos)
         {
            if(line.find("undefined") != std::string::npos)
               continue;
            std::string token("Total area:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            area[area_model::TOTAL_AREA] = boost::lexical_cast<double>(tk);
         }
         if(line.size() and line.find("Net Interconnect area:") != std::string::npos)
         {
            if(line.find("undefined") != std::string::npos)
               continue;
            std::string token("Net Interconnect area:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            area[area_model::INTERCONNECT_AREA] = boost::lexical_cast<double>(tk);
         }
      }
   }

   area_modelRef area_m;
   if(area.size())
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "** Area Report **");
      if(area.find(area_model::COMBINATIONAL_AREA) != area.end() and area[area_model::COMBINATIONAL_AREA] > 0)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "* Combinational area    : " << area[area_model::COMBINATIONAL_AREA]);
      }
      if(area.find(area_model::NONCOMBINATIONAL_AREA) != area.end() and area[area_model::NONCOMBINATIONAL_AREA] > 0)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "* Noncombinational area : " << area[area_model::NONCOMBINATIONAL_AREA]);
      }
      if(area.find(area_model::CELL_AREA) != area.end() and area[area_model::CELL_AREA] > 0)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "* Total cell area       : " << area[area_model::CELL_AREA]);
      }
      if(area.find(area_model::INTERCONNECT_AREA) != area.end() and area[area_model::INTERCONNECT_AREA] > 0)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "* Net Interconnect area : " << area[area_model::INTERCONNECT_AREA]);
      }
      if(area.find(area_model::TOTAL_AREA) != area.end() and area[area_model::TOTAL_AREA] > 0)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "* Total area            : " << area[area_model::TOTAL_AREA]);
      }
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "*****************");

      /// saving the value in the area model
      if(area.find(area_model::CELL_AREA) != area.end() and area[area_model::CELL_AREA] > 0)
      {
         area_m = area_model::create_model(device->get_type(), Param);
         area_m->set_area_value(area[area_model::CELL_AREA]);
      }
   }
   return area_m;
}

void DesignCompilerWrapper::parse_reports()
{
   parse_synthesis_reports();
   if(synthesis_result)
   {
      parse_area_reports();
      parse_time_reports();
      parse_cell_reports();
   }
   else
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Synthesis FAILED!");
   }
}

std::string DesignCompilerWrapper::write_timing_paths(const std::string& design_name, const std::vector<std::string>& timing_path)
{
   try
   {
      THROW_ASSERT(design_name.size(), "Module name not specified");
      std::string timing_path_xml_file = "timing_path.xml";
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("synthesis");

      xml_element* designRoot = nodeRoot->add_child_element("design");
      WRITE_XNVM(design_name, design_name, designRoot);

      xml_element* timingRoot = designRoot->add_child_element("timing_path");
      std::string number_of_elements = boost::lexical_cast<std::string>(timing_path.size());
      WRITE_XNVM(number_of_elements, number_of_elements, timingRoot);
      std::string type = "POST_SYNTHESIS";
      WRITE_XNVM(type, type, timingRoot);
      for(unsigned int l = 0; l < timing_path.size(); l++)
      {
         xml_element* Enode = timingRoot->add_child_element("path_element");
         std::string path = timing_path[l];
         WRITE_XNVM(path, path, Enode);
         std::string element = boost::lexical_cast<std::string>(l);
         WRITE_XNVM(element, element, Enode);
      }

      document.write_to_file_formatted(timing_path_xml_file);
      return timing_path_xml_file;
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   return "";
}

double DesignCompilerWrapper::get_synthesis_time() const
{
   return synthesis_time;
}

std::map<unsigned int, double> DesignCompilerWrapper::get_area_values() const
{
   if(area.size() == 0)
      return std::map<unsigned int, double>();
   return area;
}

std::vector<double> DesignCompilerWrapper::get_arrival_time() const
{
   if(arrival_time.size() == 0)
      return std::vector<double>();
   return arrival_time;
}

std::map<unsigned int, double> DesignCompilerWrapper::get_power_values() const
{
   if(power.size() == 0)
      return std::map<unsigned int, double>();
   return power;
}

std::vector<std::vector<std::string>> DesignCompilerWrapper::get_critical_cells() const
{
   if(critical_cells.size() == 0)
      return std::vector<std::vector<std::string>>();
   return critical_cells;
}

std::map<std::string, unsigned int> DesignCompilerWrapper::get_cell_frequency() const
{
   if(cell_frequency.size() == 0)
      return std::map<std::string, unsigned int>();
   return cell_frequency;
}

bool DesignCompilerWrapper::get_synthesis_result() const
{
   return synthesis_result;
}

std::string DesignCompilerWrapper::get_report_file(unsigned int report_type) const
{
   THROW_ASSERT(report_files.find(report_type) != report_files.end(), "Missing report file");
   return report_files.find(report_type)->second;
}
