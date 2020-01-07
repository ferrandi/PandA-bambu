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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file conn_binding_cs.cpp
 * @brief Connect the new component instantiated by the flow context_switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */

#ifndef CONN_BINDING_CS_H
#define CONN_BINDING_CS_H

#include "conn_binding.hpp"

class conn_binding_cs : public conn_binding
{
 public:
   /**
    * @Constructor
    */
   conn_binding_cs(const BehavioralHelperConstRef BH, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~conn_binding_cs();

   /**
    * @brief add_to_SM
    * @param HLSMgr
    * @param HLS
    * @param SM
    */
   void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM);

 protected:
   /**
    * @brief connect_suspension_component
    * @param HLSMgr
    * @param HLS
    */
   void instantiate_suspension_component(const HLS_managerRef HLSMgr, const hlsRef HLS);

   /**
    * @brief connectOutOr depending if module is kernel or another connect out
    * @param HLSMgr
    * @param HLS
    * @param port_out_or
    */
   void connectOutOr(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef port_out_or);
};

#endif // CONN_BINDING_CS_H
