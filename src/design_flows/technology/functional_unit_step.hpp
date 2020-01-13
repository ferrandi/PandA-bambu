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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file functional_unit_step.hpp
 * @brief Abstract class to iterate over all the cells of a template
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef FUNCTIONAL_UNIT_STEP_HPP
#define FUNCTIONAL_UNIT_STEP_HPP

/// Superclass include
#include "design_flow_step.hpp"

/// utility include
#include "custom_map.hpp"
#include "refcount.hpp"

class functional_unit;
REF_FORWARD_DECL(library_manager);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(target_manager);
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

   /// target information
   const target_managerRef target;

   /// The id of the first analyzed cell of a sequence of cells which differ for the position of the constant
   unsigned int has_first_synthesis_id;

   /// The sizes of available DSPs
   CustomMap<unsigned int, unsigned int> DSP_y_to_DSP_x;

   /**
    * Analyze the single cell
    * @param fu is the cell
    * @param prec is the precision
    * @param portsize_parameters is the size of parameters
    * @param portsize_index
    * @param pipe_parameters
    * @param stage_index
    * @param constPort is the index of the constant port
    */
   virtual void AnalyzeCell(functional_unit* fu, const unsigned int prec, const std::vector<std::string>& portsize_parameters, const size_t portsize_index, const std::vector<std::string>& pipe_parameters, const size_t stage_index,
                            const unsigned int constPort, const bool is_commutative, size_t max_lut_size) = 0;

   /**
    * Analyze all the cells built starting from a template
    * @param fu is the corresponding functional unit
    */
   virtual void AnalyzeFu(const technology_nodeRef fu);

   /**
    * Create a template instance to be specialized
    */
   technology_nodeRef create_template_instance(const technology_nodeRef& fu_template, std::string& name, const target_deviceRef& device, unsigned int prec);

 public:
   /**
    * Constructor.
    * @param target is the target
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   FunctionalUnitStep(const target_managerRef target, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FunctionalUnitStep() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
