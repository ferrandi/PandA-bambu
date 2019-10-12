; ModuleID = 'fused_nn_softmax'
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
  %3 = load <4 x float>, <4 x float>* %2, align 4, !tbaa !16
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
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !16
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
  %32 = shufflevector <4 x float> %31, <4 x float> undef, <4 x i32> zeroinitializer
  %33 = fsub <4 x float> %3, %32
  %34 = call <4 x float> @llvm.exp.v4f32(<4 x float> %33)
  %35 = extractelement <4 x float> %34, i32 0
  %36 = fadd float %35, 0.000000e+00
  %37 = extractelement <4 x float> %34, i32 1
  %38 = fadd float %36, %37
  %39 = extractelement <4 x float> %34, i32 2
  %40 = fadd float %38, %39
  %41 = extractelement <4 x float> %34, i32 3
  %42 = fadd float %40, %41
  %43 = fsub <4 x float> %18, %32
  %44 = call <4 x float> @llvm.exp.v4f32(<4 x float> %43)
  %45 = extractelement <4 x float> %44, i32 0
  %46 = fadd float %42, %45
  %47 = extractelement <4 x float> %44, i32 1
  %48 = fadd float %46, %47
  %49 = extractelement <4 x float> %44, i32 2
  %50 = fadd float %48, %49
  %51 = extractelement <4 x float> %44, i32 3
  %52 = fadd float %50, %51
  %53 = insertelement <4 x float> undef, float %52, i32 0
  %54 = shufflevector <4 x float> %53, <4 x float> undef, <4 x i32> zeroinitializer
  %55 = fdiv <4 x float> %34, %54
  %56 = bitcast i8* %1 to <4 x float>*
  store <4 x float> %55, <4 x float>* %56, align 4, !tbaa !20
  %57 = getelementptr inbounds i8, i8* %1, i64 16
  %58 = fdiv <4 x float> %44, %54
  %59 = bitcast i8* %57 to <4 x float>*
  store <4 x float> %58, <4 x float>* %59, align 4, !tbaa !20
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #2

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
