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
 *              Copyright (C) 2019 Politecnico di Milano
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
//
// Created by Marco Siracusa on 7/27/19.
//

#include "GepiCanonicalizationPass.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/Operator.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>

llvm::PHINode* get_last_phi(llvm::BasicBlock* bb)
{
   llvm::PHINode* last_phi = nullptr;

   for(llvm::Instruction& instruction : *bb)
   {
      if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(&instruction))
      {
         last_phi = phi_node;
      }
      else
      {
         break;
      }
   }

   return last_phi;
}

struct ChunkCopy
{
   llvm::LoadInst* load_inst;
   llvm::BitCastOperator* load_bitcast_op;
   llvm::Value* loaded_ptr;
   llvm::StoreInst* store_inst;
   llvm::BitCastOperator* store_bitcast_op;
   llvm::Value* stored_ptr;
   llvm::Type* src_ty;
   llvm::Type* dest_ty;

   ChunkCopy(llvm::LoadInst* load_inst, llvm::BitCastOperator* load_bitcast_op, llvm::Value* loaded_ptr, llvm::StoreInst* store_inst, llvm::BitCastOperator* store_bitcast_op, llvm::Value* stored_ptr, llvm::Type* src_ty, llvm::Type* dest_ty)
       : load_inst(load_inst), load_bitcast_op(load_bitcast_op), loaded_ptr(loaded_ptr), store_inst(store_inst), store_bitcast_op(store_bitcast_op), stored_ptr(stored_ptr), src_ty(src_ty), dest_ty(dest_ty)
   {
   }
};

struct ChunkInit
{
   llvm::StoreInst* store_inst;
   llvm::BitCastOperator* store_bitcast_op;
   llvm::Value* stored_ptr;
   llvm::ConstantInt* stored_value;
   llvm::Type* src_ty;
   llvm::Type* dest_ty;

   ChunkInit(llvm::StoreInst* store_inst, llvm::BitCastOperator* store_bitcast_op, llvm::Value* stored_ptr, llvm::ConstantInt* stored_value, llvm::Type* src_ty, llvm::Type* dest_ty)
       : store_inst(store_inst), store_bitcast_op(store_bitcast_op), stored_ptr(stored_ptr), stored_value(stored_value), src_ty(src_ty), dest_ty(dest_ty)
   {
   }
};

void recursive_copy_lowering(llvm::Type* type, std::vector<unsigned long long> gepi_idxs, llvm::Value* load_ptr, llvm::Value* store_ptr, llvm::Instruction* load_inst, llvm::Instruction* store_inst, unsigned long long fitting)
{
   if(fitting > 1)
   {
      gepi_idxs.push_back(0);
      for(unsigned long long idx = 0; idx < fitting; ++idx)
      {
         gepi_idxs.back() = idx;
         recursive_copy_lowering(type, gepi_idxs, load_ptr, store_ptr, load_inst, store_inst, 1);
      }
   }
   else if(type->isAggregateType())
   {
      gepi_idxs.push_back(0);

      if(type->isStructTy())
      {
         for(unsigned long long idx = 0; idx < type->getStructNumElements(); ++idx)
         {
            gepi_idxs.back() = idx;
            recursive_copy_lowering(type->getStructElementType(idx), gepi_idxs, load_ptr, store_ptr, load_inst, store_inst, 1);
         }
      }
      else if(type->isArrayTy())
      {
         for(unsigned long long idx = 0; idx < type->getArrayNumElements(); ++idx)
         {
            gepi_idxs.back() = idx;
            recursive_copy_lowering(type->getArrayElementType(), gepi_idxs, load_ptr, store_ptr, load_inst, store_inst, 1);
         }
      }
      else
      {
         llvm::errs() << "ERR: wrong type\n";
         exit(-1);
      }
   }
   else
   {
      std::string gepi_name = "ccgepi";
      std::vector<llvm::Value*> gepi_value_idxs;
      for(unsigned long long idx : gepi_idxs)
      {
         gepi_name += "." + std::to_string(idx);
         gepi_value_idxs.push_back(llvm::ConstantInt::get(llvm::IntegerType::get(load_ptr->getContext(), 32), idx));
      }
      llvm::GetElementPtrInst* load_gep_inst = llvm::GetElementPtrInst::CreateInBounds(nullptr, load_ptr, gepi_value_idxs, gepi_name, load_inst);
      llvm::GetElementPtrInst* store_gep_inst = llvm::GetElementPtrInst::CreateInBounds(nullptr, store_ptr, gepi_value_idxs, gepi_name, store_inst);

      llvm::LoadInst* lowered_load = new llvm::LoadInst(load_gep_inst, "ccload." + gepi_name, load_inst);
      llvm::StoreInst* lowered_store = new llvm::StoreInst(lowered_load, store_gep_inst, store_inst);

      llvm::dbgs() << "Lowered load gepi: ";
      load_gep_inst->print(llvm::dbgs(), false);
      llvm::dbgs() << "Lowered load inst: ";
      lowered_load->print(llvm::dbgs(), false);
      llvm::dbgs() << "Lowered store gepi: ";
      store_gep_inst->print(llvm::dbgs(), false);
      llvm::dbgs() << "Lowered store inst: ";
      lowered_store->print(llvm::dbgs(), false);
   }
}

void recursive_init_lowering(llvm::Type* type, llvm::ConstantInt* init_value, std::vector<unsigned long long> gepi_idxs, llvm::Value* store_ptr, llvm::StoreInst* store_inst, unsigned long long lo_bit, unsigned long long hi_bit)
{
   if(type->isAggregateType())
   {
      gepi_idxs.push_back(0);

      const llvm::DataLayout& DL = store_inst->getModule()->getDataLayout();

      if(type->isStructTy())
      {
         for(unsigned long long idx = 0; idx < type->getStructNumElements(); ++idx)
         {
            gepi_idxs.back() = idx;

            const llvm::StructLayout* struct_layout = DL.getStructLayout(llvm::dyn_cast<llvm::StructType>(type));

            const llvm::APInt& ap_val = init_value->getValue();

            unsigned long long el_lo_bit = lo_bit + struct_layout->getElementOffset(idx) * 8;
            unsigned long long el_hi_bit = (idx + 1 == type->getStructNumElements() ? hi_bit : lo_bit + struct_layout->getElementOffset(idx + 1) * 8);

            llvm::APInt ap_mask = llvm::APInt::getBitsSet(ap_val.getBitWidth(), el_lo_bit, el_hi_bit);
            llvm::APInt ap_el_val = ap_val & ap_mask;
            llvm::ConstantInt* el_init_value = llvm::dyn_cast<llvm::ConstantInt>(llvm::ConstantInt::get(init_value->getType(), ap_el_val));

            recursive_init_lowering(type->getStructElementType(idx), el_init_value, gepi_idxs, store_ptr, store_inst, el_lo_bit, el_hi_bit);
         }
      }
      else if(type->isArrayTy())
      {
         unsigned long long el_bitwidth = DL.getTypeSizeInBits(type->getArrayElementType());
         unsigned long long el_offset = lo_bit;

         for(unsigned long long idx = 0; idx < type->getArrayNumElements(); ++idx)
         {
            gepi_idxs.back() = idx;

            const llvm::APInt& ap_val = init_value->getValue();

            unsigned long long el_lo_bit = el_offset;
            el_offset += el_bitwidth;
            unsigned long long el_hi_bit = el_offset;

            llvm::APInt ap_mask = llvm::APInt::getBitsSet(ap_val.getBitWidth(), el_lo_bit, el_hi_bit);
            llvm::APInt ap_el_val = ap_val & ap_mask;
            llvm::ConstantInt* el_init_value = llvm::dyn_cast<llvm::ConstantInt>(llvm::ConstantInt::get(init_value->getType(), ap_el_val));

            recursive_init_lowering(type->getArrayElementType(), el_init_value, gepi_idxs, store_ptr, store_inst, el_lo_bit, el_hi_bit);
         }
      }
      else
      {
         llvm::errs() << "ERR: wrong type\n";
         exit(-1);
      }
   }
   else
   {
      std::string gepi_name = "ccgepi";
      std::vector<llvm::Value*> gepi_value_idxs;
      for(unsigned long long idx : gepi_idxs)
      {
         gepi_name += "." + std::to_string(idx);
         gepi_value_idxs.push_back(llvm::ConstantInt::get(llvm::IntegerType::get(store_ptr->getContext(), 32), idx));
      }

      llvm::GetElementPtrInst* store_gep_inst = llvm::GetElementPtrInst::CreateInBounds(nullptr, store_ptr, gepi_value_idxs, gepi_name, store_inst);

      llvm::APInt cast_ap_int = llvm::APInt(store_gep_inst->getType()->getPointerElementType()->getIntegerBitWidth(), init_value->getValue().getSExtValue());
      llvm::Constant* cast_init_val = llvm::ConstantInt::get(store_gep_inst->getType()->getPointerElementType(), cast_ap_int);
      llvm::StoreInst* lowered_store = new llvm::StoreInst(cast_init_val, store_gep_inst, store_inst);
   }
}

void lower_chunk_copy(const ChunkCopy& chunk_copy, const llvm::DataLayout& DL)
{
   llvm::dbgs() << "INFO: Lowered chunk copy\n";
   llvm::dbgs() << "          Load bitcast:  ";
   chunk_copy.load_bitcast_op->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Load inst:     ";
   chunk_copy.load_inst->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Store bitcast: ";
   chunk_copy.store_bitcast_op->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Store inst:    ";
   chunk_copy.store_inst->print(llvm::dbgs(), false);

   double fitting = (double)DL.getTypeAllocSize(chunk_copy.dest_ty->getPointerElementType()) / (double)DL.getTypeAllocSize(chunk_copy.src_ty->getPointerElementType());
   std::vector<unsigned long long> gepi_idxs = std::vector<unsigned long long>();
   if(fitting == 1)
   {
      gepi_idxs.push_back(0);
   }
   recursive_copy_lowering(chunk_copy.src_ty->getPointerElementType(), gepi_idxs, chunk_copy.loaded_ptr, chunk_copy.stored_ptr, chunk_copy.load_inst, chunk_copy.store_inst, fitting);

   chunk_copy.store_inst->eraseFromParent();
   if(llvm::BitCastInst* store_bitcast = llvm::dyn_cast<llvm::BitCastInst>(chunk_copy.store_bitcast_op))
   {
      if(store_bitcast->getNumUses() == 0)
      {
         store_bitcast->eraseFromParent();
      }
   }
   if(chunk_copy.load_inst->getNumUses() == 0)
   {
      chunk_copy.load_inst->eraseFromParent();
   }
   if(llvm::BitCastInst* load_bitcast = llvm::dyn_cast<llvm::BitCastInst>(chunk_copy.load_bitcast_op))
   {
      if(load_bitcast->getNumUses() == 0)
      {
         load_bitcast->eraseFromParent();
      }
   }
}

void lower_chunk_init(const ChunkInit& chunk_init)
{
   llvm::dbgs() << "INFO: Lowered chunk init\n";
   llvm::dbgs() << "          Store bitcast: ";
   chunk_init.store_bitcast_op->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Store inst:    ";
   chunk_init.store_bitcast_op->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Stored val:    ";
   chunk_init.stored_value->print(llvm::dbgs(), false);
   llvm::dbgs() << "          Stored ptr:    ";
   chunk_init.stored_ptr->print(llvm::dbgs(), false);

   const llvm::DataLayout& DL = chunk_init.store_inst->getModule()->getDataLayout();
   unsigned long long lo_bit = 0;
   unsigned long long hi_bit = DL.getTypeSizeInBits(chunk_init.src_ty->getPointerElementType());
   recursive_init_lowering(chunk_init.src_ty->getPointerElementType(), chunk_init.stored_value, std::vector<unsigned long long>(1, 0), chunk_init.stored_ptr, chunk_init.store_inst, lo_bit, hi_bit);

   chunk_init.store_inst->eraseFromParent();
   if(llvm::BitCastInst* store_bitcast = llvm::dyn_cast<llvm::BitCastInst>(chunk_init.store_bitcast_op))
   {
      if(store_bitcast->getNumUses() == 0)
      {
         store_bitcast->eraseFromParent();
      }
   }
}

void iterator_canonicalization(llvm::Use& iter_use,        /// the gepi or cmp instructions along the path
                               llvm::Value* ptr_iter_init, /// pointer iterator initialization
                               llvm::Value* int_ind_var,   /// integer iterator
                               llvm::PHINode* first_phi_node, llvm::PHINode* new_phi_node, std::map<llvm::CmpInst*, std::vector<llvm::Use*>>& encountered_cmps, std::set<llvm::PHINode*>& encountered_phis, std::set<llvm::Instruction*>& inst_to_remove,
                               llvm::LoopInfo& LI, const llvm::Loop* loop)
{
   llvm::User* user = iter_use.getUser();
   unsigned long op_num = iter_use.getOperandNo();

   if(llvm::GetElementPtrInst* gepi_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(user))
   {
      std::vector<llvm::Value*> gepi_idx_vec(std::next(gepi_inst->op_begin(), 1), gepi_inst->op_end());
      llvm::Value* first_idx = gepi_idx_vec.at(0);

      llvm::Value* new_idx = int_ind_var;
      if(!llvm::isa<llvm::ConstantInt>(first_idx) or llvm::dyn_cast<llvm::ConstantInt>(first_idx)->getSExtValue() != 0)
      {
         llvm::Value* trunc_first_idx = first_idx;
         if(llvm::ConstantInt* const_first_idx = llvm::dyn_cast<llvm::ConstantInt>(trunc_first_idx))
         {
            trunc_first_idx = llvm::ConstantInt::getSigned(int_ind_var->getType(), const_first_idx->getSExtValue());
         }
         else
         {
            if(int_ind_var->getType() != first_idx->getType())
            {
               trunc_first_idx = llvm::CastInst::Create(llvm::CastInst::Trunc, trunc_first_idx, int_ind_var->getType(), ".trunc", gepi_inst);
            }
         }

         std::string add_inst_name = gepi_inst->getName().str() + ".add";
         new_idx = llvm::BinaryOperator::Create(llvm::Instruction::Add, int_ind_var, trunc_first_idx, add_inst_name, gepi_inst);
      }

      std::vector<llvm::Value*> new_gepi_idx_vec = gepi_idx_vec;
      /*
      if (new_gepi_idx_vec.at(0)->getType() != add_inst->getType()) {
         new_gepi_idx_vec.at(0)->getType()->dump();
         add_inst->getType()->dump();
         add_inst = llvm::SExtInst::Create(llvm::CastInst::SExt, add_inst, new_gepi_idx_vec.at(0)->getType(), add_inst_name + ".sext", gepi_inst);
      }
       */
      new_gepi_idx_vec.at(0) = new_idx;
      std::string new_gepi_name = gepi_inst->getName().str() + ".gepi";
      // llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(init_ptr->getType()->getScalarType())->getElementType();
      llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, ptr_iter_init, new_gepi_idx_vec, new_gepi_name, gepi_inst);

      llvm::dbgs() << "INFO: Expanding GEPI: ";
      gepi_inst->print(llvm::dbgs(), false);
      llvm::dbgs() << "      With new GEPI: ";
      new_gepi->print(llvm::dbgs(), false);
      llvm::dbgs() << "      Having index: ";
      new_idx->print(llvm::dbgs(), false);

      gepi_inst->replaceAllUsesWith(new_gepi);

      for(llvm::Use& use : new_gepi->uses())
      {
         llvm::dbgs() << "      Replaced use #" << use.getOperandNo() << " in user: ";
         use.getUser()->print(llvm::dbgs(), false);
         iterator_canonicalization(use, ptr_iter_init, new_idx, first_phi_node, new_phi_node, encountered_cmps, encountered_phis, inst_to_remove, LI, loop);
      }

      inst_to_remove.insert(gepi_inst);
      return;
   }
   else if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(user))
   {
      if(phi_node == first_phi_node)
      {
         new_phi_node->setOperand(op_num, int_ind_var);
         llvm::dbgs() << "      Setting init PHI: ";
         new_phi_node->print(llvm::dbgs(), false);
         return;
      }
      else
      {
         if(LI.getLoopFor(phi_node->getParent()) != loop)
         {
            return;
         }

         llvm::dbgs() << "      Expanding conditional PHI: ";
         new_phi_node->print(llvm::dbgs(), false);
         llvm::PHINode* new_phi = llvm::PHINode::Create(int_ind_var->getType(), 2, phi_node->getName().str() + ".phi", phi_node);
         if(phi_node->getIncomingValue(0) == ptr_iter_init)
         {
            new_phi->addIncoming(llvm::ConstantInt::get(int_ind_var->getType(), 0, true), phi_node->getIncomingBlock(0));
            new_phi->addIncoming(int_ind_var, phi_node->getIncomingBlock(1));
         }
         else
         {
            new_phi->addIncoming(int_ind_var, phi_node->getIncomingBlock(0));
            new_phi->addIncoming(llvm::ConstantInt::get(int_ind_var->getType(), 0, true), phi_node->getIncomingBlock(1));
         }

         inst_to_remove.insert(phi_node);

         std::vector<llvm::Value*> new_gepi_idx_vec;
         new_gepi_idx_vec.push_back(new_phi);

         std::string new_gepi_name = new_phi->getName().str() + ".phigepi";
         // llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(init_ptr->getType()->getScalarType())->getElementType();
         llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, ptr_iter_init, new_gepi_idx_vec, new_gepi_name, phi_node->getParent()->getFirstNonPHI());
         phi_node->replaceAllUsesWith(new_gepi);

         for(llvm::Use& use : new_gepi->uses())
         {
            llvm::dbgs() << "      Replaced use# " << use.getOperandNo() << " in user: ";
            use.getUser()->print(llvm::dbgs(), false);
            iterator_canonicalization(use, ptr_iter_init, new_phi, first_phi_node, new_phi_node, encountered_cmps, encountered_phis, inst_to_remove, LI, loop);
         }
         return;
      }
   }
   else if(llvm::CmpInst* cmp_inst = llvm::dyn_cast<llvm::CmpInst>(user))
   {
      encountered_cmps[cmp_inst].push_back(&iter_use);
      return;
   }
   /*
      llvm::Instruction *inst_before = llvm::dyn_cast<llvm::Instruction>(user);
      if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(user)) {
         if (phi_node == first_phi_node) {
            new_phi_node->setOperand(op_num, int_ind_var);
            llvm::dbgs() << "Seti init PHI: "; new_phi_node->dump();
            return;
         }
      }
   */
   return;
}

bool ptr_iterator_simplification(llvm::Function& function, llvm::LoopInfo& LI)
{
   std::set<llvm::Instruction*> inst_to_remove;
   std::map<llvm::CmpInst*, std::vector<llvm::Use*>> encountered_cmps;
   std::set<llvm::PHINode*> encountered_phis;
   unsigned long long transformation_count = 0;

   for(const llvm::Loop* loop : LI)
   {
      llvm::BasicBlock* header = loop->getHeader();

      std::vector<llvm::PHINode*> one_op_phi_vec;
      std::vector<llvm::PHINode*> two_op_phi_vec;

      for(llvm::BasicBlock* bb : loop->blocks())
      {
         for(llvm::Instruction& instruction : *bb)
         {
            if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(&instruction))
            {
               if(phi_node->getType()->isPointerTy())
               {
                  if(!phi_node->getType()->getPointerElementType()->isAggregateType())
                  {
                     switch(phi_node->getNumOperands())
                     {
                        case 1:
                           one_op_phi_vec.push_back(phi_node);
                           break;
                        case 2:
                           two_op_phi_vec.push_back(phi_node);
                           break;
                     }
                  }
               }
            }
            else
            {
               break;
            }
         }
      }

      for(llvm::PHINode* phi_node : two_op_phi_vec)
      {
         std::vector<llvm::GetElementPtrInst*> gepi_vector;

         llvm::Value* init_ptr = nullptr;  /// The initialization of the pointer iterator
         llvm::Value* other_ptr = nullptr; /// The other value of the phi node

         llvm::CmpInst* cmp_inst = nullptr; /// The cmp inst in case the pointer is the indvar
         llvm::Value* base_ptr = nullptr;
         llvm::Value* init_val = nullptr;
         llvm::Value* stop_val = nullptr;

         bool income_0_in_loop = false;
         {
            llvm::Value* income_val = phi_node->getIncomingValue(0);
            if(llvm::Instruction* income_inst = llvm::dyn_cast<llvm::Instruction>(income_val))
            {
               llvm::Loop* income_loop = LI.getLoopFor(income_inst->getParent());
               income_0_in_loop = income_loop == loop;
            }
            else
            {
               while(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(income_val))
               {
                  income_val = gep_op->getPointerOperand();
               }

               if(llvm::Instruction* income_inst = llvm::dyn_cast<llvm::Instruction>(income_val))
               {
                  llvm::Loop* income_loop = LI.getLoopFor(income_inst->getParent());
                  income_0_in_loop = income_loop == loop;
               }
            }
         }
         bool income_1_in_loop = false;
         {
            llvm::Value* income_val = phi_node->getIncomingValue(1);
            if(llvm::Instruction* income_inst = llvm::dyn_cast<llvm::Instruction>(income_val))
            {
               llvm::Loop* income_loop = LI.getLoopFor(income_inst->getParent());
               income_1_in_loop = income_loop == loop;
            }
            else
            {
               while(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(income_val))
               {
                  income_val = gep_op->getPointerOperand();
               }

               if(llvm::Instruction* income_inst = llvm::dyn_cast<llvm::Instruction>(income_val))
               {
                  llvm::Loop* income_loop = LI.getLoopFor(income_inst->getParent());
                  income_1_in_loop = income_loop == loop;
               }
            }
         }

         // llvm::GetElementPtrInst *indvar_gepi = nullptr;
         if(income_0_in_loop xor income_1_in_loop)
         {
            llvm::Type* idx_ty = llvm::Type::getInt32Ty(phi_node->getContext());

            std::string new_phi_node_name = phi_node->getName().str() + ".phi";
            llvm::PHINode* new_phi_node = llvm::PHINode::Create(idx_ty, 2, new_phi_node_name, phi_node);

            if(income_0_in_loop)
            {
               // indvar_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(income_0);
               other_ptr = phi_node->getIncomingValue(0);
               init_ptr = phi_node->getIncomingValue(1);
               new_phi_node->addIncoming(llvm::ConstantInt::get(idx_ty, 0, true), phi_node->getIncomingBlock(0)); // It shouldnt be null, will be replaced
               new_phi_node->addIncoming(llvm::ConstantInt::get(idx_ty, 0, true), phi_node->getIncomingBlock(1));
            }
            else
            {
               // indvar_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(income_1);
               init_ptr = phi_node->getIncomingValue(0);
               other_ptr = phi_node->getIncomingValue(1);
               new_phi_node->addIncoming(llvm::ConstantInt::get(idx_ty, 0, true), phi_node->getIncomingBlock(0));
               new_phi_node->addIncoming(llvm::ConstantInt::get(idx_ty, 0, true), phi_node->getIncomingBlock(1)); // It shouldnt be null, will be replaced
            }

            {
               llvm::Value* iter_rec = other_ptr;

               do
               {
                  if(llvm::GetElementPtrInst* gep_step = llvm::dyn_cast<llvm::GetElementPtrInst>(iter_rec))
                  {
                     iter_rec = gep_step->getPointerOperand();
                  }
                  else if(llvm::PHINode* phi_step = llvm::dyn_cast<llvm::PHINode>(iter_rec))
                  {
                     if(phi_step != phi_node and phi_step->getIncomingValue(0) == init_ptr)
                     {
                        iter_rec = phi_step->getIncomingValue(1);
                     }
                     else
                     {
                        break;
                     }
                  }
                  else
                  {
                     break;
                  }
               } while(true);

               if(iter_rec != phi_node)
               {
                  continue;
               }

               /*
                              signed long gepi_offset = 0;
                              bool all_constant_idxs = true;
                              for (auto gep_it = gep_vec.rbegin(); gep_it != gep_vec.rend(); ++gep_it) {
                                 llvm::GEPOperator *gep_op = *gep_it;
                                 unsigned int bw = function.getParent()->getDataLayout().getPointerSizeInBits(gep_op->getPointerAddressSpace());
                                 llvm::APInt ap_offset = llvm::APInt(bw, 0);
                                 gep_op->accumulateConstantOffset(function.getParent()->getDataLayout(), ap_offset);
                                 signed long offset_byte = (signed long)ap_offset.getSExtValue() / (signed long)(function.getParent()->getDataLayout().getTypeSizeInBits(phi_node->getType()->getPointerElementType()) / 8);
                                 gepi_offset += offset_byte;
                                 all_constant_idxs = all_constant_idxs and gep_op->hasAllConstantIndices();

                                 gep_op->dump();
                                 llvm::dbgs() << "AP: " << gepi_offset << "\n";
                              }
               */
            }

            std::vector<llvm::Value*> gepi_idx_vec;
            /*
            if (new_gepi_idx_vec.at(0)->getType() != add_inst->getType()) {
               new_gepi_idx_vec.at(0)->getType()->dump();
               add_inst->getType()->dump();
               add_inst = llvm::SExtInst::Create(llvm::CastInst::SExt, add_inst, new_gepi_idx_vec.at(0)->getType(), add_inst_name + ".sext", gepi_inst);
            }
            */
            gepi_idx_vec.push_back(llvm::ConstantInt::get(idx_ty, 0, true));
            std::string new_gepi_name = phi_node->getName().str() + ".firstgepi";
            // llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(init_ptr->getType()->getScalarType())->getElementType();
            llvm::GetElementPtrInst* first_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, phi_node, gepi_idx_vec, new_gepi_name, phi_node->getParent()->getFirstNonPHI());

            llvm::dbgs() << "INFO: Expanding PHI: ";
            phi_node->print(llvm::dbgs(), false);
            llvm::dbgs() << "      With PHI: ";
            new_phi_node->print(llvm::dbgs(), false);
            llvm::dbgs() << "      Init ptr: ";
            init_ptr->print(llvm::dbgs(), false);
            llvm::dbgs() << "      First GEPI: ";
            first_gepi->print(llvm::dbgs(), false);

            phi_node->replaceAllUsesWith(first_gepi);
            first_gepi->setOperand(0, phi_node);

            for(llvm::Use& use : phi_node->uses())
            {
               iterator_canonicalization(use, init_ptr, new_phi_node, phi_node, new_phi_node, encountered_cmps, encountered_phis, inst_to_remove, LI, loop);
            }

            inst_to_remove.insert(phi_node);
            inst_to_remove.insert(first_gepi);
            transformation_count++;
         }
      }
   }

   for(auto& cmp_it : encountered_cmps)
   {
      llvm::CmpInst* cmp_inst = cmp_it.first;
      std::vector<llvm::Use*>& use_vec = cmp_it.second;

      if(cmp_inst->getOperand(0)->getType()->isPointerTy())
      {
         llvm::Value* op_0 = cmp_inst->getOperand(0);
         llvm::Value* op_1 = cmp_inst->getOperand(1);

         llvm::GetElementPtrInst* gepi_0 = llvm::dyn_cast<llvm::GetElementPtrInst>(op_0);
         llvm::GetElementPtrInst* gepi_1 = llvm::dyn_cast<llvm::GetElementPtrInst>(op_1);

         if(gepi_0 and gepi_1)
         {
            if(gepi_0->getPointerOperand() == gepi_1->getPointerOperand())
            {
               if(gepi_0->getNumIndices() == 1 and gepi_1->getNumIndices() == 1)
               {
                  llvm::CmpInst* new_cmp_inst = llvm::CmpInst::Create(llvm::CmpInst::OtherOps::ICmp, cmp_inst->getPredicate(), gepi_0->getOperand(1), gepi_1->getOperand(1), cmp_inst->getName().str() + ".lowered", cmp_inst);
                  cmp_inst->replaceAllUsesWith(new_cmp_inst);
                  inst_to_remove.insert(cmp_inst);
                  transformation_count++;
               }
            }
         }
      }
   }

   unsigned long num_deletion = 0;
   std::set<llvm::Instruction*> remove_set = inst_to_remove;
   do
   {
      num_deletion = 0;
      std::set<llvm::Instruction*> removed_set;

      for(llvm::Instruction* inst_to_erase : remove_set)
      {
         if(!inst_to_erase->hasNUsesOrMore(1))
         {
            removed_set.insert(inst_to_erase);
         }
      }

      for(llvm::Instruction* inst_to_erase : removed_set)
      {
         remove_set.erase(inst_to_erase);
         inst_to_erase->eraseFromParent();
      }

      num_deletion = removed_set.size();
   } while(num_deletion > 0);

   for(llvm::Instruction* inst : inst_to_remove)
   {
      // inst->eraseFromParent();
   }
   /*
      for(llvm::PHINode* phi_node : two_op_phi_vec)
      {
         llvm::GetElementPtrInst* ind_var_gepi = nullptr;
         llvm::Value* init_ptr = nullptr;
         llvm::Value* stop_ptr = nullptr;
         llvm::CmpInst* cmp_inst = nullptr;
         llvm::Value* base_ptr = nullptr;
         llvm::Value* init_val = nullptr;
         llvm::Value* stop_val = nullptr;


         for(unsigned short idx = 0; idx < 2; ++idx)
         {
            llvm::Value* incoming_value = phi_node->getIncomingValue(idx);

            std::vector<llvm::GetElementPtrInst *> gepi_array;

            if (phi_node->hasOneUse()) {
               llvm::Use *next_use = &*phi_node->use_begin();
               while (llvm::isa<llvm::GetElementPtrInst>(next_use->getUser()) and
                       llvm::dyn_cast<llvm::GetElementPtrInst>(next_use->getUser())->getPointerOperand() == next_use->get() and
                       next_use->getUser() != phi_node) {

                  gepi_array.push_back(llvm::dyn_cast<llvm::GetElementPtrInst>(next_use->getUser()));
                  if (next_use->getUser()->hasOneUse()) {
                     next_use = &*next_use->getUser()->use_begin();
                  }
               }
            }

            for (auto g : gepi_array) {
               g->dump();
            }

            exit(-1);




            if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(incoming_value))
            {
               if(gepi->getPointerOperand() == phi_node)
               {
                  if(ind_var_gepi == nullptr and init_ptr == nullptr)
                  {
                     switch(idx)
                     {
                        case 0:
                           ind_var_gepi = gepi;
                           init_ptr = phi_node->getIncomingValue(1);
                           break;
                        case 1:
                           ind_var_gepi = gepi;
                           init_ptr = phi_node->getIncomingValue(0);
                           break;
                        default:
                           exit(-1);
                     }
                  }
                  else
                  {
                     ind_var_gepi = nullptr;
                     init_ptr = nullptr;
                     break;
                  }
               }
            }
         }

         if(ind_var_gepi == nullptr or init_ptr == nullptr)
         {
            llvm::dbgs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find indvar/init):\n";
            llvm::dbgs() << "   Phi node: ";
            phi_node->dump();
            continue;
         }

         if(ind_var_gepi->getNumIndices() != 1 or !ind_var_gepi->hasOneUse())
         {
            llvm::dbgs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find indvar):\n";
            llvm::dbgs() << "   Phi node: ";
            phi_node->dump();
            ind_var_gepi = nullptr;
            continue;
         }

         for(llvm::Use& u : phi_node->uses())
         {
            if(llvm::CmpInst* cmp_inst_use = llvm::dyn_cast<llvm::CmpInst>(u.getUser()))
            {
               if(cmp_inst == nullptr)
               {
                  cmp_inst = cmp_inst_use;

                  if(u.getOperandNo() == 0)
                  {
                     stop_ptr = cmp_inst->getOperand(1);
                  }
                  else
                  {
                     stop_ptr = cmp_inst->getOperand(0);
                  }
               }
               else
               {
                  cmp_inst = nullptr;
                  stop_ptr = nullptr;
                  break;
               }
            }
         }

         if(cmp_inst == nullptr or stop_ptr == nullptr)
         {
            llvm::dbgs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find cmp/stop):\n";
            llvm::dbgs() << "   Phi node: ";
            phi_node->dump();
            continue;
         }

         if(llvm::GEPOperator* stop_ptr_gep_op = llvm::dyn_cast<llvm::GEPOperator>(stop_ptr))
         {
            if(stop_ptr_gep_op->getNumIndices() == 1)
            {
               base_ptr = stop_ptr_gep_op->getPointerOperand();
               stop_val = stop_ptr_gep_op->getOperand(1);

               if(init_ptr == base_ptr)
               {
                  init_val = llvm::ConstantInt::get(stop_val->getType(), 0);
               }
               else if(llvm::GEPOperator* init_ptr_gep_op = llvm::dyn_cast<llvm::GEPOperator>(init_ptr))
               {
                  if(init_ptr_gep_op->getNumIndices() == 1)
                  {
                     if(init_ptr_gep_op->getPointerOperand() == base_ptr)
                     {
                        init_val = init_ptr_gep_op->getOperand(1);
                     }
                     else
                     {
                        stop_val = nullptr;
                        base_ptr = nullptr;
                     }
                  }
                  else
                  {
                     stop_val = nullptr;
                     base_ptr = nullptr;
                  }
               }
               else
               {
                  stop_val = nullptr;
                  base_ptr = nullptr;
               }
            }
         }

         if(init_val == nullptr or stop_val == nullptr or base_ptr == nullptr)
         {
            llvm::dbgs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find init/stop/base):\n";
            llvm::dbgs() << "   Phi node: ";
            phi_node->dump();
            continue;
         }

         if(ind_var_gepi != nullptr and cmp_inst != nullptr and init_ptr != nullptr and stop_ptr != nullptr and init_val != nullptr and stop_val != nullptr and base_ptr != nullptr)
         {
            llvm::dbgs() << "INFO: In function " << function.getName().str() << " Canonicalizing 2op pointer iterator:\n";
            llvm::dbgs() << "   Phi node: ";
            phi_node->dump();
            llvm::dbgs() << "     Ind var gepi: ";
            ind_var_gepi->dump();
            llvm::dbgs() << "     Cmp inst: ";
            cmp_inst->dump();
            llvm::dbgs() << "     Init ptr: ";
            init_ptr->dump();
            llvm::dbgs() << "     Stop ptr: ";
            stop_ptr->dump();
            llvm::dbgs() << "     Base ptr: ";
            base_ptr->dump();
            llvm::dbgs() << "     Init val: ";
            init_val->dump();
            llvm::dbgs() << "     Stop val: ";
            stop_val->dump();
            llvm::Value* gepi_index = ind_var_gepi->getOperand(1);
            std::string new_phi_node_name = phi_node->getName().str() + ".phi";
            llvm::PHINode* new_phi_node = llvm::PHINode::Create(gepi_index->getType(), 2, new_phi_node_name, phi_node);

            for(unsigned short idx = 0; idx < 2; ++idx)
            {
               if(phi_node->getIncomingValue(idx) == init_ptr)
               {
                  new_phi_node->addIncoming(init_val, phi_node->getIncomingBlock(idx));
               }
               else if(phi_node->getIncomingValue(idx) == ind_var_gepi)
               {
                  std::string add_inst_name = ind_var_gepi->getName().str() + ".add";
                  llvm::BinaryOperator* add_inst = llvm::BinaryOperator::Create(llvm::Instruction::Add, new_phi_node, gepi_index, add_inst_name, ind_var_gepi);
                  new_phi_node->addIncoming(add_inst, phi_node->getIncomingBlock(idx));

                  llvm::dbgs() << "   New add: ";
                  add_inst->dump();
               }
               else
               {
                  exit(-1);
               }
            }

            llvm::dbgs() << "   New phi node: ";
            new_phi_node->dump();

            std::vector<llvm::Value*> idx_vec;
            if(!llvm::isa<llvm::Argument>(init_ptr))
            {
               if(init_ptr->getType()->isAggregateType())
               {
                  llvm::Constant* zero_cint = llvm::ConstantInt::get(gepi_index->getType(), 0, false);
                  idx_vec.push_back(zero_cint);
               }
            }
            idx_vec.push_back(new_phi_node);

            std::string new_gepi_name = phi_node->getName().str() + ".gepi";
            // llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(init_ptr->getType()->getScalarType())->getElementType();
            llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, base_ptr, idx_vec, new_gepi_name);
            new_gepi->insertAfter(get_last_phi(phi_node->getParent()));

            phi_node->replaceAllUsesWith(new_gepi);
            phi_node->eraseFromParent();
            ind_var_gepi->eraseFromParent();

            llvm::dbgs() << "   New gepi: ";
            new_gepi->dump();

            cmp_inst->setOperand(0, new_phi_node);
            cmp_inst->setOperand(1, stop_val);

            llvm::dbgs() << "   New cmp: ";
            cmp_inst->dump();

            ++transformation_count;
         }
      }

      for(llvm::PHINode* phi_node : one_op_phi_vec)
      {
         llvm::dbgs() << "INFO: Canonicalizing 1op pointer iterator:\n";
         llvm::dbgs() << "   Phi node: ";
         phi_node->dump();

         phi_node->replaceAllUsesWith(phi_node->getIncomingValue(0));
         phi_node->eraseFromParent();

         ++transformation_count;
      }
   */
   return transformation_count > 0;
}

bool chunk_operations_lowering(llvm::Function& function)
{
   // See for example the mpeg2 example, motion_vertical_field_select[i][j] = inmvfs[i][j];
   const llvm::DataLayout& DL = function.getParent()->getDataLayout();

   if(!DL.isLittleEndian())
   {
      return false;
   }

   std::vector<ChunkCopy> chunk_copy_vec;
   std::vector<ChunkInit> chunk_init_vec;

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::BitCastOperator* store_bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(&i))
         {
            if(store_bitcast_op->hasOneUse())
            {
               llvm::User* bitcast_single_user = store_bitcast_op->use_begin()->getUser();

               if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(bitcast_single_user))
               {
                  if(llvm::ConstantInt* constant_store_val = llvm::dyn_cast<llvm::ConstantInt>(store_inst->getValueOperand()))
                  {
                     llvm::Type* src_ty = store_bitcast_op->getSrcTy();
                     llvm::Type* dest_ty = store_bitcast_op->getDestTy();
                     chunk_init_vec.push_back(ChunkInit(store_inst, store_bitcast_op, store_bitcast_op->getOperand(0), constant_store_val, src_ty, dest_ty));
                  }
                  else if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(store_inst->getValueOperand()))
                  {
                     if(load_inst->hasOneUse())
                     {
                        if(llvm::BitCastOperator* load_bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(load_inst->getPointerOperand()))
                        {
                           if(load_bitcast_op->hasOneUse())
                           {
                              if(load_bitcast_op->getSrcTy() == store_bitcast_op->getSrcTy())
                              {
                                 llvm::Type* src_ty = load_bitcast_op->getSrcTy();

                                 if(load_bitcast_op->getDestTy() == store_bitcast_op->getDestTy())
                                 {
                                    llvm::Type* dest_ty = load_bitcast_op->getDestTy();

                                    if(DL.getTypeAllocSize(src_ty) == DL.getTypeAllocSize(dest_ty))
                                    {
                                       llvm::Value* loaded_ptr = load_bitcast_op->getOperand(0);
                                       llvm::Value* stored_ptr = store_bitcast_op->getOperand(0);

                                       chunk_copy_vec.push_back(ChunkCopy(load_inst, load_bitcast_op, loaded_ptr, store_inst, store_bitcast_op, stored_ptr, src_ty, dest_ty));
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for(const ChunkCopy& chunk_copy : chunk_copy_vec)
   {
      lower_chunk_copy(chunk_copy, function.getParent()->getDataLayout());
   }

   for(const ChunkInit& chunk_init : chunk_init_vec)
   {
      lower_chunk_init(chunk_init);
   }

   return !chunk_copy_vec.empty() or !chunk_init_vec.empty();
}

bool compute_gepi_idxs_rec(llvm::Type* type_rec, llvm::Type* dst_ty, unsigned long long idx, std::vector<unsigned long long>& gepi_idxs)
{
   if(type_rec == dst_ty)
   {
      gepi_idxs.push_back(idx);
      return true;
   }
   else if(type_rec->isStructTy())
   {
      if(type_rec->getStructNumElements() == 1)
      {
         gepi_idxs.push_back(0);
         return compute_gepi_idxs_rec(type_rec->getStructElementType(0), dst_ty, idx, gepi_idxs);
      }
      else
      {
         return false;
      }
   }
   else if(type_rec->isArrayTy())
   {
      unsigned long long array_elements = type_rec->getArrayNumElements();
      unsigned long long this_idx = (unsigned long long)std::floor((double)idx / array_elements);
      unsigned long long next_idx = idx % array_elements;
      gepi_idxs.push_back(this_idx);
      return compute_gepi_idxs_rec(type_rec->getArrayElementType(), dst_ty, next_idx, gepi_idxs);
   }
   else
   {
      return false;
   }
}

bool bitcast_vector_removal(llvm::Function& function)
{
   const llvm::DataLayout& DL = function.getParent()->getDataLayout();

   std::vector<std::tuple<llvm::Value*, llvm::ConstantInt*, llvm::BitCastOperator*, std::vector<unsigned long long>>> sequential_access_vec;
   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(&i))
         {
            if(gep_op->hasNUsesOrMore(1))
            {
               if(llvm::isa<llvm::Instruction>(gep_op->use_begin()->getUser()))
               {
                  if(llvm::ConstantInt* first_idx = llvm::dyn_cast<llvm::ConstantInt>(gep_op->getOperand(1)))
                  {
                     if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(gep_op->getPointerOperand()))
                     {
                        unsigned long long src_ty_size = DL.getTypeSizeInBits(bitcast_op->getSrcTy()->getPointerElementType());
                        unsigned long long dest_ty_size = DL.getTypeSizeInBits(bitcast_op->getDestTy()->getPointerElementType());

                        if(dest_ty_size < src_ty_size)
                        {
                           std::vector<unsigned long long> gepi_idxs;

                           bool can_simplify = compute_gepi_idxs_rec(bitcast_op->getSrcTy()->getPointerElementType(), bitcast_op->getDestTy()->getPointerElementType(), first_idx->getValue().getSExtValue(), gepi_idxs);

                           if(can_simplify)
                           {
                              sequential_access_vec.push_back(std::make_tuple(gep_op, first_idx, bitcast_op, gepi_idxs));
                           }
                        }
                     }
                  }
               }
            }
         }
         else if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(&i))
         {
            if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(load_inst->getPointerOperand()))
            {
               unsigned long long src_ty_size = DL.getTypeSizeInBits(bitcast_op->getSrcTy()->getPointerElementType());
               unsigned long long dest_ty_size = DL.getTypeSizeInBits(bitcast_op->getDestTy()->getPointerElementType());

               if(dest_ty_size < src_ty_size)
               {
                  std::vector<unsigned long long> gepi_idxs;

                  bool can_simplify = compute_gepi_idxs_rec(bitcast_op->getSrcTy()->getPointerElementType(), bitcast_op->getDestTy()->getPointerElementType(), 0, gepi_idxs);

                  if(can_simplify)
                  {
                     llvm::ConstantInt* first_idx = llvm::dyn_cast<llvm::ConstantInt>(llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(function.getContext()), llvm::APInt(32, 0)));
                     sequential_access_vec.push_back(std::make_tuple(load_inst, first_idx, bitcast_op, gepi_idxs));
                  }
               }
            }
         }
         else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&i))
         {
            if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(store_inst->getPointerOperand()))
            {
               unsigned long long src_ty_size = DL.getTypeSizeInBits(bitcast_op->getSrcTy()->getPointerElementType());
               unsigned long long dest_ty_size = DL.getTypeSizeInBits(bitcast_op->getDestTy()->getPointerElementType());

               if(dest_ty_size < src_ty_size)
               {
                  std::vector<unsigned long long> gepi_idxs;

                  bool can_simplify = compute_gepi_idxs_rec(bitcast_op->getSrcTy()->getPointerElementType(), bitcast_op->getDestTy()->getPointerElementType(), 0, gepi_idxs);

                  if(can_simplify)
                  {
                     llvm::ConstantInt* first_idx = llvm::dyn_cast<llvm::ConstantInt>(llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(function.getContext()), llvm::APInt(32, 0)));
                     sequential_access_vec.push_back(std::make_tuple(store_inst, first_idx, bitcast_op, gepi_idxs));
                  }
               }
            }
         }
      }
   }

   for(const auto& sequential_tuple : sequential_access_vec)
   {
      llvm::Value* last_inst;
      llvm::ConstantInt* first_idx;
      llvm::BitCastOperator* bitcast_op;
      std::vector<unsigned long long> gepi_idxs;

      std::tie(last_inst, first_idx, bitcast_op, gepi_idxs) = sequential_tuple;

      std::vector<llvm::Value*> gepi_idxs_val_vec;

      std::string gepi_name = last_inst->getName().str() + ".srgepi";
      for(unsigned long long idx : gepi_idxs)
      {
         llvm::ConstantInt* c_idx = llvm::ConstantInt::get(first_idx->getType(), idx, false);
         gepi_idxs_val_vec.push_back(c_idx);
         gepi_name += "." + std::to_string(idx);
      }

      if(llvm::GEPOperator* gepop = llvm::dyn_cast<llvm::GEPOperator>(last_inst))
      {
         for(unsigned long long idx = 2; idx < gepop->getNumOperands(); ++idx)
         {
            gepi_idxs_val_vec.push_back(gepop->getOperand(idx));
         }
      }

      llvm::Instruction* next_inst = nullptr;
      if(llvm::GEPOperator* gepi = llvm::dyn_cast<llvm::GEPOperator>(last_inst))
      {
         next_inst = llvm::dyn_cast<llvm::Instruction>(gepi->use_begin()->getUser());
         ;
      }
      else if(llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(last_inst))
      {
         next_inst = inst;
      }

      llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, bitcast_op->getOperand(0), gepi_idxs_val_vec, gepi_name, next_inst);

      if(llvm::GEPOperator* gepi = llvm::dyn_cast<llvm::GEPOperator>(last_inst))
      {
         last_inst->replaceAllUsesWith(new_gepi);
      }
      else if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(last_inst))
      {
         load_inst->setOperand(load_inst->getPointerOperandIndex(), new_gepi);
      }
      else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(last_inst))
      {
         store_inst->setOperand(store_inst->getPointerOperandIndex(), new_gepi);
      }

      if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(last_inst))
      {
         if(gepi->getNumUses() == 0)
         {
            gepi->eraseFromParent();
         }
      }
      if(llvm::BitCastInst* bitcast = llvm::dyn_cast<llvm::BitCastInst>(bitcast_op))
      {
         if(bitcast->getNumUses() == 0)
         {
            bitcast->eraseFromParent();
         }
      }
   }

   return !sequential_access_vec.empty();
}

void implement_copy(llvm::Type* ty, unsigned long size_to_be_copied, llvm::Value* load_val, llvm::Value* store_val, llvm::Instruction* inst)
{
   unsigned long ty_size = inst->getModule()->getDataLayout().getTypeSizeInBits(ty) / 8;

   double fitting = size_to_be_copied / ty_size;
   std::vector<unsigned long long> gepi_idxs = std::vector<unsigned long long>();
   if(fitting == 1)
   {
      gepi_idxs.push_back(0);
   }
   recursive_copy_lowering(ty, gepi_idxs, load_val, store_val, inst, inst, fitting);
}

bool remove_lifetime(llvm::Function& function)
{
   std::vector<llvm::Instruction*> intrinsic_to_remove;

   unsigned long memcpy_count = 0;
   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
         {
            llvm::Function* called_function = call_inst->getCalledFunction();

            if(called_function)
            {
               if(called_function->getIntrinsicID() == llvm::Intrinsic::ID::lifetime_start)
               {
                  intrinsic_to_remove.push_back(call_inst);
               }
               if(called_function->getIntrinsicID() == llvm::Intrinsic::ID::lifetime_end)
               {
                  intrinsic_to_remove.push_back(call_inst);
               }
               if(called_function->getIntrinsicID() == llvm::Intrinsic::ID::memcpy)
               {
                  if(llvm::BitCastOperator* src_op = llvm::dyn_cast<llvm::BitCastOperator>(call_inst->getOperand(0)))
                  {
                     if(llvm::BitCastOperator* dst_op = llvm::dyn_cast<llvm::BitCastOperator>(call_inst->getOperand(1)))
                     {
                        if(src_op->getSrcTy() == dst_op->getSrcTy())
                        {
                           llvm::Type* ty = src_op->getSrcTy()->getPointerElementType();

                           unsigned long size = function.getParent()->getDataLayout().getTypeSizeInBits(ty) / 8;

                           implement_copy(ty, size, src_op->getOperand(0), dst_op->getOperand(0), call_inst);
                           intrinsic_to_remove.push_back(call_inst);
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for(llvm::Instruction* instr : intrinsic_to_remove)
   {
      instr->eraseFromParent();
   }

   return !intrinsic_to_remove.empty() or memcpy_count > 0;
}

bool select_lowering(llvm::Function& function)
{
   std::vector<llvm::Instruction*> inst_to_remove;

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::SelectInst* select_inst = llvm::dyn_cast<llvm::SelectInst>(&i))
         {
            llvm::Type* selected_type = select_inst->getTrueValue()->getType();
            if(selected_type->isPointerTy())
            {
               if(select_inst->hasOneUse())
               {
                  if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(select_inst->use_begin()->getUser()))
                  {
                     if(gepi->hasOneUse() and (llvm::isa<llvm::LoadInst>(gepi->use_begin()->getUser())))
                     {
                        std::vector<llvm::Value*> gepi_idxs;
                        for(auto idx = gepi->idx_begin(); idx < gepi->idx_end(); ++idx)
                        {
                           gepi_idxs.push_back(idx->get());
                        }

                        if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(gepi->use_begin()->getUser()))
                        {
                           llvm::GetElementPtrInst* true_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, select_inst->getTrueValue(), gepi_idxs, gepi->getName().str() + ".true", gepi);
                           llvm::GetElementPtrInst* false_gepi = llvm::GetElementPtrInst::CreateInBounds(nullptr, select_inst->getFalseValue(), gepi_idxs, gepi->getName().str() + ".false", gepi);

                           llvm::LoadInst* true_load = new llvm::LoadInst(true_gepi, load_inst->getName().str() + ".lowered.true", gepi);
                           llvm::LoadInst* false_load = new llvm::LoadInst(false_gepi, load_inst->getName().str() + ".lowered.false", gepi);
                           llvm::SelectInst* new_select_inst = llvm::SelectInst::Create(select_inst->getCondition(), true_load, false_load, select_inst->getName().str() + ".lowered", gepi);

                           load_inst->replaceAllUsesWith(new_select_inst);

                           inst_to_remove.push_back(load_inst);
                           inst_to_remove.push_back(gepi);
                           inst_to_remove.push_back(select_inst);
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for(llvm::Instruction* inst : inst_to_remove)
   {
      inst->eraseFromParent();
   }

   return inst_to_remove.size() > 0;
}

bool canonical_idxs(llvm::Function& function)
{
   llvm::Type* idx_ty = llvm::Type::getInt32Ty(function.getContext());

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(&i))
         {
            for(unsigned long i = 0; i < gepi->getNumIndices(); ++i)
            {
               llvm::Value* operand = gepi->getOperand(i + 1);
               if(llvm::ConstantInt* c_idx = llvm::dyn_cast<llvm::ConstantInt>(operand))
               {
                  gepi->setOperand(i + 1, llvm::ConstantInt::get(idx_ty, c_idx->getSExtValue(), true));
               }
               else
               {
                  if(operand->getType()->getIntegerBitWidth() > 32)
                  {
                     std::string name = gepi->getName().str() + "." + std::to_string(i) + ".sext";
                     llvm::CastInst* trunc_inst = llvm::TruncInst::Create(llvm::CastInst::Trunc, operand, idx_ty, name, gepi);
                     gepi->setOperand(i + 1, trunc_inst);
                  }
               }
            }
         }
      }
   }

   return false;
}

bool code_simplification(llvm::Function& function, llvm::LoopInfo& LI, llvm::ScalarEvolution& SE)
{
   std::map<llvm::Use*, llvm::Value*> point_to_set_map;

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
         {
         }
         else if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(&i))
         {
            point_to_set_map.insert(std::make_pair(&load_inst->getOperandUse(load_inst->getPointerOperandIndex()), nullptr));
         }
         else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&i))
         {
            point_to_set_map.insert(std::make_pair(&store_inst->getOperandUse(store_inst->getPointerOperandIndex()), nullptr));
         }
      }
   }

   std::map<const llvm::Loop*, unsigned long long> non_const_idxs_per_loop;
   std::map<const llvm::CallInst*, unsigned long long> non_const_idxs_per_call;

   for(auto pts_it : point_to_set_map)
   {
      llvm::Use* use = pts_it.first;
      llvm::Value* base = pts_it.second;

      if(llvm::Instruction* user_inst = llvm::dyn_cast<llvm::Instruction>(use->getUser()))
      {
         if((llvm::isa<llvm::CallInst>(user_inst) and !llvm::dyn_cast<llvm::CallInst>(user_inst)->getCalledFunction()->isIntrinsic()) or llvm::isa<llvm::LoadInst>(user_inst) or llvm::isa<llvm::StoreInst>(user_inst))
         {
            if(SE.isSCEVable(use->get()->getType()))
            {
               const llvm::SCEV* use_scev = SE.getSCEV(use->get());

               const llvm::SCEV* scev_rec = use_scev;
               while(const llvm::SCEVAddRecExpr* use_add_rec_scev_rec = llvm::dyn_cast<llvm::SCEVAddRecExpr>(scev_rec))
               {
                  const llvm::Loop* loop = use_add_rec_scev_rec->getLoop();

                  if(loop)
                  {
                     auto i_it = non_const_idxs_per_loop.insert(std::make_pair(loop, 0));
                     non_const_idxs_per_loop.at(loop) += 1;

                     if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(user_inst))
                     {
                        non_const_idxs_per_call[call_inst] += 1;
                     }
                  }
                  scev_rec = use_add_rec_scev_rec->evaluateAtIteration(SE.getZero(llvm::Type::getInt32Ty(function.getContext())), SE);
               }
            }
         }
      }
   }

   for(auto loop_it : non_const_idxs_per_loop)
   {
      llvm::Loop* loop = const_cast<llvm::Loop*>(loop_it.first);

      unsigned long long inst_count = 0;
      for(const llvm::BasicBlock* bb : loop->blocks())
      {
         if(bb != loop->getHeader() or bb != loop->getLoopLatch())
         {
            for(const llvm::Instruction& inst : *bb)
            {
               if(inst.isBinaryOp() or llvm::isa<llvm::CallInst>(inst))
               {
                  inst_count++;
               }
            }
         }
      }

      unsigned long long trip_count = SE.getSmallConstantTripCount(loop);
      double cost_value = (double)trip_count * (double)inst_count / (double)loop_it.second;
      bool cost_threshold = cost_value <= 32;
      bool trip_count_limit = trip_count > 0 and trip_count <= 32;
      bool inst_limit = inst_count <= 32;

      llvm::MDNode* loopID = nullptr;
      if(cost_threshold and trip_count_limit and inst_limit)
      {
         loopID = llvm::MDNode::get(function.getContext(), llvm::MDString::get(function.getContext(), "llvm.loop.unroll.full"));
         llvm::dbgs() << "INFO: Force unroll of loop " << loop->getName() << " in function " << function.getName() << "(TripCount: " << trip_count << ", CostValue: " << cost_value << ", InstCount: " << inst_count << ")\n";
      }
      else
      {
         loopID = llvm::MDNode::get(function.getContext(), llvm::MDString::get(function.getContext(), "llvm.loop.unroll.disable"));
         llvm::dbgs() << "INFO: Disable unroll of loop " << loop->getName() << " in function " << function.getName() << "(TripCount: " << trip_count << ", CostValue: " << cost_value << ", InstCount: " << inst_count << ")\n";
      }

      std::vector<llvm::Metadata*> metas;
      metas.push_back(loopID);
      metas.push_back(loopID);
      llvm::MDTuple* tuple = llvm::MDTuple::getDistinct(function.getContext(), metas);
      tuple->replaceOperandWith(0, tuple);

      loop->setLoopID(tuple);
   }

   unsigned long long inlined_count = 0;
   for(auto call_it : non_const_idxs_per_call)
   {
      llvm::CallInst* call_inst = const_cast<llvm::CallInst*>(call_it.first);
      unsigned long long idx_count = call_it.second;

      if(call_inst->getCalledFunction())
      {
         llvm::Function* called_function = call_inst->getCalledFunction();
         if(called_function and called_function->size() > 0)
         {
            unsigned long long inst_count = 0;
            for(llvm::BasicBlock& bb : *called_function)
            {
               for(llvm::Instruction& inst : bb)
               {
                  if(inst.isBinaryOp() or llvm::isa<llvm::CallInst>(inst))
                  {
                     inst_count++;
                  }
               }
            }

            double threshold = inst_count / idx_count;
            if(threshold <= 32)
            {
               called_function->removeFnAttr(llvm::Attribute::NoInline);
               called_function->removeFnAttr(llvm::Attribute::OptimizeNone);
               llvm::InlineFunctionInfo IFI = llvm::InlineFunctionInfo();
               if((llvm::isa<llvm::CallInst>(call_inst) && !llvm::InlineFunction(llvm::dyn_cast<llvm::CallInst>(call_inst), IFI)) || (llvm::isa<llvm::InvokeInst>(call_inst) && !llvm::InlineFunction(llvm::dyn_cast<llvm::InvokeInst>(call_inst), IFI)))
               {
                  llvm::errs() << "ERR: Cannot inline function " << called_function->getName() << "\n";
                  exit(-1);
               }

               llvm::dbgs() << "INFO: Inlining call to " << called_function->getName() << " in function " << call_inst->getFunction()->getName() << "\n";
               inlined_count++;
            }
         }
      }
   }

   return inlined_count > 0;
}

bool remove_meta(llvm::Function& function)
{
   for(llvm::BasicBlock& bb : function)
   {
      llvm::Instruction* terminator = bb.getTerminator();

      terminator->setMetadata("llvm.loop", nullptr);
   }

   return false;
}

bool gepi_explicitation(llvm::Function& function)
{
   std::vector<llvm::Use*> ops_to_explicit;
   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         for(auto& op : i.operands())
         {
            if(!llvm::isa<llvm::Instruction>(op.get()))
            {
               if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(op.get()))
               {
                  ops_to_explicit.push_back(&op);
               }
            }
         }
      }
   }

   unsigned long gepi_idx = 0;
   for(unsigned long gepi_idx = 0; gepi_idx < ops_to_explicit.size(); gepi_idx++)
   {
      llvm::Use* use = ops_to_explicit.at(gepi_idx);
      if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(use->get()))
      {
         std::string gepi_name = "gepi." + std::to_string(gepi_idx);
         if(llvm::Instruction* user_inst = llvm::dyn_cast<llvm::Instruction>(use->getUser()))
         {
            llvm::Instruction* insert_point_inst = user_inst;
            if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(use->getUser()))
            {
               insert_point_inst = phi_node->getIncomingBlock(*use)->getTerminator();
            }

            std::vector<llvm::Value*> idxs = std::vector<llvm::Value*>(gep_op->idx_begin(), gep_op->idx_end());
            llvm::GetElementPtrInst* gepi = nullptr;
            if(gep_op->isInBounds())
            {
               gepi = llvm::GetElementPtrInst::CreateInBounds(gep_op->getPointerOperand(), idxs, gepi_name, insert_point_inst);
            }
            else
            {
               gepi = llvm::GetElementPtrInst::Create(nullptr, gep_op->getPointerOperand(), idxs, gepi_name, insert_point_inst);
            }

            user_inst->setOperand(use->getOperandNo(), gepi);
            llvm::Use* ptr_op_use = &gepi->getOperandUse(gepi->getPointerOperandIndex());
            /*
                        llvm::errs() << "Converting: \n";
                        llvm::errs() << "  gep_op: "; gep_op->dump();
                        llvm::errs() << "  in gep_inst: "; gepi->dump();
            */
            if(!llvm::isa<llvm::Instruction>(ptr_op_use->get()))
            {
               if(llvm::GEPOperator* gep_op_ptr = llvm::dyn_cast<llvm::GEPOperator>(ptr_op_use->get()))
               {
                  ops_to_explicit.push_back(ptr_op_use);
               }
            }
         }
         else
         {
            llvm::errs() << "ERR: wrong gepi\n";
            exit(-1);
         }
      }
   }

   return gepi_idx > 0;
}

bool clean_lcssa(llvm::Function& function)
{
   std::set<llvm::PHINode*> phi_to_remove;

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(&i))
         {
            if(phi_node->getNumOperands() == 1)
            {
               llvm::Value* val = phi_node->getIncomingValue(0);
               phi_node->replaceAllUsesWith(val);
               phi_to_remove.insert(phi_node);
            }
         }
      }
   }

   for(llvm::PHINode* phi_node : phi_to_remove)
   {
      phi_node->eraseFromParent();
   }

   return phi_to_remove.size() > 0;
}

bool GepiCanonicalizationPass::runOnFunction(llvm::Function& function)
{
   if(skipFunction(function))
      return false;
   // auto t_begin = std::chrono::high_resolution_clock::now();
   bool result = false;
   switch(optimization_selection)
   {
      case SROA_cleanLCSSA:
         result = clean_lcssa(function);
         break;
      case SROA_gepiExplicitation:
         result = gepi_explicitation(function);
         break;
      case SROA_codeSimplification:
      {
         // Check CHStone adpcm for examples
         llvm::LoopInfo& LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
         llvm::ScalarEvolution& SE = getAnalysis<llvm::ScalarEvolutionWrapperPass>().getSE();
         result = code_simplification(function, LI, SE);
         break;
      }
      case SROA_ptrIteratorSimplification:
      {
         // Check CHStone adpcm for examples
         // llvm::LoopInfo &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
         result = false; // ptr_iterator_simplification(function, LI);
         break;
      }
      case SROA_chunkOperationsLowering:
         result = chunk_operations_lowering(function);
         break;
      case SROA_bitcastVectorRemoval:
         result = bitcast_vector_removal(function);
         break;
      case SROA_removeLifetime:
         result = remove_lifetime(function);
         break;
      case SROA_selectLowering:
         result = select_lowering(function);
         break;
      case SROA_canonicalIdxs:
         result = canonical_idxs(function);
         break;
      case SROA_removeMeta:
         result = remove_meta(function);
         break;
      default:
         llvm::errs() << "ERR: No optimization found\n";
         exit(-1);
   }
   // auto t_end = std::chrono::high_resolution_clock::now();
   // double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_begin).count();
   // llvm::dbgs() << "INFO: " << optimization_names[optimization_selection] << " of " << function.getName() << " took " << duration * 1e-9 << " seconds to complete\n";
   return result;
}

GepiCanonicalizationPass* createCleanLCSSA()
{
   return new GepiCanonicalizationPass(SROA_cleanLCSSA);
}

GepiCanonicalizationPass* createGepiExplicitation()
{
   return new GepiCanonicalizationPass(SROA_gepiExplicitation);
}

GepiCanonicalizationPass* createCodeSimplificationPass()
{
   return new GepiCanonicalizationPass(SROA_codeSimplification);
}

GepiCanonicalizationPass* createPtrIteratorSimplificationPass()
{
   return new GepiCanonicalizationPass(SROA_ptrIteratorSimplification);
}

GepiCanonicalizationPass* createChunkOperationsLoweringPass()
{
   return new GepiCanonicalizationPass(SROA_chunkOperationsLowering);
}

GepiCanonicalizationPass* createBitcastVectorRemovalPass()
{
   return new GepiCanonicalizationPass(SROA_bitcastVectorRemoval);
}

GepiCanonicalizationPass* createRemoveIntrinsicPass()
{
   return new GepiCanonicalizationPass(SROA_removeLifetime);
}

GepiCanonicalizationPass* createSelectLoweringPass()
{
   return new GepiCanonicalizationPass(SROA_selectLowering);
}

GepiCanonicalizationPass* createGepiCanonicalIdxsPass()
{
   return new GepiCanonicalizationPass(SROA_canonicalIdxs);
}

GepiCanonicalizationPass* createRemoveMetaPass()
{
   return new GepiCanonicalizationPass(SROA_removeMeta);
}
