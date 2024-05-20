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
 * @file create_tree_manager.cpp
 * @brief Implementation of the class for creating the tree_manager starting from the source code files
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "create_tree_manager.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "compiler_wrapper.hpp"
#include "cost_latency_table.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "parse_tree.hpp"
#include "parser_flow_step_factory.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"

#if HAVE_FROM_AADL_ASN_BUILT
#include "parser_flow_step.hpp"
#endif

create_tree_manager::create_tree_manager(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                         const DesignFlowManagerConstRef _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, CREATE_TREE_MANAGER, _design_flow_manager, _parameters),
      compiler_wrapper(
          new CompilerWrapper(parameters, parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler),
                              parameters->getOption<CompilerWrapper_OptimizationSet>(OPT_gcc_optimization_set)))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

create_tree_manager::~create_tree_manager() = default;

void create_tree_manager::ComputeRelationships(DesignFlowStepSet& relationship,
                                               const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto DFM = design_flow_manager.lock();
#if HAVE_FROM_AADL_ASN_BUILT
         if(parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_AADL)
         {
            const auto parser_factory =
                GetPointer<const ParserFlowStepFactory>(DFM->CGetDesignFlowStepFactory(DesignFlowStep::PARSER));
            for(const auto& input_file : AppM->input_files)
            {
               const auto file_format = parameters->GetFileFormat(input_file);
               if(file_format == Parameters_FileFormat::FF_AADL)
               {
                  const auto pfs_signature = ParserFlowStep::ComputeSignature(ParserFlowStep_Type::AADL, input_file);
                  auto parser_step = DFM->GetDesignFlowStep(pfs_signature);
                  const auto design_flow_graph = DFM->CGetDesignFlowGraph();
                  const auto design_flow_step =
                      parser_step != DesignFlowGraph::null_vertex() ?
                          design_flow_graph->CGetNodeInfo(parser_step)->design_flow_step :
                          parser_factory->CreateParserStep(ParserFlowStep_Type::AADL, input_file);
                  relationship.insert(design_flow_step);
               }
               else if(file_format == Parameters_FileFormat::FF_ASN)
               {
                  const auto pfs_signature = ParserFlowStep::ComputeSignature(ParserFlowStep_Type::ASN, input_file);
                  auto parser_step = DFM->GetDesignFlowStep(pfs_signature);
                  const auto design_flow_graph = DFM->CGetDesignFlowGraph();
                  const auto design_flow_step =
                      parser_step != DesignFlowGraph::null_vertex() ?
                          design_flow_graph->CGetNodeInfo(parser_step)->design_flow_step :
                          parser_factory->CreateParserStep(ParserFlowStep_Type::ASN, input_file);
                  relationship.insert(design_flow_step);
               }
            }
         }
#endif
         const auto design_flow_graph = DFM->CGetDesignFlowGraph();
         const auto technology_flow_step_factory =
             GetPointer<const TechnologyFlowStepFactory>(DFM->CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = DFM->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
create_tree_manager::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::make_pair(PRAGMA_SUBSTITUTION, WHOLE_APPLICATION));
#endif
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

bool create_tree_manager::HasToBeExecuted() const
{
   return true;
}

void create_tree_manager::createCostTable()
{
   if(GetPointer<HLS_manager>(AppM) &&
      (!parameters->IsParameter("disable-THR") || parameters->GetParameter<unsigned int>("disable-THR") == 0))
   {
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

      const HLS_deviceRef HLS_D = GetPointer<HLS_manager>(AppM)->get_HLS_device();
      const technology_managerRef TechManager = HLS_D->get_technology_manager();
      double clock_period =
          parameters->isOption(OPT_clock_period) ? parameters->getOption<double>(OPT_clock_period) : 10;
      /// manage loads and stores
      CostTable = "store_expr|32=" + STR(clock_period);
      CostTable += ",load_expr|32=" + STR(clock_period);
      CostTable += ",nop_expr|32=" + STR(clock_period);
      for(const std::string op_name : {"mult_expr", "plus_expr", "trunc_div_expr", "trunc_mod_expr", "lshift_expr",
                                       "rshift_expr", "bit_and_expr", "bit_ior_expr", "bit_xor_expr", "cond_expr"})
      {
         for(auto fu_prec : {1, 8, 16, 32, 64})
         {
            auto component_name_op =
                "ui_" + op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
                ((op_name == "mult_expr" || op_name == "trunc_div_expr" || op_name == "trunc_mod_expr") ? "_0" : "") +
                (op_name == "cond_expr" ? ("_" + STR(fu_prec)) : "");
            technology_nodeRef op_f_unit = TechManager->get_fu(component_name_op, LIBRARY_STD_FU);
            if(op_f_unit)
            {
               auto* op_fu = GetPointer<functional_unit>(op_f_unit);
               technology_nodeRef op_node = op_fu->get_operation(op_name);
               THROW_ASSERT(op_node, "missing " + op_name + " from " + component_name_op);
               auto* op = GetPointer<operation>(op_node);
               double op_delay = op->time_m->get_execution_time();
               CostTable += "," + op_name + "|" + STR(fu_prec) + "=" + STR(op_delay);
            }
            else
            {
               THROW_ASSERT(default_InstructionLatencyTable.find(std::make_pair(op_name, STR(fu_prec))) !=
                                default_InstructionLatencyTable.end(),
                            "");
               CostTable += "," + op_name + "|" + STR(fu_prec) + "=" +
                            default_InstructionLatencyTable.at(std::make_pair(op_name, STR(fu_prec)));
            }
         }
      }
      for(const std::string op_name : {"mult_expr", "plus_expr", "rdiv_expr"})
      {
         for(auto fu_prec : {32, 64})
         {
            auto component_name_op = "fp_" + op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" +
                                     STR(fu_prec) + ((op_name == "mult_expr") ? "_200" : "_100");
            technology_nodeRef op_f_unit = TechManager->get_fu(component_name_op, LIBRARY_STD_FU);
            if(op_f_unit)
            {
               auto* op_fu = GetPointer<functional_unit>(op_f_unit);
               technology_nodeRef op_node = op_fu->get_operation(op_name);
               THROW_ASSERT(op_node, "missing " + op_name + " from " + component_name_op);
               auto* op = GetPointer<operation>(op_node);
               auto op_cycles = op->time_m->get_cycles();
               double op_delay = op_cycles ? clock_period * op_cycles : op->time_m->get_execution_time();
               CostTable += ",F" + op_name + "|" + STR(fu_prec) + "=" + STR(op_delay);
            }
            else
            {
               THROW_ASSERT(default_InstructionLatencyTable.find(std::make_pair("F" + op_name, STR(fu_prec))) !=
                                default_InstructionLatencyTable.end(),
                            "");
               CostTable += "," + op_name + "|" + STR(fu_prec) + "=" +
                            default_InstructionLatencyTable.at(std::make_pair("F" + op_name, STR(fu_prec)));
            }
         }
      }
   }
}

DesignFlowStep_Status create_tree_manager::Exec()
{
   const auto TM = AppM->get_tree_manager();

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

         command += " ar x " + std::filesystem::path(archive_file).lexically_proximate(temp_path).string() +
                    " || touch error &\n";
      }
      command += " wait\n if [ -e \"error\" ]; then exit -1; fi";
      if(IsError(PandaSystem(parameters, command)))
      {
         THROW_ERROR("ar returns an error during archive extraction.");
      }
      for(const auto& archive : std::filesystem::directory_iterator{temp_path})
      {
         const auto fileExtension = archive.path().extension().string();
         if(fileExtension != ".o" && fileExtension != ".O")
         {
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + archive.path().string());
         const auto TM_new = ParseTreeFile(parameters, archive.path().string());
         TM->merge_tree_managers(TM_new);
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
         tree_managerRef TM_tmp = ParseTreeFile(parameters, raw_file);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging " + raw_file);
         TM->merge_tree_managers(TM_tmp);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged " + raw_file);
      }
   }
   else
   {
#if !RELEASE
      // if a XML configuration file has been specified for the GCC/CLANG parameters
      if(parameters->isOption(OPT_gcc_read_xml))
      {
         compiler_wrapper->ReadXml(parameters->getOption<std::string>(OPT_gcc_read_xml));
      }
#endif
      createCostTable();
      compiler_wrapper->FillTreeManager(TM, AppM->input_files, getCostTable());

#if !RELEASE
      if(parameters->isOption(OPT_gcc_write_xml))
      {
         compiler_wrapper->WriteXml(parameters->getOption<std::string>(OPT_gcc_write_xml));
      }
#endif

      if(debug_level >= DEBUG_LEVEL_PEDANTIC)
      {
         const auto raw_file_name =
             parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / "after_raw_merge.raw";
         std::ofstream raw_file(raw_file_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Tree-Manager dumped for debug purpose");
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
      const auto fd = GetPointer<function_decl>(fnode);
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
                  fd->set_simple_pipeline(true);
               }
            }
         }
         else if(attr == FunctionArchitecture::func_pipeline_ii)
         {
            const auto pipeline_ii = std::stoi(val);
            fd->set_initiation_time(pipeline_ii);
         }
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
