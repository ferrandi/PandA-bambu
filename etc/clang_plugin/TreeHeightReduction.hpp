//===------------ TreeHeightReduction.hpp - Reduce tree height ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass implements a tree-height-reduction pass.
//
// Tree height reduction is an optimization to increase instruction level
// parallelism by transforming an operation tree like the following.
//
//            Before                                  After
//
//                    _ N1 _                  _________ N1 _________
//                   |      |                |                      |
//                 _ N2 _   L1           ___ N2 ___             ___ N3 ___
//                |      |              |          |           |          |
//              _ N3 _   L2     --->  _ N4 _     _ N5 _      _ N6 _     _ N7 _
//             |      |              |      |   |      |    |      |   |      |
//           _ N4 _   L3             L1     L2  L3     L4   L5     L6  L7     L8
//          |      |
//        _ N5 _   L4
//       |      |
//     _ N6 _   L5
//    |      |
//  _ N7 _   L6
// |      |
// L7     L8
//
//===----------------------------------------------------------------------===//
//
// An algorithm of tree height reduction is based on the paper:
//  Katherine Coons, Warren Hunt, Bertrand A. Maher, Doug Burger,
//  Kathryn S. McKinley. Optimal Huffman Tree-Height Reduction for
//  Instruction-Level Parallelism.
//
// The code has been adapted starting from this PR: https://reviews.llvm.org/D67383.
// The author of the original code is masakazu.ueno
// The porting has been done by Fabrizio Ferrandi. The main change is related to the operations latencies and on some
// assumptions.

#include <iostream>
#undef NDEBUG
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define THR_NAME "tree-height-reduction"
#define DEBUG_TYPE THR_NAME

#if __cplusplus > 201103L
#define STR(x) std::to_string(x)
#else
inline std::string __to_string(long long value)
{
   char buf[16];
   int len = std::sprintf(&buf[0], "%lld", value);
   return std::string(buf, len);
}
#define STR(x) __to_string(x)
#endif

namespace llvm
{
   static inline unsigned int resize_to_1_8_16_32_64(unsigned int value)
   {
      if(value == 1)
      {
         return 1;
      }
      else if(value <= 8)
      {
         return 8;
      }
      else if(value <= 16)
      {
         return 16;
      }
      else if(value <= 32)
      {
         return 32;
      }
      else
      {
         return 64;
      }
   }

   template <typename T>
   static bool isUsedAtCmpInst(Instruction* I)
   {
      for(User* U : I->users())
      {
         auto* UserInst = dyn_cast<Instruction>(U);
         if(UserInst && isa<T>(*UserInst))
         {
            return true;
      }
      }
      return false;
   }

   // Return true if 'I' is integer type and is target instruction of tree
   // height reduction.
   static bool isIntgerInstTHRTarget(Instruction* I)
   {
      if(!I->getType()->isIntegerTy())
      {
         return false;
      }
      // 'I' which is used at ICmpInst may be an induction variable.
      if(isUsedAtCmpInst<ICmpInst>(I))
      {
         return false;
      }
      switch(I->getOpcode())
      {
         case Instruction::Add:
         case Instruction::Mul:
         {
            return true;
         }
         default:
         {
            return false;
      }
   }
   }

   // Return true if 'I' is floating-point type and is target instruction
   // of tree height reduction.
   static bool isFpInstTHRTarget(Instruction* I)
   {
      if(!I->getType()->isFloatingPointTy())
      {
         return false;
      }
      if(!I->isFast())
      {
         return false;
      }
      switch(I->getOpcode())
      {
         case Instruction::FAdd:
         case Instruction::FMul:
         {
            return true;
         }
         default:
         {
            return false;
      }
   }
   }

   class Node
   {
    public:
      explicit Node(Value* V)
          : Inst(nullptr), Opcode(0), DefOp(V), Parent(nullptr), Left(nullptr), Right(nullptr), Latency(0), TotalCost(0)
      {
         if(auto* I = dyn_cast<Instruction>(V))
         {
            Inst = I;
            Opcode = I->getOpcode();
         }
      }

      static std::map<std::pair<std::string, std::string>, double> InstructionLatencyTable;

      // BFS: Breadth First Search
      static std::vector<Node*> getNodesAndLeavesByBFS(Node* N)
      {
         std::vector<Node*> Nodes = {N};

         for(unsigned i = 0; i < Nodes.size(); ++i)
         {
            Node* CurNode = Nodes[i];
            if(Node* L = CurNode->getLeft())
            {
               Nodes.push_back(L);
            }
            if(Node* R = CurNode->getRight())
            {
               Nodes.push_back(R);
         }
         }

         return Nodes;
      }

      /// Set parent node of current node.
      void setParent(Node* P)
      {
         Parent = P;
      }

      /// Set left node of current node.
      void setLeft(Node* L)
      {
         Left = L;
      }

      /// Set right node of current node.
      void setRight(Node* R)
      {
         Right = R;
      }

      /// Set latency of current node.
      void setLatency(double L)
      {
         Latency = L;
      }

      /// Set total cost of current node.
      void setTotalCost(int C)
      {
         TotalCost = C;
      }

      /// Get original instruction of current node.
      Instruction* getOrgInst() const
      {
         assert(Inst && "Inst should not be nullptr.");
         return Inst;
      }

      /// Get parent node of current node.
      Node* getParent() const
      {
         return Parent;
      }

      /// Get left node of current node.
      Node* getLeft() const
      {
         return Left;
      }

      /// Get right node of current node.
      Node* getRight() const
      {
         return Right;
      }

      /// Get operator code of current node.
      unsigned getOpcode() const
      {
         return Opcode;
      }

      /// Get latency of current node.
      double getLatency() const
      {
         return Latency;
      }

      /// Get total cost of current node.
      double getTotalCost() const
      {
         return TotalCost;
      }

      /// Get defined value of current node.
      Value* getDefinedValue() const
      {
         assert(DefOp && "DefOp should not be nullptr");
         return DefOp;
      }

      /// Return true if current node is a node.
      bool isNode() const
      {
         return Left != nullptr && Right != nullptr;
      }

      /// Return true if current node is a pure leaf.
      bool isLeaf() const
      {
         return !isNode();
      }

      bool isPhi() const
      {
         return Inst && isa<PHINode>(Inst);
      }

      /// Return true if current node is a root.
      bool isRoot() const
      {
         return Parent == nullptr;
      }

      /// Return true if current node is considered as a leaf node.
      /// Tree height reduction is applied only to nodes whose operator code is
      /// same, so it is necessary to consider a node whose operation is not
      /// commutative or whose operator code is different from its parent's
      /// operator code as a leaf node.
      bool isConsideredAsLeaf() const
      {
         if(isLeaf())
         {
            return true;
         }
         if(!getOrgInst()->isCommutative())
         {
            return true;
         }
         if(!isRoot() && getOpcode() != getParent()->getOpcode())
         {
            return true;
         }
         return false;
      }

      /// Update tree's latecy under current node.
      void updateTreeLatency()
      {
         std::vector<Node*> Nodes = getNodesAndLeavesByBFS(this);
         // Update node latency by bottom-up order.
         for(auto Begin = Nodes.rbegin(), End = Nodes.rend(); Begin != End; ++Begin)
         {
            Node* CurNode = *Begin;
            assert(CurNode);
            CurNode->updateNodeLatency();
         }
      }

      double getInstructionLatency(const Instruction* I) const
      {
         const auto getCost = [&](unsigned bits, const std::string& op) -> double {
            auto key = std::make_pair(op, STR(bits));
            if(InstructionLatencyTable.find(key) != InstructionLatencyTable.end())
            {
               return InstructionLatencyTable.find(key)->second;
            }
            else
            {
               llvm::errs() << "latency not defined for " << op << " " << bits << "\n";
               return 0.0;
            }
         };
         auto ty = I->getType();
         auto bits = resize_to_1_8_16_32_64(ty->getScalarSizeInBits());
         switch(I->getOpcode())
         {
            case Instruction::GetElementPtr:
            case Instruction::Add:
            case Instruction::Sub:
            {
               return getCost(bits, "plus_expr");
            }
            case Instruction::Mul:
            {
               return getCost(bits, "mult_expr");
            }
            case Instruction::PHI:
            case Instruction::Ret:
            case Instruction::Br:
            {
               return 0;
            }
            case Instruction::FAdd:
            case Instruction::FSub:
            {
               return getCost(bits, "Fplus_expr");
            }
            case Instruction::FMul:
            {
               return getCost(bits, "Fmult_expr");
            }
            case Instruction::UDiv:
            case Instruction::SDiv:
            {
               return getCost(bits, "trunc_div_expr");
            }
            case Instruction::FDiv:
            {
               return getCost(bits, "Frdiv_expr");
            }
            case Instruction::URem:
            case Instruction::SRem:
            {
               return getCost(bits, "trunc_mod_expr");
            }
            case Instruction::FRem:
            {
               llvm_unreachable("floating point remainder not foreseen yet");
               return 0;
            }
            case Instruction::Shl:
            {
               return getCost(bits, "lshift_expr");
            }
            case Instruction::LShr:
            case Instruction::AShr:
            {
               return getCost(bits, "rshift_expr");
            }
            case Instruction::And:
            {
               return getCost(bits, "bit_and_expr");
            }
            case Instruction::Or:
            {
               return getCost(bits, "bit_ior_expr");
            }
            case Instruction::Xor:
            {
               return getCost(bits, "bit_xor_expr");
            }
#if __clang_major__ >= 10
            case Instruction::FNeg:
            {
               return 0;
            }
#endif
            case Instruction::Select:
            {
               return getCost(bits, "cond_expr");
            }
            case Instruction::ICmp:
            {
               return 0;
            }
            case Instruction::FCmp:
            {
               // simplified
               return getCost(bits, "Fplus_expr");
            }
            case Instruction::Store:
            {
               return getCost(32, "store_expr");
            }
            case Instruction::Load:
            {
               return getCost(32, "load_expr");
            }
            case Instruction::ZExt:
            case Instruction::SExt:
            {
               return 0;
            }
            case Instruction::PtrToInt:
            case Instruction::IntToPtr:
            case Instruction::Trunc:
            {
               return 0;
            }
            case Instruction::FPToUI:
            case Instruction::FPToSI:
            case Instruction::FPExt:
            case Instruction::SIToFP:
            case Instruction::UIToFP:
            case Instruction::FPTrunc:
            {
               return getCost(32, "nop_expr");
            }

            case Instruction::BitCast:
            case Instruction::AddrSpaceCast:
            case Instruction::ExtractElement:
            case Instruction::InsertElement:
            case Instruction::ExtractValue:
            case Instruction::ShuffleVector:
            {
               return 0;
            }
            case Instruction::Call:
            case Instruction::Switch:
            {
               return 0;
            }
            default:
            {
               // We don't have any information on this instruction.
               return 0;
         }
      }
      }

      /// Update current node's latency.
      void updateNodeLatency()
      {
         if(isPhi())
         {
            setLatency(100000);
            setTotalCost(100000);
         }
         else
         {
         setLatency(0);
         setTotalCost(0);
         }
         if(isLeaf())
         {
            return;
         }

         updateLeftOrRightNodeLatency(UpdateLatecy::UL_Left);
         updateLeftOrRightNodeLatency(UpdateLatecy::UL_Right);

         auto* _Inst = dyn_cast<Instruction>(getDefinedValue());
         auto InstLatency = _Inst ? getInstructionLatency(_Inst) : 0;
         setLatency(getLatency() + InstLatency);
         setTotalCost(getTotalCost() + InstLatency);
      }

      enum struct UpdateLatecy
      {
         UL_Left,
         UL_Right
      };

      // Update left or right node's latency of current node.
      void updateLeftOrRightNodeLatency(UpdateLatecy UL)
      {
         const auto _Latency = getLatency();

         Node* SubNode = nullptr;
         switch(UL)
         {
            case UpdateLatecy::UL_Left:
            {
               SubNode = getLeft();
               break;
            }
            case UpdateLatecy::UL_Right:
            {
               SubNode = getRight();
               break;
            }
            default:
            {
               llvm_unreachable("Should not reach here.");
         }
         }

         assert(SubNode && "Left or right node should not be nullptr.");
         const auto SubNodeLatency = SubNode->getLatency();
         if(SubNodeLatency > _Latency)
         {
            setLatency(SubNodeLatency);
         }
         setTotalCost(getTotalCost() + SubNode->getTotalCost());
      }

    private:
      // An original instruction of current node.
      Instruction* Inst;
      // A operator code of current node.
      unsigned Opcode;

      // A defined operand used at current node.
      // This also includes constant value.
      Value* DefOp;

      // A parent node of current node.
      Node* Parent;
      // A left node and a right node of current node.
      Node *Left, *Right;

      // Instruction latency.
      double Latency;
      // Total cost of nodes under current nodes.
      double TotalCost;
   };

   static std::vector<Node*> getOnlyNodes(Node* N)
   {
      std::vector<Node*> Nodes = Node::getNodesAndLeavesByBFS(N);

      // Exclude only leaves.
      auto Iter = Nodes.begin();
      while(Iter != Nodes.end())
      {
         if((*Iter)->isNode())
         {
            ++Iter;
         }
         else
         {
            Iter = Nodes.erase(Iter);
         }
      }
      return Nodes;
   }

   static void setLeftAndRightOperand(Value*& LeftOp, Value*& RightOp, Node* N, std::queue<Value*>& Ops)
   {
      auto frontPopVal = [](std::queue<Value*>& Ops) -> Value* {
         Value* V = Ops.front();
         Ops.pop();
         return V;
      };

      Node* L = N->getLeft();
      assert(L);
      Node* R = N->getRight();
      assert(R);

      if(L->isLeaf() && R->isLeaf())
      {
         // Both left child and right child of 'N' are leaves.
         LeftOp = L->getDefinedValue();
         RightOp = R->getDefinedValue();
      }
      else
      {
         assert(!Ops.empty());
         if(L->isLeaf())
         {
            // Left child is a leaf and right child is a node.
            LeftOp = L->getDefinedValue();
            RightOp = frontPopVal(Ops);
         }
         else if(R->isLeaf())
         {
            // Left child is a node and right child is a leaf.
            LeftOp = frontPopVal(Ops);
            RightOp = R->getDefinedValue();
         }
         else
         {
            // Both left child and right child of 'N' are nodes.
            LeftOp = frontPopVal(Ops);
            RightOp = frontPopVal(Ops);
         }
      }
   }

   std::map<std::pair<std::string, std::string>, double> Node::InstructionLatencyTable;

   class TreeHeightReduction
   {
      void __split(std::vector<std::string>& cont, const std::string& str, char delim)
      {
         std::stringstream ss(str);
         std::string token;
         while(std::getline(ss, token, delim))
         {
            cont.push_back(token);
      }
      }
      void __buildMap(const std::string& input, std::map<std::pair<std::string, std::string>, double>& _map)
      {
         std::vector<std::string> vec_value;
         __split(vec_value, input, ',');
         for(auto& el : vec_value)
         {
            std::vector<std::string> vec_pair;
            __split(vec_pair, el, '=');
            std::vector<std::string> key_pair;
            __split(key_pair, vec_pair.at(0), '|');
            _map[std::make_pair(key_pair.at(0), key_pair.at(1))] = std::atof(vec_pair.at(1).c_str());
         }
      }

    public:
      explicit TreeHeightReduction() = default;

      bool runOnModule(const llvm::Module& M, llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> _GetLI,
                       llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> _GetORE,
                       const std::string& costTable, bool DisableIntTHR = false, bool EnableFpTHR = true)
      {
         __buildMap(costTable, Node::InstructionLatencyTable);
         bool changed = false;
         for(auto& fun : M.getFunctionList())
         {
            if(!fun.isIntrinsic() && !fun.isDeclaration())
            {
               auto* currentFunction = const_cast<llvm::Function*>(&fun);
               auto& LI = _GetLI(*currentFunction);
               auto ORE = &_GetORE(*currentFunction);
               if(!LI.empty())
               {
                  for(auto L : LI)
                  {
                     // Tree height reduction is applied only to inner-most loop.
                     SmallVector<Loop*, 4> Worklist;
                     for(Loop* CurLoop : llvm::depth_first(L))
                     {
#if __clang_major__ >= 12
                        if(CurLoop->isInnermost())
#else
                        if(CurLoop->empty())
#endif
                        {
                           Worklist.push_back(CurLoop);
                        }
                     }
                     for(Loop* l : Worklist)
                     {
                        auto& LoopBlocks = l->getBlocksVector();
                        for(auto* BB : LoopBlocks)
                        {
                           if(!DisableIntTHR)
                           {
                              changed |= runOnBasicBlock(BB, TargetInstTy::INTEGER, ORE);
                           }
                           if(EnableFpTHR)
                           {
                              changed |= runOnBasicBlock(BB, TargetInstTy::FLOATING_POINT, ORE);
                        }
                     }
                  }
               }
               }
               else if(!currentFunction->empty())
               {
                  for(auto& BB : *currentFunction)
                  {
                     if(!DisableIntTHR)
                     {
                        changed |= runOnBasicBlock(&BB, TargetInstTy::INTEGER, ORE);
                     }
                     if(EnableFpTHR)
                     {
                        changed |= runOnBasicBlock(&BB, TargetInstTy::FLOATING_POINT, ORE);
                  }
               }
            }
         }
         }
         return changed;
      }

    private:
      // 'GeneratedInsts' has instructions generated when tree height reduction
      // is applied. If it is not applied, 'GeneratedInsts' has nothing.
      std::set<Instruction*> GeneratedInsts;

      enum struct TargetInstTy
      {
         INTEGER,
         FLOATING_POINT
      };

      static bool isLegalToApply(Node* Root)
      {
         std::vector<Node*> Worklist = Node::getNodesAndLeavesByBFS(Root);
         int NumLeaves = 0;

         for(auto* N : Worklist)
         {
            if(N->isLeaf())
            {
               ++NumLeaves;
            }
            // We consider that it is worth applying tree height reduction
            // if the number of leaves is equal to or more than 2.
            if(NumLeaves >= 2)
            {
               return true;
         }
         }

         return false;
      }

      static void eraseOrgInsts(std::vector<Instruction*>& Insts)
      {
         for(auto* I : Insts)
         {
            if(I)
            {
               I->eraseFromParent();
      }
         }
      }

      // Tree height reduction is applied to each basic block which
      // innermost loop contains.
      bool runOnBasicBlock(BasicBlock* BB, const TargetInstTy TIT, OptimizationRemarkEmitter* ORE)
      {
         bool Applied = false;

         // Search target instruction in basic block reversely.
         for(auto Begin = BB->rbegin(); Begin != BB->rend(); ++Begin)
         {
            Instruction& I = *Begin;
            if(GeneratedInsts.count(&I) == 1)
            {
               continue;
            }
            if(!isRootCandidate(I, TIT))
            {
               continue;
            }

            // Construct operation tree from root instruction.
            auto* V = dyn_cast<Value>(&I);
            assert(V && "Defined value should not be nullptr.");
            Node* OrgTree = constructTree(V, TIT);
            if(!OrgTree)
            {
               continue;
            }

            if(!isLegalToApply(OrgTree))
            {
               destructTree(OrgTree);
               continue;
            }

            std::vector<Instruction*> Insts;
            collectInstsToBeErasedFrom(OrgTree, Insts);

            // Apply tree height reduction to constructed tree 'OrgTree',
            // and get tree 'ReducedTree' to which it is applied.
            Node* ReducedTree = applyTreeHeightReduction(OrgTree, true, TIT);
            if(!ReducedTree)
            {
               destructTree(OrgTree);
               continue;
            }

            assert(ReducedTree->getOrgInst() == OrgTree->getOrgInst() &&
                   "OrgInst of ReducedTree and OrgTree should be same.");

            // Create IRs from tree to which tree height reduction is applied.
            if(createIRs(ReducedTree))
            {
               // The following optimization message output process must be called
               // before 'eraseOrgInsts' because 'I' is erase there.
               ORE->emit([&]() {
                  return OptimizationRemark(DEBUG_TYPE, "TreeHeightReduction", &I) << "reduced tree height";
               });

               --Begin;
               eraseOrgInsts(Insts);
               Applied = true;
            }

            // 'OrgTree' and 'ReducedTree' shares memory, so it is enough to
            // release memory of 'ReducedTree'.
            destructTree(ReducedTree);
         }

         GeneratedInsts.clear();
         return Applied;
      }

      // Return true if 'I' is a target instruction of tree height reduction.
      bool isTHRTargetInst(Instruction* I, TargetInstTy CurTargetInstTy) const
      {
         switch(CurTargetInstTy)
         {
            case TargetInstTy::INTEGER:
            {
               return isIntgerInstTHRTarget(I);
            }
            case TargetInstTy::FLOATING_POINT:
            {
               return isFpInstTHRTarget(I);
            }
            default:
            {
               return false;
         }
      }
      }

      // Construct operation tree from value 'V'.
      Node* constructTree(Value* V, TargetInstTy CurTargetInstTy)
      {
         if(!isNodeCandidate(V, CurTargetInstTy))
         {
            return new Node(V);
         }

         auto* I = dyn_cast<Instruction>(V);
         assert(I && "Instruction should not be nullptr.");
         assert(I->getNumOperands() == 2 && "The number of operands should be 2.");

         Node* Parent = new Node(V);
         assert(Parent && "Should not be nullptr.");

         Value* LeftOp = I->getOperand(0);
         Node* Left = constructTree(LeftOp, CurTargetInstTy);
         assert(Left && "Should not be nullptr.");

         if(Left)
         {
            Parent->setLeft(Left);
            Left->setParent(Parent);
         }

         Value* RightOp = I->getOperand(1);
         Node* Right = constructTree(RightOp, CurTargetInstTy);
         assert(Right && "Should not be nullptr.");

         if(Right)
         {
            Parent->setRight(Right);
            Right->setParent(Parent);
         }

         return Parent;
      }

      void destructTree(Node* N)
      {
         std::vector<Node*> Nodes = Node::getNodesAndLeavesByBFS(N);
         for(auto* CurNode : Nodes)
         {
            delete CurNode;
      }
      }

      // Collect original instructions to be erased from BasicBlock.
      void collectInstsToBeErasedFrom(Node* N, std::vector<Instruction*>& Insts)
      {
         std::vector<Node*> Nodes = Node::getNodesAndLeavesByBFS(N);
         for(auto* CurNode : Nodes)
         {
            // Instruction belonging to leaf node should be saved.
            if(!CurNode->isLeaf())
            {
               Insts.push_back(CurNode->getOrgInst());
            }
      }
      }

      // Apply tree height reduction to 'N'.
      // Return value is a tree to which tree height reduction is applied.
      Node* applyTreeHeightReduction(Node* N, bool isLeft, TargetInstTy CurTargetInstTy)
      {
         // Postorder depth-first search.
         if(!N->isNode())
         {
            return N;
         }

         if(Node* Left = N->getLeft())
         {
            applyTreeHeightReduction(Left, true, CurTargetInstTy);
         }
         if(Node* Right = N->getRight())
         {
            applyTreeHeightReduction(Right, false, CurTargetInstTy);
         }

         if(!isTHRTargetInst(N->getOrgInst(), CurTargetInstTy))
         {
            return N;
         }

         // Save original parent information.
         Node* Parent = N->getParent();

         std::vector<Node*> Leaves;
         // 'ReusedNodes' holds nodes which is reused when updating parent and child
         // node's relationship at the time of reducing operation tree.
         // By doing so, the amount of memory used can be reduced.
         std::vector<Node*> ReusedNodes;
         collectLeavesAndReusedNodes(N, Leaves, ReusedNodes);

         Node* NewNode = constructOptimizedSubtree(Leaves, ReusedNodes, N);
         NewNode->setParent(Parent);
         if(Parent)
         {
            if(isLeft)
            {
               Parent->setLeft(NewNode);
            }
            else
            {
               Parent->setRight(NewNode);
         }
         }
         // Return value has meaning only if 'Parent' is nullptr because
         // this means 'Node' is a root node.
         return NewNode;
      }

      // Collect leaf nodes and reused nodes under node 'N'.
      void collectLeavesAndReusedNodes(Node* N, std::vector<Node*>& Leaves, std::vector<Node*>& ReusedNodes)
      {
         std::vector<Node*> Worklist = {N};

         // NOTE: Don't use 'getNodesAndLeavesByBFS'! Like this function, the below
         //       processing is the one which collects all nodes and all leaves by
         //       BFS, but it is a little different from this function because
         //       it is BFS with a condition.
         for(unsigned i = 0; i < Worklist.size(); ++i)
         {
            Node* CurNode = Worklist[i];
            if(CurNode->isConsideredAsLeaf())
            {
               Leaves.push_back(CurNode);
               continue;
            }

            ReusedNodes.push_back(CurNode);
            if(Node* Left = CurNode->getLeft())
            {
               Worklist.push_back(Left);
            }
            if(Node* Right = CurNode->getRight())
            {
               Worklist.push_back(Right);
         }
      }
      }

      // Apply tree height reduction to under a node 'N', and construct
      // optimized tree.
      Node* constructOptimizedSubtree(std::vector<Node*>& Leaves, std::vector<Node*>& ReusedNodes, Node* N)
      {
         auto removeNodeFromLeaves = [&](Node* N) -> void {
            auto Begin = Leaves.begin();
            while(Begin != Leaves.end())
            {
               if(*Begin == N)
               {
                  Begin = Leaves.erase(Begin);
               }
               else
               {
                  ++Begin;
               }
            }
         };

         while(Leaves.size() > 1)
         {
            std::sort(Leaves.begin(), Leaves.end(), [](Node* LHS, Node* RHS) -> bool {
               if(LHS->getLatency() != RHS->getLatency())
               {
                  return LHS->getLatency() < RHS->getLatency();
               }
               if(LHS->getTotalCost() != RHS->getTotalCost())
               {
                  return LHS->getTotalCost() < RHS->getTotalCost();
               }
               return false;
            });
            // printLeaves(Leaves, true);

            Node *Op1 = Leaves[0], *Op2 = Leaves[1];
            combineLeaves(Leaves, Op1, Op2, ReusedNodes);
            removeNodeFromLeaves(Op1);
            removeNodeFromLeaves(Op2);

            // printLeaves(Leaves, false);
         }
         // llvm::errs() << "Completed\n";

         Node* NewNode = Leaves.front();
         NewNode->updateTreeLatency();
         return NewNode;
      }

      // Combine 2 leaf elements, create node from them and put it into 'Leaves'.
      void combineLeaves(std::vector<Node*>& Leaves, Node* Op1, Node* Op2, std::vector<Node*>& ReusedNodes)
      {
         Node* N = ReusedNodes.back();
         assert(N);
         assert(Op1);
         assert(Op2);
         ReusedNodes.pop_back();

         // Update child and its parent relationship.
         N->setLeft(Op1);
         N->setRight(Op2);
         Op1->setParent(N);
         Op2->setParent(N);

         N->updateTreeLatency();
         Leaves.push_back(N);
      }

      // Create IRs from root node.
      bool createIRs(Node* Root)
      {
         Instruction* RootInst = Root->getOrgInst();
         assert(RootInst);
         IRBuilder<> Builder(RootInst);
         std::vector<Node*> Nodes = getOnlyNodes(Root);
         std::queue<Value*> Ops;
         while(!Nodes.empty())
         {
            Node* N = Nodes.back();
            assert(N);

            Value *LeftOp = nullptr, *RightOp = nullptr;
            setLeftAndRightOperand(LeftOp, RightOp, N, Ops);

            Value* NewNodeValue = createInst(Builder, N, LeftOp, RightOp);
            Ops.push(NewNodeValue);
            GeneratedInsts.insert(dyn_cast<Instruction>(NewNodeValue));

            Nodes.pop_back();
         }

         assert(Ops.size() == 1 && "The size of queue should be 1.");
         Value* NewRootValue = Ops.front();
         GeneratedInsts.insert(dyn_cast<Instruction>(NewRootValue));
         RootInst->replaceAllUsesWith(NewRootValue);
         return true;
      }

      // Create optimized integer or floating-point instructions from 'Node'.
      // 'Ops' has a operand values.
      Value* createInst(IRBuilder<>& Builder, Node* N, Value* Op1, Value* Op2)
      {
         Value* V;
         switch(N->getOpcode())
         {
            case Instruction::Add:
            {
               V = Builder.CreateAdd(Op1, Op2, Twine("thr.add"));
               break;
            }
            case Instruction::Mul:
            {
               V = Builder.CreateMul(Op1, Op2, Twine("thr.mul"));
               break;
            }
            case Instruction::FAdd:
            {
               V = Builder.CreateFAdd(Op1, Op2, Twine("thr.fadd"));
               break;
            }
            case Instruction::FMul:
            {
               V = Builder.CreateFMul(Op1, Op2, Twine("thr.fmul"));
               break;
            }
            default:
            {
               assert(0);
               return nullptr;
         }
         }

         // Take over the original instruction IR flags.
         auto* NewInst = dyn_cast<Instruction>(V);
         if(Value* OrgDef = N->getDefinedValue())
         {
            NewInst->copyIRFlags(OrgDef, true);
         }

         return V;
      }

      // Return true if 'I' is a root candidate.
      bool isRootCandidate(Instruction& I, TargetInstTy CurTargetInstTy) const
      {
         if(!isTHRTargetInst(&I, CurTargetInstTy))
         {
            return false;
         }
         for(unsigned i = 0; i < I.getNumOperands(); ++i)
         {
            if(isNodeCandidate(I.getOperand(i), CurTargetInstTy))
            {
               return true;
            }
         }
         return false;
      }

      // Return true if 'Op' is a node candidate.
      bool isNodeCandidate(Value* Op, TargetInstTy CurTargetInstTy) const
      {
         assert(Op && "Operand should not be nullptr");
         if(!Op->hasOneUse())
         {
            for(auto U : Op->users())
            {
               auto* UserInst = dyn_cast<Instruction>(U);
               if(UserInst && isTHRTargetInst(UserInst, CurTargetInstTy))
               {
            return false;
         }
            }
            return true;
         }
         if(auto* I = dyn_cast<Instruction>(Op))
         {
            return isTHRTargetInst(I, CurTargetInstTy);
         }
         return false;
      }

      void printTree(Node* N, const int Indent) const
      {
         for(int i = 0; i < Indent; ++i)
         {
            errs() << " ";
         }
         auto _Inst = dyn_cast<Instruction>(N->getDefinedValue());
         if(_Inst)
         {
            _Inst->print(llvm::errs(), true);
         }
         else
         {
            N->getDefinedValue()->print(llvm::errs(), true);
         }
         for(int i = 0; i < Indent + 4; ++i)
         {
            errs() << " ";
         }
         errs() << "Latency: " << N->getLatency();
         errs() << ", TotalCost: " << N->getTotalCost() << "\n";

         if(Node* Left = N->getLeft())
         {
            printTree(Left, Indent + 2);
         }
         if(Node* Right = N->getRight())
         {
            printTree(Right, Indent + 2);
      }
      }

      void printLeaves(const std::vector<Node*>& Leaves, bool isBefore) const
      {
         if(isBefore)
         {
            errs() << "  --- Before ---\n";
         }
         else
         {
            errs() << "  --- After ---\n";
         }
         for(auto* Node : Leaves)
         {
            printTree(Node, 2);
      }
      }
   };

} // namespace llvm
