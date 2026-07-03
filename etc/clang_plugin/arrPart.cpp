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
 *                Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "arrPart.hpp"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <cxxabi.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

#include <set>

#include "debug_print.hpp"

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

#define REPORT_WITH_PRINT(v, msg)          \
   do                                      \
   {                                       \
      llvm::errs() << "[UNEXPECTED] ";     \
      v->print(llvm::errs());              \
      llvm::errs() << "\n";                \
      REPORT_FATAL_ERROR_WITH_REPORT(msg); \
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
   scheme.clear();
   for(size_t i = 0; i < numDims; i++)
   {
      scheme.push_back(PartInfo(i));
   }
}

void setPartitionInfo(PartitionScheme& scheme, const PartInfo& p, uint64_t dim)
{
   auto& partInfo = scheme[dim];
   if(partInfo.format == NONE)
   {
      partInfo = p;
      return;
   }

   if(partInfo.format != p.format)
   {
      REPORT_FATAL_ERROR_WITH_REPORT(llvm::Twine("Conflicting partitioning formats on the same memory: (") +
                                     format_to_string(partInfo.format) + ", " + format_to_string(p.format) + ")");
   }
   partInfo.factor = std::max(partInfo.factor, p.factor);
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
      REPORT_FATAL_ERROR_WITH_REPORT("The factor of array partition should be greater than 0 or less equal to the size "
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

PartInfo createPartInfo(const std::vector<size_t>& completeDims, uint64_t format, uint64_t numMemories, uint64_t idxDim,
                        uint64_t numDims)
{
   if(idxDim >= numDims)
   {
      REPORT_FATAL_ERROR_WITH_REPORT("The dim attribute should be between 0 and the dimensions of the array");
   }

   uint64_t dimSize = completeDims[idxDim];
   auto conf = PartInfo(format, numMemories, dimSize);
   LLVM_DEBUG(llvm::dbgs() << "[ARR_PART] Part conf: " << conf.to_string() << "\n");
   return conf;
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

std::vector<size_t> getDimsFromString(const std::string& dimsStr)
{
   std::vector<size_t> dims;
   size_t start = 0;
   size_t end;
   do
   {
      end = dimsStr.find(',', start);
      std::string dimSizeStr = dimsStr.substr(start, end - start);
      size_t dimSize = std::stoull(dimSizeStr);
      dims.push_back(dimSize);
      start = end + 1;
   } while(end != std::string::npos);
   return dims;
}

bool hasConstantIndicesN(const std::vector<Value*>& indices, size_t n)
{
   if(indices.size() < n)
   {
      return false;
   }

   for(size_t i = 0; i < n; i++)
   {
      if(!isa<ConstantInt>(indices[i]))
      {
         return false;
      }
   }
   return true;
}

// Needed to do the recursive call
void collectArrayPartitionFromArg(ArrPartCtx& arrPartCtx, Argument* arg, std::vector<std::vector<PartInfo>*>& parts,
                                  std::set<Value*>& processedValues);
void collectArrayPartitionFromAlloca(ArrPartCtx& arrPartCtx, AllocaInst* alloc,
                                     std::vector<std::vector<PartInfo>*>& parts, std::set<Value*>& processedValues);

void collectArrayPartitionFromGlobalVar(ArrPartCtx& arrPartCtx, GlobalVariable* globVar,
                                        std::vector<std::vector<PartInfo>*>& parts, std::set<Value*>& processedValues);

void collectArrayPartitionFromCallInstr(ArrPartCtx& arrPartCtx, Value* val, std::vector<std::vector<PartInfo>*>& parts,
                                        std::set<Value*>& processedValues)
{
   for(const auto& use : val->uses())
   {
      const auto* callInst = dyn_cast<CallInst>(use.getUser());
      if(!callInst || callInst->getCalledFunction() == nullptr || callInst->getCalledFunction()->isIntrinsic() ||
         callInst->getCalledFunction()->getName() == "__bambu_csroa_partition__")
      {
         continue;
      }

      Function* calledFn = callInst->getCalledFunction();
      assert(calledFn != nullptr && "Only direct function calls are supported");
      const auto* it = std::find(callInst->arg_begin(), callInst->arg_end(), use);
      assert(it != callInst->arg_end() && "Use not found");
      unsigned idxParam = std::distance(callInst->arg_begin(), it);
      collectArrayPartitionFromArg(arrPartCtx, GET_ARG_PT(calledFn, idxParam), parts, processedValues);
   }
}

void collectArrayPartitionFromCalledFunctions(ArrPartCtx& arrPartCtx, Argument* arg,
                                              std::vector<std::vector<PartInfo>*>& parts,
                                              std::set<Value*>& processedValues)
{
   for(auto* user : arg->getParent()->users())
   {
      const auto* callInst = dyn_cast<CallInst>(user);
      if(!callInst || callInst->getCalledFunction() == nullptr || callInst->getCalledFunction()->isIntrinsic())
      {
         continue;
      }

      uint64_t paramIdx = arg->getArgNo();
      Value* paramValue = callInst->getArgOperand(paramIdx);
      while(!isa<AllocaInst>(paramValue) && !isa<Argument>(paramValue) && !isa<GlobalVariable>(paramValue))
      {
         if(auto* gepInst = dyn_cast<GEPOperator>(paramValue))
         {
            paramValue = gepInst->getPointerOperand();
         }
         else if(auto* bitCastInst = dyn_cast<BitCastOperator>(paramValue))
         {
            paramValue = bitCastInst->stripPointerCasts();
         }
         else
         {
            REPORT_WITH_PRINT(paramValue, "It should be a GetElementPtrInst");
         }
      }

      if(auto* paramAllocaInst = dyn_cast<AllocaInst>(paramValue))
      {
         collectArrayPartitionFromAlloca(arrPartCtx, paramAllocaInst, parts, processedValues);
      }
      else if(auto* paramArg = dyn_cast<Argument>(paramValue))
      {
         collectArrayPartitionFromArg(arrPartCtx, paramArg, parts, processedValues);
      }
      else if(auto* paramGlobVar = dyn_cast<GlobalVariable>(paramValue))
      {
         collectArrayPartitionFromGlobalVar(arrPartCtx, paramGlobVar, parts, processedValues);
      }
      else
      {
         REPORT_FATAL_ERROR_WITH_REPORT("It should be an Argument or an AllocaInst");
      }
   }
}

void partInfoUnion(PartInfo& a, const PartInfo& b)
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
      report_fatal_error(llvm::Twine("Conflicting partitioning formats on the same memory: (") +
                         format_to_string(a.format) + ", " + format_to_string(b.format) + ")");
   }
}

void partsInfoUnion(std::vector<std::vector<PartInfo>*>& parts)
{
   if(parts.empty())
   {
      return;
   }

   std::vector<PartInfo> tmpParts(*parts[0]);
   for(const std::vector<PartInfo>* partsInfo : parts)
   {
      assert(partsInfo->size() == tmpParts.size());
      for(size_t i = 0; i < tmpParts.size(); i++)
      {
         partInfoUnion(tmpParts[i], (*partsInfo)[i]);
      }
   }

   for(std::vector<PartInfo>* partsInfo : parts)
   {
      assert(partsInfo->size() == tmpParts.size());
      for(size_t i = 0; i < tmpParts.size(); i++)
      {
         (*partsInfo)[i] = tmpParts[i];
      }
   }
}

void collectArrayPartitionFromArg(ArrPartCtx& arrPartCtx, Argument* arg, std::vector<std::vector<PartInfo>*>& parts,
                                  std::set<Value*>& processedValues)
{
   LLVM_DEBUG(dbgs() << "[ARR PART] Processing the arg "; arg->print(dbgs());
              dbgs() << " in the function " << arg->getParent()->getName() << "\n";);
   if(processedValues.count(arg) || arg->getParent()->isDeclaration())
   {
      LLVM_DEBUG(dbgs() << "[ARR PART] arg already processed or the arg's function is only a declaration\n");
      return;
   }

   StringRef fnName = arg->getParent()->getName();
   auto& args = arrPartCtx.fnTable.try_emplace(fnName, arg->getParent()).first->second.args;
   auto argPartInfoIt = findPartInfoInContainer(arg, args);

   if(argPartInfoIt != args.end())
   {
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the arg `" << argPartInfoIt->argName
                        << "` (idx = " << argPartInfoIt->arg->getArgNo() << ")\n");
      parts.push_back(&argPartInfoIt->scheme);
   }
   else
   {
      auto nodeFn = arrPartCtx.doc->child("module").find_child_by_attribute("function", "symbol", fnName.data());
      auto nodeParam = nodeFn.child("parameters")
                           .find_child_by_attribute("parameter", "index", std::to_string(arg->getArgNo()).c_str());
      assert(!nodeParam.empty());
      std::string argName = nodeParam.attribute("bundle").as_string();
      std::vector<size_t> dims = getDimsFromString(nodeParam.attribute("array_dims").as_string());

      auto argPartInfo = ArgPartInfo(argName, arg, dims);
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the arg `" << argPartInfo.argName << "`\n");
      args.push_back(argPartInfo);
      parts.push_back(&args.back().scheme);
   }

   processedValues.insert(arg);
   collectArrayPartitionFromCallInstr(arrPartCtx, arg, parts, processedValues);
   collectArrayPartitionFromCalledFunctions(arrPartCtx, arg, parts, processedValues);
}

void collectArrayPartitionFromAlloca(ArrPartCtx& arrPartCtx, AllocaInst* alloc,
                                     std::vector<std::vector<PartInfo>*>& parts, std::set<Value*>& processedValues)
{
   if(processedValues.count(alloc))
   {
      return;
   }

   StringRef fnName = alloc->getFunction()->getName();
   auto& allocs = arrPartCtx.fnTable.try_emplace(fnName, alloc->getFunction()).first->second.allocs;
   auto allocPartInfoIt = llvm::find_if(allocs, [&](const AllocaPartInfo& a) { return a.inst == alloc; });

   if(allocPartInfoIt != allocs.end())
   {
      parts.push_back(&allocPartInfoIt->scheme);
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the alloc "; allocPartInfoIt->inst->print(dbgs()); dbgs() << "\n");
   }
   else
   {
      auto allocPartInfo = AllocaPartInfo(alloc);
      allocs.push_back(allocPartInfo);
      parts.push_back(&allocs.back().scheme);
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the alloc "; allocPartInfo.inst->print(dbgs()); dbgs() << "\n");
   }
   processedValues.insert(alloc);

   // This is needed since when the alloca is used in a function call, it is not passed directly, but first there
   // is a GetElementPtr instruction to get the address of the first element and then the value retrieved is
   // passed to the function
   std::vector<Value*> gepAfterAlloc;
   for(User* user : alloc->users())
   {
      auto* gepInst = dyn_cast<GetElementPtrInst>(user);
      if(!gepInst)
      {
         continue;
      }

      std::vector<Value*> gepIndices(gepInst->idx_begin(), gepInst->idx_end());
      // TODO: We should also check that the gep is taking the pointer of the first element
      // FIXME: allocPartInfoIt could be null! In the if else before if the flow goes into the else branch
      // the allocPartInfoIt is not set!
      // if(!hasConstantIndicesN(gepIndices, allocPartInfoIt->scheme.size()))
      // {
      //    continue;
      // }

      gepAfterAlloc.push_back(gepInst);
   }

   // To be sure, we search call instructions from the alloca and the GetElementPtr instructions
   collectArrayPartitionFromCallInstr(arrPartCtx, alloc, parts, processedValues);
   for(auto* v : gepAfterAlloc)
   {
      collectArrayPartitionFromCallInstr(arrPartCtx, v, parts, processedValues);
   }
}

void collectArrayPartitionFromGlobalVar(ArrPartCtx& arrPartCtx, GlobalVariable* globVar,
                                        std::vector<std::vector<PartInfo>*>& parts, std::set<Value*>& processedValues)
{
   if(processedValues.count(globVar))
   {
      return;
   }

   auto& globalVars = arrPartCtx.globalVars;
   auto globVarPartInfoIt = llvm::find_if(globalVars, [&](const GlobalPartInfo& g) { return g.var == globVar; });

   if(globVarPartInfoIt != globalVars.end())
   {
      parts.push_back(&globVarPartInfoIt->scheme);
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the global variable "; dbgs() << globVarPartInfoIt->var->getName();
                 dbgs() << " "; globVarPartInfoIt->var->getType()->print(dbgs()); dbgs() << "\n");
   }
   else
   {
      auto globVarPartInfo = GlobalPartInfo(globVar);
      globalVars.push_back(globVarPartInfo);
      parts.push_back(&globalVars.back().scheme);
      LLVM_DEBUG(dbgs() << "[ARR_PART] Inserting the global variable "; dbgs() << globVarPartInfo.var->getName();
                 dbgs() << " "; globVarPartInfo.var->getType()->print(dbgs()); dbgs() << "\n");
   }
   processedValues.insert(globVar);

   // This is needed since when the alloca is used in a function call, it is not passed directly, but first there
   // is a GetElementPtr instruction to get the address of the first element and then the value retrieved is
   // passed to the function
   std::vector<Value*> gepAfterAlloc;
   for(User* user : globVar->users())
   {
      auto* gepInst = dyn_cast<GEPOperator>(user);
      if(!gepInst)
      {
         continue;
      }

      std::vector<Value*> gepIndices(gepInst->idx_begin(), gepInst->idx_end());
      // TODO: We should also check that the gep is taking the pointer of the first element
      // FIXME: allocPartInfoIt could be null! In the if else before if the flow goes into the else branch
      // the allocPartInfoIt is not set!
      // if(!hasConstantIndicesN(gepIndices, allocPartInfoIt->scheme.size()))
      // {
      //    continue;
      // }

      gepAfterAlloc.push_back(gepInst);
   }

   // To be sure, we search call instructions from the alloca and the GetElementPtr instructions
   collectArrayPartitionFromCallInstr(arrPartCtx, globVar, parts, processedValues);
   for(auto* v : gepAfterAlloc)
   {
      collectArrayPartitionFromCallInstr(arrPartCtx, v, parts, processedValues);
   }
}

void diffuseArrPartConfigs(ArrPartCtx& arrPartCtx, std::vector<std::string>& workQueue)
{
   std::vector<std::vector<PartInfo>*> parts;
   std::set<Value*> processedValues;
   for(const auto& fnName : workQueue)
   {
      if(!arrPartCtx.fnTable.count(fnName))
      {
         continue;
      }

      LLVM_DEBUG(dbgs() << "\n[ARR_PART] Diffusing the args of the function " << fnName << "\n");
      auto fnInfo = getFnInfoOrDie(arrPartCtx.fnTable, fnName);
      for(auto& argPartInfo : fnInfo.args)
      {
         LLVM_DEBUG(dbgs() << "\n[ARR_PART] Diffusing the arg " << argPartInfo.argName
                           << " (idx = " << argPartInfo.arg->getArgNo() << ")\n");
         collectArrayPartitionFromArg(arrPartCtx, argPartInfo.arg, parts, processedValues);
         partsInfoUnion(parts);
         parts.clear();
      }

      LLVM_DEBUG(dbgs() << "\n[ARR_PART] Diffusing the allocs of the function " << fnName << "\n");
      for(auto& allocPartInfo : fnInfo.allocs)
      {
         LLVM_DEBUG(dbgs() << "\n[ARR_PART] Diffusing the alloc "; allocPartInfo.inst->print(dbgs()); dbgs() << "\n");
         collectArrayPartitionFromAlloca(arrPartCtx, allocPartInfo.inst, parts, processedValues);
         partsInfoUnion(parts);
         parts.clear();
      }
   }
}

// Helper template function to handle the common dim logic
template <typename CreateOrUpdateFn>
void setPartitionByDim(const std::vector<size_t>& completeDims, uint64_t format, uint64_t numMemories, uint64_t dim,
                       CreateOrUpdateFn createOrUpdate)
{
   uint64_t numDims = completeDims.size();

   if(dim == 0)
   {
      // Apply partitioning to all dimensions
      for(uint64_t d = 1; d <= numDims; d++)
      {
         auto conf = createPartInfo(completeDims, format, numMemories, d - 1, numDims);
         createOrUpdate(conf, d - 1);
      }
   }
   else
   {
      // Apply partitioning to specific dimension
      uint64_t idxDim = dim - 1; // dim is 1-indexed
      if(idxDim >= numDims)
      {
         REPORT_FATAL_ERROR_WITH_REPORT("The dim attribute should be between 0 and the dimensions of the array");
      }

      auto conf = createPartInfo(completeDims, format, numMemories, idxDim, numDims);
      createOrUpdate(conf, idxDim);
   }
}

inline bool isArgPartitionable(Argument* arg)
{
   return arg->getType()->isPointerTy();
}

void setArrPartArg(ArrPartCtx& arrPartCtx, Argument* arg, uint64_t format, uint64_t numMemories, uint64_t dim)
{
   auto& doc = *arrPartCtx.doc;
   StringRef fnName = arg->getParent()->getName();
   auto& args = arrPartCtx.fnTable.try_emplace(fnName, arg->getParent()).first->second.args;
   auto nodeFn = doc.child("module").find_child_by_attribute("function", "symbol", fnName.data());
   auto nodeParam = nodeFn.child("parameters")
                        .find_child_by_attribute("parameter", "index", std::to_string(arg->getArgNo()).c_str());
   std::string argName = nodeParam.attribute("bundle").as_string();
   std::vector<size_t> completeDims = getDimsFromString(nodeParam.attribute("array_dims").as_string());

   if(!isArgPartitionable(arg))
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a function argument that is not an array");
   }

   setPartitionByDim(completeDims, format, numMemories, dim, [&](const PartInfo& conf, uint64_t idxDim) {
      auto localArgPartInfoIt = findPartInfoInContainer(arg, args);
      if(localArgPartInfoIt == args.end())
      {
         ArgPartInfo argPartInfo(argName, arg, completeDims);
         argPartInfo.setPartInfo(conf, idxDim);
         args.push_back(argPartInfo);
         return;
      }
      localArgPartInfoIt->setPartInfo(conf, idxDim);
   });
}

void setArrPartAlloca(ArrPartCtx& arrPartCtx, AllocaInst* allocaInst, uint64_t format, uint64_t numMemories,
                      uint64_t dim)
{
   if(!allocaInst->getAllocatedType()->isArrayTy())
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a local value that is not an array");
   }

   StringRef fnName = allocaInst->getFunction()->getName();
   auto& allocs = arrPartCtx.fnTable.try_emplace(fnName, allocaInst->getFunction()).first->second.allocs;
   auto* completeTy = cast<ArrayType>(allocaInst->getAllocatedType());
   std::vector<size_t> completeDims = getDimsFromArrayType(completeTy);

   setPartitionByDim(completeDims, format, numMemories, dim, [&](const PartInfo& conf, uint64_t idxDim) {
      auto localAllocaPartInfoIt = llvm::find_if(allocs, [&](const AllocaPartInfo& a) { return a.inst == allocaInst; });
      if(localAllocaPartInfoIt == allocs.end())
      {
         AllocaPartInfo allocaPartInfo(allocaInst);
         allocaPartInfo.setPartInfo(conf, idxDim);
         allocs.push_back(allocaPartInfo);
         return;
      }
      localAllocaPartInfoIt->setPartInfo(conf, idxDim);
   });
}

void setArrPartGlobalVar(ArrPartCtx& arrPartCtx, GlobalVariable* globalVar, uint64_t format, uint64_t numMemories,
                         uint64_t dim)
{
   auto& globalVars = arrPartCtx.globalVars;
   auto* completeTy = dyn_cast<ArrayType>(globalVar->getValueType());

   if(!globalVar->getType()->isPointerTy() || completeTy == nullptr)
   {
      REPORT_FATAL_ERROR_WITH_REPORT("Trying to partition a global variable that is not an array");
   }

   std::vector<size_t> completeDims = getDimsFromArrayType(completeTy);
   setPartitionByDim(completeDims, format, numMemories, dim, [&](const PartInfo& conf, uint64_t idxDim) {
      auto localGlobalPartInfoIt =
          llvm::find_if(globalVars, [&](const GlobalPartInfo& g) { return g.var == globalVar; });
      if(localGlobalPartInfoIt == globalVars.end())
      {
         GlobalPartInfo globalPartInfo(globalVar);
         globalPartInfo.setPartInfo(conf, idxDim);
         globalVars.push_back(globalPartInfo);
         return;
      }
      localGlobalPartInfoIt->setPartInfo(conf, idxDim);
   });
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

void describeArrPartRequests(ArrPartCtx& ctx)
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
      auto fnPartInfo = it->second;
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

void inverseTopologicalSort(Function* fn, std::vector<std::string>& workQueue)
{
   for(auto& bb : *fn)
   {
      for(auto& inst : bb)
      {
         if(auto* callInst = dyn_cast<CallInst>(&inst))
         {
            if(!callInst->getCalledFunction() || callInst->getCalledFunction()->isIntrinsic() ||
               callInst->getCalledFunction()->getName() == BAMBU_CSROA_PARTITION_FUN_NAME ||
               llvm::is_contained(workQueue, callInst->getCalledFunction()->getName().str()))
            {
               continue;
            }

            inverseTopologicalSort(callInst->getCalledFunction(), workQueue);
         }
      }
   }

   workQueue.push_back(fn->getName().str());
}

bool loadXMLModule(pugi::xml_document& doc, std::string& outdirName)
{
   const auto arch_filename = outdirName + "/architecture.xml";
   if(!doc.load_file(arch_filename.c_str()))
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
Function* findTopFunction(Module& M, std::string& topFunctionNameArgPass)
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
