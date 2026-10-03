/* Copyright (C) 2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
// Exercise the same cross-iteration predicate used by dumpBambuIr. The loop IR
// is parsed and verified with each supported LLVM, then queried with real BasicAA.
#include "channel_seam_aa.hpp"
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/SourceMgr.h>
#include <cstdio>
#include <string>

struct Case
{
   const char* name;
   const char* entry;
   const char* loop;
   const char* calls;
   bool noalias;
   bool independent;
};

static bool check(const Case& test)
{
   const std::string qualifier = test.noalias ? " noalias " : " ";
   std::string ir =
       "declare void @read(PTR) EFFECTS\n"
       "declare void @write(PTR) EFFECTS\n"
       "declare void @read_out(PTR, PTR) EFFECTS\n"
       "declare void @write_out(PTR, PTR) EFFECTS\n"
       "declare void @unknown(PTR)\n"
       "define void @kernel(PTR" + qualifier + "%a, PTR" + qualifier + "%b, PTR" + qualifier +
       "%x, PTR" + qualifier + "%y) {\nentry:\n" + test.entry + "\n br label %loop\nloop:\n"
       " %i = phi i32 [0, %entry], [%next, %loop]\n" + test.loop + "\n" + test.calls +
       "\n %next = add i32 %i, 1\n %done = icmp eq i32 %next, 4\n"
       " br i1 %done, label %exit, label %loop\nexit:\n ret void\n}\n";
#if PANDA_LLVM_CLANG_MAJOR >= 16
   const std::string ptr = "ptr", effects = "memory(argmem: readwrite)";
#else
   const std::string ptr = "i8*", effects = "argmemonly";
#endif
   for(std::size_t pos = 0; (pos = ir.find("PTR", pos)) != std::string::npos; pos += ptr.size())
   {
      ir.replace(pos, 3, ptr);
   }
   for(std::size_t pos = 0; (pos = ir.find("EFFECTS", pos)) != std::string::npos; pos += effects.size())
   {
      ir.replace(pos, 7, effects);
   }
   llvm::LLVMContext context;
   llvm::SMDiagnostic error;
   auto module = llvm::parseAssemblyString(ir, error, context);
   if(!module)
   {
      error.print(test.name, llvm::errs());
      return false;
   }
   if(llvm::verifyModule(*module, &llvm::errs()))
   {
      return false;
   }
   auto& F = *module->getFunction("kernel");
   llvm::TargetLibraryInfoImpl TLIImpl;
   llvm::TargetLibraryInfo TLI(TLIImpl);
   llvm::AssumptionCache AC(F);
   llvm::DominatorTree DT(F);
#if PANDA_LLVM_CLANG_MAJOR >= 7
   llvm::BasicAAResult basicAA(module->getDataLayout(), F, TLI, AC, &DT);
#else
   llvm::BasicAAResult basicAA(module->getDataLayout(), TLI, AC, &DT);
#endif
   llvm::AAResults AA(TLI);
   AA.addAAResult(basicAA);
   const llvm::CallInst* A = nullptr;
   const llvm::CallInst* B = nullptr;
   for(const auto& BB : F)
   {
      for(const auto& I : BB)
      {
         if(const auto* call = llvm::dyn_cast<llvm::CallInst>(&I))
         {
            if(!A)
            {
               A = call;
            }
            else
            {
               B = call;
            }
         }
      }
   }
   if(!A || !B)
   {
      return false;
   }
   const bool independent = bambu_channel_seam::callsModRefIndependentAcrossIterations(A, B, AA);
   const bool ok = independent == test.independent;
   std::printf("[%s] %s: expected independent=%d, got=%d\n", ok ? "PASS" : "FAIL", test.name,
               test.independent, independent);
   return ok;
}

int main()
{
   const char* alternating =
       " %bit = and i32 %i, 1\n %even = icmp eq i32 %bit, 0\n"
       " %p = select i1 %even, PTR %a, PTR %b\n %q = select i1 %even, PTR %b, PTR %a\n";
   const char* calls = " call void @read(PTR %p)\n call void @write(PTR %q)\n";
   const char* direct = " call void @read(PTR %a)\n call void @write(PTR %b)\n";
   const Case cases[] = {
       {"alternating channels (select)", "", alternating, calls, true, false},
       {"alternating channels (phi)", "",
        " %p = phi PTR [%a, %entry], [%q, %loop]\n %q = phi PTR [%b, %entry], [%p, %loop]\n",
        calls, true, false},
       {"stable noalias parameters", "", "", direct, true, true},
       {"parameters may alias", "", "", direct, false, false},
       {"same channel", "", "", " call void @read(PTR %a)\n call void @write(PTR %a)\n", true, false},
       {"missing memory contract", "", "", " call void @read(PTR %a)\n call void @unknown(PTR %b)\n",
        true, false},
       {"constant-offset channel addresses", "",
        " %p = getelementptr i8, PTR %a, i32 1\n %q = getelementptr i8, PTR %b, i32 1\n",
        calls, true, true},
       {"entry-block computed addresses",
        " %p = getelementptr i8, PTR %a, i32 1\n %q = getelementptr i8, PTR %b, i32 1\n", "",
        calls, true, true},
       {"shared output", "", "", " call void @read_out(PTR %a, PTR %x)\n call void @write_out(PTR %b, PTR %x)\n",
        true, false},
       {"distinct stable outputs", "", "", " call void @read_out(PTR %a, PTR %x)\n call void @write_out(PTR %b, PTR %y)\n",
        true, true},
       {"alternating output pointers", "",
        " %bit = and i32 %i, 1\n %even = icmp eq i32 %bit, 0\n"
        " %p = select i1 %even, PTR %x, PTR %y\n %q = select i1 %even, PTR %y, PTR %x\n",
        " call void @read_out(PTR %a, PTR %p)\n call void @write_out(PTR %b, PTR %q)\n", true, false},
   };
   int fails = 0;
   for(const auto& test : cases)
   {
      fails += !check(test);
   }
   return fails;
}
