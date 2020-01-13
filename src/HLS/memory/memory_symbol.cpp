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
 * @file memory_symbol.cpp
 * @brief Implementations of the methods to manage memory symbols
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "memory_symbol.hpp"

#include "exceptions.hpp"

#include "language_writer.hpp"
#include "string_manipulation.hpp" //STR
#include "utility.hpp"

memory_symbol::memory_symbol(unsigned int var, unsigned int addr, unsigned int funId) : variable(var), name(STR(MEM_PREFIX) + "var_" + STR(var) + "_" + STR(funId)), address(addr), resolved(false)
{
}

memory_symbol::~memory_symbol() = default;

void memory_symbol::set_address(unsigned int _address)
{
   address = _address;
}

void memory_symbol::set_symbol_name(const std::string& _name)
{
   name = _name;
}

std::string memory_symbol::get_symbol_name() const
{
   return name;
}

unsigned int memory_symbol::get_address() const
{
   return address;
}

bool memory_symbol::is_resolved() const
{
   return resolved;
}

std::string memory_symbol::get_address_string(unsigned int precision) const
{
   return STR("\"") + convert_to_binary(static_cast<unsigned long long int>(address), precision) + "\"";
}

unsigned int memory_symbol::get_variable() const
{
   return variable;
}
