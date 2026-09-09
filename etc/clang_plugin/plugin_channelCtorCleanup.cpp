/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file plugin_channelCtorCleanup.cpp
 * @brief Strips ac_channel / hls::stream construction from the kernel IR.
 *
 * ac_channel.h is one class with one layout for synthesis and for simulation alike, which means its
 * std::deque<T> member is there in both. The deque constructor allocates, so a channel constructed
 * inside the synthesised call graph - a stack local under #pragma HLS dataflow, a function-local
 * static, a member of either - emits operator new / operator delete into the kernel, and bambu has no
 * functional unit for them.
 *
 * This pass erases the constructor and destructor calls on channel objects and lets GlobalDCE take the
 * bodies with them. The object stays exactly as it is, which is what keeps the same IR runnable on the
 * CPU; only bambu's pipeline runs this pass, so the host-side reference keeps its real queue.
 *
 * Deleting just the allocation would be worse: it would leave the deque half-initialised and the
 * destructor would free whatever _M_map happens to hold. A fake constructor is not needed either -
 * nothing in the kernel reads the object.
 *
 * Plugin invocation
 * -----------------
 *   opt --load-pass-plugin=opt_channelCtorCleanup.so --passes=channelCtorCleanup in.ll -o out.ll
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */

#include "plugin_channelCtorCleanup.hpp"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <cstdlib>
#include <cxxabi.h>
#include <llvm/ADT/SmallSet.h>
#include <memory>
#include <string>

#if PANDA_LLVM_CLANG_MAJOR >= 13
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#else
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#endif

#if LLVM_VERSION_MAJOR >= 16
#include "llvm/IRPrinter/IRPrintingPasses.h"
#else
#include "llvm/IR/IRPrintingPasses.h"
#endif

using namespace llvm;

namespace
{
   enum class ChannelOp
   {
      none,       ///< not a channel constructor or destructor
      erase_ctor, ///< default constructor
      erase_dtor, ///< destructor
      bad_ctor,   ///< any other constructor: a pre-filled fifo, which hardware cannot do
      bad_copy,   ///< copy construction or copy assignment: a fifo cannot be duplicated
      seam        ///< read / peek / write: the *_bambu_internal members InterfaceInfer turns into ports
   };

   std::string demangle(StringRef name)
   {
      int status = 0;
      const std::unique_ptr<char, void (*)(void*)> res(
          abi::__cxa_demangle(name.str().c_str(), nullptr, nullptr, &status), std::free);
      return (status == 0 && res) ? std::string(res.get()) : std::string();
   }

   /// Classifies a callee from its demangled name. Working from the name rather than from the pointee
   /// struct type is what keeps this going under opaque pointers, where the argument no longer carries
   /// the type at all, and under LLVM's renaming of duplicate struct names (%class.ac_channel,
   /// %class.ac_channel.0, ...).
   ChannelOp classify(StringRef mangled)
   {
      // Demangling every call in the module would cost an allocation each time; a channel constructor
      // always carries its class name in the mangled form, so this filter cannot miss one.
      if(mangled.find("ac_channel") == StringRef::npos && mangled.find("stream") == StringRef::npos)
      {
         return ChannelOp::none;
      }
      // Every seam member is named *_bambu_internal and they all get the same treatment, so one test
      // covers read, peek and write alike - and cannot miss a future one.
      if(mangled.find("_bambu_internal") != StringRef::npos)
      {
         return ChannelOp::seam;
      }
      const std::string full = demangle(mangled);
      // Everything from the first parenthesis on is the argument list. A channel of function pointers
      // would put one earlier, but it is not synthesisable to begin with.
      const size_t paren = full.find('(');
      if(paren == std::string::npos)
      {
         return ChannelOp::none;
      }
      const std::string qualified = full.substr(0, paren);
      const size_t sep = qualified.rfind("::");
      if(sep == std::string::npos)
      {
         return ChannelOp::none;
      }
      const std::string owner = qualified.substr(0, sep);
      std::string fn = qualified.substr(sep + 2);
      // The function name is the last component of a qualified name and never contains "::" itself, so
      // the last separator finds it without any bracket matching.
      const bool dtor = !fn.empty() && fn.front() == '~';
      if(dtor)
      {
         fn.erase(0, 1);
      }
      // Copy construction and copy assignment demangle to the same tail, "(<owner> const&)", so one
      // predicate covers both.
      const bool copies_a_channel = full.compare(paren, std::string::npos, "(" + owner + " const&)") == 0;
      // operator= is not named after its class, so it cannot pass the constructor gate below and has to
      // be recognised here. Testing the owner is what keeps this off the operator= of an unrelated class
      // that merely holds a channel as a member.
      if(fn == "operator=")
      {
         const bool channel_owner =
             owner.compare(0, 11, "ac_channel<") == 0 || owner.compare(0, 12, "hls::stream<") == 0;
         return (copies_a_channel && channel_owner) ? ChannelOp::bad_copy : ChannelOp::none;
      }
      // A constructor is named after the class it builds. That alone rejects bambu_bitcast_payload, the
      // union the non-blocking payload travels in: it is declared inside ac_channel, so its mangled name
      // carries "ac_channel" too, but its constructor is named after the union.
      if(fn != "ac_channel" && !(fn == "stream" && owner.compare(0, 5, "hls::") == 0))
      {
         return ChannelOp::none;
      }
      if(dtor)
      {
         return ChannelOp::erase_dtor;
      }
      if(copies_a_channel)
      {
         return ChannelOp::bad_copy;
      }
      // Taking no argument is what tells the default constructor from the ones that put elements in the
      // channel first: ac_channel<int> c(8), an initializer list, a binary file.
      return full.compare(paren, std::string::npos, "()") == 0 ? ChannelOp::erase_ctor : ChannelOp::bad_ctor;
   }

   /// Only calls are looked at, never invokes: bambu compiles every C++ input with -fno-exceptions
   /// (CompilerWrapper.cpp:1108), so clang emits no invoke at all, and a channel only exists in C++
   /// input to begin with - ac_channel.h refuses to compile as C.
   Function* calledFunction(CallInst& CI)
   {
      if(Function* direct = CI.getCalledFunction())
      {
         return direct;
      }
      // Under typed pointers - which bambu always asks for, via -Xclang -no-opaque-pointers - a base
      // class destructor called on a derived pointer goes through a bitcast of the signature:
      //   call void bitcast (void (%class.ac_channel*)* @..D2Ev to void (%"class.hls::stream"*)*)(...)
      // getCalledFunction() is null there, so the callee is recovered from the last operand.
      return dyn_cast<Function>(CI.getOperand(CI.getNumOperands() - 1)->stripPointerCasts());
   }

   /// The object a constructor or destructor runs on, and equally the handler an atexit registers.
   Value* firstArgument(CallInst& CI)
   {
      return CI.getNumOperands() > 1 ? CI.getArgOperand(0) : nullptr;
   }

   /// A channel object may only be handed to calls - the seam (*_bambu_internal), the sub-functions a
   /// #pragma HLS dataflow passes it on to, lifetime intrinsics, its own constructor and destructor.
   /// Anything that loads or stores through it is reading the queue itself (size(), empty(),
   /// operator[]), which is not synthesisable; erasing the constructor under it would silently give
   /// wrong answers, so it is reported instead. Returns the offending instruction, or null.
   const Instruction* firstDisallowedUse(const Value* obj)
   {
      SmallVector<const Value*, 16> worklist{obj};
      SmallPtrSet<const Value*, 16> seen;
      while(!worklist.empty())
      {
         const Value* v = worklist.pop_back_val();
         if(!seen.insert(v).second)
         {
            continue;
         }
         for(const User* u : v->users())
         {
            if(isa<LoadInst>(u) || isa<StoreInst>(u))
            {
               return cast<Instruction>(u);
            }
            if(isa<BitCastInst>(u) || isa<GetElementPtrInst>(u) || isa<AddrSpaceCastInst>(u))
            {
               worklist.push_back(cast<Value>(u));
            }
            // Calls are left alone: they pass the pointer on rather than touch the storage.
         }
      }
      return nullptr;
   }

   /// True when F does nothing observable any more: it writes only to its own stack slots, and every
   /// call it still makes goes to a function that is itself empty. Once the channel destructors are
   /// gone, that is what a static's __dtor_ wrapper becomes - it walks a chain of destructors that have
   /// all been emptied. Keeping its atexit registration alive would keep the whole chain alive with it,
   /// and bambu would then go looking for a module to synthesise it into.
   ///
   /// This runs at pipeline start, on IR the optimiser has not touched yet, so every function still
   /// opens by spilling its parameters to an alloca. Those stores are invisible outside the function
   /// and must not count, or nothing would ever look empty.
   bool isEffectivelyEmpty(Function* F, SmallPtrSetImpl<const Function*>& visiting, unsigned depth)
   {
      if(!F || F->isDeclaration() || depth > 8)
      {
         return false;
      }
      if(!visiting.insert(F).second)
      {
         return true; // already on the stack: a recursive chain of empty bodies is still empty
      }
      for(BasicBlock& BB : *F)
      {
         for(Instruction& I : BB)
         {
            if(auto* SI = dyn_cast<StoreInst>(&I))
            {
               if(!isa<AllocaInst>(SI->getPointerOperand()->stripPointerCasts()))
               {
                  return false;
               }
               continue;
            }
            if(auto* CI = dyn_cast<CallInst>(&I))
            {
               Function* callee = calledFunction(*CI);
               if(callee && !callee->isIntrinsic() && !isEffectivelyEmpty(callee, visiting, depth + 1))
               {
                  return false;
               }
            }
         }
      }
      return true;
   }

   void eraseCall(CallInst* CI)
   {
      if(!CI->use_empty())
      {
         // atexit returns a status: zero is the success the caller would have seen.
         CI->replaceAllUsesWith(Constant::getNullValue(CI->getType()));
      }
      CI->eraseFromParent();
   }

   std::string describe(const Function& F, const Value* obj)
   {
      std::string where = "'" + F.getName().str() + "'";
      if(obj && obj->hasName())
      {
         where += ", channel '" + obj->getName().str() + "'";
      }
      return where;
   }

   [[noreturn]] void reject(const Twine& message)
   {
      report_fatal_error(Twine("bambu: ") + message);
   }

   cl::opt<std::string> pandaTempPath("panda-temp-ctor-cleanup",
                                      cl::desc("Specify the directory where the bambu IR raw file will be written"),
                                      cl::value_desc("directory path"), cl::Required);
} // namespace

namespace llvm
{
   char ChannelCtorCleanupPass::ID = 0;
} // namespace llvm

bool ChannelCtorCleanupPass::exec(Module& M)
{
   SmallVector<CallInst*, 16> toErase;
   SmallVector<CallInst*, 4> registrations;
   SmallSet<Function*, 8> seamFns;

   for(Function& F : M)
   {
      for(BasicBlock& BB : F)
      {
         for(Instruction& I : BB)
         {
            auto* CI = dyn_cast<CallInst>(&I);
            if(!CI)
            {
               continue;
            }
            Function* callee = calledFunction(*CI);
            if(!callee)
            {
               continue;
            }
            // A function-local static registers its destructor at run time: __cxa_atexit(dtor, object,
            // handle), or - with bambu's -fno-use-cxa-atexit - atexit(__dtor_<mangled variable>).
            if(callee->getName() == "atexit" || callee->getName() == "__cxa_atexit")
            {
               registrations.push_back(CI);
               continue;
            }
            const ChannelOp op = classify(callee->getName());
            if(op == ChannelOp::none)
            {
               continue;
            }
            Value* obj = firstArgument(*CI);
            const Value* base = obj ? obj->stripPointerCasts() : nullptr;
            if(op == ChannelOp::bad_copy)
            {
               reject("a channel is copied inside the kernel in " + describe(F, base) +
                      ". A fifo cannot be duplicated in hardware: the design would be left with one "
                      "queue where the software reference has two, so the two would not agree. Pass the "
                      "channel by reference instead. Call: " +
                      callee->getName());
            }
            if(op == ChannelOp::bad_ctor)
            {
               reject("a channel with a non-default constructor is built inside the kernel in " +
                      describe(F, base) +
                      ". Pre-filling a fifo has no hardware equivalent, so the design and the software "
                      "reference would not agree; declare the channel empty and write it from the kernel, or "
                      "move it to the testbench. Constructor: " +
                      callee->getName());
            }
            // Judge only from a constructor. Reaching a channel through a destructor alone means the
            // constructor was already inlined, and the stores on the object are then its own
            // construction - indistinguishable from a kernel reading the queue.
            if(op == ChannelOp::erase_ctor && base)
            {
               if(const Instruction* bad = firstDisallowedUse(base))
               {
                  std::string instr;
                  raw_string_ostream os(instr);
                  os << *bad;
                  reject("channel storage is accessed directly in " + describe(F, base) +
                         ". size(), empty() and operator[] read the software queue and have no hardware "
                         "meaning; remove them from the kernel. Reached from: " +
                         callee->getName() + ". Offending instruction:" + os.str());
               }
            }
            if(op == ChannelOp::seam)
            {
               seamFns.insert(callee);
            }
            else
            {
               toErase.push_back(CI);
            }
         }
      }
   }

   bool changed = !toErase.empty() || !seamFns.empty();
   for(CallInst* CI : toErase)
   {
      eraseCall(CI);
   }

   for(Function* F : seamFns)
   {
      F->deleteBody();
      F->setComdat(nullptr);
      F->setLinkage(GlobalValue::ExternalLinkage);
   }

   // Only now, with the destructors gone, is it possible to tell whether a registered exit handler
   // still has anything to do.
   for(CallInst* CI : registrations)
   {
      Value* handler = firstArgument(*CI);
      SmallPtrSet<const Function*, 8> visiting;
      if(handler && isEffectivelyEmpty(dyn_cast<Function>(handler->stripPointerCasts()), visiting, 0))
      {
         eraseCall(CI);
         changed = true;
      }
   }
   return changed;
}

PreservedAnalyses ChannelCtorCleanupPass::run(Module& M, ModuleAnalysisManager& /*AM*/)
{
   return exec(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool ChannelCtorCleanupPass::runOnModule(Module& M)
{
   return exec(M);
}

StringRef ChannelCtorCleanupPass::getPassName() const
{
   return "ChannelCtorCleanupPass";
}

// ---------------------------------------------------------------------------
// Registration
//
// The pass name must match the plugin name: CompilerWrapper::load_and_run_plugin feeds the plugin name
// straight into opt's --passes= list.
// ---------------------------------------------------------------------------

// Registered for both pass managers: opt is driven with the legacy `-channelCtorCleanup` syntax up to
// clang 15 (see passesType::add_pass in CompilerWrapper.cpp) and with `--passes=` from clang 16 on.
static RegisterPass<ChannelCtorCleanupPass> XPass("channelCtorCleanup",
                                                  "Remove ac_channel/hls::stream construction from the kernel",
                                                  false /* Only looks at CFG */, false /* Analysis Pass */);

#if PANDA_LLVM_CLANG_MAJOR >= 13

#if PANDA_LLVM_CLANG_MAJOR < 16
using PandaOptLevel = PassBuilder::OptimizationLevel;
#else
using PandaOptLevel = OptimizationLevel;
#endif

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "channelCtorCleanup", LLVM_VERSION_STRING, [](PassBuilder& PB) {
              const auto load = [](ModulePassManager& MPM) {
                 auto createOutputStream = [](const std::string& Filename) -> std::unique_ptr<llvm::raw_fd_ostream> {
                     std::error_code EC;
                     auto OS = std::make_unique<llvm::raw_fd_ostream>(pandaTempPath + "/" + Filename, EC);
                     if(EC)
                     {
                        llvm::errs() << "Error opening " << Filename << ": " << EC.message() << "\n";
                        return nullptr;
                     }
                     return OS;
                 };
                 static auto pluginBeginOS = createOutputStream("begin_plugin_channelCtorCleanup.ll");
                 if(pluginBeginOS)
                    MPM.addPass(llvm::PrintModulePass(*pluginBeginOS));
                 MPM.addPass(ChannelCtorCleanupPass());
                 static auto pluginEndOS = createOutputStream("end_plugin_channelCtorCleanup.ll");
                 if(pluginEndOS)
                    MPM.addPass(llvm::PrintModulePass(*pluginEndOS));
                 // Takes the now unused constructor and destructor bodies, and with them operator new,
                 // operator delete and the deque helpers left out of line.
                 MPM.addPass(GlobalDCEPass());
              };
              PB.registerPipelineParsingCallback(
                  [&](StringRef name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) {
                     if(name != "channelCtorCleanup")
                     {
                        return false;
                     }
                     load(MPM);
                     return true;
                  });
              PB.registerPipelineStartEPCallback([&](ModulePassManager& MPM, PandaOptLevel) { load(MPM); });
           }};
}

#else

#if ADD_RSP
static void loadPass(const PassManagerBuilder&, legacy::PassManagerBase& PM)
{
   PM.add(new ChannelCtorCleanupPass());
   PM.add(createGlobalDCEPass());
}

static RegisterStandardPasses channelCtorCleanup_Ox(PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
#endif

#endif
