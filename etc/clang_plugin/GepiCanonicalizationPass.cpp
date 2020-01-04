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
 *              Copyright (C) 2019-2020 Politecnico di Milano
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
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Operator.h>

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

void recursive_copy_lowering(llvm::Type* type, std::vector<unsigned long long> gepi_idxs, llvm::Value* load_ptr, llvm::Value* store_ptr, llvm::LoadInst* load_inst, llvm::StoreInst* store_inst, unsigned long long fitting)
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
         llvm::errs() << "ERR wrong type\n";
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
      load_ptr->print(llvm::errs());
      for(const auto& v : gepi_value_idxs)
      {
         v->print(llvm::errs());
      }
      llvm::GetElementPtrInst* load_gep_inst = llvm::GetElementPtrInst::Create(nullptr, load_ptr, gepi_value_idxs, gepi_name, load_inst);
      llvm::GetElementPtrInst* store_gep_inst = llvm::GetElementPtrInst::Create(nullptr, store_ptr, gepi_value_idxs, gepi_name, store_inst);

      llvm::LoadInst* lowered_load = new llvm::LoadInst(load_gep_inst, "ccload." + gepi_name, load_inst);
      llvm::StoreInst* lowered_store = new llvm::StoreInst(lowered_load, store_gep_inst, store_inst);
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
         llvm::errs() << "ERR wrong type\n";
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

      llvm::GetElementPtrInst* store_gep_inst = llvm::GetElementPtrInst::Create(nullptr, store_ptr, gepi_value_idxs, gepi_name, store_inst);

      llvm::APInt cast_ap_int = llvm::APInt(store_gep_inst->getType()->getPointerElementType()->getIntegerBitWidth(), init_value->getValue().getSExtValue());
      llvm::Constant* cast_init_val = llvm::ConstantInt::get(store_gep_inst->getType()->getPointerElementType(), cast_ap_int);
      llvm::StoreInst* lowered_store = new llvm::StoreInst(cast_init_val, store_gep_inst, store_inst);
   }
}

void lower_chunk_copy(const ChunkCopy& chunk_copy, const llvm::DataLayout& DL)
{
   llvm::errs() << "INFO: Lowered chunk copy\n";
   llvm::errs() << "          Load bitcast:  ";
   chunk_copy.load_bitcast_op->print(llvm::errs());
   llvm::errs() << "          Load inst:     ";
   chunk_copy.load_inst->print(llvm::errs());
   llvm::errs() << "          Store bitcast: ";
   chunk_copy.store_bitcast_op->print(llvm::errs());
   llvm::errs() << "          Store inst:    ";
   chunk_copy.store_bitcast_op->print(llvm::errs());

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
   llvm::errs() << "INFO: Lowered chunk init\n";
   llvm::errs() << "          Store bitcast: ";
   chunk_init.store_bitcast_op->print(llvm::errs());
   llvm::errs() << "          Store inst:    ";
   chunk_init.store_bitcast_op->print(llvm::errs());
   llvm::errs() << "          Stored val:    ";
   chunk_init.stored_value->print(llvm::errs());
   llvm::errs() << "          Stored ptr:    ";
   chunk_init.stored_ptr->print(llvm::errs());

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

bool ptr_iterator_simplification(llvm::Function& function)
{
   unsigned long long transformation_count = 0;

   std::vector<llvm::PHINode*> one_op_phi_vec;
   std::vector<llvm::PHINode*> two_op_phi_vec;

   for(llvm::BasicBlock& bb : function)
   {
      for(llvm::Instruction& instruction : bb)
      {
         if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(&instruction))
         {
            if(phi_node->getType()->isPointerTy())
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
         else
         {
            break;
         }
      }
   }

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
         llvm::errs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find indvar/init):\n";
         llvm::errs() << "   Phi node: ";
         phi_node->print(llvm::errs());
         continue;
      }

      if(ind_var_gepi->getNumIndices() != 1 or !ind_var_gepi->hasOneUse())
      {
         llvm::errs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find indvar):\n";
         llvm::errs() << "   Phi node: ";
         phi_node->print(llvm::errs());
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
         llvm::errs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find cmp/stop):\n";
         llvm::errs() << "   Phi node: ";
         phi_node->print(llvm::errs());
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
         llvm::errs() << "INFO: In function " << function.getName().str() << " cannot canonicalize 2op pointer iterator (cannot properly find init/stop/base):\n";
         llvm::errs() << "   Phi node: ";
         phi_node->print(llvm::errs());
         continue;
      }

      if(ind_var_gepi != nullptr and cmp_inst != nullptr and init_ptr != nullptr and stop_ptr != nullptr and init_val != nullptr and stop_val != nullptr and base_ptr != nullptr)
      {
         llvm::errs() << "INFO: In function " << function.getName().str() << " Canonicalizing 2op pointer iterator:\n";
         llvm::errs() << "   Phi node: ";
         phi_node->print(llvm::errs());
         llvm::errs() << "     Ind var gepi: ";
         ind_var_gepi->print(llvm::errs());
         llvm::errs() << "     Cmp inst: ";
         cmp_inst->print(llvm::errs());
         llvm::errs() << "     Init ptr: ";
         init_ptr->print(llvm::errs());
         llvm::errs() << "     Stop ptr: ";
         stop_ptr->print(llvm::errs());
         llvm::errs() << "     Base ptr: ";
         base_ptr->print(llvm::errs());
         llvm::errs() << "     Init val: ";
         init_val->print(llvm::errs());
         llvm::errs() << "     Stop val: ";
         stop_val->print(llvm::errs());
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

               llvm::errs() << "   New add: ";
               add_inst->print(llvm::errs());
            }
            else
            {
               exit(-1);
            }
         }

         llvm::errs() << "   New phi node: ";
         new_phi_node->print(llvm::errs());

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
         llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::Create(nullptr, base_ptr, idx_vec, new_gepi_name);
         new_gepi->insertAfter(get_last_phi(phi_node->getParent()));

         phi_node->replaceAllUsesWith(new_gepi);
         phi_node->eraseFromParent();
         ind_var_gepi->eraseFromParent();

         llvm::errs() << "   New gepi: ";
         new_gepi->print(llvm::errs());

         cmp_inst->setOperand(0, new_phi_node);
         cmp_inst->setOperand(1, stop_val);

         llvm::errs() << "   New cmp: ";
         cmp_inst->print(llvm::errs());

         ++transformation_count;
      }
   }

   for(llvm::PHINode* phi_node : one_op_phi_vec)
   {
      llvm::errs() << "INFO: Canonicalizing 1op pointer iterator:\n";
      llvm::errs() << "   Phi node: ";
      phi_node->print(llvm::errs());

      phi_node->replaceAllUsesWith(phi_node->getIncomingValue(0));
      phi_node->eraseFromParent();

      ++transformation_count;
   }

   return transformation_count > 0;
}

bool chunk_operations_lowering(llvm::Function& function)
{
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

   std::vector<std::tuple<llvm::GEPOperator*, llvm::ConstantInt*, llvm::BitCastOperator*, std::vector<unsigned long long>>> sequential_access_vec;
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
      }
   }

   for(const auto& sequential_tuple : sequential_access_vec)
   {
      llvm::GEPOperator* gep_op;
      llvm::ConstantInt* first_idx;
      llvm::BitCastOperator* bitcast_op;
      std::vector<unsigned long long> gepi_idxs;

      std::tie(gep_op, first_idx, bitcast_op, gepi_idxs) = sequential_tuple;

      std::vector<llvm::Value*> gepi_idxs_val_vec;

      std::string gepi_name = gep_op->getName().str() + ".srgepi";
      for(unsigned long long idx : gepi_idxs)
      {
         llvm::ConstantInt* c_idx = llvm::ConstantInt::get(first_idx->getType(), idx, false);
         gepi_idxs_val_vec.push_back(c_idx);
         gepi_name += "." + std::to_string(idx);
      }

      for(unsigned long long idx = 2; idx < gep_op->getNumOperands(); ++idx)
      {
         gepi_idxs_val_vec.push_back(gep_op->getOperand(idx));
      }

      llvm::Instruction* next_inst = llvm::dyn_cast<llvm::Instruction>(gep_op->use_begin()->getUser());
      llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::Create(nullptr, bitcast_op->getOperand(0), gepi_idxs_val_vec, gepi_name, next_inst);

      gep_op->replaceAllUsesWith(new_gepi);
      if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(gep_op))
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

bool GepiCanonicalizationPass::runOnFunction(llvm::Function& function)
{
   switch(optimization_selection)
   {
      case SROA_ptrIteratorSimplification:
         return ptr_iterator_simplification(function);
      case SROA_chunkOperationsLowering:
         return chunk_operations_lowering(function);
      case SROA_bitcastVectorRemoval:
         return bitcast_vector_removal(function);
      default:
         llvm::errs() << "ERR No optimization found\n";
         exit(-1);
   }
   return false;
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
