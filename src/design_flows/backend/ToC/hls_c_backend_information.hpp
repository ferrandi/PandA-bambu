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
 * @file hls_c_backend_information.hpp
 * @brief Class to pass information to the hls backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */

/// Super class include
#include "c_backend_information.hpp"

/// STL include
#include "custom_map.hpp"
#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(HLS_manager);

class HLSCBackendInformation : public CBackendInformation
{
 public:
   /// The file containing input
   const std::string results_filename;

   /// A reference to the HLS manager to retrieve information if needed
   const HLS_managerConstRef HLSMgr;

   /**
    * Constructor
    * @param results_filename It is the name of the file where the the C
    * program will put its results when it will be executed. This file is a
    * necessary input for the generation of a HDL testbench
    * @param HLSMgr A reference to the HLS_manager where the information on
    * the HLS can be found
    */
   HLSCBackendInformation(std::string results_filename, const HLS_managerConstRef& HLSMgr);

   /**
    * Destructor
    */
   ~HLSCBackendInformation() override;
};
