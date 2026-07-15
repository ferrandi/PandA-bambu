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
