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
 * @file values_scheme.cpp
 * @brief Class implementation of values scheme for the storage value insertion phase
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
/// Header include
#include "values_scheme.hpp"

///. include
#include "Parameter.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/binding/storage_value_information includes
#include "storage_value_information.hpp"

/// HLS/liveness include
#include "liveness.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"

/// utility include
#include "cpu_time.hpp"

values_scheme::values_scheme(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : storage_value_insertion(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION)
{
}

values_scheme::~values_scheme() = default;

void values_scheme::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->storage_value_information = StorageValueInformationRef(new StorageValueInformation(HLSMgr, funId));
   HLS->storage_value_information->Initialize();
}

DesignFlowStep_Status values_scheme::InternalExec()
{
   long step_time;
   START_TIME(step_time);
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");
   unsigned int i = 0;
   const std::list<vertex>& support = HLS->Rliv->get_support();

   const std::list<vertex>::const_iterator vEnd = support.end();
   for(auto vIt = support.begin(); vIt != vEnd; ++vIt)
   {
      // std::cerr << "current state for sv " << HLS->Rliv->get_name(*vIt) << std::endl;
      const CustomOrderedSet<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
      const CustomOrderedSet<unsigned int>::const_iterator k_end = live.end();
      for(auto k = live.begin(); k != k_end; ++k)
      {
         if(HLS->storage_value_information->storage_index_map.find(*k) == HLS->storage_value_information->storage_index_map.end())
         {
            HLS->storage_value_information->storage_index_map[*k] = i;
            HLS->storage_value_information->variable_index_vect.push_back(*k);
            i++;
         }
      }
   }
   HLS->storage_value_information->number_of_storage_values = i;
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Storage Value Information of function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of storage values inserted: " + std::to_string(i));
   STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to compute storage value information: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
