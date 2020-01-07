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
 * @file allocation_information.hpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef ALLOCATION_SOLUTION_HPP
#define ALLOCATION_SOLUTION_HPP

/// Superclass include
#include "hls_function_ir.hpp"

#include "graph.hpp" // for vertex
#include "hash_helper.hpp"
#include "hls_manager.hpp" // for HLS_manager, HLS_manager::io_binding_type
#include "op_graph.hpp"    // for OpGraphConstRef
#include "refcount.hpp"    // for CONSTREF_FORWARD_DECL, REF_FORWARD_DECL
#include "schedule.hpp"
#include "utility.hpp" // for UINT_STRONG_TYPEDEF_FORWARD_DECL
#include <cstddef>     // for size_t
#include <iosfwd>      // for ostream
#include <string>      // for string
#include <utility>     // for pair

CONSTREF_FORWARD_DECL(AllocationInformation);
CONSTREF_FORWARD_DECL(BehavioralHelper);
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);
CONSTREF_FORWARD_DECL(HLS_constraints);
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(HLS_target);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(Schedule);
enum class Allocation_MinMax;
CONSTREF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(memory);
REF_FORWARD_DECL(node_kind_prec_info);
REF_FORWARD_DECL(technology_node);
CONSTREF_FORWARD_DECL(tree_manager);
struct updatecopy_HLS_constraints_functor;
struct operation;

/// artificial functional units
#define GIMPLE_RETURN_STD "gimple_return_FU"
#define GIMPLE_PHI_STD "gimple_phi_FU"
#define GIMPLE_ASM_STD "gimple_asm_FU"
#define GIMPLE_LABEL_STD "gimple_label_FU"
#define GIMPLE_GOTO_STD "gimple_goto_FU"
#define GIMPLE_NOP_STD "gimple_nop_FU"
#define GIMPLE_PRAGMA_STD "gimple_pragma_FU"
#define READ_COND_STD "read_cond_FU"
#define MULTI_READ_COND_STD "multi_read_cond_FU"
#define SWITCH_COND_STD "switch_cond_FU"
#define ENTRY_STD "entry_FU"
#define EXIT_STD "exit_FU"
#define NOP_STD "nop_FU"

#define ALLOCATION_MUX_MARGIN 1.0

class AllocationInformation : public HLSFunctionIR
{
 private:
   friend class allocation;
   friend struct updatecopy_HLS_constraints_functor;

   /// coefficient used to estimate connection delay
   double connection_time_ratio;

   /// coefficient used to estimate the controller delay
   double controller_delay_multiplier;

   /// coefficient used to modify setup value
   double setup_multiplier;

   /// coefficient used to modify execution time and stage time
   double time_multiplier;

   double mux_time_multiplier;

   /// coefficient used to correct timing of memories
   double memory_correction_coefficient;

   double connection_offset;

   /// Connection delay at the exit of DSP
   double output_DSP_connection_time;

   /// Connection delay at the exit of carry
   double output_carry_connection_time;

   /// The coefficient used for estimating fanout delay
   double fanout_coefficient;

   /// The maximum size of fanout
   size_t max_fanout_size{0};

   /// coefficient used to modify DSPs execution time
   double DSPs_margin;

   /// coefficient used to modify DSPs stage period
   double DSPs_margin_stage;

   /// coefficient used to modify pipelining DSPs allocation
   double DSP_allocation_coefficient;

   /// The operation graph
   OpGraphConstRef op_graph;

   /// HLS constraints
   HLS_constraintsConstRef HLS_C;

   /// reference to the information representing the target for the synthesis
   HLS_targetConstRef HLS_T;

   /// The behavioral helper
   BehavioralHelperConstRef behavioral_helper;

   /// The memory
   memoryConstRef Rmem;

   const unsigned& address_bitsize;

   /// The tree manager
   tree_managerConstRef TreeM;

   /// minimum slack
   double minimumSlack;

   /// number of complex operations
   unsigned int n_complex_operations{0};

   /// map between the functional unit identifier and the pair (library, fu) of names for the unit
   CustomUnorderedMap<unsigned int, std::pair<std::string, std::string>> id_to_fu_names;

   /// Store the set of functional units (identifiers) uniquely bounded
   CustomOrderedSet<unsigned int> is_vertex_bounded_rel;

   /// Stores the list of the functional units
   std::vector<technology_nodeRef> list_of_FU;

   /// map between memory units and the associated variables
   std::map<unsigned int, unsigned int> memory_units;

   /// define the number of ports associated with the functional unit
   std::map<unsigned int, unsigned int> nports_map;

   /// map functional units with their precision
   std::map<unsigned int, unsigned int> precision_map;

   /// put into relation proxy function units with shared functions
   std::map<unsigned int, std::string> proxy_function_units;

   /// put into relation proxy memory units with variables
   std::map<unsigned int, unsigned int> proxy_memory_units;

   /// put into relation proxy wrapped units with shared functions
   std::map<unsigned int, std::string> proxy_wrapped_units;

   /// For each functional unit (position in the vector), tech_constraints stores the maximum number of resources.
   std::vector<unsigned int> tech_constraints;

   /// for each operation (node-id, operation) return the set of functional unit that can be used
   CustomUnorderedMap<std::pair<unsigned int, std::string>, CustomOrderedSet<unsigned int>> node_id_to_fus;

   /// reverse map putting into relation functional units with the operations that can be mapped on
   CustomUnorderedMap<unsigned int, CustomOrderedSet<unsigned int>> fus_to_node_id;

   /// Puts into relation operation with type and functional units.
   /// A pair(node_id, operation_type-fu) means that the fu is used to implement the operation associated with the tree node node_id of type operation_type
   /// If the operation_type of the operation is different from the current one, the binding refers to an old version of the operation
   CustomUnorderedMap<unsigned int, std::pair<std::string, unsigned int>> binding;

   /// size of each memory unit in bytes
   std::map<unsigned int, unsigned int> memory_units_sizes;

   /// map between variables and associated memory_units
   std::map<unsigned int, unsigned int> vars_to_memory_units;

   /// store the pre-computed pipeline unit: given a functional unit it return the pipeline id compliant
   std::map<std::string, std::string> precomputed_pipeline_unit;

   /// store all cond_expr units having a Boolean condition
   CustomUnorderedSet<unsigned int> single_bool_test_cond_expr_units;

   /// in case of pointer plus expr between constants: no wire delay
   CustomUnorderedSet<unsigned int> simple_pointer_plus_expr;

   /// The roots used to compute a ssa
   mutable CustomMap<unsigned int, CustomSet<unsigned int>> ssa_roots;

   /// The bb_version-control root on which the roots of a ssa have been computed
   mutable CustomMap<unsigned int, std::pair<unsigned int, AbsControlStep>> ssa_bb_versions;

   /// The cond exprs driven by a ssa
   mutable CustomMap<unsigned int, CustomSet<unsigned int>> ssa_cond_exprs;

   /// The bb_version on which the cond exprs driven by a ssan have been computed
   mutable CustomMap<unsigned int, unsigned int> cond_expr_bb_versions;

   /// The operations reachable with delay zero
   mutable CustomMap<unsigned int, CustomSet<unsigned int>> zero_distance_ops;

   /// The bb_version on which the reachable delay zero operations
   mutable CustomMap<unsigned int, unsigned int> zero_distance_ops_bb_version;

   /// store mux timing for the current technology
   CustomMap<unsigned int, CustomUnorderedMapStable<unsigned int, double>> mux_timing_db;

   /// store mux timing for the current technology
   CustomMap<unsigned int, CustomUnorderedMapStable<unsigned int, double>> mux_area_db;

   /// store DSP x sizes
   std::vector<unsigned int> DSP_x_db;

   /// store DSP y sizes
   std::vector<unsigned int> DSP_y_db;

   /// put into relation variable and their latency when they are mapped on a private synchronous ram
   std::map<unsigned int, std::string> sync_ram_var_latency;

   std::string get_latency_string(const std::string& lat) const;

   /// return the execution time of the operation corrected by time_multiplier factor
   double time_m_execution_time(operation* op) const;

   /// return the stage time of the operation corrected by time_multiplier factor
   double time_m_stage_period(operation* op) const;

   double get_execution_time_dsp_modified(const unsigned int fu_name, const technology_nodeRef& node_op) const;

   double get_stage_period_dsp_modified(const unsigned int fu_name, const technology_nodeRef& node_op) const;

   /**
    * Returns the worst stage period for all the operations associated with the functional unit.
    * @param fu_name is the id of the functional unit.
    * @return the worst stage period for fu_name
    */
   double get_worst_stage_period(const unsigned int fu_name) const;

   /**
    * set the number of ports associated with the functional unit
    * @param fu_name is the functional unit id
    * @param n_ports is the number of ports
    */
   void set_number_channels(unsigned int fu_name, unsigned int n_ports);

   /**
    * Extract the node kind and precision, if available.
    * @param node is the vertex of the graph.
    * @param g is the graph used to retrieve the required values of the vertex
    * @param info is where the information retrieved for the node get stored.
    * @param constant_id is a tree node id (i.e., different from zero) when one of the operands is constant
    */
   void GetNodeTypePrec(const vertex node, const OpGraphConstRef g, node_kind_prec_infoRef info, HLS_manager::io_binding_type& constant_id, bool is_constrained) const;

   /**
    * Returns the technology_node associated with the given operation
    * @param fu_name is the string representing the name of the unit
    */
   static technology_nodeRef get_fu(const std::string& fu_name, const HLS_managerConstRef HLSMgr);

   /**
    * Return the connection delay due to phi
    * @param statement_index is the index of the operation producing the input of the phi
    * @return the connection delay
    */
   double GetPhiConnectionLatency(const unsigned int statement_index) const;

   /**
    * Return true if the two operations can be mapped on the same LUT
    * @param first_operation is the index of the first operation
    * @param second_operation is the index of the second operations
    * @return true if the two operations can be mapped on the same LUT
    */
   bool CanBeMerged(const unsigned int first_operation, const unsigned int second_operation) const;

   /**
    * Compute the roots to be considered for fan out computation
    * @param ssa is the ssa to be considered as starting point of the root conditions
    * @param cs is the control step where the operation using ssa must be scheduled
    * @return the set of ssas which are used in comparison which generate the input ssa
    */
   CustomSet<unsigned int> ComputeRoots(const unsigned int ssa, const AbsControlStep cs) const;

   /**
    * Compute the cond_expr indirectly driven by a ssa
    * @param ssa is the ssa to be considered
    * @return the set of cond_expr indirectly driven by ssa
    */
   CustomSet<unsigned int> ComputeDrivenCondExpr(const unsigned int ssa) const;

   /**
    * Compute the values for the initialization of the multiplxer characteristics database
    * @param allocation_information is a reference to an instance of this class
    * @return the pair mux_timing_db, mux_area_db
    */
   static const std::pair<const CustomMap<unsigned int, CustomUnorderedMapStable<unsigned int, double>>&, const CustomMap<unsigned int, CustomUnorderedMapStable<unsigned int, double>>&>
   InitializeMuxDB(const AllocationInformationConstRef allocation_information);

   /**
    * Compute the values for the initialization of the DSP characteristics database
    * @param allocation_information is a reference to an instance of this class
    */
   static const std::tuple<const std::vector<unsigned int>&, const std::vector<unsigned int>&> InitializeDSPDB(const AllocationInformationConstRef allocation_information);

   /**
    * Add the delay to reach a DSP register if necessary
    * @param statement_index is the index of the statement
    * @return the delay to reach a DSP register
    */
   double GetToDspRegisterDelay(const unsigned int statement_index) const;

   /**
    * Return the set of operations with zero time distance from source
    * @param statement_index is the operation to be considered
    * @return the set of operations at zero time distance
    */
   CustomSet<unsigned int> GetZeroDistanceOperations(const unsigned int statement_index) const;

 public:
   /**
    * Constructor
    * @param HLS_mgr is the HLS manager
    * NOTE: hls cannot be got from HLS_mgr since when allocation information constructor is called we are still in the hls constructor
    * @param function_id is the index of the function
    * @param parameters is the set of input parameters
    */
   AllocationInformation(const HLS_managerRef HLS_mgr, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    * Initialize all the data structure
    */
   void Initialize() override;

   /**
    * Clear all the data structure
    */
   void Clear() override;

   enum op_target
   {
      initiation_time,
      power_consumption,
      execution_time
   };
   //@}

   /**
    * Destructor
    */
   ~AllocationInformation() override;

   /**
    * Returns the set of functional units that can be used to implement the operation associated with vertex v.
    * @param v is the vertex.
    * @param g is the graph of the vertex v.
    */
   const CustomOrderedSet<unsigned int>& can_implement_set(const vertex v) const;
   const CustomOrderedSet<unsigned int>& can_implement_set(const unsigned int v) const;

   /**
    * Return if any functional unit has been allocated for an operation
    * @return true if at least one functional unit exists
    */
   bool CanImplementSetNotEmpty(const unsigned int v) const;

   /**
    * Returns the name of the functional unit, associated with the name of the library, given the corresponding identifier
    */
   std::pair<std::string, std::string> get_fu_name(unsigned int id) const;

   /**
    * Returns the number of functional unit given its id.
    * @param fu_name is the id of the functional unit.
    * @return the number of functional unit for fu_name
    */
   unsigned int get_number_fu(unsigned int fu_name) const;

   /**
    * Returns the number of functional units types.
    * @return the number of functional units types.
    */
   unsigned int get_number_fu_types() const;

   /**
    * Returns the execution time for a given vertex and a given functional unit.
    * @param fu_name is the id of the functional unit.
    * @param v is the vertex for which the test is performed.
    * @param g is the graph of the vertex v.
    * @return the execution time for (fu_name, v, g)
    */
   double get_execution_time(const unsigned int fu_name, const vertex v, const OpGraphConstRef g) const;
   double get_execution_time(const unsigned int fu_name, const unsigned int v) const;

   /**
    * Calculates the control steps required for a specific operation.
    * @param et is the execution time of the operation.
    * @param clock_period is the clock period of the system.
    * @return the number of control steps required.
    */
   ControlStep op_et_to_cycles(double et, double clock_period) const;

   /**
    * Computes the maximum number of resources implementing
    * the operation associated with the vertex v.
    * @param v is the vertex for which the test is performed.
    * @param g is the graph of the vertex v.
    * @return the maximum number of resources implementing the operation associated with the vertex.
    */
   unsigned int max_number_of_resources(const vertex v) const;

   /**
    * return the maximum number of operations that insists on the same type of functional unit
    * @param fu is the functional unit id
    * @return the number of operations that can be implemented by functional unit fu
    */
   unsigned int max_number_of_operations(unsigned int fu) const;

   /**
    * This method returns the min or the max value of the execution
    * time/power consumption/initiation time for a vertex.
    * @param v is the vertex for which the analysis is performed.
    * @param g is the graph of the vertex v.
    * @param allocation_min_max is an enum that indicates if the desired value is
    *        the minimum or the maximum among the fu that can handle
    *        the execution of the vertex v.
    * @param target is an enum that indicates "initiation time" if the needed
    *        value is the initiation time, "power consumption" if the needed
    *        value is the power consumption, "execution time" if the
    *        needed value is the execution time.
    * @param fu_name is the id of the functional unit associated with
    *        vertex v and with the searched attribute.
    * @param flag is true when there exist a functional_unit for
    *        vertex v.
    * @param CF is the functor used to retrive information
    * @return the max or min value.
    */
   double get_attribute_of_fu_per_op(const vertex v, const OpGraphConstRef g, const Allocation_MinMax allocation_min_max, op_target target, unsigned int& fu_name, bool& flag, const updatecopy_HLS_constraints_functor* CF = nullptr) const;

   /**
    * This method returns the min or the max value of the execution
    * time/power consumption/initiation time for a vertex.
    * @param v is the vertex for which the analysis is performed.
    * @param g is the graph of the vertex v.
    * @param allocation_min_max an enum that indicates if the wanted value is
    *        the minimum or the maximum among the fu that can handle
    *        the execution of the vertex v.
    * @param target is an enum that indicates "initiation time" if the needed
    *        value is the initiation time, "power consumption" if the needed
    *        value is the power consumption, "execution time" if the
    *        needed value is the execution time.
    * @return the max or min value.
    */
   double get_attribute_of_fu_per_op(const vertex v, const OpGraphConstRef g, const Allocation_MinMax allocation_min_max, op_target target) const;

   /**
    * Computes the minum number of resources implementing
    * the operation associated with the vertex v.
    * @param v is the vertex for which the test is performed.
    * @param g is the graph of the vertex v.
    * @return the minum number of resources implementing the operation associated with the vertex.
    */
   unsigned int min_number_of_resources(const vertex v) const;

   /**
    * @return the setup/hold time given the current technology
    */
   double get_setup_hold_time() const;

   /**
    * return true in case fu type is a resource unit performing an indirect access to memory
    * @param fu_type is functional unit id
    * @return true in case the functional unit access memories through the bus
    */
   bool is_indirect_access_memory_unit(unsigned int fu) const;

   /**
    * Returns the worst execution time for all the operations associated with the functional unit.
    * @param fu_name is the id of the functional unit.
    * @return the worst execution time for fu_name
    */
   double get_worst_execution_time(const unsigned int fu_name) const;

   /**
    * Returns the area for a given functional unit.
    * @param fu_name is the id of the functional unit.
    * @return the area for the functional unit.
    */
   double get_area(const unsigned int fu_name) const;

   /**
    * Returns the area for a given statement
    * @param statement_index is the index of the statement
    * @return the area
    */
   double GetStatementArea(const unsigned int statement_index) const;

   /**
    * return the precision of the given functional unit
    * @param fu_name is the id of the functional unit.
    * @return the precision associated with the functional unit
    */
   unsigned int get_prec(const unsigned int fu_name) const;

   /**
    * return an estimation of the number of DSPs used by the unit
    * @param fu_name is the id of the functional unit.
    */
   double get_DSPs(const unsigned int fu_name) const;

   /**
    * Returns the initiation time for a given vertex and a given functional unit.
    * @param fu_name is the id of the functional unit.
    * @param v is the vertex for which the test is performed.
    * @return the initiation_time for (fu_name, v, g).
    */
   ControlStep get_initiation_time(const unsigned int fu_name, const vertex v) const;

   /**
    * Returns the initiation time for a given operation and a given functional unit.
    * @param fu_name is the id of the functional unit.
    * @param statement_index is the operation
    * @return the initiation_time for (fu_name, v, g).
    */
   ControlStep get_initiation_time(const unsigned int fu_name, const unsigned int statement_index) const;

   /**
    * Checks if the given operation has a bounded execution time or not
    * @param g is the graph of the vertex
    * @param op is the vertex representing the operation
    * @param fu_type is the identifier of the function unit
    * @return true if the operation op has a bounded execution time when executed by the unit fu_type
    */
   bool is_operation_bounded(const OpGraphConstRef g, const vertex& op, unsigned int fu_type) const;

   /**
    * Checks if the given operation has a bounded execution time or not
    * @param index is the index of the statement
    * @param fu_type is the identifier of the function unit
    * @return true if the operation op has a bounded execution time when executed by the unit fu_type
    */
   bool is_operation_bounded(const unsigned int index, unsigned int fu_type) const;

   /**
    * Checks if the given operation has a bounded execution time or not
    * if the operation is assigned to a functional unit, check if execution time on it is bounded or not
    * if the operation is not assigned to a functional unit, return true for small operations (i.e., Boolean expression)
    */
   bool is_operation_bounded(const unsigned int index) const;

   /**
    * Checks if the given operation has its primary input registered or not
    * @param g is the graph of the vertex
    * @param op is the vertex representing the operation
    * @param fu_type is the identifier of the function unit
    * @return true if the operation op has its primary input registered or not
    */
   bool is_operation_PI_registered(const OpGraphConstRef g, const vertex& op, unsigned int fu_type) const;

   /**
    * Checks if the given operation has its primary input registered or not
    * @param index is the index of the statement
    * @param fu_type is the identifier of the function unit
    * @return true if the operation op has its primary input registered or not
    */
   bool is_operation_PI_registered(const unsigned int index, unsigned int fu_type) const;

   /**
    * Checks if the given operation has its primary input registered or not
    * @param index is the index of the statement
    * @return true if the operation op has its primary input registered or not
    */
   bool is_operation_PI_registered(const unsigned int index) const;

   /**
    * return true in case fu type is a memory unit with direct access channels
    * @param fu_type is functional unit id
    * @return true in case the functional unit has direct access channels
    */
   bool is_direct_access_memory_unit(unsigned int fu_type) const;

   /**
    * return true in case4 fu type is a memory unit accessed through a proxy module
    * @param fu_type is functional unit id
    * @return true in case the functiona unit has direct access through a proxy module
    */
   bool is_direct_proxy_memory_unit(unsigned int fu_type) const;

   /**
    * Checks if the functional unit implements a READ_COND operation.
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit can implement a READ_COND operation, false otherwise.
    */
   bool is_read_cond(const unsigned int fu_name) const;

   /**
    * Checks if the functional unit implements an ASSIGN operation.
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit can implement a ASSIGN operation, false otherwise.
    */
   bool is_assign(const unsigned int fu_name) const;

   /**
    * Checks if the functional unit implements a RETURN operation.
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit can implement a RETURN operation, false otherwise.
    */
   bool is_return(const unsigned int fu_name) const;

   /**
    * Returns true if the fu_name is a memory unit
    * @param fu_name is the id of the functional unit
    * @return true when the functional unit is a memory unit
    */
   bool is_memory_unit(const unsigned int fu_name) const;

   /**
    * check if a functional unit is a proxy
    * @param fu_name is the id of the functional unit
    * @return true when fu_name is a proxy unit
    */
   bool is_proxy_unit(const unsigned int fu_name) const;

   /**
    * check if a functional unit is a proxy function unit
    * @param fu_name is the id of the functional unit
    * @return true when fu_name is a proxy unit
    */
   bool is_proxy_function_unit(const unsigned int fu_name) const;

   /**
    * check if a functional unit is a proxy wrapped unit
    * @param fu_name is the id of the functional unit
    * @return true when fu_name is a proxy unit
    */
   bool is_proxy_wrapped_unit(const unsigned int fu_name) const;

   /**
    * Returns the base address of the functional unit
    * @param fu_name is the id of the functional unit
    * @return the base address of the functional unit
    */
   unsigned int get_memory_var(const unsigned int fu_name) const;

   /**
    * return the set of proxy memory units
    * @return the set of proxy memory units
    */
   const std::map<unsigned int, unsigned int>& get_proxy_memory_units() const;

   /**
    * return the var associated with the proxy unit
    * @param fu_name is functional unit
    * @return the var assiciated with fu_name
    */
   unsigned int get_proxy_memory_var(const unsigned int fu_name) const;

   /**
    * return the set of proxy function units
    * @return the set of proxy function units
    */
   const std::map<unsigned int, std::string>& get_proxy_function_units() const;

   /**
    * return the set of proxy wrapped units
    * @return the set of proxy wrapped units
    */
   const std::map<unsigned int, std::string>& get_proxy_wrapped_units() const;

   /**
    * Return the stage period for a given vertex and a given pipelined functional unit
    * @param fu_name is the id of the functional unit.
    * @param v is the vertex for which the test is performed.
    * @param g is the graph of the vertex v.
    * @return the stage period for (fu_name, v, g)
    */
   double get_stage_period(const unsigned int fu_name, const vertex v, const OpGraphConstRef g) const;
   double get_stage_period(const unsigned int fu_name, const unsigned int v) const;

   /**
    * return a time to be subtracted to the execution time/stage period
    * @param fu is the functional unit
    * @param operation_name is the operation name
    * @return a correction time
    */
   double get_correction_time(unsigned int fu, const std::string& operation_name, unsigned int n_ins) const;

   /**
    * estimate the delay of a mux that can be uses to mux the input of the given functional unit
    * @param fu_name is the functional unit id
    * @return the mux delay
    */
   double estimate_mux_time(unsigned int fu_name) const;

   /**
    * Return the delay due to mux with n-inputs
    * @param fu_prec is the bitsize of the data
    * @param mux_ins is the number of mux inputs
    */
   double estimate_muxNto1_delay(unsigned int fu_prec, unsigned int mux_ins) const;

   /**
    * Return the area due to mux with n-inputs
    * @param fu_prec is the bitsize of the data
    * @param mux_ins is the number of mux inputs
    */
   double estimate_muxNto1_area(unsigned int fu_prec, unsigned int mux_ins) const;

   double mux_time_unit(unsigned int fu_prec) const;
   double mux_time_unit_raw(unsigned int fu_prec) const;

   /**
    * @brief Return the number of cycles for given vertex and a given functional unit
    * @param fu_name is the id of the functional unit.
    * @param v is the vertex for which the test is performed.
    * @param g is the graph of the vertex v.
    * @return the number of cycles
    */
   unsigned int get_cycles(const unsigned int fu_name, const unsigned int v) const;
   unsigned int get_cycles(const unsigned int fu_name, const vertex v, const OpGraphConstRef g) const;

   /**
    * In case the vertex is bounded to a particular functional unit, it returns true and the functional unit.
    * @param v is the vertex.
    * @param fu_name is the eventually bounded functional unit.
    */
   bool is_vertex_bounded_with(const unsigned int v, unsigned int& fu_name) const;
   bool is_vertex_bounded_with(const vertex v, unsigned int& fu_name) const;

   /**
    * Checks if the functional unit is uniquely bounded to a vertex
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit is uniquely bounded to a vertex, false otherwise.
    */
   bool is_vertex_bounded(const unsigned int fu_name) const;

   /**
    * Returns the reference to a functional unit given its id.
    * @param fu_name is the id of the functional unit.
    * @return the reference to a functional unit
    */
   technology_nodeRef get_fu(unsigned int fu_name) const;

   /**
    * return the number of channels available for the functional unit
    * @param fu_name is the functional unit id
    * @return the number of channels
    */
   unsigned int get_number_channels(unsigned int fu_name) const;

   /**
    * estimate the delay of the controller in straightforward mode
    * @return the controller delay
    */
   double EstimateControllerDelay() const;

   /**
    * estimate the delay of the controller in feedback mode
    * @return the controller delay
    */
   double estimate_controller_delay_fb() const;

   /**
    * Returns the name of the functional for debug purpose. Do not use this string as key id.
    * @param fu_name is the id of the functional unit.
    * @return the string describing the functional unit.
    */
   std::string get_string_name(unsigned int fu_name) const;

   /**
    * compute a number representing the "weight" of the functional unit
    * @param fu_s1 is the functional unit id
    * @return normalized area of fu_s1
    */
   double compute_normalized_area(unsigned int fu_s1) const;

   /**
    * Returns the memory units
    */
   std::map<unsigned int, unsigned int> get_memory_units() const;

   /**
    * Checks if the functional unit has to be synthetized
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit is artificial, false otherwise.
    */
   bool has_to_be_synthetized(const unsigned int fu_name) const;

   /**
    *
    * @return an estimation of the delay of a call_expr
    */
   double estimate_call_delay() const;

   /**
    * Checks if the functional unit has to be shared
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit has to be shared, false otherwise.
    */
   bool has_to_be_shared(const unsigned int fu_name) const;

   /**
    * Checks library type and sets if it's ordered, complete or simple
    */
   void check_library();
   //@}

   /**
    * return true in case the functional unit is a direct_access memory unit with a single clock latency for LOAD
    * @param fu_type is functional unit id
    * @return true in case fu_type is a distributed ram, false otherwise
    */
   bool is_one_cycle_direct_access_memory_unit(unsigned int fu_type) const;

#ifndef NDEBUG
   /**
    * Prints the actual allocation.
    */
   void print_allocated_resources() const;
   //@}
#endif

   static std::string extract_bambu_provided_name(unsigned int prec_in, unsigned int prec_out, const HLS_managerConstRef HLSMgr, technology_nodeRef& current_fu);

   void print(std::ostream& os) const;

   /**
    * Checks if the functional unit is actually an artificial functional unit like: NOP, ENTRY and EXIT
    * @param fu_name is the id of the functional unit.
    * @return true when the functional unit is artificial, false otherwise.
    */
   bool is_artificial_fu(const unsigned int fu_name) const;

   /**
    * Checks if the operation associated with a vertex can be implemented by a given functional unit.
    * @param fu_name is the id of the functional unit.
    * @param v is the vertex for which the test is performed.
    * @return true when the functional unit can implement the operation associated with the vertex, false otherwise.
    * @return the minimum slack of the component estimated by scheduling
    */
   bool can_implement(const unsigned int fu_id, const vertex v) const;

   /**
    * Return the execution time of (a stage of) an operation
    * @param operation is the operation
    * @param functional_unit is the functional unit
    * @param stage is the stage to be considered for pipelined operation
    * @param ignore_connection specifies if connection delays have to be ignored
    */
   std::pair<double, double> GetTimeLatency(const unsigned int operation, const unsigned int functional_unit, const unsigned int stage = 0) const;
   std::pair<double, double> GetTimeLatency(const vertex operation, const unsigned int functional_unit, const unsigned int stage = 0) const;

   /**
    * Return the connection time for a couple of operations or the phi overhead for a single operation
    * @param first_operation is the first operation
    * @param second_operation is the second operation; if it is null, the phi contribution is returned
    * @param cs is the control step in which second operation would be scheduled
    * @param fu_type is the functional unit type to which the second operation would be assigned
    * @return the connection time
    */
   double GetConnectionTime(const unsigned int first_operation, const unsigned int second_operation, const AbsControlStep cs) const;
   double GetConnectionTime(const vertex first_operation, const vertex second_operation, const AbsControlStep cs) const;

   /**
    * Return true if the variable execution time depends on the scheduling
    * @param operation_index is the index of the operation
    * @return true if the variable execution time depends on the scheduling
    */
   bool IsVariableExecutionTime(const unsigned int operation_index) const;

   /**
    * @return the minimum slack of the component estimated by scheduling
    */
   double getMinimumSlack() const
   {
      return minimumSlack;
   }

   /**
    * set the minimum slack  of the component estimated by scheduling
    * @param slack is the timing slack
    */
   void setMinimumSlack(const double slack)
   {
      minimumSlack = slack;
   }

   /**
    * Return the execution time of a cond expr corresponding to the gimple_phi
    * @param operation_index is the index of the gimple_phi
    * @return the execution time
    */
   double GetCondExprTimeLatency(const unsigned int operation_index) const;

   /**
    * Return the latency of an operation in cycle
    * @param operation is the operation
    * @return the number of cycles required to execute the operation
    */
   unsigned int GetCycleLatency(const unsigned int operationID) const;
   unsigned int GetCycleLatency(const vertex operationID) const;

   /**
    * Return the functional unit type used to execute an operation
    * @param operation is the operation
    * @return the id of the functional unit type
    */
   unsigned int GetFuType(const unsigned int operation) const;
   unsigned int GetFuType(const vertex operation) const;

   unsigned int get_n_complex_operations() const;

   /**
    * return an estimation of the interconnection time
    * @return an estimation of the interconnection time based on setup/hold time
    */
   double get_connection_time(unsigned fu_type, bool add_delay1, bool add_delay2, size_t n_complex_ops, size_t n_mem_ops) const;

   double mux_area_unit_raw(unsigned int fu_prec) const;

   /**
    * Estimate the area of a mux attached to a given functional unit
    * @param fu_name is the functional unit id
    * @return the area of a mux connected with fu_name
    */
   double estimate_mux_area(unsigned int fu_name) const;

   unsigned int get_worst_number_of_cycles(const unsigned int fu_name) const;

   /**
    * return true in case the functional unit has an input connected to a constant value
    * @param fu_name is the id of the functional unit
    * @return true in case fu_name has a constant in its inputs.
    */
   bool has_constant_in(unsigned int fu_name) const;

   /**
    * Returns true if the fu_name is a proxy memory unit
    * @param fu_name is the id of the functional unit
    * @return true when the functional unit is a proxy memory unit
    */
   bool is_proxy_memory_unit(const unsigned int fu_name) const;

   /**
    * return true in case the functional unit is a read-only memory unit
    * @param fu_name is the functional unit
    * @return true when fu_name is a read-only memory unit
    */
   bool is_readonly_memory_unit(const unsigned int fu_name) const;

   /**
    * Return the margin to be considered when performing scheduling
    */
   double GetClockPeriodMargin() const;

   /**
    * return true in case the functional unit implement a cond_expr operation and the condition is a Boolean one
    * @param fu_name is the functional unit
    * @return true in case the functional unit is a mux
    */
   bool is_single_bool_test_cond_expr_units(const unsigned int fu_name) const;

   /**
    * return true in case the functional unit implement a pointer_plus operation and the operands are constants
    * @param fu_name is the functional unit
    * @return true in case the functional unit does a sum of two constants
    */
   bool is_simple_pointer_plus_expr(const unsigned int fu_name) const;

   static bool can_be_asynchronous_ram(tree_managerConstRef TM, unsigned int var, unsigned int threshold, bool is_read_only_variable);

   /**
    * Check if two statements can be chained
    * @param first_statement_index is the index of the first statement
    * @param second_statement_index is the index of the second statement
    * @return true if they can be chained
    */
   bool CanBeChained(const unsigned int first_statement_index, const unsigned int second_statement_index) const;

   /**
    * Check if two statements can be chained
    * @param first_statement is the first vertex
    * @param second_statement is the second vertex
    * @return true if they can be chained
    */
   bool CanBeChained(const vertex first_statement, const vertex second_statement) const;
};
typedef refcount<AllocationInformation> AllocationInformationRef;
typedef refcount<const AllocationInformation> AllocationInformationConstRef;

/**
 * @struct updatecopy_HLS_constraints_functor
 * constraint functor used by get_attribute_of_fu_per_op
 */
struct updatecopy_HLS_constraints_functor
{
   /**
    * Required functor function used to compute the number of resources associated with the given resource
    * @param name is the name of the resource
    * @return the number of resources of name
    */
   unsigned int operator()(const unsigned int name) const;

   /**
    * Function used to update the copy of the technology constraints
    * @param name is the id of the resource
    * @param the number of delta resources
    */
   void update(const unsigned int name, int delta);

   /**
    * Constructor
    * @param ALL is the reference to allocation class where technology constraints are stored
    */
   explicit updatecopy_HLS_constraints_functor(const AllocationInformationRef allocation_information);

   /**
    * Destructor
    */
   ~updatecopy_HLS_constraints_functor() = default;

 private:
   /// copy of the technology constraints
   std::vector<unsigned int> tech;
};

/**
 * @class node_kind_prec_info
 * @ingroup allocation
 *
 * This structure collect the information of input and output
 * precision of nodes and the node kind.
 */
struct node_kind_prec_info
{
   ///  Node kind.
   std::string node_kind;
   ///  Vector of input precision.
   std::vector<unsigned int> input_prec;
   /// vector storing the number of elements in case the input is a vector, 0 otherwise (used for mapping with library fus - it can be different from the real one)
   std::vector<unsigned int> base128_input_nelem;

   /// vector storing the number of elements in case the input is a vector, 0 otherwise (real value)
   std::vector<unsigned int> real_input_nelem;

   ///  Precision of the output.
   unsigned int output_prec;

   /// number of output elemnts in case the output is a a vector, 0 otherwise (used for mapping with library fus - it can be different from the real one)
   unsigned int base128_output_nelem;

   /// number of output elemnts in case the output is a a vector, 0 otherwise (real_value)
   unsigned int real_output_nelem;

   /// true when the functional unit is a cond_expr and has the first operand of type bool
   bool is_single_bool_test_cond_expr;

   /// true when the functional unit is a plointer plus expr with two constant operands
   bool is_simple_pointer_plus_expr;

   node_kind_prec_info() : output_prec(0), base128_output_nelem(0), real_output_nelem(0), is_single_bool_test_cond_expr(false), is_simple_pointer_plus_expr(false)
   {
   }
};
#endif
