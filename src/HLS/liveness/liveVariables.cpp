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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file liveVariables.cpp
 * @brief Lightweight container storing SSA liveness information on FSM states.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "liveVariables.hpp"

#include "FSMInfo.hpp"

void liveVariables::addLiveInVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step)
{
   stateLiveInVariables[v].insert(std::make_pair(var, step));
}

void liveVariables::removeLiveInVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step)
{
   stateLiveInVariables[v].erase(std::make_pair(var, step));
}

const std::set<std::pair<unsigned int, unsigned int>>&
liveVariables::getLiveInFsmVariables(FSMInfo::state_descriptor v) const
{
   if(stateLiveInVariables.find(v) != stateLiveInVariables.end())
   {
      return stateLiveInVariables.find(v)->second;
   }
   else
   {
      return empty_set;
   }
}

void liveVariables::addLiveOutVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step)
{
   stateLiveOutVariables[v].insert(std::make_pair(var, step));
}

void liveVariables::removeLiveOutVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step)
{
   stateLiveOutVariables[v].erase(std::make_pair(var, step));
}

const std::set<std::pair<unsigned int, unsigned int>>&
liveVariables::getLiveOutFsmVariables(FSMInfo::state_descriptor v) const
{
   if(stateLiveOutVariables.find(v) != stateLiveOutVariables.end())
   {
      return stateLiveOutVariables.find(v)->second;
   }
   else
   {
      return empty_set;
   }
}

const std::set<FSMInfo::state_descriptor>& liveVariables::getVariableDestinationStates(FSMInfo::state_descriptor state,
                                                                                       FSMInfo::operation_descriptor op,
                                                                                       unsigned int var) const
{
   const auto stateIt = variableDestinationStates.find(state);
   if(stateIt == variableDestinationStates.end())
   {
      return emptyStateSet;
   }

   const auto opIt = stateIt->second.find(op);
   if(opIt == stateIt->second.end())
   {
      return emptyStateSet;
   }

   const auto varIt = opIt->second.find(var);
   if(varIt == opIt->second.end())
   {
      return emptyStateSet;
   }

   return varIt->second;
}

void liveVariables::addVariableDestinationState(unsigned int var, FSMInfo::operation_descriptor op,
                                                FSMInfo::state_descriptor state, FSMInfo::state_descriptor state_in)
{
   variableDestinationStates[state][op][var].insert(state_in);
}
