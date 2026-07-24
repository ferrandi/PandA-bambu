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
 * @file c_backend_step_factory.hpp
 * @brief Factory class to create c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef C_BACKEND_STEP_FACTORY_HPP
#define C_BACKEND_STEP_FACTORY_HPP

#include "design_flow_step_factory.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(CBackendInformation);

class CBackendStepFactory : public DesignFlowStepFactory
{
 private:
   const application_managerConstRef application_manager;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param application_manager is the application manager
    * @param _parameters is the set of input parameters
    */
   CBackendStepFactory(const DesignFlowManager& design_flow_manager,
                       const application_managerConstRef application_manager, const ParameterConstRef _parameters);

   /**
    * Create a backend c step
    * @param c_backend_information is the information about the frontend to be generated
    */
   DesignFlowStepRef CreateCBackendStep(const CBackendInformationConstRef c_backend_information) const;
};
#endif
