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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file classic_datapath.hpp
 * @brief Base class for usual datapath creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
    * @param clock_obj is the object representing the clock signal
    * @param reset_obj is the object representing the reset signal
    */
   void add_clock_reset(structural_objectRef& clock_obj, structural_objectRef& reset_obj);

   /**
    * Adds the input/output ports of the module
    */
   virtual void add_ports();

 public:
   classic_datapath(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                    const DesignFlowManager& design_flow_manager,
                    const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR);

   DesignFlowStep_Status InternalExec() override;
};
#endif
