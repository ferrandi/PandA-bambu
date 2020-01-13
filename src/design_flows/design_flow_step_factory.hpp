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
 * @file design_flow_step_factory.hpp
 * @brief Pure virtual base class for all the design flow step factory
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DESIGN_FLOW_STEP_FACTORY_HPP
#define DESIGN_FLOW_STEP_FACTORY_HPP

#include "refcount.hpp"
#include <string> // for string

CONSTREF_FORWARD_DECL(DesignFlowManager);
REF_FORWARD_DECL(DesignFlowStep);
CONSTREF_FORWARD_DECL(Parameter);

class DesignFlowStepFactory
{
 protected:
   /// The design flow manager
   const Wrefcount<const DesignFlowManager> design_flow_manager;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowStepFactory(const DesignFlowManagerConstRef& design_flow_manager, const ParameterConstRef& parameters);

   /**
    * Destructor
    */
   virtual ~DesignFlowStepFactory();

   /**
    * Return the prefix of the steps created by the factory
    */
   virtual const std::string GetPrefix() const = 0;

   /**
    * Return a step given the signature
    * @param signature is the signature of the step to be created
    * @return the created step
    */
   virtual DesignFlowStepRef CreateFlowStep(const std::string& signature) const;
};
typedef refcount<DesignFlowStepFactory> DesignFlowStepFactoryRef;
#endif
