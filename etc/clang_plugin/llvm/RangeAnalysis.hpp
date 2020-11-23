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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
/**
  * The implementation started from code available at this link:https://code.google.com/archive/p/range-analysis/.
  * The original code went through a deep revision and it has been changed to be compatible to a recent version of LLVM.
  * Some extensions have been also added:
  * - Added anti range support
  * - Redesigned many Range operations to take into account wrapping and to
  *   improve the reductions performed
  * - Integrated the LLVM lazy value range analysis.
  * - Added support to range value propagation of load from constant arrays.
  * - Added support to range value propagation of load and store from generic arrays.
  *
  * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
  *
//===-------------------------- RangeAnalysis.h ---------------------------===//
//===-----        Performs the Range analysis of the variables        -----===//
//
//					 The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (C) 2011-2012, 2015, 2017  Victor Hugo Sperle Campos
//               2011               	  Douglas do Couto Teixeira
//               2012               	  Igor Rafael de Assis Costa
//
//===----------------------------------------------------------------------===//
// This file contains a pass that performs range analysis. The objective of
// the range analysis is to map each integer variable in the program to the
// range of possible values that it might assume through out the program
// execution. Ideally this range should be as constrained as possible, so that
// an optimizing compiler could learn more information about each variable.
// However, the range analysis must be conservative, that it, it will only
// constraint the range of a variable if it can prove that it is safe to do so.
// As an example, consider the program:
//
// i = read();
// if (i < 10) {
//   print (i + 1);
// else {
//   print(i - 1);
// }
//
// In this program we know, from the conditional test, that the value of i in
// the true side of the branch is in the range [-INF, 9], and in the false side
// is in the range [10, +INF].
//
//
//===----------------------------------------------------------------------===//
*/
#ifndef _RANGEANALYSIS_RANGEANALYSIS_H
#define _RANGEANALYSIS_RANGEANALYSIS_H

#include <deque>
#include <list>
#include <sstream>
#include <stack>
#include <system_error>
#include <utility>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"

#include "config_HAVE_LIBBDD.hpp"

namespace llvm
{
   class BranchInst;
   class SwitchInst;
   class PHINode;
   class LoadInst;
   class StoreInst;
   raw_ostream& dbgs();
   class ConstantRange;
   class MemoryUseOrDef;
   class MemorySSA;
} // namespace llvm

class Andersen_AA;

namespace RangeAnalysis
{
   // Comment the line below to enable the debug of SCCs and optimize the code
   // generated.
   //#define SCC_DEBUG

   // Comment the line below to disable the dot printing of Constraint Graphs
   //#define DEBUG_RA

   // Used to enable the stats computing. Comment the below line to disable it
   //#define STATS

   // Uncomment the line below to activate jump-set widening technique
   // It generally leads to better precision in spite of low overhead in
   // running time, so I recommend leaving it activated
#define JUMPSET

#ifdef SCC_DEBUG
#define ASSERT(cond, msg)                 \
   if(!(cond))                            \
   {                                      \
      errs() << "ERROR: " << msg << "\n"; \
   }
#else
#define ASSERT(cond, msg)
#endif

   //************************** Log Transactions ********************************//
   //#define LOG_TRANSACTIONS

   //****************************************************************************//

   /// In our range analysis pass we have to perform operations on ranges all the
   /// time. LLVM has a class to perform operations on ranges: the class
   /// ConstantRange. However, the class ConstantRange doesn't serve very well
   /// our purposes because we need to perform operations with big numbers
   /// (MIN_INT, MAX_INT) a lot of times, without allowing these numbers to wrap
   /// around. And the class ConstantRange allows that. So, I'm writing this class
   /// to
   /// perform operations on ranges, considering these big numbers and without
   /// allowing them to wrap around.
   /// The interface of this class is very similar to LLVM's ConstantRange class.
   ///
   /// Changes done by Fabrizio Ferrandi, Politecnico di Milano, Italy
   /// Added Anti range support to describe range in this form ~[l,m]. The idea has been taken from GCC VR_ANTI_RANGE objects.
   /// Moreover, many range computation are done by exploiting the standard LLVM ConstantRange class.

   /// Value extended for value range analysis
   using eValue = std::pair<const llvm::Value*, const llvm::Value*>;
   enum RangeType
   {
      Unknown,
      Regular,
      Empty,
      Anti
   };

   class Range_base
   {
      /// The lower bound of the range.
      llvm::APInt l;
      /// The upper bound of the range.
      llvm::APInt u;
      /// the range bit-width
      unsigned bw;
      /// the range type
      RangeType type;

      void normalizeRange(const llvm::APInt& lb, const llvm::APInt& ub, RangeType rType);

    public:
      Range_base(RangeType type, unsigned bw);
      Range_base(RangeType rType, unsigned bw, const llvm::APInt& lb, const llvm::APInt& ub);
      ~Range_base() = default;
      Range_base(const Range_base& other) = default;
      Range_base(Range_base&&) = default;
      static Range_base getAnti(const Range_base& o);
      Range_base& operator=(const Range_base& other) = default;
      Range_base& operator=(Range_base&&) = default;
      unsigned int getBitWidth() const;
      const llvm::APInt getLower() const;
      const llvm::APInt getUpper() const;
      const llvm::APInt getSignedMax() const;
      const llvm::APInt getSignedMin() const;
      const llvm::APInt getUnsignedMax() const;
      const llvm::APInt getUnsignedMin() const;

      bool isUnknown() const
      {
         return type == Unknown;
      }
      void setUnknown()
      {
         type = Unknown;
      }
      bool isRegular() const
      {
         return type == Regular;
      }
      bool isAnti() const
      {
         return type == Anti;
      }
      bool isEmpty() const
      {
         return type == Empty;
      }
      bool isSameType(const Range_base& a, const Range_base& b) const
      {
         return a.type == b.type;
      }
      bool isSameRange(const Range_base& a, const Range_base& b) const
      {
         return (a.l.eq(b.l) && a.u.eq(b.u));
      }
      bool isSingleElement()
      {
         return type == Regular && l == u;
      }

      static unsigned neededBits(const llvm::APInt& a, const llvm::APInt& b, bool sign);
   };
   class Range : public Range_base
   {
    public:
      Range(RangeType rType, unsigned bw) : Range_base(rType, bw)
      {
      }
      Range(RangeType rType, unsigned bw, const llvm::APInt& lb, const llvm::APInt& ub) : Range_base(rType, bw, lb, ub)
      {
      }
      Range(const Range_base& rb) : Range_base(rb)
      {
      }
      ~Range() = default;
      Range(const Range& other) = default;
      Range(Range&&) = default;
      Range& operator=(const Range& other) = default;
      Range& operator=(Range&&) = default;

      bool isMaxRange() const;
      bool isConstant() const;
      void print(llvm::raw_ostream& OS) const;
      Range add(const Range& other) const;
      Range sub(const Range& other) const;
      Range mul(const Range& other) const;
      Range udiv(const Range& other) const;
      Range sdiv(const Range& other) const;
      Range urem(const Range& other) const;
      Range srem(const Range& other) const;
      Range shl(const Range& other) const;
      Range lshr(const Range& other) const;
      Range ashr(const Range& other) const;
      Range And(const Range& other) const;
      Range Or(const Range& other) const;
      Range Xor(const Range& other) const;
      Range Eq(const Range& other, unsigned bw) const;
      Range Ne(const Range& other, unsigned bw) const;
      Range Ugt(const Range& other, unsigned bw) const;
      Range Uge(const Range& other, unsigned bw) const;
      Range Ult(const Range& other, unsigned bw) const;
      Range Ule(const Range& other, unsigned bw) const;
      Range Sgt(const Range& other, unsigned bw) const;
      Range Sge(const Range& other, unsigned bw) const;
      Range Slt(const Range& other, unsigned bw) const;
      Range Sle(const Range& other, unsigned bw) const;
      Range truncate(unsigned bitwidth) const;
      Range sextOrTrunc(unsigned bitwidth) const;
      Range zextOrTrunc(unsigned from_bitwidth) const;
      bool operator==(const Range& other) const;
      bool operator!=(const Range& other) const;
      Range intersectWith(const Range& other) const;
      Range unionWith(const Range& other) const;
      Range BestRange(const llvm::ConstantRange& UR, const llvm::ConstantRange& SR, unsigned bw) const;
   };

   /// This class represents a program variable.
   class VarNode
   {
    private:
      /// The program variable
      const llvm::Value* V;
      /// if not null refers to a global variable object or to an AllocaInst
      const llvm::Value* GV;
      /// A Range associated to the variable, that is,
      /// its interval inferred by the analysis.
      Range interval;
      /// Used by the crop meet operator
      char abstractState;

    public:
      explicit VarNode(const llvm::Value* _V, const llvm::Value* _GV, const llvm::DataLayout* DL);
      ~VarNode();
      VarNode(const VarNode&) = delete;
      VarNode(VarNode&&) = delete;
      VarNode& operator=(const VarNode&) = delete;
      VarNode& operator=(VarNode&&) = delete;

      /// Initializes the value of the node.
      void init(bool outside, const llvm::DataLayout* DL);
      /// Returns the range of the variable represented by this node.
      const Range getRange() const
      {
         return interval;
      }
      /// Returns the variable represented by this node.
      eValue getValue() const
      {
         return std::make_pair(V, GV);
      }
      unsigned int getBitWidth() const
      {
         return interval.getBitWidth();
      }
      /// Changes the status of the variable represented by this node.
      void setRange(const Range newInterval)
      {
         interval = newInterval;
      }

      /// Pretty print.
      void print(llvm::raw_ostream& OS) const;
      char getAbstractState()
      {
         return abstractState;
      }
      // The possible states are '0', '+', '-' and '?'.
      void storeAbstractState();
   };

   enum IntervalId
   {
      BasicIntervalId,
      SymbIntervalId
   };

   /// This class represents a basic interval of values. This class could inherit
   /// from LLVM's Range class, since it *is a Range*. However,
   /// LLVM's Range class doesn't have a virtual constructor.
   class BasicInterval
   {
    private:
      Range range;

    public:
      BasicInterval();
      explicit BasicInterval(Range range);
      virtual ~BasicInterval(); // This is a base class.
      BasicInterval(const BasicInterval&) = delete;
      BasicInterval(BasicInterval&&) = delete;
      BasicInterval& operator=(const BasicInterval&) = delete;
      BasicInterval& operator=(BasicInterval&&) = delete;

      // Methods for RTTI
      virtual IntervalId getValueId() const
      {
         return BasicIntervalId;
      }
      static bool classof(BasicInterval const* /*unused*/)
      {
         return true;
      }

      /// Returns the range of this interval.
      const Range& getRange() const
      {
         return this->range;
      }
      /// Sets the range of this interval to another range.
      void setRange(const Range& newRange)
      {
         this->range = newRange;
      }

      /// Pretty print.
      virtual void print(llvm::raw_ostream& OS) const;
   };

   /// This is an interval that contains a symbolic limit, which is
   /// given by the bounds of a program name, e.g.: [-inf, ub(b) + 1].
   class SymbInterval : public BasicInterval
   {
    private:
      /// The bound. It is a node which limits the interval of this range.
      const llvm::Value* bound;
      /// The predicate of the operation in which this interval takes part.
      /// It is useful to know how we can constrain this interval
      /// after we fix the intersects.
      llvm::CmpInst::Predicate pred;

    public:
      SymbInterval(const Range& range, const llvm::Value* bound, llvm::CmpInst::Predicate pred);
      ~SymbInterval() override;
      SymbInterval(const SymbInterval&) = delete;
      SymbInterval(SymbInterval&&) = delete;
      SymbInterval& operator=(const SymbInterval&) = delete;
      SymbInterval& operator=(SymbInterval&&) = delete;

      // Methods for RTTI
      IntervalId getValueId() const override
      {
         return SymbIntervalId;
      }
      static bool classof(SymbInterval const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicInterval const* BI)
      {
         return BI->getValueId() == SymbIntervalId;
      }

      /// Returns the opcode of the operation that create this interval.
      llvm::CmpInst::Predicate getOperation() const
      {
         return this->pred;
      }
      /// Returns the node which is the bound of this interval.
      const eValue getBound() const
      {
         return std::make_pair(this->bound, nullptr);
      }
      /// Replace symbolic intervals with hard-wired constants.
      Range fixIntersects(VarNode* bound, VarNode* sink);

      /// Prints the content of the interval.
      void print(llvm::raw_ostream& OS) const override;
   };

   /// This class represents a generic operation in our analysis.
   class BasicOp
   {
    private:
      /// The range of the operation. Each operation has a range associated to it.
      /// This range is obtained by inspecting the branches in the source program
      /// and extracting its condition and intervals.
      std::shared_ptr<BasicInterval> intersect;
      // The target of the operation, that is, the node which
      // will store the result of the operation.
      VarNode* sink;
      // The instruction that originated this op node
      const llvm::Instruction* inst;

    protected:
      /// We do not want people creating objects of this class,
      /// but we want to inherit from it.
      BasicOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst);

    public:
      enum class OperationId
      {
         UnaryOpId,
         SigmaOpId,
         BinaryOpId,
         TernaryOpId,
         PhiOpId,
         ControlDepId,
         LoadOpId,
         StoreOpId
      };

      /// The dtor. It's virtual because this is a base class.
      virtual ~BasicOp();
      // We do not want people creating objects of this class.
      BasicOp(const BasicOp&) = delete;
      BasicOp(BasicOp&&) = delete;
      BasicOp& operator=(const BasicOp&) = delete;
      BasicOp& operator=(BasicOp&&) = delete;

      // Methods for RTTI
      virtual OperationId getValueId() const = 0;
      static bool classof(BasicOp const* /*unused*/)
      {
         return true;
      }

      /// Given the input of the operation and the operation that will be
      /// performed, evaluates the result of the operation.
      virtual Range eval() = 0;
      /// Return the instruction that originated this op node
      const llvm::Instruction* getInstruction() const
      {
         return inst;
      }
      /// Replace symbolic intervals with hard-wired constants.
      void fixIntersects(VarNode* V);
      /// Returns the range of the operation.
      std::shared_ptr<BasicInterval> getIntersect() const
      {
         return intersect;
      }
      /// Changes the interval of the operation.
      void setIntersect(const Range& newIntersect)
      {
         this->intersect->setRange(newIntersect);
      }
      /// Returns the target of the operation, that is,
      /// where the result will be stored.
      const VarNode* getSink() const
      {
         return sink;
      }
      /// Returns the target of the operation, that is,
      /// where the result will be stored.
      VarNode* getSink()
      {
         return sink;
      }

      /// Prints the content of the operation.
      virtual void print(llvm::raw_ostream& OS) const = 0;
   };

   /// A constraint like sink = operation(source) \intersec [l, u]
   /// Examples: unary instructions such as truncation, sign extensions,
   /// zero extensions.
   class UnaryOp : public BasicOp
   {
    private:
      // The source node of the operation.
      VarNode* source;
      // The opcode of the operation.
      unsigned int opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      UnaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst, VarNode* source, unsigned int opcode);
      ~UnaryOp() override;
      UnaryOp(const UnaryOp&) = delete;
      UnaryOp(UnaryOp&&) = delete;
      UnaryOp& operator=(const UnaryOp&) = delete;
      UnaryOp& operator=(UnaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::UnaryOpId;
      }
      static bool classof(UnaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::UnaryOpId || BO->getValueId() == OperationId::SigmaOpId;
      }

      /// Return the opcode of the operation.
      unsigned int getOpcode() const
      {
         return opcode;
      }
      /// Returns the source of the operation.
      VarNode* getSource() const
      {
         return source;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(llvm::raw_ostream& OS) const override;
   };

   /// Specific type of UnaryOp used to represent sigma functions
   class SigmaOp : public UnaryOp
   {
    private:
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

      // The symbolic source node of the operation.
      VarNode* SymbolicSource;

      bool unresolved;

    public:
      SigmaOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst, VarNode* source, VarNode* SymbolicSource, unsigned int opcode);
      ~SigmaOp() override = default;
      SigmaOp(const SigmaOp&) = delete;
      SigmaOp(SigmaOp&&) = delete;
      SigmaOp& operator=(const SigmaOp&) = delete;
      SigmaOp& operator=(SigmaOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::SigmaOpId;
      }
      static bool classof(SigmaOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(UnaryOp const* UO)
      {
         return UO->getValueId() == OperationId::SigmaOpId;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::SigmaOpId;
      }

      bool isUnresolved() const
      {
         return unresolved;
      }
      void markResolved()
      {
         unresolved = false;
      }
      void markUnresolved()
      {
         unresolved = true;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(llvm::raw_ostream& OS) const override;
   };

   /// Specific type of BasicOp used in Nuutila's strongly connected
   /// components algorithm.
   class ControlDep : public BasicOp
   {
    private:
      VarNode* source;
      Range eval() override;

    public:
      ControlDep(VarNode* sink, VarNode* source);
      ~ControlDep() override;
      ControlDep(const ControlDep&) = delete;
      ControlDep(ControlDep&&) = delete;
      ControlDep& operator=(const ControlDep&) = delete;
      ControlDep& operator=(ControlDep&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::ControlDepId;
      }
      static bool classof(ControlDep const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::ControlDepId;
      }

      /// Returns the source of the operation.
      VarNode* getSource() const
      {
         return source;
      }

      void print(llvm::raw_ostream& OS) const override;
   };

   class LoadOp : public BasicOp
   {
    private:
      /// reference to the memory access operand
      llvm::SmallVector<const VarNode*, 2> sources;
      Range eval() override;

    public:
      LoadOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst);
      ~LoadOp() override;
      LoadOp(const LoadOp&) = delete;
      LoadOp(LoadOp&&) = delete;
      LoadOp& operator=(const LoadOp&) = delete;
      LoadOp& operator=(LoadOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::LoadOpId;
      }
      static bool classof(LoadOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::LoadOpId;
      }

      /// Add source to the vector of sources
      void addSource(const VarNode* newsrc)
      {
         sources.push_back(newsrc);
      }
      /// Return source identified by index
      const VarNode* getSource(unsigned index) const
      {
         return sources[index];
      }
      /// return the number of sources
      size_t getNumSources() const
      {
         return sources.size();
      }

      void print(llvm::raw_ostream& OS) const override;
   };

   class StoreOp : public BasicOp
   {
    private:
      /// reference to the memory access operand
      llvm::SmallVector<const VarNode*, 2> sources;
      /// union of the values at which the variable is initialized
      Range init;
      Range eval() override;

    public:
      StoreOp(VarNode* sink, const llvm::Instruction* inst, Range _init);
      ~StoreOp() override;
      StoreOp(const StoreOp&) = delete;
      StoreOp(StoreOp&&) = delete;
      StoreOp& operator=(const StoreOp&) = delete;
      StoreOp& operator=(StoreOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::StoreOpId;
      }
      static bool classof(StoreOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::StoreOpId;
      }

      /// Add source to the vector of sources
      void addSource(const VarNode* newsrc)
      {
         sources.push_back(newsrc);
      }
      /// Return source identified by index
      const VarNode* getSource(unsigned index) const
      {
         return sources[index];
      }
      /// return the number of sources
      size_t getNumSources() const
      {
         return sources.size();
      }

      void print(llvm::raw_ostream& OS) const override;
   };

   /// A constraint like sink = phi(src1, src2, ..., srcN)
   class PhiOp : public BasicOp
   {
    private:
      // Vector of sources
      llvm::SmallVector<const VarNode*, 2> sources;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      PhiOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst);
      ~PhiOp() override = default;
      PhiOp(const PhiOp&) = delete;
      PhiOp(PhiOp&&) = delete;
      PhiOp& operator=(const PhiOp&) = delete;
      PhiOp& operator=(PhiOp&&) = delete;

      /// Add source to the vector of sources
      void addSource(const VarNode* newsrc)
      {
         sources.push_back(newsrc);
      }
      /// Return source identified by index
      const VarNode* getSource(unsigned index) const
      {
         return sources[index];
      }
      /// return the number of sources
      size_t getNumSources() const
      {
         return sources.size();
      }

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::PhiOpId;
      }
      static bool classof(PhiOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::PhiOpId;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(llvm::raw_ostream& OS) const override;
   };

   /// A constraint like sink = source1 operation source2 intersect [l, u].
   class BinaryOp : public BasicOp
   {
    private:
      // The first operand.
      VarNode* source1;
      // The second operand.
      VarNode* source2;
      // The opcode of the operation.
      unsigned int opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      BinaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst, VarNode* source1, VarNode* source2, unsigned int opcode);
      ~BinaryOp() override = default;
      BinaryOp(const BinaryOp&) = delete;
      BinaryOp(BinaryOp&&) = delete;
      BinaryOp& operator=(const BinaryOp&) = delete;
      BinaryOp& operator=(BinaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::BinaryOpId;
      }
      static bool classof(BinaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::BinaryOpId;
      }

      /// Return the opcode of the operation.
      unsigned int getOpcode() const
      {
         return opcode;
      }
      /// Returns the first operand of this operation.
      VarNode* getSource1() const
      {
         return source1;
      }
      /// Returns the second operand of this operation.
      VarNode* getSource2() const
      {
         return source2;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(llvm::raw_ostream& OS) const override;
   };

   class TernaryOp : public BasicOp
   {
    private:
      // The first operand.
      VarNode* source1;
      // The second operand.
      VarNode* source2;
      // The third operand.
      VarNode* source3;
      // The opcode of the operation.
      unsigned int opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      TernaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const llvm::Instruction* inst, VarNode* source1, VarNode* source2, VarNode* source3, unsigned int opcode);
      ~TernaryOp() override = default;
      TernaryOp(const TernaryOp&) = delete;
      TernaryOp(TernaryOp&&) = delete;
      TernaryOp& operator=(const TernaryOp&) = delete;
      TernaryOp& operator=(TernaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::TernaryOpId;
      }
      static bool classof(TernaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::TernaryOpId;
      }

      /// Return the opcode of the operation.
      unsigned int getOpcode() const
      {
         return opcode;
      }
      /// Returns the first operand of this operation.
      VarNode* getSource1() const
      {
         return source1;
      }
      /// Returns the second operand of this operation.
      VarNode* getSource2() const
      {
         return source2;
      }
      /// Returns the third operand of this operation.
      VarNode* getSource3() const
      {
         return source3;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(llvm::raw_ostream& OS) const override;
   };

   /// This class is used to store the intersections that we get in the branches.
   /// I decided to write it because I think it is better to have an object
   /// to store these information than create a lot of maps
   /// in the ConstraintGraph class.
   class ValueBranchMap
   {
    private:
      const llvm::Value* V;
      const llvm::BasicBlock* BBTrue;
      const llvm::BasicBlock* BBFalse;
      std::shared_ptr<BasicInterval> ItvT;
      std::shared_ptr<BasicInterval> ItvF;

    public:
      ValueBranchMap(const llvm::Value* V, const llvm::BasicBlock* BBTrue, const llvm::BasicBlock* BBFalse, std::shared_ptr<BasicInterval> ItvT, std::shared_ptr<BasicInterval> ItvF) : V(V), BBTrue(BBTrue), BBFalse(BBFalse), ItvT(ItvT), ItvF(ItvF)
      {
      }
      ~ValueBranchMap() = default;
      ValueBranchMap(const ValueBranchMap&) = default;
      ValueBranchMap(ValueBranchMap&&) = default;
      ValueBranchMap& operator=(const ValueBranchMap&) = delete;
      ValueBranchMap& operator=(ValueBranchMap&&) = delete;

      /// Get the "false side" of the branch
      const llvm::BasicBlock* getBBFalse() const
      {
         return BBFalse;
      }
      /// Get the "true side" of the branch
      const llvm::BasicBlock* getBBTrue() const
      {
         return BBTrue;
      }
      /// Get the interval associated to the true side of the branch
      std::shared_ptr<BasicInterval> getItvT() const
      {
         return ItvT;
      }
      /// Get the interval associated to the false side of the branch
      std::shared_ptr<BasicInterval> getItvF() const
      {
         return ItvF;
      }
      /// Get the value associated to the branch.
      const llvm::Value* getV() const
      {
         return V;
      }
      /// Change the interval associated to the true side of the branch
      void setItvT(std::shared_ptr<BasicInterval> Itv)
      {
         this->ItvT = Itv;
      }
      /// Change the interval associated to the false side of the branch
      void setItvF(std::shared_ptr<BasicInterval> Itv)
      {
         this->ItvF = Itv;
      }
   };

   /// This is pretty much the same thing as ValueBranchMap
   /// but implemented specifically for switch instructions
   class ValueSwitchMap
   {
    private:
      const llvm::Value* V;
      llvm::SmallVector<std::pair<std::shared_ptr<BasicInterval>, const llvm::BasicBlock*>, 4> BBsuccs;

    public:
      ValueSwitchMap(const llvm::Value* V, llvm::SmallVector<std::pair<std::shared_ptr<BasicInterval>, const llvm::BasicBlock*>, 4>& BBsuccs) : V(V), BBsuccs(BBsuccs)
      {
      }
      ~ValueSwitchMap() = default;
      ValueSwitchMap(const ValueSwitchMap&) = default;
      ValueSwitchMap(ValueSwitchMap&&) = default;
      ValueSwitchMap& operator=(const ValueSwitchMap&) = delete;
      ValueSwitchMap& operator=(ValueSwitchMap&&) = delete;

      /// Get the corresponding basic block
      const llvm::BasicBlock* getBB(unsigned idx) const
      {
         return BBsuccs[idx].second;
      }
      /// Get the interval associated to the switch case idx
      std::shared_ptr<BasicInterval> getItv(unsigned idx) const
      {
         return BBsuccs[idx].first;
      }
      // Get how many cases this switch has
      unsigned getNumOfCases() const
      {
         return BBsuccs.size();
      }
      /// Get the value associated to the switch.
      const llvm::Value* getV() const
      {
         return V;
      }
      /// Change the interval associated to the true side of the branch
      void setItv(unsigned idx, std::shared_ptr<BasicInterval> Itv)
      {
         this->BBsuccs[idx].first = Itv;
      }
   };

   /// This class can be used to gather statistics on running time
   /// and memory footprint. It had been developed before LLVM started
   /// to provide a class for this exact purpose. We'll keep using this
   /// because it works just fine and is well put together.
   class Profile
   {
      using AccTimesMap = llvm::StringMap<llvm::TimeRecord>;
      llvm::TimerGroup* tg{};
      llvm::SmallVector<llvm::Timer*, 4> timers;

      AccTimesMap accumulatedtimes;
      ssize_t memory{0L};

    public:
      Profile()
      {
         tg = new llvm::TimerGroup("RangeAnalysis", "Range Analysis algorithm");
      }

      ~Profile()
      {
         delete tg;
         for(llvm::Timer* timer : timers)
         {
            delete timer;
         }
      }

      Profile(const Profile&) = delete;
      Profile(Profile&&) = delete;
      Profile& operator=(const Profile&) = delete;
      Profile& operator=(Profile&&) = delete;

      ssize_t getMemoryUsage() const
      {
         return memory;
      }

      llvm::Timer* registerNewTimer(llvm::StringRef key, llvm::StringRef descr)
      {
         llvm::Timer* timer = new llvm::Timer(key, descr, *tg);
         timers.push_back(timer);

         return timer;
      }

      void addTimeRecord(const llvm::Timer* timer)
      {
         llvm::StringRef key = timer->getName();
         llvm::TimeRecord time = timer->getTotalTime();
         accumulatedtimes[key] += time;
      }

      double getTimeDouble(llvm::StringRef key)
      {
         return accumulatedtimes[key].getUserTime();
      }

      llvm::TimeRecord getTimeRecord(llvm::StringRef key)
      {
         return accumulatedtimes[key];
      }

      void printTime(llvm::StringRef key)
      {
         double time = getTimeDouble(key);
         std::ostringstream formatted;
         formatted << time;
         llvm::errs() << formatted.str() << "\t - " << key << " elapsed time\n";
      }

      void registerMemoryUsage()
      {
         llvm::TimeRecord current = llvm::TimeRecord::getCurrentTime();
         ssize_t newmemory = current.getMemUsed();
         if(newmemory > memory)
         {
            memory = newmemory;
         }
      }

      void printMemoryUsage() const
      {
         std::ostringstream formatted;
         // Convert bytes to kilobytes
         double mem = memory;
         formatted << (mem / 1024);
         llvm::errs() << formatted.str() << "\t - "
                      << "Memory used in KB\n";
      }
   };

   // The VarNodes type.
   using VarNodes = llvm::DenseMap<eValue, VarNode*>;

   // The Operations type.
   using GenOprs = llvm::SmallPtrSet<BasicOp*, 32>;

   // A map from variables to the operations where these variables are used.
   using UseMap = llvm::DenseMap<eValue, llvm::SmallPtrSet<BasicOp*, 8>>;

   // A map from variables to the operations where these
   // variables are present as bounds
   using SymbMap = llvm::DenseMap<eValue, llvm::SmallPtrSet<BasicOp*, 8>>;

   // A map from varnodes to the operation in which this variable is defined
   using DefMap = llvm::DenseMap<eValue, BasicOp*>;

   using ValuesBranchMap = llvm::DenseMap<const llvm::Value*, ValueBranchMap>;

   using ValuesSwitchMap = llvm::DenseMap<const llvm::Value*, ValueSwitchMap>;

   /// This class represents our constraint graph. This graph is used to
   /// perform all computations in our analysis.
   class ConstraintGraph
   {
    protected:
      // The variables of the source program and the nodes which represent them.
      VarNodes vars;
      // The operations of the source program and the nodes which represent them.
      GenOprs oprs;

    private:
      // Save the last Function analyzed
      const llvm::Function* func{nullptr};
      // A map from variables to the operations that define them
      DefMap defMap;
      // A map from variables to the operations where these variables are used.
      UseMap useMap;
      // A map from variables to the operations where these
      // variables are present as bounds
      SymbMap symbMap;
      // This data structure is used to store intervals, basic blocks and intervals
      // obtained in the branches.
      ValuesBranchMap valuesBranchMap;
      ValuesSwitchMap valuesSwitchMap;

      // Vector containing the constants from a SCC
      // It is cleared at the beginning of every SCC resolution
      llvm::SmallVector<llvm::APInt, 2> constantvector;

      /// Adds a BinaryOp in the graph.
      void addBinaryOp(const llvm::Instruction* I, llvm::ModulePass* modulePass, const llvm::DataLayout* DL);
      /// Adds a TernaryOp in the graph.
      void addTernaryOp(const llvm::Instruction* I, llvm::ModulePass* modulePass, const llvm::DataLayout* DL);
      /// Adds a PhiOp in the graph.
      void addPhiOp(const llvm::PHINode* Phi, llvm::ModulePass* modulePass, const llvm::DataLayout* DL);
      // Adds a SigmaOp to the graph.
      void addSigmaOp(const llvm::PHINode* Sigma, llvm::ModulePass* modulePass, const llvm::DataLayout* DL, bool* changed);
      /// Add LoadOp in the graph
      void addLoadOp(const llvm::LoadInst* LI, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::ModulePass* modulePass, const llvm::DataLayout* DL,
                     llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store, bool* changed);
      /// Add StoreOp in the graph
      void addStoreOp(const llvm::StoreInst* SI, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::ModulePass* modulePass, llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store,
                      const llvm::DataLayout* DL, bool* changed);

      /// Takes an instruction and creates an operation.
      void buildOperations(const llvm::Instruction* I, llvm::ModulePass* modulePass, const llvm::DataLayout* DL, Andersen_AA* PtoSets_AA, bool arePointersResolved,
                           llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store, bool* changed);
      void buildValueBranchMap(const llvm::BranchInst* br, const llvm::DataLayout* DL);
      void buildValueSwitchMap(const llvm::SwitchInst* sw, const llvm::DataLayout* DL);
      void buildValueMaps(const llvm::Function& F, const llvm::DataLayout* DL);

      //	void clearValueMaps();

      void insertConstantIntoVector(llvm::APInt constantval);
      llvm::APInt getFirstGreaterFromVector(const llvm::SmallVector<llvm::APInt, 2>& constantvector, const llvm::APInt& val);
      llvm::APInt getFirstLessFromVector(const llvm::SmallVector<llvm::APInt, 2>& constantvector, const llvm::APInt& val);
      void buildConstantVector(const llvm::SmallPtrSet<VarNode*, 32>& component, const UseMap& compusemap);
      llvm::SmallPtrSet<const llvm::Value*, 6> ComputeConflictingStores(const llvm::StoreInst* SI, const llvm::Value* GV, const llvm::Instruction* instr, Andersen_AA* PtoSets_AA,
                                                                        llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store, llvm::ModulePass* modulePass, bool* changed);

    protected:
      // Perform the widening and narrowing operations
      void update(const UseMap& compUseMap, llvm::DenseSet<eValue>& actv, bool (*meet)(BasicOp* op, const llvm::SmallVector<llvm::APInt, 2>* constantvector));
      void update(unsigned nIterations, const UseMap& compUseMap, llvm::DenseSet<eValue>& actv);

      virtual void preUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& entryPoints) = 0;
      virtual void posUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& activeVars, const llvm::SmallPtrSet<VarNode*, 32>* component) = 0;

    public:
      ConstraintGraph() = default;
      virtual ~ConstraintGraph();
      ConstraintGraph(const ConstraintGraph&) = delete;
      ConstraintGraph(ConstraintGraph&&) = delete;
      ConstraintGraph& operator=(const ConstraintGraph&) = delete;
      ConstraintGraph& operator=(ConstraintGraph&&) = delete;
      /// Adds a VarNode in the graph.
      VarNode* addVarNode(const llvm::Value* V, const llvm::Value* GV, const llvm::DataLayout* DL);

      GenOprs* getOprs()
      {
         return &oprs;
      }
      DefMap* getDefMap()
      {
         return &defMap;
      }
      UseMap* getUseMap()
      {
         return &useMap;
      }
      /// Adds an UnaryOp to the graph.
      void addUnaryOp(const llvm::Instruction* I, llvm::ModulePass* modulePass, const llvm::DataLayout* DL);
      /// Iterates through all instructions in the function and builds the graph.
      void buildGraph(const llvm::Function& F, llvm::ModulePass* modulePass, const llvm::DataLayout* DL, Andersen_AA* PtoSets_AA, bool arePointersResolved,
                      llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store, bool* changed);
      void buildVarNodes(const llvm::DataLayout* DL);
      void buildSymbolicIntersectMap();
      UseMap buildUseMap(const llvm::SmallPtrSet<VarNode*, 32>& component);
      void propagateToNextSCC(const llvm::SmallPtrSet<VarNode*, 32>& component);

      /// Finds the intervals of the variables in the graph.
      void findIntervals();
      void generateEntryPoints(const llvm::SmallPtrSet<VarNode*, 32>& component, llvm::DenseSet<eValue>& entryPoints);
      void fixIntersectsSC(VarNode* varNode);
      void fixIntersects(const llvm::SmallPtrSet<VarNode*, 32>& component);
      void generateActivesVars(const llvm::SmallPtrSet<VarNode*, 32>& component, llvm::DenseSet<eValue>& activeVars);

      /// Prints the content of the graph in dot format. For more information
      /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
      void print(const llvm::Function& F, llvm::raw_ostream& OS) const;
      void printToFile(const llvm::Function& F, const std::string& FileName);
      /// Allow easy printing of graphs from the debugger.
      void dump(const llvm::Function& F) const
      {
         print(F, llvm::dbgs());
         llvm::dbgs() << '\n';
      }
      void computeStats();
      Range getRange(eValue v, const llvm::DataLayout* DL);
   };

   class Cousot : public ConstraintGraph
   {
    private:
      void preUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& entryPoints) override;
      void posUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& entryPoints, const llvm::SmallPtrSet<VarNode*, 32>* component) override;

    public:
      Cousot() = default;
   };

   class CropDFS : public ConstraintGraph
   {
    private:
      void preUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& entryPoints) override;
      void posUpdate(const UseMap& compUseMap, llvm::DenseSet<eValue>& activeVars, const llvm::SmallPtrSet<VarNode*, 32>* component) override;
      void storeAbstractStates(const llvm::SmallPtrSet<VarNode*, 32>& component);
      void crop(const UseMap& compUseMap, BasicOp* op);

    public:
      CropDFS() = default;
   };

   class Nuutila
   {
    public:
      VarNodes* variables;
      int index;
      llvm::DenseMap<eValue, int> dfs;
      llvm::DenseMap<eValue, eValue> root;
      llvm::DenseSet<eValue> inComponent;
      llvm::DenseMap<eValue, llvm::SmallPtrSet<VarNode*, 32>*> components;
      std::deque<eValue> worklist;
#ifdef SCC_DEBUG
      bool checkWorklist();
      bool checkComponents();
      bool checkTopologicalSort(UseMap* useMap);
      bool hasEdge(llvm::SmallPtrSet<VarNode*, 32>* componentFrom, llvm::SmallPtrSet<VarNode*, 32>* componentTo, UseMap* useMap);
#endif
    public:
      Nuutila(VarNodes* varNodes, UseMap* useMap, SymbMap* symbMap);
      ~Nuutila();
      Nuutila(const Nuutila&) = delete;
      Nuutila(Nuutila&&) = delete;
      Nuutila& operator=(const Nuutila&) = delete;
      Nuutila& operator=(Nuutila&&) = delete;

      void addControlDependenceEdges(SymbMap* symbMap, UseMap* useMap, VarNodes* vars);
      void delControlDependenceEdges(UseMap* useMap);
      void visit(eValue V, std::stack<eValue>& stack, UseMap* useMap);
      using iterator = std::deque<eValue>::reverse_iterator;
      iterator begin()
      {
         return worklist.rbegin();
      }
      iterator end()
      {
         return worklist.rend();
      }
   };

   class Meet
   {
    public:
      static bool widen(BasicOp* op, const llvm::SmallVector<llvm::APInt, 2>* constantvector);
      static bool narrow(BasicOp* op, const llvm::SmallVector<llvm::APInt, 2>* constantvector);
      static bool crop(BasicOp* op, const llvm::SmallVector<llvm::APInt, 2>* constantvector);
      static bool growth(BasicOp* op, const llvm::SmallVector<llvm::APInt, 2>* constantvector);
      static bool fixed(BasicOp* op);
   };

   class RangeAnalysis
   {
    protected:
      ConstraintGraph* CG{nullptr};
      llvm::ValueMap<const llvm::Value*, Range> ranges;
      virtual void finalizeRangeAnalysis(const llvm::Module& M) = 0;

    public:
      RangeAnalysis() = default;
      virtual ~RangeAnalysis() = default;
      RangeAnalysis(const RangeAnalysis&) = delete;
      RangeAnalysis& operator=(const RangeAnalysis&) = delete;
      RangeAnalysis(RangeAnalysis&&) = delete;
      RangeAnalysis& operator=(RangeAnalysis&&) = delete;

      /** Gets the maximum bit width of the operands in the instructions of the
       * function. This function is necessary because the class APInt only
       * supports binary operations on operands that have the same number of
       * bits; thus, all the APInts that we allocate to process the function will
       * have the maximum bit size. The complexity of this function is linear on
       * the number of operands used in the function.
       */
      static unsigned getMaxBitWidth(const llvm::Function& F);
      static void updateConstantIntegers(unsigned maxBitWidth);
      virtual const Range getRange(const llvm::Value* v);
      virtual void printRanges(llvm::Module& M, llvm::raw_ostream& O);
   };

   class InterProceduralRACropDFSHelper : public RangeAnalysis
   {
      bool changed;

    public:
      InterProceduralRACropDFSHelper()
      {
      }
      ~InterProceduralRACropDFSHelper() override;
      InterProceduralRACropDFSHelper(const InterProceduralRACropDFSHelper&) = delete;
      InterProceduralRACropDFSHelper& operator=(const InterProceduralRACropDFSHelper&) = delete;
      InterProceduralRACropDFSHelper(InterProceduralRACropDFSHelper&&) = delete;
      InterProceduralRACropDFSHelper& operator=(InterProceduralRACropDFSHelper&&) = delete;
      bool runOnModule(const llvm::Module& M, llvm::ModulePass* modulePass, Andersen_AA* PtoSets_AA);
      static unsigned getMaxBitWidth(const llvm::Module& M);

    private:
      void MatchParametersAndReturnValues(const llvm::Function& F, ConstraintGraph& G, const llvm::DataLayout* DL);
      void finalizeRangeAnalysis(const llvm::Module& M) override;
   };

} // namespace RangeAnalysis
#endif // _RANGEANALYSIS_RANGEANALYSIS_H
