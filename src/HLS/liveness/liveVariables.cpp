/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
