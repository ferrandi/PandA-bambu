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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
      {
         THROW_UNREACHABLE("");
      }
   }
   return "";
}

HLSFlowStepSpecialization::context_t CBackendInformation::GetSignatureContext() const
{
   return ComputeSignatureContext(C_BACKEND, static_cast<unsigned char>(type));
}
