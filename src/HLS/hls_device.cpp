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
 *   Copyright (C) 2023-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
