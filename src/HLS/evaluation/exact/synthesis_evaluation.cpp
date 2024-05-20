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
 * @file synthesis_evaluation.cpp
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "synthesis_evaluation.hpp"

#include "BackendFlow.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "call_graph_manager.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "time_info.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

SynthesisEvaluation::SynthesisEvaluation(const ParameterConstRef _Param, const HLS_managerRef _hls_mgr,
                                         const DesignFlowManagerConstRef _design_flow_manager)
    : EvaluationBaseStep(_Param, _hls_mgr, _design_flow_manager, HLSFlowStep_Type::SYNTHESIS_EVALUATION)
{
}

SynthesisEvaluation::~SynthesisEvaluation() = default;

HLS_step::HLSRelationships
SynthesisEvaluation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
#if HAVE_TASTE
         if(parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_TASTE_SYNTHESIS_SCRIPT,
                                       HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_SYNTHESIS_SCRIPT, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
#if HAVE_SIMULATION_WRAPPER_BUILT
         ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_VCD_BUILT
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_UTILITY, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
#endif
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status SynthesisEvaluation::Exec()
{
   HLSMgr->get_backend_flow()->ExecuteSynthesis();
   auto objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   const auto objective_vector = string_to_container<std::vector<std::string>>(objective_string, ",");
   bool printed_area = false;
   for(const auto& objective : objective_vector)
   {
      if((objective == "AREA" or objective == "AREAxTIME") and !printed_area)
      {
         printed_area = true;
         /// get the used resources from the wrapper
         area_infoRef area_m = HLSMgr->get_backend_flow()->get_used_resources();

         if(area_m)
         {
            double slices = area_m ? area_m->get_resource_value(area_info::SLICE) : 0;
            if(slices != 0.0)
            {
               HLSMgr->evaluations["SLICE"] = slices;
            }
            double sliceLuts = area_m ? area_m->get_resource_value(area_info::SLICE_LUTS) : 0;
            if(sliceLuts != 0.0)
            {
               HLSMgr->evaluations["SLICE_LUTS"] = sliceLuts;
            }
            double lut_ff_pairs = area_m ? area_m->get_resource_value(area_info::LUT_FF_PAIRS) : 0;
            if(lut_ff_pairs != 0.0)
            {
               HLSMgr->evaluations["LUT_FF_PAIRS"] = lut_ff_pairs;
            }
            double logic_elements = area_m ? area_m->get_resource_value(area_info::LOGIC_ELEMENTS) : 0;
            if(logic_elements != 0.0)
            {
               HLSMgr->evaluations["LOGIC_ELEMENTS"] = logic_elements;
            }
            double functional_elements = area_m ? area_m->get_resource_value(area_info::FUNCTIONAL_ELEMENTS) : 0;
            if(functional_elements != 0.0)
            {
               HLSMgr->evaluations["FUNCTIONAL_ELEMENTS"] = functional_elements;
            }
            double logic_area = area_m ? area_m->get_resource_value(area_info::LOGIC_AREA) : 0;
            if(logic_area != 0.0)
            {
               HLSMgr->evaluations["LOGIC_AREA"] = logic_area;
            }
            double power = area_m ? area_m->get_resource_value(area_info::POWER) : 0;
            if(power != 0.0)
            {
               HLSMgr->evaluations["POWER"] = power;
            }
            double alms = area_m ? area_m->get_resource_value(area_info::ALMS) : 0;
            if(alms != 0.0)
            {
               HLSMgr->evaluations["ALMS"] = alms;
            }
            double urams = area_m ? area_m->get_resource_value(area_info::URAM) : 0;
            if(urams != 0.0)
            {
               HLSMgr->evaluations["URAMS"] = urams;
            }
         }
         HLSMgr->evaluations["AREA"] = area_m->get_area_value();
      }
      else if(objective == "BRAMS")
      {
         area_infoRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double brams = 0;
         if(area_m)
         {
            brams = area_m->get_resource_value(area_info::BRAM);
         }
         HLSMgr->evaluations["BRAMS"] = brams;
      }
      else if(objective == "DRAMS")
      {
         area_infoRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double drams = 0;
         if(area_m)
         {
            drams = area_m->get_resource_value(area_info::DRAM);
         }
         HLSMgr->evaluations["DRAMS"] = drams;
      }
      else if(objective == "CLOCK_SLACK")
      {
         /// get the timing information after the synthesis
         const auto time_m = HLSMgr->get_backend_flow()->get_timing_results();
         const auto minimum_period = time_m->get_execution_time();
         const auto clock_period = parameters->getOption<double>(OPT_clock_period);
         const auto slack = clock_period - minimum_period;
         if(parameters->getOption<bool>(OPT_timing_violation_abort) && slack < 0.0)
         {
            THROW_UNREACHABLE("Slack is " + STR(slack));
         }
         HLSMgr->evaluations["CLOCK_SLACK"] = slack;
      }
      else if(objective == "PERIOD")
      {
         /// get the timing information after the synthesis
         time_infoRef time_m = HLSMgr->get_backend_flow()->get_timing_results();
         double minimum_period = time_m->get_execution_time();
         HLSMgr->evaluations["PERIOD"] = minimum_period;
      }
      else if(objective == "DSPS")
      {
         /// get the used resources from the wrapper
         area_infoRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double dsps = 0;
         if(area_m)
         {
            dsps = area_m->get_resource_value(area_info::DSP);
         }
         HLSMgr->evaluations["DSPS"] = dsps;
      }
      else if(objective == "FREQUENCY" or objective == "TIME" or objective == "TOTAL_TIME" or objective == "AREAxTIME")
      {
         /// get the timing information after the synthesis
         time_infoRef time_m = HLSMgr->get_backend_flow()->get_timing_results();
         double minimum_period = time_m->get_execution_time();

         double maximum_frequency = 1000.0 / minimum_period;
         HLSMgr->evaluations["FREQUENCY"] = maximum_frequency;
      }
      else if(objective == "REGISTERS")
      {
         /// get the used resources from the wrapper
         area_infoRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double reg = 0;
         reg = area_m->get_resource_value(area_info::REGISTERS);
         HLSMgr->evaluations["REGISTERS"] = reg;
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
