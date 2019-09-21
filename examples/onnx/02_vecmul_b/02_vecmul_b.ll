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
  %wide.load = load <4 x float>, <4 x float>* %3, align 4, !tbaa !16
  %4 = bitcast i8* %1 to <4 x float>*
  %wide.load1 = load <4 x float>, <4 x float>* %4, align 4, !tbaa !20
  %5 = fmul <4 x float> %wide.load, %wide.load1
  %6 = bitcast i8* %0 to <4 x float>*
  store <4 x float> %5, <4 x float>* %6, align 4, !tbaa !23
  %7 = getelementptr inbounds i8, i8* %2, i64 16
  %8 = bitcast i8* %7 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %8, align 4, !tbaa !16
  %9 = getelementptr inbounds i8, i8* %1, i64 16
  %10 = bitcast i8* %9 to <4 x float>*
  %wide.load1.1 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !20
  %11 = fmul <4 x float> %wide.load.1, %wide.load1.1
  %12 = getelementptr inbounds i8, i8* %0, i64 16
  %13 = bitcast i8* %12 to <4 x float>*
  store <4 x float> %11, <4 x float>* %13, align 4, !tbaa !23
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !16
  %16 = getelementptr inbounds i8, i8* %1, i64 32
  %17 = bitcast i8* %16 to <4 x float>*
  %wide.load1.2 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !20
  %18 = fmul <4 x float> %wide.load.2, %wide.load1.2
  %19 = getelementptr inbounds i8, i8* %0, i64 32
  %20 = bitcast i8* %19 to <4 x float>*
  store <4 x float> %18, <4 x float>* %20, align 4, !tbaa !23
  %21 = getelementptr inbounds i8, i8* %2, i64 48
  %22 = bitcast i8* %21 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !16
  %23 = getelementptr inbounds i8, i8* %1, i64 48
  %24 = bitcast i8* %23 to <4 x float>*
  %wide.load1.3 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !20
  %25 = fmul <4 x float> %wide.load.3, %wide.load1.3
  %26 = getelementptr inbounds i8, i8* %0, i64 48
  %27 = bitcast i8* %26 to <4 x float>*
  store <4 x float> %25, <4 x float>* %27, align 4, !tbaa !23
  %28 = getelementptr inbounds i8, i8* %2, i64 64
  %29 = bitcast i8* %28 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !16
  %30 = getelementptr inbounds i8, i8* %1, i64 64
  %31 = bitcast i8* %30 to <4 x float>*
  %wide.load1.4 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !20
  %32 = fmul <4 x float> %wide.load.4, %wide.load1.4
  %33 = getelementptr inbounds i8, i8* %0, i64 64
  %34 = bitcast i8* %33 to <4 x float>*
  store <4 x float> %32, <4 x float>* %34, align 4, !tbaa !23
  %35 = getelementptr inbounds i8, i8* %2, i64 80
  %36 = bitcast i8* %35 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !16
  %37 = getelementptr inbounds i8, i8* %1, i64 80
  %38 = bitcast i8* %37 to <4 x float>*
  %wide.load1.5 = load <4 x float>, <4 x float>* %38, align 4, !tbaa !20
  %39 = fmul <4 x float> %wide.load.5, %wide.load1.5
  %40 = getelementptr inbounds i8, i8* %0, i64 80
  %41 = bitcast i8* %40 to <4 x float>*
  store <4 x float> %39, <4 x float>* %41, align 4, !tbaa !23
  %42 = getelementptr inbounds i8, i8* %2, i64 96
  %43 = bitcast i8* %42 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %43, align 4, !tbaa !16
  %44 = getelementptr inbounds i8, i8* %1, i64 96
  %45 = bitcast i8* %44 to <4 x float>*
  %wide.load1.6 = load <4 x float>, <4 x float>* %45, align 4, !tbaa !20
  %46 = fmul <4 x float> %wide.load.6, %wide.load1.6
  %47 = getelementptr inbounds i8, i8* %0, i64 96
  %48 = bitcast i8* %47 to <4 x float>*
  store <4 x float> %46, <4 x float>* %48, align 4, !tbaa !23
  %49 = getelementptr inbounds i8, i8* %2, i64 112
  %50 = bitcast i8* %49 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %50, align 4, !tbaa !16
  %51 = getelementptr inbounds i8, i8* %1, i64 112
  %52 = bitcast i8* %51 to <4 x float>*
  %wide.load1.7 = load <4 x float>, <4 x float>* %52, align 4, !tbaa !20
  %53 = fmul <4 x float> %wide.load.7, %wide.load1.7
  %54 = getelementptr inbounds i8, i8* %0, i64 112
  %55 = bitcast i8* %54 to <4 x float>*
  store <4 x float> %53, <4 x float>* %55, align 4, !tbaa !23
  %56 = getelementptr inbounds i8, i8* %2, i64 128
  %57 = bitcast i8* %56 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !16
  %58 = getelementptr inbounds i8, i8* %1, i64 128
  %59 = bitcast i8* %58 to <4 x float>*
  %wide.load1.8 = load <4 x float>, <4 x float>* %59, align 4, !tbaa !20
  %60 = fmul <4 x float> %wide.load.8, %wide.load1.8
  %61 = getelementptr inbounds i8, i8* %0, i64 128
  %62 = bitcast i8* %61 to <4 x float>*
  store <4 x float> %60, <4 x float>* %62, align 4, !tbaa !23
  %63 = getelementptr inbounds i8, i8* %2, i64 144
  %64 = bitcast i8* %63 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %64, align 4, !tbaa !16
  %65 = getelementptr inbounds i8, i8* %1, i64 144
  %66 = bitcast i8* %65 to <4 x float>*
  %wide.load1.9 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !20
  %67 = fmul <4 x float> %wide.load.9, %wide.load1.9
  %68 = getelementptr inbounds i8, i8* %0, i64 144
  %69 = bitcast i8* %68 to <4 x float>*
  store <4 x float> %67, <4 x float>* %69, align 4, !tbaa !23
  %70 = getelementptr inbounds i8, i8* %2, i64 160
  %71 = bitcast i8* %70 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !16
  %72 = getelementptr inbounds i8, i8* %1, i64 160
  %73 = bitcast i8* %72 to <4 x float>*
  %wide.load1.10 = load <4 x float>, <4 x float>* %73, align 4, !tbaa !20
  %74 = fmul <4 x float> %wide.load.10, %wide.load1.10
  %75 = getelementptr inbounds i8, i8* %0, i64 160
  %76 = bitcast i8* %75 to <4 x float>*
  store <4 x float> %74, <4 x float>* %76, align 4, !tbaa !23
  %77 = getelementptr inbounds i8, i8* %2, i64 176
  %78 = bitcast i8* %77 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !16
  %79 = getelementptr inbounds i8, i8* %1, i64 176
  %80 = bitcast i8* %79 to <4 x float>*
  %wide.load1.11 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !20
  %81 = fmul <4 x float> %wide.load.11, %wide.load1.11
  %82 = getelementptr inbounds i8, i8* %0, i64 176
  %83 = bitcast i8* %82 to <4 x float>*
  store <4 x float> %81, <4 x float>* %83, align 4, !tbaa !23
  %84 = getelementptr inbounds i8, i8* %2, i64 192
  %85 = bitcast i8* %84 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %85, align 4, !tbaa !16
  %86 = getelementptr inbounds i8, i8* %1, i64 192
  %87 = bitcast i8* %86 to <4 x float>*
  %wide.load1.12 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !20
  %88 = fmul <4 x float> %wide.load.12, %wide.load1.12
  %89 = getelementptr inbounds i8, i8* %0, i64 192
  %90 = bitcast i8* %89 to <4 x float>*
  store <4 x float> %88, <4 x float>* %90, align 4, !tbaa !23
  %91 = getelementptr inbounds i8, i8* %2, i64 208
  %92 = bitcast i8* %91 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %92, align 4, !tbaa !16
  %93 = getelementptr inbounds i8, i8* %1, i64 208
  %94 = bitcast i8* %93 to <4 x float>*
  %wide.load1.13 = load <4 x float>, <4 x float>* %94, align 4, !tbaa !20
  %95 = fmul <4 x float> %wide.load.13, %wide.load1.13
  %96 = getelementptr inbounds i8, i8* %0, i64 208
  %97 = bitcast i8* %96 to <4 x float>*
  store <4 x float> %95, <4 x float>* %97, align 4, !tbaa !23
  %98 = getelementptr inbounds i8, i8* %2, i64 224
  %99 = bitcast i8* %98 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %99, align 4, !tbaa !16
  %100 = getelementptr inbounds i8, i8* %1, i64 224
  %101 = bitcast i8* %100 to <4 x float>*
  %wide.load1.14 = load <4 x float>, <4 x float>* %101, align 4, !tbaa !20
  %102 = fmul <4 x float> %wide.load.14, %wide.load1.14
  %103 = getelementptr inbounds i8, i8* %0, i64 224
  %104 = bitcast i8* %103 to <4 x float>*
  store <4 x float> %102, <4 x float>* %104, align 4, !tbaa !23
  %105 = getelementptr inbounds i8, i8* %2, i64 240
  %106 = bitcast i8* %105 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %106, align 4, !tbaa !16
  %107 = getelementptr inbounds i8, i8* %1, i64 240
  %108 = bitcast i8* %107 to <4 x float>*
  %wide.load1.15 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !20
  %109 = fmul <4 x float> %wide.load.15, %wide.load1.15
  %110 = getelementptr inbounds i8, i8* %0, i64 240
  %111 = bitcast i8* %110 to <4 x float>*
  store <4 x float> %109, <4 x float>* %111, align 4, !tbaa !23
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
