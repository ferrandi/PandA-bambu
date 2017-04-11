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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @brief Base class for top entity for context_switch.
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
*/
#ifndef TOP_ENTITY_PARALLEL_CS_H
#define TOP_ENTITY_PARALLEL_CS_H
#include "top_entity.hpp"

class top_entity_parallel_cs : public top_entity
{
   protected:

   /**
    * @brief add_port of top entity
    * @param circuit
    * @param controller_circuit
    * @param datapath_circuit
    */
   void add_port(const structural_objectRef circuit, structural_objectRef controller_circuit, structural_objectRef datapath_circuit);

    /**
     * @brief connect_port_parallel connect datapath and controller
     * @param circuit
     */
    void connect_port_parallel(const structural_objectRef circuit);

    /**
     * @brief propagate_memory_signals from datapath to top and viceversa
     * @param Datapath
     * @param circuit
     */
    void propagate_memory_signals(structural_managerRef Datapath, const structural_objectRef circuit);

public:
   top_entity_parallel_cs(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type);

   /**
    * Destructor
    */
   virtual ~top_entity_parallel_cs();


   /**
    * Add selector and suspension
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec();

};

#endif // TOP_ENTITY_PARALLEL_CS_H
