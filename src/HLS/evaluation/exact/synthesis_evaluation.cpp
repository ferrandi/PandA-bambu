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
 * @file synthesis_evaluation.cpp
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "synthesis_evaluation.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"

/// technology/physical_library/models
#include "area_model.hpp"
#include "time_model.hpp"

/// technology/physical_library/models/area
#include "clb_model.hpp"

/// wrapper/synthesis include
#include "BackendFlow.hpp"

SynthesisEvaluation::SynthesisEvaluation(const ParameterConstRef _Param, const HLS_managerRef _hls_mgr, const DesignFlowManagerConstRef _design_flow_manager)
    : EvaluationBaseStep(_Param, _hls_mgr, 0, _design_flow_manager, HLSFlowStep_Type::SYNTHESIS_EVALUATION)
{
}

SynthesisEvaluation::~SynthesisEvaluation() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> SynthesisEvaluation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case PRECEDENCE_RELATIONSHIP:
      {
#if HAVE_SIMULATION_WRAPPER_BUILT
         ret.insert(std::make_tuple(HLSFlowStep_Type::SIMULATION_EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
#endif
#if HAVE_VCD_BUILT
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_UTILITY, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
#endif
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
#if HAVE_TASTE
         if(parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_TASTE_SYNTHESIS_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_SYNTHESIS_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status SynthesisEvaluation::InternalExec()
{
   HLSMgr->get_backend_flow()->ExecuteSynthesis();
   std::string objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
   bool printed_area = false;
   for(const auto& objective : objective_vector)
   {
      if((objective == "AREA" or objective == "AREAxTIME") and !printed_area)
      {
         printed_area = true;
         /// get the used resources from the wrapper
         area_modelRef area_m = HLSMgr->get_backend_flow()->get_used_resources();

         if(area_m)
         {
            double slices = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::SLICE) : 0;
            if(slices != 0.0)
            {
               HLSMgr->evaluations["SLICE"] = std::vector<double>(1, slices);
            }
            double sliceLuts = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::SLICE_LUTS) : 0;
            if(sliceLuts != 0.0)
            {
               HLSMgr->evaluations["SLICE_LUTS"] = std::vector<double>(1, sliceLuts);
            }
            double lut_ff_pairs = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::LUT_FF_PAIRS) : 0;
            if(lut_ff_pairs != 0.0)
            {
               HLSMgr->evaluations["LUT_FF_PAIRS"] = std::vector<double>(1, lut_ff_pairs);
            }
            double logic_elements = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::LOGIC_ELEMENTS) : 0;
            if(logic_elements != 0.0)
            {
               HLSMgr->evaluations["LOGIC_ELEMENTS"] = std::vector<double>(1, logic_elements);
            }
            double functional_elements = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::FUNCTIONAL_ELEMENTS) : 0;
            if(functional_elements != 0.0)
            {
               HLSMgr->evaluations["FUNCTIONAL_ELEMENTS"] = std::vector<double>(1, functional_elements);
            }
            double alms = GetPointer<clb_model>(area_m) ? GetPointer<clb_model>(area_m)->get_resource_value(clb_model::ALMS) : 0;
            if(alms != 0.0)
            {
               HLSMgr->evaluations["ALMS"] = std::vector<double>(1, alms);
            }
         }
         HLSMgr->evaluations["AREA"] = std::vector<double>(1, area_m->get_area_value());
      }
      else if(objective == "BRAMS")
      {
         area_modelRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double brams = 0;
         if(GetPointer<clb_model>(area_m))
            brams = GetPointer<clb_model>(area_m)->get_resource_value(clb_model::BRAM);
         HLSMgr->evaluations["BRAMS"] = std::vector<double>(1, brams);
      }
      else if(objective == "CLOCK_SLACK")
      {
         /// get the timing information after the synthesis
         time_modelRef time_m = HLSMgr->get_backend_flow()->get_timing_results();
         double minimum_period = time_m->get_execution_time();

         double clock_period = HLSMgr->get_HLS(*(HLSMgr->GetCallGraphManager()->GetRootFunctions().begin()))->HLS_C->get_clock_period();
         double slack = clock_period - minimum_period;
         if(parameters->getOption<bool>(OPT_timing_violation_abort) and slack < 0.0)
         {
            THROW_UNREACHABLE("Slack is " + STR(slack));
         }
         HLSMgr->evaluations["CLOCK_SLACK"] = std::vector<double>(1, slack);
      }
      else if(objective == "PERIOD")
      {
         /// get the timing information after the synthesis
         time_modelRef time_m = HLSMgr->get_backend_flow()->get_timing_results();
         double minimum_period = time_m->get_execution_time();
         HLSMgr->evaluations["PERIOD"] = std::vector<double>(1, minimum_period);
      }
      else if(objective == "DSPS")
      {
         /// get the used resources from the wrapper
         area_modelRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double dsps = 0;
         if(GetPointer<clb_model>(area_m))
            dsps = GetPointer<clb_model>(area_m)->get_resource_value(clb_model::DSP);
         HLSMgr->evaluations["DSPS"] = std::vector<double>(1, dsps);
      }
      else if(objective == "FREQUENCY" or objective == "TIME" or objective == "TOTAL_TIME" or objective == "AREAxTIME")
      {
         /// get the timing information after the synthesis
         time_modelRef time_m = HLSMgr->get_backend_flow()->get_timing_results();
         double minimum_period = time_m->get_execution_time();

         double maximum_frequency = 1000.0 / minimum_period;
         HLSMgr->evaluations["FREQUENCY"] = std::vector<double>(1, maximum_frequency);
      }
      else if(objective == "REGISTERS")
      {
         /// get the used resources from the wrapper
         area_modelRef area_m = HLSMgr->get_backend_flow()->get_used_resources();
         double reg = 0;
         if(GetPointer<clb_model>(area_m))
            reg = GetPointer<clb_model>(area_m)->get_resource_value(clb_model::REGISTERS);
         HLSMgr->evaluations["REGISTERS"] = std::vector<double>(1, reg);
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
