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
 * @file hls_device.hpp
 * @brief HLS specialization of generic_device
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef HLS_DEVICE_HPP
#define HLS_DEVICE_HPP

#include "generic_device.hpp"

REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_device);
CONSTREF_FORWARD_DECL(HLS_device);

class HLS_device : public generic_device
{
 public:
   HLS_device(const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Factory method from XML file
    */
   static HLS_deviceRef factory(const ParameterRef& Param);
};
/// refcount definition of class
using HLS_deviceRef = refcount<HLS_device>;
using HLS_deviceConstRef = refcount<const HLS_device>;

#endif
