; ModuleID = './02_vecmul_b.ll'
source_filename = "fused_multiply"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [15 x i8] c"fused_multiply\00", align 1

; Function Attrs: nounwind
define dllexport i32 @fused_multiply(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !12, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i8* %1, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !15
  %3 = bitcast i8* %0 to %0**, !dbg !15
  %4 = load %0*, %0** %3, align 8, !dbg !15
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !15
  %6 = bitcast i8* %5 to %0**, !dbg !15
  %7 = load %0*, %0** %6, align 8, !dbg !15
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !15
  %9 = bitcast i8* %8 to %0**, !dbg !15
  %10 = load %0*, %0** %9, align 8, !dbg !15
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0, !dbg !15
  %12 = load i8*, i8** %11, align 8, !dbg !15
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0, !dbg !15
  %14 = load i8*, i8** %13, align 8, !dbg !15
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0, !dbg !15
  %16 = load i8*, i8** %15, align 8, !dbg !15
  tail call fastcc void @fused_multiply_compute_(i8* %16, i8* %12, i8* %14), !dbg !15
  ret i32 0, !dbg !15
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to <4 x float>*
  %.i0 = bitcast <4 x float>* %3 to float*
  %wide.load.i0 = load float, float* %.i0, align 4, !tbaa !16
  %.i1 = getelementptr float, float* %.i0, i32 1
  %wide.load.i1 = load float, float* %.i1, align 4, !tbaa !16
  %.i2 = getelementptr float, float* %.i0, i32 2
  %wide.load.i2 = load float, float* %.i2, align 4, !tbaa !16
  %.i3 = getelementptr float, float* %.i0, i32 3
  %wide.load.i3 = load float, float* %.i3, align 4, !tbaa !16
  %4 = bitcast i8* %1 to <4 x float>*
  %.i01 = bitcast <4 x float>* %4 to float*
  %wide.load1.i0 = load float, float* %.i01, align 4, !tbaa !20
  %.i12 = getelementptr float, float* %.i01, i32 1
  %wide.load1.i1 = load float, float* %.i12, align 4, !tbaa !20
  %.i23 = getelementptr float, float* %.i01, i32 2
  %wide.load1.i2 = load float, float* %.i23, align 4, !tbaa !20
  %.i34 = getelementptr float, float* %.i01, i32 3
  %wide.load1.i3 = load float, float* %.i34, align 4, !tbaa !20
  %.i05 = fmul float %wide.load.i0, %wide.load1.i0
  %.i16 = fmul float %wide.load.i1, %wide.load1.i1
  %.i27 = fmul float %wide.load.i2, %wide.load1.i2
  %.i38 = fmul float %wide.load.i3, %wide.load1.i3
  %5 = bitcast i8* %0 to <4 x float>*
  %.i09 = bitcast <4 x float>* %5 to float*
  store float %.i05, float* %.i09, align 4, !tbaa !23
  %.i110 = getelementptr float, float* %.i09, i32 1
  store float %.i16, float* %.i110, align 4, !tbaa !23
  %.i211 = getelementptr float, float* %.i09, i32 2
  store float %.i27, float* %.i211, align 4, !tbaa !23
  %.i312 = getelementptr float, float* %.i09, i32 3
  store float %.i38, float* %.i312, align 4, !tbaa !23
  %6 = getelementptr inbounds i8, i8* %2, i64 16
  %7 = bitcast i8* %6 to <4 x float>*
  %.i013 = bitcast <4 x float>* %7 to float*
  %wide.load.1.i0 = load float, float* %.i013, align 4, !tbaa !16
  %.i114 = getelementptr float, float* %.i013, i32 1
  %wide.load.1.i1 = load float, float* %.i114, align 4, !tbaa !16
  %.i215 = getelementptr float, float* %.i013, i32 2
  %wide.load.1.i2 = load float, float* %.i215, align 4, !tbaa !16
  %.i316 = getelementptr float, float* %.i013, i32 3
  %wide.load.1.i3 = load float, float* %.i316, align 4, !tbaa !16
  %8 = getelementptr inbounds i8, i8* %1, i64 16
  %9 = bitcast i8* %8 to <4 x float>*
  %.i017 = bitcast <4 x float>* %9 to float*
  %wide.load1.1.i0 = load float, float* %.i017, align 4, !tbaa !20
  %.i118 = getelementptr float, float* %.i017, i32 1
  %wide.load1.1.i1 = load float, float* %.i118, align 4, !tbaa !20
  %.i219 = getelementptr float, float* %.i017, i32 2
  %wide.load1.1.i2 = load float, float* %.i219, align 4, !tbaa !20
  %.i320 = getelementptr float, float* %.i017, i32 3
  %wide.load1.1.i3 = load float, float* %.i320, align 4, !tbaa !20
  %.i021 = fmul float %wide.load.1.i0, %wide.load1.1.i0
  %.i122 = fmul float %wide.load.1.i1, %wide.load1.1.i1
  %.i223 = fmul float %wide.load.1.i2, %wide.load1.1.i2
  %.i324 = fmul float %wide.load.1.i3, %wide.load1.1.i3
  %10 = getelementptr inbounds i8, i8* %0, i64 16
  %11 = bitcast i8* %10 to <4 x float>*
  %.i025 = bitcast <4 x float>* %11 to float*
  store float %.i021, float* %.i025, align 4, !tbaa !23
  %.i126 = getelementptr float, float* %.i025, i32 1
  store float %.i122, float* %.i126, align 4, !tbaa !23
  %.i227 = getelementptr float, float* %.i025, i32 2
  store float %.i223, float* %.i227, align 4, !tbaa !23
  %.i328 = getelementptr float, float* %.i025, i32 3
  store float %.i324, float* %.i328, align 4, !tbaa !23
  %12 = getelementptr inbounds i8, i8* %2, i64 32
  %13 = bitcast i8* %12 to <4 x float>*
  %.i029 = bitcast <4 x float>* %13 to float*
  %wide.load.2.i0 = load float, float* %.i029, align 4, !tbaa !16
  %.i130 = getelementptr float, float* %.i029, i32 1
  %wide.load.2.i1 = load float, float* %.i130, align 4, !tbaa !16
  %.i231 = getelementptr float, float* %.i029, i32 2
  %wide.load.2.i2 = load float, float* %.i231, align 4, !tbaa !16
  %.i332 = getelementptr float, float* %.i029, i32 3
  %wide.load.2.i3 = load float, float* %.i332, align 4, !tbaa !16
  %14 = getelementptr inbounds i8, i8* %1, i64 32
  %15 = bitcast i8* %14 to <4 x float>*
  %.i033 = bitcast <4 x float>* %15 to float*
  %wide.load1.2.i0 = load float, float* %.i033, align 4, !tbaa !20
  %.i134 = getelementptr float, float* %.i033, i32 1
  %wide.load1.2.i1 = load float, float* %.i134, align 4, !tbaa !20
  %.i235 = getelementptr float, float* %.i033, i32 2
  %wide.load1.2.i2 = load float, float* %.i235, align 4, !tbaa !20
  %.i336 = getelementptr float, float* %.i033, i32 3
  %wide.load1.2.i3 = load float, float* %.i336, align 4, !tbaa !20
  %.i037 = fmul float %wide.load.2.i0, %wide.load1.2.i0
  %.i138 = fmul float %wide.load.2.i1, %wide.load1.2.i1
  %.i239 = fmul float %wide.load.2.i2, %wide.load1.2.i2
  %.i340 = fmul float %wide.load.2.i3, %wide.load1.2.i3
  %16 = getelementptr inbounds i8, i8* %0, i64 32
  %17 = bitcast i8* %16 to <4 x float>*
  %.i041 = bitcast <4 x float>* %17 to float*
  store float %.i037, float* %.i041, align 4, !tbaa !23
  %.i142 = getelementptr float, float* %.i041, i32 1
  store float %.i138, float* %.i142, align 4, !tbaa !23
  %.i243 = getelementptr float, float* %.i041, i32 2
  store float %.i239, float* %.i243, align 4, !tbaa !23
  %.i344 = getelementptr float, float* %.i041, i32 3
  store float %.i340, float* %.i344, align 4, !tbaa !23
  %18 = getelementptr inbounds i8, i8* %2, i64 48
  %19 = bitcast i8* %18 to <4 x float>*
  %.i045 = bitcast <4 x float>* %19 to float*
  %wide.load.3.i0 = load float, float* %.i045, align 4, !tbaa !16
  %.i146 = getelementptr float, float* %.i045, i32 1
  %wide.load.3.i1 = load float, float* %.i146, align 4, !tbaa !16
  %.i247 = getelementptr float, float* %.i045, i32 2
  %wide.load.3.i2 = load float, float* %.i247, align 4, !tbaa !16
  %.i348 = getelementptr float, float* %.i045, i32 3
  %wide.load.3.i3 = load float, float* %.i348, align 4, !tbaa !16
  %20 = getelementptr inbounds i8, i8* %1, i64 48
  %21 = bitcast i8* %20 to <4 x float>*
  %.i049 = bitcast <4 x float>* %21 to float*
  %wide.load1.3.i0 = load float, float* %.i049, align 4, !tbaa !20
  %.i150 = getelementptr float, float* %.i049, i32 1
  %wide.load1.3.i1 = load float, float* %.i150, align 4, !tbaa !20
  %.i251 = getelementptr float, float* %.i049, i32 2
  %wide.load1.3.i2 = load float, float* %.i251, align 4, !tbaa !20
  %.i352 = getelementptr float, float* %.i049, i32 3
  %wide.load1.3.i3 = load float, float* %.i352, align 4, !tbaa !20
  %.i053 = fmul float %wide.load.3.i0, %wide.load1.3.i0
  %.i154 = fmul float %wide.load.3.i1, %wide.load1.3.i1
  %.i255 = fmul float %wide.load.3.i2, %wide.load1.3.i2
  %.i356 = fmul float %wide.load.3.i3, %wide.load1.3.i3
  %22 = getelementptr inbounds i8, i8* %0, i64 48
  %23 = bitcast i8* %22 to <4 x float>*
  %.i057 = bitcast <4 x float>* %23 to float*
  store float %.i053, float* %.i057, align 4, !tbaa !23
  %.i158 = getelementptr float, float* %.i057, i32 1
  store float %.i154, float* %.i158, align 4, !tbaa !23
  %.i259 = getelementptr float, float* %.i057, i32 2
  store float %.i255, float* %.i259, align 4, !tbaa !23
  %.i360 = getelementptr float, float* %.i057, i32 3
  store float %.i356, float* %.i360, align 4, !tbaa !23
  %24 = getelementptr inbounds i8, i8* %2, i64 64
  %25 = bitcast i8* %24 to <4 x float>*
  %.i061 = bitcast <4 x float>* %25 to float*
  %wide.load.4.i0 = load float, float* %.i061, align 4, !tbaa !16
  %.i162 = getelementptr float, float* %.i061, i32 1
  %wide.load.4.i1 = load float, float* %.i162, align 4, !tbaa !16
  %.i263 = getelementptr float, float* %.i061, i32 2
  %wide.load.4.i2 = load float, float* %.i263, align 4, !tbaa !16
  %.i364 = getelementptr float, float* %.i061, i32 3
  %wide.load.4.i3 = load float, float* %.i364, align 4, !tbaa !16
  %26 = getelementptr inbounds i8, i8* %1, i64 64
  %27 = bitcast i8* %26 to <4 x float>*
  %.i065 = bitcast <4 x float>* %27 to float*
  %wide.load1.4.i0 = load float, float* %.i065, align 4, !tbaa !20
  %.i166 = getelementptr float, float* %.i065, i32 1
  %wide.load1.4.i1 = load float, float* %.i166, align 4, !tbaa !20
  %.i267 = getelementptr float, float* %.i065, i32 2
  %wide.load1.4.i2 = load float, float* %.i267, align 4, !tbaa !20
  %.i368 = getelementptr float, float* %.i065, i32 3
  %wide.load1.4.i3 = load float, float* %.i368, align 4, !tbaa !20
  %.i069 = fmul float %wide.load.4.i0, %wide.load1.4.i0
  %.i170 = fmul float %wide.load.4.i1, %wide.load1.4.i1
  %.i271 = fmul float %wide.load.4.i2, %wide.load1.4.i2
  %.i372 = fmul float %wide.load.4.i3, %wide.load1.4.i3
  %28 = getelementptr inbounds i8, i8* %0, i64 64
  %29 = bitcast i8* %28 to <4 x float>*
  %.i073 = bitcast <4 x float>* %29 to float*
  store float %.i069, float* %.i073, align 4, !tbaa !23
  %.i174 = getelementptr float, float* %.i073, i32 1
  store float %.i170, float* %.i174, align 4, !tbaa !23
  %.i275 = getelementptr float, float* %.i073, i32 2
  store float %.i271, float* %.i275, align 4, !tbaa !23
  %.i376 = getelementptr float, float* %.i073, i32 3
  store float %.i372, float* %.i376, align 4, !tbaa !23
  %30 = getelementptr inbounds i8, i8* %2, i64 80
  %31 = bitcast i8* %30 to <4 x float>*
  %.i077 = bitcast <4 x float>* %31 to float*
  %wide.load.5.i0 = load float, float* %.i077, align 4, !tbaa !16
  %.i178 = getelementptr float, float* %.i077, i32 1
  %wide.load.5.i1 = load float, float* %.i178, align 4, !tbaa !16
  %.i279 = getelementptr float, float* %.i077, i32 2
  %wide.load.5.i2 = load float, float* %.i279, align 4, !tbaa !16
  %.i380 = getelementptr float, float* %.i077, i32 3
  %wide.load.5.i3 = load float, float* %.i380, align 4, !tbaa !16
  %32 = getelementptr inbounds i8, i8* %1, i64 80
  %33 = bitcast i8* %32 to <4 x float>*
  %.i081 = bitcast <4 x float>* %33 to float*
  %wide.load1.5.i0 = load float, float* %.i081, align 4, !tbaa !20
  %.i182 = getelementptr float, float* %.i081, i32 1
  %wide.load1.5.i1 = load float, float* %.i182, align 4, !tbaa !20
  %.i283 = getelementptr float, float* %.i081, i32 2
  %wide.load1.5.i2 = load float, float* %.i283, align 4, !tbaa !20
  %.i384 = getelementptr float, float* %.i081, i32 3
  %wide.load1.5.i3 = load float, float* %.i384, align 4, !tbaa !20
  %.i085 = fmul float %wide.load.5.i0, %wide.load1.5.i0
  %.i186 = fmul float %wide.load.5.i1, %wide.load1.5.i1
  %.i287 = fmul float %wide.load.5.i2, %wide.load1.5.i2
  %.i388 = fmul float %wide.load.5.i3, %wide.load1.5.i3
  %34 = getelementptr inbounds i8, i8* %0, i64 80
  %35 = bitcast i8* %34 to <4 x float>*
  %.i089 = bitcast <4 x float>* %35 to float*
  store float %.i085, float* %.i089, align 4, !tbaa !23
  %.i190 = getelementptr float, float* %.i089, i32 1
  store float %.i186, float* %.i190, align 4, !tbaa !23
  %.i291 = getelementptr float, float* %.i089, i32 2
  store float %.i287, float* %.i291, align 4, !tbaa !23
  %.i392 = getelementptr float, float* %.i089, i32 3
  store float %.i388, float* %.i392, align 4, !tbaa !23
  %36 = getelementptr inbounds i8, i8* %2, i64 96
  %37 = bitcast i8* %36 to <4 x float>*
  %.i093 = bitcast <4 x float>* %37 to float*
  %wide.load.6.i0 = load float, float* %.i093, align 4, !tbaa !16
  %.i194 = getelementptr float, float* %.i093, i32 1
  %wide.load.6.i1 = load float, float* %.i194, align 4, !tbaa !16
  %.i295 = getelementptr float, float* %.i093, i32 2
  %wide.load.6.i2 = load float, float* %.i295, align 4, !tbaa !16
  %.i396 = getelementptr float, float* %.i093, i32 3
  %wide.load.6.i3 = load float, float* %.i396, align 4, !tbaa !16
  %38 = getelementptr inbounds i8, i8* %1, i64 96
  %39 = bitcast i8* %38 to <4 x float>*
  %.i097 = bitcast <4 x float>* %39 to float*
  %wide.load1.6.i0 = load float, float* %.i097, align 4, !tbaa !20
  %.i198 = getelementptr float, float* %.i097, i32 1
  %wide.load1.6.i1 = load float, float* %.i198, align 4, !tbaa !20
  %.i299 = getelementptr float, float* %.i097, i32 2
  %wide.load1.6.i2 = load float, float* %.i299, align 4, !tbaa !20
  %.i3100 = getelementptr float, float* %.i097, i32 3
  %wide.load1.6.i3 = load float, float* %.i3100, align 4, !tbaa !20
  %.i0101 = fmul float %wide.load.6.i0, %wide.load1.6.i0
  %.i1102 = fmul float %wide.load.6.i1, %wide.load1.6.i1
  %.i2103 = fmul float %wide.load.6.i2, %wide.load1.6.i2
  %.i3104 = fmul float %wide.load.6.i3, %wide.load1.6.i3
  %40 = getelementptr inbounds i8, i8* %0, i64 96
  %41 = bitcast i8* %40 to <4 x float>*
  %.i0105 = bitcast <4 x float>* %41 to float*
  store float %.i0101, float* %.i0105, align 4, !tbaa !23
  %.i1106 = getelementptr float, float* %.i0105, i32 1
  store float %.i1102, float* %.i1106, align 4, !tbaa !23
  %.i2107 = getelementptr float, float* %.i0105, i32 2
  store float %.i2103, float* %.i2107, align 4, !tbaa !23
  %.i3108 = getelementptr float, float* %.i0105, i32 3
  store float %.i3104, float* %.i3108, align 4, !tbaa !23
  %42 = getelementptr inbounds i8, i8* %2, i64 112
  %43 = bitcast i8* %42 to <4 x float>*
  %.i0109 = bitcast <4 x float>* %43 to float*
  %wide.load.7.i0 = load float, float* %.i0109, align 4, !tbaa !16
  %.i1110 = getelementptr float, float* %.i0109, i32 1
  %wide.load.7.i1 = load float, float* %.i1110, align 4, !tbaa !16
  %.i2111 = getelementptr float, float* %.i0109, i32 2
  %wide.load.7.i2 = load float, float* %.i2111, align 4, !tbaa !16
  %.i3112 = getelementptr float, float* %.i0109, i32 3
  %wide.load.7.i3 = load float, float* %.i3112, align 4, !tbaa !16
  %44 = getelementptr inbounds i8, i8* %1, i64 112
  %45 = bitcast i8* %44 to <4 x float>*
  %.i0113 = bitcast <4 x float>* %45 to float*
  %wide.load1.7.i0 = load float, float* %.i0113, align 4, !tbaa !20
  %.i1114 = getelementptr float, float* %.i0113, i32 1
  %wide.load1.7.i1 = load float, float* %.i1114, align 4, !tbaa !20
  %.i2115 = getelementptr float, float* %.i0113, i32 2
  %wide.load1.7.i2 = load float, float* %.i2115, align 4, !tbaa !20
  %.i3116 = getelementptr float, float* %.i0113, i32 3
  %wide.load1.7.i3 = load float, float* %.i3116, align 4, !tbaa !20
  %.i0117 = fmul float %wide.load.7.i0, %wide.load1.7.i0
  %.i1118 = fmul float %wide.load.7.i1, %wide.load1.7.i1
  %.i2119 = fmul float %wide.load.7.i2, %wide.load1.7.i2
  %.i3120 = fmul float %wide.load.7.i3, %wide.load1.7.i3
  %46 = getelementptr inbounds i8, i8* %0, i64 112
  %47 = bitcast i8* %46 to <4 x float>*
  %.i0121 = bitcast <4 x float>* %47 to float*
  store float %.i0117, float* %.i0121, align 4, !tbaa !23
  %.i1122 = getelementptr float, float* %.i0121, i32 1
  store float %.i1118, float* %.i1122, align 4, !tbaa !23
  %.i2123 = getelementptr float, float* %.i0121, i32 2
  store float %.i2119, float* %.i2123, align 4, !tbaa !23
  %.i3124 = getelementptr float, float* %.i0121, i32 3
  store float %.i3120, float* %.i3124, align 4, !tbaa !23
  %48 = getelementptr inbounds i8, i8* %2, i64 128
  %49 = bitcast i8* %48 to <4 x float>*
  %.i0125 = bitcast <4 x float>* %49 to float*
  %wide.load.8.i0 = load float, float* %.i0125, align 4, !tbaa !16
  %.i1126 = getelementptr float, float* %.i0125, i32 1
  %wide.load.8.i1 = load float, float* %.i1126, align 4, !tbaa !16
  %.i2127 = getelementptr float, float* %.i0125, i32 2
  %wide.load.8.i2 = load float, float* %.i2127, align 4, !tbaa !16
  %.i3128 = getelementptr float, float* %.i0125, i32 3
  %wide.load.8.i3 = load float, float* %.i3128, align 4, !tbaa !16
  %50 = getelementptr inbounds i8, i8* %1, i64 128
  %51 = bitcast i8* %50 to <4 x float>*
  %.i0129 = bitcast <4 x float>* %51 to float*
  %wide.load1.8.i0 = load float, float* %.i0129, align 4, !tbaa !20
  %.i1130 = getelementptr float, float* %.i0129, i32 1
  %wide.load1.8.i1 = load float, float* %.i1130, align 4, !tbaa !20
  %.i2131 = getelementptr float, float* %.i0129, i32 2
  %wide.load1.8.i2 = load float, float* %.i2131, align 4, !tbaa !20
  %.i3132 = getelementptr float, float* %.i0129, i32 3
  %wide.load1.8.i3 = load float, float* %.i3132, align 4, !tbaa !20
  %.i0133 = fmul float %wide.load.8.i0, %wide.load1.8.i0
  %.i1134 = fmul float %wide.load.8.i1, %wide.load1.8.i1
  %.i2135 = fmul float %wide.load.8.i2, %wide.load1.8.i2
  %.i3136 = fmul float %wide.load.8.i3, %wide.load1.8.i3
  %52 = getelementptr inbounds i8, i8* %0, i64 128
  %53 = bitcast i8* %52 to <4 x float>*
  %.i0137 = bitcast <4 x float>* %53 to float*
  store float %.i0133, float* %.i0137, align 4, !tbaa !23
  %.i1138 = getelementptr float, float* %.i0137, i32 1
  store float %.i1134, float* %.i1138, align 4, !tbaa !23
  %.i2139 = getelementptr float, float* %.i0137, i32 2
  store float %.i2135, float* %.i2139, align 4, !tbaa !23
  %.i3140 = getelementptr float, float* %.i0137, i32 3
  store float %.i3136, float* %.i3140, align 4, !tbaa !23
  %54 = getelementptr inbounds i8, i8* %2, i64 144
  %55 = bitcast i8* %54 to <4 x float>*
  %.i0141 = bitcast <4 x float>* %55 to float*
  %wide.load.9.i0 = load float, float* %.i0141, align 4, !tbaa !16
  %.i1142 = getelementptr float, float* %.i0141, i32 1
  %wide.load.9.i1 = load float, float* %.i1142, align 4, !tbaa !16
  %.i2143 = getelementptr float, float* %.i0141, i32 2
  %wide.load.9.i2 = load float, float* %.i2143, align 4, !tbaa !16
  %.i3144 = getelementptr float, float* %.i0141, i32 3
  %wide.load.9.i3 = load float, float* %.i3144, align 4, !tbaa !16
  %56 = getelementptr inbounds i8, i8* %1, i64 144
  %57 = bitcast i8* %56 to <4 x float>*
  %.i0145 = bitcast <4 x float>* %57 to float*
  %wide.load1.9.i0 = load float, float* %.i0145, align 4, !tbaa !20
  %.i1146 = getelementptr float, float* %.i0145, i32 1
  %wide.load1.9.i1 = load float, float* %.i1146, align 4, !tbaa !20
  %.i2147 = getelementptr float, float* %.i0145, i32 2
  %wide.load1.9.i2 = load float, float* %.i2147, align 4, !tbaa !20
  %.i3148 = getelementptr float, float* %.i0145, i32 3
  %wide.load1.9.i3 = load float, float* %.i3148, align 4, !tbaa !20
  %.i0149 = fmul float %wide.load.9.i0, %wide.load1.9.i0
  %.i1150 = fmul float %wide.load.9.i1, %wide.load1.9.i1
  %.i2151 = fmul float %wide.load.9.i2, %wide.load1.9.i2
  %.i3152 = fmul float %wide.load.9.i3, %wide.load1.9.i3
  %58 = getelementptr inbounds i8, i8* %0, i64 144
  %59 = bitcast i8* %58 to <4 x float>*
  %.i0153 = bitcast <4 x float>* %59 to float*
  store float %.i0149, float* %.i0153, align 4, !tbaa !23
  %.i1154 = getelementptr float, float* %.i0153, i32 1
  store float %.i1150, float* %.i1154, align 4, !tbaa !23
  %.i2155 = getelementptr float, float* %.i0153, i32 2
  store float %.i2151, float* %.i2155, align 4, !tbaa !23
  %.i3156 = getelementptr float, float* %.i0153, i32 3
  store float %.i3152, float* %.i3156, align 4, !tbaa !23
  %60 = getelementptr inbounds i8, i8* %2, i64 160
  %61 = bitcast i8* %60 to <4 x float>*
  %.i0157 = bitcast <4 x float>* %61 to float*
  %wide.load.10.i0 = load float, float* %.i0157, align 4, !tbaa !16
  %.i1158 = getelementptr float, float* %.i0157, i32 1
  %wide.load.10.i1 = load float, float* %.i1158, align 4, !tbaa !16
  %.i2159 = getelementptr float, float* %.i0157, i32 2
  %wide.load.10.i2 = load float, float* %.i2159, align 4, !tbaa !16
  %.i3160 = getelementptr float, float* %.i0157, i32 3
  %wide.load.10.i3 = load float, float* %.i3160, align 4, !tbaa !16
  %62 = getelementptr inbounds i8, i8* %1, i64 160
  %63 = bitcast i8* %62 to <4 x float>*
  %.i0161 = bitcast <4 x float>* %63 to float*
  %wide.load1.10.i0 = load float, float* %.i0161, align 4, !tbaa !20
  %.i1162 = getelementptr float, float* %.i0161, i32 1
  %wide.load1.10.i1 = load float, float* %.i1162, align 4, !tbaa !20
  %.i2163 = getelementptr float, float* %.i0161, i32 2
  %wide.load1.10.i2 = load float, float* %.i2163, align 4, !tbaa !20
  %.i3164 = getelementptr float, float* %.i0161, i32 3
  %wide.load1.10.i3 = load float, float* %.i3164, align 4, !tbaa !20
  %.i0165 = fmul float %wide.load.10.i0, %wide.load1.10.i0
  %.i1166 = fmul float %wide.load.10.i1, %wide.load1.10.i1
  %.i2167 = fmul float %wide.load.10.i2, %wide.load1.10.i2
  %.i3168 = fmul float %wide.load.10.i3, %wide.load1.10.i3
  %64 = getelementptr inbounds i8, i8* %0, i64 160
  %65 = bitcast i8* %64 to <4 x float>*
  %.i0169 = bitcast <4 x float>* %65 to float*
  store float %.i0165, float* %.i0169, align 4, !tbaa !23
  %.i1170 = getelementptr float, float* %.i0169, i32 1
  store float %.i1166, float* %.i1170, align 4, !tbaa !23
  %.i2171 = getelementptr float, float* %.i0169, i32 2
  store float %.i2167, float* %.i2171, align 4, !tbaa !23
  %.i3172 = getelementptr float, float* %.i0169, i32 3
  store float %.i3168, float* %.i3172, align 4, !tbaa !23
  %66 = getelementptr inbounds i8, i8* %2, i64 176
  %67 = bitcast i8* %66 to <4 x float>*
  %.i0173 = bitcast <4 x float>* %67 to float*
  %wide.load.11.i0 = load float, float* %.i0173, align 4, !tbaa !16
  %.i1174 = getelementptr float, float* %.i0173, i32 1
  %wide.load.11.i1 = load float, float* %.i1174, align 4, !tbaa !16
  %.i2175 = getelementptr float, float* %.i0173, i32 2
  %wide.load.11.i2 = load float, float* %.i2175, align 4, !tbaa !16
  %.i3176 = getelementptr float, float* %.i0173, i32 3
  %wide.load.11.i3 = load float, float* %.i3176, align 4, !tbaa !16
  %68 = getelementptr inbounds i8, i8* %1, i64 176
  %69 = bitcast i8* %68 to <4 x float>*
  %.i0177 = bitcast <4 x float>* %69 to float*
  %wide.load1.11.i0 = load float, float* %.i0177, align 4, !tbaa !20
  %.i1178 = getelementptr float, float* %.i0177, i32 1
  %wide.load1.11.i1 = load float, float* %.i1178, align 4, !tbaa !20
  %.i2179 = getelementptr float, float* %.i0177, i32 2
  %wide.load1.11.i2 = load float, float* %.i2179, align 4, !tbaa !20
  %.i3180 = getelementptr float, float* %.i0177, i32 3
  %wide.load1.11.i3 = load float, float* %.i3180, align 4, !tbaa !20
  %.i0181 = fmul float %wide.load.11.i0, %wide.load1.11.i0
  %.i1182 = fmul float %wide.load.11.i1, %wide.load1.11.i1
  %.i2183 = fmul float %wide.load.11.i2, %wide.load1.11.i2
  %.i3184 = fmul float %wide.load.11.i3, %wide.load1.11.i3
  %70 = getelementptr inbounds i8, i8* %0, i64 176
  %71 = bitcast i8* %70 to <4 x float>*
  %.i0185 = bitcast <4 x float>* %71 to float*
  store float %.i0181, float* %.i0185, align 4, !tbaa !23
  %.i1186 = getelementptr float, float* %.i0185, i32 1
  store float %.i1182, float* %.i1186, align 4, !tbaa !23
  %.i2187 = getelementptr float, float* %.i0185, i32 2
  store float %.i2183, float* %.i2187, align 4, !tbaa !23
  %.i3188 = getelementptr float, float* %.i0185, i32 3
  store float %.i3184, float* %.i3188, align 4, !tbaa !23
  %72 = getelementptr inbounds i8, i8* %2, i64 192
  %73 = bitcast i8* %72 to <4 x float>*
  %.i0189 = bitcast <4 x float>* %73 to float*
  %wide.load.12.i0 = load float, float* %.i0189, align 4, !tbaa !16
  %.i1190 = getelementptr float, float* %.i0189, i32 1
  %wide.load.12.i1 = load float, float* %.i1190, align 4, !tbaa !16
  %.i2191 = getelementptr float, float* %.i0189, i32 2
  %wide.load.12.i2 = load float, float* %.i2191, align 4, !tbaa !16
  %.i3192 = getelementptr float, float* %.i0189, i32 3
  %wide.load.12.i3 = load float, float* %.i3192, align 4, !tbaa !16
  %74 = getelementptr inbounds i8, i8* %1, i64 192
  %75 = bitcast i8* %74 to <4 x float>*
  %.i0193 = bitcast <4 x float>* %75 to float*
  %wide.load1.12.i0 = load float, float* %.i0193, align 4, !tbaa !20
  %.i1194 = getelementptr float, float* %.i0193, i32 1
  %wide.load1.12.i1 = load float, float* %.i1194, align 4, !tbaa !20
  %.i2195 = getelementptr float, float* %.i0193, i32 2
  %wide.load1.12.i2 = load float, float* %.i2195, align 4, !tbaa !20
  %.i3196 = getelementptr float, float* %.i0193, i32 3
  %wide.load1.12.i3 = load float, float* %.i3196, align 4, !tbaa !20
  %.i0197 = fmul float %wide.load.12.i0, %wide.load1.12.i0
  %.i1198 = fmul float %wide.load.12.i1, %wide.load1.12.i1
  %.i2199 = fmul float %wide.load.12.i2, %wide.load1.12.i2
  %.i3200 = fmul float %wide.load.12.i3, %wide.load1.12.i3
  %76 = getelementptr inbounds i8, i8* %0, i64 192
  %77 = bitcast i8* %76 to <4 x float>*
  %.i0201 = bitcast <4 x float>* %77 to float*
  store float %.i0197, float* %.i0201, align 4, !tbaa !23
  %.i1202 = getelementptr float, float* %.i0201, i32 1
  store float %.i1198, float* %.i1202, align 4, !tbaa !23
  %.i2203 = getelementptr float, float* %.i0201, i32 2
  store float %.i2199, float* %.i2203, align 4, !tbaa !23
  %.i3204 = getelementptr float, float* %.i0201, i32 3
  store float %.i3200, float* %.i3204, align 4, !tbaa !23
  %78 = getelementptr inbounds i8, i8* %2, i64 208
  %79 = bitcast i8* %78 to <4 x float>*
  %.i0205 = bitcast <4 x float>* %79 to float*
  %wide.load.13.i0 = load float, float* %.i0205, align 4, !tbaa !16
  %.i1206 = getelementptr float, float* %.i0205, i32 1
  %wide.load.13.i1 = load float, float* %.i1206, align 4, !tbaa !16
  %.i2207 = getelementptr float, float* %.i0205, i32 2
  %wide.load.13.i2 = load float, float* %.i2207, align 4, !tbaa !16
  %.i3208 = getelementptr float, float* %.i0205, i32 3
  %wide.load.13.i3 = load float, float* %.i3208, align 4, !tbaa !16
  %80 = getelementptr inbounds i8, i8* %1, i64 208
  %81 = bitcast i8* %80 to <4 x float>*
  %.i0209 = bitcast <4 x float>* %81 to float*
  %wide.load1.13.i0 = load float, float* %.i0209, align 4, !tbaa !20
  %.i1210 = getelementptr float, float* %.i0209, i32 1
  %wide.load1.13.i1 = load float, float* %.i1210, align 4, !tbaa !20
  %.i2211 = getelementptr float, float* %.i0209, i32 2
  %wide.load1.13.i2 = load float, float* %.i2211, align 4, !tbaa !20
  %.i3212 = getelementptr float, float* %.i0209, i32 3
  %wide.load1.13.i3 = load float, float* %.i3212, align 4, !tbaa !20
  %.i0213 = fmul float %wide.load.13.i0, %wide.load1.13.i0
  %.i1214 = fmul float %wide.load.13.i1, %wide.load1.13.i1
  %.i2215 = fmul float %wide.load.13.i2, %wide.load1.13.i2
  %.i3216 = fmul float %wide.load.13.i3, %wide.load1.13.i3
  %82 = getelementptr inbounds i8, i8* %0, i64 208
  %83 = bitcast i8* %82 to <4 x float>*
  %.i0217 = bitcast <4 x float>* %83 to float*
  store float %.i0213, float* %.i0217, align 4, !tbaa !23
  %.i1218 = getelementptr float, float* %.i0217, i32 1
  store float %.i1214, float* %.i1218, align 4, !tbaa !23
  %.i2219 = getelementptr float, float* %.i0217, i32 2
  store float %.i2215, float* %.i2219, align 4, !tbaa !23
  %.i3220 = getelementptr float, float* %.i0217, i32 3
  store float %.i3216, float* %.i3220, align 4, !tbaa !23
  %84 = getelementptr inbounds i8, i8* %2, i64 224
  %85 = bitcast i8* %84 to <4 x float>*
  %.i0221 = bitcast <4 x float>* %85 to float*
  %wide.load.14.i0 = load float, float* %.i0221, align 4, !tbaa !16
  %.i1222 = getelementptr float, float* %.i0221, i32 1
  %wide.load.14.i1 = load float, float* %.i1222, align 4, !tbaa !16
  %.i2223 = getelementptr float, float* %.i0221, i32 2
  %wide.load.14.i2 = load float, float* %.i2223, align 4, !tbaa !16
  %.i3224 = getelementptr float, float* %.i0221, i32 3
  %wide.load.14.i3 = load float, float* %.i3224, align 4, !tbaa !16
  %86 = getelementptr inbounds i8, i8* %1, i64 224
  %87 = bitcast i8* %86 to <4 x float>*
  %.i0225 = bitcast <4 x float>* %87 to float*
  %wide.load1.14.i0 = load float, float* %.i0225, align 4, !tbaa !20
  %.i1226 = getelementptr float, float* %.i0225, i32 1
  %wide.load1.14.i1 = load float, float* %.i1226, align 4, !tbaa !20
  %.i2227 = getelementptr float, float* %.i0225, i32 2
  %wide.load1.14.i2 = load float, float* %.i2227, align 4, !tbaa !20
  %.i3228 = getelementptr float, float* %.i0225, i32 3
  %wide.load1.14.i3 = load float, float* %.i3228, align 4, !tbaa !20
  %.i0229 = fmul float %wide.load.14.i0, %wide.load1.14.i0
  %.i1230 = fmul float %wide.load.14.i1, %wide.load1.14.i1
  %.i2231 = fmul float %wide.load.14.i2, %wide.load1.14.i2
  %.i3232 = fmul float %wide.load.14.i3, %wide.load1.14.i3
  %88 = getelementptr inbounds i8, i8* %0, i64 224
  %89 = bitcast i8* %88 to <4 x float>*
  %.i0233 = bitcast <4 x float>* %89 to float*
  store float %.i0229, float* %.i0233, align 4, !tbaa !23
  %.i1234 = getelementptr float, float* %.i0233, i32 1
  store float %.i1230, float* %.i1234, align 4, !tbaa !23
  %.i2235 = getelementptr float, float* %.i0233, i32 2
  store float %.i2231, float* %.i2235, align 4, !tbaa !23
  %.i3236 = getelementptr float, float* %.i0233, i32 3
  store float %.i3232, float* %.i3236, align 4, !tbaa !23
  %90 = getelementptr inbounds i8, i8* %2, i64 240
  %91 = bitcast i8* %90 to <4 x float>*
  %.i0237 = bitcast <4 x float>* %91 to float*
  %wide.load.15.i0 = load float, float* %.i0237, align 4, !tbaa !16
  %.i1238 = getelementptr float, float* %.i0237, i32 1
  %wide.load.15.i1 = load float, float* %.i1238, align 4, !tbaa !16
  %.i2239 = getelementptr float, float* %.i0237, i32 2
  %wide.load.15.i2 = load float, float* %.i2239, align 4, !tbaa !16
  %.i3240 = getelementptr float, float* %.i0237, i32 3
  %wide.load.15.i3 = load float, float* %.i3240, align 4, !tbaa !16
  %92 = getelementptr inbounds i8, i8* %1, i64 240
  %93 = bitcast i8* %92 to <4 x float>*
  %.i0241 = bitcast <4 x float>* %93 to float*
  %wide.load1.15.i0 = load float, float* %.i0241, align 4, !tbaa !20
  %.i1242 = getelementptr float, float* %.i0241, i32 1
  %wide.load1.15.i1 = load float, float* %.i1242, align 4, !tbaa !20
  %.i2243 = getelementptr float, float* %.i0241, i32 2
  %wide.load1.15.i2 = load float, float* %.i2243, align 4, !tbaa !20
  %.i3244 = getelementptr float, float* %.i0241, i32 3
  %wide.load1.15.i3 = load float, float* %.i3244, align 4, !tbaa !20
  %.i0245 = fmul float %wide.load.15.i0, %wide.load1.15.i0
  %.i1246 = fmul float %wide.load.15.i1, %wide.load1.15.i1
  %.i2247 = fmul float %wide.load.15.i2, %wide.load1.15.i2
  %.i3248 = fmul float %wide.load.15.i3, %wide.load1.15.i3
  %94 = getelementptr inbounds i8, i8* %0, i64 240
  %95 = bitcast i8* %94 to <4 x float>*
  %.i0249 = bitcast <4 x float>* %95 to float*
  store float %.i0245, float* %.i0249, align 4, !tbaa !23
  %.i1250 = getelementptr float, float* %.i0249, i32 1
  store float %.i1246, float* %.i1250, align 4, !tbaa !23
  %.i2251 = getelementptr float, float* %.i0249, i32 2
  store float %.i2247, float* %.i2251, align 4, !tbaa !23
  %.i3252 = getelementptr float, float* %.i0249, i32 3
  store float %.i3248, float* %.i3252, align 4, !tbaa !23
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { nounwind }
attributes #1 = { noinline norecurse nounwind }
attributes #2 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_multiply", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!18 = !{!"0x55a5ae552220", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x55a5ae551080", !19, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x55a5ae551cf0", !19, i64 0}
