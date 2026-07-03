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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
 *
 */
#ifndef CWRITER_HPP
#define CWRITER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <deque>
#include <list>
#include <vector>

class BBGraph;
class BBNodeInfo;
class OpVertexSet;
class instrumented_call_instr_writer;
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(CBackendInformation);
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(InstructionWriter);
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(var_pp_functor);
REF_FORWARD_DECL(CWriter);
REF_FORWARD_DECL(IndentedOutputStream);
REF_FORWARD_DECL(InstructionWriter);
REF_FORWARD_DECL(machine_node);

template <typename Graph, bool ComputePostDominators>
class dominance;

/**
 * Class used to write the C code representing a program,
 * this class can't be directly instantiated since the backend
 * is not specified yet. So a subclass must be created which
 * redefines all the abstract methods of this class.
 */
class CWriter
{
   void AnalyzeInclude(const ir_nodeConstRef& tn, const BehavioralHelperConstRef& BH,
                       CustomOrderedSet<std::string>& includes_to_write, CustomSet<unsigned int>& already_visited);

   /**
    * Writes the header of the file
    */
   void WriteHeader();

   /**
    * Writes the global declarations
    */
   void WriteGlobalDeclarations();

   void WriteImplementations();

 protected:
   /// the hls manager
   const HLS_managerConstRef HLSMgr;

   /// The IR manager
   const ir_managerConstRef TM;

   /// Represents the stream we are currently writing to
   const IndentedOutputStreamRef indented_output_stream;

   /// Contains the class used to write instructions
   const InstructionWriterRef instrWriter;

   CustomOrderedSet<unsigned int> declared_functions;

   CustomOrderedSet<unsigned int> defined_functions;

   /// This set contains the list of the non built_in types already declared in the global scope
   CustomSet<std::string> globally_declared_types;

   CustomSet<unsigned int> globallyDeclVars;

   /// Counter of the invocations of writeRoutineInstructions; this counter allows to print different labels in differnt
   /// tasks to avoid problem due to multiple tasks inlineing
   size_t bb_label_counter;

   /// Verbosity means that a comment is printed for each line in the output file
   bool verbose;

   /// set of parameters
   const ParameterConstRef Param;

   /// the debug level
   int debug_level;

   /// the output level
   int output_level;

   unsigned int fake_max_ir_node_id;

   /// string to be printed at the beginning of a given basic block
   std::map<unsigned int, std::string> basic_block_prefix;

   /// string to be printed at the end of a given basic block
   std::map<unsigned int, std::string> basic_block_tail;

   /// renaming table used by phi node destruction procedure
   std::map<gc_vertex_descriptor, std::map<unsigned int, std::string>> renaming_table;

   CustomOrderedSet<gc_vertex_descriptor> bb_frontier;
   CustomOrderedSet<gc_vertex_descriptor> bb_analyzed;
   std::map<unsigned int, std::string> basic_blocks_labels;
   CustomOrderedSet<gc_vertex_descriptor> goto_list;
   CustomOrderedSet<gc_vertex_descriptor> local_rec_instructions;

   std::vector<std::string> additionalIncludes;
   CustomOrderedSet<std::string> writtenIncludes;

   /**
    * Constructor of the class
    * @param _HLSMgr is the HLS manager
    * @param instruction_writer is the instruction writer to use to print the single instruction
    * @param indented_output_stream is the stream where code has to be printed
    */
   CWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef instruction_writer,
           const IndentedOutputStreamRef indented_output_stream);

   virtual void InternalInitialize();

   /**
    * Write recursively instructions belonging to a basic block of task or of a function
    * @param fid is the identifier of the function to which instructions belong
    * @param current_vertex is the basic block which is being printed
    * @param bracket tells if bracket should be added before and after this basic block
    * @param variableFunctor is the functor used to print variables inside the generated code
    */
   void writeRoutineInstructions_rec(unsigned fid, gc_vertex_descriptor current_vertex, bool bracket,
                                     const std::unique_ptr<var_pp_functor>& variableFunctor);

   /**
    * Determines the instructions coming out from phi-node splitting
    */
   void compute_phi_nodes(const FunctionBehaviorConstRef function_behavior, const OpVertexSet& instructions,
                          const std::unique_ptr<var_pp_functor>& variableFunctor);

   /**
    * Compute the copy assignments needed by the phi nodes destruction
    * Further details can be found in:
    * - Preston Briggs, Keith D. Cooper, Timothy J. Harvey and L. Taylor Simpson,
    *   "Practical Improvements to the Construction and Destruction of Static Single Assignment Form",
    *   Software -- Practice and Experience 1998
    */
   void insert_copies(gc_vertex_descriptor b, const BBGraph& bb_domGraph, const BBGraph& bb_fcfgGraph,
                      const std::unique_ptr<var_pp_functor>& variableFunctor,
                      const CustomSet<unsigned int>& phi_instructions,
                      std::map<unsigned int, unsigned int>& created_variables,
                      std::map<unsigned int, std::string>& symbol_table,
                      std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * insert copies according the algorithm described in Briggs et. al.
    */
   void schedule_copies(gc_vertex_descriptor b, const BBGraph& bb_domGraph, const BBGraph& bb_fcfgGraph,
                        const std::unique_ptr<var_pp_functor>& variableFunctor,
                        const CustomSet<unsigned int>& phi_instructions,
                        std::map<unsigned int, unsigned int>& created_variables,
                        std::map<unsigned int, std::string>& symbol_table, std::list<unsigned int>& pushed,
                        std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

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
   void push_stack(std::string symbol_name, unsigned int dest_i, std::list<unsigned int>& pushed,
                   std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * remove from the stack all the temporaries
    * @param pushed is the list of variables to be renamed
    * @param array_of_stacks is the array of stacks used by the phi node destruction procedure
    */
   void pop_stack(std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks);

   /**
    * Write additional information on the given statement vertex, before the
    * statement itself.
    * The default for this function is to do nothing, but every derived class
    * can specify its own additional information to print
    */
   virtual void writePreInstructionInfo(const FunctionBehaviorConstRef, gc_vertex_descriptor);

   /**
    * Write additional information on the given statement vertex, after the
    * statements itself.
    * The default for this function is to do nothing, but every derived class
    * can specify its own additional information to print
    */
   virtual void writePostInstructionInfo(const FunctionBehaviorConstRef, gc_vertex_descriptor);

   /**
    * Write the instructions belonging to a body loop
    * @param function_index is the identifier of the function to which instructions belong
    * @param loop_id is the index of the loop to be printed
    * @param current_vertex is the first basic block of the loop
    * @param bracket tells if bracket should be added before and after this basic block
    * @param variableFunctor is the functor used to print variables inside the generated code
    */
   virtual void WriteBodyLoop(const unsigned int function_index, const unsigned int loop_id,
                              gc_vertex_descriptor current_vertex, bool bracket,
                              const std::unique_ptr<var_pp_functor>& variableFunctor);

   /*
    * writes code at the beginning of the basic block denoted by the
    * identifier bb_number in the function function_index.
    * the code is written before all the instructions
    * of the BB, but after the goto label of the BB (if present)
    */
   virtual void WriteBBHeader(const unsigned int bb_number, const unsigned int function_index);

   /**
    * Compute the local variables of a function
    * @param function_id is the index of a function
    * @return the local variables of a function
    */
   const CustomSet<unsigned int> GetLocalVariables(const unsigned int function_id) const;

   /**
    * Initialize data structure
    */
   virtual void Initialize();

   /**
    * Write function implementation
    * @param function_id is the index of the function to be written
    */
   virtual void WriteFunctionImplementation(unsigned int function_id);

   /**
    * Writes the body of the function to the specified stream
    * @param function_id is the index of the function
    */
   virtual void StartFunctionBody(const unsigned int function_id);

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
   virtual void
   writeRoutineInstructions(const unsigned int function_index, const OpVertexSet& instructions,
                            const std::unique_ptr<var_pp_functor>& variableFunctor,
                            gc_vertex_descriptor bb_start = gc_null_vertex(),
                            CustomOrderedSet<gc_vertex_descriptor> bb_end = CustomOrderedSet<gc_vertex_descriptor>());

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
    * @param varType the type to be declared
    * @param behavioral_helper is the behavioral helper associated with the routine declaring the type
    * @param locally_declared_type is the set of type already declared in this function
    */
   virtual void DeclareType(const ir_nodeConstRef& varType, const BehavioralHelperConstRef& behavioral_helper,
                            CustomSet<std::string>& locally_declared_type);

   /**
    * Declares the local variable; in case the variable used in the intialization of
    * curVar hasn't been declared yet it get declared
    * @param curVar is the variable to be declared
    * @param already_declared_variables is the set of already declared variables
    * @param locally_declared_type is the set of already declared types
    * @param behavioral_helper is the behavioral helper
    * @param varFunc is the printer functor
    */
   virtual void DeclareVariable(const ir_nodeConstRef& curVar, CustomSet<unsigned int>& already_declared_variables,
                                CustomSet<std::string>& locally_declared_type,
                                const BehavioralHelperConstRef& behavioral_helper,
                                const std::unique_ptr<var_pp_functor>& varFunc);

   /**
    * Declare all the types used in conversions
    * @param funId is the function to be considered
    * @param locally_declared_types is the set of already declared types
    */
   virtual void declare_cast_types(unsigned int funId, CustomSet<std::string>& locally_declared_types);

   /**
    * Declares the local variable; in case the variable used in the intialization of
    * curVar hasn't been declared yet it get declared
    * @param to_be_declared is the set of variables which have to be declared
    * @param already_declared_variables is the set of already declared variables
    * @param already_declared_types is the set of already declared types
    * @param behavioral_helper is the behavioral helper associated with the function
    * @param varFunc is the printer functor
    */
   virtual void DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared,
                                      CustomSet<unsigned int>& already_declared_variables,
                                      CustomSet<std::string>& already_declared_types,
                                      const BehavioralHelperConstRef behavioral_helper,
                                      const std::unique_ptr<var_pp_functor>& varFunc);

   /**
    * Declares the types of the parameters of a function
    * @param tn is the IR node describing the function declaration
    */
   virtual void DeclareFunctionTypes(const ir_nodeConstRef& tn);

   /**
    * Writes the declaration of the function whose IR node id is funId
    */
   virtual void WriteFunctionDeclaration(const unsigned int funId);

   /**
    * Writes the header of the file
    */
   virtual void InternalWriteHeader();

   /**
    * Writes the global declarations
    */
   virtual void InternalWriteGlobalDeclarations();

   /**
    * Writes implementation of __builtin_wait_call
    */
   virtual void WriteBuiltinWaitCall();

   virtual void InternalWriteFile();

 public:
   virtual ~CWriter() = default;

   /**
    * Factory method
    * @param c_backend_information is the information about the backend we are creating
    * @param hls_man is the hls manager
    * @param indented_output_stream is the output stream
    */
   static CWriterRef CreateCWriter(const CBackendInformationConstRef c_backend_information,
                                   const HLS_managerConstRef hls_man,
                                   const IndentedOutputStreamRef indented_output_stream);

   /**
    * Writes the final C file
    * @param file_name is the name of the file to be generated
    */
   void WriteFile(const std::string& file_name);
};
using CWriterRef = refcount<CWriter>;
#endif
