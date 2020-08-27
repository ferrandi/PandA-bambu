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
 *              Copyright (C) 2018-2019 Politecnico di Milano
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
/*
 * The implementation performs scalar replacement of aggregates.
 *
 * @author Marco Siracusa <marco.siracusa@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP

#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Dominators.h>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <set>

typedef std::set<llvm::Instruction*> inst_set_ty;
typedef std::map<llvm::Function*, std::set<llvm::AllocaInst*>> fun_to_alloca_map_ty;

#define wrapper_function_name "__non_const_wrapper__"

enum SROA_phase
{
   SROA_functionVersioning,
   SROA_disaggregation,
   SROA_wrapperInlining
};

class Expandability
{
 public:
   bool expandability;
   double area_profit;
   double latency_profit;

   Expandability() : expandability(true), area_profit(0.0), latency_profit(0.0)
   {
   }

   Expandability(bool expandability, double area_profit, double latency_profit) : expandability(expandability), area_profit(area_profit), latency_profit(latency_profit)
   {
   }

   bool cast()
   {
      bool profitable = (area_profit + latency_profit) >= 0;
      bool got_casted = expandability and !profitable;

      expandability = expandability and profitable;
      area_profit = 0.0;
      latency_profit = 0.0;

      return got_casted;
   }

   Expandability get_casted()
   {
      return Expandability(expandability and (area_profit + latency_profit) > 0, 0.0, 0.0);
   }

   void and_add(const Expandability& op)
   {
      expandability = op.expandability and expandability;
      area_profit += op.area_profit;
      latency_profit += op.latency_profit;
   }

   void and_cond_add(const Expandability& op)
   {
      expandability = op.expandability and expandability;
      if(op.expandability)
      {
         area_profit += op.area_profit;
         latency_profit += op.latency_profit;
      }
   }

   void operator*=(double factor)
   {
      area_profit *= factor;
      latency_profit *= factor;
   }

   void operator+=(const Expandability& exp)
   {
      area_profit += exp.area_profit;
      latency_profit += exp.latency_profit;
   }

   void operator-=(const Expandability& exp)
   {
      area_profit -= exp.area_profit;
      latency_profit -= exp.latency_profit;
   }

   std::string get_string() const
   {
      std::string ret = "<" + std::to_string(expandability) + "," + std::to_string(area_profit) + "," + std::to_string(latency_profit) + ">";
      return ret;
   }
};

class CustomScalarReplacementOfAggregatesPass : public llvm::ModulePass
{
 public:
   char ID = 0;

 private:
   const unsigned short sroa_phase = 0;

   const std::string kernel_name;

 public:
   explicit CustomScalarReplacementOfAggregatesPass(const std::string& _kernel_name, char& _ID, unsigned short _sroa_phase) : llvm::ModulePass(_ID), sroa_phase(_sroa_phase), kernel_name(_kernel_name)
   {
   }

   explicit CustomScalarReplacementOfAggregatesPass(const std::string& _kernel_name, unsigned short _sroa_phase) : llvm::ModulePass(ID), sroa_phase(_sroa_phase), kernel_name(_kernel_name)
   {
   }

   bool runOnModule(llvm::Module& module) override;

   void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
   {
      AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
      AU.addRequiredTransitive<llvm::ScalarEvolutionWrapperPass>();
      AU.addRequiredTransitive<llvm::TargetTransformInfoWrapperPass>();
      AU.addRequiredTransitive<llvm::TargetLibraryInfoWrapperPass>();
      AU.addRequiredTransitive<llvm::DominatorTreeWrapperPass>();
      AU.addRequiredTransitive<llvm::AssumptionCacheTracker>();
   }
};

// CustomScalarReplacementOfAggregatesPass* createSROACodeSimplificationPass(std::string kernel_name);

CustomScalarReplacementOfAggregatesPass* createSROAFunctionVersioningPass(std::string kernel_name);

CustomScalarReplacementOfAggregatesPass* createSROADisaggregationPass(std::string kernel_name);

CustomScalarReplacementOfAggregatesPass* createSROAWrapperInliningPass(std::string kernel_name);

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_CUSTOMSCALARREPLACEMENTOFAGGREGATESPASS_HPP
