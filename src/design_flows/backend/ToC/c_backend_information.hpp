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
 * @file c_backend_information.hpp
 * @brief Base class to pass information to a c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */

#ifndef C_BACKEND_INFORMATION_HPP
#define C_BACKEND_INFORMATION_HPP

#include "config_HAVE_HLS_BUILT.hpp"

#include "hls_step.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <string>

/// Base class to pass information to a c backend
class CBackendInformation : public HLSFlowStepSpecialization
{
 public:
   using Type = enum {
      CB_BBP = 0, /* Sequential c with instrumentation for basic block profiling */
#if HAVE_HLS_BUILT
      /**
       * Sequential C code instrumented to dump information on the state
       *  machine and the clock cycles when C statements are executed
       */
      CB_DISCREPANCY_ANALYSIS,
#endif
      CB_HLS,         /* Sequential c code for HLS testing */
      CB_SEQUENTIAL,  /* Sequential c without instrumentation */
      CB_MDPI_WRAPPER /* MDPI simulation wrapper */
   };

   Type type;

   std::filesystem::path src_filename;

   std::filesystem::path out_filename;

   CBackendInformation(Type type, const std::filesystem::path& src_filename,
                       const std::filesystem::path& out_filename = "");

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};
using CBackendInformationConstRef = refcount<const CBackendInformation>;
using CBackendInformationRef = refcount<CBackendInformation>;
#endif
