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
#include "plugin_includes.hpp"

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#define DEBUG_TYPE "csroa"

#include "plugin_CSROA.hpp"

static llvm::cl::opt<uint32_t> MaxNumScalarTypes("csroa-expanded-scalar-threshold",
                                                 llvm::cl::Hidden,
                                                 llvm::cl::init(64),
                                                 llvm::cl::desc(
                                                         "Max amount of scalar types contained in a type for allowing disaggregation"));

static llvm::cl::opt<uint32_t> MaxTypeByteSize("csroa-type-byte-size",
                                               llvm::cl::Hidden,
                                               llvm::cl::init(32*8),
                                               llvm::cl::desc("Max type size (in bytes) allowed for disaggregation"));

bool CustomScalarReplacementOfAggregatesPass::runOnModule(llvm::Module& module)
{
   DL = &module.getDataLayout();
   llvm::Function* kernel_function = module.getFunction(kernel_name);

   assert(kernel_function != nullptr && "Unknown kernel function!");

   // Functions contained by the kernel and the callees
   std::vector<llvm::Function*> inner_functions;

   // Global variables accessed by the inner functions
   std::set<llvm::GlobalVariable*> accessed_globals;

   if(!check_assumptions(kernel_function))
   {
      return false;
   }

   // Compute the inner functions, replicating those on call sites and check assumptions
   replicate_calls(kernel_function, inner_functions);

   // Spot the accessed global variables
   spot_accessed_globals(kernel_function, inner_functions, accessed_globals);

   // Expand the accessed global variables
   expand_globals(accessed_globals);

   // Map specifying the expanded arguments for each function
   std::map<llvm::Function*, std::set<unsigned long long>> exp_idx_args_map;
   // Map linking any function with its modified version
   std::map<llvm::Function*, llvm::Function*> exp_fun_map;

   // Expand aggregate elements in signatures and in call sites (use nullptrs for expanded arguments)
   expand_signatures_and_call_sites(inner_functions, exp_fun_map, exp_idx_args_map, kernel_function);

   // Expand all the loads/stores/calls
   expand_ptrs(kernel_function, inner_functions);

   // Cleanup the remaining code
   cleanup(exp_idx_args_map, exp_fun_map, inner_functions);

   return true;
}

void CustomScalarReplacementOfAggregatesPass::compute_base_and_offset(llvm::Use* ptr_use, llvm::Value*& base_address, std::vector<llvm::Value*>& offset_chain, std::vector<llvm::Instruction*>& inst_chain)
{
   if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(ptr_use->get()))
   {
      if(llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(ptr_use->getUser()))
      {
         inst_chain.push_back(inst);
      }
      // Recursively go through the gepi chain up to the base address
      compute_base_and_offset(&bitcast_op->getOperandUse(0), base_address, offset_chain, inst_chain);
   }
   else if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(ptr_use->get()))
   {
      llvm::Instruction* containing_inst = nullptr;

      if(llvm::GetElementPtrInst* gep_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr_use->get()))
      {
         containing_inst = gep_inst;
         inst_chain.push_back(gep_inst);
      }
      else
      {
         if(llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(ptr_use->getUser()))
         {
            containing_inst = inst;
         }
         else
         {
            llvm::errs() << "ERR: User not an inst\n";
            ptr_use->get()->dump();
            ptr_use->getUser()->dump();
            exit(-1);
         }
      }

      const llvm::DataLayout* DL = &containing_inst->getModule()->getDataLayout();

      // Recursively go through the gepi chain up to the base address
      compute_base_and_offset(&gep_op->getOperandUse(gep_op->getPointerOperandIndex()), base_address, offset_chain, inst_chain);

      if(gep_op->hasAllConstantIndices())
      {
         llvm::APInt offset_ai(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);
         assert(gep_op->accumulateConstantOffset(*DL, offset_ai));

         // If the gepi has all constant indexes different from zero fold those
         if(offset_ai.getSExtValue() != 0)
         {
            // Fold with the last element of the chain if also constant
            if(llvm::ConstantInt* c_last = llvm::dyn_cast<llvm::ConstantInt>(offset_chain.back()))
            {
               signed long long offset_sum = c_last->getSExtValue() + offset_ai.getSExtValue();
               llvm::APInt offset_sum_ai(DL->getPointerTypeSizeInBits(gep_op->getType()), offset_sum);
               offset_chain.back() = llvm::ConstantInt::get(gep_op->getContext(), offset_sum_ai);
            }
            else
            {
               offset_chain.push_back(llvm::ConstantInt::get(gep_op->getContext(), offset_ai));
            }
         }
      }
      else
      {
         llvm::APInt ConstantIndexOffset(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);

         // If some index non constant go through those and add those in the chain
         for(llvm::gep_type_iterator gt_it = llvm::gep_type_begin(gep_op), GTE = llvm::gep_type_end(gep_op); gt_it != GTE; ++gt_it)
         {
            if(gt_it.getStructTypeOrNull())
            {
               // llvm_unreachable("unexpected condition: struct LowerGetElementPtrOffset");
               // continue;
            }

            llvm::Value* index = gt_it.getOperand();

            llvm::APInt array_elmt_size = llvm::APInt(ConstantIndexOffset.getBitWidth(), DL->getTypeAllocSize(gt_it.getIndexedType()));

            // Discriminate whether constant index or not
            if(llvm::ConstantInt* c_index = llvm::dyn_cast<llvm::ConstantInt>(index))
            {
               signed long long offset = c_index->getSExtValue() * array_elmt_size.getSExtValue();
               llvm::APInt offset_ai(c_index->getBitWidth(), offset);

               // Fold with the last element of the chain if constant as well
               if(llvm::ConstantInt* c_last = llvm::dyn_cast<llvm::ConstantInt>(offset_chain.back()))
               {
                  signed long long offset_sum = c_last->getSExtValue() + offset_ai.getSExtValue();
                  llvm::APInt offset_sum_ai(DL->getPointerTypeSizeInBits(gep_op->getType()), offset_sum);
                  offset_chain.back() = llvm::ConstantInt::get(gep_op->getContext(), offset_sum_ai);
               }
               else
               {
                  offset_chain.push_back(llvm::ConstantInt::get(gep_op->getContext(), offset_ai));
               }
            }
            else
            {
               // Push the indexed size * offset mul operator for non constant indexes
               llvm::ConstantInt* c_array_elmt_size = llvm::ConstantInt::get(gep_op->getContext(), array_elmt_size);

               std::string mul_name = "lowered.mul." + gep_op->getName().str();
               llvm::Instruction* index_times_size = llvm::BinaryOperator::Create(llvm::Instruction::Mul, index, c_array_elmt_size, mul_name, containing_inst);

               offset_chain.push_back(index_times_size);
            }
         }
      }
   }
   else if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(ptr_use->get()))
   {
      // The alloca becomes the base address
      base_address = alloca_inst;

      // Initialize the offset chain with zero (which may be folded subsequently)
      offset_chain.push_back(llvm::ConstantInt::get(ptr_use->get()->getContext(), llvm::APInt(32, 0)));
   }
   else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(ptr_use->get()))
   {
      // The global variable becomes the base address
      base_address = g_var;

      // Initialize the offset chain with zero (which may be folded subsequently)
      offset_chain.push_back(llvm::ConstantInt::get(ptr_use->get()->getContext(), llvm::APInt(32, 0)));
   }
   else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(ptr_use->get()))
   {
      // The argument becomes the base address
      base_address = arg;

      // Initialize the offset chain with zero (which may be folded subsequently)
      offset_chain.push_back(llvm::ConstantInt::get(ptr_use->get()->getContext(), llvm::APInt(32, 0)));
   }
   else
   {
      llvm::errs() << "ERR: Only gepi chains supported\n";
      ptr_use->get()->dump();
      ptr_use->getUser()->dump();
      exit(-1);
   }
}

template <class T>
void expand_types(T* ptr, std::map<T*, std::vector<T*>>& exp_map_ref, std::vector<llvm::Type*>& exp_types_ref)
{
   auto exp_it = exp_map_ref.find(ptr);

   // Recursively iterate if the element has been expanded, push the type otherwise
   if(exp_it != exp_map_ref.end() and !exp_it->second.empty())
   {
      std::vector<T*>& exp_vec_ref = exp_it->second;

      for(T* exp_el : exp_vec_ref)
      {
         expand_types(exp_el, exp_map_ref, exp_types_ref);
      }
   }
   else
   {
      exp_types_ref.push_back(ptr->getType()->getPointerElementType());
   }
}

void CustomScalarReplacementOfAggregatesPass::process_pointer(llvm::Use* ptr_u, llvm::BasicBlock*& new_bb)
{
   llvm::Value* base_address = nullptr;
   std::vector<llvm::Value*> offset_chain;

   if(llvm::isa<llvm::ConstantPointerNull>(ptr_u->get()))
   {
      // TODO fix it
      return;
   }

   if(llvm::Instruction* user_inst = llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser()))
   {
      std::vector<llvm::Instruction*> inst_chain;

      compute_base_and_offset(ptr_u, base_address, offset_chain, inst_chain);

      bool is_expansion_allowed = expansion_allowed(base_address);
      if(is_expansion_allowed)
      {
         for(llvm::Instruction* i : inst_chain)
         {
            inst_to_remove.insert(i);
         }
      }

      signed long long constant_sum = 0;
      bool is_constant = true;

      // Check whether the offset is constant
      for(llvm::Value* offset : offset_chain)
      {
         if(llvm::ConstantInt* c_offset = llvm::dyn_cast<llvm::ConstantInt>(offset))
         {
            constant_sum += c_offset->getSExtValue();
         }
         else
         {
            is_constant = false;
            break;
         }
      }

      if(llvm::isa<llvm::StoreInst>(ptr_u->getUser()) or llvm::isa<llvm::LoadInst>(ptr_u->getUser()))
      {
         if(!is_expansion_allowed)
         {
            return;
         }

         if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(ptr_u->getUser()))
         {
            if(load_inst->getPointerOperand() != ptr_u->get())
            {
               llvm::errs() << "ERR: Bad load inst usage\n";
               load_inst->dump();
               exit(-1);
            }
         }
         else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(ptr_u->getUser()))
         {
            if(store_inst->getPointerOperand() != ptr_u->get())
            {
               llvm::errs() << "ERR: Bad store inst usage\n";
               store_inst->dump();
               exit(-1);
            }
         }

         if(is_constant)
         {
            const llvm::DataLayout* DL = &llvm::cast<llvm::Instruction>(ptr_u->getUser())->getModule()->getDataLayout();
            unsigned long long accessed_size = DL->getTypeAllocSize(ptr_u->get()->getType()->getPointerElementType());
            llvm::Value* exp_val = get_expanded_value(base_address, constant_sum, accessed_size, nullptr, ptr_u);

            ptr_u->set(exp_val);
         }
         else
         { // Non constant offset

            std::vector<llvm::Type*> expanded_types;

            if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_address))
            {
               expand_types(alloca_inst, exp_allocas_map, expanded_types);
            }
            else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
            {
               expand_types(arg, exp_args_map, expanded_types);
            }
            else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
            {
               expand_types(g_var, exp_globals_map, expanded_types);
            }
            else
            {
               llvm::errs() << "ERR: Unknown base\n";
               base_address->dump();
               exit(-1);
            }

            llvm::APInt zero_ai = llvm::APInt((unsigned int)32, 0, false);
            llvm::ConstantInt* zero_c = llvm::ConstantInt::get(base_address->getContext(), zero_ai);

            llvm::Value* bytes_sum = zero_c;

            // Build the chain of adders for computing the offset
            for(llvm::Value* offset : offset_chain)
            {
               std::string name = ptr_u->getUser()->getName().str() + ".add";
               bytes_sum = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Add, bytes_sum, offset, name, user_inst);
            }

            // Keep track on where to split
            llvm::Instruction* split_before = user_inst;

            unsigned long long bytes_acc = 0;

            llvm::Value* right_phi_operand = llvm::UndefValue::get(ptr_u->getUser()->getType());
            llvm::PHINode* last_phi_set = nullptr;

            const llvm::DataLayout* DL = &user_inst->getModule()->getDataLayout();

            // Create the if-then-else chain
            for(llvm::Type* exp_ty : expanded_types)
            {
               unsigned long long type_size = DL->getTypeAllocSize(exp_ty);
               llvm::APInt bytes_ai = llvm::APInt((unsigned int)32, bytes_acc, false);

               bytes_acc += type_size;

               // llvm::Type *bitcast_type = nullptr;
               llvm::Type* ty1 = exp_ty;
               llvm::Type* ty2 = ptr_u->get()->getType()->getPointerElementType();
               if(ty1->getTypeID() == ty2->getTypeID() and DL->getTypeAllocSize(ty1) > DL->getTypeAllocSize(ty2))
               {
                  // bitcast_type = ty2;
                  llvm::errs() << "ERR: bitcast brokes anything";
               }
               else if(ty1 != ty2)
               {
                  continue;
               }

               llvm::ConstantInt* bytes_c = llvm::ConstantInt::get(base_address->getContext(), bytes_ai);
               std::string cmp_name = ptr_u->getUser()->getName().str() + ".cmp." + std::to_string(0);
               llvm::CmpInst* cond = llvm::CmpInst::Create(llvm::CmpInst::OtherOps::ICmp, llvm::CmpInst::Predicate::ICMP_EQ, bytes_sum, bytes_c, cmp_name, split_before);

               llvm::TerminatorInst* then_term;
               llvm::TerminatorInst* else_term;
               llvm::SplitBlockAndInsertIfThenElse(cond, split_before, &then_term, &else_term);
               then_term->getParent()->setName("csroa.if.then");
               else_term->getParent()->setName("csroa.if.else");
               split_before->getParent()->setName("csroa.if.end");

               if(new_bb == nullptr)
               {
                  new_bb = split_before->getParent();
               }

               llvm::Instruction* new_inst = user_inst->clone();
               new_inst->insertBefore(then_term);

               if(llvm::LoadInst* load_isnt = llvm::dyn_cast<llvm::LoadInst>(ptr_u->getUser()))
               {
                  llvm::PHINode* phi_node = llvm::PHINode::Create(ptr_u->get()->getType()->getPointerElementType(), expanded_types.size(), ptr_u->getUser()->getName().str() + ".phi", split_before);

                  phi_node->addIncoming(new_inst, then_term->getParent());
                  phi_node->addIncoming(llvm::UndefValue::get(new_inst->getType()), else_term->getParent());

                  if(last_phi_set)
                  {
                     last_phi_set->setIncomingValue(1, phi_node);
                  }
                  else
                  {
                     user_inst->replaceAllUsesWith(phi_node);
                  }

                  last_phi_set = phi_node;
               }

               llvm::Value* exp_val = get_expanded_value(base_address, bytes_acc - type_size, type_size, nullptr, ptr_u);

               /*if (bitcast_type != nullptr) {
                   std::string bitcast_name = user_inst->getName().str() + ".bitcast";
                   llvm::BitCastInst *bitcast_inst = new llvm::BitCastInst(exp_val, bitcast_type, bitcast_name, new_inst);
                   exp_val = bitcast_inst;
               }*/
               new_inst->setOperand(ptr_u->getOperandNo(), exp_val);

               split_before = else_term;
            }

            // TODO Fix it
            user_inst->replaceAllUsesWith(llvm::UndefValue::get(user_inst->getType()));
            user_inst->eraseFromParent();
         }
      }
      else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(ptr_u->getUser()))
      {
         if(is_constant)
         {
            llvm::Argument* arg_u = nullptr;
            {
               llvm::Function::arg_iterator arg_u_it = call_inst->getCalledFunction()->arg_begin();

               for(unsigned long long i = 0; i < ptr_u->getOperandNo(); i++)
               {
                  arg_u_it++;
               }
               arg_u = &*arg_u_it;
            }

            auto exp_arg_it = exp_args_map.find(arg_u);

            unsigned long long arg_offset = constant_sum;
            if(exp_arg_it != exp_args_map.end())
            {
               unsigned long long exp_arg_u_idx = 0;
               for(llvm::Argument* exp_arg_u : exp_arg_it->second)
               {
                  const llvm::DataLayout* DL = &call_inst->getModule()->getDataLayout();
                  unsigned long long accessed_size = DL->getTypeAllocSize(exp_arg_u->getType()->getPointerElementType());

                  auto exp_arg_size_it = arg_size_map.find(exp_arg_u);
                  if(exp_arg_size_it != arg_size_map.end() and !exp_arg_size_it->second.empty())
                  {
                     accessed_size *= exp_arg_size_it->second.front();
                  }

                  llvm::Value* exp_val = get_expanded_value(base_address, arg_offset, accessed_size, arg_u, ptr_u);

                  // Take care of array decay now
                  if(exp_val->getType()->getPointerElementType()->isArrayTy())
                  {
                     if(exp_val->getType()->getPointerElementType()->getArrayElementType() == exp_arg_u->getType()->getPointerElementType())
                     {
                        if(!llvm::isa<llvm::Argument>(exp_val))
                        {
                           std::vector<llvm::Value*> gepi_ops = std::vector<llvm::Value*>();
                           llvm::Type* op1_ty = llvm::IntegerType::get(exp_arg_u->getContext(), 64);
                           llvm::Constant* op1 = llvm::ConstantInt::get(op1_ty, 0, false);
                           gepi_ops.push_back(op1);
                           llvm::Type* op2_ty = llvm::IntegerType::get(exp_arg_u->getContext(), 64);
                           llvm::Constant* op2 = llvm::ConstantInt::get(op2_ty, 0, false);
                           gepi_ops.push_back(op2);

                           std::string gepi_name = exp_val->getName().str() + ".decay";
                           llvm::Type* gepi_type = exp_val->getType()->getPointerElementType();

                           llvm::GetElementPtrInst* decay_gep_inst = llvm::GetElementPtrInst::Create(gepi_type, exp_val, gepi_ops, gepi_name, call_inst);

                           exp_val = decay_gep_inst;
                        }
                     }
                     else
                     {
                        // llvm::errs() << "ERR: Malformed decay!\n";
                        // call_inst->dump();
                        // exp_arg_u->dump();
                        // exp_val->dump();
                        // exit(-1);
                        // TODO Review this
                     }
                  }

                  call_inst->setOperand(exp_arg_u->getArgNo(), exp_val);

                  arg_offset += accessed_size;
                  exp_arg_u_idx++;
               }
            }
         }
         else
         {
            llvm::errs() << "ERR: Non constant access in function call operand\n";
            call_inst->dump();
            exit(-1);
         }
      }
   }
   else
   {
      llvm::errs() << "ERR: User not inst\n";
      exit(-1);
   }
}

template <class I>
llvm::Value* CustomScalarReplacementOfAggregatesPass::get_element_at_offset(I* base_address, std::map<I*, std::vector<I*>>& map, signed long long offset, unsigned long long accessed_size, const llvm::DataLayout* DL)
{
   I* el_to_exp = base_address;
   signed long long offset_to_exp = offset;
   // llvm::errs() << "Offset: " << offset << "\n";
   // llvm::errs() << "Wanna access: " << accessed_size << "\n";
   while(offset_to_exp > 0)
   {
      auto exp_it = map.find(el_to_exp);

      if(exp_it != map.end())
      {
         std::vector<I*>& subelements = exp_it->second;

         bool take_next = false;

         for(I* el : subelements)
         {
            unsigned long long allocated_size = DL->getTypeAllocSize(el->getType()->getPointerElementType());

            if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(el))
            {
               allocated_size = DL->getTypeAllocSize(gep_op->getResultElementType());
            }

            auto arg_size_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(el));
            if(arg_size_it != arg_size_map.end() and !arg_size_it->second.empty())
            {
               allocated_size *= arg_size_it->second.front();
            }

            if(offset_to_exp == 0)
            {
               if(take_next)
               {
                  el_to_exp = el;
               }

               break;
            }

            if(offset_to_exp - (signed long long)allocated_size > 0)
            {
               offset_to_exp -= allocated_size;
            }
            else if(offset_to_exp - (signed long long)allocated_size == 0)
            {
               offset_to_exp -= allocated_size;
               take_next = true;
            }
            else
            {
               el_to_exp = el;
               break;
            }
         }
      }
      else
      {
         llvm::errs() << "ERR: no expansion found!\n";
         base_address->dump();
         el_to_exp->dump();
         exit(-1);
      }
   }

   do
   {
      unsigned long long expanded_size = DL->getTypeAllocSize(el_to_exp->getType()->getPointerElementType());

      if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(el_to_exp))
      {
         expanded_size = DL->getTypeAllocSize(gep_op->getResultElementType());
      }

      auto arg_size_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(el_to_exp));
      llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(el_to_exp);
      if(arg_size_it != arg_size_map.end() and !arg_size_it->second.empty())
      {
         expanded_size *= arg_size_it->second.front();
      }

      if(accessed_size < expanded_size)
      {
         auto exp_it = map.find(el_to_exp);

         if(exp_it != map.end())
         {
            std::vector<I*>& subelements = exp_it->second;

            el_to_exp = subelements.front();
         }
         else
         {
            llvm::errs() << "ERR: El not found in map\n";
            llvm::errs() << "Offset: " << offset << "\nAcc size: " << accessed_size << "\nExp_size: " << expanded_size << "\n";
            llvm::errs() << "B: ";
            base_address->dump();
            llvm::errs() << "E: ";
            el_to_exp->dump();
            exit(-1);
         }
      }
      else if(accessed_size == expanded_size)
      {
         break;
      }
      else
      {
         llvm::errs() << "ERR: bad access size\n";
         llvm::errs() << "Offset: " << offset << "\nAcc size: " << accessed_size << "\nExp_size: " << expanded_size << "\n";
         el_to_exp->dump();
         exit(-1);
      }
   } while(true);

   return el_to_exp;
}

llvm::Value* CustomScalarReplacementOfAggregatesPass::get_expanded_value(llvm::Value* base_address, signed long long offset, unsigned long long accessed_size, llvm::Argument* arg_if_any, llvm::Use* use)
{
   if(!expansion_allowed(base_address))
   {
      std::map<llvm::Value*, std::vector<llvm::Value*>> gepi_map;

      class GenGepiMap
      {
       public:
         static void gen_gepi_map(llvm::Value* gepi_base, llvm::Argument* arg, llvm::Use* use, std::map<llvm::Value*, std::vector<llvm::Value*>>& gepi_map, const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& arg_map,
                                  std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_size_map, std::string gepi_name)
         {
            auto arg_it = arg_map.find(arg);

            if(arg_it != arg_map.end())
            {
               std::vector<llvm::Type*> type_vec;

               auto size_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(gepi_base));
               bool is_array = false;
               if(size_it != arg_size_map.end())
               {
                  if(!size_it->second.empty())
                  {
                     if(size_it->second.front() > 1)
                     {
                        for(unsigned long long idx = 0; idx < size_it->second.front(); idx++)
                        {
                           is_array = true;
                           type_vec.push_back(gepi_base->getType()->getPointerElementType());
                        }
                     }
                  }
               }

               if(!is_array)
               {
                  llvm::Type* ptd_ty;
                  if(gepi_base->getType()->isPointerTy())
                  {
                     ptd_ty = gepi_base->getType()->getPointerElementType();
                  }
                  else
                  {
                     ptd_ty = gepi_base->getType();
                  }

                  if(ptd_ty->isAggregateType())
                  {
                     if(ptd_ty->isArrayTy())
                     {
                        for(unsigned long long idx = 0; idx < ptd_ty->getArrayNumElements(); idx++)
                        {
                           type_vec.push_back(ptd_ty->getArrayElementType());
                        }
                     }
                     else if(ptd_ty->isStructTy())
                     {
                        for(unsigned long long idx = 0; idx < ptd_ty->getStructNumElements(); idx++)
                        {
                           type_vec.push_back(ptd_ty->getContainedType(idx));
                        }
                     }
                     else if(ptd_ty->isVectorTy())
                     {
                        for(unsigned long long idx = 0; idx < ptd_ty->getVectorNumElements(); idx++)
                        {
                           type_vec.push_back(ptd_ty->getVectorElementType());
                        }
                     }
                     else
                     {
                        llvm::errs() << "ERR: Unknown aggregate\n";
                        exit(-1);
                     }
                  }
               }

               unsigned long long idx = 0;
               for(llvm::Type* ty : type_vec)
               {
                  std::vector<llvm::Value*> gepi_ops = std::vector<llvm::Value*>();

                  llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(gepi_base->getType()->getScalarType())->getElementType();

                  if(!is_array)
                  {
                     llvm::Type* op1_ty = llvm::IntegerType::get(gepi_base->getContext(), 32);
                     llvm::Constant* op1 = llvm::ConstantInt::get(op1_ty, 0, false);
                     gepi_ops.push_back(op1);
                  }

                  llvm::Type* op2_ty = llvm::IntegerType::get(gepi_base->getContext(), 32);
                  llvm::Constant* op2 = llvm::ConstantInt::get(op2_ty, idx, false);
                  gepi_ops.push_back(op2);

                  std::string new_gepi_name = gepi_name + "." + std::to_string(idx);

                  llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::Create(gepi_type, gepi_base, gepi_ops, new_gepi_name, llvm::dyn_cast<llvm::CallInst>(use->getUser()));
                  gepi_map[gepi_base].push_back(new_gepi);
                  gen_gepi_map(new_gepi, arg, use, gepi_map, arg_map, arg_size_map, new_gepi_name);

                  ++idx;
               }
            }
            else
            {
               // llvm::errs() << "ERR: arg not found\n";
               // arg->dump();
               // exit(-1);
            }
         }
      };
      std::string gepi_name = arg_if_any->getName().str() + ".gepi";

      GenGepiMap::gen_gepi_map(base_address, arg_if_any, use, gepi_map, exp_args_map, arg_size_map, gepi_name);

      const llvm::DataLayout* DL = nullptr;
      if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_address))
      {
         DL = &alloca_inst->getModule()->getDataLayout();
      }
      else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
      {
         DL = &arg->getParent()->getParent()->getDataLayout();
      }
      else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
      {
         DL = &g_var->getParent()->getDataLayout();
      }
      else
      {
         llvm::errs() << "ERR: Neither alloca, argument, nor global as base address\n";
         base_address->dump();
         exit(-1);
      }

      llvm::Value* exp_el = get_element_at_offset(base_address, gepi_map, offset, accessed_size, DL);

      bool some_deletion = false;
      do
      {
         some_deletion = false;
         for(auto& g : gepi_map)
         {
            std::vector<llvm::Value*>& exp = g.second;

            for(auto it = exp.begin(); it != exp.end(); ++it)
            {
               if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(*it))
               {
                  if(gepi != exp_el)
                  {
                     if(gepi->getNumUses() == 0)
                     {
                        gepi->eraseFromParent();
                        exp.erase(it);
                        some_deletion = true;
                        break;
                     }
                  }
               }
            }

            if(some_deletion)
            {
               break;
            }
         }
      } while(some_deletion);

      return exp_el;
   }
   else if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_address))
   {
      const llvm::DataLayout* DL = &alloca_inst->getModule()->getDataLayout();
      return get_element_at_offset(alloca_inst, exp_allocas_map, offset, accessed_size, DL);
   }
   else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
   {
      const llvm::DataLayout* DL = &arg->getParent()->getParent()->getDataLayout();
      return get_element_at_offset(arg, exp_args_map, offset, accessed_size, DL);
   }
   else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
   {
      const llvm::DataLayout* DL = &g_var->getParent()->getDataLayout();
      return get_element_at_offset(g_var, exp_globals_map, offset, accessed_size, DL);
   }
   else
   {
      llvm::errs() << "ERR: Neither alloca, argument, nor global as base address\n";
      base_address->dump();
      exit(-1);
   }
}

void CustomScalarReplacementOfAggregatesPass::expand_ptrs(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions)
{
   inner_functions.insert(inner_functions.begin(), kernel_function);

   for(llvm::Function* f : inner_functions)
   {
      std::vector<llvm::BasicBlock*> bbs;

      for(llvm::BasicBlock& bb : *f)
      {
         bbs.push_back(&bb);
      }

      // for (auto bb_it = bbs.begin(); bb_it != bbs.end(); ++bb_it) {
      for(unsigned long long bb_idx = 0; bb_idx < bbs.size(); ++bb_idx)
      {
         llvm::BasicBlock* bb = bbs.at(bb_idx);

         for(llvm::Instruction& i : *bb)
         {
            llvm::BasicBlock* new_bb = nullptr;
            if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(&i))
            {
               process_pointer(&load_inst->getOperandUse(0), new_bb);
            }
            else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&i))
            {
               process_pointer(&store_inst->getOperandUse(1), new_bb);
            }
            else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               llvm::Function* called_function = call_inst->getCalledFunction();

               // TODO improve this
               if(called_function->isIntrinsic())
               {
                  continue;
               }

               for(unsigned long long op_i = 0; op_i < call_inst->getNumArgOperands(); op_i++)
               {
                  llvm::Use* op_u = &call_inst->getOperandUse(op_i);

                  if(op_u->get()->getType()->isPointerTy())
                  {
                     process_pointer(op_u, new_bb);
                  }
               }
            }

            if(new_bb != nullptr)
            {
               bbs.insert(bbs.begin() + bb_idx + 1, new_bb);
               break;
            }
         }
      }
   }

   inner_functions.erase(inner_functions.begin());
}

bool CustomScalarReplacementOfAggregatesPass::check_assumptions(llvm::Function* kernel_function)
{
   class InstChecker
   {
      static bool check_ptr(llvm::Value* ptr)
      {
         llvm::Value* ptr_rec = ptr;

         do
         {
            if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(ptr_rec))
            {
               ptr_rec = gep_op->getPointerOperand();
            }
            else if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(ptr_rec))
            {
               ptr_rec = bitcast_op->getOperand(0);
            }
            else if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(ptr_rec))
            {
               return true;
            }
            else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(ptr_rec))
            {
               return true;
            }
            else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(ptr_rec))
            {
               return g_var->isInternalLinkage(llvm::GlobalVariable::LinkageTypes::InternalLinkage);
            }
            else
            {
               llvm::errs() << "WAR: ";
               ptr_rec->print(llvm::errs());
               llvm::errs() << " inhibits transfrmation\n";
               return false;
            }
         } while(true);
      }

    public:
      static bool check_inst(llvm::Instruction* inst)
      {
         if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(inst))
         {
            return check_ptr(load_inst->getPointerOperand());
         }
         else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(inst))
         {
            return check_ptr(store_inst->getPointerOperand());
         }
         else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(inst))
         {
            llvm::Function* called_function = call_inst->getCalledFunction();

            // TODO expand it
            if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_start)
            {
               return true;
            }
            if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end)
            {
               return true;
            }

            for(llvm::Value* op : call_inst->arg_operands())
            {
               if(op->getType()->isPointerTy())
               {
                  if(check_ptr(op))
                  {
                     // TODO improve this (if an expandeable arg goes in a "unsupported" function kill anything

                     if(called_function->isIntrinsic())
                     {
                        // TODO replace with if (!called_function->isInternalLinkage(llvm::GlobalVariable::LinkageTypes::InternalLinkage)) {
                        return false;
                     }
                  }
                  else
                  {
                     llvm::errs() << "WAR: Argument ";
                     op->print(llvm::errs());
                     llvm::errs() << " inhibits transfrmation\n";
                     return false;
                  }
               }
            }

            return true;
         }
         else if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(inst))
         {
            return !phi_node->getType()->isPointerTy();
         }
         else
         {
            return true;
         }
      }
   };

   // TODO: how about circular call graphs
   // TODO: how about memOps/intrinsic/extern functions
   std::vector<llvm::CallInst*> call_inst_vec;

   // Initialize the vector containing the calls
   for(llvm::BasicBlock& bb : *kernel_function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(!InstChecker::check_inst(&i))
         {
            llvm::errs() << "WAR: ";
            i.print(llvm::errs());
            llvm::errs() << " inhibits transfrmation\n";
            return false;
         }

         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
         {
            call_inst_vec.push_back(call_inst);
         }
      }
   }

   // Go through the vector (which may grow at any iteration)
   for(unsigned long long idx = 0; idx < call_inst_vec.size(); ++idx)
   {
      llvm::CallInst* call_inst = call_inst_vec.at(idx);
      llvm::Function* called_function = call_inst->getCalledFunction();

      // TODO improve it
      if(called_function->isIntrinsic())
      {
         continue;
      }

      // add to the vector function calls inside the cloned function
      for(llvm::BasicBlock& bb : *called_function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(!InstChecker::check_inst(&i))
            {
               llvm::errs() << "WAR: ";
               i.print(llvm::errs());
               llvm::errs() << " inhibits transfrmation\n";
               return false;
            }

            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               call_inst_vec.push_back(call_inst);
            }
         }
      }
   }

   return true;

} // end check_assumptions(...)

void CustomScalarReplacementOfAggregatesPass::replicate_calls(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions)
{
   // TODO: how about circular call graphs
   // TODO: how about memOps/intrinsic/extern functions
   std::vector<llvm::CallInst*> call_inst_vec;

   // Initialize the vector containing the calls
   for(llvm::BasicBlock& bb : *kernel_function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
         {
            call_inst_vec.push_back(call_inst);
         }
      }
   }

   class FunctionDimKey
   {
    public:
      llvm::Function* const function_ptr = nullptr;
      std::vector<std::vector<unsigned long long>> arg_dims = std::vector<std::vector<unsigned long long>>();

      FunctionDimKey(llvm::Function* const function_ptr, const std::vector<std::vector<unsigned long long>>& arg_dims) : function_ptr(function_ptr), arg_dims(arg_dims)
      {
      }

      bool operator<(const FunctionDimKey& oth) const
      {
         if(function_ptr != oth.function_ptr)
         {
            return function_ptr < oth.function_ptr;
         }
         else
         {
            if(arg_dims.size() != oth.arg_dims.size())
            {
               llvm::errs() << "ERR: different number of arguments for function implementation\n";
               exit(-1);
            }

            auto this_a_it = arg_dims.begin();
            auto oth_a_it = oth.arg_dims.begin();

            for(; this_a_it != arg_dims.end() or oth_a_it != oth.arg_dims.end(); ++this_a_it, ++oth_a_it)
            {
               if(this_a_it->size() != oth_a_it->size())
               {
                  llvm::errs() << "ERR: different number of dims for function implementation\n";
                  exit(-1);
               }

               auto this_d_it = this_a_it->begin();
               auto oth_d_it = oth_a_it->begin();

               for(; this_d_it != this_a_it->end() or oth_d_it != oth_a_it->end(); ++this_d_it, ++oth_d_it)
               {
                  if(*this_d_it != *oth_d_it)
                  {
                     return *this_d_it < *oth_d_it;
                  }
               }
            }

            return false;
         }
      }
   };

   struct CmpFunDim
   {
      bool operator()(const FunctionDimKey& lhs, const FunctionDimKey& rhs) const
      {
         return lhs < rhs;
      }
   };

   std::map<FunctionDimKey, llvm::Function*, CmpFunDim> function_dim_map;

   // Go through the vector (which may grow at any iteration)
   for(unsigned long long idx = 0; idx < call_inst_vec.size(); ++idx)
   {
      llvm::CallInst* call_inst = call_inst_vec.at(idx);
      llvm::Function* called_function = call_inst->getCalledFunction();

      // TODO improve it
      if(called_function->isIntrinsic())
      {
         continue;
      }

      std::vector<std::vector<unsigned long long>> dimensions;

      // Get argument's dimensions
      for(llvm::Value* op : call_inst->arg_operands())
      {
         if(op->getType()->isPointerTy())
         {
            if(llvm::GEPOperator* gep_op_op = llvm::dyn_cast<llvm::GEPOperator>(op))
            {
               if(gep_op_op->getNumIndices() == 1)
               {
                  llvm::Value* ptr_op = gep_op_op->getPointerOperand();

                  if(llvm::Argument* arg_ptr_op = llvm::dyn_cast<llvm::Argument>(ptr_op))
                  {
                     auto arg_size_it = arg_size_map.find(arg_ptr_op);

                     if(arg_size_it != arg_size_map.end())
                     {
                        std::vector<unsigned long long> arg_dims = arg_size_it->second;

                        unsigned long long idx = 0;
                        if(llvm::ConstantInt* c_idx = llvm::dyn_cast<llvm::ConstantInt>(gep_op_op->getOperand(1)))
                        {
                           idx = (unsigned long long)c_idx->getSExtValue();
                        }

                        if(!arg_dims.empty())
                        {
                           unsigned long long new_arg_dims_front = arg_dims.front() - idx;
                           if(new_arg_dims_front > 0)
                           {
                              arg_dims.front() = new_arg_dims_front;
                           }
                           else
                           {
                              llvm::errs() << "ERR: Dim out of range\n";
                              gep_op_op->dump();
                              gep_op_op->getOperand(1);
                              exit(-1);
                           }
                        }
                        else
                        {
                           llvm::errs() << "ERR: empty dims\n";
                           gep_op_op->dump();
                           gep_op_op->getOperand(1);
                           exit(-1);
                        }

                        dimensions.push_back(arg_dims);
                     }
                     else
                     {
                        llvm::errs() << "ERR: Argument dimension not found\n";
                        arg_ptr_op->dump();
                        call_inst->dump();
                        exit(-1);
                     }
                  }
                  else
                  {
                     llvm::errs() << "ERR: Ptr op can be argument only for this gepi\n";
                     gep_op_op->dump();
                     gep_op_op->getOperand(1);
                     exit(-1);
                  }
               }
               else if(gep_op_op->hasIndices())
               {
                  llvm::Value* last_idx = gep_op_op->getOperand(gep_op_op->getNumOperands() - 1);
                  llvm::Value* last_but_one_idx = gep_op_op->getOperand(gep_op_op->getNumOperands() - 2);

                  llvm::Type* last_idx_type = nullptr;
                  llvm::Type* last_but_one_idx_type = nullptr;
                  unsigned long long op_idx = 0;
                  for(llvm::gep_type_iterator gt_it = llvm::gep_type_begin(gep_op_op), GTE = llvm::gep_type_end(gep_op_op); gt_it != GTE; ++gt_it)
                  {
                     if(op_idx == gep_op_op->getNumIndices() - 1)
                     {
                        last_idx_type = gt_it.getIndexedType();
                     }
                     else if(op_idx == gep_op_op->getNumIndices() - 2)
                     {
                        last_but_one_idx_type = gt_it.getIndexedType();
                     }

                     ++op_idx;
                  }

                  llvm::Type* rec_ty = last_but_one_idx_type;
                  std::vector<unsigned long long> gepi_dims;
                  do
                  {
                     if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
                     {
                        gepi_dims.push_back(arr_ty->getArrayNumElements());
                        rec_ty = arr_ty->getArrayElementType();
                     }
                     else
                     {
                        break;
                     }
                  } while(true);

                  unsigned long long idx = 0;
                  if(llvm::ConstantInt* c_idx = llvm::dyn_cast<llvm::ConstantInt>(last_idx))
                  {
                     idx = (unsigned long long)c_idx->getSExtValue();
                  }

                  if(!gepi_dims.empty())
                  {
                     unsigned long long new_arg_dims_front = gepi_dims.front() - idx;
                     if(new_arg_dims_front > 0)
                     {
                        gepi_dims.front() = new_arg_dims_front;
                     }
                     else
                     {
                        llvm::errs() << "ERR: Dim out of range\n";
                        gep_op_op->dump();
                        gep_op_op->getOperand(1);
                        exit(-1);
                     }
                  }
                  else
                  {
                     gepi_dims.push_back(1);
                  }

                  dimensions.push_back(gepi_dims);
               }
               else
               {
                  llvm::errs() << "ERR: Gep op with no indices\n";
                  gep_op_op->dump();
                  exit(-1);
               }
            }
            else if(llvm::Argument* arg_op = llvm::dyn_cast<llvm::Argument>(op))
            {
               auto arg_size_it = arg_size_map.find(arg_op);

               if(arg_size_it != arg_size_map.end())
               {
                  std::vector<unsigned long long> arg_dims = arg_size_it->second;
                  dimensions.push_back(arg_dims);
               }
               else
               {
                  llvm::errs() << "ERR: Argument dimension not found\n";
                  arg_op->dump();
                  call_inst->dump();
                  exit(-1);
               }
            }
            else if(llvm::AllocaInst* alloca_inst_op = llvm::dyn_cast<llvm::AllocaInst>(op))
            {
               llvm::Type* rec_ty = alloca_inst_op->getType()->getPointerElementType();
               std::vector<unsigned long long> alloca_dims;
               do
               {
                  if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
                  {
                     alloca_dims.push_back(arr_ty->getArrayNumElements());
                     rec_ty = arr_ty->getArrayElementType();
                  }
                  else
                  {
                     break;
                  }
               } while(true);

               if(alloca_dims.empty())
               {
                  alloca_dims.push_back(1);
               }
               dimensions.push_back(alloca_dims);
            }
            else if(llvm::GlobalVariable* g_var_op = llvm::dyn_cast<llvm::GlobalVariable>(op))
            {
               llvm::Type* rec_ty = g_var_op->getType()->getPointerElementType();
               std::vector<unsigned long long> g_var_dims;
               do
               {
                  if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
                  {
                     g_var_dims.push_back(arr_ty->getArrayNumElements());
                     rec_ty = arr_ty->getArrayElementType();
                  }
                  else
                  {
                     break;
                  }
               } while(true);

               if(g_var_dims.empty())
               {
                  g_var_dims.push_back(1);
               }
               dimensions.push_back(g_var_dims);
            }
         }
         else
         {
            dimensions.push_back(std::vector<unsigned long long>());
         }
      }

      llvm::Function* synthesized_function = nullptr;

      FunctionDimKey search_key = FunctionDimKey(called_function, dimensions);
      auto fd_it = function_dim_map.find(search_key);
      if(fd_it != function_dim_map.end())
      {
         synthesized_function = fd_it->second;
      }
      else
      {
         // Clone the function
         llvm::ValueToValueMapTy VMap;
         llvm::ClonedCodeInfo* code_info = new llvm::ClonedCodeInfo();
         llvm::Function* cloned_function = llvm::CloneFunction(called_function, VMap, code_info);
         std::string new_name = called_function->getName().str() + "_";
         for(auto d1 : dimensions)
         {
            for(auto d2 : d1)
            {
               new_name.append(std::to_string(d2) + ".");
            }
            new_name.append(".");
         }

         cloned_function->setName(new_name);

         synthesized_function = cloned_function;

         for(auto& arg : synthesized_function->args())
         {
            arg_size_map.insert(std::make_pair(&arg, dimensions.at(arg.getArgNo())));
         }

         function_dim_map.insert(std::make_pair(FunctionDimKey(called_function, dimensions), synthesized_function));

         inner_functions.push_back(synthesized_function);
      }

      // Replace the call site
      call_inst->setCalledFunction(synthesized_function);

      // add to the vector function calls inside the cloned function
      for(llvm::BasicBlock& bb : *synthesized_function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               call_inst_vec.push_back(call_inst);
            }
         }
      }
   }

} // end replicate_calls(...)

void CustomScalarReplacementOfAggregatesPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
   AU.addRequiredTransitive<llvm::ScalarEvolutionWrapperPass>();
}

void CustomScalarReplacementOfAggregatesPass::expand_allocas(llvm::Function* function)
{
   // Global alloca vec so to avoid loops in analysis
   std::set<llvm::AllocaInst*> alloca_vec = std::set<llvm::AllocaInst*>();

   while(true)
   {
      // Alloca vec for the current iteration
      std::set<llvm::AllocaInst*> i_alloca_vec = std::set<llvm::AllocaInst*>();

      // Go through all the instructions looking for allocas
      for(auto& bb : *function)
      {
         for(auto& i : bb)
         {
            if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&i))
            {
               // Whether allocating struct or array
               if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(alloca_inst->getAllocatedType()))
               {
                  // Not analyzed yet
                  if(alloca_vec.count(alloca_inst) == 0)
                  {
                     i_alloca_vec.insert(alloca_inst);
                     alloca_vec.insert(alloca_inst);
                  }
               }
               else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(alloca_inst->getAllocatedType()))
               {
                  if(alloca_vec.count(alloca_inst) == 0)
                  {
                     i_alloca_vec.insert(alloca_inst);
                     alloca_vec.insert(alloca_inst);
                  }
               }
            }
         }
      }

      // End if empty worklist
      if(i_alloca_vec.size() == 0)
      {
         break;
      }

      // Go through the worklist
      for(auto& a : i_alloca_vec)
      {
         if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(a))
         {
            if(expansion_allowed(alloca_inst))
            {
               alloca_to_remove[alloca_inst->getFunction()].insert(alloca_inst);

               if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(alloca_inst->getAllocatedType()))
               {
                  for(unsigned idx = 0; idx < str_ty->getStructNumElements(); ++idx)
                  {
                     llvm::Type* element = str_ty->getStructElementType(idx);

                     llvm::Type* new_alloca_type = llvm::PointerType::getUnqual(element);
                     std::string new_alloca_name = alloca_inst->getName().str() + "." + std::to_string(idx);
                     llvm::AllocaInst* new_alloca_inst = new llvm::AllocaInst(/*new_alloca_type*/ element,
#if __clang_major__ > 4
                                                                           DL->getAllocaAddrSpace(),
#endif
                                                                           new_alloca_name, alloca_inst);

                     exp_allocas_map[alloca_inst].push_back(new_alloca_inst);
                  }
               }
               else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(alloca_inst->getAllocatedType()))
               {
                  for(unsigned idx = 0; idx < arr_ty->getArrayNumElements(); ++idx)
                  {
                     llvm::Type* element_ty = arr_ty->getArrayElementType();

                     llvm::Type* new_alloca_type = llvm::PointerType::getUnqual(element_ty);
                     std::string new_alloca_name = alloca_inst->getName().str() + "." + std::to_string(idx);
                     llvm::AllocaInst* new_alloca_inst = new llvm::AllocaInst(/*new_alloca_type*/ element_ty,
#if __clang_major__ > 4
                                                                           DL->getAllocaAddrSpace(),
#endif
                                                                           new_alloca_name, alloca_inst);

                     exp_allocas_map[alloca_inst].push_back(new_alloca_inst);
                  }
               }
            }
         }
      }
   }
}

void CustomScalarReplacementOfAggregatesPass::expand_globals(std::set<llvm::GlobalVariable*> accessed_variables)
{
   // Global vector containing globals to be expanded yet
   std::vector<llvm::GlobalVariable*> globals_to_exp = std::vector<llvm::GlobalVariable*>();

   for(auto g_it = accessed_variables.begin(); g_it != accessed_variables.end(); g_it++)
   {
      globals_to_exp.push_back(*g_it);
   }

   for(unsigned long long g_idx = 0; g_idx < globals_to_exp.size(); g_idx++)
   {
      llvm::GlobalVariable* g_var = globals_to_exp.at(g_idx);

      if(expansion_allowed(g_var))
      {
         if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(g_var->getType()->getPointerElementType()))
         {
            for(unsigned e_idx = 0; e_idx < str_ty->getStructNumElements(); ++e_idx)
            {
               llvm::Type* element = str_ty->getStructElementType(e_idx);

               std::string new_global_name = g_var->getName().str() + "." + std::to_string(e_idx);

               llvm::Constant* initializer = nullptr;
               if(llvm::Constant* c_el = llvm::dyn_cast<llvm::Constant>(g_var->getInitializer()->getAggregateElement(e_idx)))
               {
                  initializer = c_el;
               }
               llvm::GlobalVariable* new_g_var = new llvm::GlobalVariable(*g_var->getParent(), element, false, g_var->getLinkage(), initializer, new_global_name, g_var);

               new_g_var->copyAttributesFrom(g_var);

               exp_globals_map[g_var].push_back(new_g_var);
               globals_to_exp.insert(globals_to_exp.begin() + g_idx + 1, new_g_var);
            }
         }
         else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(g_var->getType()->getPointerElementType()))
         {
            for(unsigned e_idx = 0; e_idx < arr_ty->getArrayNumElements(); ++e_idx)
            {
               llvm::Type* element = arr_ty->getArrayElementType();

               std::string new_global_name = g_var->getName().str() + "." + std::to_string(e_idx);

               llvm::Constant* initializer = nullptr;
               if(llvm::Constant* c_el = llvm::dyn_cast<llvm::Constant>(g_var->getInitializer()->getAggregateElement(e_idx)))
               {
                  initializer = c_el;
               }
               llvm::GlobalVariable* new_g_var = new llvm::GlobalVariable(*g_var->getParent(), element, false, g_var->getLinkage(), initializer, new_global_name, g_var);
               new_g_var->copyAttributesFrom(g_var);

               exp_globals_map[g_var].push_back(new_g_var);
               globals_to_exp.insert(globals_to_exp.begin() + g_idx + 1, new_g_var);
            }
         }
      }
   }
}

void CustomScalarReplacementOfAggregatesPass::expand_signatures_and_call_sites(std::vector<llvm::Function*>& inner_functions, std::map<llvm::Function*, llvm::Function*>& exp_fun_map,
                                                                               std::map<llvm::Function*, std::set<unsigned long long>>& exp_idx_args_map,
                                                                               // std::map<llvm::Argument *, std::vector<llvm::Argument *>> &exp_args_map,
                                                                               llvm::Function* kernel_function)
{
   // Loop through the inner functions:
   //  - recursively expanding the signatures
   //  - adapting the call sites those are called in
   for(llvm::Function* called_function : inner_functions)
   {
      class ExpArgs
      {
       public:
         static void rec(llvm::Argument* arg, std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map_ref, std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_size_map_ref)
         {
            if(llvm::PointerType* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg->getType()))
            {
               auto a_it = arg_size_map_ref.find(arg);
               if(a_it != arg_size_map_ref.end() and !a_it->second.empty())
               {
                  unsigned long long elements = a_it->second.at(0);

                  for(int e_idx = 0; e_idx < elements; ++e_idx)
                  {
                     if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(ptr_ty->getElementType()))
                     {
                        std::string new_arg_name = arg->getName().str() + "." + std::to_string(e_idx);

                        llvm::Type* new_arg_ty = llvm::PointerType::getUnqual(arr_ty->getArrayElementType());
                        llvm::Argument* new_arg = new llvm::Argument(new_arg_ty, new_arg_name, arg->getParent());

                        if(a_it->second.size() > 1)
                        {
                           arg_size_map_ref[new_arg] = std::vector<unsigned long long>(a_it->second.begin() + 1, a_it->second.end());
                        }
                        else
                        {
                           arg_size_map_ref[new_arg] = std::vector<unsigned long long>(1, 1);
                        }

                        exp_args_map_ref[arg].push_back(new_arg);

                        rec(new_arg, exp_args_map_ref, arg_size_map_ref);
                     }
                     else
                     {
                        llvm::Type* new_arg_ty = llvm::PointerType::getUnqual(ptr_ty->getElementType());
                        std::string new_arg_name = arg->getName().str() + "." + std::to_string(e_idx);
                        llvm::Argument* new_arg = new llvm::Argument(new_arg_ty, new_arg_name, arg->getParent());

                        exp_args_map_ref[arg].push_back(new_arg);

                        rec(new_arg, exp_args_map_ref, arg_size_map_ref);
                     }
                  }
               }
               else if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(ptr_ty->getElementType()))
               {
                  for(unsigned long long e_idx = 0; e_idx != str_ty->getStructNumElements(); ++e_idx)
                  {
                     llvm::Type* new_arg_ty = nullptr;
                     if(str_ty->getStructElementType(e_idx)->isArrayTy())
                     {
                        new_arg_ty = llvm::PointerType::getUnqual(str_ty->getStructElementType(e_idx)->getArrayElementType());
                     }
                     else
                     {
                        new_arg_ty = llvm::PointerType::getUnqual(str_ty->getStructElementType(e_idx));
                     }

                     std::string new_arg_name = arg->getName().str() + "." + std::to_string(e_idx);
                     llvm::Argument* new_arg = new llvm::Argument(new_arg_ty, new_arg_name, arg->getParent());

                     if(str_ty->getStructElementType(e_idx)->isArrayTy())
                     {
                        llvm::Type* rec_ty = str_ty->getStructElementType(e_idx);

                        std::vector<unsigned long long> tmp_sizes;
                        do
                        {
                           if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
                           {
                              tmp_sizes.push_back(arr_ty->getArrayNumElements());
                              rec_ty = arr_ty->getArrayElementType();
                           }
                           else
                           {
                              break;
                           }
                        } while(true);

                        arg_size_map_ref[new_arg] = tmp_sizes;
                     }

                     exp_args_map_ref[arg].push_back(new_arg);

                     rec(new_arg, exp_args_map_ref, arg_size_map_ref);
                  }
               }
               else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(ptr_ty->getElementType()))
               {
                  for(unsigned long long e_idx = 0; e_idx != arr_ty->getArrayNumElements(); ++e_idx)
                  {
                     llvm::Type* new_arg_ty = llvm::PointerType::getUnqual(arr_ty->getArrayElementType());
                     std::string new_arg_name = arg->getName().str() + "." + std::to_string(e_idx);
                     llvm::Argument* new_arg = new llvm::Argument(new_arg_ty, new_arg_name, arg->getParent());

                     exp_args_map_ref[arg].push_back(new_arg);

                     llvm::Type* rec_ty = arr_ty;

                     std::vector<unsigned long long> tmp_sizes;
                     do
                     {
                        if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
                        {
                           tmp_sizes.push_back(arr_ty->getArrayNumElements());
                           rec_ty = arr_ty->getArrayElementType();
                        }
                        else
                        {
                           break;
                        }
                     } while(true);

                     arg_size_map_ref[new_arg] = tmp_sizes;

                     rec(new_arg, exp_args_map_ref, arg_size_map_ref);
                  }
               }
            }
         }
      };

      std::vector<llvm::Type*> new_arg_ty_vec = std::vector<llvm::Type*>();

      // Keep the same return type
      llvm::Type* new_mock_return_type = called_function->getFunctionType()->getReturnType();

      llvm::FunctionType* new_mock_function_type = llvm::FunctionType::get(new_mock_return_type, false);
      llvm::GlobalValue::LinkageTypes mock_linkage = called_function->getLinkage();
      std::string new_mock_function_name = called_function->getName().str() + ".csroa.mock";

      // Create function prototype
      llvm::Function* new_mock_function = llvm::Function::Create(new_mock_function_type, mock_linkage, new_mock_function_name, called_function->getParent());

      llvm::ValueToValueMapTy mock_VMap;

      std::map<unsigned long long, llvm::Argument*> idxs_of_exp_args;
      std::map<llvm::Argument*, std::vector<llvm::Argument*>> mock_exp_args_map;

      llvm::Function::arg_iterator arg_it_b = called_function->arg_begin();
      llvm::Function::arg_iterator arg_it_e = called_function->arg_end();

      // Go through all the function arguments
      for(auto arg_it = arg_it_b; arg_it != arg_it_e; arg_it++)
      {
         llvm::Argument* arg = &*arg_it;

         if(mock_VMap.count(arg) == 0 or true)
         {
            llvm::Type* new_arg_ty = arg->getType();
            std::string new_arg_name = arg->getName();

            // Create the new argument and append it to the mock function
            llvm::Argument* new_arg = new llvm::Argument(new_arg_ty, new_arg_name, new_mock_function);

            // Assign the size to the new argument if the related argument had it
            auto a_it = arg_size_map.find(arg);
            if(a_it != arg_size_map.end())
            {
               arg_size_map[new_arg] = a_it->second;
            }

#if __clang_major__ == 4
            llvm::Argument* lastArg = &new_mock_function->getArgumentList().back();
#else
            llvm::Argument* lastArg=nullptr;
            for (auto& arg : new_mock_function->args())
            {
               lastArg = &arg;
            }
#endif
            mock_VMap[arg] = lastArg;

            idxs_of_exp_args[new_arg->getArgNo()] = arg;

            if(!expansion_allowed(arg))
            {
               continue;
            }

            ExpArgs::rec(new_arg, mock_exp_args_map, arg_size_map);
         }
      }

      for(auto& a : new_mock_function->args())
      {
         new_arg_ty_vec.push_back(a.getType());
      }

      // Keep the same return type
      llvm::Type* new_return_type = called_function->getFunctionType()->getReturnType();

      llvm::FunctionType* new_function_type = llvm::FunctionType::get(new_return_type, new_arg_ty_vec, false);
      llvm::GlobalValue::LinkageTypes linkage = called_function->getLinkage();
      std::string new_function_name = called_function->getName().str() + ".csroa";

      // Create function prototype
      llvm::Function* new_function = llvm::Function::Create(new_function_type, linkage, new_function_name, called_function->getParent());

      llvm::ValueToValueMapTy VMap;

      std::map<llvm::Argument*, llvm::Argument*> mock_to_new_arg_map;
      llvm::Function::arg_iterator mf_arg_it_b = new_mock_function->arg_begin();
      llvm::Function::arg_iterator mf_arg_it_e = new_mock_function->arg_end();
      llvm::Function::arg_iterator nf_arg_it_b = new_function->arg_begin();
      llvm::Function::arg_iterator nf_arg_it_e = new_function->arg_end();
      llvm::Function::arg_iterator nf_arg_it = nf_arg_it_b;
      llvm::Function::arg_iterator mf_arg_it = mf_arg_it_b;
      for(; nf_arg_it != nf_arg_it_e; nf_arg_it++, mf_arg_it++)
      {
         llvm::Argument* nf_arg = &*nf_arg_it;
         llvm::Argument* mf_arg = &*mf_arg_it;

         mock_to_new_arg_map[mf_arg] = nf_arg;

         if(idxs_of_exp_args.count(nf_arg->getArgNo()) != 0)
         {
            VMap[idxs_of_exp_args[nf_arg->getArgNo()]] = nf_arg;
         }

         nf_arg->setName(mf_arg->getName());

         // Assign the size to the new argument if the mock argument had it
         auto a_it = arg_size_map.find(mf_arg);
         if(a_it != arg_size_map.end())
         {
            arg_size_map.insert(std::make_pair(nf_arg, a_it->second));
         }
      }

      for(auto& ma1 : mock_exp_args_map)
      {
         llvm::Argument* mf1_arg = ma1.first;
         llvm::Argument* nf1_arg = mock_to_new_arg_map[mf1_arg];

         for(auto& ma2 : ma1.second)
         {
            llvm::Argument* mf2_arg = ma2;
            llvm::Argument* nf2_arg = mock_to_new_arg_map[mf2_arg];

            exp_args_map[nf1_arg].push_back(nf2_arg);
         }
      }

      // Clone the function
      llvm::SmallVector<llvm::ReturnInst*, 8> returns;
      llvm::ClonedCodeInfo* codeInfo = nullptr;
      llvm::CloneFunctionInto(new_function, called_function, VMap, true, returns, "", codeInfo);

      // Track the function mapping (old->new)
      exp_fun_map[called_function] = new_function;

      for(auto& mock_arg : new_mock_function->args())
      {
         arg_size_map.erase(&mock_arg);
      }
      new_mock_function->eraseFromParent();

      // Do not preserve any analysis
      llvm::PreservedAnalyses::none();

      // Class used to recursively expand operands in call sites
      class op_rec
      {
       public:
         static void rec(llvm::Argument* arg, std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map_ref, std::vector<llvm::Value*>& ops, bool is_called_operand = false)
         {
            if(llvm::PointerType* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg->getType()))
            {
               if(!is_called_operand)
               {
                  ops.push_back(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(ptr_ty->getElementType())));
               }

               auto exp_arg_it = exp_args_map_ref.find(arg);

               if(exp_arg_it != exp_args_map_ref.end())
               {
                  std::vector<llvm::Argument*>& exp_args_vec_ref = exp_arg_it->second;

                  for(auto& a : exp_args_vec_ref)
                  {
                     rec(a, exp_args_map_ref, ops);
                  }
               }
            }
         }
      };

      // Expand the (uses) call sites
      for(auto user_it = called_function->user_begin(); user_it != called_function->user_end(); user_it++)
      {
         llvm::User* user = *user_it;

         // If the uses is a call instruction
         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(user))
         {
            // Expanded inner functions as a set
            std::set<llvm::Function*> inner_functions_exp;

            // Populate the set
            for(auto& i_f : inner_functions)
            {
               inner_functions_exp.insert(i_f);
            }

            // If call site of an expanded function or kernel function
            if(inner_functions_exp.count(call_inst->getCalledFunction()) != 0 or call_inst->getFunction() == kernel_function)
            {
               // Recursively populate the operand vector, expanding with null pointers
               std::vector<llvm::Value*> new_call_ops = std::vector<llvm::Value*>();
               llvm::Function::arg_iterator arg_it = new_function->arg_begin();
               for(auto& op : call_inst->arg_operands())
               {
                  llvm::Value* operand = op.get();

                  llvm::Argument* arg = nullptr;
                  {
                     llvm::Function::arg_iterator arg_it = new_function->arg_begin();
                     for(int i = 0; i < new_call_ops.size(); i++)
                     {
                        arg_it++;
                     }
                     arg = &*arg_it;
                  }

                  new_call_ops.push_back(operand);

                  op_rec::rec(arg, exp_args_map, new_call_ops, true);
               }

               // Build the new call site
               std::string new_call_name = call_inst->getName().str() + ".csroa";
               llvm::CallInst* new_call_inst = llvm::CallInst::Create(new_function, new_call_ops, (call_inst->hasName() ? new_call_name : ""), call_inst);

               // Replace the old one
               new_call_inst->takeName(call_inst);
               call_inst->replaceAllUsesWith(new_call_inst);
               call_inst->eraseFromParent();
            }
         }
      }
   }

   // Update inner functions with the expanded ones
   unsigned long long inner_size = inner_functions.size();
   for(auto f : inner_functions)
   {
      inner_functions.push_back(exp_fun_map[f]);
   }
   inner_functions.erase(inner_functions.begin(), inner_functions.begin() + inner_size);

   // Expand allocas in kernel and inner functions
   expand_allocas(kernel_function);
   for(llvm::Function* function : inner_functions)
   {
      expand_allocas(function);
   }

} // end expand_signatures_and_call_sites

void CustomScalarReplacementOfAggregatesPass::cleanup(std::map<llvm::Function*, std::set<unsigned long long>>& exp_idx_args_map, std::map<llvm::Function*, llvm::Function*>& exp_fun_map, std::vector<llvm::Function*>& inner_functions)
{
   for(auto& i : inst_to_remove)
   {
      i->replaceAllUsesWith(llvm::UndefValue::get(i->getType()));
      i->eraseFromParent();
   }

   class CheckArg
   {
    private:
      // Recursively check whether the argument is recursively used by calls only
      static bool usedByArgsOnly(llvm::Argument* arg, std::set<llvm::Argument*> inspected_args)
      {
         if(inspected_args.count(arg) == 0)
         {
            inspected_args.insert(arg);

            for(auto& use : arg->uses())
            {
               if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(use.getUser()))
               {
                  llvm::Function::arg_iterator arg_it = call_inst->getCalledFunction()->arg_begin();

                  for(unsigned long long i = 0; i < use.getOperandNo(); i++)
                  {
                     arg_it++;
                  }

                  if(!usedByArgsOnly(&*arg_it, inspected_args))
                  {
                     return false;
                  }
               }
               else
               {
                  return false;
               }
            }
         }

         return true;
      }

      // Recursively check whether the argument is recursively loaded only
      static bool loadedOnly(llvm::Argument* arg, std::set<llvm::Argument*> inspected_args)
      {
         if(inspected_args.count(arg) == 0)
         {
            inspected_args.insert(arg);

            if(llvm::PointerType* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg->getType()))
            {
               if(!ptr_ty->getElementType()->isAggregateType())
               {
                  for(auto& use : arg->uses())
                  {
                     if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(use.getUser()))
                     {
                        // nothing to do
                     }
                     else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(use.getUser()))
                     {
                        llvm::Function::arg_iterator arg_it = call_inst->getCalledFunction()->arg_begin();

                        for(unsigned long long i = 0; i < use.getOperandNo(); i++)
                        {
                           arg_it++;
                        }

                        if(!loadedOnly(&*arg_it, inspected_args))
                        {
                           return false;
                        }
                     }
                     else
                     {
                        return false;
                     }
                  }
               }
               else
               {
                  return false;
               }
            }
            else
            {
               return false;
            }
         }

         return true;
      }

    public:
      static bool usedByArgsOnly_wrapper(llvm::Argument* arg)
      {
         return usedByArgsOnly(arg, std::set<llvm::Argument*>());
      }

      static bool loadedOnly_wrapper(llvm::Argument* arg)
      {
         return loadedOnly(arg, std::set<llvm::Argument*>());
      }
   };

   struct sate_cmp
   {
      bool operator()(const std::pair<llvm::CallInst*, unsigned long long>& lhs, const std::pair<llvm::CallInst*, unsigned long long>& rhs) const
      {
         if(lhs.first < rhs.first)
         {
            return true;
         }
         else if(lhs.first == rhs.first)
         {
            return lhs.second < rhs.second;
         }
         else
         {
            return false;
         }
      }
   };
   std::map<std::pair<llvm::CallInst*, unsigned long long>, llvm::Argument*, sate_cmp> arg_to_arg;

   for(llvm::Function* function : inner_functions)
   {
      // Create the new function type based on the recomputed parameters.
      std::vector<llvm::Type*> arg_tys;
      for(auto& a : function->args())
      {
         // If it is not an expanded argument
         if(exp_idx_args_map[function].count(a.getArgNo()) == 0)
         {
            if(exp_args_map.count(&a) == 0)
            {
               // If it is not only uesd as argument recursively
               if(!CheckArg::usedByArgsOnly_wrapper(&a))
               {
                  // Check whether recursively loaded only
                  if(CheckArg::loadedOnly_wrapper(&a))
                  {
                     arg_tys.push_back(a.getType()->getPointerElementType());
                  }
                  else
                  {
                     arg_tys.push_back(a.getType());
                  }
               }
            }
         }
      }

      llvm::FunctionType* new_fun_ty = llvm::FunctionType::get(function->getReturnType(), arg_tys, function->isVarArg());

      std::string new_fun_name = function->getName().str() + ".clean";
      llvm::Function* new_function = llvm::Function::Create(new_fun_ty, function->getLinkage(), new_fun_name, function->getParent());
      new_function->copyAttributesFrom(function);
      new_function->setComdat(function->getComdat());

      new_function->getBasicBlockList().splice(new_function->begin(), function->getBasicBlockList());

      std::set<llvm::Argument*> unused_args;
      std::set<llvm::Argument*> scalar_args;

      unsigned int i = 0;
      for(llvm::Function::arg_iterator fun_arg_it = function->arg_begin(), new_fun_arg_it = new_function->arg_begin(); fun_arg_it != function->arg_end(); ++fun_arg_it, ++i)
      {
         // If it is not an expanded argument
         if(exp_idx_args_map[function].count(fun_arg_it->getArgNo()) == 0)
         {
            if(exp_args_map.count(&*fun_arg_it) == 0)
            {
               // If it is not only uesd as argument recursively
               if(!CheckArg::usedByArgsOnly_wrapper(&*fun_arg_it))
               {
                  // Check whether recursively loaded only
                  if(CheckArg::loadedOnly_wrapper(&*fun_arg_it))
                  {
                     for(auto& use : fun_arg_it->uses())
                     {
                        if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(use.getUser()))
                        {
                           load_inst->replaceAllUsesWith(&*new_fun_arg_it);
                           load_inst->eraseFromParent();
                        }
                        else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(use.getUser()))
                        {
                           arg_to_arg.insert(std::make_pair(std::make_pair(call_inst, use.getOperandNo()), &*new_fun_arg_it));
                        }
                        else
                        {
                           llvm::errs() << "ERR\n";
                           exit(-1);
                        }
                     }

                     scalar_args.insert(&*fun_arg_it);
                     fun_arg_it->replaceAllUsesWith(llvm::UndefValue::get(fun_arg_it->getType()));
                  }
                  else
                  {
                     fun_arg_it->replaceAllUsesWith(&*new_fun_arg_it);
                  }
                  new_fun_arg_it->takeName(&*fun_arg_it);
                  ++new_fun_arg_it;
               }
               else
               {
                  unused_args.insert(&*fun_arg_it);
                  fun_arg_it->replaceAllUsesWith(llvm::Constant::getNullValue(fun_arg_it->getType()));
               }
            }
            else
            {
               fun_arg_it->replaceAllUsesWith(llvm::Constant::getNullValue(fun_arg_it->getType()));
            }
         }
         else
         {
            fun_arg_it->replaceAllUsesWith(llvm::Constant::getNullValue(fun_arg_it->getType()));
         }
      }

      for(auto user_it = function->user_begin(); user_it != function->user_end(); user_it++)
      {
         llvm::User* user = *user_it;

         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(user))
         {
            std::vector<llvm::Value*> call_ops = std::vector<llvm::Value*>();
            for(auto& op : call_inst->arg_operands())
            {
               llvm::Value* operand = op.get();

               llvm::Function::arg_iterator arg_it = call_inst->getCalledFunction()->arg_begin();

               for(auto i = 0; i < op.getOperandNo(); i++)
               {
                  arg_it++;
               }

               // If it is not an expanded argument
               if(exp_idx_args_map[function].count(arg_it->getArgNo()) == 0)
               {
                  if(exp_args_map.count(&*arg_it) == 0)
                  {
                     if(unused_args.count(&*arg_it) == 0)
                     {
                        if(scalar_args.count(&*arg_it) == 0)
                        {
                           call_ops.push_back(operand);
                        }
                        else
                        {
                           auto to_exp_it = arg_to_arg.find(std::make_pair(call_inst, arg_it->getArgNo()));

                           if(to_exp_it == arg_to_arg.end())
                           {
                              std::string new_load_name = call_inst->getName().str() + ".load." + std::to_string(i);
                              llvm::LoadInst* load_inst = new llvm::LoadInst(operand, new_load_name, call_inst);

                              call_ops.push_back(load_inst);
                           }
                           else
                           {
                              call_ops.push_back(to_exp_it->second);
                           }
                        }
                     }
                  }
               }
            }

            std::string new_call_name = call_inst->getName().str() + ".clean";
            llvm::CallInst* new_call_inst = llvm::CallInst::Create(new_function, call_ops, (call_inst->hasName() ? new_call_name : ""), call_inst);

            call_inst->replaceAllUsesWith(new_call_inst);
            call_inst->eraseFromParent();
         }
         else
         {
            llvm::errs() << "Funsction use should be call\n";
            exit(-1);
         }
      }

      function->eraseFromParent();
   }

   // Delete all the functions which has been expanded in something else and have no uses
   for(auto exp_it = exp_fun_map.begin(); exp_it != exp_fun_map.end(); ++exp_it)
   {
      llvm::Function* function = exp_it->first;

      if(function->getNumUses() == 0)
      {
         function->eraseFromParent();
      }
   }

   for(auto glob_it = exp_globals_map.begin(); glob_it != exp_globals_map.end(); ++glob_it)
   {
      std::vector<llvm::GlobalVariable*> exp_g_var_vec = glob_it->second;

      for(llvm::GlobalVariable* exp_g_var : exp_g_var_vec)
      {
         if(!exp_g_var->getType()->isAggregateType())
         {
            if(exp_g_var->getInitializer() != nullptr)
            {
               bool only_load_inst_users = true;

               for(auto& u : exp_g_var->uses())
               {
                  if(llvm::LoadInst* load_isnt = llvm::dyn_cast<llvm::LoadInst>(u.getUser()))
                  {
                     if(load_isnt->getPointerOperandIndex() != u.getOperandNo())
                     {
                        only_load_inst_users = false;
                        break;
                     }
                  }
                  else
                  {
                     only_load_inst_users = false;
                     break;
                  }
               }

               if(only_load_inst_users)
               {
                  for(auto& u : exp_g_var->uses())
                  {
                     for(auto& u : exp_g_var->uses())
                     {
                        if(llvm::LoadInst* load_isnt = llvm::dyn_cast<llvm::LoadInst>(u.getUser()))
                        {
                           load_isnt->replaceAllUsesWith(exp_g_var->getInitializer());
                           load_isnt->eraseFromParent();
                        }
                        else
                        {
                           llvm::errs() << "ERR: Non load use\n";
                           exit(-1);
                        }
                     }
                  }
               }
            }
         }
         else
         {
            if(exp_g_var->getNumUses() == 0)
            {
               exp_g_var->eraseFromParent();
            }
         }
      }
   }

} // end cleanup

void CustomScalarReplacementOfAggregatesPass::spot_accessed_globals(llvm::Function* kernel_function, std::vector<llvm::Function*>& inner_functions, std::set<llvm::GlobalVariable*>& accessed_globals)
{
   inner_functions.insert(inner_functions.begin(), kernel_function);

   for(llvm::Function* function : inner_functions)
   {
      for(llvm::BasicBlock& bb : *function)
      {
         for(llvm::Instruction& i : bb)
         {
            std::vector<llvm::Value*> ptr_vec;

            if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(&i))
            {
               ptr_vec.push_back(load_inst->getPointerOperand());
            }
            else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&i))
            {
               ptr_vec.push_back(store_inst->getPointerOperand());
            }
            else if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               for(auto& op : call_inst->arg_operands())
               {
                  if(op.get()->getType()->isPointerTy())
                  {
                     ptr_vec.push_back(op.get());
                  }
               }
            }

            for(llvm::Value* ptr : ptr_vec)
            {
               llvm::Value* ptr_rec = ptr;

               do
               {
                  if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(ptr_rec))
                  {
                     ptr_rec = gep_op->getPointerOperand();
                  }
                  else if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(ptr_rec))
                  {
                     ptr_rec = bitcast_op->getOperand(0);
                  }
                  else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(ptr_rec))
                  {
                     accessed_globals.insert(g_var);
                     break;
                  }
                  else
                  {
                     break;
                  }
               } while(true);
            }
         }
      }
   }

   inner_functions.erase(inner_functions.begin());
}

bool CustomScalarReplacementOfAggregatesPass::expansion_allowed(llvm::Value* aggregate)
{
   if(aggregate->getType()->isPointerTy())
   {
      auto arg_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(aggregate));
      if(aggregate->getType()->getPointerElementType()->isAggregateType() or (llvm::isa<llvm::GlobalValue>(aggregate) && llvm::dyn_cast<llvm::GlobalValue>(aggregate)->getValueType()->isAggregateType()) or (arg_it != arg_size_map.end() and !arg_it->second.empty() and arg_it->second.front() > 1))
      {
         std::vector<llvm::Type*> contained_types;

         llvm::Type* ptr_ty = llvm::isa<llvm::GlobalValue>(aggregate) ? llvm::dyn_cast<llvm::GlobalValue>(aggregate)->getValueType() : aggregate->getType()->getPointerElementType();
         if(arg_it != arg_size_map.end() and !arg_it->second.empty() and arg_it->second.front() > 1)
         {
            for(unsigned long long e_idx = 0; e_idx < arg_it->second.front(); e_idx++)
            {
               contained_types.push_back(ptr_ty);
            }
         }
         else
         {
            contained_types.push_back(ptr_ty);
         }

         unsigned long long non_aggregates_types = 0;

         for(auto c_idx = 0u; c_idx < contained_types.size(); c_idx++)
         {
            llvm::Type* el_ty = contained_types.at(c_idx);

            if(el_ty->isAggregateType())
            {
               if(el_ty->isStructTy())
               {
                  for(unsigned int e_idx = 0; e_idx < el_ty->getStructNumElements(); ++e_idx)
                  {
                     contained_types.push_back(el_ty->getStructElementType(e_idx));
                  }
               }
               else if(el_ty->isArrayTy())
               {
                  for(unsigned long long e_idx = 0; e_idx < el_ty->getArrayNumElements(); ++e_idx)
                  {
                     contained_types.push_back(el_ty->getArrayElementType());
                  }
               }
            }
            else
            {
               ++non_aggregates_types;
            }
         }

         const llvm::DataLayout* DL = nullptr;
         if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(aggregate))
         {
            DL = &arg->getParent()->getParent()->getDataLayout();
         }
         else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(aggregate))
         {
            DL = &g_var->getParent()->getDataLayout();
         }
         else if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(aggregate))
         {
            DL = &alloca_inst->getModule()->getDataLayout();
         }

          unsigned long long allocated_size = DL->getTypeAllocSize(ptr_ty);

         auto size_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(aggregate));
         if(size_it != arg_size_map.end() and !size_it->second.empty())
         {
            // non_aggregates_types *= size_it->second.front();
            allocated_size *= size_it->second.front();
         }

          return non_aggregates_types <= MaxNumScalarTypes and allocated_size <= MaxTypeByteSize;
      }
      else
      {
         llvm::errs()<<"2)Expansion not allowed for ";
         aggregate->print(llvm::errs());
         llvm::errs()<<"\n";
         return false;
      }
   }
   else
   {
      llvm::errs()<<"3)Expansion not allowed for ";
      aggregate->print(llvm::errs());
      llvm::errs()<<"\n";
      return false;
   }
}

CustomScalarReplacementOfAggregatesPass* createCustomScalarReplacementOfAggregatesPass(std::string kernel_name)
{
   return new CustomScalarReplacementOfAggregatesPass(kernel_name);
}

namespace llvm
{
   struct CLANG_VERSION_SYMBOL(_plugin_CSROA);
}

namespace llvm
{
   cl::opt<std::string> TopFunctionName_CSROAP("panda-KN", cl::desc("Specify the name of the top function"), cl::value_desc("name of the top function"));
   struct CLANG_VERSION_SYMBOL(_plugin_CSROA) : public CustomScalarReplacementOfAggregatesPass
   {
    public:
      static char ID;
      CLANG_VERSION_SYMBOL(_plugin_CSROA)() : CustomScalarReplacementOfAggregatesPass(TopFunctionName_CSROAP, ID)
      {
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }

      bool runOnModule(Module& M) override
      {
         return CustomScalarReplacementOfAggregatesPass::runOnModule(M);
      }
      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_CSROA);
      }
   };

   char CLANG_VERSION_SYMBOL(_plugin_CSROA)::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_CSROA)> XPass(CLANG_VERSION_STRING(_plugin_CSROA), "Custom SROA", false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(llvm::createSimpleLoopUnrollPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_CSROA)());
}
#if ADD_RSP
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_CSROA_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

#endif

#ifdef _WIN32
using namespace llvm;

INITIALIZE_PASS_BEGIN(clang7_plugin_CSROA, "clang7_plugin_CSROA", "Custom SROA", false, false)
INITIALIZE_PASS_END(clang7_plugin_CSROA, "clang7_plugin_CSROA", "Custom SROA", false, false)
namespace llvm
{
   void clang7_plugin_CSROA_init()
   {
   }
} // namespace llvm
#endif
