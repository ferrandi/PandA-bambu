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
 * @file c_writer.hpp
 * @brief This file contains the routines necessary to create a C executable program
 *
 * This file contains the routines necessary to create a C executable program starting from a behavioral specification
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CWRITER_HPP
#define CWRITER_HPP

/// design_flows/backend/ToC/progModels
#include "c_backend.hpp"

/// Graph include
#include "graph.hpp"

#include <deque>
#include <list>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(BBNodeInfo);
CONSTREF_FORWARD_DECL(BehavioralHelper);
class CBackendInformation;
REF_FORWARD_DECL(CWriter);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(IndentedOutputStream);
CONSTREF_FORWARD_DECL(InstructionWriter);
REF_FORWARD_DECL(InstructionWriter);
REF_FORWARD_DECL(Loop);
REF_FORWARD_DECL(machine_node);
CONSTREF_FORWARD_DECL(OpGraph);
class OpVertexSet;
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(var_pp_functor);

class graph;
class instrumented_call_instr_writer;

template <typename Graph>
class dominance;

/**
 * Class used to write the C code representing a program,
 * this class can't be directly instantiated since the backend
 * is not specified yet. So a subclass must be created which
 * redefines all the abstract methods of this class.
 */
class CWriter
{
 protected:
   /// the manager of the application
   const application_managerConstRef AppM;

   /// The tree manager
   const tree_managerConstRef TM;

   /// Represents the stream we are currently writing to
   const IndentedOutputStreamRef indented_output_stream;

   /// This set contains the list of the non built_in types already declared in the global scope
   CustomSet<std::string> globally_declared_types;

   CustomSet<unsigned int> globallyDeclVars;

   /// Contains the class used to write instructions
   const InstructionWriterRef instrWriter;

   /// Counter of the invocations of writeRoutineInstructions; this counter allows to print different labels in differnt tasks to avoid problem due to multiple tasks inlineing
   size_t bb_label_counter;

   /// Verbosity means that a comment is printed for each line in the output file
   bool verbose;

   /// set of parameters
   const ParameterConstRef Param;

   /// the debug level
   int debug_level;

   /// the output level
   int output_level;

   unsigned int fake_max_tree_node_id;

   /// string to be printed at the beginning of a given basic block
   std::map<unsigned int, std::string> basic_block_prefix;

   /// string to be printed at the end of a given basic block
   std::map<unsigned int, std::string> basic_block_tail;

   /// renaming table used by phi node destruction procedure
   std::map<vertex, std::map<unsigned int, std::string>> renaming_table;

   CustomOrderedSet<vertex> bb_frontier;
   CustomOrderedSet<vertex> bb_analyzed;
   std::map<unsigned int, std::string> basic_blocks_labels;
   CustomOrderedSet<vertex> goto_list;
   var_pp_functorConstRef local_rec_variableFunctor;
   FunctionBehaviorConstRef local_rec_function_behavior;
   CustomOrderedSet<vertex> local_rec_instructions;
   const dominance<BBGraph>* dominators;
   const dominance<BBGraph>* post_dominators;
   BBGraphConstRef local_rec_bb_fcfgGraph;
   OpGraphConstRef local_rec_cfgGraph;
   BehavioralHelperConstRef local_rec_behavioral_helper;
   /**
    * Write recursively instructions belonging to a basic block of task or of a function
    * @param current_vertex is the basic block which is being printed
    * @param bracket tells if bracket should be added before and after this basic block
    * @param function_index is the identifier of the function to which instructions belong
    */
   void writeRoutineInstructions_rec(vertex current_vertex, bool bracket, const unsigned int function_index);

   /**
    * Write additional information on the given statement vertex, before the
    * statement itself.
    * The default for this function is to do nothing, but every derived class
    * can specify its own additional information to print
    */
   virtual void writePreInstructionInfo(const FunctionBehaviorConstRef, const vertex);

   /**
    * Write additional information on the given statement vertex, after the
    * statements itself.
    * The default for this function is to do nothing, but every derived class
    * can specify its own additional information to print
    */
   virtual void writePostInstructionInfo(const FunctionBehaviorConstRef, const vertex);

   /**
    * Write the instructions belonging to a body loop
    * @param function_index is the identifier of the function to which instructions belong
    * @param loop_id is the index of the loop to be printed
    * @param current_vertex is the first basic block of the loop
    * @param bracket tells if bracket should be added before and after this basic block
    */
   virtual void WriteBodyLoop(const unsigned int function_index, const unsigned int, vertex current_vertex, bool bracket);

   /**
    * Determines the instructions coming out from phi-node splitting
    */
   void compute_phi_nodes(const FunctionBehaviorConstRef function_behavior, const OpVertexSet& instructions, var_pp_functorConstRef variableFunctor);

   std::vector<std::string> additionalIncludes;
   CustomOrderedSet<std::string> writtenIncludes;

   /**
    * Compute the copy assignments needed by the phi nodes destruction
    * Further details can be found in:
    * - Preston Briggs, Keith D. Cooper, Timothy J. Harvey and L. Taylor Simpson,
    *   "Practical Improvements to the Construction and Destruction of Static Single Assignment Form",
    *   Software -- Practice and Experience 1998
    */
   void insert_copies(vertex b, const BBGraphConstRef bb_domGraph, const BBGraphConstRef bb_fcfgGraph, var_pp_functorConstRef variableFunctor, const CustomSet<unsigned int>& phi_instructions, std::map<unsigned int, unsigned int>& created_variables,
                      std::map<unsigned int, std::string>& symbol_table, std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * insert copies according the algorithm described in Briggs et. al.
    */
   void schedule_copies(vertex b, const BBGraphConstRef bb_domGraph, const BBGraphConstRef bb_fcfgGraph, var_pp_functorConstRef variableFunctor, const CustomSet<unsigned int>& phi_instructions, std::map<unsigned int, unsigned int>& created_variables,
                        std::map<unsigned int, std::string>& symbol_table, std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * create an identifier for the temporaries created by phi node destruction
    * @param symbol_table is the symbol table where the new id is inserted
    * return an id for the symbol created
    */
   unsigned int create_new_identifier(std::map<unsigned int, std::string>& symbol_table);

   /**
    * push on the stack of temporary variables that has to replaced
    * @param symbol_name is the new identifier for dest_i
    * @param dest_i is the id of the variable to be renamed
    * @param pushed is the list of variables to be renamed
    * @param array_of_stacks is the array of stacks used by the phi node destruction procedure
    */
   void push_stack(std::string symbol_name, unsigned int dest_i, std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * remove from the stack all the temporaries
    * @param pushed is the list of variables to be renamed
    * @param array_of_stacks is the array of stacks used by the phi node destruction procedure
    */
   void pop_stack(std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * Write the implementation of a hash table with long long int as key and long long int as value
    */
   void WriteHashTableImplementation();

   /*
    * writes code at the beginning of the basic block denoted by the
    * identifier bb_number in the function function_index.
    * the code is written before all the instructions
    * of the BB, but after the goto label of the BB (if present)
    */
   virtual void WriteBBHeader(const unsigned int bb_number, const unsigned int function_index);

 protected:
   /**
    * Constructor of the class
    * @param AppM is the manager of the application
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the stream where code has to be printed
    * @param Param is the set of parameters
    * @param verbose tells if annotations
    */
   CWriter(const application_managerConstRef _AppM, const InstructionWriterRef instruction_writer, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef Param, bool verbose = true);

 public:
   /**
    * Destructor
    */
   virtual ~CWriter();

   /**
    * Factory method
    * @param type is the type of backend we are creating
    * @param c_backend_information is the information about the backend we are creating
    * @param app_man is the application manager
    * @param indented_output_stream is the output stream
    * @param parameters is the set of input parameters
    * @param verbose tells if produced source code has to be commented
    */
   static CWriterRef CreateCWriter(const CBackend::Type type, const CBackendInformationConstRef c_backend_information, const application_managerConstRef app_man, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters,
                                   const bool verbose);

   /**
    * Initialize data structure
    */
   virtual void Initialize();

   /**
    * Returns the instruction writer
    * @return the instruction writer
    */
   const InstructionWriterRef getInstructionWriter() const;

   /**
    * Compute the local variables of a function
    * @param function_id is the index of a function
    * @return the local variables of a function
    */
   const CustomSet<unsigned int> GetLocalVariables(const unsigned int function_id) const;

   /**
    * Write function implementation
    * @param function_id is the index of the function to be written
    */
   virtual void WriteFunctionImplementation(unsigned int function_index);

   /**
    * Writes the body of the function to the specified stream
    * @param funId is the index of the function
    */
   virtual void StartFunctionBody(const unsigned int funId);

   /**
    * Writes the body of a function
    * @param function_id is the function whose body has to be printed
    */
   virtual void WriteFunctionBody(const unsigned int function_id);

   /**
    * Writes the code necessary to close a function (this function was a function
    * also present in the original specification)
    */
   virtual void EndFunctionBody(unsigned int funId);

   /**
    * Writes the instructions of the current routine, being it a task or a function of the original program.
    * @param function_index is the index of the function
    * @param instructions is the instructions which have to be printed
    * @param variableFunctor is the variable functor
    * @param bb_start is the first basic block to be printed
    * @param bb_end is the set of first basic block not to be printed
    */
   virtual void writeRoutineInstructions(const unsigned int function_index, const OpVertexSet& instructions, var_pp_functorConstRef variableFunctor, vertex bb_start = NULL_VERTEX, CustomOrderedSet<vertex> bb_end = CustomOrderedSet<vertex>());

   /**
    * Writes the declaration of the function whose id in the tree is funId
    */
   virtual void WriteFunctionDeclaration(const unsigned int funId);

   /**
    * Writes the header of the file
    */
   virtual void WriteHeader();

   /**
    * Writes the global declarations
    */
   virtual void WriteGlobalDeclarations();

   /**
    * Writes an include directive
    * @param file_name the name of the header file to be included
    */
   virtual void writeInclude(const std::string& file_name);

   /**
    * This method should be called only if the type associated with the variable
    * is a non built_in type and, in case the non built_in type hasn't been
    * declared yet, it declares it; this method is used to declared new types
    * with scope limited to a routine (be it a function of a task)
    * @param varType the index of the type to be declared
    * @param BH the behavioral helper associated to the function which declare type
    * @param locally_declared_type is the set of type already declared in this function
    * @param routine_name is the name of the routina (function or thread)
    */
   virtual void DeclareType(unsigned int varType, const BehavioralHelperConstRef BH, CustomSet<std::string>& locally_declared_type);

   /**
    * Declares the local variable; in case the variable used in the intialization of
    * curVar hasn't been declared yet it get declared
    * @param curVar is the variable to be declared
    * @param already_decl_variables is the set of already declared variables
    * @param locally_declared_type is the set of already declared types
    * @param behavioral_helper is the behavioral helper
    * @param varFunc is the printer functor
    */
   virtual void DeclareVariable(unsigned int curVar, CustomSet<unsigned int>& already_declared_variables, CustomSet<std::string>& locally_declared_type, const BehavioralHelperConstRef behavioral_helper, const var_pp_functorConstRef varFunc);

   /**
    * Declare all the types used in conversions
    * @param function_id is the function to be condiered
    * @param locally_declared_types is the set of already declared types
    */
   virtual void declare_cast_types(unsigned int funId, CustomSet<std::string>& locally_declared_types);

   /**
    * Declares the local variable; in case the variable used in the intialization of
    * curVar hasn't been declared yet it get declared
    * @param to_be_declared is the set of variables which have to be declared
    * @param already_decl_variables is the set of already declared variables
    * @param locally_declared_type is the set of already declared types
    * @param helper is the behavioral helper associated with the function
    * @param varFunc is the printer functor
    */
   virtual void DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared, CustomSet<unsigned int>& already_declared_variables, CustomSet<std::string>& locally_declared_type, const BehavioralHelperConstRef BH,
                                      const var_pp_functorConstRef varFunc);

   /**
    * Declares the types of the parameters of a function
    * @param fun_id is the index of the function
    */
   virtual void DeclareFunctionTypes(const unsigned int);

   /**
    * Writes the final C file
    * @param file_name is the name of the file to be generated
    */
   virtual void WriteFile(const std::string& file_name);

   /**
    * Writes implementation of __builtin_wait_call
    */
   virtual void WriteBuiltinWaitCall();
};

typedef refcount<CWriter> CWriterRef;

#endif
