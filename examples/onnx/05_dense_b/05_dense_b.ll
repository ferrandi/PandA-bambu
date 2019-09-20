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
  %4 = alloca [64 x float], align 16
  %5 = alloca %4, align 8
  %.sub = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 0
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
  br i1 %12, label %vector.body, label %call_fail, !prof !19

call_fail:                                        ; preds = %vector.body, %entry
  %merge = phi i32 [ %11, %entry ], [ 0, %vector.body ]
  ret i32 %merge

vector.body:                                      ; preds = %entry
  %13 = bitcast i8* %3 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %13, align 4, !tbaa !20
  %14 = bitcast [64 x float]* %4 to <4 x float>*
  %wide.load1 = load <4 x float>, <4 x float>* %14, align 16, !tbaa !23
  %15 = fadd <4 x float> %wide.load, %wide.load1
  %16 = bitcast i8* %2 to <4 x float>*
  store <4 x float> %15, <4 x float>* %16, align 4, !tbaa !26
  %17 = getelementptr inbounds i8, i8* %3, i64 16
  %18 = bitcast i8* %17 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %18, align 4, !tbaa !20
  %19 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 4
  %20 = bitcast float* %19 to <4 x float>*
  %wide.load1.1 = load <4 x float>, <4 x float>* %20, align 16, !tbaa !23
  %21 = fadd <4 x float> %wide.load.1, %wide.load1.1
  %22 = getelementptr inbounds i8, i8* %2, i64 16
  %23 = bitcast i8* %22 to <4 x float>*
  store <4 x float> %21, <4 x float>* %23, align 4, !tbaa !26
  %24 = getelementptr inbounds i8, i8* %3, i64 32
  %25 = bitcast i8* %24 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !20
  %26 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 8
  %27 = bitcast float* %26 to <4 x float>*
  %wide.load1.2 = load <4 x float>, <4 x float>* %27, align 16, !tbaa !23
  %28 = fadd <4 x float> %wide.load.2, %wide.load1.2
  %29 = getelementptr inbounds i8, i8* %2, i64 32
  %30 = bitcast i8* %29 to <4 x float>*
  store <4 x float> %28, <4 x float>* %30, align 4, !tbaa !26
  %31 = getelementptr inbounds i8, i8* %3, i64 48
  %32 = bitcast i8* %31 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %32, align 4, !tbaa !20
  %33 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 12
  %34 = bitcast float* %33 to <4 x float>*
  %wide.load1.3 = load <4 x float>, <4 x float>* %34, align 16, !tbaa !23
  %35 = fadd <4 x float> %wide.load.3, %wide.load1.3
  %36 = getelementptr inbounds i8, i8* %2, i64 48
  %37 = bitcast i8* %36 to <4 x float>*
  store <4 x float> %35, <4 x float>* %37, align 4, !tbaa !26
  %38 = getelementptr inbounds i8, i8* %3, i64 64
  %39 = bitcast i8* %38 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %39, align 4, !tbaa !20
  %40 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 16
  %41 = bitcast float* %40 to <4 x float>*
  %wide.load1.4 = load <4 x float>, <4 x float>* %41, align 16, !tbaa !23
  %42 = fadd <4 x float> %wide.load.4, %wide.load1.4
  %43 = getelementptr inbounds i8, i8* %2, i64 64
  %44 = bitcast i8* %43 to <4 x float>*
  store <4 x float> %42, <4 x float>* %44, align 4, !tbaa !26
  %45 = getelementptr inbounds i8, i8* %3, i64 80
  %46 = bitcast i8* %45 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %46, align 4, !tbaa !20
  %47 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 20
  %48 = bitcast float* %47 to <4 x float>*
  %wide.load1.5 = load <4 x float>, <4 x float>* %48, align 16, !tbaa !23
  %49 = fadd <4 x float> %wide.load.5, %wide.load1.5
  %50 = getelementptr inbounds i8, i8* %2, i64 80
  %51 = bitcast i8* %50 to <4 x float>*
  store <4 x float> %49, <4 x float>* %51, align 4, !tbaa !26
  %52 = getelementptr inbounds i8, i8* %3, i64 96
  %53 = bitcast i8* %52 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %53, align 4, !tbaa !20
  %54 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 24
  %55 = bitcast float* %54 to <4 x float>*
  %wide.load1.6 = load <4 x float>, <4 x float>* %55, align 16, !tbaa !23
  %56 = fadd <4 x float> %wide.load.6, %wide.load1.6
  %57 = getelementptr inbounds i8, i8* %2, i64 96
  %58 = bitcast i8* %57 to <4 x float>*
  store <4 x float> %56, <4 x float>* %58, align 4, !tbaa !26
  %59 = getelementptr inbounds i8, i8* %3, i64 112
  %60 = bitcast i8* %59 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !20
  %61 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 28
  %62 = bitcast float* %61 to <4 x float>*
  %wide.load1.7 = load <4 x float>, <4 x float>* %62, align 16, !tbaa !23
  %63 = fadd <4 x float> %wide.load.7, %wide.load1.7
  %64 = getelementptr inbounds i8, i8* %2, i64 112
  %65 = bitcast i8* %64 to <4 x float>*
  store <4 x float> %63, <4 x float>* %65, align 4, !tbaa !26
  %66 = getelementptr inbounds i8, i8* %3, i64 128
  %67 = bitcast i8* %66 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %67, align 4, !tbaa !20
  %68 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 32
  %69 = bitcast float* %68 to <4 x float>*
  %wide.load1.8 = load <4 x float>, <4 x float>* %69, align 16, !tbaa !23
  %70 = fadd <4 x float> %wide.load.8, %wide.load1.8
  %71 = getelementptr inbounds i8, i8* %2, i64 128
  %72 = bitcast i8* %71 to <4 x float>*
  store <4 x float> %70, <4 x float>* %72, align 4, !tbaa !26
  %73 = getelementptr inbounds i8, i8* %3, i64 144
  %74 = bitcast i8* %73 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %74, align 4, !tbaa !20
  %75 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 36
  %76 = bitcast float* %75 to <4 x float>*
  %wide.load1.9 = load <4 x float>, <4 x float>* %76, align 16, !tbaa !23
  %77 = fadd <4 x float> %wide.load.9, %wide.load1.9
  %78 = getelementptr inbounds i8, i8* %2, i64 144
  %79 = bitcast i8* %78 to <4 x float>*
  store <4 x float> %77, <4 x float>* %79, align 4, !tbaa !26
  %80 = getelementptr inbounds i8, i8* %3, i64 160
  %81 = bitcast i8* %80 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %81, align 4, !tbaa !20
  %82 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 40
  %83 = bitcast float* %82 to <4 x float>*
  %wide.load1.10 = load <4 x float>, <4 x float>* %83, align 16, !tbaa !23
  %84 = fadd <4 x float> %wide.load.10, %wide.load1.10
  %85 = getelementptr inbounds i8, i8* %2, i64 160
  %86 = bitcast i8* %85 to <4 x float>*
  store <4 x float> %84, <4 x float>* %86, align 4, !tbaa !26
  %87 = getelementptr inbounds i8, i8* %3, i64 176
  %88 = bitcast i8* %87 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %88, align 4, !tbaa !20
  %89 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 44
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load1.11 = load <4 x float>, <4 x float>* %90, align 16, !tbaa !23
  %91 = fadd <4 x float> %wide.load.11, %wide.load1.11
  %92 = getelementptr inbounds i8, i8* %2, i64 176
  %93 = bitcast i8* %92 to <4 x float>*
  store <4 x float> %91, <4 x float>* %93, align 4, !tbaa !26
  %94 = getelementptr inbounds i8, i8* %3, i64 192
  %95 = bitcast i8* %94 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %95, align 4, !tbaa !20
  %96 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 48
  %97 = bitcast float* %96 to <4 x float>*
  %wide.load1.12 = load <4 x float>, <4 x float>* %97, align 16, !tbaa !23
  %98 = fadd <4 x float> %wide.load.12, %wide.load1.12
  %99 = getelementptr inbounds i8, i8* %2, i64 192
  %100 = bitcast i8* %99 to <4 x float>*
  store <4 x float> %98, <4 x float>* %100, align 4, !tbaa !26
  %101 = getelementptr inbounds i8, i8* %3, i64 208
  %102 = bitcast i8* %101 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !20
  %103 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 52
  %104 = bitcast float* %103 to <4 x float>*
  %wide.load1.13 = load <4 x float>, <4 x float>* %104, align 16, !tbaa !23
  %105 = fadd <4 x float> %wide.load.13, %wide.load1.13
  %106 = getelementptr inbounds i8, i8* %2, i64 208
  %107 = bitcast i8* %106 to <4 x float>*
  store <4 x float> %105, <4 x float>* %107, align 4, !tbaa !26
  %108 = getelementptr inbounds i8, i8* %3, i64 224
  %109 = bitcast i8* %108 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %109, align 4, !tbaa !20
  %110 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 56
  %111 = bitcast float* %110 to <4 x float>*
  %wide.load1.14 = load <4 x float>, <4 x float>* %111, align 16, !tbaa !23
  %112 = fadd <4 x float> %wide.load.14, %wide.load1.14
  %113 = getelementptr inbounds i8, i8* %2, i64 224
  %114 = bitcast i8* %113 to <4 x float>*
  store <4 x float> %112, <4 x float>* %114, align 4, !tbaa !26
  %115 = getelementptr inbounds i8, i8* %3, i64 240
  %116 = bitcast i8* %115 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %116, align 4, !tbaa !20
  %117 = getelementptr inbounds [64 x float], [64 x float]* %4, i64 0, i64 60
  %118 = bitcast float* %117 to <4 x float>*
  %wide.load1.15 = load <4 x float>, <4 x float>* %118, align 16, !tbaa !23
  %119 = fadd <4 x float> %wide.load.15, %wide.load1.15
  %120 = getelementptr inbounds i8, i8* %2, i64 240
  %121 = bitcast i8* %120 to <4 x float>*
  store <4 x float> %119, <4 x float>* %121, align 4, !tbaa !26
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = getelementptr inbounds i8, i8* %2, i64 8
  %4 = bitcast i8* %3 to float**
  %5 = load float*, float** %4, align 8
  %6 = getelementptr inbounds i8, i8* %2, i64 16
  %7 = bitcast i8* %6 to float**
  %8 = load float*, float** %7, align 8
  %9 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %10 = load i32, i32* %9, align 4
  %11 = add nsw i32 %10, 63
  %12 = sdiv i32 %11, %10
  %13 = add nsw i32 %0, 1
  %14 = mul nsw i32 %12, %13
  %15 = icmp slt i32 %14, 64
  %16 = select i1 %15, i32 %14, i32 64
  %17 = mul nsw i32 %12, %0
  %18 = icmp slt i32 %17, 64
  %19 = select i1 %18, i32 %17, i32 64
  %20 = icmp slt i32 %19, %16
  br i1 %20, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %21 = bitcast i8* %2 to float**
  %22 = load float*, float** %21, align 8
  %23 = load float, float* %22, align 64, !tbaa !29
  %24 = add i32 %19, 1
  %25 = sext i32 %24 to i64
  %26 = add nsw i64 %25, -1
  %27 = sext i32 %16 to i64
  %28 = add nsw i64 %27, 1
  %29 = sub nsw i64 %28, %25
  %min.iters.check = icmp ult i64 %29, 8
  br i1 %min.iters.check, label %for_body.preheader, label %vector.ph

vector.ph:                                        ; preds = %for_body.lr.ph
  %n.vec = and i64 %29, -8
  %ind.end = add nsw i64 %26, %n.vec
  %broadcast.splatinsert4 = insertelement <4 x float> undef, float %23, i32 0
  %broadcast.splat5 = shufflevector <4 x float> %broadcast.splatinsert4, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert6 = insertelement <4 x float> undef, float %23, i32 0
  %broadcast.splat7 = shufflevector <4 x float> %broadcast.splatinsert6, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %30 = add i64 %26, %index
  %31 = getelementptr inbounds float, float* %5, i64 %30
  %32 = bitcast float* %31 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %32, align 4, !tbaa !43
  %33 = getelementptr float, float* %31, i64 4
  %34 = bitcast float* %33 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %34, align 4, !tbaa !43
  %35 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %broadcast.splat5, <4 x float> %wide.load, <4 x float> zeroinitializer)
  %36 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %broadcast.splat7, <4 x float> %wide.load3, <4 x float> zeroinitializer)
  %37 = getelementptr inbounds float, float* %8, i64 %30
  %38 = fadd <4 x float> %35, zeroinitializer
  %39 = fadd <4 x float> %36, zeroinitializer
  %40 = bitcast float* %37 to <4 x float>*
  store <4 x float> %38, <4 x float>* %40, align 4, !tbaa !23
  %41 = getelementptr float, float* %37, i64 4
  %42 = bitcast float* %41 to <4 x float>*
  store <4 x float> %39, <4 x float>* %42, align 4, !tbaa !23
  %index.next = add i64 %index, 8
  %43 = icmp eq i64 %index.next, %n.vec
  br i1 %43, label %middle.block, label %vector.body, !llvm.loop !46

middle.block:                                     ; preds = %vector.body
  %cmp.n = icmp eq i64 %29, %n.vec
  br i1 %cmp.n, label %for_end, label %for_body.preheader

for_body.preheader:                               ; preds = %middle.block, %for_body.lr.ph
  %indvars.iv.ph = phi i64 [ %26, %for_body.lr.ph ], [ %ind.end, %middle.block ]
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body ], [ %indvars.iv.ph, %for_body.preheader ]
  %44 = getelementptr inbounds float, float* %5, i64 %indvars.iv
  %45 = load float, float* %44, align 4, !tbaa !43
  %46 = tail call float @llvm.fmuladd.f32(float %23, float %45, float 0.000000e+00)
  %47 = getelementptr inbounds float, float* %8, i64 %indvars.iv
  %48 = fadd float %46, 0.000000e+00
  store float %48, float* %47, align 4, !tbaa !23
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %49 = icmp slt i64 %indvars.iv.next, %27
  br i1 %49, label %for_body, label %for_end, !prof !19, !llvm.loop !48

for_end:                                          ; preds = %for_body, %middle.block, %entry
  ret i32 0
}

; Function Attrs: nounwind readnone speculatable
declare float @llvm.fmuladd.f32(float, float, float) #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>) #2

attributes #0 = { noinline }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable }

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
!22 = !{!"0x55e56bf76c50", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x55e56bff9750", !18, i64 0}
!26 = !{!27, !27, i64 0}
!27 = !{!"float32", !28, i64 0}
!28 = !{!"0x55e56bf76ca0", !18, i64 0}
!29 = !{!30, !30, i64 0}
!30 = !{!"0x55e56bf56d00.w1.b0", !31, i64 0}
!31 = !{!"0x55e56bf56d00.w2.b0", !32, i64 0}
!32 = !{!"0x55e56bf56d00.w4.b0", !33, i64 0}
!33 = !{!"0x55e56bf56d00.w8.b0", !34, i64 0}
!34 = !{!"0x55e56bf56d00.w16.b0", !35, i64 0}
!35 = !{!"0x55e56bf56d00.w32.b0", !36, i64 0}
!36 = !{!"0x55e56bf56d00.w64.b0", !37, i64 0}
!37 = !{!"0x55e56bf56d00.w128.b0", !38, i64 0}
!38 = !{!"0x55e56bf56d00.w256.b0", !39, i64 0}
!39 = !{!"0x55e56bf56d00.w512.b0", !40, i64 0}
!40 = !{!"0x55e56bf56d00.w1024.b0", !41, i64 0}
!41 = !{!"float32", !42, i64 0}
!42 = !{!"0x55e56bf56d00", !18, i64 0}
!43 = !{!44, !44, i64 0}
!44 = !{!"float32", !45, i64 0}
!45 = !{!"0x55e56bf76c00", !18, i64 0}
!46 = distinct !{!46, !47}
!47 = !{!"llvm.loop.isvectorized", i32 1}
!48 = distinct !{!48, !49, !47}
!49 = !{!"llvm.loop.unroll.runtime.disable"}
