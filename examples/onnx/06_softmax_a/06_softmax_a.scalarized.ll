; ModuleID = './06_softmax_a.ll'
source_filename = "fused_nn_softmax"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [17 x i8] c"fused_nn_softmax\00", align 1

; Function Attrs: nounwind
define dllexport i32 @fused_nn_softmax(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 !dbg !5 {
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
  tail call fastcc void @fused_nn_softmax_compute_(i8* %9, i8* %11), !dbg !15
  ret i32 0, !dbg !15
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_softmax_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to <4 x float>*
  %.i0 = bitcast <4 x float>* %2 to float*
  %.i01 = load float, float* %.i0, align 4, !tbaa !16
  %.i1 = getelementptr float, float* %.i0, i32 1
  %.i12 = load float, float* %.i1, align 4, !tbaa !16
  %.i2 = getelementptr float, float* %.i0, i32 2
  %.i23 = load float, float* %.i2, align 4, !tbaa !16
  %.i3 = getelementptr float, float* %.i0, i32 3
  %.i34 = load float, float* %.i3, align 4, !tbaa !16
  %.upto0 = insertelement <4 x float> undef, float %.i01, i32 0
  %.upto1 = insertelement <4 x float> %.upto0, float %.i12, i32 1
  %.upto2 = insertelement <4 x float> %.upto1, float %.i23, i32 2
  %3 = insertelement <4 x float> %.upto2, float %.i34, i32 3
  %4 = extractelement <4 x float> %3, i32 0
  %5 = fcmp olt float %4, 0xC7EFFFFFE0000000
  %6 = select i1 %5, float 0xC7EFFFFFE0000000, float %4
  %7 = extractelement <4 x float> %3, i32 1
  %8 = fcmp ogt float %6, %7
  %9 = select i1 %8, float %6, float %7
  %10 = extractelement <4 x float> %3, i32 2
  %11 = fcmp ogt float %9, %10
  %12 = select i1 %11, float %9, float %10
  %13 = extractelement <4 x float> %3, i32 3
  %14 = fcmp ogt float %12, %13
  %15 = select i1 %14, float %12, float %13
  %16 = getelementptr inbounds i8, i8* %0, i64 16
  %17 = bitcast i8* %16 to <4 x float>*
  %.i05 = bitcast <4 x float>* %17 to float*
  %.i06 = load float, float* %.i05, align 4, !tbaa !16
  %.i17 = getelementptr float, float* %.i05, i32 1
  %.i18 = load float, float* %.i17, align 4, !tbaa !16
  %.i29 = getelementptr float, float* %.i05, i32 2
  %.i210 = load float, float* %.i29, align 4, !tbaa !16
  %.i311 = getelementptr float, float* %.i05, i32 3
  %.i312 = load float, float* %.i311, align 4, !tbaa !16
  %.upto045 = insertelement <4 x float> undef, float %.i06, i32 0
  %.upto146 = insertelement <4 x float> %.upto045, float %.i18, i32 1
  %.upto247 = insertelement <4 x float> %.upto146, float %.i210, i32 2
  %18 = insertelement <4 x float> %.upto247, float %.i312, i32 3
  %19 = extractelement <4 x float> %18, i32 0
  %20 = fcmp ogt float %15, %19
  %21 = select i1 %20, float %15, float %19
  %22 = extractelement <4 x float> %18, i32 1
  %23 = fcmp ogt float %21, %22
  %24 = select i1 %23, float %21, float %22
  %25 = extractelement <4 x float> %18, i32 2
  %26 = fcmp ogt float %24, %25
  %27 = select i1 %26, float %24, float %25
  %28 = extractelement <4 x float> %18, i32 3
  %29 = fcmp ogt float %27, %28
  %30 = select i1 %29, float %27, float %28
  %31 = insertelement <4 x float> undef, float %30, i32 0
  %.i013 = fsub float %.i01, %30
  %.i114 = fsub float %.i12, %30
  %.i215 = fsub float %.i23, %30
  %.i316 = fsub float %.i34, %30
  %.i017 = call float @llvm.exp.f32(float %.i013)
  %.i118 = call float @llvm.exp.f32(float %.i114)
  %.i219 = call float @llvm.exp.f32(float %.i215)
  %.i320 = call float @llvm.exp.f32(float %.i316)
  %.upto048 = insertelement <4 x float> undef, float %.i017, i32 0
  %.upto149 = insertelement <4 x float> %.upto048, float %.i118, i32 1
  %.upto250 = insertelement <4 x float> %.upto149, float %.i219, i32 2
  %32 = insertelement <4 x float> %.upto250, float %.i320, i32 3
  %33 = extractelement <4 x float> %32, i32 0
  %34 = fadd float %33, 0.000000e+00
  %35 = extractelement <4 x float> %32, i32 1
  %36 = fadd float %34, %35
  %37 = extractelement <4 x float> %32, i32 2
  %38 = fadd float %36, %37
  %39 = extractelement <4 x float> %32, i32 3
  %40 = fadd float %38, %39
  %.i021 = fsub float %.i06, %30
  %.i122 = fsub float %.i18, %30
  %.i223 = fsub float %.i210, %30
  %.i324 = fsub float %.i312, %30
  %.i025 = call float @llvm.exp.f32(float %.i021)
  %.i126 = call float @llvm.exp.f32(float %.i122)
  %.i227 = call float @llvm.exp.f32(float %.i223)
  %.i328 = call float @llvm.exp.f32(float %.i324)
  %.upto051 = insertelement <4 x float> undef, float %.i025, i32 0
  %.upto152 = insertelement <4 x float> %.upto051, float %.i126, i32 1
  %.upto253 = insertelement <4 x float> %.upto152, float %.i227, i32 2
  %41 = insertelement <4 x float> %.upto253, float %.i328, i32 3
  %42 = extractelement <4 x float> %41, i32 0
  %43 = fadd float %40, %42
  %44 = extractelement <4 x float> %41, i32 1
  %45 = fadd float %43, %44
  %46 = extractelement <4 x float> %41, i32 2
  %47 = fadd float %45, %46
  %48 = extractelement <4 x float> %41, i32 3
  %49 = fadd float %47, %48
  %50 = insertelement <4 x float> undef, float %49, i32 0
  %.i029 = fdiv float %.i017, %49
  %.i130 = fdiv float %.i118, %49
  %.i231 = fdiv float %.i219, %49
  %.i332 = fdiv float %.i320, %49
  %51 = bitcast i8* %1 to <4 x float>*
  %.i033 = bitcast <4 x float>* %51 to float*
  store float %.i029, float* %.i033, align 4, !tbaa !20
  %.i134 = getelementptr float, float* %.i033, i32 1
  store float %.i130, float* %.i134, align 4, !tbaa !20
  %.i235 = getelementptr float, float* %.i033, i32 2
  store float %.i231, float* %.i235, align 4, !tbaa !20
  %.i336 = getelementptr float, float* %.i033, i32 3
  store float %.i332, float* %.i336, align 4, !tbaa !20
  %52 = getelementptr inbounds i8, i8* %1, i64 16
  %.i037 = fdiv float %.i025, %49
  %.i138 = fdiv float %.i126, %49
  %.i239 = fdiv float %.i227, %49
  %.i340 = fdiv float %.i328, %49
  %53 = bitcast i8* %52 to <4 x float>*
  %.i041 = bitcast <4 x float>* %53 to float*
  store float %.i037, float* %.i041, align 4, !tbaa !20
  %.i142 = getelementptr float, float* %.i041, i32 1
  store float %.i138, float* %.i142, align 4, !tbaa !20
  %.i243 = getelementptr float, float* %.i041, i32 2
  store float %.i239, float* %.i243, align 4, !tbaa !20
  %.i344 = getelementptr float, float* %.i041, i32 3
  store float %.i340, float* %.i344, align 4, !tbaa !20
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #2

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
!5 = distinct !DISubprogram(name: "fused_nn_softmax", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!18 = !{!"0x562edde7e740", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x562edde8a640", !19, i64 0}
