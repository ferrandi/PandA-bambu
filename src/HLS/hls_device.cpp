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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file hls_device.cpp
 * @brief HLS specialization of generic_device
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "hls_device.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "polixml.hpp"
#include "technology_manager.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

HLS_device::HLS_device(const ParameterConstRef& _Param, const technology_managerRef& _TM) : generic_device(_Param, _TM)
{
   if(_Param->isOption(OPT_clock_period))
   {
      auto clock_period_value = _Param->getOption<double>(OPT_clock_period);
      set_parameter("clock_period", clock_period_value);
   }
}

HLS_deviceRef HLS_device::factory(const ParameterRef& Param)
{
   return HLS_deviceRef(new HLS_device(Param, technology_managerRef(new technology_manager(Param))));
}
