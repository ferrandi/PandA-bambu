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
 * @file c_backend.hpp
 * @brief Simple class used to drive the backend in order to be able to print c source code
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef C_BACKEND_HPP
#define C_BACKEND_HPP

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <fstream>
#include <iosfwd>
#include <list>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(CBackendInformation);
REF_FORWARD_DECL(CWriter);
REF_FORWARD_DECL(IndentedOutputStream);
CONSTREF_FORWARD_DECL(OpGraph);
class OpVertexSet;
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);

/**
 * Class simply used to drive the backend in order to print C code
 */
class CBackend : public DesignFlowStep
{
 private:
   // CBackendStepFactory is the only class allowed to construct CBackend
   friend class CBackendStepFactory;

   /**
    * Constructor
    * @param type is the type of c backend to be created
    * @param c_backend_information is the information about the backend to be created
    * @param design_flow_manager is the design flow graph manager
    * @param AppM is the manager of the application
    * @param file_name is the file to be created
    * @param Param is the set of input parameters
    */
   CBackend(const CBackendInformationConstRef c_backend_information,
            const DesignFlowManagerConstRef design_flow_manager, const application_managerConstRef AppM,
            const ParameterConstRef _parameters);

 protected:
   const CWriterRef writer;

   const application_managerConstRef AppM;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   const CBackendInformationConstRef c_backend_info;

   bool HasToBeExecuted() const override;

   void Initialize() override;

   DesignFlowStep_Status Exec() override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   std::string GetName() const override;

   /**
    * Compute the signature for a c backend step
    */
   static signature_t ComputeSignature(const CBackendInformationConstRef type);
};
using CBackendRef = refcount<CBackend>;
using CBackendConstRef = refcount<const CBackend>;
#endif
