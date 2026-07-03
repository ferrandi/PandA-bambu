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
 * @file ir_basic_block.hpp
 * @brief Data structure describing a basic block at IR level.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IR_BASIC_BLOCK_HPP
#define IR_BASIC_BLOCK_HPP

#include "custom_set.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <list>
#include <vector>

REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(application_manager);
class ir_node_visitor;
class assign_stmt;

/// return the id given a super class or a class member
#define GETID(field) field##_ID

/**
 * constant identifying the basic block node of type entry
 */
#define BB_ENTRY 0

/**
 * constant identifying the basic block node of type exit
 */
#define BB_EXIT 1

/**
 * This struct specifies the field bloc (basic block).
 * The IR walker structure of this node is:
 * #(TOK_BLOC TOK_NUMBER (pred)+ (succ)+ (phi)* (stmt)*)
 */
struct bloc
{
 private:
   friend class use_counting;
   friend class ir_reindex_remove;

   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   /// list_of_phi is a list of eventual phi node presents in the basic block.
   std::list<ir_nodeRef> list_of_phi;

   /// list_of_stmt is the list of statements stored in the basic block.
   std::list<ir_nodeRef> list_of_stmt;

   /// consistency of ssa uses
   bool updated_ssa_uses;

   void update_new_stmt(const application_managerRef& AppM, const ir_nodeRef& new_stmt);
   void manageCallGraph(const application_managerRef& AppM, const ir_nodeRef& statement);
   bool check_function_call(const ir_nodeRef& statement, assign_stmt* ga, unsigned int& called_function_id);

 public:
   /// list_of_pred is the list of predecessors.
   std::vector<unsigned int> list_of_pred;

   /// list_of_succ is the list of successors.
   std::vector<unsigned int> list_of_succ;

   /// number is the index of the basic block.
   const unsigned int number;

   /// loop identifier
   unsigned int loop_id;

   /// Live_In of the basic block
   CustomOrderedSet<unsigned int> live_in;

   /// Live_Out of the basic block
   CustomOrderedSet<unsigned int> live_out;

   /// store the relation between the phi defs inserted in the live_out set and phi uses
   // std::map<unsigned int, unsigned int> live_out_phi_defs;

   /// The reference to the schedule
   ScheduleRef schedule;

   explicit bloc(unsigned int _number);

   virtual ~bloc() = default;

   /**
    * Add a value to list of pred.
    * @param a is a pred.
    */
   void add_pred(const unsigned int& a)
   {
      list_of_pred.push_back(a);
   }

   /**
    * Add a value to list of succ.
    * @param a is a succ.
    */
   void add_succ(const unsigned int& a)
   {
      list_of_succ.push_back(a);
   }
   /**
    * Add a value to list of phi node.
    * @param phi is the phi statement to be added.
    */
   void AddPhi(const ir_nodeRef phi);

   /**
    * Add a statement in front of list of statements.
    * @param statement is the statement to be added
    * @param AppM is the application manager used to update derived metadata
    */
   void PushFront(const ir_nodeRef statement, const application_managerRef AppM);

   /**
    * Add a statement as last non controlling statement
    * @param statement is the statement to be added
    * @param AppM is the application manager used to update derived metadata
    */
   void PushBack(const ir_nodeRef statement, const application_managerRef AppM);

   /**
    * Replace a statement with another one
    * @param old_stmt is the old statement to be removed
    * @param new_stmt is the new statement to be added
    * @param move_virtuals specifies if memdef and vdef have to be transferred
    * @param AppM is the application manager used to update derived metadata
    */
   void Replace(const ir_nodeRef old_stmt, const ir_nodeRef new_stmt, const bool move_virtuals,
                const application_managerRef AppM);

   /**
    * Add a statement before a specified one
    * @param new_stmt is the statement to be added
    * @param existing_stmt is the statement before which new_stmt has to be added
    * @param AppM is the application manager used to update derived metadata
    */
   void PushBefore(const ir_nodeRef new_stmt, const ir_nodeRef existing_stmt, const application_managerRef AppM);

   /**
    * Add a statement after a specified one
    * @param new_stmt is the statement to be added
    * @param existing_stmt is the statement after which new_stmt has to be added
    * @param AppM is the application manager used to update derived metadata
    */
   void PushAfter(const ir_nodeRef new_stmt, const ir_nodeRef existing_stmt, const application_managerRef AppM);

   /**
    * @brief ReorderLUTs reorders the LUT statements to fix the def-use relations.
    */
   void ReorderLUTs();

   /**
    * Remove a statement
    * @param statement is the statement to be removed
    * @param AppM is the application manager used to update derived metadata
    */
   void RemoveStmt(const ir_nodeRef statement, const application_managerRef AppM);

   /**
    * Remove a phi
    * @param phi is the phi to be removed
    */
   void RemovePhi(const ir_nodeRef phi);

   /**
    * Set that uses of ssa have been computed
    */
   void SetSSAUsesComputed();

   /**
    * Return the list of phi
    */
   const std::list<ir_nodeRef>& CGetPhiList() const;

   /**
    * Return the list of stmt
    */
   const std::list<ir_nodeRef>& CGetStmtList() const;

   /// constant identifying the entry basic block
   static const unsigned int ENTRY_BLOCK_ID;

   /// constant identifying the exit basic block
   static const unsigned int EXIT_BLOCK_ID;

   /// @return the string describing the node.
   std::string ToString() const;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   virtual void visit(ir_node_visitor* const v) const;

   /// visitor enum
   enum
   {
      GETID(list_of_phi) = 0,
      GETID(list_of_stmt)
   };
};

#endif
