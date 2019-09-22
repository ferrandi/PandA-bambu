; ModuleID = 'fused_nn_dense_add'
source_filename = "fused_nn_dense_add"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i32*, i32 }
%1 = type { i8*, %2, i32, %3, i64*, i64*, i64 }
%2 = type { i32, i32 }
%3 = type { i8, i8, i16 }
%4 = type { i8*, i8*, float* }

@__TVMBackendParallelLaunch = linkonce dllexport local_unnamed_addr global i32 (i32 (i32, %0*, i8*)*, i8*, i32)* null, align 8
@__tvm_main__ = weak local_unnamed_addr constant [19 x i8] c"fused_nn_dense_add\00", align 1

define dllexport i32 @fused_nn_dense_add(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !12, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i8* %1, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !15
  %3 = bitcast i8* %0 to %1**, !dbg !15
  %4 = load %1*, %1** %3, align 8, !dbg !15
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !15
  %6 = bitcast i8* %5 to %1**, !dbg !15
  %7 = load %1*, %1** %6, align 8, !dbg !15
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !15
  %9 = bitcast i8* %8 to %1**, !dbg !15
  %10 = load %1*, %1** %9, align 8, !dbg !15
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !15
  %12 = bitcast i8* %11 to %1**, !dbg !15
  %13 = load %1*, %1** %12, align 8, !dbg !15
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !15
  %15 = load i8*, i8** %14, align 8, !dbg !15
  %16 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !15
  %17 = load i8*, i8** %16, align 8, !dbg !15
  %18 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !15
  %19 = load i8*, i8** %18, align 8, !dbg !15
  %20 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !15
  %21 = load i8*, i8** %20, align 8, !dbg !15
  %22 = tail call fastcc i32 @fused_nn_dense_add_compute_(i8* %15, i8* %17, i8* %21, i8* %19), !dbg !15
  ret i32 %22, !dbg !15
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #0 {
entry:
  %4 = alloca [10 x float], align 16
  %5 = alloca %4, align 8
  %.sub = getelementptr inbounds [10 x float], [10 x float]* %4, i64 0, i64 0
  %6 = getelementptr inbounds %4, %4* %5, i64 0, i32 0
  store i8* %0, i8** %6, align 8
  %7 = getelementptr inbounds %4, %4* %5, i64 0, i32 1
  store i8* %1, i8** %7, align 8
  %8 = getelementptr inbounds %4, %4* %5, i64 0, i32 2
  store float* %.sub, float** %8, align 8
  %9 = bitcast %4* %5 to i8*
  %10 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %11 = call i32 %10(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda, i8* nonnull %9, i32 0)
  %12 = icmp eq i32 %11, 0
  br i1 %12, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %entry
  %merge = phi i32 [ %11, %entry ], [ 0, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %13 = bitcast i8* %3 to <4 x float>*
  %14 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !20
  %15 = bitcast [10 x float]* %4 to <4 x float>*
  %16 = load <4 x float>, <4 x float>* %15, align 16, !tbaa !23
  %17 = fadd <4 x float> %14, %16
  %18 = bitcast i8* %2 to <4 x float>*
  store <4 x float> %17, <4 x float>* %18, align 4, !tbaa !26
  %19 = getelementptr inbounds i8, i8* %3, i64 16
  %20 = getelementptr inbounds [10 x float], [10 x float]* %4, i64 0, i64 4
  %21 = getelementptr inbounds i8, i8* %2, i64 16
  %22 = bitcast i8* %19 to <4 x float>*
  %23 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !20
  %24 = bitcast float* %20 to <4 x float>*
  %25 = load <4 x float>, <4 x float>* %24, align 16, !tbaa !23
  %26 = fadd <4 x float> %23, %25
  %27 = bitcast i8* %21 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !26
  %28 = getelementptr inbounds i8, i8* %3, i64 32
  %29 = bitcast i8* %28 to float*
  %30 = load float, float* %29, align 4, !tbaa !20
  %31 = getelementptr inbounds [10 x float], [10 x float]* %4, i64 0, i64 8
  %32 = load float, float* %31, align 16, !tbaa !23
  %33 = fadd float %30, %32
  %34 = getelementptr inbounds i8, i8* %2, i64 32
  %35 = bitcast i8* %34 to float*
  store float %33, float* %35, align 4, !tbaa !26
  %36 = getelementptr inbounds i8, i8* %3, i64 36
  %37 = bitcast i8* %36 to float*
  %38 = load float, float* %37, align 4, !tbaa !20
  %39 = getelementptr inbounds [10 x float], [10 x float]* %4, i64 0, i64 9
  %40 = load float, float* %39, align 4, !tbaa !23
  %41 = fadd float %38, %40
  %42 = getelementptr inbounds i8, i8* %2, i64 36
  %43 = bitcast i8* %42 to float*
  store float %41, float* %43, align 4, !tbaa !26
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds i8, i8* %2, i64 16
  %9 = bitcast i8* %8 to float**
  %10 = load float*, float** %9, align 8
  %11 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %12 = load i32, i32* %11, align 4
  %13 = add nsw i32 %12, 9
  %14 = sdiv i32 %13, %12
  %15 = add nsw i32 %0, 1
  %16 = mul nsw i32 %14, %15
  %17 = icmp slt i32 %16, 10
  %18 = select i1 %17, i32 %16, i32 10
  %19 = mul nsw i32 %14, %0
  %20 = icmp slt i32 %19, 10
  %21 = select i1 %20, i32 %19, i32 10
  %22 = icmp slt i32 %21, %18
  br i1 %22, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %23 = bitcast float* %4 to <16 x float>*
  %24 = add i32 %21, 1
  %25 = sext i32 %24 to i64
  %26 = add nsw i64 %25, -1
  %27 = sext i32 %18 to i64
  %28 = load <16 x float>, <16 x float>* %23, align 64, !tbaa !29
  %29 = getelementptr inbounds float, float* %4, i64 16
  %30 = bitcast float* %29 to <16 x float>*
  %31 = load <16 x float>, <16 x float>* %30, align 64, !tbaa !29
  %32 = getelementptr inbounds float, float* %4, i64 32
  %33 = bitcast float* %32 to <16 x float>*
  %34 = load <16 x float>, <16 x float>* %33, align 64, !tbaa !29
  %35 = getelementptr inbounds float, float* %4, i64 48
  %36 = bitcast float* %35 to <16 x float>*
  %37 = load <16 x float>, <16 x float>* %36, align 64, !tbaa !29
  %38 = getelementptr inbounds float, float* %4, i64 64
  %39 = bitcast float* %38 to <16 x float>*
  %40 = load <16 x float>, <16 x float>* %39, align 64, !tbaa !29
  %41 = getelementptr inbounds float, float* %4, i64 80
  %42 = bitcast float* %41 to <16 x float>*
  %43 = load <16 x float>, <16 x float>* %42, align 64, !tbaa !29
  %44 = getelementptr inbounds float, float* %4, i64 96
  %45 = bitcast float* %44 to <16 x float>*
  %46 = load <16 x float>, <16 x float>* %45, align 64, !tbaa !29
  %47 = getelementptr inbounds float, float* %4, i64 112
  %48 = bitcast float* %47 to <16 x float>*
  %49 = load <16 x float>, <16 x float>* %48, align 64, !tbaa !29
  %50 = getelementptr inbounds float, float* %4, i64 128
  %51 = bitcast float* %50 to <16 x float>*
  %52 = load <16 x float>, <16 x float>* %51, align 64, !tbaa !29
  %53 = getelementptr inbounds float, float* %4, i64 144
  %54 = bitcast float* %53 to <16 x float>*
  %55 = load <16 x float>, <16 x float>* %54, align 64, !tbaa !29
  %56 = getelementptr inbounds float, float* %4, i64 160
  %57 = bitcast float* %56 to <16 x float>*
  %58 = load <16 x float>, <16 x float>* %57, align 64, !tbaa !29
  %59 = getelementptr inbounds float, float* %4, i64 176
  %60 = bitcast float* %59 to <16 x float>*
  %61 = load <16 x float>, <16 x float>* %60, align 64, !tbaa !29
  %62 = getelementptr inbounds float, float* %4, i64 192
  %63 = bitcast float* %62 to <16 x float>*
  %64 = load <16 x float>, <16 x float>* %63, align 64, !tbaa !29
  %65 = getelementptr inbounds float, float* %4, i64 208
  %66 = bitcast float* %65 to <16 x float>*
  %67 = load <16 x float>, <16 x float>* %66, align 64, !tbaa !29
  %68 = getelementptr inbounds float, float* %4, i64 224
  %69 = bitcast float* %68 to <16 x float>*
  %70 = load <16 x float>, <16 x float>* %69, align 64, !tbaa !29
  %71 = getelementptr inbounds float, float* %4, i64 240
  %72 = bitcast float* %71 to <16 x float>*
  %73 = load <16 x float>, <16 x float>* %72, align 64, !tbaa !29
  %74 = getelementptr inbounds float, float* %4, i64 256
  %75 = bitcast float* %74 to <16 x float>*
  %76 = load <16 x float>, <16 x float>* %75, align 64, !tbaa !29
  %77 = getelementptr inbounds float, float* %4, i64 272
  %78 = bitcast float* %77 to <16 x float>*
  %79 = load <16 x float>, <16 x float>* %78, align 64, !tbaa !29
  %80 = getelementptr inbounds float, float* %4, i64 288
  %81 = bitcast float* %80 to <16 x float>*
  %82 = load <16 x float>, <16 x float>* %81, align 64, !tbaa !29
  %83 = getelementptr inbounds float, float* %4, i64 304
  %84 = bitcast float* %83 to <16 x float>*
  %85 = load <16 x float>, <16 x float>* %84, align 64, !tbaa !29
  %86 = getelementptr inbounds float, float* %4, i64 320
  %87 = bitcast float* %86 to <16 x float>*
  %88 = load <16 x float>, <16 x float>* %87, align 64, !tbaa !29
  %89 = getelementptr inbounds float, float* %4, i64 336
  %90 = bitcast float* %89 to <16 x float>*
  %91 = load <16 x float>, <16 x float>* %90, align 64, !tbaa !29
  %92 = getelementptr inbounds float, float* %4, i64 352
  %93 = bitcast float* %92 to <16 x float>*
  %94 = load <16 x float>, <16 x float>* %93, align 64, !tbaa !29
  %95 = getelementptr inbounds float, float* %4, i64 368
  %96 = bitcast float* %95 to <16 x float>*
  %97 = load <16 x float>, <16 x float>* %96, align 64, !tbaa !29
  %98 = getelementptr inbounds float, float* %4, i64 384
  %99 = bitcast float* %98 to <16 x float>*
  %100 = load <16 x float>, <16 x float>* %99, align 64, !tbaa !29
  %101 = getelementptr inbounds float, float* %4, i64 400
  %102 = bitcast float* %101 to <16 x float>*
  %103 = load <16 x float>, <16 x float>* %102, align 64, !tbaa !29
  %104 = getelementptr inbounds float, float* %4, i64 416
  %105 = bitcast float* %104 to <16 x float>*
  %106 = load <16 x float>, <16 x float>* %105, align 64, !tbaa !29
  %107 = getelementptr inbounds float, float* %4, i64 432
  %108 = bitcast float* %107 to <16 x float>*
  %109 = load <16 x float>, <16 x float>* %108, align 64, !tbaa !29
  %110 = getelementptr inbounds float, float* %4, i64 448
  %111 = bitcast float* %110 to <16 x float>*
  %112 = load <16 x float>, <16 x float>* %111, align 64, !tbaa !29
  %113 = getelementptr inbounds float, float* %4, i64 464
  %114 = bitcast float* %113 to <16 x float>*
  %115 = load <16 x float>, <16 x float>* %114, align 64, !tbaa !29
  %116 = getelementptr inbounds float, float* %4, i64 480
  %117 = bitcast float* %116 to <16 x float>*
  %118 = load <16 x float>, <16 x float>* %117, align 64, !tbaa !29
  %119 = getelementptr inbounds float, float* %4, i64 496
  %120 = bitcast float* %119 to <16 x float>*
  %121 = load <16 x float>, <16 x float>* %120, align 64, !tbaa !29
  %122 = getelementptr inbounds float, float* %4, i64 512
  %123 = bitcast float* %122 to <16 x float>*
  %124 = load <16 x float>, <16 x float>* %123, align 64, !tbaa !29
  %125 = getelementptr inbounds float, float* %4, i64 528
  %126 = bitcast float* %125 to <16 x float>*
  %127 = load <16 x float>, <16 x float>* %126, align 64, !tbaa !29
  %128 = getelementptr inbounds float, float* %4, i64 544
  %129 = bitcast float* %128 to <16 x float>*
  %130 = load <16 x float>, <16 x float>* %129, align 64, !tbaa !29
  %131 = getelementptr inbounds float, float* %4, i64 560
  %132 = bitcast float* %131 to <16 x float>*
  %133 = load <16 x float>, <16 x float>* %132, align 64, !tbaa !29
  %134 = getelementptr inbounds float, float* %4, i64 576
  %135 = bitcast float* %134 to <16 x float>*
  %136 = load <16 x float>, <16 x float>* %135, align 64, !tbaa !29
  %137 = getelementptr inbounds float, float* %4, i64 592
  %138 = bitcast float* %137 to <16 x float>*
  %139 = load <16 x float>, <16 x float>* %138, align 64, !tbaa !29
  %140 = getelementptr inbounds float, float* %4, i64 608
  %141 = bitcast float* %140 to <16 x float>*
  %142 = load <16 x float>, <16 x float>* %141, align 64, !tbaa !29
  %143 = getelementptr inbounds float, float* %4, i64 624
  %144 = bitcast float* %143 to <16 x float>*
  %145 = load <16 x float>, <16 x float>* %144, align 64, !tbaa !29
  %146 = getelementptr inbounds float, float* %4, i64 640
  %147 = bitcast float* %146 to <16 x float>*
  %148 = load <16 x float>, <16 x float>* %147, align 64, !tbaa !29
  %149 = getelementptr inbounds float, float* %4, i64 656
  %150 = bitcast float* %149 to <16 x float>*
  %151 = load <16 x float>, <16 x float>* %150, align 64, !tbaa !29
  %152 = getelementptr inbounds float, float* %4, i64 672
  %153 = bitcast float* %152 to <16 x float>*
  %154 = load <16 x float>, <16 x float>* %153, align 64, !tbaa !29
  %155 = getelementptr inbounds float, float* %4, i64 688
  %156 = bitcast float* %155 to <16 x float>*
  %157 = load <16 x float>, <16 x float>* %156, align 64, !tbaa !29
  %158 = getelementptr inbounds float, float* %4, i64 704
  %159 = bitcast float* %158 to <16 x float>*
  %160 = load <16 x float>, <16 x float>* %159, align 64, !tbaa !29
  %161 = getelementptr inbounds float, float* %4, i64 720
  %162 = bitcast float* %161 to <16 x float>*
  %163 = load <16 x float>, <16 x float>* %162, align 64, !tbaa !29
  %164 = getelementptr inbounds float, float* %4, i64 736
  %165 = bitcast float* %164 to <16 x float>*
  %166 = load <16 x float>, <16 x float>* %165, align 64, !tbaa !29
  %167 = getelementptr inbounds float, float* %4, i64 752
  %168 = bitcast float* %167 to <16 x float>*
  %169 = load <16 x float>, <16 x float>* %168, align 64, !tbaa !29
  %170 = getelementptr inbounds float, float* %4, i64 768
  %171 = bitcast float* %170 to <16 x float>*
  %172 = load <16 x float>, <16 x float>* %171, align 64, !tbaa !29
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_body
  %indvars.iv = phi i64 [ %26, %for_body.lr.ph ], [ %indvars.iv.next, %for_body ]
  %173 = mul nsw i64 %indvars.iv, 784
  %174 = getelementptr inbounds float, float* %7, i64 %173
  %175 = bitcast float* %174 to <16 x float>*
  %176 = load <16 x float>, <16 x float>* %175, align 64, !tbaa !32
  %177 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %28, <16 x float> %176, <16 x float> zeroinitializer)
  %178 = add nsw i64 %173, 16
  %179 = getelementptr inbounds float, float* %7, i64 %178
  %180 = bitcast float* %179 to <16 x float>*
  %181 = load <16 x float>, <16 x float>* %180, align 64, !tbaa !32
  %182 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %31, <16 x float> %181, <16 x float> %177)
  %183 = add nsw i64 %173, 32
  %184 = getelementptr inbounds float, float* %7, i64 %183
  %185 = bitcast float* %184 to <16 x float>*
  %186 = load <16 x float>, <16 x float>* %185, align 64, !tbaa !32
  %187 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %34, <16 x float> %186, <16 x float> %182)
  %188 = add nsw i64 %173, 48
  %189 = getelementptr inbounds float, float* %7, i64 %188
  %190 = bitcast float* %189 to <16 x float>*
  %191 = load <16 x float>, <16 x float>* %190, align 64, !tbaa !32
  %192 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %37, <16 x float> %191, <16 x float> %187)
  %193 = add nsw i64 %173, 64
  %194 = getelementptr inbounds float, float* %7, i64 %193
  %195 = bitcast float* %194 to <16 x float>*
  %196 = load <16 x float>, <16 x float>* %195, align 64, !tbaa !32
  %197 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %40, <16 x float> %196, <16 x float> %192)
  %198 = add nsw i64 %173, 80
  %199 = getelementptr inbounds float, float* %7, i64 %198
  %200 = bitcast float* %199 to <16 x float>*
  %201 = load <16 x float>, <16 x float>* %200, align 64, !tbaa !32
  %202 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %43, <16 x float> %201, <16 x float> %197)
  %203 = add nsw i64 %173, 96
  %204 = getelementptr inbounds float, float* %7, i64 %203
  %205 = bitcast float* %204 to <16 x float>*
  %206 = load <16 x float>, <16 x float>* %205, align 64, !tbaa !32
  %207 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %46, <16 x float> %206, <16 x float> %202)
  %208 = add nsw i64 %173, 112
  %209 = getelementptr inbounds float, float* %7, i64 %208
  %210 = bitcast float* %209 to <16 x float>*
  %211 = load <16 x float>, <16 x float>* %210, align 64, !tbaa !32
  %212 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %49, <16 x float> %211, <16 x float> %207)
  %213 = add nsw i64 %173, 128
  %214 = getelementptr inbounds float, float* %7, i64 %213
  %215 = bitcast float* %214 to <16 x float>*
  %216 = load <16 x float>, <16 x float>* %215, align 64, !tbaa !32
  %217 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %52, <16 x float> %216, <16 x float> %212)
  %218 = add nsw i64 %173, 144
  %219 = getelementptr inbounds float, float* %7, i64 %218
  %220 = bitcast float* %219 to <16 x float>*
  %221 = load <16 x float>, <16 x float>* %220, align 64, !tbaa !32
  %222 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %55, <16 x float> %221, <16 x float> %217)
  %223 = add nsw i64 %173, 160
  %224 = getelementptr inbounds float, float* %7, i64 %223
  %225 = bitcast float* %224 to <16 x float>*
  %226 = load <16 x float>, <16 x float>* %225, align 64, !tbaa !32
  %227 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %58, <16 x float> %226, <16 x float> %222)
  %228 = add nsw i64 %173, 176
  %229 = getelementptr inbounds float, float* %7, i64 %228
  %230 = bitcast float* %229 to <16 x float>*
  %231 = load <16 x float>, <16 x float>* %230, align 64, !tbaa !32
  %232 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %61, <16 x float> %231, <16 x float> %227)
  %233 = add nsw i64 %173, 192
  %234 = getelementptr inbounds float, float* %7, i64 %233
  %235 = bitcast float* %234 to <16 x float>*
  %236 = load <16 x float>, <16 x float>* %235, align 64, !tbaa !32
  %237 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %64, <16 x float> %236, <16 x float> %232)
  %238 = add nsw i64 %173, 208
  %239 = getelementptr inbounds float, float* %7, i64 %238
  %240 = bitcast float* %239 to <16 x float>*
  %241 = load <16 x float>, <16 x float>* %240, align 64, !tbaa !32
  %242 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %67, <16 x float> %241, <16 x float> %237)
  %243 = add nsw i64 %173, 224
  %244 = getelementptr inbounds float, float* %7, i64 %243
  %245 = bitcast float* %244 to <16 x float>*
  %246 = load <16 x float>, <16 x float>* %245, align 64, !tbaa !32
  %247 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %70, <16 x float> %246, <16 x float> %242)
  %248 = add nsw i64 %173, 240
  %249 = getelementptr inbounds float, float* %7, i64 %248
  %250 = bitcast float* %249 to <16 x float>*
  %251 = load <16 x float>, <16 x float>* %250, align 64, !tbaa !32
  %252 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %73, <16 x float> %251, <16 x float> %247)
  %253 = add nsw i64 %173, 256
  %254 = getelementptr inbounds float, float* %7, i64 %253
  %255 = bitcast float* %254 to <16 x float>*
  %256 = load <16 x float>, <16 x float>* %255, align 64, !tbaa !32
  %257 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %76, <16 x float> %256, <16 x float> %252)
  %258 = add nsw i64 %173, 272
  %259 = getelementptr inbounds float, float* %7, i64 %258
  %260 = bitcast float* %259 to <16 x float>*
  %261 = load <16 x float>, <16 x float>* %260, align 64, !tbaa !32
  %262 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %79, <16 x float> %261, <16 x float> %257)
  %263 = add nsw i64 %173, 288
  %264 = getelementptr inbounds float, float* %7, i64 %263
  %265 = bitcast float* %264 to <16 x float>*
  %266 = load <16 x float>, <16 x float>* %265, align 64, !tbaa !32
  %267 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %82, <16 x float> %266, <16 x float> %262)
  %268 = add nsw i64 %173, 304
  %269 = getelementptr inbounds float, float* %7, i64 %268
  %270 = bitcast float* %269 to <16 x float>*
  %271 = load <16 x float>, <16 x float>* %270, align 64, !tbaa !32
  %272 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %85, <16 x float> %271, <16 x float> %267)
  %273 = add nsw i64 %173, 320
  %274 = getelementptr inbounds float, float* %7, i64 %273
  %275 = bitcast float* %274 to <16 x float>*
  %276 = load <16 x float>, <16 x float>* %275, align 64, !tbaa !32
  %277 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %88, <16 x float> %276, <16 x float> %272)
  %278 = add nsw i64 %173, 336
  %279 = getelementptr inbounds float, float* %7, i64 %278
  %280 = bitcast float* %279 to <16 x float>*
  %281 = load <16 x float>, <16 x float>* %280, align 64, !tbaa !32
  %282 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %91, <16 x float> %281, <16 x float> %277)
  %283 = add nsw i64 %173, 352
  %284 = getelementptr inbounds float, float* %7, i64 %283
  %285 = bitcast float* %284 to <16 x float>*
  %286 = load <16 x float>, <16 x float>* %285, align 64, !tbaa !32
  %287 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %94, <16 x float> %286, <16 x float> %282)
  %288 = add nsw i64 %173, 368
  %289 = getelementptr inbounds float, float* %7, i64 %288
  %290 = bitcast float* %289 to <16 x float>*
  %291 = load <16 x float>, <16 x float>* %290, align 64, !tbaa !32
  %292 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %97, <16 x float> %291, <16 x float> %287)
  %293 = add nsw i64 %173, 384
  %294 = getelementptr inbounds float, float* %7, i64 %293
  %295 = bitcast float* %294 to <16 x float>*
  %296 = load <16 x float>, <16 x float>* %295, align 64, !tbaa !32
  %297 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %100, <16 x float> %296, <16 x float> %292)
  %298 = add nsw i64 %173, 400
  %299 = getelementptr inbounds float, float* %7, i64 %298
  %300 = bitcast float* %299 to <16 x float>*
  %301 = load <16 x float>, <16 x float>* %300, align 64, !tbaa !32
  %302 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %103, <16 x float> %301, <16 x float> %297)
  %303 = add nsw i64 %173, 416
  %304 = getelementptr inbounds float, float* %7, i64 %303
  %305 = bitcast float* %304 to <16 x float>*
  %306 = load <16 x float>, <16 x float>* %305, align 64, !tbaa !32
  %307 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %106, <16 x float> %306, <16 x float> %302)
  %308 = add nsw i64 %173, 432
  %309 = getelementptr inbounds float, float* %7, i64 %308
  %310 = bitcast float* %309 to <16 x float>*
  %311 = load <16 x float>, <16 x float>* %310, align 64, !tbaa !32
  %312 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %109, <16 x float> %311, <16 x float> %307)
  %313 = add nsw i64 %173, 448
  %314 = getelementptr inbounds float, float* %7, i64 %313
  %315 = bitcast float* %314 to <16 x float>*
  %316 = load <16 x float>, <16 x float>* %315, align 64, !tbaa !32
  %317 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %112, <16 x float> %316, <16 x float> %312)
  %318 = add nsw i64 %173, 464
  %319 = getelementptr inbounds float, float* %7, i64 %318
  %320 = bitcast float* %319 to <16 x float>*
  %321 = load <16 x float>, <16 x float>* %320, align 64, !tbaa !32
  %322 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %115, <16 x float> %321, <16 x float> %317)
  %323 = add nsw i64 %173, 480
  %324 = getelementptr inbounds float, float* %7, i64 %323
  %325 = bitcast float* %324 to <16 x float>*
  %326 = load <16 x float>, <16 x float>* %325, align 64, !tbaa !32
  %327 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %118, <16 x float> %326, <16 x float> %322)
  %328 = add nsw i64 %173, 496
  %329 = getelementptr inbounds float, float* %7, i64 %328
  %330 = bitcast float* %329 to <16 x float>*
  %331 = load <16 x float>, <16 x float>* %330, align 64, !tbaa !32
  %332 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %121, <16 x float> %331, <16 x float> %327)
  %333 = add nsw i64 %173, 512
  %334 = getelementptr inbounds float, float* %7, i64 %333
  %335 = bitcast float* %334 to <16 x float>*
  %336 = load <16 x float>, <16 x float>* %335, align 64, !tbaa !32
  %337 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %124, <16 x float> %336, <16 x float> %332)
  %338 = add nsw i64 %173, 528
  %339 = getelementptr inbounds float, float* %7, i64 %338
  %340 = bitcast float* %339 to <16 x float>*
  %341 = load <16 x float>, <16 x float>* %340, align 64, !tbaa !32
  %342 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %127, <16 x float> %341, <16 x float> %337)
  %343 = add nsw i64 %173, 544
  %344 = getelementptr inbounds float, float* %7, i64 %343
  %345 = bitcast float* %344 to <16 x float>*
  %346 = load <16 x float>, <16 x float>* %345, align 64, !tbaa !32
  %347 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %130, <16 x float> %346, <16 x float> %342)
  %348 = add nsw i64 %173, 560
  %349 = getelementptr inbounds float, float* %7, i64 %348
  %350 = bitcast float* %349 to <16 x float>*
  %351 = load <16 x float>, <16 x float>* %350, align 64, !tbaa !32
  %352 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %133, <16 x float> %351, <16 x float> %347)
  %353 = add nsw i64 %173, 576
  %354 = getelementptr inbounds float, float* %7, i64 %353
  %355 = bitcast float* %354 to <16 x float>*
  %356 = load <16 x float>, <16 x float>* %355, align 64, !tbaa !32
  %357 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %136, <16 x float> %356, <16 x float> %352)
  %358 = add nsw i64 %173, 592
  %359 = getelementptr inbounds float, float* %7, i64 %358
  %360 = bitcast float* %359 to <16 x float>*
  %361 = load <16 x float>, <16 x float>* %360, align 64, !tbaa !32
  %362 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %139, <16 x float> %361, <16 x float> %357)
  %363 = add nsw i64 %173, 608
  %364 = getelementptr inbounds float, float* %7, i64 %363
  %365 = bitcast float* %364 to <16 x float>*
  %366 = load <16 x float>, <16 x float>* %365, align 64, !tbaa !32
  %367 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %142, <16 x float> %366, <16 x float> %362)
  %368 = add nsw i64 %173, 624
  %369 = getelementptr inbounds float, float* %7, i64 %368
  %370 = bitcast float* %369 to <16 x float>*
  %371 = load <16 x float>, <16 x float>* %370, align 64, !tbaa !32
  %372 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %145, <16 x float> %371, <16 x float> %367)
  %373 = add nsw i64 %173, 640
  %374 = getelementptr inbounds float, float* %7, i64 %373
  %375 = bitcast float* %374 to <16 x float>*
  %376 = load <16 x float>, <16 x float>* %375, align 64, !tbaa !32
  %377 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %148, <16 x float> %376, <16 x float> %372)
  %378 = add nsw i64 %173, 656
  %379 = getelementptr inbounds float, float* %7, i64 %378
  %380 = bitcast float* %379 to <16 x float>*
  %381 = load <16 x float>, <16 x float>* %380, align 64, !tbaa !32
  %382 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %151, <16 x float> %381, <16 x float> %377)
  %383 = add nsw i64 %173, 672
  %384 = getelementptr inbounds float, float* %7, i64 %383
  %385 = bitcast float* %384 to <16 x float>*
  %386 = load <16 x float>, <16 x float>* %385, align 64, !tbaa !32
  %387 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %154, <16 x float> %386, <16 x float> %382)
  %388 = add nsw i64 %173, 688
  %389 = getelementptr inbounds float, float* %7, i64 %388
  %390 = bitcast float* %389 to <16 x float>*
  %391 = load <16 x float>, <16 x float>* %390, align 64, !tbaa !32
  %392 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %157, <16 x float> %391, <16 x float> %387)
  %393 = add nsw i64 %173, 704
  %394 = getelementptr inbounds float, float* %7, i64 %393
  %395 = bitcast float* %394 to <16 x float>*
  %396 = load <16 x float>, <16 x float>* %395, align 64, !tbaa !32
  %397 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %160, <16 x float> %396, <16 x float> %392)
  %398 = add nsw i64 %173, 720
  %399 = getelementptr inbounds float, float* %7, i64 %398
  %400 = bitcast float* %399 to <16 x float>*
  %401 = load <16 x float>, <16 x float>* %400, align 64, !tbaa !32
  %402 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %163, <16 x float> %401, <16 x float> %397)
  %403 = add nsw i64 %173, 736
  %404 = getelementptr inbounds float, float* %7, i64 %403
  %405 = bitcast float* %404 to <16 x float>*
  %406 = load <16 x float>, <16 x float>* %405, align 64, !tbaa !32
  %407 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %166, <16 x float> %406, <16 x float> %402)
  %408 = add nsw i64 %173, 752
  %409 = getelementptr inbounds float, float* %7, i64 %408
  %410 = bitcast float* %409 to <16 x float>*
  %411 = load <16 x float>, <16 x float>* %410, align 64, !tbaa !32
  %412 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %169, <16 x float> %411, <16 x float> %407)
  %413 = add nsw i64 %173, 768
  %414 = getelementptr inbounds float, float* %7, i64 %413
  %415 = bitcast float* %414 to <16 x float>*
  %416 = load <16 x float>, <16 x float>* %415, align 64, !tbaa !32
  %417 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %172, <16 x float> %416, <16 x float> %412)
  %418 = getelementptr inbounds float, float* %10, i64 %indvars.iv
  %.0.vec.extract = extractelement <16 x float> %417, i32 0
  %419 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %417, i32 1
  %420 = fadd float %.4.vec.extract, %419
  %.8.vec.extract = extractelement <16 x float> %417, i32 2
  %421 = fadd float %.8.vec.extract, %420
  %.12.vec.extract = extractelement <16 x float> %417, i32 3
  %422 = fadd float %.12.vec.extract, %421
  %.16.vec.extract = extractelement <16 x float> %417, i32 4
  %423 = fadd float %.16.vec.extract, %422
  %.20.vec.extract = extractelement <16 x float> %417, i32 5
  %424 = fadd float %.20.vec.extract, %423
  %.24.vec.extract = extractelement <16 x float> %417, i32 6
  %425 = fadd float %.24.vec.extract, %424
  %.28.vec.extract = extractelement <16 x float> %417, i32 7
  %426 = fadd float %.28.vec.extract, %425
  %.32.vec.extract = extractelement <16 x float> %417, i32 8
  %427 = fadd float %.32.vec.extract, %426
  %.36.vec.extract = extractelement <16 x float> %417, i32 9
  %428 = fadd float %.36.vec.extract, %427
  %.40.vec.extract = extractelement <16 x float> %417, i32 10
  %429 = fadd float %.40.vec.extract, %428
  %.44.vec.extract = extractelement <16 x float> %417, i32 11
  %430 = fadd float %.44.vec.extract, %429
  %.48.vec.extract = extractelement <16 x float> %417, i32 12
  %431 = fadd float %.48.vec.extract, %430
  %.52.vec.extract = extractelement <16 x float> %417, i32 13
  %432 = fadd float %.52.vec.extract, %431
  %.56.vec.extract = extractelement <16 x float> %417, i32 14
  %433 = fadd float %.56.vec.extract, %432
  %.60.vec.extract = extractelement <16 x float> %417, i32 15
  %434 = fadd float %.60.vec.extract, %433
  store float %434, float* %418, align 4, !tbaa !23
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %435 = icmp slt i64 %indvars.iv.next, %27
  br i1 %435, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #2

; Function Attrs: nounwind
define dllexport i32 @fused_nn_softmax(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #1 !dbg !35 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !37, metadata !DIExpression()), !dbg !40
  call void @llvm.dbg.value(metadata i8* %1, metadata !38, metadata !DIExpression()), !dbg !40
  call void @llvm.dbg.value(metadata i32 %2, metadata !39, metadata !DIExpression()), !dbg !40
  %3 = bitcast i8* %0 to %1**, !dbg !40
  %4 = load %1*, %1** %3, align 8, !dbg !40
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !40
  %6 = bitcast i8* %5 to %1**, !dbg !40
  %7 = load %1*, %1** %6, align 8, !dbg !40
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !40
  %9 = load i8*, i8** %8, align 8, !dbg !40
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !40
  %11 = load i8*, i8** %10, align 8, !dbg !40
  tail call fastcc void @fused_nn_softmax_compute_(i8* %9, i8* %11), !dbg !40
  ret i32 0, !dbg !40
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_softmax_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %2 = bitcast i8* %0 to <4 x float>*
  %3 = load <4 x float>, <4 x float>* %2, align 4, !tbaa !41
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
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !41
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
  %31 = getelementptr inbounds i8, i8* %0, i64 32
  %32 = bitcast i8* %31 to float*
  %33 = load float, float* %32, align 4, !tbaa !41
  %34 = fcmp ogt float %30, %33
  %35 = select i1 %34, float %30, float %33
  %36 = getelementptr inbounds i8, i8* %0, i64 36
  %37 = bitcast i8* %36 to float*
  %38 = load float, float* %37, align 4, !tbaa !41
  %39 = fcmp ogt float %35, %38
  %40 = select i1 %39, float %35, float %38
  %41 = insertelement <4 x float> undef, float %40, i32 0
  %42 = shufflevector <4 x float> %41, <4 x float> undef, <4 x i32> zeroinitializer
  %43 = fsub <4 x float> %3, %42
  %44 = call <4 x float> @llvm.exp.v4f32(<4 x float> %43)
  %45 = extractelement <4 x float> %44, i32 0
  %46 = fadd float %45, 0.000000e+00
  %47 = extractelement <4 x float> %44, i32 1
  %48 = fadd float %46, %47
  %49 = extractelement <4 x float> %44, i32 2
  %50 = fadd float %48, %49
  %51 = extractelement <4 x float> %44, i32 3
  %52 = fadd float %50, %51
  %53 = fsub <4 x float> %18, %42
  %54 = call <4 x float> @llvm.exp.v4f32(<4 x float> %53)
  %55 = extractelement <4 x float> %54, i32 0
  %56 = fadd float %52, %55
  %57 = extractelement <4 x float> %54, i32 1
  %58 = fadd float %56, %57
  %59 = extractelement <4 x float> %54, i32 2
  %60 = fadd float %58, %59
  %61 = extractelement <4 x float> %54, i32 3
  %62 = fadd float %60, %61
  %63 = fsub float %33, %40
  %64 = tail call float @llvm.exp.f32(float %63)
  %65 = fadd float %62, %64
  %66 = fsub float %38, %40
  %67 = tail call float @llvm.exp.f32(float %66)
  %68 = fadd float %65, %67
  %69 = insertelement <4 x float> undef, float %68, i32 0
  %70 = shufflevector <4 x float> %69, <4 x float> undef, <4 x i32> zeroinitializer
  %71 = fdiv <4 x float> %44, %70
  %72 = bitcast i8* %1 to <4 x float>*
  store <4 x float> %71, <4 x float>* %72, align 4, !tbaa !44
  %73 = getelementptr inbounds i8, i8* %1, i64 16
  %74 = fdiv <4 x float> %54, %70
  %75 = bitcast i8* %73 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !44
  %76 = fdiv float %64, %68
  %77 = getelementptr inbounds i8, i8* %1, i64 32
  %78 = bitcast i8* %77 to float*
  store float %76, float* %78, align 4, !tbaa !44
  %79 = fdiv float %67, %68
  %80 = getelementptr inbounds i8, i8* %1, i64 36
  %81 = bitcast i8* %80 to float*
  store float %79, float* %81, align 4, !tbaa !44
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare float @llvm.exp.f32(float) #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #2

attributes #0 = { noinline }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { noinline nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_nn_dense_add", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!17 = !{!"ctx_ptr", !18, i64 0}
!18 = !{!"tvm-tbaa"}
!19 = !{!"branch_weights", i32 1048576, i32 1}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x555f76bc3c90", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x555f76c5e130", !18, i64 0}
!26 = !{!27, !27, i64 0}
!27 = !{!"float32", !28, i64 0}
!28 = !{!"0x555f76bc3ce0", !18, i64 0}
!29 = !{!30, !30, i64 0}
!30 = !{!"float32", !31, i64 0}
!31 = !{!"0x555f76bc3bf0", !18, i64 0}
!32 = !{!33, !33, i64 0}
!33 = !{!"float32", !34, i64 0}
!34 = !{!"0x555f76bc3c40", !18, i64 0}
!35 = distinct !DISubprogram(name: "fused_nn_softmax", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !36)
!36 = !{!37, !38, !39}
!37 = !DILocalVariable(name: "arg1", arg: 1, scope: !35, file: !1, type: !9)
!38 = !DILocalVariable(name: "arg2", arg: 2, scope: !35, file: !1, type: !9)
!39 = !DILocalVariable(name: "arg3", arg: 3, scope: !35, file: !1, type: !8)
!40 = !DILocation(line: 0, scope: !35)
!41 = !{!42, !42, i64 0}
!42 = !{!"float32", !43, i64 0}
!43 = !{!"0x555f76c5fa80", !18, i64 0}
!44 = !{!45, !45, i64 0}
!45 = !{!"float32", !46, i64 0}
!46 = !{!"0x555f76c54320", !18, i64 0}
