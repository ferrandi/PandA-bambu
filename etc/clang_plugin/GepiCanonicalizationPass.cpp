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

#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>
#include "GepiCanonicalizationPass.hpp"

llvm::PHINode *get_last_phi(llvm::BasicBlock *bb) {
    llvm::PHINode *last_phi = nullptr;

    for (llvm::Instruction &instruction : *bb) {
        if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(&instruction)) {
            last_phi = phi_node;
        } else {
            break;
        }
    }

    return last_phi;
}

bool GepiCanonicalizationPass::runOnFunction(llvm::Function &function) {

    unsigned long long transformation_count = 0;

    std::vector<llvm::PHINode *> one_op_phi_vec;
    std::vector<llvm::PHINode *> two_op_phi_vec;

    for (llvm::BasicBlock &bb : function) {
        for (llvm::Instruction &instruction : bb) {
            if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(&instruction)) {
                if (phi_node->getType()->isPointerTy()) {
                    switch (phi_node->getNumOperands()) {
                        case 1:
                            one_op_phi_vec.push_back(phi_node);
                            break;
                        case 2:
                            two_op_phi_vec.push_back(phi_node);
                            break;
                    }
                }
            } else {
                break;
            }
        }
    }

    for (llvm::PHINode *phi_node : two_op_phi_vec) {

        llvm::GetElementPtrInst *ind_var_gepi = nullptr;
        llvm::Value *init_ptr = nullptr;

        for (unsigned short idx = 0; idx < 2; ++idx) {
            llvm::Value *incoming_value = phi_node->getIncomingValue(idx);

            if (llvm::GetElementPtrInst *gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(incoming_value)) {
                if (gepi->getPointerOperand() == phi_node) {

                    if (ind_var_gepi == nullptr and init_ptr == nullptr) {
                        switch (idx) {
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
                    } else {
                        ind_var_gepi = nullptr;
                        init_ptr = nullptr;
                        break;
                    }
                }
            }
        }

        if (ind_var_gepi != nullptr and init_ptr != nullptr) {

            if (ind_var_gepi->getNumIndices() == 1) {

                llvm::Value *gepi_index = ind_var_gepi->getOperand(1);
                std::string new_phi_node_name = phi_node->getName().str() + ".c";
                llvm::PHINode *new_phi_node = llvm::PHINode::Create(gepi_index->getType(), 2, new_phi_node_name, phi_node);

                for (unsigned short idx = 0; idx < 2; ++idx) {

                    if (phi_node->getIncomingValue(idx) == init_ptr) {
                        llvm::Constant *zero_cint = llvm::ConstantInt::get(gepi_index->getType(), 0, false);
                        new_phi_node->addIncoming(zero_cint, phi_node->getIncomingBlock(idx));
                    } else if (phi_node->getIncomingValue(idx) == ind_var_gepi) {
                        llvm::Constant *one_cint = llvm::ConstantInt::get(gepi_index->getType(), 1, false);
                        std::string add_inst_name = ind_var_gepi->getName().str() + ".add";
                        llvm::BinaryOperator *add_inst = llvm::BinaryOperator::Create(llvm::Instruction::Add, new_phi_node, gepi_index, add_inst_name, ind_var_gepi);
                        new_phi_node->addIncoming(add_inst, phi_node->getIncomingBlock(idx));
                    } else {
                        exit(-1);
                    }
                }

                std::vector<llvm::Value *> idx_vec;
                if (!llvm::isa<llvm::Argument>(init_ptr)) {
                    if (init_ptr->getType()->isAggregateType()) {
                        llvm::Constant *zero_cint = llvm::ConstantInt::get(gepi_index->getType(), 0, false);
                        idx_vec.push_back(zero_cint);
                    }
                }
                idx_vec.push_back(new_phi_node);

                std::string new_gepi_name = phi_node->getName().str() + ".gepi";
                llvm::Type * gepi_type = llvm::cast<llvm::PointerType>(init_ptr->getType()->getScalarType())->getElementType();
                llvm::GetElementPtrInst *new_gepi = llvm::GetElementPtrInst::Create(gepi_type, init_ptr, idx_vec, new_gepi_name);
                new_gepi->insertAfter(get_last_phi(phi_node->getParent()));

                std::vector<llvm::CmpInst *> cmp_inst_vec;
                for (llvm::Use &u : phi_node->uses()) {
                    if (llvm::CmpInst *cmp_inst = llvm::dyn_cast<llvm::CmpInst>(u.getUser())) {
                        cmp_inst_vec.push_back(cmp_inst);
                    }
                }
                phi_node->replaceAllUsesWith(new_gepi);
                phi_node->eraseFromParent();

                for (llvm::CmpInst *cmp_inst : cmp_inst_vec) {
                    llvm::GetElementPtrInst *gepi0 = llvm::dyn_cast<llvm::GetElementPtrInst>(cmp_inst->getOperand(0));
                    llvm::GetElementPtrInst *gepi1 = llvm::dyn_cast<llvm::GetElementPtrInst>(cmp_inst->getOperand(1));

                    if (gepi0 and gepi1) {

                        if (gepi0->getNumIndices() == 1 and gepi1->getNumIndices() == 1) {
                            llvm::Value *idx0 = gepi0->getOperand(1);
                            llvm::Value *idx1 = gepi1->getOperand(1);

                            cmp_inst->setOperand(0, idx0);
                            cmp_inst->setOperand(1, idx1);
                        }
                    }
                }

                ++transformation_count;
            }
        }
    }

    for (llvm::PHINode *phi_node : one_op_phi_vec) {
        phi_node->replaceAllUsesWith(phi_node->getIncomingValue(0));
        phi_node->eraseFromParent();

        ++transformation_count;
    }

    return transformation_count > 0;

}

llvm::Pass *createGepiCanonicalizationPass() {
    return new GepiCanonicalizationPass();
}
