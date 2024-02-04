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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file hls_device.hpp
 * @brief HLS specialization of generic_device
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef HLS_DEVICE_HPP
#define HLS_DEVICE_HPP

#include "generic_device.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_device);
//@}

class HLS_device : public generic_device
{
 public:
   /**
    * Constructor
    */
   HLS_device(const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Destructor
    */
   ~HLS_device() override;

   /**
    * Factory method from XML file
    */
   static HLS_deviceRef factory(const ParameterRef& Param);
};
/// refcount definition of class
using HLS_deviceRef = refcount<HLS_device>;

#endif
