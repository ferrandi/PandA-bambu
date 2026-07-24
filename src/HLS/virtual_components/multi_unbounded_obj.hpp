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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
