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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file hdl_var_decl_fix.hpp
 * @brief Pre-analysis step fixing var_decl duplication and HDL name conflicts.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef HDL_VAR_DECL_FIX_HPP
#define HDL_VAR_DECL_FIX_HPP

/// Superclass include
#include "var_decl_fix.hpp"

enum class HDLWriter_Language;

/**
 * Pre-analysis step. It transforms the raw intermediate representation removing
 * decl duplication (two decl_node with the same name in the same function)
 * and hdl name conflicts (variable or parameters with same name of auxiliary signals
 */
class HDLVarDeclFix : public VarDeclFix
{
 protected:
   /// The hdl language
   const HDLWriter_Language hdl_writer_type;

   /**
    * Return the normalized identifier; in this class it is the identifier itself. Subclasses can specialize it
    * @param identifier is the identifier to be normalized
    * @return the normalized identifier
    */
   const std::string Normalize(const std::string& identifier) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param fun_id is the function index
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   HDLVarDeclFix(const application_managerRef AppM, unsigned int fun_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~HDLVarDeclFix() override;

   /**
    * Fixes the var_decl duplication.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
