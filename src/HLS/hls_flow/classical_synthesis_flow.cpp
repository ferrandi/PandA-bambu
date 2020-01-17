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
 * @file classical_synthesis_flow.cpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "classical_synthesis_flow.hpp"

// . include
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

ClassicalHLSSynthesisFlow::ClassicalHLSSynthesisFlow(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW)
{
   composed = true;
}

ClassicalHLSSynthesisFlow::~ClassicalHLSSynthesisFlow() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ClassicalHLSSynthesisFlow::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
#if HAVE_TASTE
         if(parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_TASTE_HDL_ARCHITECTURE, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_TASTE_SYNTHESIS_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         else
#endif
             if(parameters->isOption(OPT_top_functions_names) and parameters->getOption<const std::list<std::string>>(OPT_top_functions_names).size() == 1)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_SYNTHESIS_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         if(parameters->isOption(OPT_export_core_mode))
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_export_core_mode), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::WRITE_HLS_SUMMARY, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         if(parameters->getOption<int>(OPT_evaluation))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::EVALUATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         }
         if(parameters->getOption<bool>(OPT_generate_testbench))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_SIMULATION_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
            ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
#if HAVE_VCD_BUILT
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_UTILITY, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         }
#endif
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status ClassicalHLSSynthesisFlow::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool ClassicalHLSSynthesisFlow::HasToBeExecuted() const
{
   return true;
}
