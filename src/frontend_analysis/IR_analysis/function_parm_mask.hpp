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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
 * @file function_parm_mask.hpp
 * @brief Restructure the top level function to adhere the specified interface.
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 *
 */
#ifndef FUNCTION_PARM_MASK_HPP
#define FUNCTION_PARM_MASK_HPP

/// Superclass include
#include "Range.hpp"
#include "application_frontend_flow_step.hpp"
#include "bit_lattice.hpp"
#include "tree_node.hpp"
#include "xml_attribute.hpp"

class function_parm_mask : public ApplicationFrontendFlowStep
{
 private:
   static bool executed;
   static bit_lattice dc;

   struct funcMask
   {
      bit_lattice sign;
      int16_t exp_l;
      int16_t exp_u;
      uint8_t m_bits;
   };

   std::pair<std::string, RangeRef> tagDecode(const attribute_sequence::attribute_list& attributes, Range::bw_t bw) const;

   bool fullFunctionMask(function_decl* fd, const function_parm_mask::funcMask& fm) const;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   function_parm_mask(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~function_parm_mask() override;

   DesignFlowStep_Status Exec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};

#endif
