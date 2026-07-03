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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file OMPCGExt.hpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef OMP_CG_EXT_HPP
#define OMP_CG_EXT_HPP

#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "refcount.hpp"
#include <map>
#include <stack>
#include <string>
#include <vector>

REF_FORWARD_DECL(OMPInfo);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(time_info);

class OMPCGExt : public FunctionFrontendFlowStep
{
 private:
   /**
    * Fork call shared functions' set
    */
   static std::map<unsigned int, CustomSet<unsigned int>> shared_infos;

   static unsigned int _thread_context_count;

   const time_infoRef _assign_time_info;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Complete setup of __kmp_bambu_fork_call versioning, generates function body and multiple lambda call
    * statements
    * @param version_fnode Cloned __kmp_bambu_fork_call function node
    * @param omp_lambda_fnode omp lambda function node
    * @param nthread required number of threads
    */
   void ExpandForkCall(const ir_nodeRef& version_fnode, const ir_nodeRef& omp_lambda_fnode,
                       const unsigned int nthread) const;

   /**
    * Generate tid_from_gtid getter for a specific fork call
    * @param cores Fork infos of all cores in the fork call
    * @param nthread required number of threads
    * @param ncontext maximum number of contexts per core
    */
   void GenerateThreadIDAccessors(const std::vector<OMPInfoRef>& cores, const unsigned int nthread,
                                  const unsigned int ncontext) const;

   /**
    * Generate thread local reduce data getters and setters for a specific fork call as software functions
    * @param cores Fork infos of all cores in the fork call
    */
   void GenerateThreadLocalDataAccessorsSW(const std::vector<OMPInfoRef>& cores) const;

   /**
    * Generate thread local reduce data getters and setters for a specific fork call as hardware modules
    * @param cores Fork infos of all cores in the fork call
    */
   void GenerateThreadLocalDataAccessorsHW(const std::vector<OMPInfoRef>& cores) const;

   /**
    * Generate barrier getters and setters for a specific fork call
    * @param cores Fork infos of all cores in the fork call
    */
   void GenerateBarrierAccessors(const std::vector<OMPInfoRef>& cores) const;

   /**
    * Generate context switch getters for gtid and tid of each core
    * @param cores Fork infos of all cores in the fork call
    */
   void GenerateContextSwitchInterface(const std::vector<OMPInfoRef>& cores) const;

   /**
    * Add function to the call graph manager and set single channel memory interface
    * (and optional number of supported contexts)
    * @param fid Funtion id
    * @param omp_info OMP information relative to the function if any
    */
   void SetSimpleMemFunction(const unsigned int fid, OMPInfoRef omp_info = nullptr) const;

   /**
    * Bind function signature to library functional unit
    * @param fu_name Functional unit name
    * @param fnode Function declaration IR node
    * @param time_info Time model for the operation associated to the functional unit (copied from first fu_name
    * operation if nullptr)
    * @param op_bounded True if fu operation is time bounded, false else
    */
   void BindFunction(const std::string& fu_name, const ir_nodeRef& fnode, time_infoRef time_info = nullptr,
                     bool op_bounded = true) const;

   /**
    * Generate replicas of a function for a specific fork call
    * @param fname Function name to replicate
    * @param cores Fork infos of all cores in the fork call
    * @return std::vector<ir_nodeRef> Generated functions set
    */
   std::vector<ir_nodeRef> GenerateOMPReplicas(const std::string& fname, const std::vector<OMPInfoRef>& cores) const;

   /**
    * Generate replicas of a function for a specific fork call and bind each to a unique functiona unit
    * @param fname Function name to replicate
    * @param fu_name Functional unit name to bind
    * @param fu_bounded True if functional unit is bounded, false else
    * @param cores Fork infos of all cores in the fork call
    * @return std::vector<ir_nodeRef> Generated functions set
    */
   std::vector<ir_nodeRef> GenerateOMPReplicasFU(const std::string& fname, const std::string& fu_name,
                                                 const bool fu_bounded, const std::vector<OMPInfoRef>& cores) const;

   /**
    * Clone existing functional unit from components' library
    * @param fu_name Existing functional unit name
    * @param clone_name Cloned functional unit name
    */
   void CloneFU(const std::string& fu_name, const std::string& clone_name) const;

 protected:
   /**
    * OMP fork call cores information
    */
   static CustomMap<unsigned int, std::vector<OMPInfoRef>> fork_infos;

   const ir_managerRef TM;

   const ir_manipulationRef ir_man;

   OMPCGExt(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_id,
            const FrontendFlowStepType frontend_flow_step_type, const DesignFlowManager& design_flow_manager);

   /**
    * Generate topologically sorted list of given basic blocks from the dominator graph
    * @param function_id
    * @param AppM
    * @return std::vector<blocRef>
    */
   static std::vector<blocRef> DominatorTopologicalSort(const unsigned int function_id,
                                                        const application_managerRef& AppM);

   /**
    * Generate critical and end_critical interface for a specific lock id and fork call
    * @param loc_id Lock id
    * @param cores Fork infos of all cores in the fork call
    */
   void GenerateCriticalAccessors(const unsigned int loc_id, const std::vector<OMPInfoRef>& cores) const;

   /**
    * Clone generic function generating suffix from fork information structure
    * @param base_fnode Function declaration IR node to clone
    * @param function_info Function information
    * @return ir_nodeRef Cloned function IR node
    */
   ir_nodeRef CloneFunction(const ir_nodeRef& base_fnode, const OMPInfoRef& function_info) const;

 public:
   /**
    * Constructor.
    * @param parameters is the set of input parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   OMPCGExt(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_id,
            const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   /**
    * Updates the IR to have a more compliant CFG
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Get cores information for given fork id
    * @param fork_id Fork id
    * @return OMPInfoRef Cores information for fork id
    */
   static std::vector<OMPInfoRef> GetOMPForkInfo(unsigned int fork_id);

   /**
    * Compute the number of threads to implement on a given core id
    * @param core_id Current OMP core id
    * @param nthread Total number of threads for the current fork call
    * @param ncontext Maximum number of contexts for each core in current fork call
    * @return unsigned int Number of threads to implement for given core id
    */
   static unsigned int ComputeCurrentThreads(unsigned int core_id, unsigned int nthread, unsigned int ncontext);

   /**
    * Generate omp process suffix for given sequential identifier
    * @param sequential_id sequential function duplicate id
    * @return std::string omp function suffix
    */
   static std::string OMPProcSuffix(unsigned int sequential_id);

   /**
    * Generate omp fork suffix for given sequential identifier
    * @param fork_call_id fork call function id
    * @param sequential_id sequential function duplicate id
    * @return std::string omp fork suffix
    */
   static std::string OMPForkSuffix(unsigned int fork_call_id, unsigned int sequential_id);
};
#endif
