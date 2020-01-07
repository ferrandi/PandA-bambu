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
 * @file c_backend_step_factory.hpp
 * @brief Factory class to create c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef C_BACKEND_STEP_FACTORY_HPP
#define C_BACKEND_STEP_FACTORY_HPP

/// Autoheader include
#include "config_HAVE_GRAPH_PARTITIONING_BUILT.hpp"
#include "config_HAVE_TARGET_PROFILING.hpp"

/// Superclass include
#include "design_flow_step_factory.hpp"

/// design_flows/backend/ToC/progModels
#include "c_backend.hpp"

/// graph include
#include "graph.hpp"

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(CBackendInformation);
REF_FORWARD_DECL(DesignFlowStep);

class CBackendStepFactory : public DesignFlowStepFactory
{
 private:
   const application_managerConstRef application_manager;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   CBackendStepFactory(const DesignFlowManagerConstRef design_flow_manager, const application_managerConstRef application_manager, const ParameterConstRef param);

   /**
    * Destructor
    */
   ~CBackendStepFactory() override;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;

   /**
    * Create a backend c step
    * @param c_backend_type is the type of c backend to be created
    * @param file_name is the name of the file to be written
    * @param c_backend_information is the information about the frontend to be generated
    */
   const DesignFlowStepRef CreateCBackendStep(const CBackend::Type c_backend_type, const std::string& file_name, const CBackendInformationConstRef c_backend_information) const;
};
#endif
