; ModuleID = './12_maxp_a.ll'
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
  %4 = load float, float* %3, align 4, !tbaa !16
  %5 = fcmp olt float %4, 0xC7EFFFFFE0000000
  %6 = select i1 %5, float 0xC7EFFFFFE0000000, float %4
  %7 = getelementptr inbounds i8, i8* %1, i64 4
  %8 = bitcast i8* %7 to float*
  %9 = load float, float* %8, align 4, !tbaa !16
  %10 = fcmp ogt float %6, %9
  %11 = select i1 %10, float %6, float %9
  %12 = getelementptr inbounds i8, i8* %1, i64 8
  %13 = bitcast i8* %12 to float*
  %14 = load float, float* %13, align 4, !tbaa !16
  %15 = fcmp ogt float %11, %14
  %16 = select i1 %15, float %11, float %14
  %17 = getelementptr inbounds i8, i8* %1, i64 32
  %18 = bitcast i8* %17 to float*
  %19 = load float, float* %18, align 4, !tbaa !16
  %20 = fcmp ogt float %16, %19
  %21 = select i1 %20, float %16, float %19
  %22 = getelementptr inbounds i8, i8* %1, i64 36
  %23 = bitcast i8* %22 to float*
  %24 = load float, float* %23, align 4, !tbaa !16
  %25 = fcmp ogt float %21, %24
  %26 = select i1 %25, float %21, float %24
  %27 = getelementptr inbounds i8, i8* %1, i64 40
  %28 = bitcast i8* %27 to float*
  %29 = load float, float* %28, align 4, !tbaa !16
  %30 = fcmp ogt float %26, %29
  %31 = select i1 %30, float %26, float %29
  %32 = getelementptr inbounds i8, i8* %1, i64 64
  %33 = bitcast i8* %32 to float*
  %34 = load float, float* %33, align 4, !tbaa !16
  %35 = fcmp ogt float %31, %34
  %36 = select i1 %35, float %31, float %34
  %37 = getelementptr inbounds i8, i8* %1, i64 68
  %38 = bitcast i8* %37 to float*
  %39 = load float, float* %38, align 4, !tbaa !16
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = getelementptr inbounds i8, i8* %1, i64 72
  %43 = bitcast i8* %42 to float*
  %44 = load float, float* %43, align 4, !tbaa !16
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  store float %46, float* %2, align 4, !tbaa !20
  %47 = getelementptr inbounds i8, i8* %0, i64 4
  %48 = bitcast i8* %47 to float*
  %49 = getelementptr inbounds i8, i8* %1, i64 12
  %50 = bitcast i8* %49 to float*
  %51 = load float, float* %50, align 4, !tbaa !16
  %52 = fcmp olt float %51, 0xC7EFFFFFE0000000
  %53 = select i1 %52, float 0xC7EFFFFFE0000000, float %51
  %54 = getelementptr inbounds i8, i8* %1, i64 16
  %55 = bitcast i8* %54 to float*
  %56 = load float, float* %55, align 4, !tbaa !16
  %57 = fcmp ogt float %53, %56
  %58 = select i1 %57, float %53, float %56
  %59 = getelementptr inbounds i8, i8* %1, i64 20
  %60 = bitcast i8* %59 to float*
  %61 = load float, float* %60, align 4, !tbaa !16
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = getelementptr inbounds i8, i8* %1, i64 44
  %65 = bitcast i8* %64 to float*
  %66 = load float, float* %65, align 4, !tbaa !16
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = getelementptr inbounds i8, i8* %1, i64 48
  %70 = bitcast i8* %69 to float*
  %71 = load float, float* %70, align 4, !tbaa !16
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  %74 = getelementptr inbounds i8, i8* %1, i64 52
  %75 = bitcast i8* %74 to float*
  %76 = load float, float* %75, align 4, !tbaa !16
  %77 = fcmp ogt float %73, %76
  %78 = select i1 %77, float %73, float %76
  %79 = getelementptr inbounds i8, i8* %1, i64 76
  %80 = bitcast i8* %79 to float*
  %81 = load float, float* %80, align 4, !tbaa !16
  %82 = fcmp ogt float %78, %81
  %83 = select i1 %82, float %78, float %81
  %84 = getelementptr inbounds i8, i8* %1, i64 80
  %85 = bitcast i8* %84 to float*
  %86 = load float, float* %85, align 4, !tbaa !16
  %87 = fcmp ogt float %83, %86
  %88 = select i1 %87, float %83, float %86
  %89 = getelementptr inbounds i8, i8* %1, i64 84
  %90 = bitcast i8* %89 to float*
  %91 = load float, float* %90, align 4, !tbaa !16
  %92 = fcmp ogt float %88, %91
  %93 = select i1 %92, float %88, float %91
  store float %93, float* %48, align 4, !tbaa !20
  %94 = getelementptr inbounds i8, i8* %0, i64 8
  %95 = bitcast i8* %94 to float*
  %96 = getelementptr inbounds i8, i8* %1, i64 96
  %97 = bitcast i8* %96 to float*
  %98 = load float, float* %97, align 4, !tbaa !16
  %99 = fcmp olt float %98, 0xC7EFFFFFE0000000
  %100 = select i1 %99, float 0xC7EFFFFFE0000000, float %98
  %101 = getelementptr inbounds i8, i8* %1, i64 100
  %102 = bitcast i8* %101 to float*
  %103 = load float, float* %102, align 4, !tbaa !16
  %104 = fcmp ogt float %100, %103
  %105 = select i1 %104, float %100, float %103
  %106 = getelementptr inbounds i8, i8* %1, i64 104
  %107 = bitcast i8* %106 to float*
  %108 = load float, float* %107, align 4, !tbaa !16
  %109 = fcmp ogt float %105, %108
  %110 = select i1 %109, float %105, float %108
  %111 = getelementptr inbounds i8, i8* %1, i64 128
  %112 = bitcast i8* %111 to float*
  %113 = load float, float* %112, align 4, !tbaa !16
  %114 = fcmp ogt float %110, %113
  %115 = select i1 %114, float %110, float %113
  %116 = getelementptr inbounds i8, i8* %1, i64 132
  %117 = bitcast i8* %116 to float*
  %118 = load float, float* %117, align 4, !tbaa !16
  %119 = fcmp ogt float %115, %118
  %120 = select i1 %119, float %115, float %118
  %121 = getelementptr inbounds i8, i8* %1, i64 136
  %122 = bitcast i8* %121 to float*
  %123 = load float, float* %122, align 4, !tbaa !16
  %124 = fcmp ogt float %120, %123
  %125 = select i1 %124, float %120, float %123
  %126 = getelementptr inbounds i8, i8* %1, i64 160
  %127 = bitcast i8* %126 to float*
  %128 = load float, float* %127, align 4, !tbaa !16
  %129 = fcmp ogt float %125, %128
  %130 = select i1 %129, float %125, float %128
  %131 = getelementptr inbounds i8, i8* %1, i64 164
  %132 = bitcast i8* %131 to float*
  %133 = load float, float* %132, align 4, !tbaa !16
  %134 = fcmp ogt float %130, %133
  %135 = select i1 %134, float %130, float %133
  %136 = getelementptr inbounds i8, i8* %1, i64 168
  %137 = bitcast i8* %136 to float*
  %138 = load float, float* %137, align 4, !tbaa !16
  %139 = fcmp ogt float %135, %138
  %140 = select i1 %139, float %135, float %138
  store float %140, float* %95, align 4, !tbaa !20
  %141 = getelementptr inbounds i8, i8* %0, i64 12
  %142 = bitcast i8* %141 to float*
  %143 = getelementptr inbounds i8, i8* %1, i64 108
  %144 = bitcast i8* %143 to float*
  %145 = load float, float* %144, align 4, !tbaa !16
  %146 = fcmp olt float %145, 0xC7EFFFFFE0000000
  %147 = select i1 %146, float 0xC7EFFFFFE0000000, float %145
  %148 = getelementptr inbounds i8, i8* %1, i64 112
  %149 = bitcast i8* %148 to float*
  %150 = load float, float* %149, align 4, !tbaa !16
  %151 = fcmp ogt float %147, %150
  %152 = select i1 %151, float %147, float %150
  %153 = getelementptr inbounds i8, i8* %1, i64 116
  %154 = bitcast i8* %153 to float*
  %155 = load float, float* %154, align 4, !tbaa !16
  %156 = fcmp ogt float %152, %155
  %157 = select i1 %156, float %152, float %155
  %158 = getelementptr inbounds i8, i8* %1, i64 140
  %159 = bitcast i8* %158 to float*
  %160 = load float, float* %159, align 4, !tbaa !16
  %161 = fcmp ogt float %157, %160
  %162 = select i1 %161, float %157, float %160
  %163 = getelementptr inbounds i8, i8* %1, i64 144
  %164 = bitcast i8* %163 to float*
  %165 = load float, float* %164, align 4, !tbaa !16
  %166 = fcmp ogt float %162, %165
  %167 = select i1 %166, float %162, float %165
  %168 = getelementptr inbounds i8, i8* %1, i64 148
  %169 = bitcast i8* %168 to float*
  %170 = load float, float* %169, align 4, !tbaa !16
  %171 = fcmp ogt float %167, %170
  %172 = select i1 %171, float %167, float %170
  %173 = getelementptr inbounds i8, i8* %1, i64 172
  %174 = bitcast i8* %173 to float*
  %175 = load float, float* %174, align 4, !tbaa !16
  %176 = fcmp ogt float %172, %175
  %177 = select i1 %176, float %172, float %175
  %178 = getelementptr inbounds i8, i8* %1, i64 176
  %179 = bitcast i8* %178 to float*
  %180 = load float, float* %179, align 4, !tbaa !16
  %181 = fcmp ogt float %177, %180
  %182 = select i1 %181, float %177, float %180
  %183 = getelementptr inbounds i8, i8* %1, i64 180
  %184 = bitcast i8* %183 to float*
  %185 = load float, float* %184, align 4, !tbaa !16
  %186 = fcmp ogt float %182, %185
  %187 = select i1 %186, float %182, float %185
  store float %187, float* %142, align 4, !tbaa !20
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
!18 = !{!"0x5591a2483730", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x5591a24836e0", !19, i64 0}
