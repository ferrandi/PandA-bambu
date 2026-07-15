/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
