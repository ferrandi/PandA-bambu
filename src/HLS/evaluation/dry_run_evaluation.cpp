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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @file dry_runevaluation.cpp
 * @brief Class to generate an empty evaluation report
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "dry_run_evaluation.hpp"

///. include
#include "Parameter.hpp"

/// constants include
#include "bambu_results_xml.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

/// polixml include
#include "xml_document.hpp"

/// utility include
#include "string_manipulation.hpp" // for GET_CLASS
#include "xml_helper.hpp"

DryRunEvaluation::DryRunEvaluation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : EvaluationBaseStep(_parameters, _HLSMgr, 0, _design_flow_manager, HLSFlowStep_Type::DRY_RUN_EVALUATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DryRunEvaluation::~DryRunEvaluation() = default;

bool DryRunEvaluation::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status DryRunEvaluation::InternalExec()
{
   std::string objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
   for(const auto& objective : objective_vector)
   {
      HLSMgr->evaluations[objective] = std::vector<double>(1, 0.0);
      if(objective == "CYCLES")
      {
         HLSMgr->evaluations["NUM_EXECUTIONS"] = std::vector<double>(1, 1);
         HLSMgr->evaluations["TOTAL_CYCLES"] = std::vector<double>(1, 0.0);
      }
      if(objective == "FREQUENCY" or objective == "TIME" or objective == "TOTAL_TIME" or objective == "AREAxTIME")
      {
         auto clock_period = parameters->getOption<double>(OPT_clock_period);
         HLSMgr->evaluations["PERIOD"] = std::vector<double>(1, clock_period);
      }
      if(objective == "TIME")
      {
         auto clock_period = parameters->getOption<double>(OPT_clock_period);
         HLSMgr->evaluations["CYCLES"] = std::vector<double>(1, 0.0);
         HLSMgr->evaluations["FREQUENCY"] = std::vector<double>(1, 1000 / clock_period);
      }
      if(objective == "PERIOD")
      {
         auto clock_period = parameters->getOption<double>(OPT_clock_period);
         HLSMgr->evaluations[objective] = std::vector<double>(1, clock_period);
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
