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

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_GRAPH_PARTITIONING_BUILT.hpp"
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_TARGET_PROFILING.hpp"
#include "config_HAVE_ZEBU_BUILT.hpp"

/// Superclass include
#include "design_flow_step.hpp"

/// graph include
#include "graph.hpp"

/// utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

/// Std include
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

/**
 * Class simply used to drive the backend in order to print C code
 */
class CBackend : public DesignFlowStep
{
 protected:
   /// The set of already analyzed nodes during search of header to include; it is used to avoid infinite recursion
   CustomUnorderedSet<unsigned int> already_visited;

   /// The output stream
   const IndentedOutputStreamRef indented_output_stream;

   /// the writer
   const CWriterRef writer;

   /// The name of the file to be created
   const std::string file_name;

   /// the manager of the application
   const application_managerConstRef AppM;

   /// The tree_manager
   const tree_managerConstRef TM;

   /**
    * Computes the variables which have to be declared for a function
    * @param inGraph is the data flow graph of the function
    * @param gblVariables is the set containing the node ids of the global variables
    * @param funParams is the list of function parameters
    * @param computed_variables is the set where the computed variables will be stored
    */
   void compute_variables(const OpGraphConstRef inGraph, const CustomUnorderedSet<unsigned int>& gblVariables, std::list<unsigned int>& funParams, CustomUnorderedSet<unsigned int>& computed_variables);

   /**
    * Analyze a variable or a type to identify the includes to be added
    * @param index is the index of the variable or of the type
    * @param BH is the behavioral helper
    * @param includes is where include has to be inseted
    */
   virtual void AnalyzeInclude(unsigned int index, const BehavioralHelperConstRef BH, CustomOrderedSet<std::string>& includes);

   /**
    * Writes the file header, i.e the comments at the beginning of the file
    */
   virtual void writeIncludes();

   /**
    * Write the global declarations
    */
   virtual void WriteGlobalDeclarations();

   virtual void writeImplementations();

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /// The types of backend
   typedef enum
   {
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
#if HAVE_TARGET_PROFILING
      CB_ESCAPED_SEQUENTIAL, /** Sequential c with instrumentation to escape from execution at arbitrary point */
#endif
#if HAVE_BAMBU_BUILT
      CB_HLS, /** Sequential c code for HLS testing */
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
      CB_INSTRUMENTED_PARALLEL, /** Parallel instrumented c */
#endif
#if HAVE_TARGET_PROFILING
      CB_INSTRUMENTED_SEQUENTIAL, /** Sequential c instrumented for target performance profiling*/
#endif
#if HAVE_ZEBU_BUILT
      CB_POINTED_DATA_EVALUATION, /** Sequential instrumented for pointed data evaluation*/
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
      CB_PARALLEL, /**< Parallel c without instrumentation */
#endif
      CB_SEQUENTIAL, /**< Sequential c without instrumentation */
   } Type;

   /// The type of this instance
   const Type c_backend_type;

 private:
   // CBackendStepFactory is the only class allowed to construct CBackend
   friend class CBackendStepFactory;

   /// This set usually is equal to the set of functions without a body.
   CustomOrderedSet<unsigned int> functions_to_be_declared;

   ///
   CustomOrderedSet<unsigned int> functions_to_be_defined;

   /**
    * Constructor
    * @param type is the type of c backend to be created
    * @param c_backend_information is the information about the backend to be created
    * @param design_flow_manager is the design flow graph manager
    * @param AppM is the manager of the application
    * @param file_name is the file to be created
    * @param Param is the set of input parameters
    */
   CBackend(const Type type, const CBackendInformationConstRef c_backend_information, const DesignFlowManagerConstRef design_flow_manager, const application_managerConstRef AppM, std::string file_name, const ParameterConstRef Param);

 public:
   /**
    * Destructor
    */
   ~CBackend() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Compute the signature for a c backend step
    */
   static const std::string ComputeSignature(const Type type);

   /**
    * Return a unified identifier of this design step
    * @return the signature of the design step
    */
   const std::string GetSignature() const override;

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   const std::string GetName() const override;

   /**
    * Return the factory to create this type of steps
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * @return the associated c writer
    */
   const CWriterRef GetCWriter() const;
};
typedef refcount<CBackend> CBackendRef;
typedef refcount<const CBackend> CBackendConstRef;
#endif
