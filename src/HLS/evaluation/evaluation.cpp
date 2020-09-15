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
 * @file evaluation.cpp
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "evaluation.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"

#include <boost/filesystem/operations.hpp>

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// constants include
#include "bambu_results_xml.hpp"

/// hls includes
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"

/// polixml include
#include "xml_document.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "xml_helper.hpp"

/// HLS include
#include "SimulationInformation.hpp"
#include "hls_flow_step_factory.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// technology/physical_library/models includes
#include "area_model.hpp"
#include "time_model.hpp"

/// wrapper/synthesis include
#include "BackendFlow.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

Evaluation::Evaluation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::EVALUATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

Evaluation::~Evaluation() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> Evaluation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::string objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         switch(parameters->getOption<Evaluation_Mode>(OPT_evaluation_mode))
         {
            case Evaluation_Mode::DRY_RUN:
            {
               ret.insert(std::make_tuple(HLSFlowStep_Type::DRY_RUN_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
               break;
            }
#if HAVE_EXPERIMENTAL
            case Evaluation_Mode::ESTIMATION:
            {
               for(const auto objective : objective_vector)
               {
                  if(objective == "AREA")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::AREA_ESTIMATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else if(objective == "CLOCK_SLACK")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::CLOCK_SLACK_ESTIMATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else if(objective == "TIME")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::TIME_ESTIMATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else
                     THROW_ERROR("Estimation objective not yet supported " + objective);
               }
               break;
            }
#endif
            case Evaluation_Mode::EXACT:
            {
               for(const auto& objective : objective_vector)
               {
                  if(false)
                  {
                  }
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
                  else if(objective == "AREA")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT && HAVE_SIMULATION_WRAPPER_BUILT
                  else if(objective == "AREAxTIME")
                  {
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  }
#endif
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
                  else if(objective == "BRAMS")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else if(objective == "CLOCK_SLACK")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_SIMULATION_WRAPPER_BUILT
                  else if(objective == "CYCLES" || objective == "TOTAL_CYCLES")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
                  else if(objective == "DSPS")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_EXPERIMENTAL
                  else if(objective == "EDGES_REDUCTION_EVALUATION")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::EDGES_REDUCTION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::ALL_FUNCTIONS));
#endif
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
                  else if(objective == "FREQUENCY")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else if(objective == "PERIOD")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  else if(objective == "REGISTERS")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_EXPERIMENTAL
                  else if(objective == "NUM_AF_EDGES")
                     ret.insert(std::make_tuple(HLSFlowStep_Type::NUM_AF_EDGES_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::ALL_FUNCTIONS));
#endif
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT && HAVE_SIMULATION_WRAPPER_BUILT
                  else if(objective == "TIME" || objective == "TOTAL_TIME")
                  {
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                     ret.insert(std::make_tuple(HLSFlowStep_Type::SYNTHESIS_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
                  }
#endif
                  else
                     THROW_ERROR("Evaluaton objective not yet supported " + objective);
               }
               break;
            }
            case Evaluation_Mode::NONE:
            {
               THROW_UNREACHABLE("");
               break;
            }
            default:
               THROW_UNREACHABLE("");
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
#if HAVE_VCD_BUILT
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_UTILITY, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
#endif
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status Evaluation::Exec()
{
   bool printed_area = false;
   bool printed_frequency = false;
   std::string objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
   const auto& evaluations = HLSMgr->evaluations;
   for(const auto& objective : objective_vector)
   {
      const auto evaluation = [&]() -> double {
         if(objective == "AREAxTIME" or objective == "TIME" or objective == "TOTAL_TIME")
         {
            return 0.0;
         }
         THROW_ASSERT(HLSMgr->evaluations.find(objective) != HLSMgr->evaluations.end(), objective);
         const auto local_evaluations = HLSMgr->evaluations.at(objective);
         THROW_ASSERT(local_evaluations.size() == 1, objective);
         return local_evaluations[0];
      }();
      if(objective == "CYCLES")
      {
         THROW_ASSERT(evaluations.find("TOTAL_CYCLES") != evaluations.end(), "");
         THROW_ASSERT(evaluations.at("TOTAL_CYCLES").size() == 1, "");
         unsigned long long int tot_cycles = static_cast<unsigned long long int>(evaluations.at("TOTAL_CYCLES")[0]);
         THROW_ASSERT(evaluations.find("NUM_EXECUTIONS") != evaluations.end(), "");
         THROW_ASSERT(evaluations.at("NUM_EXECUTIONS").size() == 1, "");
         unsigned long long int num_executions = static_cast<unsigned long long int>(evaluations.at("NUM_EXECUTIONS")[0]);
         auto avg_cycles = static_cast<unsigned long long int>(evaluation);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Total cycles             : " + STR(tot_cycles) + " cycles");
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of executions     : " + STR(num_executions));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Average execution        : " + STR(avg_cycles) + " cycles");
      }
      else if((objective == "AREA" or objective == "AREAxTIME") and !printed_area)
      {
         printed_area = true;
         //         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Area                     : " + STR(evaluations.at("AREA").at(0) ));
         if(evaluations.find("SLICE") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("SLICE").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Slices                   : " + STR(evaluations.at("SLICE").at(0)));
         }
         if(evaluations.find("SLICE_LUTS") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("SLICE_LUTS").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Luts                     : " + STR(evaluations.at("SLICE_LUTS").at(0)));
         }
         if(evaluations.find("LUT_FF_PAIRS") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("LUT_FF_PAIRS").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Lut FF Pairs             : " + STR(evaluations.at("LUT_FF_PAIRS").at(0)));
         }
         if(evaluations.find("LOGIC_ELEMENTS") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("LOGIC_ELEMENTS").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Logic Elements           : " + STR(evaluations.at("LOGIC_ELEMENTS").at(0)));
         }
         if(evaluations.find("FUNCTIONAL_ELEMENTS") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("FUNCTIONAL_ELEMENTS").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Functional Elements      : " + STR(evaluations.at("FUNCTIONAL_ELEMENTS").at(0)));
         }
         if(evaluations.find("LOGIC_AREA") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("LOGIC_AREA").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Logic Area               : " + STR(evaluations.at("LOGIC_AREA").at(0)));
         }
         if(evaluations.find("POWER") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("POWER").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Power                    : " + STR(evaluations.at("POWER").at(0)));
         }
         if(evaluations.find("ALMS") != evaluations.end())
         {
            THROW_ASSERT(evaluations.at("ALMS").size() == 1, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---ALMs                     : " + STR(evaluations.at("ALMS").at(0)));
         }
      }
      else if(objective == "BRAMS")
      {
         double brams = evaluation;
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BRAMs                    : " + STR(brams));
      }
      else if(objective == "CLOCK_SLACK")
      {
         /// get the timing information after the synthesis
         const auto slack = evaluation;
         if(parameters->getOption<bool>(OPT_timing_violation_abort) and slack < 0.0)
         {
            THROW_UNREACHABLE("Slack is " + STR(slack));
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Design slack             : " + STR(slack));
      }
      else if(objective == "PERIOD")
      {
         /// get the timing information after the synthesis
         double minimum_period = evaluation;
         auto clock_period = parameters->getOption<double>(OPT_clock_period);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Clock period             : " + boost::lexical_cast<std::string>(clock_period));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Design minimum period    : " + boost::lexical_cast<std::string>(minimum_period));
      }
      else if(objective == "DSPS")
      {
         /// get the used resources from the wrapper
         const auto dsps = evaluation;
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---DSPs                     : " + STR(dsps));
      }
      else if((objective == "FREQUENCY" or objective == "TIME" or objective == "TOTAL_TIME" or objective == "AREAxTIME") and not printed_frequency)
      {
         printed_frequency = true;
         /// get the timing information after the synthesis
         THROW_ASSERT(evaluations.find("PERIOD") != evaluations.end(), "");
         THROW_ASSERT(evaluations.at("PERIOD").size() == 1, "");
         double minimum_period = evaluations.at("PERIOD").at(0);
         double maximum_frequency = 1000.0 / minimum_period;
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Frequency                : " + STR(maximum_frequency));
      }
      else if(objective == "REGISTERS")
      {
         /// get the used resources from the wrapper
         double reg = evaluation;
         if(reg > 0.0)
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Registers                : " + STR(reg));
         else
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---(no registers)");
      }
   }

   std::string out_file_name = "bambu_results";

   unsigned int progressive = 0;
   std::string candidate_out_file_name;
   do
   {
      candidate_out_file_name = out_file_name + "_" + boost::lexical_cast<std::string>(progressive++) + ".xml";
   } while(boost::filesystem::exists(candidate_out_file_name));

   out_file_name = candidate_out_file_name;
   xml_document document;
   xml_element* nodeRoot = document.create_root_node("bambu_results");

   std::string bench_name;
   if(parameters->isOption(OPT_configuration_name))
   {
      bench_name += parameters->getOption<std::string>(OPT_configuration_name) + ":";
   }
   if(parameters->isOption(OPT_benchmark_name))
   {
      bench_name += parameters->getOption<std::string>(OPT_benchmark_name);
   }
   if(bench_name == "" || !parameters->IsParameter("simple-benchmark-name") || parameters->GetParameter<int>("simple-benchmark-name") == 0)
   {
      const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
#if HAVE_TASTE
      if(parameters->getOption<bool>(OPT_generate_taste_architecture))
      {
         if(bench_name == "")
         {
            bench_name += "taste_architecture";
         }
         else
         {
            bench_name += ":taste_architecture";
         }

         bench_name += "_" + boost::lexical_cast<std::string>(progressive - 1);
      }
      else
#endif
      {
#ifndef NDEBUG
         if(parameters->isOption(OPT_dry_run_evaluation) and parameters->getOption<bool>(OPT_dry_run_evaluation))
         {
            if(bench_name == "")
            {
               bench_name = "dry_run";
            }
         }
         else
#endif
         {
            THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
            const auto top_fun_id = *(top_function_ids.begin());
            const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(top_fun_id);
            if(bench_name == "")
            {
               bench_name += FB->CGetBehavioralHelper()->get_function_name();
            }
            else
               bench_name += ":" + FB->CGetBehavioralHelper()->get_function_name();

            bench_name += "_" + boost::lexical_cast<std::string>(progressive - 1);
         }
      }
   }

   const std::string bambu_args = parameters->getOption<std::string>(OPT_cat_args);
   const std::string bambu_version = parameters->PrintVersion();
   const std::string current_time = TimeStamp::GetCurrentTimeStamp();
   WRITE_XNVM2(STR_XML_bambu_results_bambu_args, bambu_args, nodeRoot);
   WRITE_XNVM2(STR_XML_bambu_results_bambu_version, bambu_version, nodeRoot);
   WRITE_XNVM2(STR_XML_bambu_results_timestamp, current_time, nodeRoot);
   WRITE_XNVM2("benchmark_name", bench_name, nodeRoot);
   xml_element* child_element;

   for(auto const& objective : objective_vector)
   {
      std::string value;
      if(objective == "AREAxTIME")
      {
         THROW_ASSERT(HLSMgr->evaluations.find("AREA") != HLSMgr->evaluations.end(), "Area value not found");
         THROW_ASSERT(HLSMgr->evaluations.find("CYCLES") != HLSMgr->evaluations.end(), "Cycles value not found");
         THROW_ASSERT(HLSMgr->evaluations.find("FREQUENCY") != HLSMgr->evaluations.end(), "Frequency value not found");
         value = STR(HLSMgr->evaluations.find("AREA")->second[0] * HLSMgr->evaluations.find("CYCLES")->second[0] / HLSMgr->evaluations.find("FREQUENCY")->second[0]);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---AreaxTime                : " + value);
      }
      else if(objective == "TIME")
      {
         THROW_ASSERT(HLSMgr->evaluations.find("CYCLES") != HLSMgr->evaluations.end(), "Cycles value not found");
         THROW_ASSERT(HLSMgr->evaluations.find("FREQUENCY") != HLSMgr->evaluations.end(), "Frequency value not found");
         value = STR(HLSMgr->evaluations.find("CYCLES")->second[0] / HLSMgr->evaluations.find("FREQUENCY")->second[0]);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Time                     : " + value);
      }
      else if(objective == "TOTAL_TIME")
      {
         THROW_ASSERT(HLSMgr->evaluations.find("TOTAL_CYCLES") != HLSMgr->evaluations.end(), "Cycles value not found");
         THROW_ASSERT(HLSMgr->evaluations.find("FREQUENCY") != HLSMgr->evaluations.end(), "Frequency value not found");
         value = STR(HLSMgr->evaluations.find("TOTAL_CYCLES")->second[0] / HLSMgr->evaluations.find("FREQUENCY")->second[0]);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Tot. Time                : " + value);
      }
      else
      {
         THROW_ASSERT(HLSMgr->evaluations.find(objective) != HLSMgr->evaluations.end(), "Value of " + objective + " not found");
         const std::vector<double>& local_evaluations = HLSMgr->evaluations.find(objective)->second;
         for(auto const single_evaluation : local_evaluations)
         {
            if(value != "")
               value += ",";
            value += STR(single_evaluation);
         }
      }
      child_element = nodeRoot->add_child_element(objective);
      WRITE_XNVM2("value", value, child_element);

      if(objective == "AREA" && HLSMgr->evaluations.find("SLICE") != HLSMgr->evaluations.end())
      {
         child_element = nodeRoot->add_child_element("SLICE");
         value = STR(HLSMgr->evaluations.find("SLICE")->second[0]);
         WRITE_XNVM2("value", value, child_element);
      }
      if(objective == "AREA" && HLSMgr->evaluations.find("SLICE_LUTS") != HLSMgr->evaluations.end())
      {
         child_element = nodeRoot->add_child_element("SLICE_LUTS");
         value = STR(HLSMgr->evaluations.find("SLICE_LUTS")->second[0]);
         WRITE_XNVM2("value", value, child_element);
      }
   }
   child_element = nodeRoot->add_child_element("HLS_execution_time");
   long double exec_time = HLSMgr->HLS_execution_time;
   WRITE_XNVM2("value", STR(exec_time / 1000), child_element);

   document.write_to_file_formatted(out_file_name);
   return DesignFlowStep_Status::SUCCESS;
}

bool Evaluation::HasToBeExecuted() const
{
   return true;
}
