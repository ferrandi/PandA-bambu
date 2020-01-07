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
 * @file classic_datapath.hpp
 * @brief Base class for usual datapath creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 * $Locker:  $
 * $State: Exp $
 *
 */

#ifndef CLASSIC_DATAPATH_HPP
#define CLASSIC_DATAPATH_HPP

#include "datapath_creator.hpp"
REF_FORWARD_DECL(structural_object);

class classic_datapath : public datapath_creator
{
 protected:
   /**
    * Adds the clock and reset ports to the structural description of the circuit
    * @param clock_sign is the object representing the clock signal
    * @param reset_sign is the object representing the reset signal
    */
   void add_clock_reset(structural_objectRef& clock_sign, structural_objectRef& reset_sign);

   /**
    * Adds the input/output ports of the module
    */
   virtual void add_ports();

 public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    */
   classic_datapath(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR);

   /**
    * Destructor.
    */
   ~classic_datapath() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
