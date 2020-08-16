//===-- PredicateInfo.cpp - PredicateInfo Builder--------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file implements the eSSA class.
//
//===----------------------------------------------------------------===//
#include "eSSA.hpp"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#ifdef _WIN32
#include "llvm/Analysis/OrderedInstructions.h"
#elif __clang_major__ == 4
#include "my_OrderedInstructions.hpp"
#elif __clang_major__ > 4 && __clang_major__ < 8
#include "llvm/Transforms/Utils/OrderedInstructions.h"
#else
#include "llvm/Analysis/OrderedInstructions.h"
#endif

#include "llvm/IR/IRBuilder.h"

namespace eSSAInfoClasses
{
   enum PredicateType
   {
      PT_Branch,
      PT_Assume,
      PT_Switch
   };

   // Base class for all predicate information we provide.
   // All of our predicate information has at least a comparison.
   class PredicateBase : public llvm::ilist_node<PredicateBase>
   {
    public:
      PredicateType Type;
      // The original operand before we renamed it.
      // This can be use by passes, when destroying predicateinfo, to know
      // whether they can just drop the intrinsic, or have to merge metadata.
      llvm::Value* OriginalOp;
      PredicateBase(const PredicateBase&) = delete;
      PredicateBase& operator=(const PredicateBase&) = delete;
      PredicateBase() = delete;
      virtual ~PredicateBase() = default;

    protected:
      PredicateBase(PredicateType PT, llvm::Value* Op) : Type(PT), OriginalOp(Op)
      {
      }
   };

   class PredicateWithCondition : public PredicateBase
   {
    public:
      llvm::Value* Condition;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == PT_Assume || PB->Type == PT_Branch || PB->Type == PT_Switch;
      }

    protected:
      PredicateWithCondition(PredicateType PT, llvm::Value* Op, llvm::Value* Condition) : PredicateBase(PT, Op), Condition(Condition)
      {
      }
   };

   // Provides predicate information for assumes.  Since assumes are always true,
   // we simply provide the assume instruction, so you can tell your relative
   // position to it.
   class PredicateAssume : public PredicateWithCondition
   {
    public:
      llvm::IntrinsicInst* AssumeInst;
      PredicateAssume(llvm::Value* Op, llvm::IntrinsicInst* AssumeInst, llvm::Value* Condition) : PredicateWithCondition(PT_Assume, Op, Condition), AssumeInst(AssumeInst)
      {
      }
      PredicateAssume() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == PT_Assume;
      }
   };

   // Mixin class for edge predicates.  The FROM block is the block where the
   // predicate originates, and the TO block is the block where the predicate is
   // valid.
   class PredicateWithEdge : public PredicateWithCondition
   {
    public:
      llvm::BasicBlock* From;
      llvm::BasicBlock* To;
      PredicateWithEdge() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == PT_Branch || PB->Type == PT_Switch;
      }

    protected:
      PredicateWithEdge(PredicateType PType, llvm::Value* Op, llvm::BasicBlock* From, llvm::BasicBlock* To, llvm::Value* Cond) : PredicateWithCondition(PType, Op, Cond), From(From), To(To)
      {
      }
   };

   // Provides predicate information for branches.
   class PredicateBranch : public PredicateWithEdge
   {
    public:
      // If true, SplitBB is the true successor, otherwise it's the false successor.
      bool TrueEdge;
      PredicateBranch(llvm::Value* Op, llvm::BasicBlock* BranchBB, llvm::BasicBlock* SplitBB, llvm::Value* Condition, bool TakenEdge) : PredicateWithEdge(PT_Branch, Op, BranchBB, SplitBB, Condition), TrueEdge(TakenEdge)
      {
      }
      PredicateBranch() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == PT_Branch;
      }
   };

   class PredicateSwitch : public PredicateWithEdge
   {
    public:
      llvm::Value* CaseValue;
      // This is the switch instruction.
      llvm::SwitchInst* Switch;
      PredicateSwitch(llvm::Value* Op, llvm::BasicBlock* SwitchBB, llvm::BasicBlock* TargetBB, llvm::Value* CaseValue, llvm::SwitchInst* SI) : PredicateWithEdge(PT_Switch, Op, SwitchBB, TargetBB, SI->getCondition()), CaseValue(CaseValue), Switch(SI)
      {
      }
      PredicateSwitch() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == PT_Switch;
      }
   };
   // Given a predicate info that is a type of branching terminator, get the
   // branching block.
   const llvm::BasicBlock* getBranchBlock(const PredicateBase* PB)
   {
      assert(llvm::isa<PredicateWithEdge>(PB) && "Only branches and switches should have PHIOnly defs that "
                                                 "require branch blocks.");
      return llvm::cast<PredicateWithEdge>(PB)->From;
   }

   // Given a predicate info that is a type of branching terminator, get the
   // branching terminator.
   //    static llvm::Instruction* getBranchTerminator(const PredicateBase* PB)
   //    {
   //       assert(llvm::isa<PredicateWithEdge>(PB) && "Not a predicate info type we know how to get a terminator from.");
   //       return llvm::cast<PredicateWithEdge>(PB)->From->getTerminator();
   //    }

   // Given a predicate info that is a type of branching terminator, get the
   // edge this predicate info represents
   const std::pair<llvm::BasicBlock*, llvm::BasicBlock*> getBlockEdge(const PredicateBase* PB)
   {
      assert(llvm::isa<PredicateWithEdge>(PB) && "Not a predicate info type we know how to get an edge from.");
      const auto* PEdge = llvm::cast<PredicateWithEdge>(PB);
      return std::make_pair(PEdge->From, PEdge->To);
   }

   enum LocalNum
   {
      // Operations that must appear first in the block.
      LN_First,
      // Operations that are somewhere in the middle of the block, and are sorted on
      // demand.
      LN_Middle,
      // Operations that must appear last in a block, like successor phi node uses.
      LN_Last
   };

   // Associate global and local DFS info with defs and uses, so we can sort them
   // into a global domination ordering.
   struct ValueDFS
   {
      int DFSIn = 0;
      int DFSOut = 0;
      unsigned int LocalNum = LN_Middle;
      // Only one of Def or Use will be set.
      llvm::Value* Def = nullptr;
      llvm::Use* U = nullptr;
      // Neither PInfo nor EdgeOnly participate in the ordering
      PredicateBase* PInfo = nullptr;
      bool EdgeOnly = false;
   };

   // Perform a strict weak ordering on instructions and arguments.
   static bool valueComesBefore(llvm::OrderedInstructions& OI, const llvm::Value* A, const llvm::Value* B)
   {
      auto* ArgA = llvm::dyn_cast_or_null<llvm::Argument>(A);
      auto* ArgB = llvm::dyn_cast_or_null<llvm::Argument>(B);
      if(ArgA && !ArgB)
      {
         return true;
      }
      if(ArgB && !ArgA)
      {
         return false;
      }
      if(ArgA && ArgB)
      {
         return ArgA->getArgNo() < ArgB->getArgNo();
      }
      return OI.dominates(llvm::cast<llvm::Instruction>(A), llvm::cast<llvm::Instruction>(B));
   }

   // This compares ValueDFS structures, creating OrderedBasicBlocks where
   // necessary to compare uses/defs in the same block.  Doing so allows us to walk
   // the minimum number of instructions necessary to compute our def/use ordering.
   struct ValueDFS_Compare
   {
      llvm::OrderedInstructions& OI;
      ValueDFS_Compare(llvm::OrderedInstructions& OI) : OI(OI)
      {
      }

      bool operator()(const ValueDFS& A, const ValueDFS& B) const
      {
         if(&A == &B)
         {
            return false;
         }
         // The only case we can't directly compare them is when they in the same
         // block, and both have localnum == middle.  In that case, we have to use
         // comesbefore to see what the real ordering is, because they are in the
         // same basic block.

         bool SameBlock = std::tie(A.DFSIn, A.DFSOut) == std::tie(B.DFSIn, B.DFSOut);

         // We want to put the def that will get used for a given set of phi uses,
         // before those phi uses.
         // So we sort by edge, then by def.
         // Note that only phi nodes uses and defs can come last.
         if(SameBlock && A.LocalNum == LN_Last && B.LocalNum == LN_Last)
         {
            return comparePHIRelated(A, B);
         }

         if(!SameBlock || A.LocalNum != LN_Middle || B.LocalNum != LN_Middle)
         {
            return std::tie(A.DFSIn, A.DFSOut, A.LocalNum, A.Def, A.U) < std::tie(B.DFSIn, B.DFSOut, B.LocalNum, B.Def, B.U);
         }
         return localComesBefore(A, B);
      }

      // For a phi use, or a non-materialized def, return the edge it represents.
      const std::pair<llvm::BasicBlock*, llvm::BasicBlock*> getBlockEdge_local(const ValueDFS& VD) const
      {
         if(!VD.Def && VD.U)
         {
            auto* PHI = llvm::cast<llvm::PHINode>(VD.U->getUser());
            return std::make_pair(PHI->getIncomingBlock(*VD.U), PHI->getParent());
         }
         // This is really a non-materialized def.
         return getBlockEdge(VD.PInfo);
      }

      // For two phi related values, return the ordering.
      bool comparePHIRelated(const ValueDFS& A, const ValueDFS& B) const
      {
         auto& ABlockEdge = getBlockEdge_local(A);
         auto& BBlockEdge = getBlockEdge_local(B);
         // Now sort by block edge and then defs before uses.
         return std::tie(ABlockEdge, A.Def, A.U) < std::tie(BBlockEdge, B.Def, B.U);
      }

      // Get the definition of an instruction that occurs in the middle of a block.
      llvm::Value* getMiddleDef(const ValueDFS& VD) const
      {
         if(VD.Def)
         {
            return VD.Def;
         }
         // It's possible for the defs and uses to be null.  For branches, the local
         // numbering will say the placed predicaeinfos should go first (IE
         // LN_beginning), so we won't be in this function. For assumes, we will end
         // up here, beause we need to order the def we will place relative to the
         // assume.  So for the purpose of ordering, we pretend the def is the assume
         // because that is where we will insert the info.
         if(!VD.U)
         {
            assert(VD.PInfo && "No def, no use, and no predicateinfo should not occur");
            assert(llvm::isa<PredicateAssume>(VD.PInfo) && "Middle of block should only occur for assumes");
            return llvm::cast<PredicateAssume>(VD.PInfo)->AssumeInst;
         }
         return nullptr;
      }

      // Return either the Def, if it's not null, or the user of the Use, if the def
      // is null.
      const llvm::Instruction* getDefOrUser(const llvm::Value* Def, const llvm::Use* U) const
      {
         if(Def)
         {
            return llvm::cast<llvm::Instruction>(Def);
         }
         return llvm::cast<llvm::Instruction>(U->getUser());
      }

      // This performs the necessary local basic block ordering checks to tell
      // whether A comes before B, where both are in the same basic block.
      bool localComesBefore(const ValueDFS& A, const ValueDFS& B) const
      {
         auto* ADef = getMiddleDef(A);
         auto* BDef = getMiddleDef(B);

         // See if we have real values or uses. If we have real values, we are
         // guaranteed they are instructions or arguments. No matter what, we are
         // guaranteed they are in the same block if they are instructions.
         auto* ArgA = llvm::dyn_cast_or_null<llvm::Argument>(ADef);
         auto* ArgB = llvm::dyn_cast_or_null<llvm::Argument>(BDef);

         if(ArgA || ArgB)
         {
            return valueComesBefore(OI, ArgA, ArgB);
         }

         auto* AInst = getDefOrUser(ADef, A.U);
         auto* BInst = getDefOrUser(BDef, B.U);
         return valueComesBefore(OI, AInst, BInst);
      }
   };
   using ValueDFSStack = llvm::SmallVectorImpl<ValueDFS>;

   bool stackIsInScope(const ValueDFSStack& Stack, const ValueDFS& VDUse, const llvm::DominatorTree* DT)
   {
      if(Stack.empty())
      {
         return false;
      }
      // If it's a phi only use, make sure it's for this phi node edge, and that the
      // use is in a phi node.  If it's anything else, and the top of the stack is
      // EdgeOnly, we need to pop the stack.  We deliberately sort phi uses next to
      // the defs they must go with so that we can know it's time to pop the stack
      // when we hit the end of the phi uses for a given def.
      if(Stack.back().EdgeOnly)
      {
         if(!VDUse.U)
         {
            return false;
         }
         auto* PHI = llvm::dyn_cast<llvm::PHINode>(VDUse.U->getUser());
         if(!PHI)
         {
            return false;
         }
         // Check edge
         llvm::BasicBlock* EdgePred = PHI->getIncomingBlock(*VDUse.U);
         if(EdgePred != getBranchBlock(Stack.back().PInfo))
         {
            return false;
         }

         // Use dominates, which knows how to handle edge dominance.
         const auto bbedge = getBlockEdge(Stack.back().PInfo);
         const auto BBedge = llvm::BasicBlockEdge(bbedge.first, bbedge.second);
         return DT->dominates(BBedge, *VDUse.U);
      }

      return (VDUse.DFSIn >= Stack.back().DFSIn && VDUse.DFSOut <= Stack.back().DFSOut);
   }

   void popStackUntilDFSScope(ValueDFSStack& Stack, const ValueDFS& VD, const llvm::DominatorTree* DT)
   {
      while(!Stack.empty() && !stackIsInScope(Stack, VD, DT))
      {
         Stack.pop_back();
      }
   }

   // Convert the uses of Op into a vector of uses, associating global and local
   // DFS info with each one.
   void convertUsesToDFSOrdered(llvm::Value* Op, llvm::SmallVectorImpl<ValueDFS>& DFSOrderedSet, const llvm::DominatorTree* DT)
   {
      for(auto& U : Op->uses())
      {
         if(auto* I = llvm::dyn_cast<llvm::Instruction>(U.getUser()))
         {
            ValueDFS VD;
            // Put the phi node uses in the incoming block.
            llvm::BasicBlock* IBlock;
            if(auto* PN = llvm::dyn_cast<llvm::PHINode>(I))
            {
               IBlock = PN->getIncomingBlock(U);
               // Make phi node users appear last in the incoming block
               // they are from.
               VD.LocalNum = LN_Last;
            }
            else
            {
               // If it's not a phi node use, it is somewhere in the middle of the
               // block.
               IBlock = I->getParent();
               VD.LocalNum = LN_Middle;
            }
            llvm::DomTreeNode* DomNode = DT->getNode(IBlock);
            // It's possible our use is in an unreachable block. Skip it if so.
            if(!DomNode)
            {
               continue;
            }
            VD.DFSIn = DomNode->getDFSNumIn();
            VD.DFSOut = DomNode->getDFSNumOut();
            VD.U = &U;
            DFSOrderedSet.push_back(VD);
         }
      }
   }

   // Used to store information about each value we might rename.
   struct ValueInfo
   {
      // Information about each possible copy. During processing, this is each
      // inserted info. After processing, we move the uninserted ones to the
      // uninserted vector.
      llvm::SmallVector<PredicateBase*, 4> Infos;
      llvm::SmallVector<PredicateBase*, 4> UninsertedInfos;
   };

   const ValueInfo& getValueInfo(llvm::Value* Operand, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos)
   {
      auto OINI = ValueInfoNums.lookup(Operand);
      assert(OINI != 0 && "Operand was not really in the Value Info Numbers");
      assert(OINI < ValueInfos.size() && "Value Info Number greater than size of Value Info Table");
      return ValueInfos[OINI];
   }

   // Given the renaming stack, make all the operands currently on the stack real
   // by inserting them into the IR.  Return the last operation's value.
   llvm::Value* materializeStack(unsigned int& Counter, ValueDFSStack& RenameStack, llvm::Value* OrigOp, llvm::Function& /*F*/, llvm::DenseMap<const llvm::Value*, const PredicateBase*>& PredicateMap)
   {
      // Find the first thing we have to materialize
      auto RevIter = RenameStack.rbegin();
      for(; RevIter != RenameStack.rend(); ++RevIter)
      {
         if(RevIter->Def)
         {
            break;
         }
      }

      size_t Start = RevIter - RenameStack.rbegin();
      // The maximum number of things we should be trying to materialize at once
      // right now is 4, depending on if we had an assume, a branch, and both used
      // and of conditions.
      for(auto RenameIter = RenameStack.end() - Start; RenameIter != RenameStack.end(); ++RenameIter)
      {
         auto* Op = RenameIter == RenameStack.begin() ? OrigOp : (RenameIter - 1)->Def;
         ValueDFS& Result = *RenameIter;
         auto* ValInfo = Result.PInfo;
         // For edge predicates, we can just place the operand in the block before
         // the terminator.  For assume, we have to place it right before the assume
         // to ensure we dominate all of our uses.  Always insert right before the
         // relevant instruction (terminator, assume), so that we insert in proper
         // order in the case of multiple predicateinfo in the same block.
         if(llvm::isa<PredicateWithEdge>(ValInfo))
         {
            llvm::IRBuilder<> B(&llvm::cast<PredicateWithEdge>(ValInfo)->To->front());
            auto* PIC = B.CreatePHI(Op->getType(), 1, Op->getName() + "." + llvm::Twine(Counter++));
            PIC->addIncoming(Op, llvm::cast<PredicateWithEdge>(ValInfo)->From);
            PredicateMap.insert({PIC, ValInfo});
            Result.Def = PIC;
         }
         else
         {
            llvm_unreachable("assume intrinsic not yet supported");
            auto* PAssume = llvm::dyn_cast<PredicateAssume>(ValInfo);
            assert(PAssume && "Should not have gotten here without it being an assume");
            // llvm::IRBuilder<> B(PAssume->AssumeInst);
            // llvm::Function *IF = llvm::Intrinsic::getDeclaration(
            //                        F.getParent(), llvm::Intrinsic::ssa_copy, Op->getType());
            // llvm::CallInst *PIC = B.CreateCall(IF, Op);
            // PredicateMap.insert({PIC, ValInfo});
            // Result.Def = PIC;
         }
      }
      return RenameStack.back().Def;
   }

   // Instead of the standard SSA renaming algorithm, which is O(Number of
   // instructions), and walks the entire dominator tree, we walk only the defs +
   // uses.  The standard SSA renaming algorithm does not really rely on the
   // dominator tree except to order the stack push/pops of the renaming stacks, so
   // that defs end up getting pushed before hitting the correct uses.  This does
   // not require the dominator tree, only the *order* of the dominator tree. The
   // complete and correct ordering of the defs and uses, in dominator tree is
   // contained in the DFS numbering of the dominator tree. So we sort the defs and
   // uses into the DFS ordering, and then just use the renaming stack as per
   // normal, pushing when we hit a def (which is a predicateinfo instruction),
   // popping when we are out of the dfs scope for that def, and replacing any uses
   // with top of stack if it exists.  In order to handle liveness without
   // propagating liveness info, we don't actually insert the predicateinfo
   // instruction def until we see a use that it would dominate.  Once we see such
   // a use, we materialize the predicateinfo instruction in the right place and
   // use it.
   //
   void renameUses(llvm::SmallPtrSetImpl<llvm::Value*>& OpSet, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<ValueInfo, 32>& ValueInfos, llvm::Function& fun, const llvm::DominatorTree* DT, llvm::OrderedInstructions* OI,
                   llvm::DenseSet<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>& EdgeUsesOnly)
   {
      // This maps from copy operands to Predicate Info. Note that it does not own
      // the Predicate Info, they belong to the ValueInfo structs in the ValueInfos
      // vector.
      llvm::DenseMap<const llvm::Value*, const PredicateBase*> PredicateMap;
      // Sort OpsToRename since we are going to iterate it.
      llvm::SmallVector<llvm::Value*, 8> OpsToRename(OpSet.begin(), OpSet.end());
      auto Comparator = [&](const llvm::Value* A, const llvm::Value* B) { return valueComesBefore(*OI, A, B); };
      std::sort(OpsToRename.begin(), OpsToRename.end(), Comparator);
      ValueDFS_Compare Compare(*OI);
      // Compute liveness, and rename in O(uses) per Op.
      for(auto* Op : OpsToRename)
      {
         unsigned Counter = 0;
         llvm::SmallVector<ValueDFS, 16> OrderedUses;
         const auto& ValueInfo = getValueInfo(Op, ValueInfoNums, ValueInfos);
         // Insert the possible copies into the def/use list.
         // They will become real copies if we find a real use for them, and never
         // created otherwise.
         for(auto& PossibleCopy : ValueInfo.Infos)
         {
            ValueDFS VD;
            // Determine where we are going to place the copy by the copy type.
            // The predicate info for branches always come first, they will get
            // materialized in the split block at the top of the block.
            // The predicate info for assumes will be somewhere in the middle,
            // it will get materialized in front of the assume.
            if(const auto* PAssume = llvm::dyn_cast<PredicateAssume>(PossibleCopy))
            {
               VD.LocalNum = LN_Middle;
               llvm::DomTreeNode* DomNode = DT->getNode(PAssume->AssumeInst->getParent());
               if(!DomNode)
               {
                  continue;
               }
               VD.DFSIn = DomNode->getDFSNumIn();
               VD.DFSOut = DomNode->getDFSNumOut();
               VD.PInfo = PossibleCopy;
               OrderedUses.push_back(VD);
            }
            else if(llvm::isa<PredicateWithEdge>(PossibleCopy))
            {
               // If we can only do phi uses, we treat it like it's in the branch
               // block, and handle it specially. We know that it goes last, and only
               // dominate phi uses.
               auto BlockEdge = getBlockEdge(PossibleCopy);
               if(EdgeUsesOnly.count(BlockEdge))
               {
                  VD.LocalNum = LN_Last;
                  auto* DomNode = DT->getNode(BlockEdge.first);
                  if(DomNode)
                  {
                     VD.DFSIn = DomNode->getDFSNumIn();
                     VD.DFSOut = DomNode->getDFSNumOut();
                     VD.PInfo = PossibleCopy;
                     VD.EdgeOnly = true;
                     OrderedUses.push_back(VD);
                  }
               }
               else
               {
                  // Otherwise, we are in the split block (even though we perform
                  // insertion in the branch block).
                  // Insert a possible copy at the split block and before the branch.
                  VD.LocalNum = LN_First;
                  auto* DomNode = DT->getNode(BlockEdge.second);
                  if(DomNode)
                  {
                     VD.DFSIn = DomNode->getDFSNumIn();
                     VD.DFSOut = DomNode->getDFSNumOut();
                     VD.PInfo = PossibleCopy;
                     OrderedUses.push_back(VD);
                  }
               }
            }
         }

         convertUsesToDFSOrdered(Op, OrderedUses, DT);
         // Here we require a stable sort because we do not bother to try to
         // assign an order to the operands the uses represent. Thus, two
         // uses in the same instruction do not have a strict sort order
         // currently and will be considered equal. We could get rid of the
         // stable sort by creating one if we wanted.
         std::stable_sort(OrderedUses.begin(), OrderedUses.end(), Compare);
         llvm::SmallVector<ValueDFS, 8> RenameStack;
         // For each use, sorted into dfs order, push values and replaces uses with
         // top of stack, which will represent the reaching def.
         for(auto& VD : OrderedUses)
         {
            // We currently do not materialize copy over copy, but we should decide if
            // we want to.
            bool PossibleCopy = VD.PInfo != nullptr;
#if DEBUG_ESSA
            if(RenameStack.empty())
            {
               llvm::errs() << "Rename Stack is empty\n";
            }
            else
            {
               llvm::errs() << "Rename Stack Top DFS numbers are (" << RenameStack.back().DFSIn << "," << RenameStack.back().DFSOut << ")\n";
            }

            llvm::errs() << "Current DFS numbers are (" << VD.DFSIn << "," << VD.DFSOut << ")\n";
#endif
            bool ShouldPush = (VD.Def || PossibleCopy);
            bool OutOfScope = !stackIsInScope(RenameStack, VD, DT);
            if(OutOfScope || ShouldPush)
            {
               // Sync to our current scope.
               popStackUntilDFSScope(RenameStack, VD, DT);
               if(ShouldPush)
               {
                  RenameStack.push_back(VD);
               }
            }
            // If we get to this point, and the stack is empty we must have a use
            // with no renaming needed, just skip it.
            if(RenameStack.empty())
            {
               continue;
            }
            // Skip values, only want to rename the uses
            if(VD.Def || PossibleCopy)
            {
               continue;
            }

            ValueDFS& Result = RenameStack.back();

            // If the possible copy dominates something, materialize our stack up to
            // this point. This ensures every comparison that affects our operation
            // ends up with predicateinfo.
            if(!Result.Def)
            {
               Result.Def = materializeStack(Counter, RenameStack, Op, fun, PredicateMap);
            }

#if DEBUG_ESSA
            llvm::errs() << "Found replacement " << *Result.Def << " for " << *VD.U->get() << " in " << *(VD.U->getUser()) << "\n";
#endif
            assert(DT->dominates(llvm::cast<llvm::Instruction>(Result.Def), *VD.U) && "Predicateinfo def should have dominated this use");
            VD.U->set(Result.Def);
         }
      }
   }

   ValueInfo& getOrCreateValueInfo(llvm::Value* Operand, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos)
   {
      auto OIN = ValueInfoNums.find(Operand);
      if(OIN == ValueInfoNums.end())
      {
         // This will grow it
         ValueInfos.resize(ValueInfos.size() + 1);
         // This will use the new size and give us a 0 based number of the info
         auto InsertResult = ValueInfoNums.insert({Operand, ValueInfos.size() - 1});
         assert(InsertResult.second && "Value info number already existed?");
         return ValueInfos[InsertResult.first->second];
      }
      return ValueInfos[OIN->second];
   }

   void addInfoFor(llvm::SmallPtrSetImpl<llvm::Value*>& OpsToRename, llvm::Value* Op, PredicateBase* PB, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos)
   {
      OpsToRename.insert(Op);
      auto& OperandInfo = getOrCreateValueInfo(Op, ValueInfoNums, ValueInfos);
      //   AllInfos.push_back(PB);
      OperandInfo.Infos.push_back(PB);
   }

   // Collect relevant operations from Comparison that we may want to insert copies
   // for.
   void collectCmpOps(llvm::CmpInst* Comparison, llvm::SmallVectorImpl<llvm::Value*>& CmpOperands)
   {
      auto* Op0 = Comparison->getOperand(0);
      auto* Op1 = Comparison->getOperand(1);
      if(Op0 == Op1)
      {
         return;
      }
      CmpOperands.push_back(Comparison);
      // Only want real values, not constants.  Additionally, operands with one use
      // are only being used in the comparison, which means they will not be useful
      // for us to consider for predicateinfo.
      //
      if((llvm::isa<llvm::Instruction>(Op0) || llvm::isa<llvm::Argument>(Op0)) && !Op0->hasOneUse())
      {
         CmpOperands.push_back(Op0);
      }
      if((llvm::isa<llvm::Instruction>(Op1) || llvm::isa<llvm::Argument>(Op1)) && !Op1->hasOneUse())
      {
         CmpOperands.push_back(Op1);
      }
   }

   // Process an assume instruction and place relevant operations we want to rename
   // into OpsToRename.
   void processAssume(llvm::IntrinsicInst* II, llvm::BasicBlock* /*AssumeBB*/, llvm::SmallPtrSetImpl<llvm::Value*>& OpsToRename, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos)
   {
      // See if we have a comparison we support
      llvm::SmallVector<llvm::Value*, 8> CmpOperands;
      llvm::SmallVector<llvm::Value*, 2> ConditionsToProcess;
      llvm::CmpInst::Predicate Pred;
      llvm::Value* Operand = II->getOperand(0);
      if(m_c_And(m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value()), m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value())).match(II->getOperand(0)))
      {
         ConditionsToProcess.push_back(llvm::cast<llvm::BinaryOperator>(Operand)->getOperand(0));
         ConditionsToProcess.push_back(llvm::cast<llvm::BinaryOperator>(Operand)->getOperand(1));
         ConditionsToProcess.push_back(Operand);
      }
      else if(llvm::isa<llvm::CmpInst>(Operand))
      {
         ConditionsToProcess.push_back(Operand);
      }
      for(auto Cond : ConditionsToProcess)
      {
         if(auto* Cmp = llvm::dyn_cast<llvm::CmpInst>(Cond))
         {
            collectCmpOps(Cmp, CmpOperands);
            // Now add our copy infos for our operands
            for(auto* Op : CmpOperands)
            {
               auto* PA = new PredicateAssume(Op, II, Cmp);
               addInfoFor(OpsToRename, Op, PA, ValueInfoNums, ValueInfos);
            }
            CmpOperands.clear();
         }
         else if(auto* BinOp = llvm::dyn_cast<llvm::BinaryOperator>(Cond))
         {
            // Otherwise, it should be an AND.
            assert(BinOp->getOpcode() == llvm::Instruction::And && "Should have been an AND");
            auto* PA = new PredicateAssume(BinOp, II, BinOp);
            addInfoFor(OpsToRename, BinOp, PA, ValueInfoNums, ValueInfos);
         }
         else
         {
            llvm_unreachable("Unknown type of condition");
         }
      }
   }

   // Process a block terminating branch, and place relevant operations to be
   // renamed into OpsToRename.
   void processBranch(llvm::BranchInst* BI, llvm::BasicBlock* BranchBB, llvm::SmallPtrSetImpl<llvm::Value*>& OpsToRename, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos,
                      llvm::DenseSet<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>& EdgeUsesOnly)
   {
      llvm::BasicBlock* FirstBB = BI->getSuccessor(0);
      llvm::BasicBlock* SecondBB = BI->getSuccessor(1);
      llvm::SmallVector<llvm::BasicBlock*, 2> SuccsToProcess;
      SuccsToProcess.push_back(FirstBB);
      SuccsToProcess.push_back(SecondBB);
      llvm::SmallVector<llvm::Value*, 2> ConditionsToProcess;

      auto InsertHelper = [&](llvm::Value* Op, bool isAnd, bool isOr, llvm::Value* Cond) {
         for(auto* Succ : SuccsToProcess)
         {
            // Don't try to insert on a self-edge. This is mainly because we will
            // eliminate during renaming anyway.
            if(Succ == BranchBB)
            {
               continue;
            }
            bool TakenEdge = (Succ == FirstBB);
            // For and, only insert on the true edge
            // For or, only insert on the false edge
            if((isAnd && !TakenEdge) || (isOr && TakenEdge))
            {
               continue;
            }
            PredicateBase* PB = new PredicateBranch(Op, BranchBB, Succ, Cond, TakenEdge);
            addInfoFor(OpsToRename, Op, PB, ValueInfoNums, ValueInfos);
            if(!Succ->getSinglePredecessor())
            {
               EdgeUsesOnly.insert({BranchBB, Succ});
            }
         }
      };

      // Match combinations of conditions.
      bool isAnd = false;
      bool isOr = false;
      llvm::SmallVector<llvm::Value*, 8> CmpOperands;
      /* llvm::CmpInst::Predicate Pred;
      if (match(BI->getCondition(), m_And(m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value()),
                                          m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value()))) ||
          match(BI->getCondition(), m_Or(m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value()),
                                         m_Cmp(Pred, llvm::PatternMatch::m_Value(), llvm::PatternMatch::m_Value())))) {
         auto *BinOp = llvm::cast<llvm::BinaryOperator>(BI->getCondition());
         if (BinOp->getOpcode() == llvm::Instruction::And)
            isAnd = true;
         else if (BinOp->getOpcode() == llvm::Instruction::Or)
            isOr = true;
         ConditionsToProcess.push_back(BinOp->getOperand(0));
         ConditionsToProcess.push_back(BinOp->getOperand(1));
         ConditionsToProcess.push_back(BI->getCondition());
      } else */
      if(llvm::isa<llvm::CmpInst>(BI->getCondition()))
      {
         ConditionsToProcess.push_back(BI->getCondition());
      }
      for(auto Cond : ConditionsToProcess)
      {
         if(auto* Cmp = llvm::dyn_cast<llvm::CmpInst>(Cond))
         {
            collectCmpOps(Cmp, CmpOperands);
            // Now add our copy infos for our operands
            for(auto* Op : CmpOperands)
            {
               InsertHelper(Op, isAnd, isOr, Cmp);
            }
         }
         else if(auto* BinOp = llvm::dyn_cast<llvm::BinaryOperator>(Cond))
         {
            // This must be an AND or an OR.
            assert((BinOp->getOpcode() == llvm::Instruction::And || BinOp->getOpcode() == llvm::Instruction::Or) && "Should have been an AND or an OR");
            // The actual value of the binop is not subject to the same restrictions
            // as the comparison. It's either true or false on the true/false branch.
            InsertHelper(BinOp, false, false, BinOp);
         }
         else
         {
            llvm_unreachable("Unknown type of condition");
         }
         CmpOperands.clear();
      }
   }
   // Process a block terminating switch, and place relevant operations to be
   // renamed into OpsToRename.
   void processSwitch(llvm::SwitchInst* SI, llvm::BasicBlock* BranchBB, llvm::SmallPtrSetImpl<llvm::Value*>& OpsToRename, llvm::DenseMap<llvm::Value*, unsigned int>& ValueInfoNums, llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32>& ValueInfos,
                      llvm::DenseSet<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>& EdgeUsesOnly)
   {
      llvm::Value* Op = SI->getCondition();
      if((!llvm::isa<llvm::Instruction>(Op) && !llvm::isa<llvm::Argument>(Op)) || Op->hasOneUse())
      {
         return;
      }

      // Remember how many outgoing edges there are to every successor.
      llvm::SmallDenseMap<llvm::BasicBlock*, unsigned, 16> SwitchEdges;
      for(unsigned i = 0, e = SI->getNumSuccessors(); i != e; ++i)
      {
         llvm::BasicBlock* TargetBlock = SI->getSuccessor(i);
         ++SwitchEdges[TargetBlock];
      }

      // Now propagate info for each case value
      for(auto C : SI->cases())
      {
         llvm::BasicBlock* TargetBlock = C.getCaseSuccessor();
         if(SwitchEdges.lookup(TargetBlock) == 1)
         {
            auto* PS = new PredicateSwitch(Op, SI->getParent(), TargetBlock, C.getCaseValue(), SI);
            addInfoFor(OpsToRename, Op, PS, ValueInfoNums, ValueInfos);
            if(!TargetBlock->getSinglePredecessor())
            {
               EdgeUsesOnly.insert({BranchBB, TargetBlock});
            }
         }
      }
   }

} // namespace eSSAInfoClasses

bool eSSA::runOnFunction(llvm::Function& fun, llvm::ModulePass* modulePass)
{
   auto DT = &modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(fun).getDomTree();
   // auto AC = &modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(fun);
   auto OI = new llvm::OrderedInstructions(DT);
   DT->updateDFSNumbers();
   // This stores info about each operand or comparison result we make copies
   // of.  The real ValueInfos start at index 1, index 0 is unused so that we can
   // more easily detect invalid indexing.
   llvm::SmallVector<eSSAInfoClasses::ValueInfo, 32> ValueInfos;
   // This gives the index into the ValueInfos array for a given Value.  Because
   // 0 is not a valid Value Info index, you can use DenseMap::lookup and tell
   // whether it returned a valid result.
   llvm::DenseMap<llvm::Value*, unsigned int> ValueInfoNums;
   // The set of edges along which we can only handle phi uses, due to critical edges.
   llvm::DenseSet<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> EdgeUsesOnly;

   ValueInfos.resize(1);

   // Collect operands to rename from all conditional branch terminators, as well
   // as assume statements.
   llvm::SmallPtrSet<llvm::Value*, 8> OpsToRename;
   for(auto DTN : depth_first(DT->getRootNode()))
   {
      llvm::BasicBlock* BranchBB = DTN->getBlock();
      if(auto* BI = llvm::dyn_cast<llvm::BranchInst>(BranchBB->getTerminator()))
      {
         if(!BI->isConditional())
         {
            continue;
         }
         // Can't insert conditional information if they all go to the same place.
         if(BI->getSuccessor(0) == BI->getSuccessor(1))
         {
            continue;
         }
         processBranch(BI, BranchBB, OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly);
      }
      else if(auto* SI = llvm::dyn_cast<llvm::SwitchInst>(BranchBB->getTerminator()))
      {
         processSwitch(SI, BranchBB, OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly);
      }
   }
#if 0
   for (auto &Assume : AC->assumptions()) {
      if (auto *II = llvm::dyn_cast_or_null<llvm::IntrinsicInst>(Assume))
         processAssume(II, II->getParent(), OpsToRename, ValueInfoNums, ValueInfos);
   }
#endif
   // Now rename all our operations.
   renameUses(OpsToRename, ValueInfoNums, ValueInfos, fun, DT, OI, EdgeUsesOnly);
   delete OI;
   return true;
}
