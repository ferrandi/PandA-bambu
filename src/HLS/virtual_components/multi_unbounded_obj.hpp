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
 * @file multi_unbounded_obj.hpp
 * @brief Base class for all unbounded objects added to datapath
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef MULTI_UNBOUNDED_OBJ_HPP
#define MULTI_UNBOUNDED_OBJ_HPP
#include "HLS/fsm/FSMInfo.hpp"
#include "generic_obj.hpp"
#include "graph.hpp"

#include <vector>

/**
 * class modeling a register object
 */
class multi_unbounded_obj : public generic_obj
{
   FSMInfo::state_descriptor fsm_state;
   std::vector<FSMInfo::operation_descriptor> ops;
   generic_objRef mu_enable;

 public:
   /**
    * This is the constructor of the multi_unbounded_obj class, with a given id
    * @param _fsm_state is the FSM state associated with this object
    * @param _ops is the operations controlled by this object
    * @param _name is the name of the multi_unbounded_obj
    */
   explicit multi_unbounded_obj(FSMInfo::state_descriptor _fsm_state,
                                const std::vector<FSMInfo::operation_descriptor>& _ops, const std::string& _name)
       : generic_obj(MULTI_UNBOUNDED_OBJ, _name), fsm_state(_fsm_state), ops(_ops)
   {
   }

   /**
    * @return the all done object associated with a multi-unbounded controller
    */
   FSMInfo::state_descriptor get_fsm_state() const
   {
      return fsm_state;
   }

   const auto& get_ops()
   {
      return ops;
   }
   /**
    * Gets the write enable object for the given register
    * @return a set of sets where each of them can enable register write (when all conditions contained are
    *        true)
    */
   generic_objRef get_mu_enable() const
   {
      return mu_enable;
   }

   /**
    * Sets the write enable for given register
    */
   void set_mu_enable(const generic_objRef& mu_en)
   {
      mu_enable = mu_en;
   }
};

#endif
