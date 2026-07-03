/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2012-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * This file is part of the PandA framework.
 *
 * Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @file
 * @brief Module for the generation of the top WB4 component.
 */

#ifndef WB4INTERCON_INTERFACE_H
#define WB4INTERCON_INTERFACE_H

#include "WB4_interface.hpp"

/**
 * @brief Class generating the top WB4 module.
 */
class WB4Intercon_interface : public WB4_interface
{
 public:
   /**
    * Constructor.
    */
   WB4Intercon_interface(const ParameterConstRef Param, const HLS_managerRef HLSManager, unsigned int functionId,
                         const DesignFlowManager& design_flow_manager);

   virtual void exec();
};

#endif /* WB4INTERCON_INTERFACE_H */
