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
 * @file datapath_parallel.hpp
 * @brief Instantiate the kernel and connect port
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */

#ifndef DATAPATH_PARALLEL_CS_H
#define DATAPATH_PARALLEL_CS_H

#include "classic_datapath.hpp"

REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_manager);

class datapath_parallel_cs : public classic_datapath
{
 protected:
   /**
    * @brief instantiate memory_parallel
    * @param HLS
    * @param clock_port
    * @param reset_port
    */
   void instantiate_component_parallel(structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * @brief connect mem_parallel with datapath and kernels
    * @param SM
    * @param memory_modules
    * @param circuit
    */
   void manage_extern_global_port_parallel(const structural_managerRef SM, const CustomOrderedSet<structural_objectRef>& memory_modules, const structural_objectRef circuit);

   /**
    * @brief connect datapath with each kernel
    * @param kernel
    * @param num_kernel
    */
   void connect_module_kernel(structural_objectRef kernel, unsigned int num_kernel);

   /**
    * @brief connect datapath with each kernel
    * @param kernel
    */
   void connect_i_module_kernel(structural_objectRef kernel);

   /**
    * Adds the input/output ports of the module
    */
   virtual void add_ports();

   /**
    * @brief ComputeHLSRelationships datapath need kernel in order to be created
    * @param relationship_type
    * @return
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   /**
    * @brief for each port resize it depending on the type of bus port
    * @param vector_size
    * @param port
    */
   void resize_dimension_bus_port(unsigned int vector_size, structural_objectRef port);

   /**
    * @brief for each port decide the vector size
    * @param mem_par_mod
    */
   void resize_ctrl_parallel_ports(structural_objectRef mem_par_mod);

 public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    */
   datapath_parallel_cs(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor.
    */
   virtual ~datapath_parallel_cs();

   /**
    * @brief InternalExec
    * @return
    */
   DesignFlowStep_Status InternalExec();
};

#endif // DATAPATH_PARALLEL_CS_H
