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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file c_backend_information.cpp
 * @brief Base class to pass information to a c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */
#include "c_backend_information.hpp"

#include "exceptions.hpp"

CBackendInformation::CBackendInformation(Type _type, const std::filesystem::path& _src_filename,
                                         const std::filesystem::path& _out_filename)
    : type(_type), src_filename(_src_filename), out_filename(_out_filename)
{
}

std::string CBackendInformation::GetName() const
{
   switch(type)
   {
      case(CBackendInformation::CB_BBP):
         return "BasicBlocksProfiling";
#if HAVE_HLS_BUILT
      case(CBackendInformation::CB_DISCREPANCY_ANALYSIS):
         return "DiscrepancyAnalysis";
#endif
      case(CBackendInformation::CB_HLS):
         return "HighLevelSynthesis";
      case(CBackendInformation::CB_SEQUENTIAL):
         return "Sequential";
      case(CBackendInformation::CB_MDPI_WRAPPER):
         return "MDPIWrapper";
      default:
         break;
   }
   THROW_UNREACHABLE("");
   return "";
}

HLSFlowStepSpecialization::context_t CBackendInformation::GetSignatureContext() const
{
   return ComputeSignatureContext(C_BACKEND, static_cast<unsigned char>(type));
}
