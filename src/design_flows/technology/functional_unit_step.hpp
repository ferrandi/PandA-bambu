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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file functional_unit_step.hpp
 * @brief Abstract class to iterate over all the cells of a template
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef FUNCTIONAL_UNIT_STEP_HPP
#define FUNCTIONAL_UNIT_STEP_HPP
#include "custom_map.hpp"
#include "design_flow_step.hpp"
#include "refcount.hpp"

class functional_unit;
REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);

/**
 * Step which loads device dependent technology information
 */
class FunctionalUnitStep : public virtual DesignFlowStep
{
 protected:
   /// Technology manager
   const technology_managerRef TM;

   /// device information
   const generic_deviceRef device;

   /// The id of the first analyzed cell of a sequence of cells which differ for the position of the constant
   unsigned int has_first_synthesis_id;

   /// The sizes of available DSPs
   CustomMap<unsigned int, unsigned int> DSP_y_to_DSP_x;

   /**
    * Analyze the single cell
    * @param fu is the cell
    * @param prec is the required precision
    * @param portsize_parameters lists the port size parameter strings
    * @param portsize_index selects one of the `portsize_parameters`
    * @param pipe_parameters lists the pipeline parameter strings
    * @param stage_index selects the current pipeline stage inside the cell
    * @param constPort is the index of the constant port
    * @param is_commutative tells if the operation is commutative
    * @param max_lut_size upper bound for LUT conversion
    */
   virtual void AnalyzeCell(functional_unit* fu, const unsigned int prec,
                            const std::vector<std::string>& portsize_parameters, const size_t portsize_index,
                            const std::vector<std::string>& pipe_parameters, const size_t stage_index,
                            const unsigned int constPort, const bool is_commutative, size_t max_lut_size) = 0;

   /**
    * Analyze all the cells built starting from a template
    * @param f_unit is the corresponding functional unit template being processed
    */
   virtual void AnalyzeFu(const technology_nodeRef f_unit);

   /**
    * Create a template instance to be specialized
    */
   technology_nodeRef create_template_instance(const technology_nodeRef& fu_template, const std::string& name,
                                               unsigned int prec);

 public:
   /**
    * Constructor.
    * @param _device is the device
    */
   FunctionalUnitStep(const generic_deviceRef _device);

   virtual ~FunctionalUnitStep() override = default;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
