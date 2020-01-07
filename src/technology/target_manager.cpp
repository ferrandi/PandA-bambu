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
 * @file target_manager.cpp
 * @brief Implementation of some methods to manage a target for the synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "target_manager.hpp"

#include "target_device.hpp"

target_manager::target_manager(const ParameterConstRef& _Param, const technology_managerRef& _TM, const target_deviceRef& _device) : Param(_Param), TM(_TM), device(_device)
{
   // Technology library manager
   set_technology_manager(_TM);
   /// creating the datastructure representing the target device
   set_target_device(_device);
}

target_manager::~target_manager() = default;

void target_manager::set_technology_manager(const technology_managerRef& _TM)
{
   TM = _TM;
}

const technology_managerRef target_manager::get_technology_manager() const
{
   return TM;
}

void target_manager::set_target_technology(const target_technologyRef& _target)
{
   target = _target;
}

const target_technologyRef target_manager::get_target_technology() const
{
   return target;
}

void target_manager::set_target_device(const target_deviceRef& _device)
{
   device = _device;
   set_target_technology(device->get_target_technology());
}

const target_deviceRef target_manager::get_target_device() const
{
   return device;
}
