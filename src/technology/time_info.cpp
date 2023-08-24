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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file time_info.cpp
 * @brief Collect information about resource performance
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "time_info.hpp"

#include "schedule.hpp"

const double time_info::execution_time_DEFAULT = 0;
const ControlStep time_info::initiation_time_DEFAULT =
    ControlStep(0u); /// zero means that the operation is not pipelined
const unsigned int time_info::cycles_time_DEFAULT =
    0; /// zero means that the operation last in ceil(execution_time/clock_period)
const double time_info::stage_period_DEFAULT = 0; /// zero means a non-pipelined operation

time_info::time_info(const ParameterConstRef _Param)
    : Param(_Param),
      initiation_time(initiation_time_DEFAULT),
      cycles(cycles_time_DEFAULT),
      synthesis_dependent(false),
      stage_period(stage_period_DEFAULT),
      execution_time(execution_time_DEFAULT)
{
}

time_info::~time_info() = default;

void time_info::set_execution_time(double _execution_time, unsigned int _cycles)
{
   execution_time = _execution_time;
   cycles = _cycles;
}

void time_info::set_synthesis_dependent(bool value)
{
   synthesis_dependent = value;
}

bool time_info::get_synthesis_dependent() const
{
   return synthesis_dependent;
}

double time_info::get_execution_time() const
{
   return execution_time;
}

void time_info::set_initiation_time(const ControlStep _initiation_time)
{
   initiation_time = _initiation_time;
}

ControlStep time_info::get_initiation_time() const
{
   return initiation_time;
}

unsigned int time_info::get_cycles() const
{
   return cycles;
}

double time_info::get_stage_period() const
{
   return stage_period;
}

void time_info::set_stage_period(double st_per)
{
   stage_period = st_per;
}

time_infoRef time_info::factory(const ParameterConstRef Param)
{
   return time_infoRef(new time_info(Param));
}
