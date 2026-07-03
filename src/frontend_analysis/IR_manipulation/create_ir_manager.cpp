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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file create_ir_manager.cpp
 * @brief Implementation of the class for creating the ir_manager starting from the source code files
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "create_ir_manager.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "cost_latency_table.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_manager.hpp"
#include "parse_ir.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "utility.hpp"

#include <algorithm>

create_ir_manager::create_ir_manager(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                     const DesignFlowManager& _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, CREATE_IR_MANAGER, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

void create_ir_manager::ComputeRelationships(DesignFlowStepSet& relationship,
                                             const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
create_ir_manager::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool create_ir_manager::HasToBeExecuted() const
{
   return true;
}

std::string create_ir_manager::createCostTable()
{
   std::string cost_table;
   if(!GetPointer<HLS_manager>(AppM) ||
      (parameters->IsParameter("disable-THR") && parameters->GetParameter<unsigned int>("disable-THR")))
   {
      return cost_table;
   }
   std::map<std::pair<std::string, std::string>, std::string> default_InstructionLatencyTable;
   auto latencies = string_to_container<std::vector<std::string>>(STR_cost_latency_table_default, ",");
   for(const auto& el : latencies)
   {
      auto key_value = string_to_container<std::vector<std::string>>(el, "=");
      THROW_ASSERT(key_value.size() == 2, "unexpected condition");
      auto op_bit = string_to_container<std::vector<std::string>>(key_value.at(0), "|");
      THROW_ASSERT(op_bit.size() == 2, "unexpected condition");
      default_InstructionLatencyTable[std::make_pair(op_bit.at(0), op_bit.at(1))] = key_value.at(1);
   }

   const auto HLS_D = GetPointer<HLS_manager>(AppM)->get_HLS_device();
   const auto TechManager = HLS_D->get_technology_manager();
   double clock_period = parameters->isOption(OPT_clock_period) ? parameters->getOption<double>(OPT_clock_period) : 10;
   /// manage loads and stores
   cost_table = "store_node|32=" + STR(clock_period);
   cost_table += ",load_node|32=" + STR(clock_period);
   cost_table += ",nop_node|32=" + STR(clock_period);
   for(const std::string op_name : {"mul_node", "add_node", "idiv_node", "irem_node", "shl_node", "shr_node",
                                    "and_node", "or_node", "xor_node", "select_node"})
   {
      for(auto fu_prec : {1, 8, 16, 32, 64})
      {
         auto component_name_op =
             "ui_" + op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
             ((op_name == "mul_node" || op_name == "idiv_node" || op_name == "irem_node") ? "_0" : "") +
             (op_name == "select_node" ? ("_" + STR(fu_prec)) : "");
         technology_nodeRef op_f_unit = TechManager->get_fu(component_name_op, LIBRARY_STD_FU);
         if(op_f_unit)
         {
            auto* op_fu = GetPointer<functional_unit>(op_f_unit);
            technology_nodeRef op_node = op_fu->get_operation(op_name);
            THROW_ASSERT(op_node, "missing " + op_name + " from " + component_name_op);
            auto* op = GetPointer<operation>(op_node);
            double op_delay = op->time_m->get_execution_time();
            cost_table += "," + op_name + "|" + STR(fu_prec) + "=" + STR(op_delay);
         }
         else
         {
            THROW_ASSERT(default_InstructionLatencyTable.find(std::make_pair(op_name, STR(fu_prec))) !=
                             default_InstructionLatencyTable.end(),
                         "");
            cost_table += "," + op_name + "|" + STR(fu_prec) + "=" +
                          default_InstructionLatencyTable.at(std::make_pair(op_name, STR(fu_prec)));
         }
      }
   }
   for(const std::string op_name : {"mul_node", "addsub_node", "fdiv_node"})
   {
      for(auto fu_prec : {32, 64})
      {
         auto component_name_op = "fp_" + op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" +
                                  STR(fu_prec) + ((op_name == "mul_node") ? "_200" : "_100");
         technology_nodeRef op_f_unit = TechManager->get_fu(component_name_op, LIBRARY_STD_FU);
         if(op_f_unit)
         {
            auto* op_fu = GetPointer<functional_unit>(op_f_unit);
            technology_nodeRef op_node = op_fu->get_operation(op_name == "addsub_node" ? "add_node" : op_name);
            THROW_ASSERT(op_node, "missing " + op_name + " from " + component_name_op);
            auto* op = GetPointer<operation>(op_node);
            auto op_cycles = op->time_m->get_cycles();
            double op_delay = op_cycles ? clock_period * op_cycles : op->time_m->get_execution_time();
            cost_table += ",F" + op_name + "|" + STR(fu_prec) + "=" + STR(op_delay);
         }
         else
         {
            THROW_ASSERT(default_InstructionLatencyTable.find(std::make_pair("F" + op_name, STR(fu_prec))) !=
                             default_InstructionLatencyTable.end(),
                         "");
            cost_table += ",F" + op_name + "|" + STR(fu_prec) + "=" +
                          default_InstructionLatencyTable.at(std::make_pair("F" + op_name, STR(fu_prec)));
         }
      }
   }
   return cost_table;
}

DesignFlowStep_Status create_ir_manager::Exec()
{
   const auto TM = AppM->get_ir_manager();

   if(!parameters->isOption(OPT_input_file))
   {
      THROW_ERROR("At least one source file has to be passed to the tool");
   }

   /// parsing of archive files
   if(parameters->isOption(OPT_archive_files))
   {
      const auto archive_files = parameters->getOption<CustomSet<std::string>>(OPT_archive_files);
      const auto output_temporary_directory =
          parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory);
      const auto temp_path = output_temporary_directory / "archives";
      std::filesystem::create_directories(temp_path);
      std::string command = "cd " + temp_path.string() + "\n";
      for(const auto& archive_file : archive_files)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reading " + archive_file);
         if(!std::filesystem::exists(archive_file))
         {
            THROW_ERROR("File " + archive_file + " does not exist");
         }

         command += " ar x " + std::filesystem::absolute(archive_file).lexically_proximate(temp_path).string() +
                    " || touch error &\n";
      }
      command += " wait\n if [ -e \"error\" ]; then exit -1; fi";
      if(IsError(PandaSystem(parameters, command)))
      {
         THROW_ERROR("ar returns an error during archive extraction.");
      }
      std::vector<std::filesystem::path> extracted_objects;
      for(const auto& archive : std::filesystem::directory_iterator{temp_path})
      {
         const auto fileExtension = archive.path().extension().string();
         if(fileExtension != ".o" && fileExtension != ".O")
         {
            continue;
         }
         extracted_objects.emplace_back(archive.path());
      }
      std::sort(extracted_objects.begin(), extracted_objects.end());
      for(const auto& archive_path : extracted_objects)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + archive_path.string());
         const auto TM_new = ParseIRFile(parameters, archive_path.string());
         TM->merge_ir_managers(TM_new);
      }
      if(!parameters->getOption<bool>(OPT_no_clean))
      {
         std::filesystem::remove_all(temp_path);
      }
   }

   if(parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_RAW)
   {
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
      {
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                           "*********************************************************************************");
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                           "*               Building internal representation from raw files                 *");
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                           "*********************************************************************************");
         }
         else
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           " =============== Building internal representation from raw files ===============");
         }
      }
      const auto raw_files = parameters->getOption<CustomSet<std::string>>(OPT_input_file);
      for(const auto& raw_file : raw_files)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Parsing " + raw_file);
         if(!std::filesystem::exists(std::filesystem::path(raw_file)))
         {
            THROW_ERROR("File " + raw_file + " does not exist");
         }
         ir_managerRef TM_tmp = ParseIRFile(parameters, raw_file);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging " + raw_file);
         TM->merge_ir_managers(TM_tmp);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged " + raw_file);
      }
   }
   else
   {
      CompilerWrapper compiler_wrapper(parameters,
                                       parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler));
      const auto cost_table = createCostTable();
      compiler_wrapper.FillIRManager(TM, AppM->input_files, cost_table);

      if(debug_level >= DEBUG_LEVEL_PEDANTIC)
      {
         const auto raw_file_name =
             parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / "after_raw_merge.raw";
         std::ofstream raw_file(raw_file_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "IR manager dumped for debug purpose");
         raw_file << TM;
         raw_file.close();
      }
   }

   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto arch_filename =
       parameters->isOption(OPT_architecture_xml) ?
           parameters->getOption<std::string>(OPT_architecture_xml) :
           (parameters->getOption<std::string>(OPT_output_temporary_directory) + "/architecture.xml");
   HLSMgr->module_arch = refcount<ModuleArchitecture>(new ModuleArchitecture(arch_filename));

   for(auto& [symbol, arch] : *HLSMgr->module_arch)
   {
      const auto fnode = TM->GetFunction(symbol);
      if(!fnode)
      {
         if(output_level >= OUTPUT_LEVEL_VERY_VERY_PEDANTIC)
         {
            THROW_WARNING("Function specified in architecture XML is missing in the IR: " + symbol);
         }
         continue;
      }
      const auto fd = GetPointer<function_val_node>(fnode);
      for(auto& [attr, val] : arch->attrs)
      {
         if(attr == FunctionArchitecture::func_pipeline_style)
         {
            if(val == "off")
            {
               fd->set_pipelining(false);
            }
            else
            {
               fd->set_pipelining(true);
               if(val == "frp")
               {
                  fd->set_pipeline_style(function_val_node::FRP_STYLE);
               }
               else if(val == "flp")
               {
                  fd->set_pipeline_style(function_val_node::FLP_STYLE);
               }
               else if(val == "stp")
               {
                  fd->set_pipeline_style(function_val_node::STP_STYLE);
               }
               else
               {
                  THROW_ERROR("Unsupported function pipelining style");
               }
            }
         }
         else if(attr == FunctionArchitecture::func_pipeline_ii)
         {
            const auto pipeline_ii = static_cast<unsigned>(std::stoul(val));
            fd->set_initiation_time(pipeline_ii);
         }
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
