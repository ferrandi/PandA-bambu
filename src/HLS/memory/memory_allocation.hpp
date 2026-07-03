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
 * @file memory_allocation.hpp
 * @brief Base class to allocate memories in high-level synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef _MEMORY_ALLOCATION_HPP_
#define _MEMORY_ALLOCATION_HPP_

#include "custom_set.hpp"
#include "hls_step.hpp"
#include "refcount.hpp"
#include <list>
#include <map>
#include <string>
#include <tuple>

REF_FORWARD_DECL(memory_allocation);
REF_FORWARD_DECL(memory);

/**
 * The allocation memory polycy
 */
enum class MemoryAllocation_Policy
{
   NO_BRAM = 0,  /// all objects that need to be stored in memory are allocated on an external memory
   LSS = 1,      /// all local variables, static variables and strings are allocated on BRAMs
   GSS = 2,      /// all global variables, static variables and strings are allocated on BRAMs
   GLSS = 3,     /// all global variables, local variables, strings and static variables are allocated on BRAMs
   ALL_BRAM = 4, /// all objects that need to be stored in memory are allocated on BRAMs
   EXT_PIPELINED_BRAM =
       5,   /// all objects that need to be stored in memory are allocated on an external pipelined memory
   NONE = 7 /// no policy
};

/**
 * The number of channels
 */
enum class MemoryAllocation_ChannelsType
{
   MEM_ACC_11 = 0, /// for each memory at maximum one direct access and one indirect access
   MEM_ACC_N1 = 1, /// for each memory at maximum n parallel direct accesses and one indirect access
   MEM_ACC_NN = 3, /// for each memory at maximum n parallel direct accesses and n parallel indirect accesses
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
   MemoryAllocationSpecialization(const MemoryAllocation_Policy memory_allocation_policy,
                                  const MemoryAllocation_ChannelsType memory_allocation_channels_type);

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};

/**
 * Allocation memory class
 */
class memory_allocation : public HLS_step
{
 private:
   /* Sum of reached body functions' bb+bitvalue versions after last Exec call */
   unsigned int last_ver_sum;

 protected:
   /// list of functions to be analyzed
   CustomOrderedSet<unsigned int> func_list;

   /// The version of memory representation on which this step was applied
   unsigned int memory_version;

   /**
    * Prepares the data structures for the memory allocation
    */
   void setup_memory_allocation();

   /**
    * Performs a final analysis of the memory allocation to finalize the data-structure
    */
   void finalize_memory_allocation();

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() = 0;

 public:
   /**
    * Constructor
    * @param _parameters is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the algorithm to be used
    * @param hls_flow_step_specialization is the optional specialization associated with this step
    */
   memory_allocation(
       const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, const DesignFlowManager& design_flow_manager,
       const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * @brief Allocate parameters for given function
    *
    * @param functionId Id of the function to allocate paramters for
    * @param Rmem memory object ref to use (Rmem from HLS manager used if nullptr)
    */
   void allocate_parameters(unsigned int functionId, const std::unique_ptr<memory>& Rmem = nullptr);

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const final;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() final;
};

#endif
