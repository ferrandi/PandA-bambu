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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file parser_flow_step_factory.hpp
 * @brief Factory for parser flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef PARSER_FLOW_STEP_FACTORY_HPP
#define PARSER_FLOW_STEP_FACTORY_HPP
#include "design_flow_step_factory.hpp"
#include "parser_flow_step.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(DesignFlowStep);

class ParserFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The application manager
   const application_managerRef AppM;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param AppM is the application manager
    * @param parameters is the set of input parameters
    */
   ParserFlowStepFactory(const DesignFlowManagerConstRef design_flow_manager, const application_managerRef AppM,
                         const ParameterConstRef parameters);

   ~ParserFlowStepFactory() override;

   /**
    * @brief Create a parset step of given type
    *
    * @param parser_type Parser step type
    * @param file_name Filename associated to the parser
    * @return DesignFlowStepRef
    */
   DesignFlowStepRef CreateParserStep(ParserFlowStep_Type parser_type, const std::string& file_name) const;
};
#endif
