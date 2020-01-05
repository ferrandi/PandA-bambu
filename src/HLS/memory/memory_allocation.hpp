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
 * @file memory_allocation.hpp
 * @brief Base class to allocate memories in high-level synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef _MEMORY_ALLOCATION_HPP_
#define _MEMORY_ALLOCATION_HPP_

#include "hls_step.hpp"
REF_FORWARD_DECL(memory_allocation);
#include <list>

/**
 * The allocation memory polycy
 */
enum class MemoryAllocation_Policy
{
   LSS = 0,            /// all local variables, static variables and strings are allocated on BRAMs
   GSS,                /// all global variables, static variables and strings are allocated on BRAMs
   ALL_BRAM,           /// all objects that need to be stored in memory are allocated on BRAMs
   NO_BRAM,            /// all objects that need to be stored in memory are allocated on an external memory
   EXT_PIPELINED_BRAM, /// all objects that need to be stored in memory are allocated on an external pipelined memory
   INTERN_UNALIGNED,   /// all objects with an unaligned access are clustered on a single STD_BRAM
   NONE                /// no policy
};

/**
 * The number of channels
 */
enum class MemoryAllocation_ChannelsType
{
   MEM_ACC_11 = 0, /// for each memory at maximum one direct access and one indirect access
   MEM_ACC_N1,     /// for each memory at maximum n parallel direct accesses and one indirect access
   MEM_ACC_NN,     /// for each memory at maximum n parallel direct accesses and n parallel indirect accesses
   MEM_ACC_P1N,    /// only external memory access Datapath see only 1 memory port, while the bus manage parallel accesses
   MEM_ACC_CS      /// memory architecture for non blocking request
};

/**
 * The information about how memory allocation has to be specialized
 */
class MemoryAllocationSpecialization : public HLSFlowStepSpecialization
{
 public:
   /// memory allocation policy
   const MemoryAllocation_Policy memory_allocation_policy;

   /// number of channels
   const MemoryAllocation_ChannelsType memory_allocation_channels_type;

   /**
    * Constructor
    * @param memory_allocation_policy is the memory allocation policy
    * @param memory_allocation_channels_type is the number of channels
    */
   MemoryAllocationSpecialization(const MemoryAllocation_Policy memory_allocation_policy, const MemoryAllocation_ChannelsType memory_allocation_channels_type);

   /**
    * Return the string representation of this
    */
   const std::string GetKindText() const override;

   /**
    * Return the contribution to the signature of a step given by the specialization
    */
   const std::string GetSignature() const override;
};

/**
 * Allocation memory class
 */
class memory_allocation : public HLS_step
{
 protected:
   /// list of functions to be analyzed
   CustomOrderedSet<unsigned int> func_list;

   /// The memory allocation policy
   const MemoryAllocation_Policy memory_allocation_policy;

   /// The version of memory representation on which this step was applied
   unsigned int memory_version;

   /// The version of BB IR representation on which this step was applied
   std::map<unsigned int, unsigned int> last_bb_ver;

   /// The version of bit value IR representation on which this step was applied
   std::map<unsigned int, unsigned int> last_bitvalue_ver;

   /**
    * Prepares the datastructures for the memory allocation
    */
   void setup_memory_allocation();

   /**
    * Performs a final analysis of the memory allocation to finalize the datastructure
    */
   void finalize_memory_allocation();

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() = 0;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the algorithm to be used
    */
   memory_allocation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
                     const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * Destructor
    */
   ~memory_allocation() override;

   void allocate_parameters(unsigned int functionId);

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};

#endif
