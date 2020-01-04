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
 * @file time_model.hpp
 * @brief Class specification for time_model
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"

#include "Statistics.hpp"
#include "time_model.hpp"

#if HAVE_CMOS_BUILT
#include "liberty_model.hpp"
#endif
#include "LUT_model.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

#include "target_device.hpp"

#include "exceptions.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

const double time_model::execution_time_DEFAULT = 0;
const ControlStep time_model::initiation_time_DEFAULT = ControlStep(0u); /// zero means that the operation is not pipelined
const unsigned int time_model::cycles_time_DEFAULT = 0;                  /// zero means that the operation last in ceil(execution_time/clock_period)
const double time_model::stage_period_DEFAULT = 0;                       /// zero means a non-pipelined operation

time_model::time_model(const ParameterConstRef _Param_)
    : statistical_delay(ComputeStatisticalDelay(execution_time_DEFAULT, 250)),
      Param(_Param_),
      initiation_time(initiation_time_DEFAULT),
      cycles(cycles_time_DEFAULT),
      synthesis_dependent(false),
      stage_period(stage_period_DEFAULT),
      execution_time(execution_time_DEFAULT)
{
}

time_model::~time_model() = default;

void time_model::set_execution_time(double _execution_time, unsigned int _cycles)
{
   execution_time = _execution_time;
   cycles = _cycles;
}

void time_model::set_synthesis_dependent(bool value)
{
   synthesis_dependent = value;
}

bool time_model::get_synthesis_dependent() const
{
   return synthesis_dependent;
}

double time_model::get_execution_time() const
{
   return execution_time;
}

void time_model::set_initiation_time(const ControlStep _initiation_time)
{
   initiation_time = _initiation_time;
}

ControlStep time_model::get_initiation_time() const
{
   return initiation_time;
}

unsigned int time_model::get_cycles() const
{
   return cycles;
}

double time_model::get_stage_period() const
{
   return stage_period;
}

void time_model::set_stage_period(double st_per)
{
   stage_period = st_per;
}

time_modelRef time_model::create_model(TargetDevice_Type dv_type, const ParameterConstRef Param)
{
   switch(dv_type)
   {
      case TargetDevice_Type::FPGA:
         return time_modelRef(new LUT_model(Param));
#if HAVE_CMOS_BUILT
      case TargetDevice_Type::IC:
         return time_modelRef(new liberty_model(Param));
#endif
      default:
         THROW_UNREACHABLE("");
   }
   return time_modelRef();
}

unsigned int time_model::xload_timing_path(xml_element* node)
{
   std::string type;
   LOAD_XVM(type, node);
   unsigned int path_type = 0;
   if(type == "POST_SYNTHESIS")
   {
      path_type = POST_SYNTHESIS;
   }
   else if(type == "POST_LAYOUT")
   {
      path_type = POST_LAYOUT;
   }
   else
   {
      THROW_ERROR("Unknown critical path: " + type);
   }

   unsigned int number_of_elements = 0;
   LOAD_XVM(number_of_elements, node);
   std::vector<std::string> path_elements;
   path_elements.resize(number_of_elements);

   xml_node::node_list infos = node->get_children();
   for(auto& info : infos)
   {
      auto* Enode = GetPointer<xml_element>(info);
      if(!Enode)
         continue;

      if(Enode->get_name() == "path_element")
      {
         std::string path;
         LOAD_XVM(path, Enode);
         unsigned int element;
         LOAD_XVM(element, Enode);
         THROW_ASSERT(element < number_of_elements, "Malformed timing path");
         path_elements[element] = path;
      }
   }

   THROW_ASSERT(path_type != 0, "Unknown critical path type");
   THROW_ASSERT(number_of_elements > 0, "Missing timing path");
   critical_paths[path_type] = path_elements;

   return path_type;
}

std::vector<std::string> time_model::get_critical_path(unsigned int type) const
{
   THROW_ASSERT(critical_paths.find(type) != critical_paths.end(), "Missing critical path type");
   return critical_paths.find(type)->second;
}

bool time_model::has_max_delay(unsigned int type) const
{
   return max_delay.find(type) != max_delay.end();
}

void time_model::set_max_delay(unsigned int type, float value)
{
   max_delay[type] = value;
}

float time_model::get_max_delay(unsigned int type) const
{
   THROW_ASSERT(has_max_delay(type), "Missing value");
   return max_delay.find(type)->second;
}
