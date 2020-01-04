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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
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
 * @file flopoco_wrapper.cpp
 * @brief Implementation of the wrapper to FloPoCo for VHDL code generation.
 *
 * Implementation of the object used to invoke the FloPoCo framework
 * to generate VHDL code for floating-point functional units
 *
 * @author Daniele Mastrandrea <daniele.mastrandrea@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_SKIP_WARNING_SECTIONS.hpp"
#include "config_WPEDANTIC.hpp"

/// circuit include
#include "structural_objects.hpp"

/// Includes the class definition
#include "flopoco_wrapper.hpp"

/// Standard PandA include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "simple_indent.hpp"
#include "utility.hpp"

/// Standard include
#include <cerrno>
#include <unistd.h>

/// Streams include
#include <fstream>
#include <iosfwd>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <vector>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if WPEDANTIC
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#else
#pragma GCC diagnostic warning "-Wshadow"
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wold-style-cast"
#pragma GCC diagnostic warning "-Wsign-conversion"
#pragma GCC diagnostic warning "-Wredundant-decls"
#pragma GCC diagnostic warning "-Wunused-parameter"
#if WPEDANTIC
#pragma GCC diagnostic warning "-Wpedantic"
#endif
#endif

/// FloPoCo include
#undef DEBUG
#include "FP2Fix.hpp"
#include "FPAdderSinglePath.hpp"
#include "FPAssign.hpp"
#include "FPDiv.hpp"
#include "FPExp.hpp"
#include "FPLog.hpp"
#include "FPMultiplier.hpp"
#include "FPPow.hpp"
#include "FPSqrt.hpp"
#include "FPSqrtPoly.hpp"
#include "FPge_expr.hpp"
#include "FPgt_expr.hpp"
#include "FPle_expr.hpp"
#include "FPlt_expr.hpp"
#include "Fix2FP.hpp"
#include "InputIEEE.hpp"
#include "Operator.hpp"
#include "OutputIEEE.hpp"
#include "Target.hpp"
#include "Targets/CycloneII.hpp"
#include "Targets/CycloneV.hpp"
#include "Targets/Spartan3.hpp"
#include "Targets/StratixII.hpp"
#include "Targets/StratixIII.hpp"
#include "Targets/StratixIV.hpp"
#include "Targets/Virtex4.hpp"
#include "Targets/Virtex5.hpp"
#include "Targets/Virtex6.hpp"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

/// Redefinition of FloPoCo's global variables, declared in flopoco/src/main.cpp
/// None of the following is actually used inside the wrapper

#define OPLIST oplist

namespace flopoco
{
   std::string filename = "flopoco.vhdl";
   std::string cl_name = "";
   int LongAccN = 0;
   /* flex vars */
   int yyTheCycle;
   vector<pair<string, int>> theUseTable;
   bool combinatorialOperator;
} // namespace flopoco

flopoco_wrapper::flopoco_wrapper(int
#ifndef NDEBUG
                                     _debug_level
#endif
                                 ,
                                 const std::string& FU_target)
    :
#ifndef NDEBUG
      debug_level(_debug_level),
#endif
      PP(STD_OPENING_CHAR, STD_CLOSING_CHAR, 3),
      type(UT_UNKNOWN),
      signed_p(false)
{
   // Get the target architecture
   if("Spartan-3" == FU_target)
      target = new flopoco::Spartan3();
   else if("Virtex-4" == FU_target)
      target = new flopoco::Virtex4();
   else if("Virtex-5" == FU_target)
      target = new flopoco::Virtex5();
   else if("Virtex-6" == FU_target)
      target = new flopoco::Virtex6();
   else if("Virtex-7" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("Zynq" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("Zynq-VVD" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("Zynq-YOSYS-VVD" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("Virtex-7-VVD" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("Artix-7-VVD" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if(FU_target.find("CycloneII") != std::string::npos)
      target = new flopoco::CycloneII();
   else if(FU_target.find("CycloneV") != std::string::npos)
      target = new flopoco::CycloneV();
   else if(FU_target.find("StratixII") != std::string::npos)
      target = new flopoco::StratixII();
   else if(FU_target.find("StratixIII") != std::string::npos)
      target = new flopoco::StratixIII();
   else if(FU_target.find("StratixIV") != std::string::npos)
      target = new flopoco::StratixIV();
   else if(FU_target.find("StratixV") != std::string::npos)
      target = new flopoco::StratixIV();
   else if(FU_target.find("LatticeECP3") != std::string::npos)
      target = new flopoco::CycloneII();
   else if("NG-medium" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else if("NG-large" == FU_target) /// does not exist so we use Virtex 6 target
      target = new flopoco::Virtex6();
   else
      THROW_UNREACHABLE("Non supported target architecture.");

   /// sollya initialization
   jmp_buf recover;

   initTool();
   if(setjmp(recover))
   {
      /* If we are here, we have come back from an error in the library */
      THROW_ERROR("An error occurred somewhere");
   }
   setRecoverEnvironment(&recover);
   extern int recoverEnvironmentReady;
   recoverEnvironmentReady = 1;
}

flopoco_wrapper::~flopoco_wrapper()
{
   // finishTool();
}

void flopoco_wrapper::add_FU(const std::string& FU_type, unsigned int FU_prec_in, unsigned int FU_prec_out, const std::string& FU_name, const std::string& pipe_parameter)
{
   // Get the number of bits for the number representation
   unsigned int n_mant_in, n_exp_in;
   unsigned int n_mant_out, n_exp_out;
   DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
   DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
   signed_p = false;
   double freq;

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating FloPoCo operator for unit " + FU_type + "(" + STR(FU_prec_in) + "-" + STR(FU_prec_out) + "-" + pipe_parameter + ")");

   if(pipe_parameter != "" && pipe_parameter != "0")
   {
      target->setPipelined();
      freq = boost::lexical_cast<double>(pipe_parameter);
   }
   else
   {
      target->setNotPipelined();
      freq = DEFAULT_TARGET_FREQUENCY;
   }

   /// set the target frequency
   target->setFrequency(1e6 * freq);

   flopoco::Operator* op = nullptr;
   THROW_ASSERT(n_mant_in > 0 && n_exp_in > 0, "Unsupported significand and exponent values.");
   THROW_ASSERT(n_mant_out > 0 && n_exp_out > 0, "Unsupported significand and exponent values.");

   // Get the Functional Unit, sets correct name, then adds to resources
   type = flopoco_wrapper::UT_UNKNOWN;
   if("FPAdder" == FU_type)
   {
      type = flopoco_wrapper::UT_ADD;
      op = new flopoco::FPAdderSinglePath(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FPSub" == FU_type)
   {
      type = flopoco_wrapper::UT_SUB;
      op = new flopoco::FPAdderSinglePath(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FPAddSub" == FU_type)
   {
      type = flopoco_wrapper::UT_ADDSUB;
      op = new flopoco::FPAdderSinglePath(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FPMultiplier" == FU_type)
   {
      type = flopoco_wrapper::UT_MULT;
      op = new flopoco::FPMultiplier(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FPDiv" == FU_type)
   {
      type = flopoco_wrapper::UT_DIV;
      op = new flopoco::FPDiv(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("FPExp" == FU_type)
   {
      type = flopoco_wrapper::UT_EXP;
      op = new flopoco::FPExp(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), 0, 0, -1, false, 0.7f);
   }
   else if("FPSqrtPoly" == FU_type)
   {
      type = flopoco_wrapper::UT_SQRT;
      op = new flopoco::FPSqrtPoly(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), false, 3 /*degree*/);
   }
   else if("FPSqrt" == FU_type)
   {
      type = flopoco_wrapper::UT_SQRT;
      op = new flopoco::FPSqrt(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("Fix2FP_32_32" == FU_type)
   {
      signed_p = true;
      type = flopoco_wrapper::UT_IFIX2FP;
      FU_prec_out = 32;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 1, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("Fix2FP_32_64" == FU_type)
   {
      signed_p = true;
      type = flopoco_wrapper::UT_IFIX2FP;
      FU_prec_out = 64;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 1, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("Fix2FP_64_32" == FU_type)
   {
      signed_p = true;
      FU_prec_out = 32;
      type = flopoco_wrapper::UT_IFIX2FP;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 1, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("Fix2FP_64_64" == FU_type)
   {
      signed_p = true;
      type = flopoco_wrapper::UT_IFIX2FP;
      FU_prec_out = 64;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 1, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("UFix2FP_32_32" == FU_type)
   {
      type = flopoco_wrapper::UT_UFIX2FP;
      FU_prec_out = 32;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 0, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("UFix2FP_32_64" == FU_type)
   {
      type = flopoco_wrapper::UT_UFIX2FP;
      FU_prec_out = 64;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 0, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("UFix2FP_64_32" == FU_type)
   {
      type = flopoco_wrapper::UT_UFIX2FP;
      FU_prec_out = 32;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 0, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("UFix2FP_64_64" == FU_type)
   {
      type = flopoco_wrapper::UT_UFIX2FP;
      FU_prec_out = 64;
      DECODE_BITS(FU_prec_out, n_mant_out, n_exp_out);
      op = new flopoco::Fix2FP(target, 0, static_cast<int>(FU_prec_in) - 1, 0, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FP2Fix_32_32" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2IFIX;
      FU_prec_in = 32;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 1, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_32_u32" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2UFIX;
      FU_prec_in = 32;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 0, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_32_64" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2IFIX;
      FU_prec_in = 32;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 1, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_32_u64" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2UFIX;
      FU_prec_in = 32;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 0, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_64_32" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2IFIX;
      FU_prec_in = 64;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 1, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_64_u32" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2UFIX;
      FU_prec_in = 64;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 0, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_64_64" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2IFIX;
      FU_prec_in = 64;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 1, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FP2Fix_64_u64" == FU_type)
   {
      type = flopoco_wrapper::UT_FP2UFIX;
      FU_prec_in = 64;
      DECODE_BITS(FU_prec_in, n_mant_in, n_exp_in);
      op = new flopoco::FP2Fix(target, 0, static_cast<int>(FU_prec_out) - 1, 0, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), true);
   }
   else if("FF_CONV" == FU_type)
   {
      type = flopoco_wrapper::UT_FF_CONV;
      op = new flopoco::FPAssign(target, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
   }
   else if("FPgt_expr" == FU_type)
   {
      type = flopoco_wrapper::UT_compare_expr;
      op = new flopoco::FPgt_expr(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("FPlt_expr" == FU_type)
   {
      type = flopoco_wrapper::UT_compare_expr;
      op = new flopoco::FPlt_expr(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("FPge_expr" == FU_type)
   {
      type = flopoco_wrapper::UT_compare_expr;
      op = new flopoco::FPge_expr(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("FPle_expr" == FU_type)
   {
      type = flopoco_wrapper::UT_compare_expr;
      op = new flopoco::FPle_expr(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
   }
   else if("FPLog" == FU_type)
   {
      type = flopoco_wrapper::UT_LOG;
      op = new flopoco::FPLog(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), 0);
   }
   else if("FPPow" == FU_type)
   {
      type = flopoco_wrapper::UT_POW;
      op = new flopoco::FPPow(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), 0);
   }
   else
   {
      THROW_UNREACHABLE("Not supported FU: " + FU_type);
   }
   OPLIST.push_back(op);
   std::string FU_name_stored;
   FU_name_stored = ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter);
   op->changeName(WRAPPED_PREFIX + FU_name_stored);
   FUs[WRAPPED_PREFIX + FU_name_stored] = op;
   FU_to_prec.insert(make_pair(FU_name_stored, std::pair<unsigned int, unsigned int>(FU_prec_in, FU_prec_out)));

   // Adds two additional Functional Units to perform conversion
   // from FloCoCo encoding to IEEE-754 number format and viceversa
   if(type != flopoco_wrapper::UT_IFIX2FP and type != flopoco_wrapper::UT_UFIX2FP)
   {
      if(type == flopoco_wrapper::UT_FF_CONV)
         op = new flopoco::InputIEEE(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out));
      else
         op = new flopoco::InputIEEE(target, static_cast<int>(n_exp_in), static_cast<int>(n_mant_in), static_cast<int>(n_exp_in), static_cast<int>(n_mant_in));
      OPLIST.push_back(op);
      op->changeName(IN_WRAP_PREFIX + FU_name_stored);
      FUs[IN_WRAP_PREFIX + FU_name_stored] = op;
   }
   if(type != flopoco_wrapper::UT_FP2UFIX and type != flopoco_wrapper::UT_FP2IFIX and type != flopoco_wrapper::UT_compare_expr)
   {
      op = new flopoco::OutputIEEE(target, static_cast<int>(n_exp_out), static_cast<int>(n_mant_out), static_cast<int>(n_exp_out), static_cast<int>(n_mant_out), true);
      OPLIST.push_back(op);
      op->changeName(OUT_WRAP_PREFIX + FU_name_stored);
      FUs[OUT_WRAP_PREFIX + FU_name_stored] = op;
   }
}

unsigned int flopoco_wrapper::get_FUPipelineDepth(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const std::string& pipe_parameter) const
{
   std::string FU_name_stored = ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter);
   unsigned int fu_pipe_depth = static_cast<unsigned int>(get_FU(WRAPPED_PREFIX + FU_name_stored)->getPipelineDepth());
   if(type != flopoco_wrapper::UT_IFIX2FP and type != flopoco_wrapper::UT_UFIX2FP)
      fu_pipe_depth += static_cast<unsigned int>(get_FU(IN_WRAP_PREFIX + FU_name_stored)->getPipelineDepth());
   if(type != flopoco_wrapper::UT_FP2UFIX and type != flopoco_wrapper::UT_FP2IFIX and type != flopoco_wrapper::UT_compare_expr)
      fu_pipe_depth += static_cast<unsigned int>(get_FU(OUT_WRAP_PREFIX + FU_name_stored)->getPipelineDepth());
   return fu_pipe_depth;
}

flopoco::Operator* flopoco_wrapper::get_FU(std::string FU_name_stored) const
{
   auto op_found = FUs.find(FU_name_stored);
   THROW_ASSERT(op_found != FUs.end(), "Functional unit " + FU_name_stored + " not found (maybe not yet generated)");
   return op_found->second;
}

void flopoco_wrapper::outputWrapVHDL(const std::string& FU_name_stored, std::ostream& os, const std::string& pipe_parameter)
{
   outputHeaderVHDL(FU_name_stored, os);
   // Write library declaration
   PP(os, "library ieee;\n");
   PP(os, "use ieee.std_logic_1164.all;\n");
   PP(os, "use ieee.numeric_std.all;\n");
   PP(os, "\n");
   PP(os, "entity " + FU_name_stored + " is\n");
   PP.indent();
   PP(os, "generic(");
   CustomUnorderedMap<std::string, std::pair<unsigned int, unsigned int>>::const_iterator FU_to_prec_it = FU_to_prec.find(FU_name_stored);
   // Write input port(s) generics
   const std::vector<std::string> p_in = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_in, false);
   for(const auto& p_in_it : p_in)
      PP(os, "BITSIZE_" + p_in_it + ": integer := " + STR(FU_to_prec_it->second.first) + "; ");
   // Write output port(s) generics
   const std::vector<std::string> p_out = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_out, false);
   for(auto p_out_it = p_out.begin(); p_out_it != p_out.end(); ++p_out_it)
      if(p_out_it + 1 != p_out.end())
         PP(os, "BITSIZE_" + *p_out_it + ": integer := " + STR(FU_to_prec_it->second.second) + ";");
      else
         PP(os, "BITSIZE_" + *p_out_it + ": integer := " + STR(FU_to_prec_it->second.second));
   PP(os, ");\n");
   PP(os, "port (\n");
   PP.indent();
   // Write port declaration for top entity
   outputPortDeclaration(WRAPPED_PREFIX, FU_name_stored, os, top, pipe_parameter);
   // Begin writing the architecture of top entity
   PP.deindent();
   PP(os, ");\n");
   PP.deindent();
   PP(os, "end entity;\n");
   PP(os, "\n");
   PP(os, "architecture arch of " + FU_name_stored + " is\n");
   PP.indent();
   // Write declaration for each of entity's components
   std::string prefixes[] = {WRAPPED_PREFIX, IN_WRAP_PREFIX, OUT_WRAP_PREFIX};
   for(const auto& prefixe : prefixes)
   {
      if(!(((type == flopoco_wrapper::UT_UFIX2FP or type == flopoco_wrapper::UT_IFIX2FP) && prefixe == IN_WRAP_PREFIX) ||
           (((type == flopoco_wrapper::UT_FP2UFIX or type == flopoco_wrapper::UT_FP2IFIX) || type == flopoco_wrapper::UT_compare_expr) && prefixe == OUT_WRAP_PREFIX)))
      {
         PP(os, "component " + prefixe + FU_name_stored + "\n");
         PP.indent();
         PP(os, "port (\n");
         PP.indent();
         if(WRAPPED_PREFIX == prefixe)
            outputPortDeclaration(prefixe, FU_name_stored, os, wrapped, pipe_parameter);
         else if(IN_WRAP_PREFIX == prefixe)
            outputPortDeclaration(prefixe, FU_name_stored, os, in_wrap, pipe_parameter);
         else if(OUT_WRAP_PREFIX == prefixe)
            outputPortDeclaration(prefixe, FU_name_stored, os, out_wrap, pipe_parameter);
         else
            THROW_UNREACHABLE("Something wrong happened");
         PP.deindent();
         PP(os, ");\n");
         PP.deindent();
         PP(os, "end component;\n");
      }
   }
   // Write the declaration of needed signals
   outputSignals(FU_name_stored, os);
   // Begin behavioral description
   PP(os, "begin\n");
   PP.indent();
   // Write port mapping
   if(type == flopoco_wrapper::UT_FP2UFIX)
   {
      PP(os, "O <= unsigned(wireOut1);\n");
   }
   else if(type == flopoco_wrapper::UT_FP2IFIX)
   {
      PP(os, "O <= signed(wireOut1);\n");
   }
   outputPortMap(FU_name_stored, os, pipe_parameter);
   // End with architecture
   PP.deindent();
   PP.deindent();
   PP(os, "end architecture;\n");
   PP(os, "\n");
}

void flopoco_wrapper::outputPortMap(const std::string& FU_name_stored, std::ostream& os, const std::string& pipe_parameter)
{
   std::string mapping;
   const std::vector<std::string> p_wrapped_in = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_in, false);
   const std::vector<std::string> p_wrapped_out = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_out, false);
   std::vector<std::string> p_in_wrap_in;
   std::vector<std::string> p_in_wrap_out;
   if(type != flopoco_wrapper::UT_IFIX2FP and type != flopoco_wrapper::UT_UFIX2FP)
   {
      p_in_wrap_in = get_ports(IN_WRAP_PREFIX + FU_name_stored, 0, port_in, false);
      p_in_wrap_out = get_ports(IN_WRAP_PREFIX + FU_name_stored, 0, port_out, false);
   }
   std::vector<std::string> p_out_wrap_in;
   std::vector<std::string> p_out_wrap_out;
   if(type != flopoco_wrapper::UT_FP2UFIX and type != flopoco_wrapper::UT_FP2IFIX and type != flopoco_wrapper::UT_compare_expr)
   {
      p_out_wrap_in = get_ports(OUT_WRAP_PREFIX + FU_name_stored, 0, port_in, false);
      p_out_wrap_out = get_ports(OUT_WRAP_PREFIX + FU_name_stored, 0, port_out, false);
   }

   CustomUnorderedMap<std::string, std::pair<unsigned int, unsigned int>>::const_iterator FU_to_prec_it = FU_to_prec.find(FU_name_stored);
   unsigned int n_bits_in /*, n_bits_out*/;
   unsigned int prec_in /*, prec_out*/;
   prec_in = FU_to_prec_it->second.first;
   // prec_out = FU_to_prec_it->second.second;
   n_bits_in = prec_in;
   //   n_bits_out=prec_out;

   // Write mapping for wrapped component
   mapping = "";
   for(unsigned int i = 0; i < p_wrapped_in.size(); i++)
   {
      if(type == flopoco_wrapper::UT_ADDSUB && i == 1)
         mapping += p_wrapped_in.at(i) + " => wire_ADDSUB, ";
      else if(type == flopoco_wrapper::UT_SUB && i == 1)
         mapping += p_wrapped_in.at(i) + " => wire_SUB, ";
      else if(type == flopoco_wrapper::UT_IFIX2FP)
      {
         mapping += p_wrapped_in.at(i) + " => std_logic_vector(" + p_wrapped_in.at(i) + "), ";
      }
      else if(type == flopoco_wrapper::UT_UFIX2FP)
      {
         mapping += p_wrapped_in.at(i) + " => std_logic_vector(" + p_wrapped_in.at(i) + "), ";
      }
      else
         mapping += p_wrapped_in.at(i) + " => wireIn" + STR(i + 1) + ", ";
   }
   for(unsigned int j = 0; j < p_wrapped_out.size(); j++)
   {
      if(type == flopoco_wrapper::UT_compare_expr)
         mapping += p_wrapped_out.at(j) + " => " + p_wrapped_out.at(j) + "(0 downto 0)";
      else
         mapping += p_wrapped_out.at(j) + " => wireOut" + STR(j + 1);
   }
   if(pipe_parameter != "" && pipe_parameter != "0")
   {
      const std::string p_clock = get_port(clk);
      const std::string p_reset = get_port(rst);
      mapping += ", " + p_clock + "=> " + std::string(CLOCK_PORT_NAME) + ", " + p_reset + "=> " + std::string(RESET_PORT_NAME);
   }

   if(type == flopoco_wrapper::UT_ADDSUB)
   {
      PP(os, "wire_ADDSUB <= wireIn2(" + STR(n_bits_in + 1) + " downto " + STR(n_bits_in) + ") & (wireIn2(" + STR(n_bits_in - 1) + ") xor sel_minus_expr) & wireIn2(" + STR(n_bits_in - 2) + " downto 0);\n");
   }
   else if(type == flopoco_wrapper::UT_SUB)
   {
      PP(os, "wire_SUB <= wireIn2(" + STR(n_bits_in + 1) + " downto " + STR(n_bits_in) + ") & (wireIn2(" + STR(n_bits_in - 1) + ") xor '1') & wireIn2(" + STR(n_bits_in - 2) + " downto 0);\n");
   }
   else if(type == flopoco_wrapper::UT_EXP || type == flopoco_wrapper::UT_SQRT || type == flopoco_wrapper::UT_LOG || type == flopoco_wrapper::UT_POW)
   {
      PP(os, std::string(DONE_PORT_NAME) + " <= '0';\n");
   }
   PP(os, "fu : " + STR(WRAPPED_PREFIX + FU_name_stored) + " port map (" + mapping + ");\n");
   // Write mapping for input converters
   if(type != flopoco_wrapper::UT_IFIX2FP and type != flopoco_wrapper::UT_UFIX2FP)
   {
      for(unsigned int i = 0; i < p_wrapped_in.size(); i++)
      {
         mapping = "";
         mapping += p_in_wrap_in.at(0) + "=>" + p_wrapped_in.at(i) + ", ";
         mapping += p_in_wrap_out.at(0) + "=>wireIn" + STR(i + 1);
         if(pipe_parameter != "" && pipe_parameter != "0")
         {
            const std::string p_clock = get_port(clk);
            const std::string p_reset = get_port(rst);
            mapping += ", " + p_clock + "=> " + std::string(CLOCK_PORT_NAME) + ", " + p_reset + "=> " + std::string(RESET_PORT_NAME);
         }
         PP(os, "in" + STR(i + 1) + " : " + STR(IN_WRAP_PREFIX + FU_name_stored) + " port map (" + mapping + ");\n");
      }
   }
   // Write mapping for output converters
   if(type != flopoco_wrapper::UT_FP2UFIX and type != flopoco_wrapper::UT_FP2IFIX and type != flopoco_wrapper::UT_compare_expr)
   {
      for(unsigned int i = 0; i < p_wrapped_out.size(); i++)
      {
         mapping = "";
         mapping += p_out_wrap_in.at(0) + "=>wireOut" + STR(i + 1) + ", ";
         mapping += p_out_wrap_out.at(0) + "=>" + p_wrapped_out.at(i);
         if(pipe_parameter != "" && pipe_parameter != "0")
         {
            const std::string p_clock = get_port(clk);
            const std::string p_reset = get_port(rst);
            mapping += ", " + p_clock + "=> " + std::string(CLOCK_PORT_NAME) + ", " + p_reset + "=> " + std::string(RESET_PORT_NAME);
         }
         PP(os, "out" + STR(i + 1) + " : " + STR(OUT_WRAP_PREFIX + FU_name_stored) + " port map (" + mapping + ");\n");
      }
   }
}

void flopoco_wrapper::outputSignals(const std::string& FU_name_stored, std::ostream& os)
{
   std::string Signals = "";
   const std::vector<std::string> p_in = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_in, false);
   const std::vector<std::string> p_out = get_ports(WRAPPED_PREFIX + FU_name_stored, 0, port_out, false);
   CustomUnorderedMap<std::string, std::pair<unsigned int, unsigned int>>::const_iterator FU_to_prec_it = FU_to_prec.find(FU_name_stored);
   unsigned int n_bits_in, n_bits_out;
   unsigned int prec_in, prec_out;
   prec_in = FU_to_prec_it->second.first;
   prec_out = FU_to_prec_it->second.second;
   n_bits_in = prec_in;
   n_bits_out = prec_out;

   if(type == flopoco_wrapper::UT_ADDSUB)
      Signals += "wire_ADDSUB, ";
   else if(type == flopoco_wrapper::UT_SUB)
      Signals += "wire_SUB, ";
   else if(type == flopoco_wrapper::UT_FF_CONV)
      n_bits_in = prec_out;

   if(type == flopoco_wrapper::UT_IFIX2FP or type == flopoco_wrapper::UT_UFIX2FP)
   {
      if(prec_in != prec_out)
         PP(os, "signal I_temp : std_logic_vector(" + STR(std::max(prec_in, prec_out) - 1) + " downto 0);\n");
   }
   else
   {
      size_t n_in_elements = p_in.size();
      for(unsigned int i = 0; i < n_in_elements; i++)
         Signals += "wireIn" + STR(i + 1) + (i + 1 != n_in_elements ? ", " : "");
      PP(os, "signal " + Signals + " : std_logic_vector(" + STR(n_bits_in + 1) + " downto 0);\n");
   }
   if(type == flopoco_wrapper::UT_FP2UFIX or type == flopoco_wrapper::UT_FP2IFIX)
   {
      Signals = "";
      for(unsigned int j = 0; j < p_out.size(); j++)
         Signals += "wireOut" + STR(j + 1) + (j + 1 != p_out.size() ? ", " : "");
      PP(os, "signal " + Signals + " : std_logic_vector(" + STR(n_bits_out - 1) + " downto 0);\n");
   }
   else if(type != flopoco_wrapper::UT_compare_expr)
   {
      Signals = "";
      for(unsigned int j = 0; j < p_out.size(); j++)
         Signals += "wireOut" + STR(j + 1) + (j + 1 != p_out.size() ? ", " : "");
      PP(os, "signal " + Signals + " : std_logic_vector(" + STR(n_bits_out + 1) + " downto 0);\n");
   }
}

void flopoco_wrapper::outputPortDeclaration(const std::string& FU_prefix, const std::string& FU_name_stored, std::ostream& os, component_type c_type, const std::string& pipe_parameter)
{
   // Compute offsets for addition bits' handling
   int in_offset, out_offset;
   unsigned int n_bits_in, n_bits_out;
   unsigned int prec_in, prec_out;
   CustomUnorderedMap<std::string, std::pair<unsigned int, unsigned int>>::const_iterator FU_to_prec_it = FU_to_prec.find(FU_name_stored);
   in_offset = out_offset = -1;
   n_bits_in = n_bits_out = 0;
   prec_in = FU_to_prec_it->second.first;
   prec_out = FU_to_prec_it->second.second;
   if(wrapped == c_type)
   {
      if(type != flopoco_wrapper::UT_IFIX2FP and type != flopoco_wrapper::UT_UFIX2FP)
         in_offset += FLOPOCO_ADDITIONAL_BITS;
      if(type != flopoco_wrapper::UT_FP2UFIX and type != flopoco_wrapper::UT_FP2IFIX and type != flopoco_wrapper::UT_compare_expr)
         out_offset += FLOPOCO_ADDITIONAL_BITS;
      if(type == flopoco_wrapper::UT_FF_CONV)
         n_bits_in = n_bits_out = prec_out;
      else if(type == flopoco_wrapper::UT_compare_expr)
      {
         n_bits_in = prec_in;
         n_bits_out = 1;
      }
      else
      {
         n_bits_in = prec_in;
         n_bits_out = prec_out;
      }
   }
   else if(in_wrap == c_type)
   {
      out_offset += FLOPOCO_ADDITIONAL_BITS;
      if(type == flopoco_wrapper::UT_FF_CONV)
      {
         n_bits_in = prec_in;
         n_bits_out = prec_out;
      }
      else
         n_bits_in = n_bits_out = prec_in;
   }
   else if(out_wrap == c_type)
   {
      in_offset += FLOPOCO_ADDITIONAL_BITS;
      n_bits_in = n_bits_out = prec_out;
   }
   if(pipe_parameter != "" && pipe_parameter != "0")
   {
      if(wrapped == c_type || in_wrap == c_type || out_wrap == c_type)
      {
         PP(os, get_port(clk) + " : in std_logic;\n");
         PP(os, get_port(rst) + " : in std_logic;\n");
      }
   }
   const std::vector<std::string> p_in = get_ports(FU_prefix + FU_name_stored, 0, port_in, false);
   for(const auto& p_in_it : p_in)
      if(top == c_type)
      {
         if(type == flopoco_wrapper::UT_IFIX2FP)
         {
            PP(os, p_in_it + " : in signed(BITSIZE_" + p_in_it + "-1 downto 0);\n");
         }
         else if(type == flopoco_wrapper::UT_UFIX2FP)
         {
            PP(os, p_in_it + " : in unsigned(BITSIZE_" + p_in_it + "-1 downto 0);\n");
         }
         else
         {
            PP(os, p_in_it + " : in std_logic_vector(BITSIZE_" + p_in_it + "-1 downto 0);\n");
         }
      }
      else if(static_cast<int>(n_bits_in) + in_offset > 0)
         PP(os, p_in_it + " : in std_logic_vector(" + STR(static_cast<int>(n_bits_in) + in_offset) + " downto 0);\n");
      else
         PP(os, p_in_it + " : in std_logic;\n");
   // Write clock and reset ports declaration, only for top and wrapped entities
   if(top == c_type)
   {
      PP(os, std::string(CLOCK_PORT_NAME) + " : in std_logic;\n");
      PP(os, std::string(RESET_PORT_NAME) + " : in std_logic;\n");

      if(type == flopoco_wrapper::UT_ADDSUB)
      {
         PP(os, std::string("sel_plus_expr") + " : in std_logic;\n");
         PP(os, std::string("sel_minus_expr") + " : in std_logic;\n");
      }
      else if(type == flopoco_wrapper::UT_EXP || type == flopoco_wrapper::UT_SQRT || type == flopoco_wrapper::UT_LOG || type == flopoco_wrapper::UT_POW)
      {
         PP(os, std::string(START_PORT_NAME) + " : in std_logic;\n");
         PP(os, std::string(DONE_PORT_NAME) + " : out std_logic;\n");
      }
   }
   // Write output port(s) declaration
   const std::vector<std::string> p_out = get_ports(FU_prefix + FU_name_stored, 0, port_out, false);
   for(auto p_out_it = p_out.begin(); p_out_it != p_out.end(); ++p_out_it)
   {
      if(top == c_type)
      {
         if(type == flopoco_wrapper::UT_FP2IFIX)
         {
            PP(os, *p_out_it + " : out signed(BITSIZE_" + *p_out_it + "-1 downto 0)\n");
         }
         else if(type == flopoco_wrapper::UT_FP2UFIX)
         {
            PP(os, *p_out_it + " : out unsigned(BITSIZE_" + *p_out_it + "-1 downto 0)\n");
         }
         else
         {
            PP(os, *p_out_it + " : out std_logic_vector(BITSIZE_" + *p_out_it + "-1 downto 0)\n");
         }
      }
      else
         PP(os, *p_out_it + " : out std_logic_vector(" + STR(static_cast<int>(n_bits_out) + out_offset) + " downto 0)\n");
      if(p_out_it + 1 != p_out.end())
      {
         PP(os, ";\n");
      }
      else
      {
         PP(os, "\n");
      }
   }
}

void flopoco_wrapper::outputHeaderVHDL(const std::string& FU_name_stored, std::ostream& os) const
{
   os << "--------------------------------------------------------------------------------" << endl;
   os << "--                              " << FU_name_stored << endl;
   os << "-- Operator automatically generated by " << PACKAGE_NAME << " framework version " << PACKAGE_VERSION << endl;
   os << "-- assemblying operators generated by the Infinite Virtual Library FloPoCoLib" << endl;
   os << "-- Send any bug to: " << PACKAGE_BUGREPORT << endl;
   os << "--------------------------------------------------------------------------------" << endl;
}

int flopoco_wrapper::InternalWriteVHDL(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const std::string& filename, const std::string& pipe_parameter)
{
   std::string FU_name_stored = ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Writing VHDL code for unit " + FU_name_stored + " to file " + filename);
   if(FU_files.find(FU_name_stored) != FU_files.end())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "- Code for unit has already been generated");
      return 1;
   }
   std::ofstream file(filename.c_str());
   if(!file.is_open())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "- Something went wrong in file creation");
      return -1;
   }
   // Call FloPoCo method to generate VHDL for Functional Unit and Conversion Units
   {
      try
      {
         flopoco::Operator::outputVHDLToFile(OPLIST, file);
      }
      catch(const std::string& s)
      {
         cerr << "Exception while generating '" << s << endl;
      }
   }

   // Generate VHDL for Functional Unit wrapper entity
   outputWrapVHDL(FU_name_stored, file, pipe_parameter);
   // Mark the file as generated
   FU_files.insert(filename);
   file.close();
   OPLIST.clear();
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "- Successfully written to file!");
   return 0;
}

int flopoco_wrapper::writeVHDL(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, std::string pipe_parameter, std::string& filename)
{
   filename = ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter) + FILE_EXT;
   return this->InternalWriteVHDL(FU_name, FU_prec_in, FU_prec_out, filename, pipe_parameter);
}

std::string flopoco_wrapper::writeVHDLcommon()
{
   std::vector<flopoco::Operator*>* common_oplist = target->getGlobalOpListRef();
   if(!common_oplist || common_oplist->empty())
      return "";
   std::string filename = std::string("FloPoCo_common") + FILE_EXT;
   std::ofstream file(filename.c_str());
   if(!file.is_open())
   {
      THROW_UNREACHABLE("Something went wrong in file creation");
      return "";
   }
   // Call FloPoCo method to generate VHDL for all common units
   {
      try
      {
         flopoco::Operator::outputVHDLToFile(*common_oplist, file);
      }
      catch(const std::string& s)
      {
         THROW_UNREACHABLE("Exception while generating " + s);
      }
   }
   // Mark the file as generated
   return filename;
}

const std::vector<std::string> flopoco_wrapper::get_ports(const std::string& FU_name_stored, unsigned int ASSERT_PARAMETER(expected_ports), port_type local_type, bool ASSERT_PARAMETER(check_ports)) const
{
   std::vector<std::string> ports;
   flopoco::Operator* op = get_FU(FU_name_stored);
   for(int i = 0; i < op->getIOListSize(); i++)
   {
      flopoco::Signal* sig = op->getIOListSignal(i);
      if(local_type == port_in && sig->type() == flopoco::Signal::in)
         ports.push_back(sig->getName());
      else if(local_type == port_out && sig->type() == flopoco::Signal::out)
         ports.push_back(sig->getName());
   }
   THROW_ASSERT(!check_ports || expected_ports == ports.size(), "Expected a different number of " + (local_type == port_in ? std::string("input") : std::string("output")) + " ports");
   return ports;
}

const std::string flopoco_wrapper::get_port(port_type local_type) const
{
   if(local_type == rst)
      return "rst";
   else if(local_type == clk)
      return "clk";
   THROW_UNREACHABLE("Something went wrong!");
   return "";
}

void flopoco_wrapper::DECODE_BITS(unsigned int FU_prec, unsigned int& n_mant, unsigned int& n_exp)
{
   switch(FU_prec)
   {
      case 16: // IEEE-754 half precision
      {
         n_exp = 5;
         n_mant = 10;
         break;
      }
      case 32: // IEEE-754 single precision
      {
         n_exp = 8;
         n_mant = 23;
         break;
      }
      case 64: // IEEE-754 double precision
      {
         n_exp = 11;
         n_mant = 52;
         break;
      }
      case 96: /// Intel 80 bit extended precision padded with 16bits
      {
         THROW_ERROR("Intel Extended Precision not currently supported");
         n_exp = 15;
         n_mant = 64;
         break;
      }
      case 128: // IEEE-754 quad precision
      {
         n_exp = 15;
         n_mant = 112;
         break;
      }
      default: // Linear growth for exponent number of bits
      {
         n_exp = 8 + (3u * (FU_prec - 32) / 32);
         n_mant = FU_prec - n_exp - 1;
         break;
      }
   }
}
