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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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

#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#include "hls_step.hpp"
#include "refcount.hpp"

#include <string>

/// Base class to pass information to a c backend
class CBackendInformation : public HLSFlowStepSpecialization
{
 public:
   using Type = enum {
#if HAVE_HOST_PROFILING_BUILT
      CB_BBP, /** Sequential c with instrumentation for basic block profiling */
#endif
#if HAVE_HLS_BUILT
      /**
       * Sequential C code instrumented to dump information on the state
       *  machine and the clock cycles when C statements are executed
       */
      CB_DISCREPANCY_ANALYSIS,
#endif
#if HAVE_BAMBU_BUILT
      CB_HLS, /** Sequential c code for HLS testing */
#endif
      CB_SEQUENTIAL, /**< Sequential c without instrumentation */
   };

   Type type;

   std::string src_filename;

   std::string out_filename;

   CBackendInformation(Type type, const std::string& src_filename, const std::string& out_filename = "");

   virtual ~CBackendInformation();

   std::string GetKindText() const override;

   std::string GetSignature() const override;
};
using CBackendInformationConstRef = refcount<const CBackendInformation>;
using CBackendInformationRef = refcount<CBackendInformation>;
#endif
