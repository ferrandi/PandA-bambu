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
 * @file hls_target.hpp
 * @brief Data structure representing the target information for the HLS
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _HLS_TARGET_HPP_
#define _HLS_TARGET_HPP_

/// superclass include
#include "target_manager.hpp"

#include "target_device.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(target_technology);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_target);
REF_FORWARD_DECL(BackendFlow);
//@}

class HLS_target : public target_manager
{
 public:
   /**
    * Constructor
    */
   HLS_target(const ParameterConstRef& Param, const technology_managerRef& TM, const target_deviceRef& _target);

   /**
    * Destructor
    */
   ~HLS_target() override;

   /**
    * Factory method from XML file
    */
   static HLS_targetRef create_target(const ParameterRef& Param);
};
/// refcount definition of class
typedef refcount<HLS_target> HLS_targetRef;

#endif
