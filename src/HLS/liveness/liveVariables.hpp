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
 * @file liveVariables.hpp
 * @brief Lightweight container storing SSA live variable information on FSM states.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef LIVEVARIABLES_HPP
#define LIVEVARIABLES_HPP

#include "HLS/fsm/FSMInfo.hpp"
#include "custom_set.hpp"

/**
 * @brief Lightweight container that tracks SSA live-in/out sets per FSM state.
 *
 * It stores, for each FSM state, the set of SSA variables that are
 * live along with the pipeline step in which the variable is visible.
 */
class liveVariables
{
 private:
   /// Live-in variable sets per FSM state.
   std::map<FSMInfo::state_descriptor, std::set<std::pair<unsigned int, unsigned int>>> stateLiveInVariables;

   /// Live-out variable sets per FSM state.
   std::map<FSMInfo::state_descriptor, std::set<std::pair<unsigned int, unsigned int>>> stateLiveOutVariables;

   /// Shared empty container for callers that need a const reference.
   const std::set<std::pair<unsigned int, unsigned int>> empty_set;

   /// Shared empty state set used when no destination exists.
   const std::set<FSMInfo::state_descriptor> emptyStateSet;

   /// Maps (state, operation, variable) tuples to the set of destination states that egress that variable.
   std::map<FSMInfo::state_descriptor,
            std::map<FSMInfo::operation_descriptor, std::map<unsigned int, std::set<FSMInfo::state_descriptor>>>>
       variableDestinationStates;

 public:
   liveVariables() = default;

   /**
    * Mark a single variable live at the input of a state.
    */
   void addLiveInVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step);

   /**
    * Remove a variable from the live-in set of a state.
    */
   void removeLiveInVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step);

   /**
    * Mark a variable live at the output of a state.
    */
   void addLiveOutVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step);

   /**
    * Remove a variable from the live-out set of a state.
    */
   void removeLiveOutVariable(FSMInfo::state_descriptor v, unsigned int var, unsigned int step);

   /**
    * Return the live-in set for the given FSM state (empty if unknown).
    */
   const std::set<std::pair<unsigned int, unsigned int>>& getLiveInFsmVariables(FSMInfo::state_descriptor v) const;

   /**
    * Return the live-out set for the given FSM state (empty if unknown).
    */
   const std::set<std::pair<unsigned int, unsigned int>>& getLiveOutFsmVariables(FSMInfo::state_descriptor v) const;

   /**
    * Lookup the destination states of a variable defined by an operation in a state.
    * Returns an empty set when no mapping exists.
    */
   const std::set<FSMInfo::state_descriptor>& getVariableDestinationStates(FSMInfo::state_descriptor state,
                                                                           FSMInfo::operation_descriptor op,
                                                                           unsigned int var) const;

   /**
    * Record that the variable defined by the operation in the state is consumed by the destination state.
    */
   void addVariableDestinationState(unsigned int var, FSMInfo::operation_descriptor op, FSMInfo::state_descriptor state,
                                    FSMInfo::state_descriptor destState);
};

// refcount definition for class
using liveVariablesRef = refcount<liveVariables>;

#endif
