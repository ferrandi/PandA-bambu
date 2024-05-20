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
 * @file to_data_file_step_factory.hpp
 * @brief Factory for to data file step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef TO_DATA_FILE_STEP_FACTORY_HPP
#define TO_DATA_FILE_STEP_FACTORY_HPP
#include "design_flow_step_factory.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(generic_device);

class ToDataFileStepFactory : public DesignFlowStepFactory
{
   const generic_deviceRef device;

 public:
   /**
    * Constructor
    * @param _device is the device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   ToDataFileStepFactory(const generic_deviceRef _device, const DesignFlowManagerConstRef design_flow_manager,
                         const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~ToDataFileStepFactory() override;

   /**
    * Creates a step
    * @param signature is the signature of the step to be created
    * @return the created step
    */
   DesignFlowStepRef CreateStep(DesignFlowStep::signature_t signature) const;
};
#endif
