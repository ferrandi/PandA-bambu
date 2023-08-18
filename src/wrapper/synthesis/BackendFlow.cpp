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
 * @file BackendFlow.cpp
 * @brief This file contains the implementation of the methods for the class defining the backend flow
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "BackendFlow.hpp"

#include "DesignParameters.hpp"

/// wrapper/synthesis/altera includes
#include "AlteraBackendFlow.hpp"
#include "quartus_13_wrapper.hpp"
#include "quartus_wrapper.hpp"

/// wrapper/synthesis/xilinx includes
#include "XilinxBackendFlow.hpp"
#if HAVE_TASTE
#include "xilinx_taste_backend_flow.hpp"
#endif

#include "HDL_manager.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "target_manager.hpp"
#include "technology_manager.hpp"

/// implemented flows
#include "BashBackendFlow.hpp"
#include "LatticeBackendFlow.hpp"
#include "NanoXploreBackendFlow.hpp"

/// target devices
#include "FPGA_device.hpp"

#include "ToolManager.hpp"
/// supported synthesis steps
#include "SynthesisTool.hpp"
// ASIC
#include "DesignCompilerWrapper.hpp"
// Xilinx
#include "map_wrapper.hpp"
#include "ngdbuild_wrapper.hpp"
#include "par_wrapper.hpp"
#include "trce_wrapper.hpp"
#include "vivado_flow_wrapper.hpp"
#include "xst_wrapper.hpp"
// Altera
#include "quartus_13_report_wrapper.hpp"
#include "quartus_report_wrapper.hpp"
// Lattice
#include "lattice_flow_wrapper.hpp"
// NanoXplore
#include "nxpython_flow_wrapper.hpp"
// Generic
#include "bash_flow_wrapper.hpp"

#include "area_model.hpp"
#include "target_device.hpp"
#include "technology_node.hpp"
#include "time_model.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "exceptions.hpp"

#include <filesystem>

#include <iosfwd>
#include <utility>


#include "Parameter.hpp"
#include "cpu_time.hpp"
#include "fileIO.hpp"

#include <boost/algorithm/string/case_conv.hpp>

BackendFlow::BackendFlow(const ParameterConstRef _Param, std::string _flow_name, const target_managerRef _manager)
    : Param(_Param),
      output_level(_Param->getOption<unsigned int>(OPT_output_level)),
      flow_name(std::move(_flow_name)),
      out_dir(Param->getOption<std::string>(OPT_output_directory) + "/" + flow_name),
      target(_manager),
      root(nullptr),
      default_flow_parameters(DesignParametersRef(new DesignParameters))
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));

   if(!std::filesystem::exists(out_dir))
   {
      std::filesystem::create_directories(out_dir);
   }
}

BackendFlow::~BackendFlow() = default;

BackendFlow::type_t BackendFlow::DetermineBackendFlowType(const target_deviceRef device, const ParameterConstRef
#if HAVE_TASTE
                                                                                             parameters
#endif
)
{
   if(GetPointer<FPGA_device>(device))
   {
      if(!device->has_parameter("vendor"))
      {
         THROW_ERROR("FPGA device vendor not specified");
      }
      auto vendor = device->get_parameter<std::string>("vendor");
      boost::algorithm::to_lower(vendor);
      if(vendor == "xilinx")
      {
#if HAVE_TASTE
         if(parameters->isOption(OPT_generate_taste_architecture) and
            parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            return XILINX_TASTE_FPGA;
         }
#endif
         return XILINX_FPGA;
      }
      else if(vendor == "altera")
      {
         return ALTERA_FPGA;
      }
      else if(vendor == "lattice")
      {
         return LATTICE_FPGA;
      }
      else if(vendor == "nanoxplore")
      {
         return NANOXPLORE_FPGA;
      }
      else if(vendor == "generic")
      {
         return GENERIC;
      }
      else
      {
         THROW_ERROR("FPGA device vendor \"" + vendor + "\" not supported");
      }
   }
   return UNKNOWN;
}

BackendFlowRef BackendFlow::CreateFlow(const ParameterConstRef Param, const std::string& flow_name,
                                       const target_managerRef target)
{
   type_t type = DetermineBackendFlowType(target->get_target_device(), Param);
   switch(type)
   {
      case XILINX_FPGA:
         return BackendFlowRef(new XilinxBackendFlow(Param, flow_name, target));
#if HAVE_TASTE
      case XILINX_TASTE_FPGA:
         return BackendFlowRef(new XilinxTasteBackendFlow(Param, flow_name, target));
#endif
      case ALTERA_FPGA:
         return BackendFlowRef(new AlteraBackendFlow(Param, flow_name, target));
      case LATTICE_FPGA:
         return BackendFlowRef(new LatticeBackendFlow(Param, flow_name, target));
      case NANOXPLORE_FPGA:
         return BackendFlowRef(new NanoXploreBackendFlow(Param, flow_name, target));
      case GENERIC:
         return BackendFlowRef(new BashBackendFlow(Param, flow_name, target));
      case UNKNOWN:
      default:
         THROW_UNREACHABLE("Backend flow not supported");
   }
   return BackendFlowRef();
}

std::string BackendFlow::GenerateSynthesisScripts(const std::string& fu_name, const structural_managerRef SM,
                                                  const std::list<std::string>& hdl_files,
                                                  const std::list<std::string>& aux_files)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating synthesis scripts");
   auto resource_name = "_" + fu_name;
   std::string synthesis_file_list;
   for(const auto& hdl_file : hdl_files)
   {
      synthesis_file_list += hdl_file + ";";
   }
   const structural_objectRef obj = SM->get_circ();
   actual_parameters = DesignParametersRef(new DesignParameters);
   actual_parameters->component_name = obj->get_id();
   if(flow_name.size())
   {
      actual_parameters->chain_name = flow_name;
   }

   for(const auto& aux_file : aux_files)
   {
      synthesis_file_list += aux_file + ";";
   }
   actual_parameters->parameter_values[PARAM_HDL_files] = synthesis_file_list;
   const technology_managerRef TM = target->get_technology_manager();
   std::string library = TM->get_library(resource_name);
   bool is_combinational = false;
   auto is_time_unit_PS = target->get_target_device()->has_parameter("USE_TIME_UNIT_PS") &&
                          target->get_target_device()->get_parameter<int>("USE_TIME_UNIT_PS") == 1;
   if(library.size())
   {
      const technology_nodeRef tn = TM->get_fu(resource_name, library);
      actual_parameters->parameter_values[PARAM_clk_period] =
          STR((is_time_unit_PS ? 1000 : 1) * GetPointer<functional_unit>(tn)->get_clock_period());
      actual_parameters->parameter_values[PARAM_clk_period_ps] =
          STR(1000 * GetPointer<functional_unit>(tn)->get_clock_period());
      if(GetPointer<functional_unit>(tn)->logical_type == functional_unit::COMBINATIONAL)
      {
         is_combinational = true;
      }
   }
   else
   {
      library = TM->get_library(fu_name);
      if(library.size())
      {
         const technology_nodeRef tn = TM->get_fu(fu_name, library);
         actual_parameters->parameter_values[PARAM_clk_period] =
             STR((is_time_unit_PS ? 1000 : 1) * GetPointer<functional_unit>(tn)->get_clock_period());
         actual_parameters->parameter_values[PARAM_clk_period_ps] =
             STR(1000 * GetPointer<functional_unit>(tn)->get_clock_period());
         if(GetPointer<functional_unit>(tn)->logical_type == functional_unit::COMBINATIONAL)
         {
            is_combinational = true;
         }
         if(GetPointer<functional_unit>(tn)->fu_template_name.size())
         {
            actual_parameters->parameter_values[PARAM_fu] = GetPointer<functional_unit>(tn)->fu_template_name;
         }
         else
         {
            actual_parameters->parameter_values[PARAM_fu] = fu_name;
         }
      }
   }
   actual_parameters->parameter_values[PARAM_is_combinational] = STR(is_combinational);
   bool time_constrained = false;
   if(actual_parameters->parameter_values.find(PARAM_clk_period) != actual_parameters->parameter_values.end() and
      boost::lexical_cast<double>(actual_parameters->parameter_values[PARAM_clk_period]) != 0.0)
   {
      time_constrained = true;
   }
   actual_parameters->parameter_values[PARAM_time_constrained] = STR(time_constrained);
   if(!time_constrained)
   {
      actual_parameters->parameter_values[PARAM_clk_period] =
          STR((is_time_unit_PS ? 1000 : 1) * PARAM_clk_period_default);
      actual_parameters->parameter_values[PARAM_clk_period_ps] = STR(1000 * PARAM_clk_period_default);
   }
   actual_parameters->parameter_values[PARAM_clk_freq] =
       STR((is_time_unit_PS ? 1000 : 1) * 1000 /
           boost::lexical_cast<double>(actual_parameters->parameter_values[PARAM_clk_period]));

   if(Param->isOption(OPT_clock_name))
   {
      actual_parameters->parameter_values[PARAM_clk_name] = Param->getOption<std::string>(OPT_clock_name);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_clk_name] = CLOCK_PORT_NAME;
   }
   bool connect_iob = false;
   if(Param->isOption(OPT_connect_iob) && Param->getOption<bool>(OPT_connect_iob))
   {
      connect_iob = true;
   }
   actual_parameters->parameter_values[PARAM_connect_iob] = STR(connect_iob);
   if(Param->isOption(OPT_top_design_name))
   {
      actual_parameters->parameter_values[PARAM_top_id] = Param->getOption<std::string>(OPT_top_design_name);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_top_id] = actual_parameters->component_name;
   }
   if(Param->isOption(OPT_backend_script_extensions))
   {
      actual_parameters->parameter_values[PARAM_has_script_extensions] = STR(true);
      actual_parameters->parameter_values[PARAM_backend_script_extensions] =
          Param->getOption<std::string>(OPT_backend_script_extensions);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_has_script_extensions] = STR(false);
   }
   if(Param->isOption(OPT_VHDL_library))
   {
      actual_parameters->parameter_values[PARAM_has_VHDL_library] = STR(true);
      actual_parameters->parameter_values[PARAM_VHDL_library] = Param->getOption<std::string>(OPT_VHDL_library);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_has_VHDL_library] = STR(false);
   }
   if(Param->isOption(OPT_parallel_backend))
   {
      if(Param->getOption<bool>(OPT_parallel_backend))
      {
         actual_parameters->parameter_values[PARAM_parallel_backend] = STR(true);
      }
      else
      {
         actual_parameters->parameter_values[PARAM_parallel_backend] = STR(false);
      }
   }

   InitDesignParameters();

   const auto ret = CreateScripts(actual_parameters);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated synthesis scripts");
   return ret;
}

void BackendFlow::ExecuteSynthesis()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing synthesis");
   for(auto& step : steps)
   {
      step->tool->CheckExecution();
   }

   ToolManagerRef tool(new ToolManager(Param));
   tool->configure(generated_synthesis_script, "");
   std::vector<std::string> parameters, input_files, output_files;
   const std::string synthesis_file_output =
       Param->getOption<std::string>(OPT_output_temporary_directory) + "/synthesis_output";
   tool->execute(parameters, input_files, output_files, synthesis_file_output, false);

   CheckSynthesisResults();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Executed synthesis");
}

area_modelRef BackendFlow::get_used_resources() const
{
   return area_m;
}

time_modelRef BackendFlow::get_timing_results() const
{
   return time_m;
}

void BackendFlow::parse_flow(const XMLDomParserRef parser)
{
   parser->Exec();
   root = parser->get_document()->get_root_node(); // deleted by DomParser.

   const xml_node::node_list list = root->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(!child || child->get_name() != "flow")
      {
         continue;
      }
      std::string name;
      LOAD_XVM(name, child);
      if(name != flow_name)
      {
         continue;
      }
      xload(child);
   }
}

void BackendFlow::xload(const xml_element* node)
{
   LOAD_XVFM(default_flow_parameters->chain_name, node, name);
   THROW_ASSERT(default_flow_parameters->chain_name == flow_name,
                "wrong values: " + default_flow_parameters->chain_name + " vs. " + flow_name);

   const xml_node::node_list list = node->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(!child)
      {
         continue;
      }

      if(child->get_name() == "config")
      {
         LOAD_XVM(out_dir, child);
      }

      /// list of supported synthesis steps
      if(child->get_name() == "step")
      {
         BackendStepRef step = BackendStepRef(new BackendStep);
         std::string id;
         LOAD_XVM(id, child);
         step->name = id;

         std::string config;
         if(!CE_XVM(config, child))
         {
            THROW_ERROR("Missing configuration for component " + id);
         }
         LOAD_XVM(config, child);
         step->config_name = config;

         std::string script_name;
         if(CE_XVM(script_name, child))
         {
            LOAD_XVM(script_name, child);
            step->script_name = config;
         }

         SynthesisTool::type_t type = SynthesisTool::UNKNOWN;
         if(id == DESIGN_COMPILER_TOOL_ID)
         {
            type = SynthesisTool::DESIGN_COMPILER;
            if(step->script_name.size() == 0)
            {
               step->script_name = "script.dc";
            }
         }
         else if(id == XST_TOOL_ID)
         {
            type = SynthesisTool::XST;
            if(step->script_name.size() == 0)
            {
               step->script_name = "xst.tcl";
            }
         }
         else if(id == NGDBUILD_TOOL_ID)
         {
            type = SynthesisTool::NGDBUILD;
         }
         else if(id == MAP_TOOL_ID)
         {
            type = SynthesisTool::MAP;
         }
         else if(id == TRCE_TOOL_ID)
         {
            type = SynthesisTool::TRCE;
         }
         else if(id == PAR_TOOL_ID)
         {
            type = SynthesisTool::PAR;
         }
         else if(id == VIVADO_FLOW_TOOL_ID)
         {
            type = SynthesisTool::VIVADO_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "vivado.tcl";
            }
         }
         else if(id == QUARTUS_SETUP_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_SETUP;
            if(step->script_name.size() == 0)
            {
               step->script_name = "quartus_setup.tcl";
            }
         }
         else if(id == QUARTUS_13_SETUP_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_13_SETUP;
            if(step->script_name.size() == 0)
            {
               step->script_name = "quartus_setup.tcl";
            }
         }
         else if(id == QUARTUS_FLOW_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "quartus_flow.tcl";
            }
         }
         else if(id == QUARTUS_13_FLOW_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_13_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "quartus_flow.tcl";
            }
         }
         else if(id == QUARTUS_POWER_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_POW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "quartus_pow_arguments";
            }
         }
         else if(id == QUARTUS_REPORT_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_STA;
            if(step->script_name.size() == 0)
            {
               step->script_name = "report_sta.tcl";
            }
         }
         else if(id == QUARTUS_13_REPORT_TOOL_ID)
         {
            type = SynthesisTool::QUARTUS_13_STA;
            if(step->script_name.size() == 0)
            {
               step->script_name = "report_sta.tcl";
            }
         }
         else if(id == LATTICE_FLOW_TOOL_ID)
         {
            type = SynthesisTool::LATTICE_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "project.tcl";
            }
         }
         else if(id == NXPYTHON_FLOW_TOOL_ID)
         {
            type = SynthesisTool::NXPYTHON_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "script.py";
            }
         }
         else if(id == BASH_FLOW_TOOL_ID)
         {
            type = SynthesisTool::BASH_FLOW;
            if(step->script_name.size() == 0)
            {
               step->script_name = "bash_script.sh";
            }
         }
         else
         {
            THROW_ERROR("Step <" + id + "> is currently not supported");
         }

         step->tool = SynthesisTool::create_synthesis_tool(type, Param, flow_name, target->get_target_device());
         /// update with the actual name of the output directory
         step->out_dir = step->tool->get_output_directory();

         /// parse the proper script configuration for the tool in the device configuration XML
         step->tool->xload(root, step->config_name);

         add_backend_step(step);
      }
   }
}

std::string BackendFlow::get_flow_name() const
{
   return flow_name;
}

void BackendFlow::add_backend_step(const BackendStepRef& step)
{
   /// add the step after all the previous ones
   steps.push_back(step);
}

std::string BackendFlow::CreateScripts(const DesignParametersRef dp)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                  "-->creating scripts for module \"" + dp->component_name + "\" on chain \"" + dp->chain_name + "\"");

   CustomOrderedSet<std::string> module_undefined_parameters = undefined_parameters;
   DesignParametersRef exec_params = default_flow_parameters->clone();
   exec_params->component_name = dp->component_name;
   THROW_ASSERT(exec_params->chain_name == dp->chain_name,
                "Mismatching!! exec = \"" + exec_params->chain_name + "\" vs. dp = \"" + dp->chain_name + "\"");

   for(auto p = dp->parameter_values.begin(); p != dp->parameter_values.end(); ++p)
   {
      exec_params->parameter_values[p->first] = p->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "-->setting parameter \"" + p->first + "\" to value \"" + p->second + "\"");
      if(module_undefined_parameters.find(p->first) != module_undefined_parameters.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                        "---removed parameter \"" + p->first + "\" from undefined parameters");
         module_undefined_parameters.erase(p->first);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
   }

   if(module_undefined_parameters.size() > 0)
   {
      for(const auto& module_undefined_parameter : module_undefined_parameters)
      {
         PRINT_MSG("missing definition for parameter " + module_undefined_parameter);
      }
      THROW_ERROR("Some parameters still need to be defined: " + STR(module_undefined_parameters.size()));
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---all the parameters have been correctly set");
   }

   std::ostringstream script;
   script << "#!/bin/bash" << std::endl;
   script << "##########################################################" << std::endl;
   script << "#     Automatically generated by the PandA framework     #" << std::endl;
   script << "##########################################################" << std::endl << std::endl;
   script << "# Synthesis script for COMPONENT: " << exec_params->component_name << std::endl << std::endl;

   WriteFlowConfiguration(script);

   for(const auto& step : steps)
   {
      THROW_ASSERT(step->tool, "Tool not valid for step: " + step->name);
      script << "# STEP: " << step->name << std::endl;

      /// output directory
      if(!std::filesystem::exists(step->out_dir))
      {
         THROW_ERROR("Output directory \"" + step->out_dir + "\" has not been created!");
      }
      std::filesystem::create_directory(step->out_dir);

      script << "cd " << GetCurrentPath() << std::endl;

      /// script file
      std::string script_path;
      if(step->script_name.size())
      {
         script_path = step->tool->get_output_directory() + "/" + step->script_name;
      }
      step->tool->generate_synthesis_script(exec_params, script_path);

      script << step->tool->get_command_line(exec_params) << std::endl;
   }

   // Write the synthesis script
   generated_synthesis_script = std::string("./synthesize");
   if(exec_params->chain_name.size())
   {
      generated_synthesis_script += std::string("_") + exec_params->chain_name;
   }
   if(exec_params->component_name.size())
   {
      generated_synthesis_script += std::string("_") + exec_params->component_name;
   }
   generated_synthesis_script += std::string(".sh");
   generated_synthesis_script = GetPath(generated_synthesis_script);

   std::ofstream file_stream;
   file_stream.open(generated_synthesis_script.c_str());
   file_stream << script.str() << std::endl;
   file_stream.close();

   ToolManagerRef tool(new ToolManager(Param));
   tool->configure("chmod", "");
   std::vector<std::string> parameters, input_files, output_files;
   parameters.push_back("+x");
   parameters.push_back(generated_synthesis_script);
   input_files.push_back(generated_synthesis_script);
   tool->execute(parameters, input_files, output_files,
                 Param->getOption<std::string>(OPT_output_temporary_directory) + "/synthesis_script_generation_output");

   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      create_xml_scripts(GetPath("exported_flow.xml"));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                  "<--Completed the generation of scripts for module \"" + exec_params->component_name +
                      "\" on chain \"" + exec_params->chain_name + "\"");

   return generated_synthesis_script;
}

void BackendFlow::set_initial_parameters(const DesignParametersRef& _flow_parameters,
                                         const CustomOrderedSet<std::string>& _undefined_parameters)
{
   default_flow_parameters = _flow_parameters;
   undefined_parameters = _undefined_parameters;
}

void BackendFlow::create_xml_scripts(const std::string& xml_file)
{
   xml_document doc;
   doc.set_encoding("UTF-8");
   doc.set_name("synthesis_tools");
   xml_element* out_root = doc.add_child_element("synthesis_tools");

   for(const auto& step : steps)
   {
      THROW_ASSERT(step->tool, "Tool not valid for step: " + step->name);
      out_root->add_child_element(step->tool->xwrite());
   }

   doc.write_to_file_formatted(xml_file);
}


void BackendFlow::InitDesignParameters()
{
   /// nothing to do
}
