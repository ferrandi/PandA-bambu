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
