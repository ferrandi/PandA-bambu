# Makefile for flopoco/src
SUBDIRS = 
DIST_SUBDIRS =

EXTRA_DIST = VHDLLexerRule.l FPExpressions/ExpressionScanner.l FPExpressions/ExpressionParser.y $(top_srcdir)/src/FPExpressions/ExpressionParser.h \
                $(top_srcdir)/src/FPExpressions/FlopocoScanner.h
ACLOCAL_AMFLAGS = -I ../m4

BUILT_SOURCES = VHDLLexer.cpp \
                ExpressionParser.cpp \
                ExpressionScanner.cpp
                
CLEANFILES = $(BUILT_SOURCES) lex.yy.c *.output

noinst_LTLIBRARIES = libflopoco.la

noinst_HEADERS = \
          LexerContext.hpp \
          FloPoCo.hpp \
          Operator.hpp \
          Signal.hpp \
          utils.hpp \
          FlopocoStream.hpp \
          FPPipeline.hpp \
          Tools/ResourceEstimationHelper.hpp \
          Tools/FloorplanningHelper.hpp \
          FPExpressions/ExpressionParserData.h \
          sollya.h \
          UtilSollya.hh \
          apps/Collision.hpp \
          apps/FPFMAcc.hpp \
          apps/FPJacobi.hpp \
          ConstMult/IntIntKCM.hpp \
          ConstMult/CRFPConstMult.hpp \
          ConstMult/FixRealKCM.hpp \
          ConstMult/FPRealKCM.hpp \
          ConstMult/FPConstMult.hpp \
          ConstMult/IntConstMult.hpp \
          ConstMult/KCMTable.hpp \
          ConstMult/ShiftAddDag.hpp \
          ConstMult/ShiftAddOp.hpp \
          IntConstDiv.hpp \
          FPConstDiv.hpp \
          FPPow.hpp \
          FPLog/FirstInvTable.hpp \
          FPLog/FirstLogTable.hpp \
          FPLog/OtherLogTable.hpp \
          LNS/AtanPow.hpp \
          LNS/Cotran.hpp \
          LNS/CotranHybrid.hpp \
          LNS/CotranTables.hpp \
          LNS/LNSAdd.hpp \
          LNS/LNSAddSub.hpp \
          LNS/LNSDiv.hpp \
          LNS/LNSMul.hpp \
          LNS/LNSSqrt.hpp \
          LNS/LogSinCos.hpp \
          squareroot/PolynomialTableCorrectRounded.hpp \
          squareroot/PolynomialTable.hpp \
          Targets/DSP.hpp \
          Targets/CycloneII.hpp \
          Targets/CycloneIII.hpp \
          Targets/CycloneIV.hpp \
          Targets/CycloneV.hpp \
          Targets/StratixII.hpp \
          Targets/StratixIII.hpp \
          Targets/StratixIV.hpp \
          Targets/StratixV.hpp \
          Targets/Spartan3.hpp \
          Targets/Virtex4.hpp \
          Targets/Virtex5.hpp \
          Targets/Virtex6.hpp \
          DotProduct.hpp \
          DualTable.hpp \
          DualTable2.hpp \
          Fix2FP.hpp \
          FP2Fix.hpp \
          FPAdderDualPath.hpp \
          FPAdderSinglePath.hpp \
          FPAdder3Input.hpp \
          FPDiv.hpp \
          FPExp.hpp \
          FPLog.hpp \
          FPMultiplier.hpp \
          FPMultiplierKaratsuba.hpp \
          FPNumber.hpp \
          FPSqrt.hpp \
          FPSqrtPoly.hpp \
          FPSquarer.hpp \
          IEEENumber.hpp \
          InputIEEE.hpp \
          IntAdder.hpp \
          IntAddition/IntAdderClassical.hpp \
          IntAddition/IntAdderAlternative.hpp \
          IntAddition/IntAdderShortLatency.hpp \
          IntAddition/LongIntAdderAddAddMuxGen1.hpp \
          IntAddition/LongIntAdderCmpCmpAddGen1.hpp \
          IntAddition/LongIntAdderCmpAddIncGen1.hpp \
          IntAddition/IntAdderSpecific.hpp \
          IntAddition/IntComparatorSpecific.hpp \
          IntAddition/CarryGenerationCircuit.hpp \
          IntAddition/LongIntAdderAddAddMuxGen2.hpp \
          IntAddition/LongIntAdderCmpCmpAddGen2.hpp \
          IntAddition/LongIntAdderCmpAddIncGen2.hpp \
          IntAddition/LongIntAdderMuxNetwork.hpp \
          IntMultiAdder.hpp \
          IntAddition/IntNAdder.hpp \
          IntAddition/IntCompressorTree.hpp \
          IntAddition/PopCount.hpp \
          IntAddition/NewCompressorTree.hpp \
          IntAddition/BasicCompressor.hpp \
          BitHeap.hpp \
          WeightedBit.hpp \
          IntComparator.hpp \
          IntDualSub.hpp \
          Plotter.hpp \
          IntMultiplier.hpp \
          FixMultAdd.hpp \
          IntMultipliers/IntKaratsuba.hpp \
          IntSquarer.hpp \
          IntMultipliers/FormalBinaryProduct.hpp \
          IntMultipliers/GenericBinaryPolynomial.hpp \
          IntMultipliers/IntPower.hpp \
          IntMultipliers/MultiplierBlock.hpp \
          IntMultipliers/FixSinPoly.hpp \
          IntMultipliers/FixXPow3Div6.hpp \
          ConstMult/IntConstDiv3.hpp \
          LongAcc2FP.hpp \
          LongAcc.hpp LZOC.hpp LZOCShifterSticky.hpp\
          OutputIEEE.hpp \
          Shifters.hpp \
          FixFunctions/Function.hpp \
          FixFunctions/PiecewiseFunction.hpp \
          FixFunctions/FunctionTable.hpp \
          FixFunctions/PolyCoeffTable.hpp \
          FixFunctions/PolynomialEvaluator.hpp \
          FixFunctions/FunctionEvaluator.hpp \
          FixFunctions/HOTBM.hpp \
          FixFunctions/HOTBM/Component.hh \
          FixFunctions/HOTBM/HOTBMInstance.hh\
          FixFunctions/HOTBM/Param.hh\
          FixFunctions/HOTBM/Exhaustive.hh\
          FixFunctions/HOTBM/Util.hh\
          FixFunctions/HOTBM/PWPolynomial.hh\
          FixFunctions/HOTBM/Polynomial.hh\
          FixFunctions/HOTBM/MPPolynomial.hh\
          FixFunctions/HOTBM/Power.hh\
          FixFunctions/HOTBM/PowerAdHoc.hh\
          FixFunctions/HOTBM/TermROM.hh\
          FixFunctions/HOTBM/Term.hh\
          FixFunctions/HOTBM/Minimax.hh\
          FixFunctions/HOTBM/PowerROM.hh\
          FixFunctions/HOTBM/TermPowMult.hh \
          FixFunctions/GenericEvaluator.hpp \
          Complex/FixedComplexAdder.hpp \
          Complex/FixedComplexMultiplier.hpp \
          Table.hpp \
          Target.hpp \
          TestBench.hpp \
          TestCase.hpp \
          UserDefinedOperator.hpp \
          Wrapper.hpp \
          FPSumOfSquares.hpp \
          FixSinCos/FixSinCos.hpp \
          FixSinCos/FixSinOrCos.hpp \
          FixSinCos/CordicSinCos.hpp \
          FixSinCos/CordicAtan2.hpp\
          FixFIR.hpp \
          FixDCT.hpp \
          FixHalfSine.hpp \
          FixRCF.hpp \
          FixRRCF.hpp \
          FPAddSub.hpp \
          random/urng/LutSrRng.hpp

libflopoco_la_SOURCES = \
                        Operator.cpp \
                        Signal.cpp \
                        utils.cpp \
                        FlopocoStream.cpp \
                        VHDLLexer.cpp \
                        Tools/ResourceEstimationHelper.cpp \
                        Tools/FloorplanningHelper.cpp \
                        Targets/DSP.cpp \
                        Target.cpp \
                        Targets/CycloneII.cpp \
                        Targets/CycloneIII.cpp \
                        Targets/CycloneIV.cpp \
                        Targets/CycloneV.cpp \
                        Targets/StratixII.cpp \
                        Targets/StratixIII.cpp \
                        Targets/StratixIV.cpp \
                        Targets/StratixV.cpp \
                        Targets/Spartan3.cpp \
                        Targets/Virtex4.cpp \
                        Targets/Virtex5.cpp \
                        Targets/Virtex6.cpp \
                        Table.cpp \
                        DualTable.cpp \
                        DualTable2.cpp \
                        FPExpressions/ExpressionParserData.cpp \
                        ExpressionParser.cpp \
                        ExpressionScanner.cpp \
                        FPPipeline.cpp \
                        Wrapper.cpp \
                        TestBench.cpp \
                        UserDefinedOperator.cpp\
                        IntAdder.cpp\
                        IntAddition/IntAdderClassical.cpp\
                        IntAddition/IntAdderAlternative.cpp\
                        IntAddition/IntAdderShortLatency.cpp\
                        IntAddition/LongIntAdderAddAddMuxGen1.cpp \
                        IntAddition/LongIntAdderCmpCmpAddGen1.cpp \
                        IntAddition/LongIntAdderCmpAddIncGen1.cpp \
                        IntAddition/IntAdderSpecific.cpp \
                        IntAddition/IntComparatorSpecific.cpp \
                        IntAddition/CarryGenerationCircuit.cpp \
                        IntAddition/LongIntAdderAddAddMuxGen2.cpp \
                        IntAddition/LongIntAdderCmpCmpAddGen2.cpp \
                        IntAddition/LongIntAdderCmpAddIncGen2.cpp \
                        IntAddition/LongIntAdderMuxNetwork.cpp \
                        IntMultiAdder.cpp \
                        IntAddition/IntNAdder.cpp \
                        IntAddition/IntCompressorTree.cpp \
                        IntAddition/PopCount.cpp \
                        IntAddition/NewCompressorTree.cpp \
                        IntAddition/BasicCompressor.cpp \
                        BitHeap.cpp \
                        WeightedBit.cpp \
                        IntComparator.cpp \
                        IntDualSub.cpp \
                        Plotter.cpp \
                        IntMultiplier.cpp \
			FixMultAdd.cpp \
                        IntMultipliers/IntKaratsuba.cpp \
                        IntSquarer.cpp \
                        IntMultipliers/FormalBinaryProduct.cpp \
                        IntMultipliers/GenericBinaryPolynomial.cpp \
                        IntMultipliers/IntPower.cpp \
                        IntMultipliers/MultiplierBlock.cpp \
                        IntMultipliers/FixSinPoly.cpp \
                        IntMultipliers/FixXPow3Div6.cpp \
                        ConstMult/IntConstDiv3.cpp \
                        LZOC.cpp \
                        LZOCShifterSticky.cpp \
                        Shifters.cpp \
                        FixFunctions/Function.cpp \
                        FixFunctions/PiecewiseFunction.cpp \
                        FixFunctions/FunctionTable.cpp \
                        FixFunctions/PolyCoeffTable.cpp \
                        FixFunctions/PolynomialEvaluator.cpp \
                        FixFunctions/FunctionEvaluator.cpp \
                        FixFunctions/HOTBM.cpp \
                        FixFunctions/HOTBM/HOTBMInstance.cc\
                        FixFunctions/HOTBM/Param.cc\
                        FixFunctions/HOTBM/Exhaustive.cc\
                        FixFunctions/HOTBM/Util.cc\
                        FixFunctions/HOTBM/PWPolynomial.cc\
                        FixFunctions/HOTBM/Polynomial.cc\
                        FixFunctions/HOTBM/MPPolynomial.cc\
                        FixFunctions/HOTBM/Power.cc\
                        FixFunctions/HOTBM/PowerAdHoc.cc\
                        FixFunctions/HOTBM/TermROM.cc\
                        FixFunctions/HOTBM/Term.cc\
                        FixFunctions/HOTBM/Minimax.cc\
                        FixFunctions/HOTBM/PowerROM.cc\
                        FixFunctions/HOTBM/TermPowMult.cc \
                        FixSinCos/FixSinCos.cpp \
                        FixSinCos/FixSinOrCos.cpp \
                        FixSinCos/CordicSinCos.cpp \
                        FixSinCos/CordicAtan2.cpp\
                        FixFIR.cpp \
                        FixDCT.cpp \
                        FixHalfSine.cpp \
                        FixRCF.cpp \
                        FixRRCF.cpp \
                        FPAdderDualPath.cpp \
                        FPAdderSinglePath.cpp \
                        FPAdder3Input.cpp \
                        FPAddSub.cpp \
                        FPMultiplier.cpp \
                        FPMultiplierKaratsuba.cpp \
                        FPSquarer.cpp \
                        FPDiv.cpp \
                        FPSqrt.cpp \
                        FPSqrtPoly.cpp \
                        squareroot/PolynomialTable.cpp \
                        squareroot/PolynomialTableCorrectRounded.cpp \
                        DotProduct.cpp \
                        ConstMult/IntConstMult.cpp \
                        ConstMult/ShiftAddOp.cpp \
                        ConstMult/ShiftAddDag.cpp \
                        ConstMult/IntIntKCM.cpp \
                        ConstMult/KCMTable.cpp \
                        ConstMult/FPConstMult.cpp \
                        ConstMult/CRFPConstMult.cpp \
                        ConstMult/FixRealKCM.cpp \
                        ConstMult/FPRealKCM.cpp \
                        IntConstDiv.cpp\
                        FPConstDiv.cpp\
                        FPLog/FirstInvTable.cpp \
                        FPLog/FirstLogTable.cpp \
                        FPLog/OtherLogTable.cpp \
                        FPLog.cpp \
                        FPExp.cpp \
                        FPPow.cpp \
                        LongAcc.cpp \
                        LongAcc2FP.cpp \
                        FPSumOfSquares.cpp \
                        Fix2FP.cpp \
                        InputIEEE.cpp \
                        OutputIEEE.cpp \
                        LNS/LNSAdd.cpp \
                        LNS/Cotran.cpp \
                        LNS/CotranHybrid.cpp \
                        LNS/CotranTables.cpp \
                        LNS/LNSAddSub.cpp \
                        LNS/LNSMul.cpp \
                        LNS/LNSDiv.cpp \
                        LNS/LNSSqrt.cpp \
                        LNS/AtanPow.cpp \
                        FixFunctions/GenericEvaluator.cpp \
                        Complex/FixedComplexAdder.cpp \
                        Complex/FixedComplexMultiplier.cpp \
                        LNS/LogSinCos.cpp \
                        apps/Collision.cpp \
                        apps/FPFMAcc.cpp \
                        apps/FPJacobi.cpp \
                        TestCase.cpp \
                        FPNumber.cpp \
                        IEEENumber.cpp \
                        UtilSollya.cc \
                        FP2Fix.cpp \
                        random/random_main.cpp \
                        random/urng/LutSrRng.cpp

libflopoco_la_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/FPExpressions 

VHDLLexer.cpp:$(top_srcdir)/src/VHDLLexerRule.l
	${LEX} -o VHDLLexer.cpp $(top_srcdir)/src/VHDLLexerRule.l

VHDLLexer.hpp:VHDLLexer.cpp

ExpressionParser.cpp: $(top_srcdir)/src/FPExpressions/ExpressionParser.y ExpressionScanner.cpp
	${YACC} -y -d --defines=$(top_srcdir)/src/FPExpressions/ExpressionParser.h --output=ExpressionParser.cpp --name-prefix=FlopocoExpression $(top_srcdir)/src/FPExpressions/ExpressionParser.y

ExpressionScanner.cpp:$(top_srcdir)/src/FPExpressions/ExpressionScanner.l
	${LEX} --prefix=FlopocoExpression --header-file=$(top_srcdir)/src/FPExpressions/FlopocoScanner.h -o ExpressionScanner.cpp  $(top_srcdir)/src/FPExpressions/ExpressionScanner.l

$(top_srcdir)/src/FPExpressions/FlopocoScanner.h:ExpressionScanner.cpp ExpressionParser.cpp

$(top_srcdir)/src/FPExpressions/ExpressionParser.h:ExpressionParser.cpp
