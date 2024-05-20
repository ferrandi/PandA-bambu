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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file RTL_characterization.hpp
 * @brief Class for performing RTL characterization
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef RTL_CHARACTERIZATION_HPP
#define RTL_CHARACTERIZATION_HPP
#include "functional_unit_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"
#include <vector>

/**
 * @name forward declarations
 */
//@{
// parameters
REF_FORWARD_DECL(area_info);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(time_info);
class xml_element;
class module;
//@}

class RTLCharacterization : public FunctionalUnitStep
{
 private:
   /// Library manager
   library_managerRef LM;

   /// The component to be characterized
   const std::string component;

   /// The cells to be characterized
   const CustomSet<std::string> cells;

   /// The area model of the last characterization
   area_infoRef prev_area_characterization;

   /// The time model of the last characterization
   time_infoRef prev_timing_characterization;

#ifndef NDEBUG
   /// True if we are performing dummy synthesis
   const bool dummy_synthesis;
#endif

   /**
    * Characterize the given functional unit with respect to the target device
    */
   void characterize_fu(const technology_nodeRef functional_unit);

   /**
    * @brief resize the port w.r.t a given precision
    * @param port
    */
   void resize_port(const structural_objectRef& port, unsigned int prec);
   /**
    * Performing the specialization of the given object
    */
   void specialize_fu(const module* mod, unsigned int prec, unsigned int bus_data_bitsize,
                      unsigned int bus_addr_bitsize, unsigned int bus_size_bitsize, unsigned int bus_tag_bitsize,
                      size_t portsize_value);

   /**
    * Generate the output file
    */
   void xwrite_device_file();

   /**
    * Add the characterization to the output file
    */
   void xwrite_characterization(xml_element* nodeRoot);

   /// set of units completed with success
   CustomOrderedSet<std::string> completed;

   /**
    * Fix the execution time by removing set/hold/pad timings
    */
   void fix_execution_time_std();

   /**
    * Fix execution/stage period value for proxies and bounded memory controllers
    */
   void fix_proxies_execution_time_std();

   /**
    * fix the estimation of mux timing
    */
   void fix_muxes();

   void add_input_register(structural_objectRef port_in, const std::string& register_library,
                           const std::string& port_prefix, structural_objectRef reset_port,
                           structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef e_port,
                           structural_managerRef SM);

   void add_output_register(structural_managerRef SM, structural_objectRef e_port, structural_objectRef circuit,
                            structural_objectRef reset_port, structural_objectRef port_out,
                            const std::string& port_prefix, structural_objectRef clock_port,
                            const std::string& register_library);

   /**
    * Extract the component name from list of cells
    * @param input is the input string
    */
   std::string ComputeComponent(const std::string& input) const;

   /**
    * Extract the cell lists
    * @param input is the input string
    */
   CustomSet<std::string> ComputeCells(const std::string& input) const;

   void AnalyzeCell(functional_unit* fu, const unsigned int prec, const std::vector<std::string>& portsize_parameters,
                    const size_t portsize_index, const std::vector<std::string>& pipe_parameters,
                    const size_t stage_index, const unsigned int constPort, const bool is_commutative,
                    size_t max_lut_size) override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param _device is the device
    * @param _cells is the component to be characterized
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   RTLCharacterization(const generic_deviceRef _device, const std::string& _cells,
                       const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   ~RTLCharacterization() override;

   std::string GetName() const override;

   bool HasToBeExecuted() const override;

   void Initialize() override;

   DesignFlowStep_Status Exec() override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;
};
#endif
