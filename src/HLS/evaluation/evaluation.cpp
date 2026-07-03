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
 * @file evaluation.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "evaluation.hpp"

#include "BackendWrapper.hpp"
#include "Parameter.hpp"
#include "area_estimation.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_manager.hpp"
#include "evaluation_mode.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace
{
   constexpr std::array<const char*, 13> evaluation_objective_order = {
       "AREA",  "AREAxTIME",   "TIME", "TOTAL_TIME", "CYCLES", "TOTAL_CYCLES", "BRAMS",
       "DRAMS", "CLOCK_SLACK", "DSPS", "FREQUENCY",  "PERIOD", "REGISTERS"};
   constexpr std::size_t evaluation_name_column = 25;

   struct normalized_area_breakdown
   {
      double logic_area = 0.0;
      double dsp_lut_equivalent_area = 0.0;
      const char* primary_attr = nullptr;
      const char* secondary_attr = nullptr;
   };

   pugi::xml_node get_or_append_child(pugi::xml_node parent, const char* const name)
   {
      if(auto child = parent.child(name))
      {
         return child;
      }
      return parent.append_child(name);
   }

   bool has_objective(const CustomSet<std::string>& objectives, const char* const objective)
   {
      return objectives.find(objective) != objectives.end();
   }

   bool read_resource(const pugi::xml_node resources, const char* const name, double& value)
   {
      if(const auto attr = resources.attribute(name))
      {
         value = attr.as_double();
         return true;
      }
      return false;
   }

   double read_resource_or(const pugi::xml_node resources, const std::initializer_list<const char*> names,
                           const double fallback)
   {
      double value = fallback;
      for(const auto* const name : names)
      {
         if(read_resource(resources, name, value))
         {
            return value;
         }
      }
      return fallback;
   }

   double target_period_ns(const pugi::xml_node app)
   {
      const auto target = app.child("target");
      if(const auto period_ps = target.attribute("period_ps"))
      {
         return period_ps.as_double() / 1000.0;
      }
      return target.attribute("period").as_double();
   }

   std::string_view target_vendor(const pugi::xml_node app)
   {
      return app.child("target").attribute("vendor").as_string();
   }

   bool is_fpga_target(const pugi::xml_node app)
   {
      const auto vendor = target_vendor(app);
      return vendor == "Xilinx" || vendor == "Altera" || vendor == "Lattice" || vendor == "NanoXplore";
   }

   normalized_area_breakdown get_normalized_area_breakdown(const pugi::xml_node app, const pugi::xml_node resources,
                                                           const HLS_deviceConstRef& HLS_D)
   {
      normalized_area_breakdown breakdown;
      const auto vendor = target_vendor(app);
      if(vendor == "Xilinx")
      {
         breakdown.logic_area = read_resource_or(resources, {"LUTS", "SLICES", "AREA"}, 0.0);
         breakdown.primary_attr =
             resources.attribute("LUTS") ? "LUTS" : (resources.attribute("SLICES") ? "SLICES" : nullptr);
         breakdown.secondary_attr = resources.attribute("SLICES") ? "SLICES" : nullptr;
      }
      else if(vendor == "Lattice")
      {
         breakdown.logic_area = read_resource_or(resources, {"LUTS", "SLICES", "AREA"}, 0.0);
         breakdown.primary_attr =
             resources.attribute("LUTS") ? "LUTS" : (resources.attribute("SLICES") ? "SLICES" : nullptr);
         breakdown.secondary_attr = resources.attribute("LUTS") && resources.attribute("SLICES") ? "SLICES" : nullptr;
      }
      else if(vendor == "NanoXplore")
      {
         breakdown.logic_area = read_resource_or(resources, {"FE", "LUTS", "AREA"}, 0.0);
         breakdown.primary_attr = resources.attribute("FE") ? "FE" : (resources.attribute("LUTS") ? "LUTS" : nullptr);
         breakdown.secondary_attr = resources.attribute("FE") && resources.attribute("LUTS") ? "LUTS" : nullptr;
      }
      else if(vendor == "Altera")
      {
         breakdown.logic_area = read_resource_or(resources, {"ALM", "ALUT", "AREA"}, 0.0);
         breakdown.primary_attr = resources.attribute("ALM") ? "ALM" : (resources.attribute("ALUT") ? "ALUT" : nullptr);
         breakdown.secondary_attr = resources.attribute("ALM") && resources.attribute("ALUT") ? "ALUT" : nullptr;
      }
      else
      {
         breakdown.logic_area = read_resource_or(resources, {"AREA", "SLICES"}, 0.0);
      }

      if(is_fpga_target(app))
      {
         breakdown.dsp_lut_equivalent_area =
             read_resource_or(resources, {"DSPS"}, 0.0) * area_estimation::get_dsp_lut_scale(HLS_D);
      }
      return breakdown;
   }

   double normalized_area_value(const pugi::xml_node app, const pugi::xml_node resources,
                                const HLS_deviceConstRef& HLS_D)
   {
      const auto breakdown = get_normalized_area_breakdown(app, resources, HLS_D);
      return breakdown.logic_area + breakdown.dsp_lut_equivalent_area;
   }

   std::string display_objective_name(const char* const objective)
   {
      const auto objective_name = std::string_view{objective};
      if(objective_name == "TIME")
      {
         return "TIME [ns]";
      }
      if(objective_name == "TOTAL_TIME")
      {
         return "TOTAL_TIME [ns]";
      }
      return std::string{objective};
   }

   std::string display_area_detail_name(const char* const attr_name)
   {
      const auto name = std::string_view{attr_name};
      if(name == "LUTS")
      {
         return "AREA LUT";
      }
      if(name == "SLICES")
      {
         return "AREA SLICE";
      }
      if(name == "ALM")
      {
         return "AREA ALM";
      }
      if(name == "ALUT")
      {
         return "AREA ALUT";
      }
      if(name == "FE")
      {
         return "AREA FE";
      }
      return "AREA";
   }

   std::string format_number(const double value)
   {
      std::ostringstream oss;
      oss << std::setprecision(15) << value;
      return oss.str();
   }

   std::string format_resource_line(const std::string& label, const std::string& value,
                                    const std::string_view prefix = "---")
   {
      std::string line(prefix);
      line += label;
      if(label.size() < evaluation_name_column)
      {
         line.append(evaluation_name_column - label.size(), ' ');
      }
      line += ": ";
      line += value;
      return line;
   }

   void set_resource(pugi::xml_node resources, const char* const name, const double value)
   {
      auto attr = resources.attribute(name);
      if(!attr)
      {
         attr = resources.append_attribute(name);
      }
      attr = value;
   }

   pugi::xml_node get_simulation_results(pugi::xml_node app)
   {
      auto timing = get_or_append_child(app, "timing");
      if(auto simulation = timing.child("simulation"))
      {
         return simulation;
      }
      if(const auto legacy_eval = timing.child("evaluation"))
      {
         auto simulation = timing.append_copy(legacy_eval);
         simulation.set_name("simulation");
         timing.remove_child(legacy_eval);
         return simulation;
      }
      return timing.append_child("simulation");
   }

   double read_metric_or(const pugi::xml_node evaluation, const pugi::xml_node resources,
                         const std::initializer_list<const char*> names, const double fallback)
   {
      double value = fallback;
      for(const auto* const name : names)
      {
         if(read_resource(evaluation, name, value) || read_resource(resources, name, value))
         {
            return value;
         }
      }
      return fallback;
   }

   void normalize_evaluation_results(pugi::xml_node app, const CustomSet<std::string>& objectives,
                                     const HLS_deviceConstRef& HLS_D)
   {
      auto resources = get_or_append_child(app, "resources");
      auto evaluation = get_or_append_child(app, "evaluation");
      const auto simulation = get_simulation_results(app);

      unsigned run_count = 0;
      double total_cycles = 0.0;
      for(const auto& run : simulation.children("run"))
      {
         total_cycles += run.text().as_double();
         ++run_count;
      }

      const auto average_cycles = run_count ? total_cycles / run_count : 0.0;
      const auto period = read_metric_or(evaluation, resources, {"PERIOD", "DELAY"}, target_period_ns(app));
      const auto area = normalized_area_value(app, resources, HLS_D);
      const auto clock_slack =
          read_metric_or(evaluation, resources, {"CLOCK_SLACK", "SLACK"}, target_period_ns(app) - period);
      const auto frequency =
          read_metric_or(evaluation, resources, {"FREQUENCY"}, period != 0.0 ? 1000.0 / period : 0.0);
      const auto has_simulation_results = run_count != 0;
      const auto time = average_cycles * period;
      const auto total_time = total_cycles * period;

      if(resources.attribute("AREA") || has_objective(objectives, "AREA") || has_objective(objectives, "AREAxTIME"))
      {
         set_resource(evaluation, "AREA", area);
      }
      if(has_objective(objectives, "AREAxTIME") && has_simulation_results)
      {
         set_resource(evaluation, "AREAxTIME", area * time);
      }
      if(has_objective(objectives, "TIME") && has_simulation_results)
      {
         set_resource(evaluation, "TIME", time);
      }
      if(has_objective(objectives, "TOTAL_TIME") && has_simulation_results)
      {
         set_resource(evaluation, "TOTAL_TIME", total_time);
      }
      if(has_objective(objectives, "CYCLES") && has_simulation_results)
      {
         set_resource(evaluation, "CYCLES", average_cycles);
      }
      if(has_objective(objectives, "TOTAL_CYCLES") && has_simulation_results)
      {
         set_resource(evaluation, "TOTAL_CYCLES", total_cycles);
      }
      if(has_objective(objectives, "BRAMS"))
      {
         set_resource(evaluation, "BRAMS", read_metric_or(evaluation, resources, {"BRAMS"}, 0.0));
      }
      if(has_objective(objectives, "DRAMS"))
      {
         set_resource(evaluation, "DRAMS", read_metric_or(evaluation, resources, {"DRAMS"}, 0.0));
      }
      if(has_objective(objectives, "CLOCK_SLACK"))
      {
         set_resource(evaluation, "CLOCK_SLACK", clock_slack);
      }
      if(has_objective(objectives, "DSPS"))
      {
         set_resource(evaluation, "DSPS", read_metric_or(evaluation, resources, {"DSPS"}, 0.0));
      }
      if(has_objective(objectives, "FREQUENCY"))
      {
         set_resource(evaluation, "FREQUENCY", frequency);
      }
      if(has_objective(objectives, "PERIOD"))
      {
         set_resource(evaluation, "PERIOD", period);
      }
      if(has_objective(objectives, "REGISTERS"))
      {
         set_resource(evaluation, "REGISTERS", read_metric_or(evaluation, resources, {"REGISTERS"}, 0.0));
      }
   }
} // namespace

Evaluation::Evaluation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                       const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::EVALUATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
Evaluation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         const auto backend_pipeline = parameters->getOption<std::string>(OPT_backend_pipeline);
         if(backend_pipeline.find("simulation/") != std::string::npos)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
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
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool Evaluation::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status Evaluation::Exec()
{
   pugi::xml_document results_xml;
   const auto eval_mode = parameters->getOption<EvaluationMode::evaluation_mode>(OPT_evaluation_mode);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                  "---Backend pipeline: " + parameters->getOption<std::string>(OPT_backend_pipeline));
   if(eval_mode == EvaluationMode::DRY_RUN)
   {
      auto app = results_xml.append_child("application");
      auto resources = app.append_child("resources");
      const auto clk_period = parameters->getOption<double>(OPT_clock_period);
      resources.append_attribute("PERIOD") = clk_period;
      resources.append_attribute("FREQUENCY") = 1000.0 / clk_period;
      auto timing = app.append_child("timing");
      auto simulation = timing.append_child("simulation");
      simulation.append_attribute("return_value") = "0";
      simulation.append_child("run").text() = "0";
   }
   else
   {
      BackendWrapper backend(parameters, HLSMgr->get_HLS_device(),
                             parameters->getOption<std::vector<std::string>>(OPT_backend_pipeline));
      backend.init(HLSMgr);
      if(eval_mode == EvaluationMode::NONE)
      {
         return DesignFlowStep_Status::SUCCESS;
      }
      results_xml = backend.run();
   }

   auto app = results_xml.child("application");
   auto resources = get_or_append_child(app, "resources");
   const auto objectives = parameters->getOption<CustomSet<std::string>>(OPT_evaluation_objectives);
   normalize_evaluation_results(app, objectives, HLSMgr->get_HLS_device());
   const auto evaluation = get_or_append_child(app, "evaluation");
   BackendWrapper::StoreResults(results_xml, parameters);

   for(const auto* const objective : evaluation_objective_order)
   {
      if(has_objective(objectives, objective))
      {
         const auto attr = evaluation.attribute(objective);
         if(!attr)
         {
            continue;
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        format_resource_line(display_objective_name(objective), attr.value()));
         if(std::string_view{objective} == "AREA")
         {
            const auto breakdown = get_normalized_area_breakdown(app, resources, HLSMgr->get_HLS_device());
            if(breakdown.primary_attr)
            {
               if(const auto detail_attr = resources.attribute(breakdown.primary_attr))
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 format_resource_line(display_area_detail_name(breakdown.primary_attr),
                                                      detail_attr.value(), "---- "));
               }
            }
            if(breakdown.secondary_attr)
            {
               if(const auto detail_attr = resources.attribute(breakdown.secondary_attr))
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 format_resource_line(display_area_detail_name(breakdown.secondary_attr),
                                                      detail_attr.value(), "---- "));
               }
            }
            if(breakdown.dsp_lut_equivalent_area != 0.0)
            {
               INDENT_OUT_MEX(
                   OUTPUT_LEVEL_MINIMUM, output_level,
                   format_resource_line("AREA DSP LUTEQ", format_number(breakdown.dsp_lut_equivalent_area), "---- "));
            }
         }
      }
   }

   const auto timing_simulation = app.child("timing").child("simulation");
   if(timing_simulation)
   {
      unsigned run_count = 0;
      unsigned long long tot_cycles = 0;
      for(const auto& run : timing_simulation.children("run"))
      {
         tot_cycles += run.text().as_ullong();
         ++run_count;
      }
      if(run_count)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Total cycles             : " + std::to_string(tot_cycles) + " cycles");
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Number of executions     : " + std::to_string(run_count));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Average execution        : " + std::to_string(tot_cycles / run_count) + " cycles");
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
