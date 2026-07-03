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
 * @file mux_obj.hpp
 * @brief Base class for multiplexer into datapath
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "mux_obj.hpp"

mux_obj::mux_obj(const generic_objRef _first, const generic_objRef _second, unsigned int _level,
                 const std::string& _name, const generic_objRef overall_target)
    : generic_obj(CONNECTION_ELEMENT, _name),
      bitsize(0),
      first(_first),
      second(_second),
      tree_target(overall_target),
      level(_level)
{
}

void mux_obj::set_target(const generic_objRef tgt)
{
   target = tgt;
}

generic_objRef mux_obj::get_final_target()
{
   return tree_target;
}

generic_objRef mux_obj::GetSelector() const
{
   return selector;
}

void mux_obj::set_selector(const generic_objRef sel)
{
   selector = sel;
}

unsigned int mux_obj::get_level() const
{
   return level;
}
unsigned int mux_obj::get_bitsize() const
{
   return bitsize;
}
