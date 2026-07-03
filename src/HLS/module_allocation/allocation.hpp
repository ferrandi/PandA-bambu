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
 * @file allocation.hpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @defgroup allocation Allocation Package
 * @ingroup HLS
 *
 * Since all the HLS packages use this module to retrieve technology information
 * (see \ref src_HLS_module_allocation for details), it is also used to
 * introduce a simple functional unit allocator (see \ref src_HLS_binding_constraints_page for details).
 * It returns a list of functional units compatible with the graph under analysis. Each functional unit
 * is represented by an id of unsigned int type. Given this id, all the HLS packages can retrieve:
 *  - performances (i.e., execution time, initiation time, power consumption, area of the functional unit)
 *  - constraints (number of available functional units)
 * On the other hand, the hls_flow class can control the allocation of the functional units during
 * the instantiation of the allocation object.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP
#include "hls_function_step.hpp"

#include "design_flow_step.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "refcount.hpp"

#include <string>

REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(HLS_constraints);
REF_FORWARD_DECL(HLS_device);
REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(node_kind_prec_info);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(technology_manager);
struct functional_unit;
struct operation;

enum class Allocation_MinMax
{
   MIN,
   MAX
};

/**
 * @class allocation
 * @ingroup allocation
 *
 * This wrapper collects all the methods used by the High-level synthesis classes to
 * retrieve information about the functional units.
 */
class allocation : public HLSFunctionStep
{
 protected:
   /// The allocation solution
   AllocationInformationRef allocation_information;
   friend struct updatecopy_HLS_constraints_functor;

   /// store the precomputed pipeline unit: given a functional unit it return the pipeline id compliant
   std::map<std::string, std::string> precomputed_pipeline_unit;

   std::map<technology_nodeRef, std::map<unsigned long long, std::map<HLS_manager::io_binding_type, unsigned int>>>
       fu_list;

   /// The HLS target
   HLS_deviceRef HLS_D;

   /// The technology manager
   technology_managerRef TechM;

   /**
    * Returns the technology_node associated with the given operation
    * @param fu_name is the string representing the name of the unit
    */
   technology_nodeRef get_fu(const std::string& fu_name) const;

   /**
    * In case the current functional unit has pipelined operations
    * then it return an id identifying the most compliant functional unit given the current clock period
    */
   std::string get_compliant_pipelined_unit(double clock, const std::string& pipe_parameter,
                                            const technology_nodeRef current_fu, const std::string& curr_op,
                                            const std::string& library_name, const std::string& template_suffix,
                                            unsigned long long module_prec);

   technology_nodeRef extract_bambu_provided(const std::string& library_name, operation* curr_op,
                                             const std::string& bambu_provided_resource_);

   /**
    * set the number of ports associated with the functional unit
    * @param fu_name is the functional unit id
    * @param n_ports is the number of ports
    */
   void set_number_channels(unsigned int fu_name, unsigned int n_ports);

   double get_execution_time_dsp_modified(const unsigned int fu_name, const technology_nodeRef& node_op) const;

   double get_stage_period_dsp_modified(const unsigned int fu_name, const technology_nodeRef& node_op) const;

   /**
    * Add a proxy function to the WORK library.
    */
   void add_proxy_function_module(const HLS_constraintsRef HLS_C, technology_nodeRef techNode_obj,
                                  const std::string& orig_fun_name);

   /**
    *  Add a proxy wrapper to the WORK library
    */
   void add_proxy_function_wrapper(const std::string& library_name, technology_nodeRef techNode_obj,
                                   const std::string& orig_fun_name);

   /**
    * Build the proxy wrapper
    */
   void BuildProxyWrapper(functional_unit* current_fu, const std::string& orig_fun_name,
                          const std::string& orig_library_name);

   /**
    * Build the proxy function in Verilog
    */
   void BuildProxyFunctionVerilog(functional_unit* current_fu);

   /**
    * Build the proxy function in VHDL
    */
   void BuildProxyFunctionVHDL(functional_unit* current_fu);

   /**
    * Build the proxy function
    */
   void BuildProxyFunction(functional_unit* current_fu);

   void add_tech_constraint(technology_nodeRef cur_fu, unsigned int tech_constrain_value, unsigned int pos,
                            bool proxy_constrained);
   bool check_templated_units(double clock_period, node_kind_prec_infoRef node_info, const library_managerRef library,
                              technology_nodeRef current_fu, operation* curr_op);
   bool check_for_memory_compliancy(bool Has_extern_allocated_data, technology_nodeRef current_fu,
                                    const std::string& memory_ctrl_type, const std::string& channels_type,
                                    const bool bus_has_variable_latency);
   bool check_type_and_precision(operation* curr_op, bool no_constant_characterization,
                                 node_kind_prec_infoRef node_info);
   bool check_proxies(const library_managerRef library, const std::string& fu_name_);
   bool check_generated_bambu(structural_managerRef structManager_obj, std::string& bambu_provided_resource,
                              technology_nodeRef current_fu);
   bool is_ram_not_timing_compliant(const HLS_constraintsRef HLS_C, unsigned int var, technology_nodeRef current_fu);
   std::string get_synch_ram_latency(const std::string& ram_template, const std::string& latency_postfix,
                                     const HLS_constraintsRef HLS_C, unsigned int var);

   /**
    * @brief Check if given functional unit has no_constant_characterizaiton flag set
    *
    * @param library Component library of fu_node
    * @param fu_node Functional unit technology node
    * @return true If fu_node has a template functional unit with no_constant_characterizaiton flag set
    * @return false If fu_node has a template functional unit with no_constant_characterizaiton flag not set or does not
    * have a template
    */
   bool is_no_constant_characterization(const library_managerRef& library, const technology_nodeRef& fu_node) const;

   /**
    * Integrate technology libraries with special functional units
    */
   virtual void IntegrateTechnologyLibraries();

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   allocation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
              const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type = HLSFlowStep_Type::ALLOCATION);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   void PrintInitialIR() const override;
};
#endif
