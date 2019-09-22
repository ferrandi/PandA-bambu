; ModuleID = 'fused_tanh_exp_nn_relu_sigmoid'
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
  %25 = fcmp ogt <4 x float> %24, <float -9.000000e+00, float -9.000000e+00, float -9.000000e+00, float -9.000000e+00>
  %26 = select <4 x i1> %25, <4 x float> %24, <4 x float> <float -9.000000e+00, float -9.000000e+00, float -9.000000e+00, float -9.000000e+00>
  %27 = fmul <4 x float> %26, %26
  %28 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> <float 0xBCB3E4B800000000, float 0xBCB3E4B800000000, float 0xBCB3E4B800000000, float 0xBCB3E4B800000000>, <4 x float> <float 0x3D4C266FC0000000, float 0x3D4C266FC0000000, float 0x3D4C266FC0000000, float 0x3D4C266FC0000000>)
  %29 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %28, <4 x float> <float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000>)
  %30 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %29, <4 x float> <float 0x3E6B800820000000, float 0x3E6B800820000000, float 0x3E6B800820000000, float 0x3E6B800820000000>)
  %31 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %30, <4 x float> <float 0x3EEF286940000000, float 0x3EEF286940000000, float 0x3EEF286940000000, float 0x3EEF286940000000>)
  %32 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %31, <4 x float> <float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000>)
  %33 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %32, <4 x float> <float 0x3F740B3B80000000, float 0x3F740B3B80000000, float 0x3F740B3B80000000, float 0x3F740B3B80000000>)
  %34 = fmul <4 x float> %26, %33
  %35 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> <float 0x3EB41A7B00000000, float 0x3EB41A7B00000000, float 0x3EB41A7B00000000, float 0x3EB41A7B00000000>, <4 x float> <float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000>)
  %36 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %35, <4 x float> <float 0x3F629540A0000000, float 0x3F629540A0000000, float 0x3F629540A0000000, float 0x3F629540A0000000>)
  %37 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %27, <4 x float> %36, <4 x float> <float 0x3F740B3BA0000000, float 0x3F740B3BA0000000, float 0x3F740B3BA0000000, float 0x3F740B3BA0000000>)
  %38 = fdiv <4 x float> %34, %37
  %39 = call <4 x float> @llvm.exp.v4f32(<4 x float> %38)
  %40 = fcmp ogt <4 x float> %39, zeroinitializer
  %41 = select <4 x i1> %40, <4 x float> %39, <4 x float> zeroinitializer
  %42 = fsub <4 x float> zeroinitializer, %41
  %43 = call <4 x float> @llvm.exp.v4f32(<4 x float> %42)
  %44 = fadd <4 x float> %43, <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>
  %45 = fdiv <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %44
  %46 = bitcast i8* %0 to <4 x float>*
  store <4 x float> %45, <4 x float>* %46, align 4, !tbaa !20
  %47 = getelementptr inbounds i8, i8* %1, i64 16
  %48 = bitcast i8* %47 to float*
  %49 = load float, float* %48, align 4, !tbaa !16
  %50 = fcmp olt float %49, 9.000000e+00
  %51 = select i1 %50, float %49, float 9.000000e+00
  %52 = getelementptr inbounds i8, i8* %1, i64 20
  %53 = bitcast i8* %52 to float*
  %54 = load float, float* %53, align 4, !tbaa !16
  %55 = fcmp olt float %54, 9.000000e+00
  %56 = select i1 %55, float %54, float 9.000000e+00
  %57 = getelementptr inbounds i8, i8* %1, i64 24
  %58 = bitcast i8* %57 to float*
  %59 = load float, float* %58, align 4, !tbaa !16
  %60 = fcmp olt float %59, 9.000000e+00
  %61 = select i1 %60, float %59, float 9.000000e+00
  %62 = getelementptr inbounds i8, i8* %1, i64 28
  %63 = bitcast i8* %62 to float*
  %64 = load float, float* %63, align 4, !tbaa !16
  %65 = fcmp olt float %64, 9.000000e+00
  %66 = select i1 %65, float %64, float 9.000000e+00
  %67 = insertelement <4 x float> undef, float %51, i32 0
  %68 = insertelement <4 x float> %67, float %56, i32 1
  %69 = insertelement <4 x float> %68, float %61, i32 2
  %70 = insertelement <4 x float> %69, float %66, i32 3
  %71 = fcmp ogt <4 x float> %70, <float -9.000000e+00, float -9.000000e+00, float -9.000000e+00, float -9.000000e+00>
  %72 = select <4 x i1> %71, <4 x float> %70, <4 x float> <float -9.000000e+00, float -9.000000e+00, float -9.000000e+00, float -9.000000e+00>
  %73 = fmul <4 x float> %72, %72
  %74 = getelementptr inbounds i8, i8* %0, i64 16
  %75 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> <float 0xBCB3E4B800000000, float 0xBCB3E4B800000000, float 0xBCB3E4B800000000, float 0xBCB3E4B800000000>, <4 x float> <float 0x3D4C266FC0000000, float 0x3D4C266FC0000000, float 0x3D4C266FC0000000, float 0x3D4C266FC0000000>)
  %76 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %75, <4 x float> <float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000, float 0xBDD7A6FFE0000000>)
  %77 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %76, <4 x float> <float 0x3E6B800820000000, float 0x3E6B800820000000, float 0x3E6B800820000000, float 0x3E6B800820000000>)
  %78 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %77, <4 x float> <float 0x3EEF286940000000, float 0x3EEF286940000000, float 0x3EEF286940000000, float 0x3EEF286940000000>)
  %79 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %78, <4 x float> <float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000, float 0x3F44E1BDA0000000>)
  %80 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %79, <4 x float> <float 0x3F740B3B80000000, float 0x3F740B3B80000000, float 0x3F740B3B80000000, float 0x3F740B3B80000000>)
  %81 = fmul <4 x float> %72, %80
  %82 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> <float 0x3EB41A7B00000000, float 0x3EB41A7B00000000, float 0x3EB41A7B00000000, float 0x3EB41A7B00000000>, <4 x float> <float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000, float 0x3F1F12BAC0000000>)
  %83 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %82, <4 x float> <float 0x3F629540A0000000, float 0x3F629540A0000000, float 0x3F629540A0000000, float 0x3F629540A0000000>)
  %84 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %73, <4 x float> %83, <4 x float> <float 0x3F740B3BA0000000, float 0x3F740B3BA0000000, float 0x3F740B3BA0000000, float 0x3F740B3BA0000000>)
  %85 = fdiv <4 x float> %81, %84
  %86 = call <4 x float> @llvm.exp.v4f32(<4 x float> %85)
  %87 = fcmp ogt <4 x float> %86, zeroinitializer
  %88 = select <4 x i1> %87, <4 x float> %86, <4 x float> zeroinitializer
  %89 = fsub <4 x float> zeroinitializer, %88
  %90 = call <4 x float> @llvm.exp.v4f32(<4 x float> %89)
  %91 = fadd <4 x float> %90, <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>
  %92 = fdiv <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %91
  %93 = bitcast i8* %74 to <4 x float>*
  store <4 x float> %92, <4 x float>* %93, align 4, !tbaa !20
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>) #2

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
