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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"

/// Header include
#include "create_tree_manager.hpp"

/// behavior include
#include "application_manager.hpp"

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// Intermediate Representation at raw level
#include "parse_tree.hpp"
#include "tree_manager.hpp"

#if HAVE_FROM_AADL_ASN_BUILT
/// parser include
#include "parser_flow_step.hpp"
#endif

/// Wrapper include
#include "compiler_wrapper.hpp"

/// Utility include
#include "Parameter.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#if HAVE_BAMBU_BUILT
#include "cost_latency_table.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_model.hpp"
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
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_AADL_ASN_BUILT
         if(parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_AADL)
         {
            const auto input_files = AppM->input_files;
            for(const auto& input_file : input_files)
            {
               const auto file_format = parameters->GetFileFormat(input_file.first);
               if(file_format == Parameters_FileFormat::FF_AADL)
               {
                  const auto signature = ParserFlowStep::ComputeSignature(ParserFlowStep_Type::AADL, input_file.first);
                  vertex parser_step = design_flow_manager.lock()->GetDesignFlowStep(signature);
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step =
                      parser_step != NULL_VERTEX ?
                          design_flow_graph->CGetDesignFlowStepInfo(parser_step)->design_flow_step :
                          design_flow_manager.lock()->CreateFlowStep(signature);
                  relationship.insert(design_flow_step);
               }
               else if(file_format == Parameters_FileFormat::FF_ASN)
               {
                  const auto signature = ParserFlowStep::ComputeSignature(ParserFlowStep_Type::ASN, input_file.first);
                  vertex parser_step = design_flow_manager.lock()->GetDesignFlowStep(signature);
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step =
                      parser_step != NULL_VERTEX ?
                          design_flow_graph->CGetDesignFlowStepInfo(parser_step)->design_flow_step :
                          design_flow_manager.lock()->CreateFlowStep(signature);
                  relationship.insert(design_flow_step);
               }
            }
         }
#endif
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ?
                 design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
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
#if HAVE_ZEBU_BUILT
         relationships.insert(std::make_pair(SIZEOF_SUBSTITUTION, WHOLE_APPLICATION));
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
#if HAVE_BAMBU_BUILT
   if(GetPointer<HLS_manager>(AppM) &&
      (!parameters->IsParameter("disable-THR") || parameters->GetParameter<unsigned int>("disable-THR") == 0))
   {
      std::map<std::pair<std::string, std::string>, std::string> default_InstructionLatencyTable;
      auto latencies = SplitString(STR_cost_latency_table_default, ",");
      for(const auto& el : latencies)
      {
         auto key_value = SplitString(el, "=");
         THROW_ASSERT(key_value.size() == 2, "unexpected condition");
         auto op_bit = SplitString(key_value.at(0), "|");
         THROW_ASSERT(op_bit.size() == 2, "unexpected condition");
         default_InstructionLatencyTable[std::make_pair(op_bit.at(0), op_bit.at(1))] = key_value.at(1);
      }

      const HLS_targetRef HLS_T = GetPointer<HLS_manager>(AppM)->get_HLS_target();
      const technology_managerRef TechManager = HLS_T->get_technology_manager();
      double clock_period =
          parameters->isOption(OPT_clock_period) ? parameters->getOption<double>(OPT_clock_period) : 10;
      /// manage loads and stores
      CostTable = "store_expr|32=" + STR(clock_period);
      CostTable += ",load_expr|32=" + STR(clock_period);
      CostTable += ",nop_expr|32=" + STR(clock_period);
      for(const std::string& op_name : {"mult_expr", "plus_expr", "trunc_div_expr", "trunc_mod_expr", "lshift_expr",
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
      for(const std::string& op_name : {"mult_expr", "plus_expr", "rdiv_expr"})
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

#endif
}

DesignFlowStep_Status create_tree_manager::Exec()
{
   const tree_managerRef TreeM = AppM->get_tree_manager();

   if(!parameters->isOption(OPT_input_file))
   {
      THROW_ERROR("At least one source file has to be passed to the tool");
   }

   /// parsing of archive files
   if(parameters->isOption(OPT_archive_files))
   {
      const auto archive_files = parameters->getOption<const CustomSet<std::string>>(OPT_archive_files);
      for(const auto& archive_file : archive_files)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Reading " + archive_file);
         if(!boost::filesystem::exists(boost::filesystem::path(archive_file)))
         {
            THROW_ERROR("File " + archive_file + " does not exist");
         }
         std::string temporary_directory_pattern;
         temporary_directory_pattern =
             parameters->getOption<std::string>(OPT_output_temporary_directory) + "/temp-archive-dir";
         // The %s are required by the mkdtemp function
         boost::filesystem::path temp_path = temporary_directory_pattern + "-%%%%-%%%%-%%%%-%%%%";
         boost::filesystem::path temp_path_obtained = boost::filesystem::unique_path(temp_path);
         boost::filesystem::create_directories(temp_path_obtained);
         boost::filesystem::path local_archive_file = GetPath(archive_file);

         std::string command = "cd " + temp_path_obtained.string() + "; ar x " + local_archive_file.string();
         int ret = PandaSystem(parameters, command);
         if(IsError(ret))
         {
            THROW_ERROR("ar returns an error during archive extraction ");
         }
         for(auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(temp_path_obtained), {}))
         {
            auto fileExtension = GetExtension(entry.path());
            if(fileExtension != "o" && fileExtension != "O")
            {
               continue;
            }
            const tree_managerRef TM_new = ParseTreeFile(parameters, entry.path().string());
            TreeM->merge_tree_managers(TM_new);
         }
         if(not(parameters->getOption<bool>(OPT_no_clean)))
         {
            boost::filesystem::remove_all(temp_path_obtained);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Read " + archive_file);
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
      const auto raw_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
      for(const auto& raw_file : raw_files)
      {
         if(!boost::filesystem::exists(boost::filesystem::path(raw_file)))
         {
            THROW_ERROR("File " + raw_file + " does not exist");
         }
         tree_managerRef TreeM_tmp = ParseTreeFile(parameters, raw_file);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging " + raw_file);
         TreeM->merge_tree_managers(TreeM_tmp);
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
      compiler_wrapper->FillTreeManager(TreeM, AppM->input_files, getCostTable());

#if !RELEASE
      if(parameters->isOption(OPT_gcc_write_xml))
      {
         compiler_wrapper->WriteXml(parameters->getOption<std::string>(OPT_gcc_write_xml));
      }
#endif

      if(debug_level >= DEBUG_LEVEL_PEDANTIC)
      {
         std::string raw_file_name =
             parameters->getOption<std::string>(OPT_output_temporary_directory) + "after_raw_merge.raw";
         std::ofstream raw_file(raw_file_name.c_str());
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Tree-Manager dumped for debug purpose");
         raw_file << TreeM;
         raw_file.close();
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
