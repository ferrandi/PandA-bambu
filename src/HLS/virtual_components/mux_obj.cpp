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
 * @file mux_obj.hpp
 * @brief Base class for multiplexer into datapath
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mux_obj.hpp"

mux_obj::mux_obj(const generic_objRef _first, const generic_objRef _second, unsigned int _level, const std::string& _name, const generic_objRef overall_target)
    : generic_obj(CONNECTION_ELEMENT, _name), bitsize(0), first(_first), second(_second), tree_target(overall_target), level(_level)
{
}

mux_obj::~mux_obj() = default;

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
