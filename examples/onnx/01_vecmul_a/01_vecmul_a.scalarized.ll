; ModuleID = './01_vecmul_a.ll'
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
  %.i01 = load float, float* %.i0, align 4, !tbaa !16
  %.i1 = getelementptr float, float* %.i0, i32 1
  %.i12 = load float, float* %.i1, align 4, !tbaa !16
  %.i2 = getelementptr float, float* %.i0, i32 2
  %.i23 = load float, float* %.i2, align 4, !tbaa !16
  %.i3 = getelementptr float, float* %.i0, i32 3
  %.i34 = load float, float* %.i3, align 4, !tbaa !16
  %4 = bitcast i8* %1 to <4 x float>*
  %.i05 = bitcast <4 x float>* %4 to float*
  %.i06 = load float, float* %.i05, align 4, !tbaa !20
  %.i17 = getelementptr float, float* %.i05, i32 1
  %.i18 = load float, float* %.i17, align 4, !tbaa !20
  %.i29 = getelementptr float, float* %.i05, i32 2
  %.i210 = load float, float* %.i29, align 4, !tbaa !20
  %.i311 = getelementptr float, float* %.i05, i32 3
  %.i312 = load float, float* %.i311, align 4, !tbaa !20
  %.i013 = fmul float %.i01, %.i06
  %.i114 = fmul float %.i12, %.i18
  %.i215 = fmul float %.i23, %.i210
  %.i316 = fmul float %.i34, %.i312
  %5 = bitcast i8* %0 to <4 x float>*
  %.i017 = bitcast <4 x float>* %5 to float*
  store float %.i013, float* %.i017, align 4, !tbaa !23
  %.i118 = getelementptr float, float* %.i017, i32 1
  store float %.i114, float* %.i118, align 4, !tbaa !23
  %.i219 = getelementptr float, float* %.i017, i32 2
  store float %.i215, float* %.i219, align 4, !tbaa !23
  %.i320 = getelementptr float, float* %.i017, i32 3
  store float %.i316, float* %.i320, align 4, !tbaa !23
  %6 = getelementptr inbounds i8, i8* %2, i64 16
  %7 = getelementptr inbounds i8, i8* %1, i64 16
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %6 to <4 x float>*
  %.i021 = bitcast <4 x float>* %9 to float*
  %.i022 = load float, float* %.i021, align 4, !tbaa !16
  %.i123 = getelementptr float, float* %.i021, i32 1
  %.i124 = load float, float* %.i123, align 4, !tbaa !16
  %.i225 = getelementptr float, float* %.i021, i32 2
  %.i226 = load float, float* %.i225, align 4, !tbaa !16
  %.i327 = getelementptr float, float* %.i021, i32 3
  %.i328 = load float, float* %.i327, align 4, !tbaa !16
  %10 = bitcast i8* %7 to <4 x float>*
  %.i029 = bitcast <4 x float>* %10 to float*
  %.i030 = load float, float* %.i029, align 4, !tbaa !20
  %.i131 = getelementptr float, float* %.i029, i32 1
  %.i132 = load float, float* %.i131, align 4, !tbaa !20
  %.i233 = getelementptr float, float* %.i029, i32 2
  %.i234 = load float, float* %.i233, align 4, !tbaa !20
  %.i335 = getelementptr float, float* %.i029, i32 3
  %.i336 = load float, float* %.i335, align 4, !tbaa !20
  %.i037 = fmul float %.i022, %.i030
  %.i138 = fmul float %.i124, %.i132
  %.i239 = fmul float %.i226, %.i234
  %.i340 = fmul float %.i328, %.i336
  %11 = bitcast i8* %8 to <4 x float>*
  %.i041 = bitcast <4 x float>* %11 to float*
  store float %.i037, float* %.i041, align 4, !tbaa !23
  %.i142 = getelementptr float, float* %.i041, i32 1
  store float %.i138, float* %.i142, align 4, !tbaa !23
  %.i243 = getelementptr float, float* %.i041, i32 2
  store float %.i239, float* %.i243, align 4, !tbaa !23
  %.i344 = getelementptr float, float* %.i041, i32 3
  store float %.i340, float* %.i344, align 4, !tbaa !23
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
!18 = !{!"0x562d8b4b2b70", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x562d8b4b2940", !19, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x562d8b4b3a40", !19, i64 0}
