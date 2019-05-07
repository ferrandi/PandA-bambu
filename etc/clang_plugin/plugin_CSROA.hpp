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
 *              Copyright (C) 2018-2019 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
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
/*
 * The implementation performs scalar replacement of aggregates.
 *
 * @author Marco Siracusa <marco.siracusa@mail.polimi.it>
 *
 */

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <set>

typedef std::set<llvm::Instruction*> inst_set_ty;
typedef std::map<llvm::Function*, std::set<llvm::AllocaInst*>> fun_to_alloca_map_ty;

class CustomScalarReplacementOfAggregatesPass : public llvm::ModulePass
{
 public:
   char ID = 0;

 private:
   const std::string kernel_name;

   inst_set_ty inst_to_remove;
   fun_to_alloca_map_ty alloca_to_remove;
   const llvm::DataLayout* DL;

   // Map specifying how arguments have been expanded
   std::map<llvm::Argument*, std::vector<llvm::Argument*>> exp_args_map;
   // Map specifying how allocas have been expanded
   std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>> exp_allocas_map;
   // Map specifying how global variables have been expanded
   std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>> exp_globals_map;
   // Map specifying array argument sizes
   std::map<llvm::Argument*, std::vector<unsigned long long>> arg_size_map;

 public:
   explicit CustomScalarReplacementOfAggregatesPass(const std::string& kernel_name, char& _ID) : llvm::ModulePass(_ID), kernel_name(kernel_name), DL(nullptr)
   {
   }
   explicit CustomScalarReplacementOfAggregatesPass(const std::string& kernel_name) : llvm::ModulePass(ID), kernel_name(kernel_name), DL(nullptr)
   {
   }

   bool runOnModule(llvm::Module& module) override;

   void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

 private:
   bool check_assumptions(llvm::Function* kernel_function);

   bool expansion_allowed(llvm::Value* aggregate);

   void spot_accessed_globals(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions, std::set<llvm::GlobalVariable*>& accessed_globals);

   void replicate_calls(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions);

   void expand_ptrs(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions);

   void process_pointer(llvm::Use* ptr_u, llvm::BasicBlock*& new_bb);

   void compute_base_and_offset(llvm::Use* ptr_use, llvm::Value*& base_address, std::vector<llvm::Value*>& offset_chain, std::vector<llvm::Instruction*>& inst_chain);

   template <class I>
   llvm::Value* get_element_at_offset(I* base_address, std::map<I*, std::vector<I*>>& map, signed long long offset, unsigned long long accessed_size, const llvm::DataLayout* DL);

   llvm::Value* get_expanded_value(llvm::Value* base_address, signed long long offset, unsigned long long accessed_size, llvm::Argument* arg_if_any = nullptr, llvm::Use* use = nullptr);

   void expand_signatures_and_call_sites(std::vector<llvm::Function*>& inner_functions, std::map<llvm::Function*, llvm::Function*>& exp_fun_map, std::map<llvm::Function*, std::set<unsigned long long>>& exp_idx_args_map, llvm::Function* kernel_function);

   void expand_allocas(llvm::Function* function);

   void expand_globals(std::set<llvm::GlobalVariable*> accessed_variables);

   void cleanup(std::map<llvm::Function*, std::set<unsigned long long>>& exp_idx_args_map, std::map<llvm::Function*, llvm::Function*>& exp_fun_map, std::vector<llvm::Function*>& inner_functions);
};

CustomScalarReplacementOfAggregatesPass* createCustomScalarReplacementOfAggregatesPass(std::string kernel_name);

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP
