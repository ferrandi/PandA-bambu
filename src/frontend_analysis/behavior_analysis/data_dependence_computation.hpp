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
 * @file data_dependence_computation.hpp
 * @brief Base class for different data dependence computation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef DATA_DEPENDENCE_COMPUTATION_HPP
#define DATA_DEPENDENCE_COMPUTATION_HPP

/// Autoheader
#include "config_HAVE_STDCXX_11.hpp"

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// utility include
#include "custom_set.hpp"

/// Forward declaration
enum class FunctionBehavior_VariableAccessType;

class DataDependenceComputation : public FunctionFrontendFlowStep
{
 private:
   void do_dependence_reduction();

 protected:
   /**
    * Compute the dependencies
    * @param dfg_selector is the selector to be used for DFG dependence
    * @param fb_dfg_selector is the selector to be used for DFG feedback dependence
    * @param adg_selector is the selector to be used for ADG dependence
    * @param fb_adg_selector is the selector to be used for ADG feedback dependence
    */
   template <typename type>
   DesignFlowStep_Status Computedependencies(const int dfg_selector, const int fb_dfg_selector, const int adg_selector, const int fb_adg_selector);

   /**
    * Return the variables accessed in a node
    * It is specialized in the different subclasses of this
    * @param statement is the statement to be considered
    * @param variable_access_type is the type of accesses to be considered
    */
   template <typename type>
   CustomSet<type> GetVariables(const vertex statement, const FunctionBehavior_VariableAccessType variable_access_type) const;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param frontend_flow_step_type is the type of data flow analysis
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    * */
   DataDependenceComputation(const application_managerRef _AppM, unsigned int function_id, const FrontendFlowStepType frontend_flow_step_type, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~DataDependenceComputation() override;

   /**
    * Cleans the fake data dependencies
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec()
#if HAVE_STDCXX_11
       final
#endif
       ;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override = 0;
};
#endif
