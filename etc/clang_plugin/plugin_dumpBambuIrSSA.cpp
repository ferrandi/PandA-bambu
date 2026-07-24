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
 *   Copyright (C) 2018-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file plugin_dumpBambuIrSSA.cpp
 * @brief Plugin to dump functions and global variables starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
#include "condInstCombIfTaggedPass.hpp"
#include "plugin_includes.hpp"
#include "pointerResolutionPass.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/TypeBasedAliasAnalysis.h>
#include <llvm/CodeGen/Passes.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

#if PANDA_LLVM_CLANG_MAJOR != 4
#include <llvm/Analysis/MemorySSA.h>
#else
#include <llvm/Transforms/Utils/MemorySSA.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR > 5
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR >= 7
#ifndef VVD
#include <llvm/Transforms/Utils.h>
#endif
#endif
#if PANDA_LLVM_CLANG_MAJOR >= 13
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/ArgumentPromotion.h>
#include <llvm/Transforms/IPO/DeadArgumentElimination.h>
#include <llvm/Transforms/IPO/ForceFunctionAttrs.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/GlobalSplit.h>
#include <llvm/Transforms/IPO/InferFunctionAttrs.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/MergeFunctions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/DeadStoreElimination.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/JumpThreading.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Scalar/LoopDeletion.h>
#include <llvm/Transforms/Scalar/LoopFlatten.h>
#include <llvm/Transforms/Scalar/LoopFuse.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/MemCpyOptimizer.h>
#include <llvm/Transforms/Scalar/MergedLoadStoreMotion.h>
#include <llvm/Transforms/Scalar/NewGVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Scalar/TailRecursionElimination.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#if PANDA_LLVM_CLANG_MAJOR >= 16
#include <llvm/Transforms/Scalar/LowerAtomicPass.h>
#include <llvm/Transforms/Scalar/SROA.h>
#else
#include <llvm/Transforms/Scalar/LowerAtomic.h>
#endif
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

#if PANDA_LLVM_CLANG_MAJOR >= 16
#include <llvm/IRPrinter/IRPrintingPasses.h>
#else
#include <llvm/IR/IRPrintingPasses.h>
#endif

#include <pugixml.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef CPP_LANGUAGE
#define CLANG_VERSION_SYMBOL_DUMP_SSA dumpBambuIrSSACpp
#define CLANG_VERSION_STRING_DUMP_SSA "dumpBambuIrSSACpp"
#else
#define CLANG_VERSION_SYMBOL_DUMP_SSA dumpBambuIrSSA
#define CLANG_VERSION_STRING_DUMP_SSA "dumpBambuIrSSA"
#endif

#define DEBUG_TYPE "dump-bambuir-ssa"

namespace llvm
{
   cl::opt<std::string> TopFunctionName("panda-topfname", cl::desc("Specify the name of the top function"),
                                        cl::value_desc("name of the top function"));
   cl::opt<std::string> outdir_name("panda-outputdir",
                                    cl::desc("Specify the directory where the bambuir file will be written"),
                                    cl::value_desc("directory path"), cl::Required);
   cl::list<std::string> InFile("panda-infile", cl::desc("Specify the name of the compiled source file"),
                                cl::value_desc("filename path"), cl::OneOrMore, cl::CommaSeparated);
   cl::opt<std::string> CostTable("panda-cost-table", cl::desc("Specify the cost per operation"),
                                  cl::value_desc("cost table"));

   static constexpr const char* BAMBU_ORIG_NAME_ATTR = "bambu.orig_name";
   static constexpr const char* BAMBU_ORIG_INDEX_ATTR = "bambu.orig_index";

   static bool parseIndexValue(const char* rawValue, size_t& outIndex)
   {
      if(rawValue == nullptr || *rawValue == '\0')
      {
         return false;
      }
      char* endPtr = nullptr;
      const auto parsedIndex = std::strtoul(rawValue, &endPtr, 10);
      if(endPtr == rawValue || *endPtr != '\0')
      {
         return false;
      }
      outIndex = parsedIndex;
      return true;
   }

   static bool parseParamIndex(const pugi::xml_attribute& indexAttr, size_t& outIndex)
   {
      if(indexAttr.empty())
      {
         return false;
      }
      return parseIndexValue(indexAttr.value(), outIndex);
   }

   static bool isGlobalBundle(const pugi::xml_node& bundleNode)
   {
      const auto globalAttr = bundleNode.attribute("global");
      return !globalAttr.empty() && std::string(globalAttr.value()) == "1";
   }

   static std::string getArgumentStringAttribute(const llvm::Function& F, unsigned int argIndex, const char* attrName)
   {
#if PANDA_LLVM_CLANG_MAJOR >= 5
      const auto argAttr = F.getAttributes().getParamAttr(argIndex, attrName);
#else
      const auto argAttr = F.getAttributes().getAttribute(argIndex + 1, attrName);
#endif
      if(!argAttr.isStringAttribute())
      {
         return "";
      }
      return argAttr.getValueAsString().str();
   }

   static bool pruneAndReindexFunctionParameters(pugi::xml_node xmlFunction, const llvm::Function& llvmFunction)
   {
      auto parametersRoot = xmlFunction.child("parameters");
      if(!parametersRoot)
      {
         return false;
      }
      const auto functionSymbol = std::string(xmlFunction.attribute("symbol").value());
      const auto functionName = std::string(xmlFunction.attribute("name").value());

      struct parameterInfo
      {
         pugi::xml_node node;
         size_t oldIndex = 0;
         bool hasIndex = false;
         bool selected = false;
      };

      std::vector<parameterInfo> xmlParameters;
      for(auto p : parametersRoot.children("parameter"))
      {
         parameterInfo info;
         info.node = p;
         info.hasIndex = parseParamIndex(p.attribute("index"), info.oldIndex);
         xmlParameters.push_back(info);
      }

      bool changed = false;
      std::vector<size_t> remainingOriginalIndices;
      std::vector<std::string> remainingOriginalNames;
      unsigned int argIndex = 0;
      for(const auto& arg : llvmFunction.args())
      {
         (void)arg;
         size_t desiredOldIndex = argIndex;
         size_t parsedOrigIndex = 0;
         const auto origIndexAttr = getArgumentStringAttribute(llvmFunction, argIndex, BAMBU_ORIG_INDEX_ATTR);
         if(parseIndexValue(origIndexAttr.c_str(), parsedOrigIndex))
         {
            desiredOldIndex = parsedOrigIndex;
         }
         remainingOriginalIndices.push_back(desiredOldIndex);

         auto originalName = getArgumentStringAttribute(llvmFunction, argIndex, BAMBU_ORIG_NAME_ATTR);
         if(!originalName.empty())
         {
            remainingOriginalNames.push_back(originalName);
         }

         parameterInfo* selectedParam = nullptr;
         for(auto& param : xmlParameters)
         {
            if(!param.selected && param.hasIndex && param.oldIndex == desiredOldIndex)
            {
               selectedParam = &param;
               break;
            }
         }

         if(!selectedParam && !originalName.empty())
         {
            for(auto& param : xmlParameters)
            {
               if(param.selected)
               {
                  continue;
               }
               auto paramName = std::string(param.node.attribute("name").value());
               if(paramName.empty())
               {
                  paramName = param.node.attribute("port").value();
               }
               if(paramName == originalName)
               {
                  selectedParam = &param;
                  break;
               }
            }
         }

         if(!selectedParam && desiredOldIndex != argIndex)
         {
            for(auto& param : xmlParameters)
            {
               if(!param.selected && param.hasIndex && param.oldIndex == argIndex)
               {
                  selectedParam = &param;
                  break;
               }
            }
         }

         if(!selectedParam)
         {
            continue;
         }

         selectedParam->selected = true;
         const auto newIndex = std::to_string(argIndex);
         auto indexAttr = selectedParam->node.attribute("index");
         if(indexAttr.empty())
         {
            selectedParam->node.append_attribute("index").set_value(newIndex.c_str());
            changed = true;
         }
         else if(newIndex != std::string(indexAttr.value()))
         {
            indexAttr.set_value(newIndex.c_str());
            changed = true;
         }
         ++argIndex;
      }

      for(const auto& param : xmlParameters)
      {
         if(!param.selected)
         {
            bool hasCorrespondingOriginalIndex =
                param.hasIndex && llvm::is_contained(remainingOriginalIndices, param.oldIndex);
            auto unmatchedParamName = std::string(param.node.attribute("name").value());
            if(unmatchedParamName.empty())
            {
               unmatchedParamName = param.node.attribute("port").value();
            }
            const bool hasCorrespondingOriginalName =
                !unmatchedParamName.empty() && llvm::is_contained(remainingOriginalNames, unmatchedParamName);

            if(!hasCorrespondingOriginalIndex && !hasCorrespondingOriginalName)
            {
               parametersRoot.remove_child(param.node);
               changed = true;
               continue;
            }
            if(unmatchedParamName.empty())
            {
               unmatchedParamName = "<unnamed>";
            }

            std::string unmatchedParamIndex = "<missing>";
            if(param.hasIndex)
            {
               unmatchedParamIndex = std::to_string(param.oldIndex);
            }
            else if(!param.node.attribute("index").empty())
            {
               unmatchedParamIndex = param.node.attribute("index").value();
            }

            const auto functionId = functionSymbol.empty() ? functionName : functionSymbol;
            std::string errorMessage = "Invalid architecture.xml parameter mapping for function '" + functionId + "'";
            if(!functionName.empty() && functionName != functionId)
            {
               errorMessage += " (name '" + functionName + "')";
            }
            errorMessage += ": parameter '" + unmatchedParamName + "' at index '" + unmatchedParamIndex +
                            "' has no correspondence with original LLVM parameters";
            report_fatal_error(llvm::StringRef(errorMessage));
         }
      }

      auto bundlesRoot = xmlFunction.child("bundles");
      if(bundlesRoot)
      {
         std::vector<std::string> usedBundleNames;
         for(auto& param : parametersRoot.children("parameter"))
         {
            const auto bundleName = std::string(param.attribute("bundle").value());
            if(bundleName.empty() || llvm::is_contained(usedBundleNames, bundleName))
            {
               continue;
            }
            usedBundleNames.push_back(bundleName);
         }

         for(auto bundle = bundlesRoot.child("bundle"); bundle;)
         {
            const auto nextBundle = bundle.next_sibling("bundle");
            const auto bundleName = std::string(bundle.attribute("name").value());
            if(!bundleName.empty() && !llvm::is_contained(usedBundleNames, bundleName) && !isGlobalBundle(bundle))
            {
               bundlesRoot.remove_child(bundle);
               changed = true;
            }
            bundle = nextBundle;
         }
      }

      return changed;
   }

   static bool pruneArchitectureXMLWithModule(pugi::xml_document& doc, const llvm::Module& M)
   {
      auto moduleNode = doc.child("module");
      if(!moduleNode)
      {
         return false;
      }

      bool changed = false;
      for(auto f = moduleNode.child("function"); f;)
      {
         const auto next = f.next_sibling("function");
         const auto funcSymbol = std::string(f.attribute("symbol").value());
         const auto* llvmFunction = funcSymbol.empty() ? nullptr : M.getFunction(funcSymbol);
         const bool isOriginalCsroaFunction = f.attribute("original").as_bool();
         if(!llvmFunction)
         {
            if(!isOriginalCsroaFunction)
            {
               moduleNode.remove_child(f);
               changed = true;
            }
            f = next;
            continue;
         }

         if(pruneAndReindexFunctionParameters(f, *llvmFunction))
         {
            changed = true;
         }
         f = next;
      }

      return changed;
   }

   static std::map<std::string, std::vector<std::string>> loadFunctionParamTracking(const pugi::xml_document& doc)
   {
      std::map<std::string, std::vector<std::string>> fun2params;
      const auto module = doc.child("module");
      for(auto& f : module.children("function"))
      {
         const auto funcSymbol = std::string(f.attribute("symbol").value());
         if(funcSymbol.empty())
         {
            continue;
         }

         auto& funcParms = fun2params[funcSymbol];
         const auto paramsRoot = f.child("parameters");
         for(auto& p : paramsRoot)
         {
            size_t idx = 0;
            if(!parseParamIndex(p.attribute("index"), idx))
            {
               continue;
            }
            if(funcParms.size() <= idx)
            {
               funcParms.resize(idx + 1);
            }

            auto paramName = std::string(p.attribute("name").value());
            if(paramName.empty())
            {
               paramName = p.attribute("port").value();
            }
            if(paramName.empty())
            {
               paramName = "P" + std::to_string(idx);
            }
            funcParms[idx] = paramName;
         }
      }
      return fun2params;
   }

   static void extendTopFunctionNamesFromArchitecture(const pugi::xml_document& doc, std::vector<std::string>& topNames)
   {
      const auto module = doc.child("module");
      for(auto& f : module.children("function"))
      {
         const auto funcSymbol = std::string(f.attribute("symbol").value());
         const auto funcName = std::string(f.attribute("name").value());
         const auto isDataflow = !f.attribute("dataflow_top").empty() || !f.attribute("dataflow_module").empty();
         if(!isDataflow || funcSymbol.empty())
         {
            continue;
         }
         if(llvm::find(topNames, funcSymbol) == topNames.end() && llvm::find(topNames, funcName) == topNames.end())
         {
            topNames.push_back(funcSymbol);
         }
      }
   }

   static bool addFunctionParamTrackingAttrs(Module& M,
                                             const std::map<std::string, std::vector<std::string>>& fun2params)
   {
      auto& llvmContext = M.getContext();
      const auto addFunctionStringParamAttr = [](Function& F, unsigned int argIndex, const char* attrName,
                                                 const std::string& attrValue, LLVMContext& context) {
#if PANDA_LLVM_CLANG_MAJOR >= 5
         F.addParamAttr(argIndex, Attribute::get(context, attrName, attrValue));
#else
         auto attrs = F.getAttributes();
         attrs = attrs.addAttribute(context, argIndex + 1, attrName, attrValue);
         F.setAttributes(attrs);
#endif
      };

      bool changed = false;
      for(auto& F : M)
      {
         if(F.isDeclaration())
         {
            continue;
         }
         const auto it = fun2params.find(F.getName().str());
         if(it == fun2params.end())
         {
            continue;
         }
         const auto& trackedParams = it->second;
         unsigned int argIndex = 0;
         for(auto& A : F.args())
         {
            std::string origName;
            if(argIndex < trackedParams.size())
            {
               origName = trackedParams[argIndex];
            }
            if(origName.empty())
            {
               origName = A.getName().str();
            }
            if(origName.empty())
            {
               origName = "P" + std::to_string(argIndex);
            }
            addFunctionStringParamAttr(F, argIndex, BAMBU_ORIG_NAME_ATTR, origName, llvmContext);
            addFunctionStringParamAttr(F, argIndex, BAMBU_ORIG_INDEX_ATTR, std::to_string(argIndex), llvmContext);
            changed = true;
            ++argIndex;
         }
      }
      return changed;
   }

   static bool addFunctionParamTrackingAttrsFromXML(Module& M, const std::string& architectureXML)
   {
      pugi::xml_document doc;
      if(!doc.load_file(architectureXML.c_str()))
      {
         return false;
      }
      const auto fun2params = loadFunctionParamTracking(doc);
      const auto attrsChanged = addFunctionParamTrackingAttrs(M, fun2params);
      return attrsChanged;
   }

#if PANDA_LLVM_CLANG_MAJOR >= 13
   struct BambuParamTrackingPass : public PassInfoMixin<BambuParamTrackingPass>
   {
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager&)
      {
         const auto changed = addFunctionParamTrackingAttrsFromXML(M, outdir_name + "/architecture.xml");
         return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
      }
   };
#else
   struct BambuParamTrackingLegacyPass : public ModulePass
   {
      static char ID;
      BambuParamTrackingLegacyPass() : ModulePass(ID)
      {
      }

      bool runOnModule(Module& M) override
      {
         return addFunctionParamTrackingAttrsFromXML(M, outdir_name + "/architecture.xml");
      }
   };
   char BambuParamTrackingLegacyPass::ID = 0;
#endif

   struct CLANG_VERSION_SYMBOL_DUMP_SSA : public ModulePass
#if PANDA_LLVM_CLANG_MAJOR >= 13
       ,
                                          public PassInfoMixin<CLANG_VERSION_SYMBOL_DUMP_SSA>
#endif
   {
      static char ID;
      bool earlyAnalysis;

      CLANG_VERSION_SYMBOL_DUMP_SSA(bool _earlyAnalysis = false) : ModulePass(ID), earlyAnalysis(_earlyAnalysis)
      {
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLazyValueInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeMemorySSAWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
         initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
#if PANDA_LLVM_CLANG_MAJOR > 5
         initializeOptimizationRemarkEmitterWrapperPassPass(*PassRegistry::getPassRegistry());
#endif
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      CLANG_VERSION_SYMBOL_DUMP_SSA(const CLANG_VERSION_SYMBOL_DUMP_SSA& other)
          : CLANG_VERSION_SYMBOL_DUMP_SSA(other.earlyAnalysis)
      {
      }
#endif

      bool exec(Module& M, llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> GetTLI,
                llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> GetMSSA,
                llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> GetLVI,
                llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE,
#endif
                const std::string& costTable)
      {
         std::vector<std::string> TopFunctionNames;
         for(std::size_t last = 0, it = 0; it < TopFunctionName.size(); last = it + 1)
         {
            it = TopFunctionName.find(",", last);
            TopFunctionNames.push_back(TopFunctionName.substr(last, it));
         }
         if(!TopFunctionNames.empty())
         {
            LLVM_DEBUG(llvm::dbgs() << "Top function names: "
                                    << llvm::join(TopFunctionNames.begin(), TopFunctionNames.end(), ", ") << "\n");
         }

         /// load parameter names
         std::map<std::string, std::vector<std::string>> Fun2Params;
         const auto first_filename = InFile.front();

         pugi::xml_document doc;
         const auto arch_filename = outdir_name + "/architecture.xml";
         if(doc.load_file(arch_filename.c_str()))
         {
            if(pruneArchitectureXMLWithModule(doc, M))
            {
               doc.save_file(arch_filename.c_str());
            }
            extendTopFunctionNamesFromArchitecture(doc, TopFunctionNames);
            Fun2Params = loadFunctionParamTracking(doc);
            for(const auto& funPair : Fun2Params)
            {
               LLVM_DEBUG(dbgs() << "FUNC: " << funPair.first << "("
                                 << llvm::join(funPair.second.begin(), funPair.second.end(), ", ") << ")\n");
            }
         }

         DumpBambuIR RawWriter(outdir_name, first_filename, false, &Fun2Params, earlyAnalysis);
         auto res = RawWriter.exec(M, TopFunctionNames, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                                   GetORE,
#endif
                                   costTable);
         return res;
      }

      bool runOnModule(Module& M) override
      {
#if PANDA_LLVM_CLANG_MAJOR < 13
#if PANDA_LLVM_CLANG_MAJOR >= 10
         auto GetTLI = [&](llvm::Function& F) -> llvm::TargetLibraryInfo& {
            return getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI(F);
         };
#else
         auto GetTLI = [&](llvm::Function&) -> llvm::TargetLibraryInfo& {
            return getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
         };
#endif
         auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
            return getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
         };
         auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
            return getAnalysis<llvm::DominatorTreeWrapperPass>(F).getDomTree();
         };
         auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& {
            return getAnalysis<llvm::LoopInfoWrapperPass>(F).getLoopInfo();
         };
         auto GetMSSA = [&](llvm::Function& F) -> MemorySSAAnalysisResult& {
            return getAnalysis<llvm::MemorySSAWrapperPass>(F);
         };
         auto GetLVI = [&](llvm::Function& F) -> llvm::LazyValueInfo& {
            return getAnalysis<llvm::LazyValueInfoWrapperPass>(F).getLVI();
         };
         auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
            return getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(F);
         };
#if PANDA_LLVM_CLANG_MAJOR > 5
         auto GetORE = [&](llvm::Function& F) -> llvm::OptimizationRemarkEmitter& {
#if PANDA_LLVM_CLANG_MAJOR >= 11
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#else
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#endif
         };
#endif

         return exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                     GetORE,
#endif
                     CostTable);
#else
         report_fatal_error("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING_DUMP_SSA;
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<LoopInfoWrapperPass>();
         AU.addPreserved<MemorySSAWrapperPass>();
         AU.addRequired<MemorySSAWrapperPass>();
         AU.addRequired<LazyValueInfoWrapperPass>();
         AU.addRequired<TargetTransformInfoWrapperPass>();
         AU.addRequired<TargetLibraryInfoWrapperPass>();
         AU.addRequired<AssumptionCacheTracker>();
         AU.addRequired<DominatorTreeWrapperPass>();
#if PANDA_LLVM_CLANG_MAJOR > 5
         AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
#endif
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         MAM.invalidate(M, llvm::PreservedAnalyses::none());
         auto& FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
         auto GetTLI = [&](llvm::Function& F) -> llvm::TargetLibraryInfo& {
            return FAM.getResult<llvm::TargetLibraryAnalysis>(F);
         };
         auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
            return FAM.getResult<llvm::TargetIRAnalysis>(F);
         };
         auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
            return FAM.getResult<llvm::DominatorTreeAnalysis>(F);
         };
         auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& { return FAM.getResult<llvm::LoopAnalysis>(F); };
         auto GetMSSA = [&](llvm::Function& F) -> MemorySSAAnalysisResult& {
            return FAM.getResult<llvm::MemorySSAAnalysis>(F);
         };
         auto GetLVI = [&](llvm::Function& F) -> llvm::LazyValueInfo& {
            return FAM.getResult<llvm::LazyValueAnalysis>(F);
         };
         auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
            return FAM.getResult<llvm::AssumptionAnalysis>(F);
         };
         auto GetORE = [&](llvm::Function& F) -> llvm::OptimizationRemarkEmitter& {
            return FAM.getResult<llvm::OptimizationRemarkEmitterAnalysis>(F);
         };

         const auto changed = exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC, GetORE, CostTable);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };

   char CLANG_VERSION_SYMBOL_DUMP_SSA::ID = 0;

} // namespace llvm

// Currently there is no difference between c++ or c serialization
#if !defined(_WIN32)
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL_DUMP_SSA<true>>
// XPassEarly("dumpBambuIrSSAEarly", "Custom Value Range Based optimization step: LLVM pass",
// false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL_DUMP_SSA> XPass(CLANG_VERSION_STRING_DUMP_SSA,
                                                                     "Dump bambuir starting from LLVM IR: LLVM pass",
                                                                     false /* Only looks at CFG */,
                                                                     false /* Analysis Pass */);
#endif

#if PANDA_LLVM_CLANG_MAJOR >= 13
llvm::PassPluginLibraryInfo getdumpSSAPluginInfo()
{
   return {
       LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING_DUMP_SSA, "v0.12", [](llvm::PassBuilder& PB) {
          const auto loadEarly = [](llvm::ModulePassManager& MPM) { MPM.addPass(llvm::BambuParamTrackingPass()); };
          const auto loadLate = [](llvm::ModulePassManager& MPM, bool doOpt) {
             // MPM.addPass(llvm::PrintModulePass(llvm::errs()));
             MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::LowerAtomicPass()));
             if(doOpt)
             {
                MPM.addPass(llvm::GlobalDCEPass());
                MPM.addPass(llvm::ForceFunctionAttrsPass());
                MPM.addPass(llvm::InferFunctionAttrsPass());
                MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::PostOrderFunctionAttrsPass()));
                MPM.addPass(llvm::ReversePostOrderFunctionAttrsPass());
                MPM.addPass(llvm::GlobalSplitPass());
                MPM.addPass(llvm::GlobalOptPass());
             }
             // MPM.addPass(llvm::PrintModulePass(llvm::errs()));
             MPM.addPass(createModuleToFunctionPassAdaptor(llvm::PromotePass()));
             if(doOpt)
             {
                MPM.addPass(llvm::DeadArgumentEliminationPass());
                llvm::FunctionPassManager PeepholeFPM;
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(PeepholeFPM)));
                MPM.addPass(llvm::CondInstCombIfTaggedPass());
             }
             // MPM.addPass(llvm::PrintModulePass(llvm::errs()));
             MPM.addPass(llvm::GlobalOptPass());
             MPM.addPass(llvm::GlobalDCEPass());
             MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::ArgumentPromotionPass(256)));
             MPM.addPass(llvm::CondInstCombIfTaggedPass());
             if(doOpt)
             {
                llvm::FunctionPassManager FPM1;
                FPM1.addPass(llvm::JumpThreadingPass());
#if PANDA_LLVM_CLANG_MAJOR >= 16
                FPM1.addPass(llvm::SROAPass(llvm::SROAOptions::ModifyCFG));
#endif
                FPM1.addPass(llvm::TailCallElimPass());
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1)));
                MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::PostOrderFunctionAttrsPass()));
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::InvalidateAnalysisPass<llvm::AAManager>()));

                llvm::FunctionPassManager FPM2;
                FPM2.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::LICMPass(
#if PANDA_LLVM_CLANG_MAJOR >= 16
                                                                       llvm::LICMOptions()
#endif
                                                                           ),
                                                                   /*USeMemorySSA=*/true,
                                                                   /*UseBlockFrequencyInfo=*/true));
                // FPM2.addPass(llvm::NewGVNPass());
                FPM2.addPass(llvm::MemCpyOptPass());
                FPM2.addPass(llvm::DSEPass());
                FPM2.addPass(llvm::MergedLoadStoreMotionPass());

                llvm::LoopPassManager LPM2;
                LPM2.addPass(llvm::LoopRotatePass());
#if PANDA_LLVM_CLANG_MAJOR >= 16
                LPM2.addPass(llvm::LoopFlattenPass());
#endif
                LPM2.addPass(llvm::IndVarSimplifyPass());
                LPM2.addPass(llvm::LoopDeletionPass());
                LPM2.addPass(llvm::LoopRotatePass());
                FPM2.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM2), /*UseMemorySSA=*/false,
                                                                   /*UseBlockFrequencyInfo=*/true));
                FPM2.addPass(llvm::LoopFusePass());
                FPM2.addPass(llvm::JumpThreadingPass());
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM2)));
                // MPM.addPass(llvm::PrintModulePass(llvm::errs()));
                MPM.addPass(createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass(llvm::SimplifyCFGOptions()
#if PANDA_LLVM_CLANG_MAJOR >= 16
                                                                                        .convertSwitchRangeToICmp(true)
#endif
                                                                                        .sinkCommonInsts(false)
                                                                                        .hoistCommonInsts(true))));
                MPM.addPass(llvm::GlobalDCEPass());
             }
             llvm::FunctionPassManager FPM3;
             FPM3.addPass(llvm::UnifyFunctionExitNodesPass());
             MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM3)));
             // MPM.addPass(llvm::PointerResolutionPass(llvm::outdir_name));
             // llvm::FunctionPassManager FPM4;
             // FPM4.addPass(llvm::GVNSinkPass());
             // MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM4)));
             // MPM.addPass(createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass(llvm::SimplifyCFGOptions()
             //                                                                         .sinkCommonInsts(true))));
             MPM.addPass(llvm::CLANG_VERSION_SYMBOL_DUMP_SSA());
          };
          PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                 llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
             if(Name == CLANG_VERSION_STRING_DUMP_SSA)
             {
                loadEarly(MPM);
                loadLate(MPM, false);
                return true;
             }
             return false;
          });
          PB.registerPipelineStartEPCallback([&](llvm::ModulePassManager& MPM,
#if PANDA_LLVM_CLANG_MAJOR <= 13
                                                 llvm::PassBuilder::OptimizationLevel Opt
#else
                  llvm::OptimizationLevel Opt
#endif
                                             ) {
             (void)Opt;
             loadEarly(MPM);
          });
          PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if PANDA_LLVM_CLANG_MAJOR <= 13
                                                 llvm::PassBuilder::OptimizationLevel Opt
#else
                 llvm::OptimizationLevel Opt
#endif
                                             ) {
             loadLate(MPM,
#if PANDA_LLVM_CLANG_MAJOR <= 13
                      Opt != llvm::PassBuilder::OptimizationLevel::O0 && Opt != llvm::PassBuilder::OptimizationLevel::O1
#else
                     Opt != llvm::OptimizationLevel::O0 && Opt != llvm::OptimizationLevel::O1
#endif
             );
          });
       }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return getdumpSSAPluginInfo();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadBambuParamTrackingEarly(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::BambuParamTrackingLegacyPass());
}

static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   //   PM.add(llvm::createGVNPass());
   //   PM.add(llvm::createGVNHoistPass());
   //   PM.add(llvm::createMergedLoadStoreMotionPass());
   PM.add(llvm::createLowerAtomicPass());
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(llvm::createGlobalOptimizerPass());
   PM.add(llvm::createGlobalDCEPass());
   PM.add(llvm::createArgumentPromotionPass(256));
   PM.add(new llvm::CondInstCombIfTaggedPass());
   PM.add(llvm::createUnifyFunctionExitNodesPass());

   PM.add(new llvm::CLANG_VERSION_SYMBOL_DUMP_SSA());
}

static llvm::RegisterStandardPasses llvmtoolLoader_Early(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                         loadBambuParamTrackingEarly);
static llvm::RegisterStandardPasses llvmtoolLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif
