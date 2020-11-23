//===- eSSA.hpp - Build eSSA ----------------------------------------*-C++-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief  This file implements the eSSA analysis, which creates an Extended
/// SSA form for very useful for value range analysis.
///
/// Copies of these operations are inserted into the true/false edge (and after
/// assumes), and information attached to the copies.  All uses of the original
/// operation in blocks dominated by the true/false edge (and assume), are
/// replaced with uses of the copies.  This enables passes to easily and sparsely
/// propagate condition based info into the operations that may be affected.
///
/// Example:
/// %cmp = icmp eq i32 %x, 50
/// br i1 %cmp, label %true, label %false
/// true:
/// ret i32 %x
/// false:
/// ret i32 1
///
/// will become
///
/// %cmp = icmp eq i32, %x, 50
/// br i1 %cmp, label %true, label %false
/// true:
/// %x.0 = phi(i32 %x)
/// ret i32 %x.0
/// false:
/// ret i32 1
///
/// In order to reduce the number of copies inserted, the PHI/copy operation is only
/// inserted where it would actually be live.  This means if there are no uses of
/// an operation dominated by the branch edges, or by an assume, the associated
/// predicate info is never inserted.
///
///
//===----------------------------------------------------------------------===//

#ifndef ESSA_HPP
#define ESSA_HPP

#define DEBUG_ESSA 0

namespace llvm
{
   class Function;
   class ModulePass;
} // namespace llvm
class eSSA
{
 public:
   eSSA()
   {
   }
   ~eSSA()
   {
   }
   bool runOnFunction(llvm::Function& fun, llvm::ModulePass* modulePass, bool* changed);
};

#endif
