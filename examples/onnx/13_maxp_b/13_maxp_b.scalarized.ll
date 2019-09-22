; ModuleID = './13_maxp_b.ll'
source_filename = "fused_nn_max_pool2d"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [20 x i8] c"fused_nn_max_pool2d\00", align 1

; Function Attrs: nounwind
define dllexport i32 @fused_nn_max_pool2d(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 !dbg !5 {
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
  tail call fastcc void @fused_nn_max_pool2d_compute_(i8* %11, i8* %9), !dbg !15
  ret i32 0, !dbg !15
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_body

for_body:                                         ; preds = %for_end3, %entry
  %indvars.iv3 = phi i64 [ 0, %entry ], [ %indvars.iv.next4, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv3, 10
  %5 = mul nuw nsw i64 %indvars.iv3, 96
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %6 = add nuw nsw i64 %indvars.iv, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = mul nuw nsw i64 %indvars.iv, 3
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %3, i64 %9
  %11 = load float, float* %10, align 4, !tbaa !16
  %12 = fcmp olt float %11, 0xC7EFFFFFE0000000
  %13 = select i1 %12, float 0xC7EFFFFFE0000000, float %11
  %14 = add nuw nsw i64 %9, 1
  %15 = getelementptr inbounds float, float* %3, i64 %14
  %16 = load float, float* %15, align 4, !tbaa !16
  %17 = fcmp ogt float %13, %16
  %18 = select i1 %17, float %13, float %16
  %19 = add nuw nsw i64 %9, 2
  %20 = getelementptr inbounds float, float* %3, i64 %19
  %21 = load float, float* %20, align 4, !tbaa !16
  %22 = fcmp ogt float %18, %21
  %23 = select i1 %22, float %18, float %21
  %24 = add nuw nsw i64 %9, 32
  %25 = getelementptr inbounds float, float* %3, i64 %24
  %26 = load float, float* %25, align 4, !tbaa !16
  %27 = fcmp ogt float %23, %26
  %28 = select i1 %27, float %23, float %26
  %29 = add nuw nsw i64 %9, 33
  %30 = getelementptr inbounds float, float* %3, i64 %29
  %31 = load float, float* %30, align 4, !tbaa !16
  %32 = fcmp ogt float %28, %31
  %33 = select i1 %32, float %28, float %31
  %34 = add nuw nsw i64 %9, 34
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = load float, float* %35, align 4, !tbaa !16
  %37 = fcmp ogt float %33, %36
  %38 = select i1 %37, float %33, float %36
  %39 = add nuw nsw i64 %9, 64
  %40 = getelementptr inbounds float, float* %3, i64 %39
  %41 = load float, float* %40, align 4, !tbaa !16
  %42 = fcmp ogt float %38, %41
  %43 = select i1 %42, float %38, float %41
  %44 = add nuw nsw i64 %9, 65
  %45 = getelementptr inbounds float, float* %3, i64 %44
  %46 = load float, float* %45, align 4, !tbaa !16
  %47 = fcmp ogt float %43, %46
  %48 = select i1 %47, float %43, float %46
  %49 = add nuw nsw i64 %9, 66
  %50 = getelementptr inbounds float, float* %3, i64 %49
  %51 = load float, float* %50, align 4, !tbaa !16
  %52 = fcmp ogt float %48, %51
  %53 = select i1 %52, float %48, float %51
  store float %53, float* %7, align 4, !tbaa !20
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 10
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !23

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 10
  br i1 %exitcond5, label %for_end, label %for_body, !prof !23
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
!5 = distinct !DISubprogram(name: "fused_nn_max_pool2d", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!18 = !{!"0x5626be098730", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x5626be0986e0", !19, i64 0}
!23 = !{!"branch_weights", i32 1, i32 1048576}
