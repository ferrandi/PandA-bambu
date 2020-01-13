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
 * @file multi_unbounded_obj.hpp
 * @brief Base class for all unbounded objects added to datapath
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef MULTI_UNBOUNDED_OBJ_HPP
#define MULTI_UNBOUNDED_OBJ_HPP

#include "custom_set.hpp"
#include "generic_obj.hpp"
#include "graph.hpp"
#include "refcount.hpp"

/**
 * class modeling a register object
 */
class multi_unbounded_obj : public generic_obj
{
   vertex fsm_state;
   CustomOrderedSet<vertex> ops;

 public:
   /**
    * This is the constructor of the multi_unbounded_obj class, with a given id
    * @param _name is the name of the multi_unbounded_obj
    */
   explicit multi_unbounded_obj(vertex _fsm_state, const CustomOrderedSet<vertex>& _ops, const std::string& _name) : generic_obj(MULTI_UNBOUNDED_OBJ, _name), fsm_state(_fsm_state), ops(_ops)
   {
   }

   /**
    * Destructor.
    */
   ~multi_unbounded_obj() override = default;

   /**
    * @return the all done object associated with a multi-unbounded controller
    */
   vertex get_fsm_state() const
   {
      return fsm_state;
   }

   const CustomOrderedSet<vertex>& get_ops()
   {
      return ops;
   }
};

/// RefCount definition for multi_unbounded_obj class
typedef refcount<multi_unbounded_obj> multi_unbounded_objRef;

#endif
