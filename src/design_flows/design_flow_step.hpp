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
 * @file design_flow_step.hpp
 * @brief Base class for step of design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DESIGN_FLOW_STEP_HPP
#define DESIGN_FLOW_STEP_HPP
#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <iosfwd>
#include <string>

#include "config_HAVE_UNORDERED.hpp"

CONSTREF_FORWARD_DECL(DesignFlowManager);
REF_FORWARD_DECL(DesignFlowStep);
CONSTREF_FORWARD_DECL(DesignFlowStepFactory);
CONSTREF_FORWARD_DECL(Parameter);
class DesignFlowStepNecessitySorter;

struct DesignFlowStepHash
{
   size_t operator()(const DesignFlowStepRef& step) const;
};

struct DesignFlowStepEqual
{
   bool operator()(const DesignFlowStepRef& x, const DesignFlowStepRef& y) const;
};

using DesignFlowStepSet = CustomUnorderedSet<DesignFlowStepRef, DesignFlowStepHash, DesignFlowStepEqual>;

/// The status of a step
enum class DesignFlowStep_Status
{
   ABORTED,     //! Step aborts
   EMPTY,       //! Step is symbolic and it has already been marked
   NONEXISTENT, //! Step does not exits
   SKIPPED,     //! Step skipped
   SUCCESS,     //! Step successfully executed
   UNCHANGED,   //! Step successfully executed but without any IR change
   UNEXECUTED,  //! Step not yet executed
   UNNECESSARY, //! Step not yet executed and unnecessary
};

/**
 * The base class for design step
 */
class DesignFlowStep
{
 public:
   /**
    * The relationship type
    */
   enum RelationshipType
   {
      DEPENDENCE_RELATIONSHIP = 0, //! Source must be executed to satisfy target
      INVALIDATION_RELATIONSHIP,   //! Target must be reexecuted
      PRECEDENCE_RELATIONSHIP      //! Source must be executed before target
   };

   /**
    * @brief Set of derivate classes for unique signature
    *
    */
   enum StepClass
   {
      AUX = 0,
      TECHNOLOGY,
      DESIGN_FLOW,
      PARSER,
      C_BACKEND,
      TO_DATA_FILE,
      FRONTEND,
      APPLICATION_FRONTEND,
      SYMBOLIC_APPLICATION_FRONTEND,
      FUNCTION_FRONTEND,
      HLS,
      HLS_FUNCTION,
      RTL_CHARACTERIZATION
   };

   using signature_t = unsigned long long int;

 protected:
   /// True if this step represents a composition of design flow steps (e.g., a flow); must be set by specialized
   /// constructors
   bool composed;

   /// The design flow manager
   const Wrefcount<const DesignFlowManager> design_flow_manager;

   /// Set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

   /// The output level
   const int output_level;

   const signature_t signature;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowStep(const signature_t signature, const DesignFlowManagerConstRef design_flow_manager,
                  const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~DesignFlowStep();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status Exec() = 0;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   virtual bool HasToBeExecuted() const = 0;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   virtual void Initialize();

   /**
    * Return a unified identifier of this design step
    * @return the signature of the design step
    */
   signature_t GetSignature() const;

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   virtual std::string GetName() const;

   /**
    * Return the status of this design step
    */
   DesignFlowStep_Status GetStatus() const;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   virtual void ComputeRelationships(DesignFlowStepSet& relationship,
                                     const DesignFlowStep::RelationshipType relationship_type) = 0;

   /**
    * Write the label for a dot graph
    * @param out is the stream where label has to be printed
    */
   virtual void WriteDot(std::ostream& out) const;

   /**
    * Return the factory to create this type of steps
    */
   virtual DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const = 0;

   /**
    * Returns if this step is composed
    * @return if this step is composed
    */
   bool IsComposed() const;

   /**
    * Return the debug level of the step
    * @return the debug level of the step
    */
   int CGetDebugLevel() const;

   /**
    * Dump the initial intermediate representation
    */
   virtual void PrintInitialIR() const;

   /**
    * Dump the final intermediate representation
    */
   virtual void PrintFinalIR() const;

   /**
    * @brief Compute design flow step signature
    *
    * @param step_class Design flow step super class id
    * @param step_type Design flow step type id
    * @param context Additional 40-bits context id
    * @return signature_t Design flow step unique id
    */
   static signature_t ComputeSignature(StepClass step_class, unsigned short step_type, unsigned long long context);

   /**
    * @brief Get the step class from signature
    *
    * @param signature
    * @return StepClass
    */
   static StepClass GetStepClass(signature_t signature);

   /**
    * @brief Get the step type from signature
    *
    * @param signature
    * @return unsigned short
    */
   static unsigned short GetStepType(signature_t signature);

   /**
    * @brief Get the signature context from signature
    *
    * @param signature
    * @return unsigned long long
    */
   static unsigned long long GetSignatureContext(signature_t signature);
};
using DesignFlowStepRef = refcount<DesignFlowStep>;
using DesignFlowStepConstRef = refcount<const DesignFlowStep>;

#endif
