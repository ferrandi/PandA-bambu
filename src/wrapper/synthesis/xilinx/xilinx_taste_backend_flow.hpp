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
 * @file xilinx_taste_backend_flow.hpp
 * @brief Wrapper to implement a synthesis tools by Xilinx targeting Taste architecture
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef XILINX_TASTE_BACKEND_FLOW_HPP
#define XILINX_TASTE_BACKEND_FLOW_HPP

/// superclass include
#include "XilinxBackendFlow.hpp"

class XilinxTasteBackendFlow : public XilinxBackendFlow
{
 protected:
   /**
    * Creates the UCF file
    */
   void create_cf(const DesignParametersRef dp, bool xst) override;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param flow_name is the name of the create flow
    * @param manager is the target manager
    */
   XilinxTasteBackendFlow(const ParameterConstRef& parameters, const std::string& flow_name, const target_managerRef& manager);

   /**
    * Generates the synthesis scripts for the specified design
    */
   std::string GenerateSynthesisScripts(const std::string& fu_name, const structural_managerRef SM, const std::list<std::string>& hdl_files, const std::list<std::string>& aux_files) override;
};
#endif
