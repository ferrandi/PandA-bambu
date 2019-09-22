; ModuleID = './15_thxprlsg.ll'
source_filename = "fused_tanh_exp_nn_relu_sigmoid"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [31 x i8] c"fused_tanh_exp_nn_relu_sigmoid\00", align 1

; Function Attrs: nounwind
define dllexport i32 @fused_tanh_exp_nn_relu_sigmoid(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !12, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i8* %1, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !15
  %3 = bitcast i8* %0 to %0**, !dbg !15
  %4 = load %0*, %0** %3, align 8, !dbg !15
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !15
  %6 = bitcast i8* %5 to %0**, !dbg !15
  %7 = load %0*, %0** %6, align 8, !dbg !15
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0, !dbg !15
  %9 = load i8*, i8** %8, align 8, !dbg !15
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0, !dbg !15
  %11 = load i8*, i8** %10, align 8, !dbg !15
  tail call fastcc void @fused_tanh_exp_nn_relu_sigmoid_compute_(i8* %11, i8* %9), !dbg !15
  ret i32 0, !dbg !15
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_tanh_exp_nn_relu_sigmoid_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = load float, float* %2, align 4, !tbaa !16
  %4 = fcmp olt float %3, 9.000000e+00
  %5 = select i1 %4, float %3, float 9.000000e+00
  %6 = getelementptr inbounds i8, i8* %1, i64 4
  %7 = bitcast i8* %6 to float*
  %8 = load float, float* %7, align 4, !tbaa !16
  %9 = fcmp olt float %8, 9.000000e+00
  %10 = select i1 %9, float %8, float 9.000000e+00
  %11 = getelementptr inbounds i8, i8* %1, i64 8
  %12 = bitcast i8* %11 to float*
  %13 = load float, float* %12, align 4, !tbaa !16
  %14 = fcmp olt float %13, 9.000000e+00
  %15 = select i1 %14, float %13, float 9.000000e+00
  %16 = getelementptr inbounds i8, i8* %1, i64 12
  %17 = bitcast i8* %16 to float*
  %18 = load float, float* %17, align 4, !tbaa !16
  %19 = fcmp olt float %18, 9.000000e+00
  %20 = select i1 %19, float %18, float 9.000000e+00
  %21 = insertelement <4 x float> undef, float %5, i32 0
  %22 = insertelement <4 x float> %21, float %10, i32 1
  %23 = insertelement <4 x float> %22, float %15, i32 2
  %24 = insertelement <4 x float> %23, float %20, i32 3
  %.i0 = fcmp ogt float %5, -9.000000e+00
  %.i1 = fcmp ogt float %10, -9.000000e+00
  %.i2 = fcmp ogt float %15, -9.000000e+00
  %.i3 = fcmp ogt float %20, -9.000000e+00
  %.i01 = select i1 %.i0, float %5, float -9.000000e+00
  %.i12 = select i1 %.i1, float %10, float -9.000000e+00
  %.i23 = select i1 %.i2, float %15, float -9.000000e+00
  %.i34 = select i1 %.i3, float %20, float -9.000000e+00
  %.i05 = fmul float %.i01, %.i01
  %.i16 = fmul float %.i12, %.i12
  %.i27 = fmul float %.i23, %.i23
  %.i38 = fmul float %.i34, %.i34
  %.i09 = call float @llvm.fmuladd.f32(float %.i05, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i110 = call float @llvm.fmuladd.f32(float %.i16, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i211 = call float @llvm.fmuladd.f32(float %.i27, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i312 = call float @llvm.fmuladd.f32(float %.i38, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i013 = call float @llvm.fmuladd.f32(float %.i05, float %.i09, float 0xBDD7A6FFE0000000)
  %.i114 = call float @llvm.fmuladd.f32(float %.i16, float %.i110, float 0xBDD7A6FFE0000000)
  %.i215 = call float @llvm.fmuladd.f32(float %.i27, float %.i211, float 0xBDD7A6FFE0000000)
  %.i316 = call float @llvm.fmuladd.f32(float %.i38, float %.i312, float 0xBDD7A6FFE0000000)
  %.i017 = call float @llvm.fmuladd.f32(float %.i05, float %.i013, float 0x3E6B800820000000)
  %.i118 = call float @llvm.fmuladd.f32(float %.i16, float %.i114, float 0x3E6B800820000000)
  %.i219 = call float @llvm.fmuladd.f32(float %.i27, float %.i215, float 0x3E6B800820000000)
  %.i320 = call float @llvm.fmuladd.f32(float %.i38, float %.i316, float 0x3E6B800820000000)
  %.i021 = call float @llvm.fmuladd.f32(float %.i05, float %.i017, float 0x3EEF286940000000)
  %.i122 = call float @llvm.fmuladd.f32(float %.i16, float %.i118, float 0x3EEF286940000000)
  %.i223 = call float @llvm.fmuladd.f32(float %.i27, float %.i219, float 0x3EEF286940000000)
  %.i324 = call float @llvm.fmuladd.f32(float %.i38, float %.i320, float 0x3EEF286940000000)
  %.i025 = call float @llvm.fmuladd.f32(float %.i05, float %.i021, float 0x3F44E1BDA0000000)
  %.i126 = call float @llvm.fmuladd.f32(float %.i16, float %.i122, float 0x3F44E1BDA0000000)
  %.i227 = call float @llvm.fmuladd.f32(float %.i27, float %.i223, float 0x3F44E1BDA0000000)
  %.i328 = call float @llvm.fmuladd.f32(float %.i38, float %.i324, float 0x3F44E1BDA0000000)
  %.i029 = call float @llvm.fmuladd.f32(float %.i05, float %.i025, float 0x3F740B3B80000000)
  %.i130 = call float @llvm.fmuladd.f32(float %.i16, float %.i126, float 0x3F740B3B80000000)
  %.i231 = call float @llvm.fmuladd.f32(float %.i27, float %.i227, float 0x3F740B3B80000000)
  %.i332 = call float @llvm.fmuladd.f32(float %.i38, float %.i328, float 0x3F740B3B80000000)
  %.i033 = fmul float %.i01, %.i029
  %.i134 = fmul float %.i12, %.i130
  %.i235 = fmul float %.i23, %.i231
  %.i336 = fmul float %.i34, %.i332
  %.i037 = call float @llvm.fmuladd.f32(float %.i05, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i138 = call float @llvm.fmuladd.f32(float %.i16, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i239 = call float @llvm.fmuladd.f32(float %.i27, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i340 = call float @llvm.fmuladd.f32(float %.i38, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i041 = call float @llvm.fmuladd.f32(float %.i05, float %.i037, float 0x3F629540A0000000)
  %.i142 = call float @llvm.fmuladd.f32(float %.i16, float %.i138, float 0x3F629540A0000000)
  %.i243 = call float @llvm.fmuladd.f32(float %.i27, float %.i239, float 0x3F629540A0000000)
  %.i344 = call float @llvm.fmuladd.f32(float %.i38, float %.i340, float 0x3F629540A0000000)
  %.i045 = call float @llvm.fmuladd.f32(float %.i05, float %.i041, float 0x3F740B3BA0000000)
  %.i146 = call float @llvm.fmuladd.f32(float %.i16, float %.i142, float 0x3F740B3BA0000000)
  %.i247 = call float @llvm.fmuladd.f32(float %.i27, float %.i243, float 0x3F740B3BA0000000)
  %.i348 = call float @llvm.fmuladd.f32(float %.i38, float %.i344, float 0x3F740B3BA0000000)
  %.i049 = fdiv float %.i033, %.i045
  %.i150 = fdiv float %.i134, %.i146
  %.i251 = fdiv float %.i235, %.i247
  %.i352 = fdiv float %.i336, %.i348
  %.i053 = call float @llvm.exp.f32(float %.i049)
  %.i154 = call float @llvm.exp.f32(float %.i150)
  %.i255 = call float @llvm.exp.f32(float %.i251)
  %.i356 = call float @llvm.exp.f32(float %.i352)
  %.i057 = fcmp ogt float %.i053, 0.000000e+00
  %.i158 = fcmp ogt float %.i154, 0.000000e+00
  %.i259 = fcmp ogt float %.i255, 0.000000e+00
  %.i360 = fcmp ogt float %.i356, 0.000000e+00
  %.i061 = select i1 %.i057, float %.i053, float 0.000000e+00
  %.i162 = select i1 %.i158, float %.i154, float 0.000000e+00
  %.i263 = select i1 %.i259, float %.i255, float 0.000000e+00
  %.i364 = select i1 %.i360, float %.i356, float 0.000000e+00
  %.i065 = fsub float 0.000000e+00, %.i061
  %.i166 = fsub float 0.000000e+00, %.i162
  %.i267 = fsub float 0.000000e+00, %.i263
  %.i368 = fsub float 0.000000e+00, %.i364
  %.i069 = call float @llvm.exp.f32(float %.i065)
  %.i170 = call float @llvm.exp.f32(float %.i166)
  %.i271 = call float @llvm.exp.f32(float %.i267)
  %.i372 = call float @llvm.exp.f32(float %.i368)
  %.i073 = fadd float %.i069, 1.000000e+00
  %.i174 = fadd float %.i170, 1.000000e+00
  %.i275 = fadd float %.i271, 1.000000e+00
  %.i376 = fadd float %.i372, 1.000000e+00
  %.i077 = fdiv float 1.000000e+00, %.i073
  %.i178 = fdiv float 1.000000e+00, %.i174
  %.i279 = fdiv float 1.000000e+00, %.i275
  %.i380 = fdiv float 1.000000e+00, %.i376
  %25 = bitcast i8* %0 to <4 x float>*
  %.i081 = bitcast <4 x float>* %25 to float*
  store float %.i077, float* %.i081, align 4, !tbaa !20
  %.i182 = getelementptr float, float* %.i081, i32 1
  store float %.i178, float* %.i182, align 4, !tbaa !20
  %.i283 = getelementptr float, float* %.i081, i32 2
  store float %.i279, float* %.i283, align 4, !tbaa !20
  %.i384 = getelementptr float, float* %.i081, i32 3
  store float %.i380, float* %.i384, align 4, !tbaa !20
  %26 = getelementptr inbounds i8, i8* %1, i64 16
  %27 = bitcast i8* %26 to float*
  %28 = load float, float* %27, align 4, !tbaa !16
  %29 = fcmp olt float %28, 9.000000e+00
  %30 = select i1 %29, float %28, float 9.000000e+00
  %31 = getelementptr inbounds i8, i8* %1, i64 20
  %32 = bitcast i8* %31 to float*
  %33 = load float, float* %32, align 4, !tbaa !16
  %34 = fcmp olt float %33, 9.000000e+00
  %35 = select i1 %34, float %33, float 9.000000e+00
  %36 = getelementptr inbounds i8, i8* %1, i64 24
  %37 = bitcast i8* %36 to float*
  %38 = load float, float* %37, align 4, !tbaa !16
  %39 = fcmp olt float %38, 9.000000e+00
  %40 = select i1 %39, float %38, float 9.000000e+00
  %41 = getelementptr inbounds i8, i8* %1, i64 28
  %42 = bitcast i8* %41 to float*
  %43 = load float, float* %42, align 4, !tbaa !16
  %44 = fcmp olt float %43, 9.000000e+00
  %45 = select i1 %44, float %43, float 9.000000e+00
  %46 = insertelement <4 x float> undef, float %30, i32 0
  %47 = insertelement <4 x float> %46, float %35, i32 1
  %48 = insertelement <4 x float> %47, float %40, i32 2
  %49 = insertelement <4 x float> %48, float %45, i32 3
  %.i085 = fcmp ogt float %30, -9.000000e+00
  %.i186 = fcmp ogt float %35, -9.000000e+00
  %.i287 = fcmp ogt float %40, -9.000000e+00
  %.i388 = fcmp ogt float %45, -9.000000e+00
  %.i089 = select i1 %.i085, float %30, float -9.000000e+00
  %.i190 = select i1 %.i186, float %35, float -9.000000e+00
  %.i291 = select i1 %.i287, float %40, float -9.000000e+00
  %.i392 = select i1 %.i388, float %45, float -9.000000e+00
  %.i093 = fmul float %.i089, %.i089
  %.i194 = fmul float %.i190, %.i190
  %.i295 = fmul float %.i291, %.i291
  %.i396 = fmul float %.i392, %.i392
  %50 = getelementptr inbounds i8, i8* %0, i64 16
  %.i097 = call float @llvm.fmuladd.f32(float %.i093, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i198 = call float @llvm.fmuladd.f32(float %.i194, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i299 = call float @llvm.fmuladd.f32(float %.i295, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i3100 = call float @llvm.fmuladd.f32(float %.i396, float 0xBCB3E4B800000000, float 0x3D4C266FC0000000)
  %.i0101 = call float @llvm.fmuladd.f32(float %.i093, float %.i097, float 0xBDD7A6FFE0000000)
  %.i1102 = call float @llvm.fmuladd.f32(float %.i194, float %.i198, float 0xBDD7A6FFE0000000)
  %.i2103 = call float @llvm.fmuladd.f32(float %.i295, float %.i299, float 0xBDD7A6FFE0000000)
  %.i3104 = call float @llvm.fmuladd.f32(float %.i396, float %.i3100, float 0xBDD7A6FFE0000000)
  %.i0105 = call float @llvm.fmuladd.f32(float %.i093, float %.i0101, float 0x3E6B800820000000)
  %.i1106 = call float @llvm.fmuladd.f32(float %.i194, float %.i1102, float 0x3E6B800820000000)
  %.i2107 = call float @llvm.fmuladd.f32(float %.i295, float %.i2103, float 0x3E6B800820000000)
  %.i3108 = call float @llvm.fmuladd.f32(float %.i396, float %.i3104, float 0x3E6B800820000000)
  %.i0109 = call float @llvm.fmuladd.f32(float %.i093, float %.i0105, float 0x3EEF286940000000)
  %.i1110 = call float @llvm.fmuladd.f32(float %.i194, float %.i1106, float 0x3EEF286940000000)
  %.i2111 = call float @llvm.fmuladd.f32(float %.i295, float %.i2107, float 0x3EEF286940000000)
  %.i3112 = call float @llvm.fmuladd.f32(float %.i396, float %.i3108, float 0x3EEF286940000000)
  %.i0113 = call float @llvm.fmuladd.f32(float %.i093, float %.i0109, float 0x3F44E1BDA0000000)
  %.i1114 = call float @llvm.fmuladd.f32(float %.i194, float %.i1110, float 0x3F44E1BDA0000000)
  %.i2115 = call float @llvm.fmuladd.f32(float %.i295, float %.i2111, float 0x3F44E1BDA0000000)
  %.i3116 = call float @llvm.fmuladd.f32(float %.i396, float %.i3112, float 0x3F44E1BDA0000000)
  %.i0117 = call float @llvm.fmuladd.f32(float %.i093, float %.i0113, float 0x3F740B3B80000000)
  %.i1118 = call float @llvm.fmuladd.f32(float %.i194, float %.i1114, float 0x3F740B3B80000000)
  %.i2119 = call float @llvm.fmuladd.f32(float %.i295, float %.i2115, float 0x3F740B3B80000000)
  %.i3120 = call float @llvm.fmuladd.f32(float %.i396, float %.i3116, float 0x3F740B3B80000000)
  %.i0121 = fmul float %.i089, %.i0117
  %.i1122 = fmul float %.i190, %.i1118
  %.i2123 = fmul float %.i291, %.i2119
  %.i3124 = fmul float %.i392, %.i3120
  %.i0125 = call float @llvm.fmuladd.f32(float %.i093, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i1126 = call float @llvm.fmuladd.f32(float %.i194, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i2127 = call float @llvm.fmuladd.f32(float %.i295, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i3128 = call float @llvm.fmuladd.f32(float %.i396, float 0x3EB41A7B00000000, float 0x3F1F12BAC0000000)
  %.i0129 = call float @llvm.fmuladd.f32(float %.i093, float %.i0125, float 0x3F629540A0000000)
  %.i1130 = call float @llvm.fmuladd.f32(float %.i194, float %.i1126, float 0x3F629540A0000000)
  %.i2131 = call float @llvm.fmuladd.f32(float %.i295, float %.i2127, float 0x3F629540A0000000)
  %.i3132 = call float @llvm.fmuladd.f32(float %.i396, float %.i3128, float 0x3F629540A0000000)
  %.i0133 = call float @llvm.fmuladd.f32(float %.i093, float %.i0129, float 0x3F740B3BA0000000)
  %.i1134 = call float @llvm.fmuladd.f32(float %.i194, float %.i1130, float 0x3F740B3BA0000000)
  %.i2135 = call float @llvm.fmuladd.f32(float %.i295, float %.i2131, float 0x3F740B3BA0000000)
  %.i3136 = call float @llvm.fmuladd.f32(float %.i396, float %.i3132, float 0x3F740B3BA0000000)
  %.i0137 = fdiv float %.i0121, %.i0133
  %.i1138 = fdiv float %.i1122, %.i1134
  %.i2139 = fdiv float %.i2123, %.i2135
  %.i3140 = fdiv float %.i3124, %.i3136
  %.i0141 = call float @llvm.exp.f32(float %.i0137)
  %.i1142 = call float @llvm.exp.f32(float %.i1138)
  %.i2143 = call float @llvm.exp.f32(float %.i2139)
  %.i3144 = call float @llvm.exp.f32(float %.i3140)
  %.i0145 = fcmp ogt float %.i0141, 0.000000e+00
  %.i1146 = fcmp ogt float %.i1142, 0.000000e+00
  %.i2147 = fcmp ogt float %.i2143, 0.000000e+00
  %.i3148 = fcmp ogt float %.i3144, 0.000000e+00
  %.i0149 = select i1 %.i0145, float %.i0141, float 0.000000e+00
  %.i1150 = select i1 %.i1146, float %.i1142, float 0.000000e+00
  %.i2151 = select i1 %.i2147, float %.i2143, float 0.000000e+00
  %.i3152 = select i1 %.i3148, float %.i3144, float 0.000000e+00
  %.i0153 = fsub float 0.000000e+00, %.i0149
  %.i1154 = fsub float 0.000000e+00, %.i1150
  %.i2155 = fsub float 0.000000e+00, %.i2151
  %.i3156 = fsub float 0.000000e+00, %.i3152
  %.i0157 = call float @llvm.exp.f32(float %.i0153)
  %.i1158 = call float @llvm.exp.f32(float %.i1154)
  %.i2159 = call float @llvm.exp.f32(float %.i2155)
  %.i3160 = call float @llvm.exp.f32(float %.i3156)
  %.i0161 = fadd float %.i0157, 1.000000e+00
  %.i1162 = fadd float %.i1158, 1.000000e+00
  %.i2163 = fadd float %.i2159, 1.000000e+00
  %.i3164 = fadd float %.i3160, 1.000000e+00
  %.i0165 = fdiv float 1.000000e+00, %.i0161
  %.i1166 = fdiv float 1.000000e+00, %.i1162
  %.i2167 = fdiv float 1.000000e+00, %.i2163
  %.i3168 = fdiv float 1.000000e+00, %.i3164
  %51 = bitcast i8* %50 to <4 x float>*
  %.i0169 = bitcast <4 x float>* %51 to float*
  store float %.i0165, float* %.i0169, align 4, !tbaa !20
  %.i1170 = getelementptr float, float* %.i0169, i32 1
  store float %.i1166, float* %.i1170, align 4, !tbaa !20
  %.i2171 = getelementptr float, float* %.i0169, i32 2
  store float %.i2167, float* %.i2171, align 4, !tbaa !20
  %.i3172 = getelementptr float, float* %.i0169, i32 3
  store float %.i3168, float* %.i3172, align 4, !tbaa !20
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #2

; Function Attrs: nounwind readnone speculatable
declare float @llvm.fmuladd.f32(float, float, float) #2

; Function Attrs: nounwind readnone speculatable
declare float @llvm.exp.f32(float) #2

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind }
attributes #2 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_tanh_exp_nn_relu_sigmoid", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
!6 = !DISubroutineType(types: !7)
!7 = !{!8, !9, !9, !8}
!8 = !DIBasicType(name: "int32", size: 32, encoding: DW_ATE_signed)
!9 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10)
!10 = !DIBasicType(name: "int8", size: 8, encoding: DW_ATE_signed)
!11 = !{!12, !13, !14}
!12 = !DILocalVariable(name: "arg1", arg: 1, scope: !5, file: !1, type: !9)
!13 = !DILocalVariable(name: "arg2", arg: 2, scope: !5, file: !1, type: !9)
!14 = !DILocalVariable(name: "arg3", arg: 3, scope: !5, file: !1, type: !8)
!15 = !DILocation(line: 0, scope: !5)
!16 = !{!17, !17, i64 0}
!17 = !{!"float32", !18, i64 0}
!18 = !{!"0x55cd2b695550", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x55cd2b69dbb0", !19, i64 0}
