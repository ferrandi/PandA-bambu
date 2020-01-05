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
 * @file xst_wrapper.cpp
 * @brief Implementation of the wrapper to XST
 *
 * Implementation of the methods to wrap XST by Xilinx
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Includes the class definition
#include "xst_wrapper.hpp"

#include "config_HAVE_XILINX.hpp"
#include "config_XILINX_SETTINGS.hpp"

#include "ToolManager.hpp"
#include "xml_script_command.hpp"

#include "Parameter.hpp"

#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "utility.hpp"

// constructor
xst_wrapper::xst_wrapper(const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device) : XilinxWrapper(_Param, XST_TOOL_ID, _device, _output_dir, "xst")
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the XST wrapper...");
}

// destructor
xst_wrapper::~xst_wrapper() = default;

void xst_wrapper::init_reserved_vars()
{
   XilinxWrapper::init_reserved_vars();
   ADD_RES_VAR(PARAM_xst_log_file);
}

void xst_wrapper::GenerateProjectFile(const DesignParametersRef& dp)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating project file for xst");
   std::string HDL_files = dp->get_value(PARAM_HDL_files);
   std::vector<std::string> files = convert_string_to_vector<std::string>(HDL_files, ";");

   std::string top_name = dp->get_value(PARAM_top_id);
   std::string xst_tmpdir = dp->get_value(PARAM_xst_tmpdir);
   std::string project_filename = xst_tmpdir + "/" + top_name + ".prj";
   std::ofstream prj_file(project_filename.c_str());
   const size_t file_number = files.size();
   for(unsigned int v = 0; v < file_number; v++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding file " + files[v]);
      boost::filesystem::path file_path(files[v]);
      std::string extension = GetExtension(file_path);
      std::string filename;
      std::string language;
      if(extension == "vhd" || extension == "vhdl" || extension == "VHD" || extension == "VHDL")
      {
         language = "VHDL";
      }
      else if(extension == "v" || extension == "V" || extension == "sv" || extension == "SV")
      {
         language = "VERILOG";
      }
      else
      {
         THROW_ERROR("Extension not recognized! " + extension);
      }
      filename = file_path.string();
      prj_file << language << " "
               << "work"
               << " " << filename << std::endl;
   }
   prj_file.close();
   dp->assign(PARAM_xst_prj_file, project_filename, false);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated project file for xst");
}

void xst_wrapper::EvaluateVariables(const DesignParametersRef dp)
{
   std::string top_name = dp->get_value(PARAM_top_id);
   dp->assign(PARAM_xst_tmpdir, output_dir, false);
   dp->assign(PARAM_xst_hdpdir, output_dir, false);
   dp->assign(PARAM_xst_log_file, output_dir + "/" + top_name + ".log", false);
   GenerateProjectFile(dp);
   dp->assign(PARAM_xst_report, output_dir + "/" + top_name + "_xst.xrpt", false);
}

std::string xst_wrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec() << " -ifn " << dp->parameter_values[SCRIPT_FILENAME];
   for(const auto& option : xml_tool_options)
   {
      if(option->checkCondition(dp))
      {
         std::string value = toString(option, dp);
         replace_parameters(dp, value);
         s << " " << value;
      }
   }
   s << std::endl;
   return s.str();
}
