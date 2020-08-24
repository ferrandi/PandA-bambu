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
 * @file allocation.hpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @defgroup allocation Allocation Package
 * @ingroup HLS
 *
 * Since all the HLS packages use this module to retrieve technology information
 * (see \ref src_HLS_allocation_page for details), it is also used to
 * introduce a simple functional unit allocator (see \ref src_HLS_binding_constraints_page for details).
 * It returns a list of functional units compatible with the graph under analysis. Each functional unit
 * is represented by an id of unsigned int type. Given this id, all the HLS packages can retrieve:
 *  - performances (i.e., execution time, initiation time, power consumption, area of the functional unit)
 *  - constraints (number of available functional units)
 * On the other hand, the hls_flow class can control the allocation of the functional units during
 * the instantiation of the allocation object.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP

#include "design_flow_step.hpp"  // for DesignFlowManagerConstRef, Design...
#include "hls_function_step.hpp" // for HLSFunctionStep
#include "hls_manager.hpp"       // for HLS_manager, HLS_manager::io_bind...
#include "hls_step.hpp"          // for HLSFlowStep_Type, HLSFlowStep_Typ...
#include "refcount.hpp"          // for REF_FORWARD_DECL, CONSTREF_FORWAR...
#include <string>                // for string

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(HLS_constraints);
REF_FORWARD_DECL(HLS_target);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(allocation);
REF_FORWARD_DECL(graph);
REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(node_kind_prec_info);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(technology_manager);
struct functional_unit;
struct operation;
//@}

/**
 * @name Min or max
 */
//@{
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
   std::map<unsigned int, unsigned int> last_bb_ver;

   /// The allocation solution
   AllocationInformationRef allocation_information;
   friend struct updatecopy_HLS_constraints_functor;

   /// store the precomputed pipeline unit: given a functional unit it return the pipeline id compliant
   std::map<std::string, std::string> precomputed_pipeline_unit;

   std::map<technology_nodeRef, std::map<unsigned int, std::map<HLS_manager::io_binding_type, unsigned int>>> fu_list;

   /// The HLS target
   HLS_targetRef HLS_T;

   /// The technology manager
   technology_managerRef TM;

   /**
    * Returns the technology_node associated with the given operation
    * @param fu_name is the string representing the name of the unit
    */
   technology_nodeRef get_fu(const std::string& fu_name);

   /**
    * In case the current functional unit has pipelined operations
    * then it return an id identifying the most compliant functional unit given the current clock period
    */
   std::string get_compliant_pipelined_unit(double clock, const std::string& pipe_parameter, const technology_nodeRef current_fu, const std::string& curr_op, const std::string& library_name, const std::string& template_suffix, unsigned int module_prec);

   technology_nodeRef extract_bambu_provided(const std::string& library_name, operation* curr_op, const std::string& bambu_provided_resource_);

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
   void add_proxy_function_module(const HLS_constraintsRef HLS_C, technology_nodeRef techNode_obj, const std::string& orig_fun_name);

   /**
    *  Add a proxy wrapper to the WORK library
    */
   void add_proxy_function_wrapper(const std::string& library_name, technology_nodeRef techNode_obj, const std::string& orig_fun_name);

   /**
    * Build the proxy wrapper
    */
   void BuildProxyWrapper(functional_unit* current_fu, const std::string& orig_fun_name, const std::string& orig_library_name);

   /**
    * Build the proxy function in Verilog
    */
   void BuildProxyFunctionVerilog(functional_unit* orig_fu);

   /**
    * Build the proxy function in VHDL
    */
   void BuildProxyFunctionVHDL(functional_unit* orig_fu);

   /**
    * Build the proxy function
    */
   void BuildProxyFunction(functional_unit* orig_fu);

   void add_tech_constraint(technology_nodeRef cur_fu, unsigned int tech_constrain_value, unsigned int pos, bool proxy_constrained);
   void add_resource_to_fu_list(std::string channels_type, const OpGraphConstRef g, technology_nodeRef current_fu, CustomOrderedSet<vertex> vertex_analysed, node_kind_prec_infoRef node_info, unsigned int current_id,
                                CustomOrderedSet<vertex>::const_iterator vert, const std::vector<std::string>& libraries, bool isMemory, std::string bambu_provided_resource, operation* curr_op, std::string specialized_fuName, bool predicate_2,
                                std::string current_op, HLS_manager::io_binding_type constant_id, bool varargs_fu, unsigned int l, std::string memory_ctrl_type, std::map<std::string, technology_nodeRef> new_fu, unsigned int tech_constrain_value);
   bool check_templated_units(double clock_period, node_kind_prec_infoRef node_info, const library_managerRef library, technology_nodeRef current_fu, operation* curr_op);
   bool check_for_memory_compliancy(bool Has_extern_allocated_data, technology_nodeRef current_fu, const std::string& memory_ctrl_type, std::string channels_type);
   bool check_type_and_precision(operation* curr_op, node_kind_prec_infoRef node_info);
   bool check_proxies(const library_managerRef library, const std::string& fu_name_);
   bool check_generated_bambu_flopoco(bool skip_softfloat_resources, structural_managerRef structManager_obj, std::string& bambu_provided_resource, bool skip_flopoco_resources, technology_nodeRef current_fu);
   bool is_ram_not_timing_compliant(const HLS_constraintsRef HLS_C, unsigned int var, technology_nodeRef current_fu);
   std::string get_synch_ram_latency(const std::string& ram_template, const std::string& latency_postfix, const HLS_constraintsRef HLS_C, unsigned int var);

   /**
    * Integrate technology libraries with special functional units
    */
   virtual void IntegrateTechnologyLibraries();

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * @name Constructors and destructors.
    */
   //@{
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    */
   allocation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type = HLSFlowStep_Type::ALLOCATION);

   /**
    * Destructor.
    */
   ~allocation() override;
   //@}

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Dump the initial intermediate representation
    */
   void PrintInitialIR() const override;
};
#endif
