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
 * @file testbench_generation_base_step.hpp
 * @brief Class to compute testbenches for high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#ifndef HLS_TESTBENCH_HPP
#define HLS_TESTBENCH_HPP

#include "application_manager.hpp"
#include "refcount.hpp"

/// Superclass include
#include "hls_step.hpp"

/**
 * @name forward declarations
 */
//@{
class module;
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(HLS_constraints);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(memory);
REF_FORWARD_DECL(language_writer);
//@}

/// STD include
#include <string>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <tuple>
#include <vector>

/**
 * TestbenchGenerationBaseStep is a Facade class that hide implementation details of the
 * testbench creation.
 */
class TestbenchGenerationBaseStep : public HLS_step
{
 protected:
   const language_writerRef writer;

   structural_objectRef cir;

   const module* mod;

   double target_period;

   /// output directory
   const std::string output_directory;

   const std::string c_testbench_basename;

   /// testbench basename
   std::string hdl_testbench_basename;

   /// true if the c testbench is generated to wrap c++
   bool flag_cpp;

   /**
    * Creates the HDL testbench file associated with the given component
    */
   std::string create_HDL_testbench(bool xilinx_isim) const;

   /**
    * Write the hdl testbench.
    *
    * This function takes care of generating the hdl testbench top component.
    *
    * @param hdl_file Output stream of the source file of the generated testbench.
    * @param writer Language writer.
    * @param simulation_values_path Path of the values file.
    * @param generate_vcd_output Enable/Disable vcd generation.
    * @param xilinx_isim Xilinx isim?
    * @param TreeM Tree Manager of the design under test.
    */
   void write_hdl_testbench(std::string simulation_values_path, bool generate_vcd_output, bool xilinx_isim, const tree_managerConstRef TreeM) const;

   void write_hdl_testbench_prolog() const;

   void write_module_begin() const;

   void write_compute_ulps_functions() const;

   void write_auxiliary_signal_declaration() const;

   void write_module_instantiation(bool xilinx_isim) const;

   void begin_initial_block() const;

   void open_result_file(const std::string& result_file) const;

   void initialize_auxiliary_variables() const;

   void initialize_input_signals(const tree_managerConstRef TreeM) const;

   void open_value_file(const std::string& input_values_filename) const;

   void memory_initialization() const;

   void write_output_checks(const tree_managerConstRef TreeM) const;

   void end_initial_block() const;

   void begin_file_reading_operation() const;

   void reading_base_memory_address_from_file() const;

   void memory_initialization_from_file() const;

   void read_input_value_from_file(const std::string& input_name, bool& first_valid_input) const;

   void end_file_reading_operation() const;

   void write_max_simulation_time_control() const;

   void write_clock_process() const;

   void write_module_end() const;

   void testbench_controller_machine() const;

   void write_sim_time_calc() const;

   void write_underlying_testbench(std::string simulation_values_path, bool generate_vcd_output, bool xilinx_isim, const tree_managerConstRef TreeM) const;

   void write_initial_block(const std::string& simulation_values_path, bool withMemory, const tree_managerConstRef TreeM, bool generate_vcd_output) const;

   /**
    * Generates and execute the Verilator testbench file associated with the given component
    */
   std::string verilator_testbench() const;

   /**
    * Write the verilator testbench.
    *
    * @param input_file Filename of the stimuli file.
    */
   std::string write_verilator_testbench(const std::string& input_file) const;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   virtual void write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool& hasMultiIrq) const = 0;

   virtual void write_slave_initializations(bool withMemory) const = 0;

   virtual void write_memory_handler() const = 0;

   virtual void write_interface_handler() const = 0;

   virtual void write_call(bool hasMultiIrq) const = 0;

   virtual void write_file_reading_operations() const = 0;

   virtual void init_extra_signals(bool withMemory) const;

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Constructor.
    *
    * Declared protected to prevent direct instantiation. Use
    * Create() factory methods instead.
    */
   TestbenchGenerationBaseStep(const ParameterConstRef Param, const HLS_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type, std::string c_testbench_basename = "values");

 public:
   /**
    * Destructor.
    */
   ~TestbenchGenerationBaseStep() override;

   static std::string print_var_init(const tree_managerConstRef TreeM, unsigned int var, const memoryRef mem);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
