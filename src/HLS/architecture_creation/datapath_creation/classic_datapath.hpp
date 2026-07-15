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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
