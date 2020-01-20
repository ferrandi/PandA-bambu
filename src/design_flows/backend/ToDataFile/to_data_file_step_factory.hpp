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
 * @file to_data_file_step_factory.hpp
 * @brief Factory for to data file step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef TO_DATA_FILE_STEP_FACTORY_HPP
#define TO_DATA_FILE_STEP_FACTORY_HPP

/// Autoheader include
#include "config_HAVE_CIRCUIT_BUILT.hpp"

/// Superclass include
#include "design_flow_step_factory.hpp"

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(target_manager);

class ToDataFileStepFactory : public DesignFlowStepFactory
{
 private:
#if HAVE_CIRCUIT_BUILT
   /// The target device
   const target_managerRef target;
#endif

 public:
   /**
    * Constructor
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   ToDataFileStepFactory(
#if HAVE_CIRCUIT_BUILT
       const target_managerRef target,
#endif
       const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~ToDataFileStepFactory() override;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;

   /**
    * Creates a step
    * @param signature is the signature of the step to be created
    * @return the created step
    */
   const DesignFlowStepRef CreateStep(const std::string& signature) const;
};
#endif
