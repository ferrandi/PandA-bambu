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
/*
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "ArrPart.hpp"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <cxxabi.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <set>

#include "debug_print.hpp"

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

#if LLVM_VERSION_MAJOR >= 10
#define GET_ARG_PT(fn, i) fn->getArg(i)
#else
#define GET_ARG_PT(fn, i) (&*std::next(fn->arg_begin(), i))
#endif

void printModuleOnFile(Module& M, const std::string& outPath)
{
   std::error_code EC;
   raw_fd_ostream OS(outPath, EC,
#if LLVM_VERSION_MAJOR >= 10
                     sys::fs::OF_None
#else
                     sys::fs::F_None
#endif
   );
   M.print(OS, nullptr);
}

StringRef format_to_string(const PartInfoFormat& f)
{
   switch(f)
   {
      case COMPLETE:
         return "complete";
      case BLOCK:
         return "block";
      case CYCLIC:
         return "cyclic";
      case NONE:
         return "none";
   }
   REPORT_FATAL_ERROR_WITH_REPORT("Invalid Format");
}

std::vector<size_t> getDimsFromArrayType(ArrayType* arrTy)
{
   std::vector<size_t> dims;
   while(arrTy)
   {
      dims.push_back(arrTy->getArrayNumElements());
      arrTy = dyn_cast<ArrayType>(arrTy->getElementType());
   }
   return dims;
}

void initializePartitionScheme(PartitionScheme& scheme, size_t numDims)
{
   scheme.assign(numDims, PartInfo());
}

static void partInfoUnion(PartInfo& a, const PartInfo& b)
{
   if(b.format == PartInfoFormat::NONE)
   {
      return;
   }

   if(a.format == PartInfoFormat::NONE)
   {
      a = b;
   }
   else if(a.format == b.format)
   {
      a.factor = std::max(a.factor, b.factor);
   }
   else
   {
      REPORT_FATAL_ERROR_WITH_REPORT(llvm::Twine("Conflicting partitioning formats on the same memory: (") +
                                     format_to_string(a.format) + ", " + format_to_string(b.format) + ")");
   }
}

void setPartitionInfo(PartitionScheme& scheme, const PartInfo& p, uint64_t dim)
{
   partInfoUnion(scheme[dim], p);
}

[[nodiscard]] uint64_t getNumPartitionsFromScheme(const PartitionScheme& scheme)
{
   uint64_t numPartitions = 1;
   for(const auto& partInfo : scheme)
   {
      if(partInfo.format != PartInfoFormat::NONE)
      {
         numPartitions *= partInfo.factor;
      }
   }
   return numPartitions;
}

[[nodiscard]] std::vector<size_t> getPartitionedDimsFromScheme(const std::vector<size_t>& origDims,
                                                               const PartitionScheme& scheme)
{
   if(origDims.size() != scheme.size())
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Inconsistent dimensions and partition scheme sizes");
   }

   std::vector<size_t> partitionedDims;
   for(size_t i = 0; i < scheme.size(); i++)
   {
      assert(scheme[i].format == NONE || scheme[i].format == CYCLIC || scheme[i].format == BLOCK ||
             scheme[i].format == COMPLETE);
      const auto dimSize = origDims[i];
      if(scheme[i].format != PartInfoFormat::NONE)
      {
         partitionedDims.push_back(dimSize / scheme[i].factor);
      }
      else
      {
         partitionedDims.push_back(dimSize);
      }
   }
   return partitionedDims;
}

/*
 * @brief Get the base type of an array type (i.e., the type of the elements of the innermost dimension,
 * e.g [4 x [3 x i32]] -> i32)
 *
 * @param t: the array type
 */
Type* getArrayBaseType(Type* t)
{
   while(auto* arrTy = dyn_cast<ArrayType>(t))
   {
      t = arrTy->getArrayElementType();
   }
   return t;
}

[[nodiscard]] ArrayType* getPartitionedTypeFromDims(Type* originalType, const std::vector<size_t>& dims)
{
   Type* t = getArrayBaseType(originalType);
   for(size_t i = 0; i < dims.size(); i++)
   {
      t = ArrayType::get(t, dims[dims.size() - 1 - i]);
   }
   return cast<ArrayType>(t);
}

[[nodiscard]] std::string toStringWithPartitionScheme(const Value* value, const PartitionScheme& scheme)
{
   std::string s;
   llvm::raw_string_ostream os(s);
   value->print(os);
   os << " => ";
   for(size_t i = 0; i < scheme.size(); i++)
   {
      os << "[" << format_to_string(scheme[i].format) << ", " << scheme[i].factor << "]";
      if(i < scheme.size() - 1)
      {
         os << "; ";
      }
   }
   return os.str();
}

void PartInfo::validatePartitionFactor(uint64_t numMemoriesArg, uint64_t dimSize)
{
   if(numMemoriesArg == 0 || numMemoriesArg > dimSize)
   {
      REPORT_FATAL_ERROR_WITH_REPORT("The factor of array partition should be greater than 0 and less than or equal to the size "
                                     "of the dimension");
   }
}

PartInfo::PartInfo(size_t formatArg, size_t numMemoriesArg, size_t dimSize)
{
   switch(formatArg)
   {
      case 0:
         format = PartInfoFormat::COMPLETE;
         factor = dimSize;
         break;
      case 1:
         validatePartitionFactor(numMemoriesArg, dimSize);
         format = PartInfoFormat::BLOCK;
         factor = numMemoriesArg;
         break;
      case 2:
         validatePartitionFactor(numMemoriesArg, dimSize);
         format = PartInfoFormat::CYCLIC;
         factor = numMemoriesArg;
         break;
      default:
         REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
   }
}

void FnPartInfo::remap(ValueToValueMapTy& vMap)
{
   for(auto& argPartInfo : args)
   {
      auto* arg = argPartInfo.arg;
      auto* argRemapped = cast<Argument>(vMap.lookup(arg));
      argPartInfo.arg = argRemapped;
   }

   for(auto& allocPartInfo : allocs)
   {
      auto* alloc = allocPartInfo.inst;
      auto* allocRemapped = cast<AllocaInst>(vMap.lookup(alloc));
      allocPartInfo.inst = allocRemapped;
   }

   errBB = cast_or_null<BasicBlock>(vMap.lookup(errBB));
}

FnPartInfo& getFnInfoOrDie(StringMap<FnPartInfo>& table, StringRef fnName)
{
   auto it = table.find(fnName);
   if(it == table.end())
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Function not found " + fnName);
   }
   return it->second;
}

/// Parses a comma separated list of array dimensions.
/// The `array_dims` attribute is absent for every non-array parameter, in which case pugixml
/// hands us an empty string: that is not an error, it just means there is nothing to partition
static std::vector<size_t> getDimsFromString(const std::string& dimsStr)
{
   std::vector<size_t> dims;
   StringRef rest(dimsStr);
   while(!rest.empty())
   {
      const auto parts = rest.split(',');
      rest = parts.second;
      size_t dimSize = 0;
      if(parts.first.trim().getAsInteger(10, dimSize))
      {
         REPORT_FATAL_ERROR_WITH_REPORT("Malformed array_dims entry `" + parts.first + "` in `" +
                                        StringRef(dimsStr) + "`");
      }
      dims.push_back(dimSize);
   }
   return dims;
}

static void partsInfoUnion(std::vector<std::vector<PartInfo>*>& parts)
{
   if(parts.empty())
   {
      return;
   }

   std::vector<PartInfo> tmpParts(*parts[0]);
   for(const std::vector<PartInfo>* partsInfo : parts)
   {
      if(partsInfo->size() != tmpParts.size())
      {
         REPORT_FATAL_ERROR_WITH_REPORT("Array partition propagated between memories of different rank");
      }
      for(size_t i = 0; i < tmpParts.size(); i++)
      {
         partInfoUnion(tmpParts[i], (*partsInfo)[i]);
      }
   }

   for(std::vector<PartInfo>* partsInfo : parts)
   {
      *partsInfo = tmpParts;
   }
}

static bool isWholeArrayPointer(const Value* V)
{
   // Direct allocation base or function argument carrying the full array.
   if(isa<AllocaInst>(V) || isa<Argument>(V) || isa<GlobalVariable>(V))
      return true;

   // A GEP is "whole-array" only if every index is the constant zero.
   // This covers the C array-decay pattern but not element addressing.
   if(const auto* GEP = dyn_cast<GEPOperator>(V))
   {
      return std::all_of(GEP->idx_begin(), GEP->idx_end(), [](const Use& Idx) {
         const auto* CI = dyn_cast<ConstantInt>(Idx.get());
         return CI && CI->isZero();
      });
   }

   // Bitcast / addrspacecast: delegate to the operand.
   if(const auto* Cast = dyn_cast<CastInst>(V))
   {
      if(Cast->getOpcode() == Instruction::BitCast || Cast->getOpcode() == Instruction::AddrSpaceCast)
         return isWholeArrayPointer(Cast->getOperand(0));
   }
   if(const auto* CE = dyn_cast<ConstantExpr>(V))
   {
      if(CE->getOpcode() == Instruction::BitCast)
         return isWholeArrayPointer(CE->getOperand(0));
   }

   return false; // conservative: unknown provenance → not a whole-array ptr
}

/// Walks back through whole-array GEPs and bitcasts to the memory a pointer refers to.
/// Returns nullptr when the pointer addresses only a part of the memory (or its origin is unknown).
static Value* getWholeArrayBase(Value* v)
{
   while(!isa<AllocaInst>(v) && !isa<Argument>(v) && !isa<GlobalVariable>(v))
   {
      if(isa<GEPOperator>(v) && isWholeArrayPointer(v))
      {
         v = cast<GEPOperator>(v)->getPointerOperand();
      }
      else if(isa<BitCastOperator>(v))
      {
         v = cast<BitCastOperator>(v)->getOperand(0);
      }
      else
      {
         return nullptr;
      }
   }
   return v;
}

/// Down: formal arguments of the (tracked) callees receiving `ptr` as a whole array.
static void pushCalleeArgs(Value* ptr, const std::set<Function*>& tracked, std::vector<Value*>& out)
{
   for(Use& use : ptr->uses())
   {
      User* user = use.getUser();
      if(auto* callInst = dyn_cast<CallInst>(user))
      {
         Function* calledFn = callInst->getCalledFunction();
         const auto idxParam = use.getOperandNo();
         if(tracked.count(calledFn) && idxParam < calledFn->arg_size())
         {
            out.push_back(GET_ARG_PT(calledFn, idxParam));
         }
      }
      else if((isa<GEPOperator>(user) && isWholeArrayPointer(user)) || isa<BitCastOperator>(user))
      {
         pushCalleeArgs(user, tracked, out);
      }
   }
}

/// Up: memories the (tracked) callers pass as whole array to the argument `arg`.
static void pushCallerMemories(Argument* arg, const std::set<Function*>& tracked, std::vector<Value*>& out)
{
   Function* fn = arg->getParent();
   for(User* user : fn->users())
   {
      auto* callInst = dyn_cast<CallInst>(user);
      if(!callInst || callInst->getCalledFunction() != fn || !tracked.count(callInst->getFunction()))
      {
         continue;
      }
      if(auto* base = getWholeArrayBase(callInst->getArgOperand(arg->getArgNo())))
      {
         out.push_back(base);
      }
   }
}

/// The architecture.xml entry describing the parameter bound to `arg`
static pugi::xml_node getParamNode(const ArrPartCtx& arrPartCtx, const Argument* arg)
{
   const std::string fnName = arg->getParent()->getName().str();
   auto nodeFn = arrPartCtx.doc->child("module").find_child_by_attribute("function", "symbol", fnName.c_str());
   auto nodeParam = nodeFn.child("parameters")
                        .find_child_by_attribute("parameter", "index", std::to_string(arg->getArgNo()).c_str());
   if(nodeParam.empty())
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Parameter " + Twine(arg->getArgNo()) + " of " + fnName +
                                     " not found in architecture.xml");
   }
   return nodeParam;
}

static PartitionScheme& getOrCreateScheme(ArrPartCtx& arrPartCtx, Value* v)
{
   if(auto* arg = dyn_cast<Argument>(v))
   {
      StringRef fnName = arg->getParent()->getName();
      auto& args = arrPartCtx.fnTable.try_emplace(fnName, arg->getParent()).first->second.args;
      auto argPartInfoIt = findPartInfoInContainer(arg, args);
      if(argPartInfoIt != args.end())
      {
         return argPartInfoIt->scheme;
      }

      auto nodeParam = getParamNode(arrPartCtx, arg);
      std::string argName = nodeParam.attribute("bundle").as_string();
      std::vector<size_t> dims = getDimsFromString(nodeParam.attribute("array_dims").as_string());
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the arg `" << argName << "` of " << fnName << "\n");
      args.push_back(ArgPartInfo(argName, arg, dims));
      return args.back().scheme;
   }

   if(auto* alloc = dyn_cast<AllocaInst>(v))
   {
      auto& allocs = arrPartCtx.fnTable.try_emplace(alloc->getFunction()->getName(), alloc->getFunction())
                         .first->second.allocs;
      auto allocPartInfoIt = llvm::find_if(allocs, [&](const AllocaPartInfo& a) { return a.inst == alloc; });
      if(allocPartInfoIt != allocs.end())
      {
         return allocPartInfoIt->scheme;
      }
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the alloc "; alloc->print(dbgs()); dbgs() << "\n");
      allocs.push_back(AllocaPartInfo(alloc));
      return allocs.back().scheme;
   }

   auto* globVar = cast<GlobalVariable>(v);
   auto& globalVars = arrPartCtx.globalVars;
   auto globVarPartInfoIt = llvm::find_if(globalVars, [&](const GlobalPartInfo& g) { return g.var == globVar; });
   if(globVarPartInfoIt != globalVars.end())
   {
      return globVarPartInfoIt->scheme;
   }
   LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the global variable " << globVar->getName() << "\n");
   globalVars.push_back(GlobalPartInfo(globVar));
   return globalVars.back().scheme;
}

/**
 * Every memory aliased through a chain of calls (towards callers and callees, at any depth) must share
 * the same partition scheme: the connected components of the alias graph are computed starting from
 * the memories with a partition request, and the schemes of each component are merged.
 */
void diffuseArrPartConfigs(ArrPartCtx& arrPartCtx, const std::vector<std::string>& workQueue)
{
   Module& M = *arrPartCtx.topFn->getParent();
   std::set<Function*> tracked;
   for(const auto& fnName : workQueue)
   {
      // Declarations have no body (nor architecture.xml entry): nothing to propagate through them
      Function* fn = M.getFunction(fnName);
      if(fn && !fn->isDeclaration())
      {
         tracked.insert(fn);
      }
   }

   std::vector<Value*> seeds;
   for(const auto& fnName : workQueue)
   {
      auto it = arrPartCtx.fnTable.find(fnName);
      if(it == arrPartCtx.fnTable.end())
      {
         continue;
      }
      for(const auto& argPartInfo : it->second.args)
      {
         seeds.push_back(argPartInfo.arg);
      }
      for(const auto& allocPartInfo : it->second.allocs)
      {
         seeds.push_back(allocPartInfo.inst);
      }
   }
   for(const auto& globalPartInfo : arrPartCtx.globalVars)
   {
      seeds.push_back(globalPartInfo.var);
   }

   std::set<Value*> visited;
   for(Value* seed : seeds)
   {
      if(!visited.insert(seed).second)
      {
         continue;
      }

      std::vector<Value*> component;
      std::vector<Value*> worklist{seed};
      while(!worklist.empty())
      {
         Value* node = worklist.back();
         worklist.pop_back();
         component.push_back(node);

         std::vector<Value*> neighbors;
         pushCalleeArgs(node, tracked, neighbors);
         if(auto* arg = dyn_cast<Argument>(node))
         {
            pushCallerMemories(arg, tracked, neighbors);
         }
         for(Value* neighbor : neighbors)
         {
            if(visited.insert(neighbor).second)
            {
               worklist.push_back(neighbor);
            }
         }
      }

      LLVM_DEBUG(dbgs() << "\n[ARR_PART] Diffusing over " << component.size() << " memories from ";
                 seed->print(dbgs()); dbgs() << "\n");
      // Create all the entries first: inserting would invalidate the pointers to the schemes
      for(Value* v : component)
      {
         getOrCreateScheme(arrPartCtx, v);
      }
      std::vector<std::vector<PartInfo>*> parts;
      for(Value* v : component)
      {
         parts.push_back(&getOrCreateScheme(arrPartCtx, v));
      }
      partsInfoUnion(parts);
   }
}

/// Applies a partition request to the dimension `dim` of `mem` (1-indexed), or to all its dimensions when `dim` is 0
static void setPartitionByDim(ArrPartCtx& arrPartCtx, Value* mem, const std::vector<size_t>& completeDims,
                              uint64_t format, uint64_t numMemories, uint64_t dim)
{
   if(dim > completeDims.size())
   {
      REPORT_FATAL_ERROR_WITH_REPORT("The dim attribute should be between 0 and the dimensions of the array");
   }
   const uint64_t firstDim = dim == 0 ? 0 : dim - 1;
   const uint64_t endDim = dim == 0 ? completeDims.size() : dim;
   if(firstDim == endDim)
   {
      return;
   }

   auto& scheme = getOrCreateScheme(arrPartCtx, mem);
   for(uint64_t idxDim = firstDim; idxDim < endDim; idxDim++)
   {
      auto conf = PartInfo(format, numMemories, completeDims[idxDim]);
      LLVM_DEBUG(llvm::dbgs() << "[ARR_PART] Part conf: " << conf.to_string() << "\n");
      setPartitionInfo(scheme, conf, idxDim);
   }
}

static bool isArgPartitionable(const Argument* arg)
{
   return arg->getType()->isPointerTy();
}

static void setArrPartArg(ArrPartCtx& arrPartCtx, Argument* arg, uint64_t format, uint64_t numMemories, uint64_t dim)
{
   if(!isArgPartitionable(arg))
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a function argument that is not an array");
   }
   const auto completeDims = getDimsFromString(getParamNode(arrPartCtx, arg).attribute("array_dims").as_string());
   setPartitionByDim(arrPartCtx, arg, completeDims, format, numMemories, dim);
}

static void setArrPartAlloca(ArrPartCtx& arrPartCtx, AllocaInst* allocaInst, uint64_t format, uint64_t numMemories,
                             uint64_t dim)
{
   auto* completeTy = dyn_cast<ArrayType>(allocaInst->getAllocatedType());
   if(completeTy == nullptr)
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a local value that is not an array");
   }
   setPartitionByDim(arrPartCtx, allocaInst, getDimsFromArrayType(completeTy), format, numMemories, dim);
}

static void setArrPartGlobalVar(ArrPartCtx& arrPartCtx, GlobalVariable* globalVar, uint64_t format,
                                uint64_t numMemories, uint64_t dim)
{
   auto* completeTy = dyn_cast<ArrayType>(globalVar->getValueType());
   if(!globalVar->getType()->isPointerTy() || completeTy == nullptr)
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a global variable that is not an array");
   }
   setPartitionByDim(arrPartCtx, globalVar, getDimsFromArrayType(completeTy), format, numMemories, dim);
}

/**
 * Functions to check the assumptions of the pass
 */

bool isArrPartFunctionPresent(Module& M)
{
   Function* builtinArrPartFun = M.getFunction(BAMBU_CSROA_PARTITION_FUN_NAME);
   if(!builtinArrPartFun)
   {
      LLVM_DEBUG(dbgs() << "[ARR_PART] No elements to partition\n");
      return false;
   }
   return true;
}

void populateArrPartCtx(Module& M, ArrPartCtx& arrPartCtx)
{
   LLVM_DEBUG(dbgs() << "[ARR_PART] Builtin Array Partitioning Function Use Analysis\n");
   Function* builtinArrPartFun = M.getFunction(BAMBU_CSROA_PARTITION_FUN_NAME);
   for(User* builtinUser : builtinArrPartFun->users())
   {
      auto* callInst = cast<CallInst>(builtinUser);
      auto fnName = callInst->getFunction()->getName();
      LLVM_DEBUG(dbgs() << "\n[ARR_PART] Builtin Call "; callInst->print(dbgs()); dbgs() << " in " << fnName << "\n");

      auto* memToPartition = callInst->getOperand(0)->stripPointerCasts();
      uint64_t formatArg = cast<ConstantInt>(callInst->getOperand(1))->getZExtValue();

      // This is the real number of memories only for cyclic and block array partition type
      uint64_t numMemoriesArg = cast<ConstantInt>(callInst->getOperand(2))->getZExtValue();

      // Remember that if dimArg = 0 => partition all the dimensions with that specific type
      uint64_t dimArg = cast<ConstantInt>(callInst->getOperand(3))->getZExtValue();

      if(auto* arg = dyn_cast<Argument>(memToPartition))
      {
         setArrPartArg(arrPartCtx, arg, formatArg, numMemoriesArg, dimArg);
      }
      else if(auto* alloc = dyn_cast<AllocaInst>(memToPartition))
      {
         setArrPartAlloca(arrPartCtx, alloc, formatArg, numMemoriesArg, dimArg);
      }
      else if(auto* global = dyn_cast<GlobalVariable>(memToPartition))
      {
         setArrPartGlobalVar(arrPartCtx, global, formatArg, numMemoriesArg, dimArg);
      }
      else
      {
         REPORT_FATAL_ERROR_WITH_REPORT(
             "The array partition variable should be an Argument or a local array or a global array");
      }
   }
}

void describeArrPartRequests(const ArrPartCtx& ctx)
{
   if(!ctx.globalVars.empty())
   {
      dbgs() << "Global Vars:\n";
   }

   for(const auto& globalVar : ctx.globalVars)
   {
      dbgs() << "  " << globalVar.to_string() << "\n";
   }

   for(auto it = ctx.fnTable.begin(), end = ctx.fnTable.end(); it != end; it++)
   {
      StringRef fnName = it->first();
      const auto& fnPartInfo = it->second;
      dbgs() << "Function " << fnName << "\n";
      if(!fnPartInfo.args.empty())
      {
         dbgs() << "  Arguments:\n";
      }

      for(const auto& arg : fnPartInfo.args)
      {
         dbgs() << "    " << arg.argName << ": " << arg.to_string() << "\n";
      }

      if(!fnPartInfo.allocs.empty())
      {
         dbgs() << "  Allocs:\n";
      }

      for(const auto& alloc : fnPartInfo.allocs)
      {
         dbgs() << "  " << alloc.to_string() << "\n";
      }
      dbgs() << "\n";
   }
}

static void inverseTopologicalSort(Function* fn, std::vector<std::string>& workQueue, std::set<Function*>& visited)
{
   // Marked before visiting the callees, otherwise a recursive call never terminates
   visited.insert(fn);
   for(auto& bb : *fn)
   {
      for(auto& inst : bb)
      {
         if(auto* callInst = dyn_cast<CallInst>(&inst))
         {
            Function* calledFn = callInst->getCalledFunction();
            if(!calledFn || calledFn->isIntrinsic() || calledFn->getName() == BAMBU_CSROA_PARTITION_FUN_NAME ||
               visited.count(calledFn))
            {
               continue;
            }

            inverseTopologicalSort(calledFn, workQueue, visited);
         }
      }
   }

   workQueue.push_back(fn->getName().str());
}

void inverseTopologicalSort(Function* fn, std::vector<std::string>& workQueue)
{
   std::set<Function*> visited;
   inverseTopologicalSort(fn, workQueue, visited);
}

// ==================================================================================
// architecture.xml rewriting
//
// A partitioned parameter becomes one bundle and one parameter entry per bank, so the
// interface bambu builds from architecture.xml matches the signature CSROA produced.
// ==================================================================================
namespace
{
   /// The baseType is expected to be in the form of type*, like float* or int*
   std::string getOriginalType(const std::string& baseType, const ArgPartInfo& argPartInfo)
   {
      // In this way, it builds something like float (*) or int (*)
      std::string originalTy = baseType.substr(0, baseType.find('*')) + " (*)";
      const auto& originalTypeDims = argPartInfo.getOrigTypeDims();

      for(size_t i = 1; i < originalTypeDims.size(); i++)
      {
         const auto& partInfo = argPartInfo.scheme[i];
         switch(partInfo.format)
         {
            case COMPLETE:
               break;
            case BLOCK:
            case CYCLIC:
               originalTy += "[" + std::to_string(originalTypeDims[i] / partInfo.factor) + "]";
               break;
            case PartInfoFormat::NONE:
               originalTy += "[" + std::to_string(originalTypeDims[i]) + "]";
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
         }
      }

      return originalTy;
   }

   void createBundleEntryPartition(pugi::xml_node& bundlesNode, const pugi::xml_node& origBundleNode,
                                   const std::string& name, const std::string& includes)
   {
      auto newBundle = bundlesNode.append_child("bundle");

      std::string oldMode = origBundleNode.attribute("mode").as_string();
      newBundle.append_attribute("mode").set_value(
          includes.find("hls_stream.h") != std::string::npos ? "fifo" : oldMode.c_str());
      newBundle.append_attribute("name").set_value(name.c_str());
   }

   bool isCompletelyPartitioned(const ArgPartInfo& argPartInfo)
   {
      return !argPartInfo.scheme.empty() && llvm::all_of(argPartInfo.scheme, [](const PartInfo& partInfo) {
         return partInfo.format == PartInfoFormat::COMPLETE;
      });
   }

   bool isScalarPartitionInterfaceMode(const std::string& mode)
   {
      return mode == "none" || mode == "ptrdefault" || mode == "handshake" || mode == "valid" || mode == "ovalid" ||
             mode == "acknowledge";
   }

   void createParameterEntryPartition(pugi::xml_node& paramsNode, const std::string& name, uint64_t elemCount,
                                      const std::string& includes, uint64_t index,
                                      const std::string& original_type, uint64_t sizeInBytes,
                                      const std::string& type)
   {
      auto newParamNode = paramsNode.append_child("parameter");

      newParamNode.append_attribute("bundle").set_value(name.c_str());
      newParamNode.append_attribute("elem_count").set_value(elemCount);
      newParamNode.append_attribute("includes").set_value(includes.c_str());
      newParamNode.append_attribute("index").set_value(index);
      newParamNode.append_attribute("original_typename").set_value(original_type.c_str());
      newParamNode.append_attribute("port").set_value(name.c_str());
      newParamNode.append_attribute("size_in_bytes").set_value(sizeInBytes);
      newParamNode.append_attribute("typename").set_value(type.c_str());
   }

   void createFnPartNode(const pugi::xml_node& origFnNode, pugi::xml_node& fnXml, StringRef topFnName,
                         const std::vector<ArgPartInfo>& objs)
   {
      fnXml.append_attribute("csroa").set_value(true);
      fnXml.append_attribute("name").set_value(topFnName.str().c_str());
      fnXml.append_attribute("symbol").set_value(topFnName.str().c_str());
      for(const auto& attr : origFnNode.attributes())
      {
         const auto attrName = attr.name();
         if(strcmp(attrName, "name") == 0 || strcmp(attrName, "symbol") == 0 || strcmp(attrName, "original") == 0)
         {
            continue;
         }
         fnXml.append_attribute(attrName).set_value(attr.value());
      }

      auto origBundles = origFnNode.child("bundles");
      auto origParms = origFnNode.child("parameters");

      auto bundles = fnXml.append_child("bundles");
      auto parms = fnXml.append_child("parameters");

      uint64_t idxParmCsroa = 0;
      for(uint64_t idxOrig = 0;; idxOrig++)
      {
         auto parmNodeCsroa = origParms.find_child_by_attribute("parameter", "index", std::to_string(idxOrig).c_str());
         auto bundleNodeCsroa =
             origBundles.find_child_by_attribute("bundle", "name", parmNodeCsroa.attribute("bundle").as_string());
         if(!parmNodeCsroa || !bundleNodeCsroa)
         {
            break;
         }

         std::string bundle = parmNodeCsroa.attribute("bundle").as_string();
         uint64_t storedElemCount = parmNodeCsroa.attribute("elem_count").as_ullong();
         std::string includes = parmNodeCsroa.attribute("includes").as_string();
         std::string storedOriginalType = parmNodeCsroa.attribute("original_typename").as_string();
         uint64_t storedSizeInBytes = parmNodeCsroa.attribute("size_in_bytes").as_ullong();
         std::string type = parmNodeCsroa.attribute("typename").as_string();

         auto obj = llvm::find_if(objs, [&](const ArgPartInfo& a) { return a.argName == bundle; });
         if(obj != objs.end())
         {
            const std::string bundleMode = bundleNodeCsroa.attribute("mode").as_string();
            if(isScalarPartitionInterfaceMode(bundleMode) && !isCompletelyPartitioned(*obj))
            {
               REPORT_FATAL_ERROR_WITH_REPORT(llvm::Twine("Interface mode '") + bundleMode +
                                              "' on array-partitioned parameter '" + bundle +
                                              "' is supported only with complete partitioning of all array dimensions");
            }
            if(storedElemCount == 0)
            {
               REPORT_FATAL_ERROR_WITH_REPORT(llvm::Twine("architecture.xml parameter '") + bundle +
                                              "' of an array-partitioned function has no usable elem_count attribute");
            }
            uint64_t elemCount = storedElemCount / obj->getNumPartitions();
            std::string originalType = getOriginalType(type, *obj);

            for(uint64_t numPartition = 0; numPartition < obj->getNumPartitions(); numPartition++)
            {
               std::string namePartition = bundle + "_" + std::to_string(numPartition);
               createBundleEntryPartition(bundles, bundleNodeCsroa, namePartition, includes);
               createParameterEntryPartition(parms, namePartition, elemCount, includes, idxParmCsroa, originalType,
                                             storedSizeInBytes / storedElemCount * elemCount, type);
               idxParmCsroa += 1;
            }
         }
         else
         {
            auto copy = parms.append_copy(parmNodeCsroa);
            copy.attribute("index").set_value(idxParmCsroa);
            copy.remove_attribute("array_dims");
            bundles.append_copy(bundleNodeCsroa);
            idxParmCsroa += 1;
         }
      }
   }
} // namespace

void modifyXMLModule(ArrPartCtx& arrPartCtx, const std::string& architectureFile)
{
   LLVM_DEBUG(llvm::dbgs() << "[CSROA] Modify architecture.xml file\n");
   auto& doc = *arrPartCtx.doc;
   auto xmlModule = doc.child("module");
   std::vector<pugi::xml_node> functions(xmlModule.begin(), xmlModule.end());

   for(auto& f : functions)
   {
      const std::string symbol = f.attribute("symbol").as_string();
      auto fnInfoIt = arrPartCtx.fnTable.find(symbol);
      if(fnInfoIt == arrPartCtx.fnTable.end() || fnInfoIt->second.args.empty())
      {
         continue;
      }

      auto& fnInfo = fnInfoIt->second;
      pugi::xml_node& origFnNode = f;
      if(symbol == arrPartCtx.topFn->getName())
      {
         LLVM_DEBUG(llvm::dbgs() << "[CSROA] Update top function " << symbol << "\n");
         std::string nameFnOriginal = "original_" + std::string(symbol);
         origFnNode.attribute("name").set_value(nameFnOriginal.c_str());
         origFnNode.attribute("symbol").set_value(nameFnOriginal.c_str());
         origFnNode.append_attribute("original").set_value(true);
      }

      auto fnPartNode = xmlModule.append_child("function");
      createFnPartNode(origFnNode, fnPartNode, symbol, fnInfo.args);

      for(auto& parm : origFnNode.child("parameters").children("parameter"))
      {
         std::string parmName = parm.attribute("bundle").as_string();

         auto argPartInfoIt =
             llvm::find_if(fnInfo.args, [&](const ArgPartInfo& argPartInfo) { return argPartInfo.argName == parmName; });
         if(argPartInfoIt == fnInfo.args.end())
         {
            continue;
         }

         std::string arrayPartitionTypesValue;
         std::string arrayPartitionFactorsValue;
         for(size_t i = 0; i < argPartInfoIt->scheme.size(); i++)
         {
            if(i > 0)
            {
               arrayPartitionTypesValue += ",";
               arrayPartitionFactorsValue += ",";
            }
            const auto& partInfo = argPartInfoIt->scheme[i];
            arrayPartitionTypesValue += format_to_string(partInfo.format).str();
            arrayPartitionFactorsValue += std::to_string(partInfo.factor);
         }
         parm.append_attribute("array_partition_types").set_value(arrayPartitionTypesValue.c_str());
         parm.append_attribute("array_partition_factors").set_value(arrayPartitionFactorsValue.c_str());
      }
   }

   LLVM_DEBUG(dbgs() << "[CSROA] Saving the new architecture.xml\n");
   doc.save_file(architectureFile.c_str(), "  ", pugi::format_indent | pugi::format_no_empty_element_tags);
}

bool loadXMLModule(pugi::xml_document& doc, const std::string& architectureFile)
{
   if(!doc.load_file(architectureFile.c_str()))
   {
      errs() << "[ARR_PART] architecture.xml cannot be loaded\n";
      return false;
   }
   assert(!doc.empty() && "The architecture.xml file should have at least 1 module");
   assert(doc.next_sibling().empty() &&
          "There should not be another module (FIXME: in future there could be multiple modules?)");
   return true;
}

std::string getDemangled(const std::string& declname)
{
   int status;
   char* demangledOutbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
   if(status == 0)
   {
      std::string res = declname;
      if(std::string(demangledOutbuffer).find_last_of('(') != std::string::npos)
      {
         res = demangledOutbuffer;
         auto parPos = res.find('(');
         assert(parPos != std::string::npos);
         res = res.substr(0, parPos);
      }
      free(demangledOutbuffer);
      return res;
   }

   assert(demangledOutbuffer == nullptr);

   return declname;
}

/**
 * Searches the top function in the Module and set it to the arrPartCtx.topFn.
 * It returns true if it is found, false otherwise.
 */
Function* findTopFunction(Module& M, const std::string& topFunctionNameArgPass)
{
   auto pred = [&](const Function& f) { return getDemangled(f.getName().str()) == topFunctionNameArgPass; };
   size_t numTopFn = std::count_if(M.functions().begin(), M.functions().end(), pred);

   if(numTopFn == 0)
   {
      errs() << "[ARR_PART] The top function specified `" << topFunctionNameArgPass
             << "` does not correspond to any function in the program\n";
      return nullptr;
   }

   if(numTopFn != 1)
   {
      errs() << "[ARR_PART] The top function specified `" << topFunctionNameArgPass
             << "` correspond to 2 or more functions in the program\n";
      return nullptr;
   }

   return &(*std::find_if(M.functions().begin(), M.functions().end(), pred));
}
