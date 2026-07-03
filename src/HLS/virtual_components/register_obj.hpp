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
 * @file register_obj.hpp
 * @brief Base class for all register into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "generic_obj.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"

/**
 * class modeling a register object
 */
class register_obj : public generic_obj
{
   generic_objRef wr_enable;

 private:
   unsigned int register_index;

 public:
   /**
    * This is the constructor of the object class, with a given id
    * @param index is the new value for register entry
    */
   explicit register_obj(const unsigned int index) : generic_obj(REGISTER, std::string("reg_") + STR(index))
   {
      register_index = index;
   }

   /**
    * Gets the write enable object for the given register
    * @return a set of sets where each of them can enable register write (when all conditions contained are
    *        true)
    */
   generic_objRef get_wr_enable() const
   {
      return wr_enable;
   }

   /**
    * Sets the write enable for given register
    */
   void set_wr_enable(const generic_objRef& wr_en)
   {
      wr_enable = wr_en;
   }

   /**
    * Gets the index of the register represented by this object
    * @return the index of the represented register
    */
   unsigned int get_register_index()
   {
      return register_index;
   }
};

/// RefCount definition for register_obj class
using register_objRef = refcount<register_obj>;

#endif
