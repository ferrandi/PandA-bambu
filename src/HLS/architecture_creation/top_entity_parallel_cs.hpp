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
 *              Copyright (c) 2016-2024 Politecnico di Milano
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
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

class top_entity_parallel_cs : public top_entity
{
 protected:
   /**
    * @brief connect_port_parallel connect datapath and controller
    * @param circuit
    */
   void connect_port_parallel(const structural_objectRef circuit, unsigned long long loopBW);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   unsigned long long BW_loop_iter(const structural_objectRef circuit);
   void connect_loop_iter(const structural_objectRef circuit, unsigned long long loopBW);
   /**
    * @brief resize_controller_parallel
    * @param controller_circuit
    */
   void resize_controller_parallel(structural_objectRef controller_circuit, unsigned long long loopBW);

 public:
   top_entity_parallel_cs(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                          const DesignFlowManagerConstRef design_flow_manager,
                          const HLSFlowStep_Type _hls_flow_step_type);

   /**
    * Destructor
    */
   ~top_entity_parallel_cs() override;

   /**
    * Add selector and suspension
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif // TOP_ENTITY_PARALLEL_CS_H
