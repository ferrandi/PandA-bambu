; ModuleID = 'fused_multiply'
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
  %4 = load <4 x float>, <4 x float>* %3, align 4, !tbaa !16
  %5 = bitcast i8* %1 to <4 x float>*
  %6 = load <4 x float>, <4 x float>* %5, align 4, !tbaa !20
  %7 = fmul <4 x float> %4, %6
  %8 = bitcast i8* %0 to <4 x float>*
  store <4 x float> %7, <4 x float>* %8, align 4, !tbaa !23
  %9 = getelementptr inbounds i8, i8* %2, i64 16
  %10 = getelementptr inbounds i8, i8* %1, i64 16
  %11 = getelementptr inbounds i8, i8* %0, i64 16
  %12 = bitcast i8* %9 to <4 x float>*
  %13 = load <4 x float>, <4 x float>* %12, align 4, !tbaa !16
  %14 = bitcast i8* %10 to <4 x float>*
  %15 = load <4 x float>, <4 x float>* %14, align 4, !tbaa !20
  %16 = fmul <4 x float> %13, %15
  %17 = bitcast i8* %11 to <4 x float>*
  store <4 x float> %16, <4 x float>* %17, align 4, !tbaa !23
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
