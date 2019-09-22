; ModuleID = 'fused_layout_transform_9'
source_filename = "fused_layout_transform_9"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i32*, i32 }
%1 = type { i8*, %2, i32, %3, i64*, i64*, i64 }
%2 = type { i32, i32 }
%3 = type { i8, i8, i16 }
%4 = type { i8*, i8* }
%5 = type { i8*, i8* }
%6 = type { i8*, i8*, i8* }
%7 = type { i8*, i8*, i8* }
%8 = type { i8*, i8*, i8* }
%9 = type { i8*, i8* }
%10 = type { i8*, i8*, i8*, i8* }
%11 = type { i8*, i8* }
%12 = type { i8*, i8* }
%13 = type { i8*, i8*, i8*, i8* }
%14 = type { i8*, i8* }
%15 = type { i8*, i8*, i8*, i8*, i32 }
%16 = type { i8*, i8* }
%17 = type { i8*, i8*, i8*, i8* }
%18 = type { i8*, i8* }
%19 = type { i8*, i8*, i8*, i8* }
%20 = type { i8*, i8* }

@__TVMBackendParallelLaunch = linkonce dllexport local_unnamed_addr global i32 (i32 (i32, %0*, i8*)*, i8*, i32)* null, align 8
@__TVMBackendAllocWorkspace = linkonce dllexport local_unnamed_addr global i8* (i32, i32, i64, i32, i32)* null, align 8
@__TVMBackendFreeWorkspace = linkonce dllexport local_unnamed_addr global i32 (i32, i32, i8*)* null, align 8
@__tvm_main__ = weak local_unnamed_addr constant [25 x i8] c"fused_layout_transform_9\00", align 1

define dllexport i32 @fused_layout_transform_9(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !12, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i8* %1, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !15
  %3 = bitcast i8* %0 to %1**, !dbg !15
  %4 = load %1*, %1** %3, align 8, !dbg !15
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !15
  %6 = bitcast i8* %5 to %1**, !dbg !15
  %7 = load %1*, %1** %6, align 8, !dbg !15
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !15
  %9 = load i8*, i8** %8, align 8, !dbg !15
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !15
  %11 = load i8*, i8** %10, align 8, !dbg !15
  %12 = tail call fastcc i32 @fused_layout_transform_9_compute_(i8* %11, i8* %9), !dbg !15
  ret i32 %12, !dbg !15
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_9_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %4, align 8
  %3 = getelementptr inbounds %4, %4* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %4, %4* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %4* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda, i8* nonnull %5, i32 5)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 223
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 224
  %15 = select i1 %14, i32 %13, i32 224
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 224
  %18 = select i1 %17, i32 %16, i32 224
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  %24 = xor i32 %16, -1
  %25 = icmp sgt i32 %24, -225
  %smax = select i1 %25, i32 %24, i32 -225
  %26 = mul i32 %smax, 224
  %27 = sub i32 -224, %26
  %28 = sub i32 49952, %26
  %29 = sub i32 100128, %26
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %indvars.iv7 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next8, %for_end3 ]
  %30 = mul i32 %indvar, 224
  %31 = mul nsw i64 %indvars.iv7, 672
  %32 = trunc i64 %indvars.iv7 to i32
  %33 = mul i32 %32, 224
  %34 = add i32 %29, %30
  %35 = add i32 %28, %30
  %36 = add i32 %27, %30
  %37 = icmp sgt i32 %36, 2147483424
  %38 = icmp sgt i32 %35, 2147483424
  %39 = or i1 %37, %38
  %40 = icmp sgt i32 %34, 2147483424
  %41 = or i1 %39, %40
  br i1 %41, label %for_body2.preheader, label %vector.body.preheader

vector.body.preheader:                            ; preds = %for_body
  br label %vector.body

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

vector.body:                                      ; preds = %vector.body.preheader, %vector.body
  %index = phi i64 [ %index.next, %vector.body ], [ 0, %vector.body.preheader ]
  %42 = mul nuw nsw i64 %index, 3
  %43 = add nsw i64 %42, %31
  %44 = trunc i64 %index to i32
  %45 = add i32 %33, %44
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds float, float* %7, i64 %46
  %48 = bitcast float* %47 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %48, align 4, !tbaa !20
  %49 = add i32 %45, 50176
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to <4 x i32>*
  %wide.load18 = load <4 x i32>, <4 x i32>* %52, align 4, !tbaa !20
  %53 = or i64 %43, 2
  %54 = add i32 %45, 100352
  %55 = sext i32 %54 to i64
  %56 = getelementptr inbounds float, float* %7, i64 %55
  %57 = bitcast float* %56 to <4 x i32>*
  %wide.load19 = load <4 x i32>, <4 x i32>* %57, align 4, !tbaa !20
  %58 = getelementptr inbounds float, float* %4, i64 %53
  %59 = getelementptr float, float* %58, i64 -2
  %60 = bitcast float* %59 to <12 x i32>*
  %61 = shufflevector <4 x i32> %wide.load, <4 x i32> %wide.load18, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %62 = shufflevector <4 x i32> %wide.load19, <4 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef>
  %interleaved.vec = shufflevector <8 x i32> %61, <8 x i32> %62, <12 x i32> <i32 0, i32 4, i32 8, i32 1, i32 5, i32 9, i32 2, i32 6, i32 10, i32 3, i32 7, i32 11>
  store <12 x i32> %interleaved.vec, <12 x i32>* %60, align 4, !tbaa !23
  %index.next = add i64 %index, 4
  %63 = icmp eq i64 %index.next, 224
  br i1 %63, label %for_end3, label %vector.body, !llvm.loop !26

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %64 = mul nuw nsw i64 %indvars.iv, 3
  %65 = add nsw i64 %64, %31
  %66 = trunc i64 %indvars.iv to i32
  %67 = add i32 %33, %66
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds float, float* %7, i64 %68
  %70 = bitcast float* %69 to i32*
  %71 = load i32, i32* %70, align 4, !tbaa !20
  %72 = getelementptr inbounds float, float* %4, i64 %65
  %73 = bitcast float* %72 to i32*
  store i32 %71, i32* %73, align 4, !tbaa !23
  %74 = add nsw i64 %65, 1
  %75 = add i32 %67, 50176
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %7, i64 %76
  %78 = bitcast float* %77 to i32*
  %79 = load i32, i32* %78, align 4, !tbaa !20
  %80 = getelementptr inbounds float, float* %4, i64 %74
  %81 = bitcast float* %80 to i32*
  store i32 %79, i32* %81, align 4, !tbaa !23
  %82 = add nsw i64 %65, 2
  %83 = add i32 %67, 100352
  %84 = sext i32 %83 to i64
  %85 = getelementptr inbounds float, float* %7, i64 %84
  %86 = bitcast float* %85 to i32*
  %87 = load i32, i32* %86, align 4, !tbaa !20
  %88 = getelementptr inbounds float, float* %4, i64 %82
  %89 = bitcast float* %88 to i32*
  store i32 %87, i32* %89, align 4, !tbaa !23
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 224
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28, !llvm.loop !29

for_end3:                                         ; preds = %vector.body, %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %90 = icmp slt i64 %indvars.iv.next8, %23
  %indvar.next = add i32 %indvar, 1
  br i1 %90, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_max_pool2d_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !30 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !32, metadata !DIExpression()), !dbg !35
  call void @llvm.dbg.value(metadata i8* %1, metadata !33, metadata !DIExpression()), !dbg !35
  call void @llvm.dbg.value(metadata i32 %2, metadata !34, metadata !DIExpression()), !dbg !35
  %3 = bitcast i8* %0 to %1**, !dbg !35
  %4 = load %1*, %1** %3, align 8, !dbg !35
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !35
  %6 = bitcast i8* %5 to %1**, !dbg !35
  %7 = load %1*, %1** %6, align 8, !dbg !35
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !35
  %9 = load i8*, i8** %8, align 8, !dbg !35
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !35
  %11 = load i8*, i8** %10, align 8, !dbg !35
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_2_compute_(i8* %11, i8* %9), !dbg !35
  ret i32 %12, !dbg !35
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_2_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %5, align 8
  %3 = getelementptr inbounds %5, %5* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %5, %5* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %5* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.1, i8* nonnull %5, i32 5)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.1(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 215
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 216
  %15 = select i1 %14, i32 %13, i32 216
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 216
  %18 = select i1 %17, i32 %16, i32 216
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv7 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next8, %for_end3 ]
  %24 = mul nsw i64 %indvars.iv7, 216
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = srem i32 %25, 27
  %27 = mul nsw i32 %26, 880
  %28 = sdiv i32 %25, 27
  %29 = mul nsw i32 %28, 24200
  %30 = add nsw i32 %29, %27
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %31 = shl i64 %indvars.iv, 3
  %32 = add nsw i64 %31, %24
  %33 = getelementptr inbounds float, float* %4, i64 %32
  %34 = bitcast float* %33 to <8 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %35 = shl i32 %indvars.iv.tr, 4
  %36 = add i32 %30, %35
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to <8 x float>*
  %40 = load <8 x float>, <8 x float>* %39, align 32, !tbaa !36
  %41 = fcmp olt <8 x float> %40, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %42 = select <8 x i1> %41, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %40
  %43 = add i32 %36, 8
  %44 = sext i32 %43 to i64
  %45 = getelementptr inbounds float, float* %7, i64 %44
  %46 = bitcast float* %45 to <8 x float>*
  %47 = load <8 x float>, <8 x float>* %46, align 32, !tbaa !36
  %48 = fcmp ogt <8 x float> %42, %47
  %49 = select <8 x i1> %48, <8 x float> %42, <8 x float> %47
  %50 = add i32 %36, 16
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to <8 x float>*
  %54 = load <8 x float>, <8 x float>* %53, align 32, !tbaa !36
  %55 = fcmp ogt <8 x float> %49, %54
  %56 = select <8 x i1> %55, <8 x float> %49, <8 x float> %54
  %57 = add i32 %36, 440
  %58 = sext i32 %57 to i64
  %59 = getelementptr inbounds float, float* %7, i64 %58
  %60 = bitcast float* %59 to <8 x float>*
  %61 = load <8 x float>, <8 x float>* %60, align 32, !tbaa !36
  %62 = fcmp ogt <8 x float> %56, %61
  %63 = select <8 x i1> %62, <8 x float> %56, <8 x float> %61
  %64 = add i32 %36, 448
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to <8 x float>*
  %68 = load <8 x float>, <8 x float>* %67, align 32, !tbaa !36
  %69 = fcmp ogt <8 x float> %63, %68
  %70 = select <8 x i1> %69, <8 x float> %63, <8 x float> %68
  %71 = add i32 %36, 456
  %72 = sext i32 %71 to i64
  %73 = getelementptr inbounds float, float* %7, i64 %72
  %74 = bitcast float* %73 to <8 x float>*
  %75 = load <8 x float>, <8 x float>* %74, align 32, !tbaa !36
  %76 = fcmp ogt <8 x float> %70, %75
  %77 = select <8 x i1> %76, <8 x float> %70, <8 x float> %75
  %78 = add i32 %36, 880
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to <8 x float>*
  %82 = load <8 x float>, <8 x float>* %81, align 32, !tbaa !36
  %83 = fcmp ogt <8 x float> %77, %82
  %84 = select <8 x i1> %83, <8 x float> %77, <8 x float> %82
  %85 = add i32 %36, 888
  %86 = sext i32 %85 to i64
  %87 = getelementptr inbounds float, float* %7, i64 %86
  %88 = bitcast float* %87 to <8 x float>*
  %89 = load <8 x float>, <8 x float>* %88, align 32, !tbaa !36
  %90 = fcmp ogt <8 x float> %84, %89
  %91 = select <8 x i1> %90, <8 x float> %84, <8 x float> %89
  %92 = add i32 %36, 896
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to <8 x float>*
  %96 = load <8 x float>, <8 x float>* %95, align 32, !tbaa !36
  %97 = fcmp ogt <8 x float> %91, %96
  %98 = select <8 x i1> %97, <8 x float> %91, <8 x float> %96
  store <8 x float> %98, <8 x float>* %34, align 32, !tbaa !39
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 27
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %99 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %99, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_dense_add_nn_relu_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !42 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !44, metadata !DIExpression()), !dbg !47
  call void @llvm.dbg.value(metadata i8* %1, metadata !45, metadata !DIExpression()), !dbg !47
  call void @llvm.dbg.value(metadata i32 %2, metadata !46, metadata !DIExpression()), !dbg !47
  %3 = bitcast i8* %0 to %1**, !dbg !47
  %4 = load %1*, %1** %3, align 8, !dbg !47
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !47
  %6 = bitcast i8* %5 to %1**, !dbg !47
  %7 = load %1*, %1** %6, align 8, !dbg !47
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !47
  %9 = bitcast i8* %8 to %1**, !dbg !47
  %10 = load %1*, %1** %9, align 8, !dbg !47
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !47
  %12 = bitcast i8* %11 to %1**, !dbg !47
  %13 = load %1*, %1** %12, align 8, !dbg !47
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !47
  %15 = load i8*, i8** %14, align 8, !dbg !47
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !47
  %17 = load i32, i32* %16, align 4, !dbg !47
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !47
  %19 = load i8*, i8** %18, align 8, !dbg !47
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !47
  %21 = load i8*, i8** %20, align 8, !dbg !47
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !47
  %23 = load i8*, i8** %22, align 8, !dbg !47
  %24 = tail call fastcc i32 @fused_nn_dense_add_nn_relu_1_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !47
  ret i32 %24, !dbg !47
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_nn_relu_1_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 16384, i32 2, i32 32)
  %7 = alloca %6, align 8
  %8 = getelementptr inbounds %6, %6* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %6, %6* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %6, %6* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %6* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.2, i8* nonnull %11, i32 5)
  %14 = icmp eq i32 %13, 0
  br i1 %14, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %for_end, %entry
  %merge = phi i32 [ %13, %entry ], [ 0, %for_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %15 = bitcast i8* %3 to float*
  %16 = bitcast i8* %6 to float*
  %17 = bitcast i8* %2 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %call_end
  %index = phi i64 [ 0, %call_end ], [ %index.next, %vector.body ]
  %18 = getelementptr inbounds float, float* %15, i64 %index
  %19 = bitcast float* %18 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !48
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !48
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !51
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !51
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = fcmp ogt <4 x float> %26, zeroinitializer
  %29 = fcmp ogt <4 x float> %27, zeroinitializer
  %30 = select <4 x i1> %28, <4 x float> %26, <4 x float> zeroinitializer
  %31 = select <4 x i1> %29, <4 x float> %27, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %17, i64 %index
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %30, <4 x float>* %33, align 4, !tbaa !54
  %34 = getelementptr float, float* %32, i64 4
  %35 = bitcast float* %34 to <4 x float>*
  store <4 x float> %31, <4 x float>* %35, align 4, !tbaa !54
  %index.next = add i64 %index, 8
  %36 = icmp eq i64 %index.next, 4096
  br i1 %36, label %for_end, label %vector.body, !llvm.loop !57

for_end:                                          ; preds = %vector.body
  %37 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %38 = call i32 %37(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.2(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
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
  %13 = add nsw i32 %12, 4095
  %14 = sdiv i32 %13, %12
  %15 = add nsw i32 %0, 1
  %16 = mul nsw i32 %14, %15
  %17 = icmp slt i32 %16, 4096
  %18 = select i1 %17, i32 %16, i32 4096
  %19 = mul nsw i32 %14, %0
  %20 = icmp slt i32 %19, 4096
  %21 = select i1 %20, i32 %19, i32 4096
  %22 = icmp slt i32 %21, %18
  br i1 %22, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %23 = add i32 %21, 1
  %24 = sext i32 %23 to i64
  %25 = add nsw i64 %24, -1
  %26 = sext i32 %18 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv6 = phi i64 [ %25, %for_body.lr.ph ], [ %indvars.iv.next7, %for_end3 ]
  %27 = mul nsw i64 %indvars.iv6, 9216
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %.05 = phi <16 x float> [ zeroinitializer, %for_body ], [ %36, %for_body2 ]
  %28 = shl nsw i64 %indvars.iv, 4
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <16 x float>*
  %31 = load <16 x float>, <16 x float>* %30, align 64, !tbaa !58
  %32 = add nsw i64 %28, %27
  %33 = getelementptr inbounds float, float* %7, i64 %32
  %34 = bitcast float* %33 to <16 x float>*
  %35 = load <16 x float>, <16 x float>* %34, align 64, !tbaa !61
  %36 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %31, <16 x float> %35, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 576
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %37 = getelementptr inbounds float, float* %10, i64 %indvars.iv6
  %.0.vec.extract = extractelement <16 x float> %36, i32 0
  %38 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %36, i32 1
  %39 = fadd float %.4.vec.extract, %38
  %.8.vec.extract = extractelement <16 x float> %36, i32 2
  %40 = fadd float %.8.vec.extract, %39
  %.12.vec.extract = extractelement <16 x float> %36, i32 3
  %41 = fadd float %.12.vec.extract, %40
  %.16.vec.extract = extractelement <16 x float> %36, i32 4
  %42 = fadd float %.16.vec.extract, %41
  %.20.vec.extract = extractelement <16 x float> %36, i32 5
  %43 = fadd float %.20.vec.extract, %42
  %.24.vec.extract = extractelement <16 x float> %36, i32 6
  %44 = fadd float %.24.vec.extract, %43
  %.28.vec.extract = extractelement <16 x float> %36, i32 7
  %45 = fadd float %.28.vec.extract, %44
  %.32.vec.extract = extractelement <16 x float> %36, i32 8
  %46 = fadd float %.32.vec.extract, %45
  %.36.vec.extract = extractelement <16 x float> %36, i32 9
  %47 = fadd float %.36.vec.extract, %46
  %.40.vec.extract = extractelement <16 x float> %36, i32 10
  %48 = fadd float %.40.vec.extract, %47
  %.44.vec.extract = extractelement <16 x float> %36, i32 11
  %49 = fadd float %.44.vec.extract, %48
  %.48.vec.extract = extractelement <16 x float> %36, i32 12
  %50 = fadd float %.48.vec.extract, %49
  %.52.vec.extract = extractelement <16 x float> %36, i32 13
  %51 = fadd float %.52.vec.extract, %50
  %.56.vec.extract = extractelement <16 x float> %36, i32 14
  %52 = fadd float %.56.vec.extract, %51
  %.60.vec.extract = extractelement <16 x float> %36, i32 15
  %53 = fadd float %.60.vec.extract, %52
  store float %53, float* %37, align 4, !tbaa !51
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %54 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %54, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #3

define dllexport i32 @fused_nn_dense_add(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !64 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !66, metadata !DIExpression()), !dbg !69
  call void @llvm.dbg.value(metadata i8* %1, metadata !67, metadata !DIExpression()), !dbg !69
  call void @llvm.dbg.value(metadata i32 %2, metadata !68, metadata !DIExpression()), !dbg !69
  %3 = bitcast i8* %0 to %1**, !dbg !69
  %4 = load %1*, %1** %3, align 8, !dbg !69
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !69
  %6 = bitcast i8* %5 to %1**, !dbg !69
  %7 = load %1*, %1** %6, align 8, !dbg !69
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !69
  %9 = bitcast i8* %8 to %1**, !dbg !69
  %10 = load %1*, %1** %9, align 8, !dbg !69
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !69
  %12 = bitcast i8* %11 to %1**, !dbg !69
  %13 = load %1*, %1** %12, align 8, !dbg !69
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !69
  %15 = load i8*, i8** %14, align 8, !dbg !69
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !69
  %17 = load i32, i32* %16, align 4, !dbg !69
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !69
  %19 = load i8*, i8** %18, align 8, !dbg !69
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !69
  %21 = load i8*, i8** %20, align 8, !dbg !69
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !69
  %23 = load i8*, i8** %22, align 8, !dbg !69
  %24 = tail call fastcc i32 @fused_nn_dense_add_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !69
  ret i32 %24, !dbg !69
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 4000, i32 2, i32 32)
  %7 = alloca %7, align 8
  %8 = getelementptr inbounds %7, %7* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %7, %7* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %7, %7* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %7* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.3, i8* nonnull %11, i32 5)
  %14 = icmp eq i32 %13, 0
  br i1 %14, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %for_end, %entry
  %merge = phi i32 [ %13, %entry ], [ 0, %for_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %15 = bitcast i8* %3 to float*
  %16 = bitcast i8* %6 to float*
  %17 = bitcast i8* %2 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %call_end
  %index = phi i64 [ 0, %call_end ], [ %index.next, %vector.body ]
  %18 = getelementptr inbounds float, float* %15, i64 %index
  %19 = bitcast float* %18 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !70
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !70
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !73
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !73
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = getelementptr inbounds float, float* %17, i64 %index
  %29 = bitcast float* %28 to <4 x float>*
  store <4 x float> %26, <4 x float>* %29, align 4, !tbaa !76
  %30 = getelementptr float, float* %28, i64 4
  %31 = bitcast float* %30 to <4 x float>*
  store <4 x float> %27, <4 x float>* %31, align 4, !tbaa !76
  %index.next = add i64 %index, 8
  %32 = icmp eq i64 %index.next, 1000
  br i1 %32, label %for_end, label %vector.body, !llvm.loop !79

for_end:                                          ; preds = %vector.body
  %33 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %34 = call i32 %33(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.3(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
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
  %13 = add nsw i32 %12, 999
  %14 = sdiv i32 %13, %12
  %15 = add nsw i32 %0, 1
  %16 = mul nsw i32 %14, %15
  %17 = icmp slt i32 %16, 1000
  %18 = select i1 %17, i32 %16, i32 1000
  %19 = mul nsw i32 %14, %0
  %20 = icmp slt i32 %19, 1000
  %21 = select i1 %20, i32 %19, i32 1000
  %22 = icmp slt i32 %21, %18
  br i1 %22, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %23 = add i32 %21, 1
  %24 = sext i32 %23 to i64
  %25 = add nsw i64 %24, -1
  %26 = sext i32 %18 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv6 = phi i64 [ %25, %for_body.lr.ph ], [ %indvars.iv.next7, %for_end3 ]
  %27 = trunc i64 %indvars.iv6 to i32
  %28 = shl i32 %27, 12
  %29 = sext i32 %28 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %.05 = phi <16 x float> [ zeroinitializer, %for_body ], [ %38, %for_body2 ]
  %30 = shl nsw i64 %indvars.iv, 4
  %31 = getelementptr inbounds float, float* %4, i64 %30
  %32 = bitcast float* %31 to <16 x float>*
  %33 = load <16 x float>, <16 x float>* %32, align 64, !tbaa !80
  %34 = add nsw i64 %30, %29
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <16 x float>*
  %37 = load <16 x float>, <16 x float>* %36, align 64, !tbaa !83
  %38 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %33, <16 x float> %37, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %39 = getelementptr inbounds float, float* %10, i64 %indvars.iv6
  %.0.vec.extract = extractelement <16 x float> %38, i32 0
  %40 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %38, i32 1
  %41 = fadd float %.4.vec.extract, %40
  %.8.vec.extract = extractelement <16 x float> %38, i32 2
  %42 = fadd float %.8.vec.extract, %41
  %.12.vec.extract = extractelement <16 x float> %38, i32 3
  %43 = fadd float %.12.vec.extract, %42
  %.16.vec.extract = extractelement <16 x float> %38, i32 4
  %44 = fadd float %.16.vec.extract, %43
  %.20.vec.extract = extractelement <16 x float> %38, i32 5
  %45 = fadd float %.20.vec.extract, %44
  %.24.vec.extract = extractelement <16 x float> %38, i32 6
  %46 = fadd float %.24.vec.extract, %45
  %.28.vec.extract = extractelement <16 x float> %38, i32 7
  %47 = fadd float %.28.vec.extract, %46
  %.32.vec.extract = extractelement <16 x float> %38, i32 8
  %48 = fadd float %.32.vec.extract, %47
  %.36.vec.extract = extractelement <16 x float> %38, i32 9
  %49 = fadd float %.36.vec.extract, %48
  %.40.vec.extract = extractelement <16 x float> %38, i32 10
  %50 = fadd float %.40.vec.extract, %49
  %.44.vec.extract = extractelement <16 x float> %38, i32 11
  %51 = fadd float %.44.vec.extract, %50
  %.48.vec.extract = extractelement <16 x float> %38, i32 12
  %52 = fadd float %.48.vec.extract, %51
  %.52.vec.extract = extractelement <16 x float> %38, i32 13
  %53 = fadd float %.52.vec.extract, %52
  %.56.vec.extract = extractelement <16 x float> %38, i32 14
  %54 = fadd float %.56.vec.extract, %53
  %.60.vec.extract = extractelement <16 x float> %38, i32 15
  %55 = fadd float %.60.vec.extract, %54
  store float %55, float* %39, align 4, !tbaa !73
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %56 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %56, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_dense_add_nn_relu(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !86 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !88, metadata !DIExpression()), !dbg !91
  call void @llvm.dbg.value(metadata i8* %1, metadata !89, metadata !DIExpression()), !dbg !91
  call void @llvm.dbg.value(metadata i32 %2, metadata !90, metadata !DIExpression()), !dbg !91
  %3 = bitcast i8* %0 to %1**, !dbg !91
  %4 = load %1*, %1** %3, align 8, !dbg !91
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !91
  %6 = bitcast i8* %5 to %1**, !dbg !91
  %7 = load %1*, %1** %6, align 8, !dbg !91
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !91
  %9 = bitcast i8* %8 to %1**, !dbg !91
  %10 = load %1*, %1** %9, align 8, !dbg !91
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !91
  %12 = bitcast i8* %11 to %1**, !dbg !91
  %13 = load %1*, %1** %12, align 8, !dbg !91
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !91
  %15 = load i8*, i8** %14, align 8, !dbg !91
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !91
  %17 = load i32, i32* %16, align 4, !dbg !91
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !91
  %19 = load i8*, i8** %18, align 8, !dbg !91
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !91
  %21 = load i8*, i8** %20, align 8, !dbg !91
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !91
  %23 = load i8*, i8** %22, align 8, !dbg !91
  %24 = tail call fastcc i32 @fused_nn_dense_add_nn_relu_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !91
  ret i32 %24, !dbg !91
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_nn_relu_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 16384, i32 2, i32 32)
  %7 = alloca %8, align 8
  %8 = getelementptr inbounds %8, %8* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %8, %8* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %8, %8* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %8* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.4, i8* nonnull %11, i32 5)
  %14 = icmp eq i32 %13, 0
  br i1 %14, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %for_end, %entry
  %merge = phi i32 [ %13, %entry ], [ 0, %for_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %15 = bitcast i8* %3 to float*
  %16 = bitcast i8* %6 to float*
  %17 = bitcast i8* %2 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %call_end
  %index = phi i64 [ 0, %call_end ], [ %index.next, %vector.body ]
  %18 = getelementptr inbounds float, float* %15, i64 %index
  %19 = bitcast float* %18 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !92
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !92
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !95
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !95
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = fcmp ogt <4 x float> %26, zeroinitializer
  %29 = fcmp ogt <4 x float> %27, zeroinitializer
  %30 = select <4 x i1> %28, <4 x float> %26, <4 x float> zeroinitializer
  %31 = select <4 x i1> %29, <4 x float> %27, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %17, i64 %index
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %30, <4 x float>* %33, align 4, !tbaa !98
  %34 = getelementptr float, float* %32, i64 4
  %35 = bitcast float* %34 to <4 x float>*
  store <4 x float> %31, <4 x float>* %35, align 4, !tbaa !98
  %index.next = add i64 %index, 8
  %36 = icmp eq i64 %index.next, 4096
  br i1 %36, label %for_end, label %vector.body, !llvm.loop !101

for_end:                                          ; preds = %vector.body
  %37 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %38 = call i32 %37(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.4(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
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
  %13 = add nsw i32 %12, 4095
  %14 = sdiv i32 %13, %12
  %15 = add nsw i32 %0, 1
  %16 = mul nsw i32 %14, %15
  %17 = icmp slt i32 %16, 4096
  %18 = select i1 %17, i32 %16, i32 4096
  %19 = mul nsw i32 %14, %0
  %20 = icmp slt i32 %19, 4096
  %21 = select i1 %20, i32 %19, i32 4096
  %22 = icmp slt i32 %21, %18
  br i1 %22, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %23 = add i32 %21, 1
  %24 = sext i32 %23 to i64
  %25 = add nsw i64 %24, -1
  %26 = sext i32 %18 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv6 = phi i64 [ %25, %for_body.lr.ph ], [ %indvars.iv.next7, %for_end3 ]
  %27 = trunc i64 %indvars.iv6 to i32
  %28 = shl i32 %27, 12
  %29 = sext i32 %28 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %.05 = phi <16 x float> [ zeroinitializer, %for_body ], [ %38, %for_body2 ]
  %30 = shl nsw i64 %indvars.iv, 4
  %31 = getelementptr inbounds float, float* %4, i64 %30
  %32 = bitcast float* %31 to <16 x float>*
  %33 = load <16 x float>, <16 x float>* %32, align 64, !tbaa !102
  %34 = add nsw i64 %30, %29
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <16 x float>*
  %37 = load <16 x float>, <16 x float>* %36, align 64, !tbaa !105
  %38 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %33, <16 x float> %37, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %39 = getelementptr inbounds float, float* %10, i64 %indvars.iv6
  %.0.vec.extract = extractelement <16 x float> %38, i32 0
  %40 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %38, i32 1
  %41 = fadd float %.4.vec.extract, %40
  %.8.vec.extract = extractelement <16 x float> %38, i32 2
  %42 = fadd float %.8.vec.extract, %41
  %.12.vec.extract = extractelement <16 x float> %38, i32 3
  %43 = fadd float %.12.vec.extract, %42
  %.16.vec.extract = extractelement <16 x float> %38, i32 4
  %44 = fadd float %.16.vec.extract, %43
  %.20.vec.extract = extractelement <16 x float> %38, i32 5
  %45 = fadd float %.20.vec.extract, %44
  %.24.vec.extract = extractelement <16 x float> %38, i32 6
  %46 = fadd float %.24.vec.extract, %45
  %.28.vec.extract = extractelement <16 x float> %38, i32 7
  %47 = fadd float %.28.vec.extract, %46
  %.32.vec.extract = extractelement <16 x float> %38, i32 8
  %48 = fadd float %.32.vec.extract, %47
  %.36.vec.extract = extractelement <16 x float> %38, i32 9
  %49 = fadd float %.36.vec.extract, %48
  %.40.vec.extract = extractelement <16 x float> %38, i32 10
  %50 = fadd float %.40.vec.extract, %49
  %.44.vec.extract = extractelement <16 x float> %38, i32 11
  %51 = fadd float %.44.vec.extract, %50
  %.48.vec.extract = extractelement <16 x float> %38, i32 12
  %52 = fadd float %.48.vec.extract, %51
  %.52.vec.extract = extractelement <16 x float> %38, i32 13
  %53 = fadd float %.52.vec.extract, %52
  %.56.vec.extract = extractelement <16 x float> %38, i32 14
  %54 = fadd float %.56.vec.extract, %53
  %.60.vec.extract = extractelement <16 x float> %38, i32 15
  %55 = fadd float %.60.vec.extract, %54
  store float %55, float* %39, align 4, !tbaa !95
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %56 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %56, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !108 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !110, metadata !DIExpression()), !dbg !113
  call void @llvm.dbg.value(metadata i8* %1, metadata !111, metadata !DIExpression()), !dbg !113
  call void @llvm.dbg.value(metadata i32 %2, metadata !112, metadata !DIExpression()), !dbg !113
  %3 = bitcast i8* %0 to %1**, !dbg !113
  %4 = load %1*, %1** %3, align 8, !dbg !113
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !113
  %6 = bitcast i8* %5 to %1**, !dbg !113
  %7 = load %1*, %1** %6, align 8, !dbg !113
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !113
  %9 = bitcast i8* %8 to %1**, !dbg !113
  %10 = load %1*, %1** %9, align 8, !dbg !113
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !113
  %12 = bitcast i8* %11 to %1**, !dbg !113
  %13 = load %1*, %1** %12, align 8, !dbg !113
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !113
  %15 = load i8*, i8** %14, align 8, !dbg !113
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !113
  %17 = load i32, i32* %16, align 4, !dbg !113
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !113
  %19 = load i8*, i8** %18, align 8, !dbg !113
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !113
  %21 = load i8*, i8** %20, align 8, !dbg !113
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !113
  %23 = load i8*, i8** %22, align 8, !dbg !113
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !113
  ret i32 %24, !dbg !113
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 172800, i32 2, i32 32)
  %7 = alloca %9, align 8
  %8 = getelementptr inbounds %9, %9* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %9, %9* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %9* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.5, i8* nonnull %10, i32 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %21, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %10, align 8
  %15 = getelementptr inbounds %10, %10* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %10, %10* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %10, %10* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %10, %10* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = bitcast %10* %14 to i8*
  %20 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %21 = call i32 %20(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.6, i8* nonnull %19, i32 5)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %23 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %24 = call i32 %23(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.5(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 359
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 360
  %15 = select i1 %14, i32 %13, i32 360
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 360
  %18 = select i1 %17, i32 %16, i32 360
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %219, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 120
  %22 = srem i32 %20, 15
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 13
  %24 = sdiv i32 %20, 15
  %25 = mul nsw i32 %24, 1352
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 104
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 3
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 13
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 15
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !28

for_body2.split.us.us:                            ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -112
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to i32*
  %40 = load i32, i32* %39, align 4, !tbaa !114
  %41 = sext i32 %32 to i64
  %42 = getelementptr inbounds float, float* %4, i64 %41
  %43 = bitcast float* %42 to i32*
  store i32 %40, i32* %43, align 4, !tbaa !117
  %44 = or i64 %27, 1
  %45 = trunc i64 %44 to i32
  %46 = add i32 %21, %45
  %47 = trunc i64 %44 to i32
  %48 = add i32 %47, -112
  %49 = add i32 %26, %48
  %50 = add i32 %49, %25
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to i32*
  %54 = load i32, i32* %53, align 4, !tbaa !114
  %55 = sext i32 %46 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to i32*
  store i32 %54, i32* %57, align 4, !tbaa !117
  %58 = or i64 %27, 2
  %59 = trunc i64 %58 to i32
  %60 = add i32 %21, %59
  %61 = trunc i64 %58 to i32
  %62 = add i32 %61, -112
  %63 = add i32 %26, %62
  %64 = add i32 %63, %25
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to i32*
  %68 = load i32, i32* %67, align 4, !tbaa !114
  %69 = sext i32 %60 to i64
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = bitcast float* %70 to i32*
  store i32 %68, i32* %71, align 4, !tbaa !117
  %72 = or i64 %27, 3
  %73 = trunc i64 %72 to i32
  %74 = add i32 %21, %73
  %75 = trunc i64 %72 to i32
  %76 = add i32 %75, -112
  %77 = add i32 %26, %76
  %78 = add i32 %77, %25
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !114
  %83 = sext i32 %74 to i64
  %84 = getelementptr inbounds float, float* %4, i64 %83
  %85 = bitcast float* %84 to i32*
  store i32 %82, i32* %85, align 4, !tbaa !117
  %86 = or i64 %27, 4
  %87 = trunc i64 %86 to i32
  %88 = add i32 %21, %87
  %89 = trunc i64 %86 to i32
  %90 = add i32 %89, -112
  %91 = add i32 %26, %90
  %92 = add i32 %91, %25
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to i32*
  %96 = load i32, i32* %95, align 4, !tbaa !114
  %97 = sext i32 %88 to i64
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = bitcast float* %98 to i32*
  store i32 %96, i32* %99, align 4, !tbaa !117
  %100 = or i64 %27, 5
  %101 = trunc i64 %100 to i32
  %102 = add i32 %21, %101
  %103 = trunc i64 %100 to i32
  %104 = add i32 %103, -112
  %105 = add i32 %26, %104
  %106 = add i32 %105, %25
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to i32*
  %110 = load i32, i32* %109, align 4, !tbaa !114
  %111 = sext i32 %102 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to i32*
  store i32 %110, i32* %113, align 4, !tbaa !117
  %114 = or i64 %27, 6
  %115 = trunc i64 %114 to i32
  %116 = add i32 %21, %115
  %117 = trunc i64 %114 to i32
  %118 = add i32 %117, -112
  %119 = add i32 %26, %118
  %120 = add i32 %119, %25
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to i32*
  %124 = load i32, i32* %123, align 4, !tbaa !114
  %125 = sext i32 %116 to i64
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = bitcast float* %126 to i32*
  store i32 %124, i32* %127, align 4, !tbaa !117
  %128 = or i64 %27, 7
  %129 = trunc i64 %128 to i32
  %130 = add i32 %21, %129
  %131 = trunc i64 %128 to i32
  %132 = add i32 %131, -112
  %133 = add i32 %26, %132
  %134 = add i32 %133, %25
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %7, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !114
  %139 = sext i32 %130 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to i32*
  store i32 %138, i32* %141, align 4, !tbaa !117
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %142 = sext i32 %32 to i64
  %143 = getelementptr inbounds float, float* %4, i64 %142
  store float 0.000000e+00, float* %143, align 4, !tbaa !117
  %144 = trunc i64 %27 to i32
  %145 = or i32 %144, 1
  %146 = add i32 %145, %21
  %147 = sext i32 %146 to i64
  %148 = getelementptr inbounds float, float* %4, i64 %147
  store float 0.000000e+00, float* %148, align 4, !tbaa !117
  %149 = trunc i64 %27 to i32
  %150 = or i32 %149, 2
  %151 = add i32 %150, %21
  %152 = sext i32 %151 to i64
  %153 = getelementptr inbounds float, float* %4, i64 %152
  store float 0.000000e+00, float* %153, align 4, !tbaa !117
  %154 = trunc i64 %27 to i32
  %155 = or i32 %154, 3
  %156 = add i32 %155, %21
  %157 = sext i32 %156 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  store float 0.000000e+00, float* %158, align 4, !tbaa !117
  %159 = trunc i64 %27 to i32
  %160 = or i32 %159, 4
  %161 = add i32 %160, %21
  %162 = sext i32 %161 to i64
  %163 = getelementptr inbounds float, float* %4, i64 %162
  store float 0.000000e+00, float* %163, align 4, !tbaa !117
  %164 = trunc i64 %27 to i32
  %165 = or i32 %164, 5
  %166 = add i32 %165, %21
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  store float 0.000000e+00, float* %168, align 4, !tbaa !117
  %169 = trunc i64 %27 to i32
  %170 = or i32 %169, 6
  %171 = add i32 %170, %21
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds float, float* %4, i64 %172
  store float 0.000000e+00, float* %173, align 4, !tbaa !117
  %174 = trunc i64 %27 to i32
  %175 = or i32 %174, 7
  %176 = add i32 %175, %21
  %177 = sext i32 %176 to i64
  %178 = getelementptr inbounds float, float* %4, i64 %177
  store float 0.000000e+00, float* %178, align 4, !tbaa !117
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %179 = shl nsw i64 %indvars.iv, 3
  %180 = trunc i64 %179 to i32
  %181 = add i32 %21, %180
  %182 = sext i32 %181 to i64
  %183 = getelementptr inbounds float, float* %4, i64 %182
  store float 0.000000e+00, float* %183, align 4, !tbaa !117
  %184 = trunc i64 %179 to i32
  %185 = or i32 %184, 1
  %186 = add i32 %185, %21
  %187 = sext i32 %186 to i64
  %188 = getelementptr inbounds float, float* %4, i64 %187
  store float 0.000000e+00, float* %188, align 4, !tbaa !117
  %189 = trunc i64 %179 to i32
  %190 = or i32 %189, 2
  %191 = add i32 %190, %21
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %4, i64 %192
  store float 0.000000e+00, float* %193, align 4, !tbaa !117
  %194 = trunc i64 %179 to i32
  %195 = or i32 %194, 3
  %196 = add i32 %195, %21
  %197 = sext i32 %196 to i64
  %198 = getelementptr inbounds float, float* %4, i64 %197
  store float 0.000000e+00, float* %198, align 4, !tbaa !117
  %199 = trunc i64 %179 to i32
  %200 = or i32 %199, 4
  %201 = add i32 %200, %21
  %202 = sext i32 %201 to i64
  %203 = getelementptr inbounds float, float* %4, i64 %202
  store float 0.000000e+00, float* %203, align 4, !tbaa !117
  %204 = trunc i64 %179 to i32
  %205 = or i32 %204, 5
  %206 = add i32 %205, %21
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  store float 0.000000e+00, float* %208, align 4, !tbaa !117
  %209 = trunc i64 %179 to i32
  %210 = or i32 %209, 6
  %211 = add i32 %210, %21
  %212 = sext i32 %211 to i64
  %213 = getelementptr inbounds float, float* %4, i64 %212
  store float 0.000000e+00, float* %213, align 4, !tbaa !117
  %214 = trunc i64 %179 to i32
  %215 = or i32 %214, 7
  %216 = add i32 %215, %21
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds float, float* %4, i64 %217
  store float 0.000000e+00, float* %218, align 4, !tbaa !117
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 15
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %219 = add nsw i32 %20, 1
  %220 = icmp slt i32 %219, %15
  br i1 %220, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.6(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds i8, i8* %2, i64 16
  %9 = bitcast i8* %8 to float**
  %10 = load float*, float** %9, align 8
  %11 = getelementptr inbounds i8, i8* %2, i64 24
  %12 = bitcast i8* %11 to float**
  %13 = load float*, float** %12, align 8
  %14 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %15 = load i32, i32* %14, align 4
  %16 = add nsw i32 %15, 623
  %17 = sdiv i32 %16, %15
  %18 = add nsw i32 %0, 1
  %19 = mul nsw i32 %17, %18
  %20 = icmp slt i32 %19, 624
  %21 = select i1 %20, i32 %19, i32 624
  %22 = mul nsw i32 %17, %0
  %23 = icmp slt i32 %22, 624
  %24 = select i1 %23, i32 %22, i32 624
  %25 = icmp slt i32 %24, %21
  br i1 %25, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %26 = add i32 %24, 1
  %27 = sext i32 %26 to i64
  %28 = add nsw i64 %27, -1
  %29 = sext i32 %21 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv138 = phi i64 [ %28, %for_body.lr.ph ], [ %indvars.iv.next139, %for_end3 ]
  %30 = trunc i64 %indvars.iv138 to i32
  %31 = srem i32 %30, 13
  %32 = sdiv i32 %30, 13
  %33 = mul nsw i32 %32, 13824
  %34 = sext i32 %33 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv132 = phi i64 [ 0, %for_body ], [ %indvars.iv.next133, %for_end6 ]
  %.lcssa38.lcssa.lcssa112 = phi <8 x float> [ zeroinitializer, %for_body ], [ %419, %for_end6 ]
  %.lcssa36.lcssa.lcssa110 = phi <8 x float> [ zeroinitializer, %for_body ], [ %413, %for_end6 ]
  %.lcssa34.lcssa.lcssa108 = phi <8 x float> [ zeroinitializer, %for_body ], [ %407, %for_end6 ]
  %.lcssa32.lcssa.lcssa106 = phi <8 x float> [ zeroinitializer, %for_body ], [ %401, %for_end6 ]
  %.lcssa30.lcssa.lcssa104 = phi <8 x float> [ zeroinitializer, %for_body ], [ %395, %for_end6 ]
  %.lcssa28.lcssa.lcssa102 = phi <8 x float> [ zeroinitializer, %for_body ], [ %389, %for_end6 ]
  %.lcssa26.lcssa.lcssa100 = phi <8 x float> [ zeroinitializer, %for_body ], [ %383, %for_end6 ]
  %.lcssa24.lcssa.lcssa98 = phi <8 x float> [ zeroinitializer, %for_body ], [ %377, %for_end6 ]
  %.lcssa22.lcssa.lcssa96 = phi <8 x float> [ zeroinitializer, %for_body ], [ %371, %for_end6 ]
  %.lcssa20.lcssa.lcssa95 = phi <8 x float> [ zeroinitializer, %for_body ], [ %365, %for_end6 ]
  %.lcssa18.lcssa.lcssa93 = phi <8 x float> [ zeroinitializer, %for_body ], [ %359, %for_end6 ]
  %.lcssa16.lcssa.lcssa91 = phi <8 x float> [ zeroinitializer, %for_body ], [ %353, %for_end6 ]
  %.lcssa.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body ], [ %347, %for_end6 ]
  %35 = mul nuw nsw i64 %indvars.iv132, 576
  %36 = add nsw i64 %35, %34
  %37 = trunc i64 %indvars.iv132 to i32
  %38 = mul i32 %37, 1800
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %39 = mul nsw i64 %indvars.iv138, 104
  %40 = shl nsw i32 %32, 3
  %41 = sext i32 %40 to i64
  %42 = getelementptr inbounds float, float* %13, i64 %41
  %43 = bitcast float* %42 to <8 x float>*
  %44 = load <8 x float>, <8 x float>* %43, align 32, !tbaa !120
  %45 = fadd <8 x float> %44, %347
  %46 = fcmp ogt <8 x float> %45, zeroinitializer
  %47 = select <8 x i1> %46, <8 x float> %45, <8 x float> zeroinitializer
  %48 = getelementptr inbounds float, float* %10, i64 %39
  %49 = bitcast float* %48 to <8 x float>*
  store <8 x float> %47, <8 x float>* %49, align 32, !tbaa !123
  %50 = add nsw i64 %39, 8
  %51 = fadd <8 x float> %44, %353
  %52 = fcmp ogt <8 x float> %51, zeroinitializer
  %53 = select <8 x i1> %52, <8 x float> %51, <8 x float> zeroinitializer
  %54 = getelementptr inbounds float, float* %10, i64 %50
  %55 = bitcast float* %54 to <8 x float>*
  store <8 x float> %53, <8 x float>* %55, align 32, !tbaa !123
  %56 = add nsw i64 %39, 16
  %57 = fadd <8 x float> %44, %359
  %58 = fcmp ogt <8 x float> %57, zeroinitializer
  %59 = select <8 x i1> %58, <8 x float> %57, <8 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %10, i64 %56
  %61 = bitcast float* %60 to <8 x float>*
  store <8 x float> %59, <8 x float>* %61, align 32, !tbaa !123
  %62 = add nsw i64 %39, 24
  %63 = fadd <8 x float> %44, %365
  %64 = fcmp ogt <8 x float> %63, zeroinitializer
  %65 = select <8 x i1> %64, <8 x float> %63, <8 x float> zeroinitializer
  %66 = getelementptr inbounds float, float* %10, i64 %62
  %67 = bitcast float* %66 to <8 x float>*
  store <8 x float> %65, <8 x float>* %67, align 32, !tbaa !123
  %68 = add nsw i64 %39, 32
  %69 = fadd <8 x float> %44, %371
  %70 = fcmp ogt <8 x float> %69, zeroinitializer
  %71 = select <8 x i1> %70, <8 x float> %69, <8 x float> zeroinitializer
  %72 = getelementptr inbounds float, float* %10, i64 %68
  %73 = bitcast float* %72 to <8 x float>*
  store <8 x float> %71, <8 x float>* %73, align 32, !tbaa !123
  %74 = add nsw i64 %39, 40
  %75 = fadd <8 x float> %44, %377
  %76 = fcmp ogt <8 x float> %75, zeroinitializer
  %77 = select <8 x i1> %76, <8 x float> %75, <8 x float> zeroinitializer
  %78 = getelementptr inbounds float, float* %10, i64 %74
  %79 = bitcast float* %78 to <8 x float>*
  store <8 x float> %77, <8 x float>* %79, align 32, !tbaa !123
  %80 = add nsw i64 %39, 48
  %81 = fadd <8 x float> %44, %383
  %82 = fcmp ogt <8 x float> %81, zeroinitializer
  %83 = select <8 x i1> %82, <8 x float> %81, <8 x float> zeroinitializer
  %84 = getelementptr inbounds float, float* %10, i64 %80
  %85 = bitcast float* %84 to <8 x float>*
  store <8 x float> %83, <8 x float>* %85, align 32, !tbaa !123
  %86 = add nsw i64 %39, 56
  %87 = fadd <8 x float> %44, %389
  %88 = fcmp ogt <8 x float> %87, zeroinitializer
  %89 = select <8 x i1> %88, <8 x float> %87, <8 x float> zeroinitializer
  %90 = getelementptr inbounds float, float* %10, i64 %86
  %91 = bitcast float* %90 to <8 x float>*
  store <8 x float> %89, <8 x float>* %91, align 32, !tbaa !123
  %92 = add nsw i64 %39, 64
  %93 = fadd <8 x float> %44, %395
  %94 = fcmp ogt <8 x float> %93, zeroinitializer
  %95 = select <8 x i1> %94, <8 x float> %93, <8 x float> zeroinitializer
  %96 = getelementptr inbounds float, float* %10, i64 %92
  %97 = bitcast float* %96 to <8 x float>*
  store <8 x float> %95, <8 x float>* %97, align 32, !tbaa !123
  %98 = add nsw i64 %39, 72
  %99 = fadd <8 x float> %44, %401
  %100 = fcmp ogt <8 x float> %99, zeroinitializer
  %101 = select <8 x i1> %100, <8 x float> %99, <8 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %10, i64 %98
  %103 = bitcast float* %102 to <8 x float>*
  store <8 x float> %101, <8 x float>* %103, align 32, !tbaa !123
  %104 = add nsw i64 %39, 80
  %105 = fadd <8 x float> %44, %407
  %106 = fcmp ogt <8 x float> %105, zeroinitializer
  %107 = select <8 x i1> %106, <8 x float> %105, <8 x float> zeroinitializer
  %108 = getelementptr inbounds float, float* %10, i64 %104
  %109 = bitcast float* %108 to <8 x float>*
  store <8 x float> %107, <8 x float>* %109, align 32, !tbaa !123
  %110 = add nsw i64 %39, 88
  %111 = fadd <8 x float> %44, %413
  %112 = fcmp ogt <8 x float> %111, zeroinitializer
  %113 = select <8 x i1> %112, <8 x float> %111, <8 x float> zeroinitializer
  %114 = getelementptr inbounds float, float* %10, i64 %110
  %115 = bitcast float* %114 to <8 x float>*
  store <8 x float> %113, <8 x float>* %115, align 32, !tbaa !123
  %116 = add nsw i64 %39, 96
  %117 = fadd <8 x float> %44, %419
  %118 = fcmp ogt <8 x float> %117, zeroinitializer
  %119 = select <8 x i1> %118, <8 x float> %117, <8 x float> zeroinitializer
  %120 = getelementptr inbounds float, float* %10, i64 %116
  %121 = bitcast float* %120 to <8 x float>*
  store <8 x float> %119, <8 x float>* %121, align 32, !tbaa !123
  %indvars.iv.next139 = add nsw i64 %indvars.iv138, 1
  %122 = icmp slt i64 %indvars.iv.next139, %29
  br i1 %122, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_end12.2, %for_body2
  %indvars.iv128 = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next129, %for_end12.2 ]
  %.lcssa38.lcssa87 = phi <8 x float> [ %.lcssa38.lcssa.lcssa112, %for_body2 ], [ %419, %for_end12.2 ]
  %.lcssa36.lcssa85 = phi <8 x float> [ %.lcssa36.lcssa.lcssa110, %for_body2 ], [ %413, %for_end12.2 ]
  %.lcssa34.lcssa83 = phi <8 x float> [ %.lcssa34.lcssa.lcssa108, %for_body2 ], [ %407, %for_end12.2 ]
  %.lcssa32.lcssa81 = phi <8 x float> [ %.lcssa32.lcssa.lcssa106, %for_body2 ], [ %401, %for_end12.2 ]
  %.lcssa30.lcssa79 = phi <8 x float> [ %.lcssa30.lcssa.lcssa104, %for_body2 ], [ %395, %for_end12.2 ]
  %.lcssa28.lcssa77 = phi <8 x float> [ %.lcssa28.lcssa.lcssa102, %for_body2 ], [ %389, %for_end12.2 ]
  %.lcssa26.lcssa75 = phi <8 x float> [ %.lcssa26.lcssa.lcssa100, %for_body2 ], [ %383, %for_end12.2 ]
  %.lcssa24.lcssa73 = phi <8 x float> [ %.lcssa24.lcssa.lcssa98, %for_body2 ], [ %377, %for_end12.2 ]
  %.lcssa22.lcssa71 = phi <8 x float> [ %.lcssa22.lcssa.lcssa96, %for_body2 ], [ %371, %for_end12.2 ]
  %.lcssa20.lcssa69 = phi <8 x float> [ %.lcssa20.lcssa.lcssa95, %for_body2 ], [ %365, %for_end12.2 ]
  %.lcssa18.lcssa68 = phi <8 x float> [ %.lcssa18.lcssa.lcssa93, %for_body2 ], [ %359, %for_end12.2 ]
  %.lcssa16.lcssa66 = phi <8 x float> [ %.lcssa16.lcssa.lcssa91, %for_body2 ], [ %353, %for_end12.2 ]
  %.lcssa.lcssa64 = phi <8 x float> [ %.lcssa.lcssa.lcssa89, %for_body2 ], [ %347, %for_end12.2 ]
  %123 = phi i32 [ 0, %for_body2 ], [ %420, %for_end12.2 ]
  %reass.add = add nsw i32 %123, %31
  %reass.mul = mul i32 %reass.add, 120
  %124 = add nsw i32 %reass.mul, %38
  %125 = mul nuw nsw i64 %indvars.iv128, 192
  %126 = add nsw i64 %36, %125
  %127 = sext i32 %124 to i64
  br label %for_body11

for_end6:                                         ; preds = %for_end12.2
  %indvars.iv.next133 = add nuw nsw i64 %indvars.iv132, 1
  %exitcond134 = icmp eq i64 %indvars.iv.next133, 24
  br i1 %exitcond134, label %for_end3, label %for_body2, !prof !28

for_body11:                                       ; preds = %for_body11, %for_body5
  %indvars.iv = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next, %for_body11 ]
  %128 = phi <8 x float> [ %.lcssa38.lcssa87, %for_body5 ], [ %223, %for_body11 ]
  %129 = phi <8 x float> [ %.lcssa36.lcssa85, %for_body5 ], [ %217, %for_body11 ]
  %130 = phi <8 x float> [ %.lcssa34.lcssa83, %for_body5 ], [ %211, %for_body11 ]
  %131 = phi <8 x float> [ %.lcssa32.lcssa81, %for_body5 ], [ %205, %for_body11 ]
  %132 = phi <8 x float> [ %.lcssa30.lcssa79, %for_body5 ], [ %199, %for_body11 ]
  %133 = phi <8 x float> [ %.lcssa28.lcssa77, %for_body5 ], [ %193, %for_body11 ]
  %134 = phi <8 x float> [ %.lcssa26.lcssa75, %for_body5 ], [ %187, %for_body11 ]
  %135 = phi <8 x float> [ %.lcssa24.lcssa73, %for_body5 ], [ %181, %for_body11 ]
  %136 = phi <8 x float> [ %.lcssa22.lcssa71, %for_body5 ], [ %175, %for_body11 ]
  %137 = phi <8 x float> [ %.lcssa20.lcssa69, %for_body5 ], [ %169, %for_body11 ]
  %138 = phi <8 x float> [ %.lcssa18.lcssa68, %for_body5 ], [ %163, %for_body11 ]
  %139 = phi <8 x float> [ %.lcssa16.lcssa66, %for_body5 ], [ %157, %for_body11 ]
  %140 = phi <8 x float> [ %.lcssa.lcssa64, %for_body5 ], [ %151, %for_body11 ]
  %141 = add nsw i64 %indvars.iv, %127
  %142 = getelementptr inbounds float, float* %4, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !117
  %144 = insertelement <8 x float> undef, float %143, i32 0
  %145 = shufflevector <8 x float> %144, <8 x float> undef, <8 x i32> zeroinitializer
  %146 = shl i64 %indvars.iv, 3
  %147 = add nsw i64 %126, %146
  %148 = getelementptr inbounds float, float* %7, i64 %147
  %149 = bitcast float* %148 to <8 x float>*
  %150 = load <8 x float>, <8 x float>* %149, align 32, !tbaa !126
  %151 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %145, <8 x float> %150, <8 x float> %140)
  %152 = add nsw i64 %141, 8
  %153 = getelementptr inbounds float, float* %4, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !117
  %155 = insertelement <8 x float> undef, float %154, i32 0
  %156 = shufflevector <8 x float> %155, <8 x float> undef, <8 x i32> zeroinitializer
  %157 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %156, <8 x float> %150, <8 x float> %139)
  %158 = add nsw i64 %141, 16
  %159 = getelementptr inbounds float, float* %4, i64 %158
  %160 = load float, float* %159, align 4, !tbaa !117
  %161 = insertelement <8 x float> undef, float %160, i32 0
  %162 = shufflevector <8 x float> %161, <8 x float> undef, <8 x i32> zeroinitializer
  %163 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %162, <8 x float> %150, <8 x float> %138)
  %164 = add nsw i64 %141, 24
  %165 = getelementptr inbounds float, float* %4, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !117
  %167 = insertelement <8 x float> undef, float %166, i32 0
  %168 = shufflevector <8 x float> %167, <8 x float> undef, <8 x i32> zeroinitializer
  %169 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %168, <8 x float> %150, <8 x float> %137)
  %170 = add nsw i64 %141, 32
  %171 = getelementptr inbounds float, float* %4, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !117
  %173 = insertelement <8 x float> undef, float %172, i32 0
  %174 = shufflevector <8 x float> %173, <8 x float> undef, <8 x i32> zeroinitializer
  %175 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %174, <8 x float> %150, <8 x float> %136)
  %176 = add nsw i64 %141, 40
  %177 = getelementptr inbounds float, float* %4, i64 %176
  %178 = load float, float* %177, align 4, !tbaa !117
  %179 = insertelement <8 x float> undef, float %178, i32 0
  %180 = shufflevector <8 x float> %179, <8 x float> undef, <8 x i32> zeroinitializer
  %181 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %180, <8 x float> %150, <8 x float> %135)
  %182 = add nsw i64 %141, 48
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !117
  %185 = insertelement <8 x float> undef, float %184, i32 0
  %186 = shufflevector <8 x float> %185, <8 x float> undef, <8 x i32> zeroinitializer
  %187 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %186, <8 x float> %150, <8 x float> %134)
  %188 = add nsw i64 %141, 56
  %189 = getelementptr inbounds float, float* %4, i64 %188
  %190 = load float, float* %189, align 4, !tbaa !117
  %191 = insertelement <8 x float> undef, float %190, i32 0
  %192 = shufflevector <8 x float> %191, <8 x float> undef, <8 x i32> zeroinitializer
  %193 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %192, <8 x float> %150, <8 x float> %133)
  %194 = add nsw i64 %141, 64
  %195 = getelementptr inbounds float, float* %4, i64 %194
  %196 = load float, float* %195, align 4, !tbaa !117
  %197 = insertelement <8 x float> undef, float %196, i32 0
  %198 = shufflevector <8 x float> %197, <8 x float> undef, <8 x i32> zeroinitializer
  %199 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %198, <8 x float> %150, <8 x float> %132)
  %200 = add nsw i64 %141, 72
  %201 = getelementptr inbounds float, float* %4, i64 %200
  %202 = load float, float* %201, align 4, !tbaa !117
  %203 = insertelement <8 x float> undef, float %202, i32 0
  %204 = shufflevector <8 x float> %203, <8 x float> undef, <8 x i32> zeroinitializer
  %205 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %204, <8 x float> %150, <8 x float> %131)
  %206 = add nsw i64 %141, 80
  %207 = getelementptr inbounds float, float* %4, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !117
  %209 = insertelement <8 x float> undef, float %208, i32 0
  %210 = shufflevector <8 x float> %209, <8 x float> undef, <8 x i32> zeroinitializer
  %211 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %210, <8 x float> %150, <8 x float> %130)
  %212 = add nsw i64 %141, 88
  %213 = getelementptr inbounds float, float* %4, i64 %212
  %214 = load float, float* %213, align 4, !tbaa !117
  %215 = insertelement <8 x float> undef, float %214, i32 0
  %216 = shufflevector <8 x float> %215, <8 x float> undef, <8 x i32> zeroinitializer
  %217 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %216, <8 x float> %150, <8 x float> %129)
  %218 = add nsw i64 %141, 96
  %219 = getelementptr inbounds float, float* %4, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !117
  %221 = insertelement <8 x float> undef, float %220, i32 0
  %222 = shufflevector <8 x float> %221, <8 x float> undef, <8 x i32> zeroinitializer
  %223 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %222, <8 x float> %150, <8 x float> %128)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end12, label %for_body11, !prof !28

for_end12:                                        ; preds = %for_body11
  %224 = add nsw i64 %127, 8
  %225 = add nsw i64 %126, 64
  br label %for_body11.1

for_body11.1:                                     ; preds = %for_body11.1, %for_end12
  %indvars.iv.1 = phi i64 [ 0, %for_end12 ], [ %indvars.iv.next.1, %for_body11.1 ]
  %226 = phi <8 x float> [ %223, %for_end12 ], [ %321, %for_body11.1 ]
  %227 = phi <8 x float> [ %217, %for_end12 ], [ %315, %for_body11.1 ]
  %228 = phi <8 x float> [ %211, %for_end12 ], [ %309, %for_body11.1 ]
  %229 = phi <8 x float> [ %205, %for_end12 ], [ %303, %for_body11.1 ]
  %230 = phi <8 x float> [ %199, %for_end12 ], [ %297, %for_body11.1 ]
  %231 = phi <8 x float> [ %193, %for_end12 ], [ %291, %for_body11.1 ]
  %232 = phi <8 x float> [ %187, %for_end12 ], [ %285, %for_body11.1 ]
  %233 = phi <8 x float> [ %181, %for_end12 ], [ %279, %for_body11.1 ]
  %234 = phi <8 x float> [ %175, %for_end12 ], [ %273, %for_body11.1 ]
  %235 = phi <8 x float> [ %169, %for_end12 ], [ %267, %for_body11.1 ]
  %236 = phi <8 x float> [ %163, %for_end12 ], [ %261, %for_body11.1 ]
  %237 = phi <8 x float> [ %157, %for_end12 ], [ %255, %for_body11.1 ]
  %238 = phi <8 x float> [ %151, %for_end12 ], [ %249, %for_body11.1 ]
  %239 = add nsw i64 %224, %indvars.iv.1
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !117
  %242 = insertelement <8 x float> undef, float %241, i32 0
  %243 = shufflevector <8 x float> %242, <8 x float> undef, <8 x i32> zeroinitializer
  %244 = shl i64 %indvars.iv.1, 3
  %245 = add nsw i64 %225, %244
  %246 = getelementptr inbounds float, float* %7, i64 %245
  %247 = bitcast float* %246 to <8 x float>*
  %248 = load <8 x float>, <8 x float>* %247, align 32, !tbaa !126
  %249 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %243, <8 x float> %248, <8 x float> %238)
  %250 = add nsw i64 %239, 8
  %251 = getelementptr inbounds float, float* %4, i64 %250
  %252 = load float, float* %251, align 4, !tbaa !117
  %253 = insertelement <8 x float> undef, float %252, i32 0
  %254 = shufflevector <8 x float> %253, <8 x float> undef, <8 x i32> zeroinitializer
  %255 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %254, <8 x float> %248, <8 x float> %237)
  %256 = add nsw i64 %239, 16
  %257 = getelementptr inbounds float, float* %4, i64 %256
  %258 = load float, float* %257, align 4, !tbaa !117
  %259 = insertelement <8 x float> undef, float %258, i32 0
  %260 = shufflevector <8 x float> %259, <8 x float> undef, <8 x i32> zeroinitializer
  %261 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %260, <8 x float> %248, <8 x float> %236)
  %262 = add nsw i64 %239, 24
  %263 = getelementptr inbounds float, float* %4, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !117
  %265 = insertelement <8 x float> undef, float %264, i32 0
  %266 = shufflevector <8 x float> %265, <8 x float> undef, <8 x i32> zeroinitializer
  %267 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %266, <8 x float> %248, <8 x float> %235)
  %268 = add nsw i64 %239, 32
  %269 = getelementptr inbounds float, float* %4, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !117
  %271 = insertelement <8 x float> undef, float %270, i32 0
  %272 = shufflevector <8 x float> %271, <8 x float> undef, <8 x i32> zeroinitializer
  %273 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %272, <8 x float> %248, <8 x float> %234)
  %274 = add nsw i64 %239, 40
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !117
  %277 = insertelement <8 x float> undef, float %276, i32 0
  %278 = shufflevector <8 x float> %277, <8 x float> undef, <8 x i32> zeroinitializer
  %279 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %278, <8 x float> %248, <8 x float> %233)
  %280 = add nsw i64 %239, 48
  %281 = getelementptr inbounds float, float* %4, i64 %280
  %282 = load float, float* %281, align 4, !tbaa !117
  %283 = insertelement <8 x float> undef, float %282, i32 0
  %284 = shufflevector <8 x float> %283, <8 x float> undef, <8 x i32> zeroinitializer
  %285 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %284, <8 x float> %248, <8 x float> %232)
  %286 = add nsw i64 %239, 56
  %287 = getelementptr inbounds float, float* %4, i64 %286
  %288 = load float, float* %287, align 4, !tbaa !117
  %289 = insertelement <8 x float> undef, float %288, i32 0
  %290 = shufflevector <8 x float> %289, <8 x float> undef, <8 x i32> zeroinitializer
  %291 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %290, <8 x float> %248, <8 x float> %231)
  %292 = add nsw i64 %239, 64
  %293 = getelementptr inbounds float, float* %4, i64 %292
  %294 = load float, float* %293, align 4, !tbaa !117
  %295 = insertelement <8 x float> undef, float %294, i32 0
  %296 = shufflevector <8 x float> %295, <8 x float> undef, <8 x i32> zeroinitializer
  %297 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %296, <8 x float> %248, <8 x float> %230)
  %298 = add nsw i64 %239, 72
  %299 = getelementptr inbounds float, float* %4, i64 %298
  %300 = load float, float* %299, align 4, !tbaa !117
  %301 = insertelement <8 x float> undef, float %300, i32 0
  %302 = shufflevector <8 x float> %301, <8 x float> undef, <8 x i32> zeroinitializer
  %303 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %302, <8 x float> %248, <8 x float> %229)
  %304 = add nsw i64 %239, 80
  %305 = getelementptr inbounds float, float* %4, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !117
  %307 = insertelement <8 x float> undef, float %306, i32 0
  %308 = shufflevector <8 x float> %307, <8 x float> undef, <8 x i32> zeroinitializer
  %309 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %308, <8 x float> %248, <8 x float> %228)
  %310 = add nsw i64 %239, 88
  %311 = getelementptr inbounds float, float* %4, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !117
  %313 = insertelement <8 x float> undef, float %312, i32 0
  %314 = shufflevector <8 x float> %313, <8 x float> undef, <8 x i32> zeroinitializer
  %315 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %314, <8 x float> %248, <8 x float> %227)
  %316 = add nsw i64 %239, 96
  %317 = getelementptr inbounds float, float* %4, i64 %316
  %318 = load float, float* %317, align 4, !tbaa !117
  %319 = insertelement <8 x float> undef, float %318, i32 0
  %320 = shufflevector <8 x float> %319, <8 x float> undef, <8 x i32> zeroinitializer
  %321 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %320, <8 x float> %248, <8 x float> %226)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 8
  br i1 %exitcond.1, label %for_end12.1, label %for_body11.1, !prof !28

for_end12.1:                                      ; preds = %for_body11.1
  %322 = add nsw i64 %127, 16
  %323 = add nsw i64 %126, 128
  br label %for_body11.2

for_body11.2:                                     ; preds = %for_body11.2, %for_end12.1
  %indvars.iv.2 = phi i64 [ 0, %for_end12.1 ], [ %indvars.iv.next.2, %for_body11.2 ]
  %324 = phi <8 x float> [ %321, %for_end12.1 ], [ %419, %for_body11.2 ]
  %325 = phi <8 x float> [ %315, %for_end12.1 ], [ %413, %for_body11.2 ]
  %326 = phi <8 x float> [ %309, %for_end12.1 ], [ %407, %for_body11.2 ]
  %327 = phi <8 x float> [ %303, %for_end12.1 ], [ %401, %for_body11.2 ]
  %328 = phi <8 x float> [ %297, %for_end12.1 ], [ %395, %for_body11.2 ]
  %329 = phi <8 x float> [ %291, %for_end12.1 ], [ %389, %for_body11.2 ]
  %330 = phi <8 x float> [ %285, %for_end12.1 ], [ %383, %for_body11.2 ]
  %331 = phi <8 x float> [ %279, %for_end12.1 ], [ %377, %for_body11.2 ]
  %332 = phi <8 x float> [ %273, %for_end12.1 ], [ %371, %for_body11.2 ]
  %333 = phi <8 x float> [ %267, %for_end12.1 ], [ %365, %for_body11.2 ]
  %334 = phi <8 x float> [ %261, %for_end12.1 ], [ %359, %for_body11.2 ]
  %335 = phi <8 x float> [ %255, %for_end12.1 ], [ %353, %for_body11.2 ]
  %336 = phi <8 x float> [ %249, %for_end12.1 ], [ %347, %for_body11.2 ]
  %337 = add nsw i64 %322, %indvars.iv.2
  %338 = getelementptr inbounds float, float* %4, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !117
  %340 = insertelement <8 x float> undef, float %339, i32 0
  %341 = shufflevector <8 x float> %340, <8 x float> undef, <8 x i32> zeroinitializer
  %342 = shl i64 %indvars.iv.2, 3
  %343 = add nsw i64 %323, %342
  %344 = getelementptr inbounds float, float* %7, i64 %343
  %345 = bitcast float* %344 to <8 x float>*
  %346 = load <8 x float>, <8 x float>* %345, align 32, !tbaa !126
  %347 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %341, <8 x float> %346, <8 x float> %336)
  %348 = add nsw i64 %337, 8
  %349 = getelementptr inbounds float, float* %4, i64 %348
  %350 = load float, float* %349, align 4, !tbaa !117
  %351 = insertelement <8 x float> undef, float %350, i32 0
  %352 = shufflevector <8 x float> %351, <8 x float> undef, <8 x i32> zeroinitializer
  %353 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %352, <8 x float> %346, <8 x float> %335)
  %354 = add nsw i64 %337, 16
  %355 = getelementptr inbounds float, float* %4, i64 %354
  %356 = load float, float* %355, align 4, !tbaa !117
  %357 = insertelement <8 x float> undef, float %356, i32 0
  %358 = shufflevector <8 x float> %357, <8 x float> undef, <8 x i32> zeroinitializer
  %359 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %358, <8 x float> %346, <8 x float> %334)
  %360 = add nsw i64 %337, 24
  %361 = getelementptr inbounds float, float* %4, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !117
  %363 = insertelement <8 x float> undef, float %362, i32 0
  %364 = shufflevector <8 x float> %363, <8 x float> undef, <8 x i32> zeroinitializer
  %365 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %364, <8 x float> %346, <8 x float> %333)
  %366 = add nsw i64 %337, 32
  %367 = getelementptr inbounds float, float* %4, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !117
  %369 = insertelement <8 x float> undef, float %368, i32 0
  %370 = shufflevector <8 x float> %369, <8 x float> undef, <8 x i32> zeroinitializer
  %371 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %370, <8 x float> %346, <8 x float> %332)
  %372 = add nsw i64 %337, 40
  %373 = getelementptr inbounds float, float* %4, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !117
  %375 = insertelement <8 x float> undef, float %374, i32 0
  %376 = shufflevector <8 x float> %375, <8 x float> undef, <8 x i32> zeroinitializer
  %377 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %376, <8 x float> %346, <8 x float> %331)
  %378 = add nsw i64 %337, 48
  %379 = getelementptr inbounds float, float* %4, i64 %378
  %380 = load float, float* %379, align 4, !tbaa !117
  %381 = insertelement <8 x float> undef, float %380, i32 0
  %382 = shufflevector <8 x float> %381, <8 x float> undef, <8 x i32> zeroinitializer
  %383 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %382, <8 x float> %346, <8 x float> %330)
  %384 = add nsw i64 %337, 56
  %385 = getelementptr inbounds float, float* %4, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !117
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shufflevector <8 x float> %387, <8 x float> undef, <8 x i32> zeroinitializer
  %389 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %388, <8 x float> %346, <8 x float> %329)
  %390 = add nsw i64 %337, 64
  %391 = getelementptr inbounds float, float* %4, i64 %390
  %392 = load float, float* %391, align 4, !tbaa !117
  %393 = insertelement <8 x float> undef, float %392, i32 0
  %394 = shufflevector <8 x float> %393, <8 x float> undef, <8 x i32> zeroinitializer
  %395 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %394, <8 x float> %346, <8 x float> %328)
  %396 = add nsw i64 %337, 72
  %397 = getelementptr inbounds float, float* %4, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !117
  %399 = insertelement <8 x float> undef, float %398, i32 0
  %400 = shufflevector <8 x float> %399, <8 x float> undef, <8 x i32> zeroinitializer
  %401 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %400, <8 x float> %346, <8 x float> %327)
  %402 = add nsw i64 %337, 80
  %403 = getelementptr inbounds float, float* %4, i64 %402
  %404 = load float, float* %403, align 4, !tbaa !117
  %405 = insertelement <8 x float> undef, float %404, i32 0
  %406 = shufflevector <8 x float> %405, <8 x float> undef, <8 x i32> zeroinitializer
  %407 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %406, <8 x float> %346, <8 x float> %326)
  %408 = add nsw i64 %337, 88
  %409 = getelementptr inbounds float, float* %4, i64 %408
  %410 = load float, float* %409, align 4, !tbaa !117
  %411 = insertelement <8 x float> undef, float %410, i32 0
  %412 = shufflevector <8 x float> %411, <8 x float> undef, <8 x i32> zeroinitializer
  %413 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %412, <8 x float> %346, <8 x float> %325)
  %414 = add nsw i64 %337, 96
  %415 = getelementptr inbounds float, float* %4, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !117
  %417 = insertelement <8 x float> undef, float %416, i32 0
  %418 = shufflevector <8 x float> %417, <8 x float> undef, <8 x i32> zeroinitializer
  %419 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %418, <8 x float> %346, <8 x float> %324)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 8
  br i1 %exitcond.2, label %for_end12.2, label %for_body11.2, !prof !28

for_end12.2:                                      ; preds = %for_body11.2
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %420 = add nuw nsw i32 %123, 1
  %exitcond131 = icmp eq i64 %indvars.iv.next129, 3
  br i1 %exitcond131, label %for_end6, label %for_body5, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare <8 x float> @llvm.fmuladd.v8f32(<8 x float>, <8 x float>, <8 x float>) #3

; Function Attrs: nounwind
define dllexport i32 @fused_layout_transform_nn_batch_flatten_nn_batch_flatten(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 !dbg !129 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !131, metadata !DIExpression()), !dbg !134
  call void @llvm.dbg.value(metadata i8* %1, metadata !132, metadata !DIExpression()), !dbg !134
  call void @llvm.dbg.value(metadata i32 %2, metadata !133, metadata !DIExpression()), !dbg !134
  %3 = bitcast i8* %0 to %1**, !dbg !134
  %4 = load %1*, %1** %3, align 8, !dbg !134
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !134
  %6 = bitcast i8* %5 to %1**, !dbg !134
  %7 = load %1*, %1** %6, align 8, !dbg !134
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !134
  %9 = load i8*, i8** %8, align 8, !dbg !134
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !134
  %11 = load i8*, i8** %10, align 8, !dbg !134
  tail call fastcc void @fused_layout_transform_nn_batch_flatten_nn_batch_flatten_compute_(i8* %11, i8* %9), !dbg !134
  ret i32 0, !dbg !134
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_layout_transform_nn_batch_flatten_nn_batch_flatten_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #4 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_body

for_body:                                         ; preds = %for_body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for_body ]
  %4 = trunc i64 %indvars.iv to i32
  %5 = urem i32 %4, 288
  %6 = udiv i32 %5, 36
  %7 = urem i32 %4, 36
  %8 = shl nuw nsw i32 %7, 3
  %9 = sub nsw i32 %4, %5
  %10 = add nsw i32 %9, %8
  %11 = add nsw i32 %10, %6
  %12 = sext i32 %11 to i64
  %13 = getelementptr inbounds float, float* %2, i64 %12
  %14 = bitcast float* %13 to i32*
  %15 = load i32, i32* %14, align 4, !tbaa !135
  %16 = getelementptr inbounds float, float* %3, i64 %indvars.iv
  %17 = bitcast float* %16 to i32*
  store i32 %15, i32* %17, align 4, !tbaa !138
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 9216
  br i1 %exitcond, label %for_end, label %for_body, !prof !28

for_end:                                          ; preds = %for_body
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_batch_flatten(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 !dbg !141 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !143, metadata !DIExpression()), !dbg !146
  call void @llvm.dbg.value(metadata i8* %1, metadata !144, metadata !DIExpression()), !dbg !146
  call void @llvm.dbg.value(metadata i32 %2, metadata !145, metadata !DIExpression()), !dbg !146
  %3 = bitcast i8* %0 to %1**, !dbg !146
  %4 = load %1*, %1** %3, align 8, !dbg !146
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !146
  %6 = bitcast i8* %5 to %1**, !dbg !146
  %7 = load %1*, %1** %6, align 8, !dbg !146
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !146
  %9 = load i8*, i8** %8, align 8, !dbg !146
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !146
  %11 = load i8*, i8** %10, align 8, !dbg !146
  tail call fastcc void @fused_nn_batch_flatten_compute_(i8* %11, i8* %9), !dbg !146
  ret i32 0, !dbg !146
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_batch_flatten_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #4 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16384, i32 4, i1 false)
  ret void
}

define dllexport i32 @fused_nn_max_pool2d(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !147 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !149, metadata !DIExpression()), !dbg !152
  call void @llvm.dbg.value(metadata i8* %1, metadata !150, metadata !DIExpression()), !dbg !152
  call void @llvm.dbg.value(metadata i32 %2, metadata !151, metadata !DIExpression()), !dbg !152
  %3 = bitcast i8* %0 to %1**, !dbg !152
  %4 = load %1*, %1** %3, align 8, !dbg !152
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !152
  %6 = bitcast i8* %5 to %1**, !dbg !152
  %7 = load %1*, %1** %6, align 8, !dbg !152
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !152
  %9 = load i8*, i8** %8, align 8, !dbg !152
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !152
  %11 = load i8*, i8** %10, align 8, !dbg !152
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_compute_(i8* %11, i8* %9), !dbg !152
  ret i32 %12, !dbg !152
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %11, align 8
  %3 = getelementptr inbounds %11, %11* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %11, %11* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %11* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.7, i8* nonnull %5, i32 5)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.7(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 191
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 192
  %15 = select i1 %14, i32 %13, i32 192
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 192
  %18 = select i1 %17, i32 %16, i32 192
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_body
  %indvars.iv = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next, %for_body ]
  %24 = mul nsw i64 %indvars.iv, 48
  %25 = trunc i64 %indvars.iv to i32
  %26 = srem i32 %25, 6
  %27 = mul nsw i32 %26, 208
  %28 = sdiv i32 %25, 6
  %29 = mul nsw i32 %28, 1352
  %30 = add nsw i32 %29, %27
  %31 = getelementptr inbounds float, float* %4, i64 %24
  %32 = bitcast float* %31 to <8 x float>*
  %33 = sext i32 %30 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to <8 x float>*
  %36 = load <8 x float>, <8 x float>* %35, align 32, !tbaa !153
  %37 = fcmp olt <8 x float> %36, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %38 = select <8 x i1> %37, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %36
  %39 = add i32 %30, 8
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to <8 x float>*
  %43 = load <8 x float>, <8 x float>* %42, align 32, !tbaa !153
  %44 = fcmp ogt <8 x float> %38, %43
  %45 = select <8 x i1> %44, <8 x float> %38, <8 x float> %43
  %46 = add i32 %30, 16
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to <8 x float>*
  %50 = load <8 x float>, <8 x float>* %49, align 32, !tbaa !153
  %51 = fcmp ogt <8 x float> %45, %50
  %52 = select <8 x i1> %51, <8 x float> %45, <8 x float> %50
  %53 = add i32 %30, 104
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %7, i64 %54
  %56 = bitcast float* %55 to <8 x float>*
  %57 = load <8 x float>, <8 x float>* %56, align 32, !tbaa !153
  %58 = fcmp ogt <8 x float> %52, %57
  %59 = select <8 x i1> %58, <8 x float> %52, <8 x float> %57
  %60 = add i32 %30, 112
  %61 = sext i32 %60 to i64
  %62 = getelementptr inbounds float, float* %7, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %64 = load <8 x float>, <8 x float>* %63, align 32, !tbaa !153
  %65 = fcmp ogt <8 x float> %59, %64
  %66 = select <8 x i1> %65, <8 x float> %59, <8 x float> %64
  %67 = add i32 %30, 120
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds float, float* %7, i64 %68
  %70 = bitcast float* %69 to <8 x float>*
  %71 = load <8 x float>, <8 x float>* %70, align 32, !tbaa !153
  %72 = fcmp ogt <8 x float> %66, %71
  %73 = select <8 x i1> %72, <8 x float> %66, <8 x float> %71
  %74 = add i32 %30, 208
  %75 = sext i32 %74 to i64
  %76 = getelementptr inbounds float, float* %7, i64 %75
  %77 = bitcast float* %76 to <8 x float>*
  %78 = load <8 x float>, <8 x float>* %77, align 32, !tbaa !153
  %79 = fcmp ogt <8 x float> %73, %78
  %80 = select <8 x i1> %79, <8 x float> %73, <8 x float> %78
  %81 = add i32 %30, 216
  %82 = sext i32 %81 to i64
  %83 = getelementptr inbounds float, float* %7, i64 %82
  %84 = bitcast float* %83 to <8 x float>*
  %85 = load <8 x float>, <8 x float>* %84, align 32, !tbaa !153
  %86 = fcmp ogt <8 x float> %80, %85
  %87 = select <8 x i1> %86, <8 x float> %80, <8 x float> %85
  %88 = add i32 %30, 224
  %89 = sext i32 %88 to i64
  %90 = getelementptr inbounds float, float* %7, i64 %89
  %91 = bitcast float* %90 to <8 x float>*
  %92 = load <8 x float>, <8 x float>* %91, align 32, !tbaa !153
  %93 = fcmp ogt <8 x float> %87, %92
  %94 = select <8 x i1> %93, <8 x float> %87, <8 x float> %92
  store <8 x float> %94, <8 x float>* %32, align 32, !tbaa !156
  %95 = or i64 %24, 8
  %96 = getelementptr inbounds float, float* %4, i64 %95
  %97 = bitcast float* %96 to <8 x float>*
  %98 = fcmp olt <8 x float> %50, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %99 = select <8 x i1> %98, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %50
  %100 = add i32 %30, 24
  %101 = sext i32 %100 to i64
  %102 = getelementptr inbounds float, float* %7, i64 %101
  %103 = bitcast float* %102 to <8 x float>*
  %104 = load <8 x float>, <8 x float>* %103, align 32, !tbaa !153
  %105 = fcmp ogt <8 x float> %99, %104
  %106 = select <8 x i1> %105, <8 x float> %99, <8 x float> %104
  %107 = add i32 %30, 32
  %108 = sext i32 %107 to i64
  %109 = getelementptr inbounds float, float* %7, i64 %108
  %110 = bitcast float* %109 to <8 x float>*
  %111 = load <8 x float>, <8 x float>* %110, align 32, !tbaa !153
  %112 = fcmp ogt <8 x float> %106, %111
  %113 = select <8 x i1> %112, <8 x float> %106, <8 x float> %111
  %114 = add i32 %30, 120
  %115 = sext i32 %114 to i64
  %116 = getelementptr inbounds float, float* %7, i64 %115
  %117 = bitcast float* %116 to <8 x float>*
  %118 = load <8 x float>, <8 x float>* %117, align 32, !tbaa !153
  %119 = fcmp ogt <8 x float> %113, %118
  %120 = select <8 x i1> %119, <8 x float> %113, <8 x float> %118
  %121 = add i32 %30, 128
  %122 = sext i32 %121 to i64
  %123 = getelementptr inbounds float, float* %7, i64 %122
  %124 = bitcast float* %123 to <8 x float>*
  %125 = load <8 x float>, <8 x float>* %124, align 32, !tbaa !153
  %126 = fcmp ogt <8 x float> %120, %125
  %127 = select <8 x i1> %126, <8 x float> %120, <8 x float> %125
  %128 = add i32 %30, 136
  %129 = sext i32 %128 to i64
  %130 = getelementptr inbounds float, float* %7, i64 %129
  %131 = bitcast float* %130 to <8 x float>*
  %132 = load <8 x float>, <8 x float>* %131, align 32, !tbaa !153
  %133 = fcmp ogt <8 x float> %127, %132
  %134 = select <8 x i1> %133, <8 x float> %127, <8 x float> %132
  %135 = add i32 %30, 224
  %136 = sext i32 %135 to i64
  %137 = getelementptr inbounds float, float* %7, i64 %136
  %138 = bitcast float* %137 to <8 x float>*
  %139 = load <8 x float>, <8 x float>* %138, align 32, !tbaa !153
  %140 = fcmp ogt <8 x float> %134, %139
  %141 = select <8 x i1> %140, <8 x float> %134, <8 x float> %139
  %142 = add i32 %30, 232
  %143 = sext i32 %142 to i64
  %144 = getelementptr inbounds float, float* %7, i64 %143
  %145 = bitcast float* %144 to <8 x float>*
  %146 = load <8 x float>, <8 x float>* %145, align 32, !tbaa !153
  %147 = fcmp ogt <8 x float> %141, %146
  %148 = select <8 x i1> %147, <8 x float> %141, <8 x float> %146
  %149 = add i32 %30, 240
  %150 = sext i32 %149 to i64
  %151 = getelementptr inbounds float, float* %7, i64 %150
  %152 = bitcast float* %151 to <8 x float>*
  %153 = load <8 x float>, <8 x float>* %152, align 32, !tbaa !153
  %154 = fcmp ogt <8 x float> %148, %153
  %155 = select <8 x i1> %154, <8 x float> %148, <8 x float> %153
  store <8 x float> %155, <8 x float>* %97, align 32, !tbaa !156
  %156 = add nsw i64 %24, 16
  %157 = getelementptr inbounds float, float* %4, i64 %156
  %158 = bitcast float* %157 to <8 x float>*
  %159 = add i32 %30, 32
  %160 = sext i32 %159 to i64
  %161 = getelementptr inbounds float, float* %7, i64 %160
  %162 = bitcast float* %161 to <8 x float>*
  %163 = load <8 x float>, <8 x float>* %162, align 32, !tbaa !153
  %164 = fcmp olt <8 x float> %163, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %165 = select <8 x i1> %164, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %163
  %166 = add i32 %30, 40
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %7, i64 %167
  %169 = bitcast float* %168 to <8 x float>*
  %170 = load <8 x float>, <8 x float>* %169, align 32, !tbaa !153
  %171 = fcmp ogt <8 x float> %165, %170
  %172 = select <8 x i1> %171, <8 x float> %165, <8 x float> %170
  %173 = add i32 %30, 48
  %174 = sext i32 %173 to i64
  %175 = getelementptr inbounds float, float* %7, i64 %174
  %176 = bitcast float* %175 to <8 x float>*
  %177 = load <8 x float>, <8 x float>* %176, align 32, !tbaa !153
  %178 = fcmp ogt <8 x float> %172, %177
  %179 = select <8 x i1> %178, <8 x float> %172, <8 x float> %177
  %180 = add i32 %30, 136
  %181 = sext i32 %180 to i64
  %182 = getelementptr inbounds float, float* %7, i64 %181
  %183 = bitcast float* %182 to <8 x float>*
  %184 = load <8 x float>, <8 x float>* %183, align 32, !tbaa !153
  %185 = fcmp ogt <8 x float> %179, %184
  %186 = select <8 x i1> %185, <8 x float> %179, <8 x float> %184
  %187 = add i32 %30, 144
  %188 = sext i32 %187 to i64
  %189 = getelementptr inbounds float, float* %7, i64 %188
  %190 = bitcast float* %189 to <8 x float>*
  %191 = load <8 x float>, <8 x float>* %190, align 32, !tbaa !153
  %192 = fcmp ogt <8 x float> %186, %191
  %193 = select <8 x i1> %192, <8 x float> %186, <8 x float> %191
  %194 = add i32 %30, 152
  %195 = sext i32 %194 to i64
  %196 = getelementptr inbounds float, float* %7, i64 %195
  %197 = bitcast float* %196 to <8 x float>*
  %198 = load <8 x float>, <8 x float>* %197, align 32, !tbaa !153
  %199 = fcmp ogt <8 x float> %193, %198
  %200 = select <8 x i1> %199, <8 x float> %193, <8 x float> %198
  %201 = add i32 %30, 240
  %202 = sext i32 %201 to i64
  %203 = getelementptr inbounds float, float* %7, i64 %202
  %204 = bitcast float* %203 to <8 x float>*
  %205 = load <8 x float>, <8 x float>* %204, align 32, !tbaa !153
  %206 = fcmp ogt <8 x float> %200, %205
  %207 = select <8 x i1> %206, <8 x float> %200, <8 x float> %205
  %208 = add i32 %30, 248
  %209 = sext i32 %208 to i64
  %210 = getelementptr inbounds float, float* %7, i64 %209
  %211 = bitcast float* %210 to <8 x float>*
  %212 = load <8 x float>, <8 x float>* %211, align 32, !tbaa !153
  %213 = fcmp ogt <8 x float> %207, %212
  %214 = select <8 x i1> %213, <8 x float> %207, <8 x float> %212
  %215 = add i32 %30, 256
  %216 = sext i32 %215 to i64
  %217 = getelementptr inbounds float, float* %7, i64 %216
  %218 = bitcast float* %217 to <8 x float>*
  %219 = load <8 x float>, <8 x float>* %218, align 32, !tbaa !153
  %220 = fcmp ogt <8 x float> %214, %219
  %221 = select <8 x i1> %220, <8 x float> %214, <8 x float> %219
  store <8 x float> %221, <8 x float>* %158, align 32, !tbaa !156
  %222 = add nsw i64 %24, 24
  %223 = getelementptr inbounds float, float* %4, i64 %222
  %224 = bitcast float* %223 to <8 x float>*
  %225 = add i32 %30, 48
  %226 = sext i32 %225 to i64
  %227 = getelementptr inbounds float, float* %7, i64 %226
  %228 = bitcast float* %227 to <8 x float>*
  %229 = load <8 x float>, <8 x float>* %228, align 32, !tbaa !153
  %230 = fcmp olt <8 x float> %229, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %231 = select <8 x i1> %230, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %229
  %232 = add i32 %30, 56
  %233 = sext i32 %232 to i64
  %234 = getelementptr inbounds float, float* %7, i64 %233
  %235 = bitcast float* %234 to <8 x float>*
  %236 = load <8 x float>, <8 x float>* %235, align 32, !tbaa !153
  %237 = fcmp ogt <8 x float> %231, %236
  %238 = select <8 x i1> %237, <8 x float> %231, <8 x float> %236
  %239 = add i32 %30, 64
  %240 = sext i32 %239 to i64
  %241 = getelementptr inbounds float, float* %7, i64 %240
  %242 = bitcast float* %241 to <8 x float>*
  %243 = load <8 x float>, <8 x float>* %242, align 32, !tbaa !153
  %244 = fcmp ogt <8 x float> %238, %243
  %245 = select <8 x i1> %244, <8 x float> %238, <8 x float> %243
  %246 = add i32 %30, 152
  %247 = sext i32 %246 to i64
  %248 = getelementptr inbounds float, float* %7, i64 %247
  %249 = bitcast float* %248 to <8 x float>*
  %250 = load <8 x float>, <8 x float>* %249, align 32, !tbaa !153
  %251 = fcmp ogt <8 x float> %245, %250
  %252 = select <8 x i1> %251, <8 x float> %245, <8 x float> %250
  %253 = add i32 %30, 160
  %254 = sext i32 %253 to i64
  %255 = getelementptr inbounds float, float* %7, i64 %254
  %256 = bitcast float* %255 to <8 x float>*
  %257 = load <8 x float>, <8 x float>* %256, align 32, !tbaa !153
  %258 = fcmp ogt <8 x float> %252, %257
  %259 = select <8 x i1> %258, <8 x float> %252, <8 x float> %257
  %260 = add i32 %30, 168
  %261 = sext i32 %260 to i64
  %262 = getelementptr inbounds float, float* %7, i64 %261
  %263 = bitcast float* %262 to <8 x float>*
  %264 = load <8 x float>, <8 x float>* %263, align 32, !tbaa !153
  %265 = fcmp ogt <8 x float> %259, %264
  %266 = select <8 x i1> %265, <8 x float> %259, <8 x float> %264
  %267 = add i32 %30, 256
  %268 = sext i32 %267 to i64
  %269 = getelementptr inbounds float, float* %7, i64 %268
  %270 = bitcast float* %269 to <8 x float>*
  %271 = load <8 x float>, <8 x float>* %270, align 32, !tbaa !153
  %272 = fcmp ogt <8 x float> %266, %271
  %273 = select <8 x i1> %272, <8 x float> %266, <8 x float> %271
  %274 = add i32 %30, 264
  %275 = sext i32 %274 to i64
  %276 = getelementptr inbounds float, float* %7, i64 %275
  %277 = bitcast float* %276 to <8 x float>*
  %278 = load <8 x float>, <8 x float>* %277, align 32, !tbaa !153
  %279 = fcmp ogt <8 x float> %273, %278
  %280 = select <8 x i1> %279, <8 x float> %273, <8 x float> %278
  %281 = add i32 %30, 272
  %282 = sext i32 %281 to i64
  %283 = getelementptr inbounds float, float* %7, i64 %282
  %284 = bitcast float* %283 to <8 x float>*
  %285 = load <8 x float>, <8 x float>* %284, align 32, !tbaa !153
  %286 = fcmp ogt <8 x float> %280, %285
  %287 = select <8 x i1> %286, <8 x float> %280, <8 x float> %285
  store <8 x float> %287, <8 x float>* %224, align 32, !tbaa !156
  %288 = add nsw i64 %24, 32
  %289 = getelementptr inbounds float, float* %4, i64 %288
  %290 = bitcast float* %289 to <8 x float>*
  %291 = add i32 %30, 64
  %292 = sext i32 %291 to i64
  %293 = getelementptr inbounds float, float* %7, i64 %292
  %294 = bitcast float* %293 to <8 x float>*
  %295 = load <8 x float>, <8 x float>* %294, align 32, !tbaa !153
  %296 = fcmp olt <8 x float> %295, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %297 = select <8 x i1> %296, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %295
  %298 = add i32 %30, 72
  %299 = sext i32 %298 to i64
  %300 = getelementptr inbounds float, float* %7, i64 %299
  %301 = bitcast float* %300 to <8 x float>*
  %302 = load <8 x float>, <8 x float>* %301, align 32, !tbaa !153
  %303 = fcmp ogt <8 x float> %297, %302
  %304 = select <8 x i1> %303, <8 x float> %297, <8 x float> %302
  %305 = add i32 %30, 80
  %306 = sext i32 %305 to i64
  %307 = getelementptr inbounds float, float* %7, i64 %306
  %308 = bitcast float* %307 to <8 x float>*
  %309 = load <8 x float>, <8 x float>* %308, align 32, !tbaa !153
  %310 = fcmp ogt <8 x float> %304, %309
  %311 = select <8 x i1> %310, <8 x float> %304, <8 x float> %309
  %312 = add i32 %30, 168
  %313 = sext i32 %312 to i64
  %314 = getelementptr inbounds float, float* %7, i64 %313
  %315 = bitcast float* %314 to <8 x float>*
  %316 = load <8 x float>, <8 x float>* %315, align 32, !tbaa !153
  %317 = fcmp ogt <8 x float> %311, %316
  %318 = select <8 x i1> %317, <8 x float> %311, <8 x float> %316
  %319 = add i32 %30, 176
  %320 = sext i32 %319 to i64
  %321 = getelementptr inbounds float, float* %7, i64 %320
  %322 = bitcast float* %321 to <8 x float>*
  %323 = load <8 x float>, <8 x float>* %322, align 32, !tbaa !153
  %324 = fcmp ogt <8 x float> %318, %323
  %325 = select <8 x i1> %324, <8 x float> %318, <8 x float> %323
  %326 = add i32 %30, 184
  %327 = sext i32 %326 to i64
  %328 = getelementptr inbounds float, float* %7, i64 %327
  %329 = bitcast float* %328 to <8 x float>*
  %330 = load <8 x float>, <8 x float>* %329, align 32, !tbaa !153
  %331 = fcmp ogt <8 x float> %325, %330
  %332 = select <8 x i1> %331, <8 x float> %325, <8 x float> %330
  %333 = add i32 %30, 272
  %334 = sext i32 %333 to i64
  %335 = getelementptr inbounds float, float* %7, i64 %334
  %336 = bitcast float* %335 to <8 x float>*
  %337 = load <8 x float>, <8 x float>* %336, align 32, !tbaa !153
  %338 = fcmp ogt <8 x float> %332, %337
  %339 = select <8 x i1> %338, <8 x float> %332, <8 x float> %337
  %340 = add i32 %30, 280
  %341 = sext i32 %340 to i64
  %342 = getelementptr inbounds float, float* %7, i64 %341
  %343 = bitcast float* %342 to <8 x float>*
  %344 = load <8 x float>, <8 x float>* %343, align 32, !tbaa !153
  %345 = fcmp ogt <8 x float> %339, %344
  %346 = select <8 x i1> %345, <8 x float> %339, <8 x float> %344
  %347 = add i32 %30, 288
  %348 = sext i32 %347 to i64
  %349 = getelementptr inbounds float, float* %7, i64 %348
  %350 = bitcast float* %349 to <8 x float>*
  %351 = load <8 x float>, <8 x float>* %350, align 32, !tbaa !153
  %352 = fcmp ogt <8 x float> %346, %351
  %353 = select <8 x i1> %352, <8 x float> %346, <8 x float> %351
  store <8 x float> %353, <8 x float>* %290, align 32, !tbaa !156
  %354 = add nsw i64 %24, 40
  %355 = getelementptr inbounds float, float* %4, i64 %354
  %356 = bitcast float* %355 to <8 x float>*
  %357 = add i32 %30, 80
  %358 = sext i32 %357 to i64
  %359 = getelementptr inbounds float, float* %7, i64 %358
  %360 = bitcast float* %359 to <8 x float>*
  %361 = load <8 x float>, <8 x float>* %360, align 32, !tbaa !153
  %362 = fcmp olt <8 x float> %361, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %363 = select <8 x i1> %362, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %361
  %364 = add i32 %30, 88
  %365 = sext i32 %364 to i64
  %366 = getelementptr inbounds float, float* %7, i64 %365
  %367 = bitcast float* %366 to <8 x float>*
  %368 = load <8 x float>, <8 x float>* %367, align 32, !tbaa !153
  %369 = fcmp ogt <8 x float> %363, %368
  %370 = select <8 x i1> %369, <8 x float> %363, <8 x float> %368
  %371 = add i32 %30, 96
  %372 = sext i32 %371 to i64
  %373 = getelementptr inbounds float, float* %7, i64 %372
  %374 = bitcast float* %373 to <8 x float>*
  %375 = load <8 x float>, <8 x float>* %374, align 32, !tbaa !153
  %376 = fcmp ogt <8 x float> %370, %375
  %377 = select <8 x i1> %376, <8 x float> %370, <8 x float> %375
  %378 = add i32 %30, 184
  %379 = sext i32 %378 to i64
  %380 = getelementptr inbounds float, float* %7, i64 %379
  %381 = bitcast float* %380 to <8 x float>*
  %382 = load <8 x float>, <8 x float>* %381, align 32, !tbaa !153
  %383 = fcmp ogt <8 x float> %377, %382
  %384 = select <8 x i1> %383, <8 x float> %377, <8 x float> %382
  %385 = add i32 %30, 192
  %386 = sext i32 %385 to i64
  %387 = getelementptr inbounds float, float* %7, i64 %386
  %388 = bitcast float* %387 to <8 x float>*
  %389 = load <8 x float>, <8 x float>* %388, align 32, !tbaa !153
  %390 = fcmp ogt <8 x float> %384, %389
  %391 = select <8 x i1> %390, <8 x float> %384, <8 x float> %389
  %392 = add i32 %30, 200
  %393 = sext i32 %392 to i64
  %394 = getelementptr inbounds float, float* %7, i64 %393
  %395 = bitcast float* %394 to <8 x float>*
  %396 = load <8 x float>, <8 x float>* %395, align 32, !tbaa !153
  %397 = fcmp ogt <8 x float> %391, %396
  %398 = select <8 x i1> %397, <8 x float> %391, <8 x float> %396
  %399 = add i32 %30, 288
  %400 = sext i32 %399 to i64
  %401 = getelementptr inbounds float, float* %7, i64 %400
  %402 = bitcast float* %401 to <8 x float>*
  %403 = load <8 x float>, <8 x float>* %402, align 32, !tbaa !153
  %404 = fcmp ogt <8 x float> %398, %403
  %405 = select <8 x i1> %404, <8 x float> %398, <8 x float> %403
  %406 = add i32 %30, 296
  %407 = sext i32 %406 to i64
  %408 = getelementptr inbounds float, float* %7, i64 %407
  %409 = bitcast float* %408 to <8 x float>*
  %410 = load <8 x float>, <8 x float>* %409, align 32, !tbaa !153
  %411 = fcmp ogt <8 x float> %405, %410
  %412 = select <8 x i1> %411, <8 x float> %405, <8 x float> %410
  %413 = add i32 %30, 304
  %414 = sext i32 %413 to i64
  %415 = getelementptr inbounds float, float* %7, i64 %414
  %416 = bitcast float* %415 to <8 x float>*
  %417 = load <8 x float>, <8 x float>* %416, align 32, !tbaa !153
  %418 = fcmp ogt <8 x float> %412, %417
  %419 = select <8 x i1> %418, <8 x float> %412, <8 x float> %417
  store <8 x float> %419, <8 x float>* %356, align 32, !tbaa !156
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %420 = icmp slt i64 %indvars.iv.next, %23
  br i1 %420, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !159 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !161, metadata !DIExpression()), !dbg !164
  call void @llvm.dbg.value(metadata i8* %1, metadata !162, metadata !DIExpression()), !dbg !164
  call void @llvm.dbg.value(metadata i32 %2, metadata !163, metadata !DIExpression()), !dbg !164
  %3 = bitcast i8* %0 to %1**, !dbg !164
  %4 = load %1*, %1** %3, align 8, !dbg !164
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !164
  %6 = bitcast i8* %5 to %1**, !dbg !164
  %7 = load %1*, %1** %6, align 8, !dbg !164
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !164
  %9 = bitcast i8* %8 to %1**, !dbg !164
  %10 = load %1*, %1** %9, align 8, !dbg !164
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !164
  %12 = bitcast i8* %11 to %1**, !dbg !164
  %13 = load %1*, %1** %12, align 8, !dbg !164
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !164
  %15 = load i8*, i8** %14, align 8, !dbg !164
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !164
  %17 = load i32, i32* %16, align 4, !dbg !164
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !164
  %19 = load i8*, i8** %18, align 8, !dbg !164
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !164
  %21 = load i8*, i8** %20, align 8, !dbg !164
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !164
  %23 = load i8*, i8** %22, align 8, !dbg !164
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !164
  ret i32 %24, !dbg !164
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 230400, i32 2, i32 32)
  %7 = alloca %12, align 8
  %8 = getelementptr inbounds %12, %12* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %12, %12* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %12* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.8, i8* nonnull %10, i32 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %21, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %13, align 8
  %15 = getelementptr inbounds %13, %13* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %13, %13* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %13, %13* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %13, %13* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = bitcast %13* %14 to i8*
  %20 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %21 = call i32 %20(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.9, i8* nonnull %19, i32 5)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %23 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %24 = call i32 %23(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.8(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 479
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 480
  %15 = select i1 %14, i32 %13, i32 480
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 480
  %18 = select i1 %17, i32 %16, i32 480
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %219, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 120
  %22 = srem i32 %20, 15
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 13
  %24 = sdiv i32 %20, 15
  %25 = mul nsw i32 %24, 1352
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 104
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 3
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 13
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 15
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !28

for_body2.split.us.us:                            ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -112
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to i32*
  %40 = load i32, i32* %39, align 4, !tbaa !165
  %41 = sext i32 %32 to i64
  %42 = getelementptr inbounds float, float* %4, i64 %41
  %43 = bitcast float* %42 to i32*
  store i32 %40, i32* %43, align 4, !tbaa !168
  %44 = or i64 %27, 1
  %45 = trunc i64 %44 to i32
  %46 = add i32 %21, %45
  %47 = trunc i64 %44 to i32
  %48 = add i32 %47, -112
  %49 = add i32 %26, %48
  %50 = add i32 %49, %25
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to i32*
  %54 = load i32, i32* %53, align 4, !tbaa !165
  %55 = sext i32 %46 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to i32*
  store i32 %54, i32* %57, align 4, !tbaa !168
  %58 = or i64 %27, 2
  %59 = trunc i64 %58 to i32
  %60 = add i32 %21, %59
  %61 = trunc i64 %58 to i32
  %62 = add i32 %61, -112
  %63 = add i32 %26, %62
  %64 = add i32 %63, %25
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to i32*
  %68 = load i32, i32* %67, align 4, !tbaa !165
  %69 = sext i32 %60 to i64
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = bitcast float* %70 to i32*
  store i32 %68, i32* %71, align 4, !tbaa !168
  %72 = or i64 %27, 3
  %73 = trunc i64 %72 to i32
  %74 = add i32 %21, %73
  %75 = trunc i64 %72 to i32
  %76 = add i32 %75, -112
  %77 = add i32 %26, %76
  %78 = add i32 %77, %25
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !165
  %83 = sext i32 %74 to i64
  %84 = getelementptr inbounds float, float* %4, i64 %83
  %85 = bitcast float* %84 to i32*
  store i32 %82, i32* %85, align 4, !tbaa !168
  %86 = or i64 %27, 4
  %87 = trunc i64 %86 to i32
  %88 = add i32 %21, %87
  %89 = trunc i64 %86 to i32
  %90 = add i32 %89, -112
  %91 = add i32 %26, %90
  %92 = add i32 %91, %25
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to i32*
  %96 = load i32, i32* %95, align 4, !tbaa !165
  %97 = sext i32 %88 to i64
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = bitcast float* %98 to i32*
  store i32 %96, i32* %99, align 4, !tbaa !168
  %100 = or i64 %27, 5
  %101 = trunc i64 %100 to i32
  %102 = add i32 %21, %101
  %103 = trunc i64 %100 to i32
  %104 = add i32 %103, -112
  %105 = add i32 %26, %104
  %106 = add i32 %105, %25
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to i32*
  %110 = load i32, i32* %109, align 4, !tbaa !165
  %111 = sext i32 %102 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to i32*
  store i32 %110, i32* %113, align 4, !tbaa !168
  %114 = or i64 %27, 6
  %115 = trunc i64 %114 to i32
  %116 = add i32 %21, %115
  %117 = trunc i64 %114 to i32
  %118 = add i32 %117, -112
  %119 = add i32 %26, %118
  %120 = add i32 %119, %25
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to i32*
  %124 = load i32, i32* %123, align 4, !tbaa !165
  %125 = sext i32 %116 to i64
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = bitcast float* %126 to i32*
  store i32 %124, i32* %127, align 4, !tbaa !168
  %128 = or i64 %27, 7
  %129 = trunc i64 %128 to i32
  %130 = add i32 %21, %129
  %131 = trunc i64 %128 to i32
  %132 = add i32 %131, -112
  %133 = add i32 %26, %132
  %134 = add i32 %133, %25
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %7, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !165
  %139 = sext i32 %130 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to i32*
  store i32 %138, i32* %141, align 4, !tbaa !168
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %142 = sext i32 %32 to i64
  %143 = getelementptr inbounds float, float* %4, i64 %142
  store float 0.000000e+00, float* %143, align 4, !tbaa !168
  %144 = trunc i64 %27 to i32
  %145 = or i32 %144, 1
  %146 = add i32 %145, %21
  %147 = sext i32 %146 to i64
  %148 = getelementptr inbounds float, float* %4, i64 %147
  store float 0.000000e+00, float* %148, align 4, !tbaa !168
  %149 = trunc i64 %27 to i32
  %150 = or i32 %149, 2
  %151 = add i32 %150, %21
  %152 = sext i32 %151 to i64
  %153 = getelementptr inbounds float, float* %4, i64 %152
  store float 0.000000e+00, float* %153, align 4, !tbaa !168
  %154 = trunc i64 %27 to i32
  %155 = or i32 %154, 3
  %156 = add i32 %155, %21
  %157 = sext i32 %156 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  store float 0.000000e+00, float* %158, align 4, !tbaa !168
  %159 = trunc i64 %27 to i32
  %160 = or i32 %159, 4
  %161 = add i32 %160, %21
  %162 = sext i32 %161 to i64
  %163 = getelementptr inbounds float, float* %4, i64 %162
  store float 0.000000e+00, float* %163, align 4, !tbaa !168
  %164 = trunc i64 %27 to i32
  %165 = or i32 %164, 5
  %166 = add i32 %165, %21
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  store float 0.000000e+00, float* %168, align 4, !tbaa !168
  %169 = trunc i64 %27 to i32
  %170 = or i32 %169, 6
  %171 = add i32 %170, %21
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds float, float* %4, i64 %172
  store float 0.000000e+00, float* %173, align 4, !tbaa !168
  %174 = trunc i64 %27 to i32
  %175 = or i32 %174, 7
  %176 = add i32 %175, %21
  %177 = sext i32 %176 to i64
  %178 = getelementptr inbounds float, float* %4, i64 %177
  store float 0.000000e+00, float* %178, align 4, !tbaa !168
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %179 = shl nsw i64 %indvars.iv, 3
  %180 = trunc i64 %179 to i32
  %181 = add i32 %21, %180
  %182 = sext i32 %181 to i64
  %183 = getelementptr inbounds float, float* %4, i64 %182
  store float 0.000000e+00, float* %183, align 4, !tbaa !168
  %184 = trunc i64 %179 to i32
  %185 = or i32 %184, 1
  %186 = add i32 %185, %21
  %187 = sext i32 %186 to i64
  %188 = getelementptr inbounds float, float* %4, i64 %187
  store float 0.000000e+00, float* %188, align 4, !tbaa !168
  %189 = trunc i64 %179 to i32
  %190 = or i32 %189, 2
  %191 = add i32 %190, %21
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %4, i64 %192
  store float 0.000000e+00, float* %193, align 4, !tbaa !168
  %194 = trunc i64 %179 to i32
  %195 = or i32 %194, 3
  %196 = add i32 %195, %21
  %197 = sext i32 %196 to i64
  %198 = getelementptr inbounds float, float* %4, i64 %197
  store float 0.000000e+00, float* %198, align 4, !tbaa !168
  %199 = trunc i64 %179 to i32
  %200 = or i32 %199, 4
  %201 = add i32 %200, %21
  %202 = sext i32 %201 to i64
  %203 = getelementptr inbounds float, float* %4, i64 %202
  store float 0.000000e+00, float* %203, align 4, !tbaa !168
  %204 = trunc i64 %179 to i32
  %205 = or i32 %204, 5
  %206 = add i32 %205, %21
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  store float 0.000000e+00, float* %208, align 4, !tbaa !168
  %209 = trunc i64 %179 to i32
  %210 = or i32 %209, 6
  %211 = add i32 %210, %21
  %212 = sext i32 %211 to i64
  %213 = getelementptr inbounds float, float* %4, i64 %212
  store float 0.000000e+00, float* %213, align 4, !tbaa !168
  %214 = trunc i64 %179 to i32
  %215 = or i32 %214, 7
  %216 = add i32 %215, %21
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds float, float* %4, i64 %217
  store float 0.000000e+00, float* %218, align 4, !tbaa !168
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 15
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %219 = add nsw i32 %20, 1
  %220 = icmp slt i32 %219, %15
  br i1 %220, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.9(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds i8, i8* %2, i64 16
  %9 = bitcast i8* %8 to float**
  %10 = load float*, float** %9, align 8
  %11 = getelementptr inbounds i8, i8* %2, i64 24
  %12 = bitcast i8* %11 to float**
  %13 = load float*, float** %12, align 8
  %14 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %15 = load i32, i32* %14, align 4
  %16 = add nsw i32 %15, 415
  %17 = sdiv i32 %16, %15
  %18 = add nsw i32 %0, 1
  %19 = mul nsw i32 %17, %18
  %20 = icmp slt i32 %19, 416
  %21 = select i1 %20, i32 %19, i32 416
  %22 = mul nsw i32 %17, %0
  %23 = icmp slt i32 %22, 416
  %24 = select i1 %23, i32 %22, i32 416
  %25 = icmp slt i32 %24, %21
  br i1 %25, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %26 = add i32 %24, 1
  %27 = sext i32 %26 to i64
  %28 = add nsw i64 %27, -1
  %29 = sext i32 %21 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv138 = phi i64 [ %28, %for_body.lr.ph ], [ %indvars.iv.next139, %for_end3 ]
  %30 = trunc i64 %indvars.iv138 to i32
  %31 = srem i32 %30, 13
  %32 = sdiv i32 %30, 13
  %33 = mul nsw i32 %32, 18432
  %34 = sext i32 %33 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv132 = phi i64 [ 0, %for_body ], [ %indvars.iv.next133, %for_end6 ]
  %.lcssa38.lcssa.lcssa112 = phi <8 x float> [ zeroinitializer, %for_body ], [ %419, %for_end6 ]
  %.lcssa36.lcssa.lcssa110 = phi <8 x float> [ zeroinitializer, %for_body ], [ %413, %for_end6 ]
  %.lcssa34.lcssa.lcssa108 = phi <8 x float> [ zeroinitializer, %for_body ], [ %407, %for_end6 ]
  %.lcssa32.lcssa.lcssa106 = phi <8 x float> [ zeroinitializer, %for_body ], [ %401, %for_end6 ]
  %.lcssa30.lcssa.lcssa104 = phi <8 x float> [ zeroinitializer, %for_body ], [ %395, %for_end6 ]
  %.lcssa28.lcssa.lcssa102 = phi <8 x float> [ zeroinitializer, %for_body ], [ %389, %for_end6 ]
  %.lcssa26.lcssa.lcssa100 = phi <8 x float> [ zeroinitializer, %for_body ], [ %383, %for_end6 ]
  %.lcssa24.lcssa.lcssa98 = phi <8 x float> [ zeroinitializer, %for_body ], [ %377, %for_end6 ]
  %.lcssa22.lcssa.lcssa96 = phi <8 x float> [ zeroinitializer, %for_body ], [ %371, %for_end6 ]
  %.lcssa20.lcssa.lcssa95 = phi <8 x float> [ zeroinitializer, %for_body ], [ %365, %for_end6 ]
  %.lcssa18.lcssa.lcssa93 = phi <8 x float> [ zeroinitializer, %for_body ], [ %359, %for_end6 ]
  %.lcssa16.lcssa.lcssa91 = phi <8 x float> [ zeroinitializer, %for_body ], [ %353, %for_end6 ]
  %.lcssa.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body ], [ %347, %for_end6 ]
  %35 = mul nuw nsw i64 %indvars.iv132, 576
  %36 = add nsw i64 %35, %34
  %37 = trunc i64 %indvars.iv132 to i32
  %38 = mul i32 %37, 1800
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %39 = mul nsw i64 %indvars.iv138, 104
  %40 = shl nsw i32 %32, 3
  %41 = sext i32 %40 to i64
  %42 = getelementptr inbounds float, float* %13, i64 %41
  %43 = bitcast float* %42 to <8 x float>*
  %44 = load <8 x float>, <8 x float>* %43, align 32, !tbaa !171
  %45 = fadd <8 x float> %44, %347
  %46 = fcmp ogt <8 x float> %45, zeroinitializer
  %47 = select <8 x i1> %46, <8 x float> %45, <8 x float> zeroinitializer
  %48 = getelementptr inbounds float, float* %10, i64 %39
  %49 = bitcast float* %48 to <8 x float>*
  store <8 x float> %47, <8 x float>* %49, align 32, !tbaa !174
  %50 = add nsw i64 %39, 8
  %51 = fadd <8 x float> %44, %353
  %52 = fcmp ogt <8 x float> %51, zeroinitializer
  %53 = select <8 x i1> %52, <8 x float> %51, <8 x float> zeroinitializer
  %54 = getelementptr inbounds float, float* %10, i64 %50
  %55 = bitcast float* %54 to <8 x float>*
  store <8 x float> %53, <8 x float>* %55, align 32, !tbaa !174
  %56 = add nsw i64 %39, 16
  %57 = fadd <8 x float> %44, %359
  %58 = fcmp ogt <8 x float> %57, zeroinitializer
  %59 = select <8 x i1> %58, <8 x float> %57, <8 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %10, i64 %56
  %61 = bitcast float* %60 to <8 x float>*
  store <8 x float> %59, <8 x float>* %61, align 32, !tbaa !174
  %62 = add nsw i64 %39, 24
  %63 = fadd <8 x float> %44, %365
  %64 = fcmp ogt <8 x float> %63, zeroinitializer
  %65 = select <8 x i1> %64, <8 x float> %63, <8 x float> zeroinitializer
  %66 = getelementptr inbounds float, float* %10, i64 %62
  %67 = bitcast float* %66 to <8 x float>*
  store <8 x float> %65, <8 x float>* %67, align 32, !tbaa !174
  %68 = add nsw i64 %39, 32
  %69 = fadd <8 x float> %44, %371
  %70 = fcmp ogt <8 x float> %69, zeroinitializer
  %71 = select <8 x i1> %70, <8 x float> %69, <8 x float> zeroinitializer
  %72 = getelementptr inbounds float, float* %10, i64 %68
  %73 = bitcast float* %72 to <8 x float>*
  store <8 x float> %71, <8 x float>* %73, align 32, !tbaa !174
  %74 = add nsw i64 %39, 40
  %75 = fadd <8 x float> %44, %377
  %76 = fcmp ogt <8 x float> %75, zeroinitializer
  %77 = select <8 x i1> %76, <8 x float> %75, <8 x float> zeroinitializer
  %78 = getelementptr inbounds float, float* %10, i64 %74
  %79 = bitcast float* %78 to <8 x float>*
  store <8 x float> %77, <8 x float>* %79, align 32, !tbaa !174
  %80 = add nsw i64 %39, 48
  %81 = fadd <8 x float> %44, %383
  %82 = fcmp ogt <8 x float> %81, zeroinitializer
  %83 = select <8 x i1> %82, <8 x float> %81, <8 x float> zeroinitializer
  %84 = getelementptr inbounds float, float* %10, i64 %80
  %85 = bitcast float* %84 to <8 x float>*
  store <8 x float> %83, <8 x float>* %85, align 32, !tbaa !174
  %86 = add nsw i64 %39, 56
  %87 = fadd <8 x float> %44, %389
  %88 = fcmp ogt <8 x float> %87, zeroinitializer
  %89 = select <8 x i1> %88, <8 x float> %87, <8 x float> zeroinitializer
  %90 = getelementptr inbounds float, float* %10, i64 %86
  %91 = bitcast float* %90 to <8 x float>*
  store <8 x float> %89, <8 x float>* %91, align 32, !tbaa !174
  %92 = add nsw i64 %39, 64
  %93 = fadd <8 x float> %44, %395
  %94 = fcmp ogt <8 x float> %93, zeroinitializer
  %95 = select <8 x i1> %94, <8 x float> %93, <8 x float> zeroinitializer
  %96 = getelementptr inbounds float, float* %10, i64 %92
  %97 = bitcast float* %96 to <8 x float>*
  store <8 x float> %95, <8 x float>* %97, align 32, !tbaa !174
  %98 = add nsw i64 %39, 72
  %99 = fadd <8 x float> %44, %401
  %100 = fcmp ogt <8 x float> %99, zeroinitializer
  %101 = select <8 x i1> %100, <8 x float> %99, <8 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %10, i64 %98
  %103 = bitcast float* %102 to <8 x float>*
  store <8 x float> %101, <8 x float>* %103, align 32, !tbaa !174
  %104 = add nsw i64 %39, 80
  %105 = fadd <8 x float> %44, %407
  %106 = fcmp ogt <8 x float> %105, zeroinitializer
  %107 = select <8 x i1> %106, <8 x float> %105, <8 x float> zeroinitializer
  %108 = getelementptr inbounds float, float* %10, i64 %104
  %109 = bitcast float* %108 to <8 x float>*
  store <8 x float> %107, <8 x float>* %109, align 32, !tbaa !174
  %110 = add nsw i64 %39, 88
  %111 = fadd <8 x float> %44, %413
  %112 = fcmp ogt <8 x float> %111, zeroinitializer
  %113 = select <8 x i1> %112, <8 x float> %111, <8 x float> zeroinitializer
  %114 = getelementptr inbounds float, float* %10, i64 %110
  %115 = bitcast float* %114 to <8 x float>*
  store <8 x float> %113, <8 x float>* %115, align 32, !tbaa !174
  %116 = add nsw i64 %39, 96
  %117 = fadd <8 x float> %44, %419
  %118 = fcmp ogt <8 x float> %117, zeroinitializer
  %119 = select <8 x i1> %118, <8 x float> %117, <8 x float> zeroinitializer
  %120 = getelementptr inbounds float, float* %10, i64 %116
  %121 = bitcast float* %120 to <8 x float>*
  store <8 x float> %119, <8 x float>* %121, align 32, !tbaa !174
  %indvars.iv.next139 = add nsw i64 %indvars.iv138, 1
  %122 = icmp slt i64 %indvars.iv.next139, %29
  br i1 %122, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_end12.2, %for_body2
  %indvars.iv128 = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next129, %for_end12.2 ]
  %.lcssa38.lcssa87 = phi <8 x float> [ %.lcssa38.lcssa.lcssa112, %for_body2 ], [ %419, %for_end12.2 ]
  %.lcssa36.lcssa85 = phi <8 x float> [ %.lcssa36.lcssa.lcssa110, %for_body2 ], [ %413, %for_end12.2 ]
  %.lcssa34.lcssa83 = phi <8 x float> [ %.lcssa34.lcssa.lcssa108, %for_body2 ], [ %407, %for_end12.2 ]
  %.lcssa32.lcssa81 = phi <8 x float> [ %.lcssa32.lcssa.lcssa106, %for_body2 ], [ %401, %for_end12.2 ]
  %.lcssa30.lcssa79 = phi <8 x float> [ %.lcssa30.lcssa.lcssa104, %for_body2 ], [ %395, %for_end12.2 ]
  %.lcssa28.lcssa77 = phi <8 x float> [ %.lcssa28.lcssa.lcssa102, %for_body2 ], [ %389, %for_end12.2 ]
  %.lcssa26.lcssa75 = phi <8 x float> [ %.lcssa26.lcssa.lcssa100, %for_body2 ], [ %383, %for_end12.2 ]
  %.lcssa24.lcssa73 = phi <8 x float> [ %.lcssa24.lcssa.lcssa98, %for_body2 ], [ %377, %for_end12.2 ]
  %.lcssa22.lcssa71 = phi <8 x float> [ %.lcssa22.lcssa.lcssa96, %for_body2 ], [ %371, %for_end12.2 ]
  %.lcssa20.lcssa69 = phi <8 x float> [ %.lcssa20.lcssa.lcssa95, %for_body2 ], [ %365, %for_end12.2 ]
  %.lcssa18.lcssa68 = phi <8 x float> [ %.lcssa18.lcssa.lcssa93, %for_body2 ], [ %359, %for_end12.2 ]
  %.lcssa16.lcssa66 = phi <8 x float> [ %.lcssa16.lcssa.lcssa91, %for_body2 ], [ %353, %for_end12.2 ]
  %.lcssa.lcssa64 = phi <8 x float> [ %.lcssa.lcssa.lcssa89, %for_body2 ], [ %347, %for_end12.2 ]
  %123 = phi i32 [ 0, %for_body2 ], [ %420, %for_end12.2 ]
  %reass.add = add nsw i32 %123, %31
  %reass.mul = mul i32 %reass.add, 120
  %124 = add nsw i32 %reass.mul, %38
  %125 = mul nuw nsw i64 %indvars.iv128, 192
  %126 = add nsw i64 %36, %125
  %127 = sext i32 %124 to i64
  br label %for_body11

for_end6:                                         ; preds = %for_end12.2
  %indvars.iv.next133 = add nuw nsw i64 %indvars.iv132, 1
  %exitcond134 = icmp eq i64 %indvars.iv.next133, 32
  br i1 %exitcond134, label %for_end3, label %for_body2, !prof !28

for_body11:                                       ; preds = %for_body11, %for_body5
  %indvars.iv = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next, %for_body11 ]
  %128 = phi <8 x float> [ %.lcssa38.lcssa87, %for_body5 ], [ %223, %for_body11 ]
  %129 = phi <8 x float> [ %.lcssa36.lcssa85, %for_body5 ], [ %217, %for_body11 ]
  %130 = phi <8 x float> [ %.lcssa34.lcssa83, %for_body5 ], [ %211, %for_body11 ]
  %131 = phi <8 x float> [ %.lcssa32.lcssa81, %for_body5 ], [ %205, %for_body11 ]
  %132 = phi <8 x float> [ %.lcssa30.lcssa79, %for_body5 ], [ %199, %for_body11 ]
  %133 = phi <8 x float> [ %.lcssa28.lcssa77, %for_body5 ], [ %193, %for_body11 ]
  %134 = phi <8 x float> [ %.lcssa26.lcssa75, %for_body5 ], [ %187, %for_body11 ]
  %135 = phi <8 x float> [ %.lcssa24.lcssa73, %for_body5 ], [ %181, %for_body11 ]
  %136 = phi <8 x float> [ %.lcssa22.lcssa71, %for_body5 ], [ %175, %for_body11 ]
  %137 = phi <8 x float> [ %.lcssa20.lcssa69, %for_body5 ], [ %169, %for_body11 ]
  %138 = phi <8 x float> [ %.lcssa18.lcssa68, %for_body5 ], [ %163, %for_body11 ]
  %139 = phi <8 x float> [ %.lcssa16.lcssa66, %for_body5 ], [ %157, %for_body11 ]
  %140 = phi <8 x float> [ %.lcssa.lcssa64, %for_body5 ], [ %151, %for_body11 ]
  %141 = add nsw i64 %indvars.iv, %127
  %142 = getelementptr inbounds float, float* %4, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !168
  %144 = insertelement <8 x float> undef, float %143, i32 0
  %145 = shufflevector <8 x float> %144, <8 x float> undef, <8 x i32> zeroinitializer
  %146 = shl i64 %indvars.iv, 3
  %147 = add nsw i64 %126, %146
  %148 = getelementptr inbounds float, float* %7, i64 %147
  %149 = bitcast float* %148 to <8 x float>*
  %150 = load <8 x float>, <8 x float>* %149, align 32, !tbaa !177
  %151 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %145, <8 x float> %150, <8 x float> %140)
  %152 = add nsw i64 %141, 8
  %153 = getelementptr inbounds float, float* %4, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !168
  %155 = insertelement <8 x float> undef, float %154, i32 0
  %156 = shufflevector <8 x float> %155, <8 x float> undef, <8 x i32> zeroinitializer
  %157 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %156, <8 x float> %150, <8 x float> %139)
  %158 = add nsw i64 %141, 16
  %159 = getelementptr inbounds float, float* %4, i64 %158
  %160 = load float, float* %159, align 4, !tbaa !168
  %161 = insertelement <8 x float> undef, float %160, i32 0
  %162 = shufflevector <8 x float> %161, <8 x float> undef, <8 x i32> zeroinitializer
  %163 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %162, <8 x float> %150, <8 x float> %138)
  %164 = add nsw i64 %141, 24
  %165 = getelementptr inbounds float, float* %4, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !168
  %167 = insertelement <8 x float> undef, float %166, i32 0
  %168 = shufflevector <8 x float> %167, <8 x float> undef, <8 x i32> zeroinitializer
  %169 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %168, <8 x float> %150, <8 x float> %137)
  %170 = add nsw i64 %141, 32
  %171 = getelementptr inbounds float, float* %4, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !168
  %173 = insertelement <8 x float> undef, float %172, i32 0
  %174 = shufflevector <8 x float> %173, <8 x float> undef, <8 x i32> zeroinitializer
  %175 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %174, <8 x float> %150, <8 x float> %136)
  %176 = add nsw i64 %141, 40
  %177 = getelementptr inbounds float, float* %4, i64 %176
  %178 = load float, float* %177, align 4, !tbaa !168
  %179 = insertelement <8 x float> undef, float %178, i32 0
  %180 = shufflevector <8 x float> %179, <8 x float> undef, <8 x i32> zeroinitializer
  %181 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %180, <8 x float> %150, <8 x float> %135)
  %182 = add nsw i64 %141, 48
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !168
  %185 = insertelement <8 x float> undef, float %184, i32 0
  %186 = shufflevector <8 x float> %185, <8 x float> undef, <8 x i32> zeroinitializer
  %187 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %186, <8 x float> %150, <8 x float> %134)
  %188 = add nsw i64 %141, 56
  %189 = getelementptr inbounds float, float* %4, i64 %188
  %190 = load float, float* %189, align 4, !tbaa !168
  %191 = insertelement <8 x float> undef, float %190, i32 0
  %192 = shufflevector <8 x float> %191, <8 x float> undef, <8 x i32> zeroinitializer
  %193 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %192, <8 x float> %150, <8 x float> %133)
  %194 = add nsw i64 %141, 64
  %195 = getelementptr inbounds float, float* %4, i64 %194
  %196 = load float, float* %195, align 4, !tbaa !168
  %197 = insertelement <8 x float> undef, float %196, i32 0
  %198 = shufflevector <8 x float> %197, <8 x float> undef, <8 x i32> zeroinitializer
  %199 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %198, <8 x float> %150, <8 x float> %132)
  %200 = add nsw i64 %141, 72
  %201 = getelementptr inbounds float, float* %4, i64 %200
  %202 = load float, float* %201, align 4, !tbaa !168
  %203 = insertelement <8 x float> undef, float %202, i32 0
  %204 = shufflevector <8 x float> %203, <8 x float> undef, <8 x i32> zeroinitializer
  %205 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %204, <8 x float> %150, <8 x float> %131)
  %206 = add nsw i64 %141, 80
  %207 = getelementptr inbounds float, float* %4, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !168
  %209 = insertelement <8 x float> undef, float %208, i32 0
  %210 = shufflevector <8 x float> %209, <8 x float> undef, <8 x i32> zeroinitializer
  %211 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %210, <8 x float> %150, <8 x float> %130)
  %212 = add nsw i64 %141, 88
  %213 = getelementptr inbounds float, float* %4, i64 %212
  %214 = load float, float* %213, align 4, !tbaa !168
  %215 = insertelement <8 x float> undef, float %214, i32 0
  %216 = shufflevector <8 x float> %215, <8 x float> undef, <8 x i32> zeroinitializer
  %217 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %216, <8 x float> %150, <8 x float> %129)
  %218 = add nsw i64 %141, 96
  %219 = getelementptr inbounds float, float* %4, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !168
  %221 = insertelement <8 x float> undef, float %220, i32 0
  %222 = shufflevector <8 x float> %221, <8 x float> undef, <8 x i32> zeroinitializer
  %223 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %222, <8 x float> %150, <8 x float> %128)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end12, label %for_body11, !prof !28

for_end12:                                        ; preds = %for_body11
  %224 = add nsw i64 %127, 8
  %225 = add nsw i64 %126, 64
  br label %for_body11.1

for_body11.1:                                     ; preds = %for_body11.1, %for_end12
  %indvars.iv.1 = phi i64 [ 0, %for_end12 ], [ %indvars.iv.next.1, %for_body11.1 ]
  %226 = phi <8 x float> [ %223, %for_end12 ], [ %321, %for_body11.1 ]
  %227 = phi <8 x float> [ %217, %for_end12 ], [ %315, %for_body11.1 ]
  %228 = phi <8 x float> [ %211, %for_end12 ], [ %309, %for_body11.1 ]
  %229 = phi <8 x float> [ %205, %for_end12 ], [ %303, %for_body11.1 ]
  %230 = phi <8 x float> [ %199, %for_end12 ], [ %297, %for_body11.1 ]
  %231 = phi <8 x float> [ %193, %for_end12 ], [ %291, %for_body11.1 ]
  %232 = phi <8 x float> [ %187, %for_end12 ], [ %285, %for_body11.1 ]
  %233 = phi <8 x float> [ %181, %for_end12 ], [ %279, %for_body11.1 ]
  %234 = phi <8 x float> [ %175, %for_end12 ], [ %273, %for_body11.1 ]
  %235 = phi <8 x float> [ %169, %for_end12 ], [ %267, %for_body11.1 ]
  %236 = phi <8 x float> [ %163, %for_end12 ], [ %261, %for_body11.1 ]
  %237 = phi <8 x float> [ %157, %for_end12 ], [ %255, %for_body11.1 ]
  %238 = phi <8 x float> [ %151, %for_end12 ], [ %249, %for_body11.1 ]
  %239 = add nsw i64 %224, %indvars.iv.1
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !168
  %242 = insertelement <8 x float> undef, float %241, i32 0
  %243 = shufflevector <8 x float> %242, <8 x float> undef, <8 x i32> zeroinitializer
  %244 = shl i64 %indvars.iv.1, 3
  %245 = add nsw i64 %225, %244
  %246 = getelementptr inbounds float, float* %7, i64 %245
  %247 = bitcast float* %246 to <8 x float>*
  %248 = load <8 x float>, <8 x float>* %247, align 32, !tbaa !177
  %249 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %243, <8 x float> %248, <8 x float> %238)
  %250 = add nsw i64 %239, 8
  %251 = getelementptr inbounds float, float* %4, i64 %250
  %252 = load float, float* %251, align 4, !tbaa !168
  %253 = insertelement <8 x float> undef, float %252, i32 0
  %254 = shufflevector <8 x float> %253, <8 x float> undef, <8 x i32> zeroinitializer
  %255 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %254, <8 x float> %248, <8 x float> %237)
  %256 = add nsw i64 %239, 16
  %257 = getelementptr inbounds float, float* %4, i64 %256
  %258 = load float, float* %257, align 4, !tbaa !168
  %259 = insertelement <8 x float> undef, float %258, i32 0
  %260 = shufflevector <8 x float> %259, <8 x float> undef, <8 x i32> zeroinitializer
  %261 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %260, <8 x float> %248, <8 x float> %236)
  %262 = add nsw i64 %239, 24
  %263 = getelementptr inbounds float, float* %4, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !168
  %265 = insertelement <8 x float> undef, float %264, i32 0
  %266 = shufflevector <8 x float> %265, <8 x float> undef, <8 x i32> zeroinitializer
  %267 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %266, <8 x float> %248, <8 x float> %235)
  %268 = add nsw i64 %239, 32
  %269 = getelementptr inbounds float, float* %4, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !168
  %271 = insertelement <8 x float> undef, float %270, i32 0
  %272 = shufflevector <8 x float> %271, <8 x float> undef, <8 x i32> zeroinitializer
  %273 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %272, <8 x float> %248, <8 x float> %234)
  %274 = add nsw i64 %239, 40
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !168
  %277 = insertelement <8 x float> undef, float %276, i32 0
  %278 = shufflevector <8 x float> %277, <8 x float> undef, <8 x i32> zeroinitializer
  %279 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %278, <8 x float> %248, <8 x float> %233)
  %280 = add nsw i64 %239, 48
  %281 = getelementptr inbounds float, float* %4, i64 %280
  %282 = load float, float* %281, align 4, !tbaa !168
  %283 = insertelement <8 x float> undef, float %282, i32 0
  %284 = shufflevector <8 x float> %283, <8 x float> undef, <8 x i32> zeroinitializer
  %285 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %284, <8 x float> %248, <8 x float> %232)
  %286 = add nsw i64 %239, 56
  %287 = getelementptr inbounds float, float* %4, i64 %286
  %288 = load float, float* %287, align 4, !tbaa !168
  %289 = insertelement <8 x float> undef, float %288, i32 0
  %290 = shufflevector <8 x float> %289, <8 x float> undef, <8 x i32> zeroinitializer
  %291 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %290, <8 x float> %248, <8 x float> %231)
  %292 = add nsw i64 %239, 64
  %293 = getelementptr inbounds float, float* %4, i64 %292
  %294 = load float, float* %293, align 4, !tbaa !168
  %295 = insertelement <8 x float> undef, float %294, i32 0
  %296 = shufflevector <8 x float> %295, <8 x float> undef, <8 x i32> zeroinitializer
  %297 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %296, <8 x float> %248, <8 x float> %230)
  %298 = add nsw i64 %239, 72
  %299 = getelementptr inbounds float, float* %4, i64 %298
  %300 = load float, float* %299, align 4, !tbaa !168
  %301 = insertelement <8 x float> undef, float %300, i32 0
  %302 = shufflevector <8 x float> %301, <8 x float> undef, <8 x i32> zeroinitializer
  %303 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %302, <8 x float> %248, <8 x float> %229)
  %304 = add nsw i64 %239, 80
  %305 = getelementptr inbounds float, float* %4, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !168
  %307 = insertelement <8 x float> undef, float %306, i32 0
  %308 = shufflevector <8 x float> %307, <8 x float> undef, <8 x i32> zeroinitializer
  %309 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %308, <8 x float> %248, <8 x float> %228)
  %310 = add nsw i64 %239, 88
  %311 = getelementptr inbounds float, float* %4, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !168
  %313 = insertelement <8 x float> undef, float %312, i32 0
  %314 = shufflevector <8 x float> %313, <8 x float> undef, <8 x i32> zeroinitializer
  %315 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %314, <8 x float> %248, <8 x float> %227)
  %316 = add nsw i64 %239, 96
  %317 = getelementptr inbounds float, float* %4, i64 %316
  %318 = load float, float* %317, align 4, !tbaa !168
  %319 = insertelement <8 x float> undef, float %318, i32 0
  %320 = shufflevector <8 x float> %319, <8 x float> undef, <8 x i32> zeroinitializer
  %321 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %320, <8 x float> %248, <8 x float> %226)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 8
  br i1 %exitcond.1, label %for_end12.1, label %for_body11.1, !prof !28

for_end12.1:                                      ; preds = %for_body11.1
  %322 = add nsw i64 %127, 16
  %323 = add nsw i64 %126, 128
  br label %for_body11.2

for_body11.2:                                     ; preds = %for_body11.2, %for_end12.1
  %indvars.iv.2 = phi i64 [ 0, %for_end12.1 ], [ %indvars.iv.next.2, %for_body11.2 ]
  %324 = phi <8 x float> [ %321, %for_end12.1 ], [ %419, %for_body11.2 ]
  %325 = phi <8 x float> [ %315, %for_end12.1 ], [ %413, %for_body11.2 ]
  %326 = phi <8 x float> [ %309, %for_end12.1 ], [ %407, %for_body11.2 ]
  %327 = phi <8 x float> [ %303, %for_end12.1 ], [ %401, %for_body11.2 ]
  %328 = phi <8 x float> [ %297, %for_end12.1 ], [ %395, %for_body11.2 ]
  %329 = phi <8 x float> [ %291, %for_end12.1 ], [ %389, %for_body11.2 ]
  %330 = phi <8 x float> [ %285, %for_end12.1 ], [ %383, %for_body11.2 ]
  %331 = phi <8 x float> [ %279, %for_end12.1 ], [ %377, %for_body11.2 ]
  %332 = phi <8 x float> [ %273, %for_end12.1 ], [ %371, %for_body11.2 ]
  %333 = phi <8 x float> [ %267, %for_end12.1 ], [ %365, %for_body11.2 ]
  %334 = phi <8 x float> [ %261, %for_end12.1 ], [ %359, %for_body11.2 ]
  %335 = phi <8 x float> [ %255, %for_end12.1 ], [ %353, %for_body11.2 ]
  %336 = phi <8 x float> [ %249, %for_end12.1 ], [ %347, %for_body11.2 ]
  %337 = add nsw i64 %322, %indvars.iv.2
  %338 = getelementptr inbounds float, float* %4, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !168
  %340 = insertelement <8 x float> undef, float %339, i32 0
  %341 = shufflevector <8 x float> %340, <8 x float> undef, <8 x i32> zeroinitializer
  %342 = shl i64 %indvars.iv.2, 3
  %343 = add nsw i64 %323, %342
  %344 = getelementptr inbounds float, float* %7, i64 %343
  %345 = bitcast float* %344 to <8 x float>*
  %346 = load <8 x float>, <8 x float>* %345, align 32, !tbaa !177
  %347 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %341, <8 x float> %346, <8 x float> %336)
  %348 = add nsw i64 %337, 8
  %349 = getelementptr inbounds float, float* %4, i64 %348
  %350 = load float, float* %349, align 4, !tbaa !168
  %351 = insertelement <8 x float> undef, float %350, i32 0
  %352 = shufflevector <8 x float> %351, <8 x float> undef, <8 x i32> zeroinitializer
  %353 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %352, <8 x float> %346, <8 x float> %335)
  %354 = add nsw i64 %337, 16
  %355 = getelementptr inbounds float, float* %4, i64 %354
  %356 = load float, float* %355, align 4, !tbaa !168
  %357 = insertelement <8 x float> undef, float %356, i32 0
  %358 = shufflevector <8 x float> %357, <8 x float> undef, <8 x i32> zeroinitializer
  %359 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %358, <8 x float> %346, <8 x float> %334)
  %360 = add nsw i64 %337, 24
  %361 = getelementptr inbounds float, float* %4, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !168
  %363 = insertelement <8 x float> undef, float %362, i32 0
  %364 = shufflevector <8 x float> %363, <8 x float> undef, <8 x i32> zeroinitializer
  %365 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %364, <8 x float> %346, <8 x float> %333)
  %366 = add nsw i64 %337, 32
  %367 = getelementptr inbounds float, float* %4, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !168
  %369 = insertelement <8 x float> undef, float %368, i32 0
  %370 = shufflevector <8 x float> %369, <8 x float> undef, <8 x i32> zeroinitializer
  %371 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %370, <8 x float> %346, <8 x float> %332)
  %372 = add nsw i64 %337, 40
  %373 = getelementptr inbounds float, float* %4, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !168
  %375 = insertelement <8 x float> undef, float %374, i32 0
  %376 = shufflevector <8 x float> %375, <8 x float> undef, <8 x i32> zeroinitializer
  %377 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %376, <8 x float> %346, <8 x float> %331)
  %378 = add nsw i64 %337, 48
  %379 = getelementptr inbounds float, float* %4, i64 %378
  %380 = load float, float* %379, align 4, !tbaa !168
  %381 = insertelement <8 x float> undef, float %380, i32 0
  %382 = shufflevector <8 x float> %381, <8 x float> undef, <8 x i32> zeroinitializer
  %383 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %382, <8 x float> %346, <8 x float> %330)
  %384 = add nsw i64 %337, 56
  %385 = getelementptr inbounds float, float* %4, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !168
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shufflevector <8 x float> %387, <8 x float> undef, <8 x i32> zeroinitializer
  %389 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %388, <8 x float> %346, <8 x float> %329)
  %390 = add nsw i64 %337, 64
  %391 = getelementptr inbounds float, float* %4, i64 %390
  %392 = load float, float* %391, align 4, !tbaa !168
  %393 = insertelement <8 x float> undef, float %392, i32 0
  %394 = shufflevector <8 x float> %393, <8 x float> undef, <8 x i32> zeroinitializer
  %395 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %394, <8 x float> %346, <8 x float> %328)
  %396 = add nsw i64 %337, 72
  %397 = getelementptr inbounds float, float* %4, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !168
  %399 = insertelement <8 x float> undef, float %398, i32 0
  %400 = shufflevector <8 x float> %399, <8 x float> undef, <8 x i32> zeroinitializer
  %401 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %400, <8 x float> %346, <8 x float> %327)
  %402 = add nsw i64 %337, 80
  %403 = getelementptr inbounds float, float* %4, i64 %402
  %404 = load float, float* %403, align 4, !tbaa !168
  %405 = insertelement <8 x float> undef, float %404, i32 0
  %406 = shufflevector <8 x float> %405, <8 x float> undef, <8 x i32> zeroinitializer
  %407 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %406, <8 x float> %346, <8 x float> %326)
  %408 = add nsw i64 %337, 88
  %409 = getelementptr inbounds float, float* %4, i64 %408
  %410 = load float, float* %409, align 4, !tbaa !168
  %411 = insertelement <8 x float> undef, float %410, i32 0
  %412 = shufflevector <8 x float> %411, <8 x float> undef, <8 x i32> zeroinitializer
  %413 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %412, <8 x float> %346, <8 x float> %325)
  %414 = add nsw i64 %337, 96
  %415 = getelementptr inbounds float, float* %4, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !168
  %417 = insertelement <8 x float> undef, float %416, i32 0
  %418 = shufflevector <8 x float> %417, <8 x float> undef, <8 x i32> zeroinitializer
  %419 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %418, <8 x float> %346, <8 x float> %324)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 8
  br i1 %exitcond.2, label %for_end12.2, label %for_body11.2, !prof !28

for_end12.2:                                      ; preds = %for_body11.2
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %420 = add nuw nsw i32 %123, 1
  %exitcond131 = icmp eq i64 %indvars.iv.next129, 3
  br i1 %exitcond131, label %for_end6, label %for_body5, !prof !28
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !180 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !182, metadata !DIExpression()), !dbg !185
  call void @llvm.dbg.value(metadata i8* %1, metadata !183, metadata !DIExpression()), !dbg !185
  call void @llvm.dbg.value(metadata i32 %2, metadata !184, metadata !DIExpression()), !dbg !185
  %3 = bitcast i8* %0 to %1**, !dbg !185
  %4 = load %1*, %1** %3, align 8, !dbg !185
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !185
  %6 = bitcast i8* %5 to %1**, !dbg !185
  %7 = load %1*, %1** %6, align 8, !dbg !185
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !185
  %9 = bitcast i8* %8 to %1**, !dbg !185
  %10 = load %1*, %1** %9, align 8, !dbg !185
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !185
  %12 = bitcast i8* %11 to %1**, !dbg !185
  %13 = load %1*, %1** %12, align 8, !dbg !185
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !185
  %15 = load i8*, i8** %14, align 8, !dbg !185
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !185
  %17 = load i32, i32* %16, align 4, !dbg !185
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !185
  %19 = load i8*, i8** %18, align 8, !dbg !185
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !185
  %21 = load i8*, i8** %20, align 8, !dbg !185
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !185
  %23 = load i8*, i8** %22, align 8, !dbg !185
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !185
  ret i32 %24, !dbg !185
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 618348, i32 2, i32 32)
  %7 = alloca %14, align 8
  %8 = getelementptr inbounds %14, %14* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %14, %14* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %14* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.10, i8* nonnull %10, i32 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %15, align 8
  %15 = getelementptr inbounds %15, %15* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %15, %15* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %15, %15* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %15, %15* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %15, %15* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %15* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.11, i8* nonnull %20, i32 5)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.10(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 226
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 227
  %15 = select i1 %14, i32 %13, i32 227
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 227
  %18 = select i1 %17, i32 %16, i32 227
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %93, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 681
  %.off = add i32 %20, -2
  %22 = icmp ult i32 %.off, 224
  %23 = mul nsw i32 %20, 672
  br i1 %22, label %for_body2.us.preheader, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %for_end6.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body2.us.preheader ]
  %24 = mul nuw nsw i64 %indvars.iv21, 3
  %25 = trunc i64 %indvars.iv21 to i32
  %26 = add i32 %25, -2
  %27 = icmp ult i32 %26, 224
  %28 = trunc i64 %24 to i32
  %29 = add i32 %21, %28
  br i1 %27, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 227
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !28

for_body2.split.us.us:                            ; preds = %for_body2.us
  %30 = trunc i64 %24 to i32
  %31 = add i32 %30, -1350
  %32 = add i32 %31, %23
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to i32*
  %36 = load i32, i32* %35, align 4, !tbaa !186
  %37 = sext i32 %29 to i64
  %38 = getelementptr inbounds float, float* %4, i64 %37
  %39 = bitcast float* %38 to i32*
  store i32 %36, i32* %39, align 4, !tbaa !189
  %40 = trunc i64 %24 to i32
  %41 = add i32 %40, 1
  %42 = add i32 %21, %41
  %43 = trunc i64 %24 to i32
  %44 = add i32 %43, -1349
  %45 = add i32 %44, %23
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds float, float* %7, i64 %46
  %48 = bitcast float* %47 to i32*
  %49 = load i32, i32* %48, align 4, !tbaa !186
  %50 = sext i32 %42 to i64
  %51 = getelementptr inbounds float, float* %4, i64 %50
  %52 = bitcast float* %51 to i32*
  store i32 %49, i32* %52, align 4, !tbaa !189
  %53 = trunc i64 %24 to i32
  %54 = add i32 %53, 2
  %55 = add i32 %21, %54
  %56 = trunc i64 %24 to i32
  %57 = add i32 %56, -1348
  %58 = add i32 %57, %23
  %59 = sext i32 %58 to i64
  %60 = getelementptr inbounds float, float* %7, i64 %59
  %61 = bitcast float* %60 to i32*
  %62 = load i32, i32* %61, align 4, !tbaa !186
  %63 = sext i32 %55 to i64
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = bitcast float* %64 to i32*
  store i32 %62, i32* %65, align 4, !tbaa !189
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %66 = sext i32 %29 to i64
  %67 = getelementptr inbounds float, float* %4, i64 %66
  store float 0.000000e+00, float* %67, align 4, !tbaa !189
  %68 = trunc i64 %24 to i32
  %69 = add i32 %68, 1
  %70 = add i32 %69, %21
  %71 = sext i32 %70 to i64
  %72 = getelementptr inbounds float, float* %4, i64 %71
  store float 0.000000e+00, float* %72, align 4, !tbaa !189
  %73 = trunc i64 %24 to i32
  %74 = add i32 %73, 2
  %75 = add i32 %74, %21
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %4, i64 %76
  store float 0.000000e+00, float* %77, align 4, !tbaa !189
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %78 = mul nuw nsw i64 %indvars.iv, 3
  %79 = trunc i64 %78 to i32
  %80 = add i32 %21, %79
  %81 = sext i32 %80 to i64
  %82 = getelementptr inbounds float, float* %4, i64 %81
  store float 0.000000e+00, float* %82, align 4, !tbaa !189
  %83 = trunc i64 %78 to i32
  %84 = add i32 %83, 1
  %85 = add i32 %84, %21
  %86 = sext i32 %85 to i64
  %87 = getelementptr inbounds float, float* %4, i64 %86
  store float 0.000000e+00, float* %87, align 4, !tbaa !189
  %88 = trunc i64 %78 to i32
  %89 = add i32 %88, 2
  %90 = add i32 %89, %21
  %91 = sext i32 %90 to i64
  %92 = getelementptr inbounds float, float* %4, i64 %91
  store float 0.000000e+00, float* %92, align 4, !tbaa !189
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 227
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %93 = add nsw i32 %20, 1
  %94 = icmp slt i32 %93, %15
  br i1 %94, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.11(i32, %0* nocapture readonly, i8* nocapture readonly) {
entry:
  %3 = alloca [11 x <8 x float>], align 32
  %4 = bitcast [11 x <8 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0
  %5 = bitcast i8* %2 to float**
  %6 = load float*, float** %5, align 8
  %7 = getelementptr inbounds i8, i8* %2, i64 8
  %8 = bitcast i8* %7 to float**
  %9 = load float*, float** %8, align 8
  %10 = getelementptr inbounds i8, i8* %2, i64 16
  %11 = bitcast i8* %10 to float**
  %12 = load float*, float** %11, align 8
  %13 = getelementptr inbounds i8, i8* %2, i64 24
  %14 = bitcast i8* %13 to float**
  %15 = load float*, float** %14, align 8
  %16 = getelementptr inbounds i8, i8* %2, i64 32
  %17 = bitcast i8* %16 to i32*
  %18 = load i32, i32* %17, align 4
  %19 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %20 = load i32, i32* %19, align 4
  %21 = add nsw i32 %20, 439
  %22 = sdiv i32 %21, %20
  %23 = add nsw i32 %0, 1
  %24 = mul nsw i32 %22, %23
  %25 = icmp slt i32 %24, 440
  %26 = select i1 %25, i32 %24, i32 440
  %27 = mul nsw i32 %22, %0
  %28 = icmp slt i32 %27, 440
  %29 = select i1 %28, i32 %27, i32 440
  %30 = icmp slt i32 %29, %26
  br i1 %30, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %31 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %32 = bitcast float* %31 to <8 x float>*
  %33 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %34 = bitcast float* %33 to <8 x float>*
  %35 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %36 = bitcast float* %35 to <8 x float>*
  %37 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %38 = bitcast float* %37 to <8 x float>*
  %39 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %40 = bitcast float* %39 to <8 x float>*
  %41 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %42 = bitcast float* %41 to <8 x float>*
  %43 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %44 = bitcast float* %43 to <8 x float>*
  %45 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %46 = bitcast float* %45 to <8 x float>*
  %47 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %48 = bitcast float* %47 to <8 x float>*
  %49 = getelementptr inbounds [11 x <8 x float>], [11 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %50 = bitcast float* %49 to <8 x float>*
  %51 = add i32 %29, 1
  %52 = sext i32 %51 to i64
  %53 = add nsw i64 %52, -1
  %54 = sext i32 %26 to i64
  %55 = bitcast [11 x <8 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv107 = phi i64 [ %53, %for_body.lr.ph ], [ %indvars.iv.next108, %for_end3 ]
  %56 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %57 = tail call i8* %56(i32 1, i32 %18, i64 1760, i32 2, i32 32)
  %58 = trunc i64 %indvars.iv107 to i32
  %59 = srem i32 %58, 55
  %60 = mul nsw i32 %59, 2724
  %61 = sdiv i32 %58, 55
  %62 = mul nsw i32 %61, 2904
  %63 = sext i32 %62 to i64
  %64 = sext i32 %60 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end6 ]
  %65 = mul nuw nsw i64 %indvar, 352
  %scevgep = getelementptr i8, i8* %57, i64 %65
  %66 = mul nuw nsw i64 %indvar, 132
  %67 = add nsw i64 %66, %64
  call void @llvm.memset.p0i8.i64(i8* nonnull %55, i8 0, i64 352, i32 32, i1 false)
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %68 = mul nsw i64 %indvars.iv107, 440
  %69 = shl nsw i32 %61, 3
  %70 = sext i32 %69 to i64
  %71 = getelementptr inbounds float, float* %15, i64 %70
  %72 = bitcast float* %71 to <8 x float>*
  %73 = load <8 x float>, <8 x float>* %72, align 32, !tbaa !192
  %74 = bitcast i8* %57 to <8 x float>*
  %75 = load <8 x float>, <8 x float>* %74, align 32, !tbaa !195
  %76 = fadd <8 x float> %73, %75
  %77 = fcmp ogt <8 x float> %76, zeroinitializer
  %78 = select <8 x i1> %77, <8 x float> %76, <8 x float> zeroinitializer
  %79 = getelementptr inbounds float, float* %12, i64 %68
  %80 = bitcast float* %79 to <8 x float>*
  store <8 x float> %78, <8 x float>* %80, align 32, !tbaa !198
  %81 = getelementptr inbounds i8, i8* %57, i64 32
  %82 = bitcast i8* %81 to <8 x float>*
  %83 = load <8 x float>, <8 x float>* %82, align 32, !tbaa !195
  %84 = fadd <8 x float> %73, %83
  %85 = fcmp ogt <8 x float> %84, zeroinitializer
  %86 = select <8 x i1> %85, <8 x float> %84, <8 x float> zeroinitializer
  %87 = mul i64 %indvars.iv107, 1889785610240
  %sext = add i64 %87, 34359738368
  %88 = ashr exact i64 %sext, 32
  %89 = getelementptr inbounds float, float* %12, i64 %88
  %90 = bitcast float* %89 to <8 x float>*
  store <8 x float> %86, <8 x float>* %90, align 32, !tbaa !198
  %91 = getelementptr inbounds i8, i8* %57, i64 64
  %92 = bitcast i8* %91 to <8 x float>*
  %93 = load <8 x float>, <8 x float>* %92, align 32, !tbaa !195
  %94 = fadd <8 x float> %73, %93
  %95 = fcmp ogt <8 x float> %94, zeroinitializer
  %96 = select <8 x i1> %95, <8 x float> %94, <8 x float> zeroinitializer
  %97 = mul i64 %indvars.iv107, 1889785610240
  %sext109 = add i64 %97, 68719476736
  %98 = ashr exact i64 %sext109, 32
  %99 = getelementptr inbounds float, float* %12, i64 %98
  %100 = bitcast float* %99 to <8 x float>*
  store <8 x float> %96, <8 x float>* %100, align 32, !tbaa !198
  %101 = getelementptr inbounds i8, i8* %57, i64 96
  %102 = bitcast i8* %101 to <8 x float>*
  %103 = load <8 x float>, <8 x float>* %102, align 32, !tbaa !195
  %104 = fadd <8 x float> %73, %103
  %105 = fcmp ogt <8 x float> %104, zeroinitializer
  %106 = select <8 x i1> %105, <8 x float> %104, <8 x float> zeroinitializer
  %107 = mul i64 %indvars.iv107, 1889785610240
  %sext110 = add i64 %107, 103079215104
  %108 = ashr exact i64 %sext110, 32
  %109 = getelementptr inbounds float, float* %12, i64 %108
  %110 = bitcast float* %109 to <8 x float>*
  store <8 x float> %106, <8 x float>* %110, align 32, !tbaa !198
  %111 = getelementptr inbounds i8, i8* %57, i64 128
  %112 = bitcast i8* %111 to <8 x float>*
  %113 = load <8 x float>, <8 x float>* %112, align 32, !tbaa !195
  %114 = fadd <8 x float> %73, %113
  %115 = fcmp ogt <8 x float> %114, zeroinitializer
  %116 = select <8 x i1> %115, <8 x float> %114, <8 x float> zeroinitializer
  %117 = mul i64 %indvars.iv107, 1889785610240
  %sext111 = add i64 %117, 137438953472
  %118 = ashr exact i64 %sext111, 32
  %119 = getelementptr inbounds float, float* %12, i64 %118
  %120 = bitcast float* %119 to <8 x float>*
  store <8 x float> %116, <8 x float>* %120, align 32, !tbaa !198
  %121 = getelementptr inbounds i8, i8* %57, i64 160
  %122 = bitcast i8* %121 to <8 x float>*
  %123 = load <8 x float>, <8 x float>* %122, align 32, !tbaa !195
  %124 = fadd <8 x float> %73, %123
  %125 = fcmp ogt <8 x float> %124, zeroinitializer
  %126 = select <8 x i1> %125, <8 x float> %124, <8 x float> zeroinitializer
  %127 = mul i64 %indvars.iv107, 1889785610240
  %sext112 = add i64 %127, 171798691840
  %128 = ashr exact i64 %sext112, 32
  %129 = getelementptr inbounds float, float* %12, i64 %128
  %130 = bitcast float* %129 to <8 x float>*
  store <8 x float> %126, <8 x float>* %130, align 32, !tbaa !198
  %131 = getelementptr inbounds i8, i8* %57, i64 192
  %132 = bitcast i8* %131 to <8 x float>*
  %133 = load <8 x float>, <8 x float>* %132, align 32, !tbaa !195
  %134 = fadd <8 x float> %73, %133
  %135 = fcmp ogt <8 x float> %134, zeroinitializer
  %136 = select <8 x i1> %135, <8 x float> %134, <8 x float> zeroinitializer
  %137 = mul i64 %indvars.iv107, 1889785610240
  %sext113 = add i64 %137, 206158430208
  %138 = ashr exact i64 %sext113, 32
  %139 = getelementptr inbounds float, float* %12, i64 %138
  %140 = bitcast float* %139 to <8 x float>*
  store <8 x float> %136, <8 x float>* %140, align 32, !tbaa !198
  %141 = getelementptr inbounds i8, i8* %57, i64 224
  %142 = bitcast i8* %141 to <8 x float>*
  %143 = load <8 x float>, <8 x float>* %142, align 32, !tbaa !195
  %144 = fadd <8 x float> %73, %143
  %145 = fcmp ogt <8 x float> %144, zeroinitializer
  %146 = select <8 x i1> %145, <8 x float> %144, <8 x float> zeroinitializer
  %147 = mul i64 %indvars.iv107, 1889785610240
  %sext114 = add i64 %147, 240518168576
  %148 = ashr exact i64 %sext114, 32
  %149 = getelementptr inbounds float, float* %12, i64 %148
  %150 = bitcast float* %149 to <8 x float>*
  store <8 x float> %146, <8 x float>* %150, align 32, !tbaa !198
  %151 = getelementptr inbounds i8, i8* %57, i64 256
  %152 = bitcast i8* %151 to <8 x float>*
  %153 = load <8 x float>, <8 x float>* %152, align 32, !tbaa !195
  %154 = fadd <8 x float> %73, %153
  %155 = fcmp ogt <8 x float> %154, zeroinitializer
  %156 = select <8 x i1> %155, <8 x float> %154, <8 x float> zeroinitializer
  %157 = mul i64 %indvars.iv107, 1889785610240
  %sext115 = add i64 %157, 274877906944
  %158 = ashr exact i64 %sext115, 32
  %159 = getelementptr inbounds float, float* %12, i64 %158
  %160 = bitcast float* %159 to <8 x float>*
  store <8 x float> %156, <8 x float>* %160, align 32, !tbaa !198
  %161 = getelementptr inbounds i8, i8* %57, i64 288
  %162 = bitcast i8* %161 to <8 x float>*
  %163 = load <8 x float>, <8 x float>* %162, align 32, !tbaa !195
  %164 = fadd <8 x float> %73, %163
  %165 = fcmp ogt <8 x float> %164, zeroinitializer
  %166 = select <8 x i1> %165, <8 x float> %164, <8 x float> zeroinitializer
  %167 = mul i64 %indvars.iv107, 1889785610240
  %sext116 = add i64 %167, 309237645312
  %168 = ashr exact i64 %sext116, 32
  %169 = getelementptr inbounds float, float* %12, i64 %168
  %170 = bitcast float* %169 to <8 x float>*
  store <8 x float> %166, <8 x float>* %170, align 32, !tbaa !198
  %171 = getelementptr inbounds i8, i8* %57, i64 320
  %172 = bitcast i8* %171 to <8 x float>*
  %173 = load <8 x float>, <8 x float>* %172, align 32, !tbaa !195
  %174 = fadd <8 x float> %73, %173
  %175 = fcmp ogt <8 x float> %174, zeroinitializer
  %176 = select <8 x i1> %175, <8 x float> %174, <8 x float> zeroinitializer
  %177 = mul i64 %indvars.iv107, 1889785610240
  %sext117 = add i64 %177, 343597383680
  %178 = ashr exact i64 %sext117, 32
  %179 = getelementptr inbounds float, float* %12, i64 %178
  %180 = bitcast float* %179 to <8 x float>*
  store <8 x float> %176, <8 x float>* %180, align 32, !tbaa !198
  %181 = getelementptr inbounds i8, i8* %57, i64 352
  %182 = bitcast i8* %181 to <8 x float>*
  %183 = load <8 x float>, <8 x float>* %182, align 32, !tbaa !195
  %184 = fadd <8 x float> %73, %183
  %185 = fcmp ogt <8 x float> %184, zeroinitializer
  %186 = select <8 x i1> %185, <8 x float> %184, <8 x float> zeroinitializer
  %187 = mul i64 %indvars.iv107, 1889785610240
  %sext118 = add i64 %187, 377957122048
  %188 = ashr exact i64 %sext118, 32
  %189 = getelementptr inbounds float, float* %12, i64 %188
  %190 = bitcast float* %189 to <8 x float>*
  store <8 x float> %186, <8 x float>* %190, align 32, !tbaa !198
  %191 = getelementptr inbounds i8, i8* %57, i64 384
  %192 = bitcast i8* %191 to <8 x float>*
  %193 = load <8 x float>, <8 x float>* %192, align 32, !tbaa !195
  %194 = fadd <8 x float> %73, %193
  %195 = fcmp ogt <8 x float> %194, zeroinitializer
  %196 = select <8 x i1> %195, <8 x float> %194, <8 x float> zeroinitializer
  %197 = mul i64 %indvars.iv107, 1889785610240
  %sext119 = add i64 %197, 412316860416
  %198 = ashr exact i64 %sext119, 32
  %199 = getelementptr inbounds float, float* %12, i64 %198
  %200 = bitcast float* %199 to <8 x float>*
  store <8 x float> %196, <8 x float>* %200, align 32, !tbaa !198
  %201 = getelementptr inbounds i8, i8* %57, i64 416
  %202 = bitcast i8* %201 to <8 x float>*
  %203 = load <8 x float>, <8 x float>* %202, align 32, !tbaa !195
  %204 = fadd <8 x float> %73, %203
  %205 = fcmp ogt <8 x float> %204, zeroinitializer
  %206 = select <8 x i1> %205, <8 x float> %204, <8 x float> zeroinitializer
  %207 = mul i64 %indvars.iv107, 1889785610240
  %sext120 = add i64 %207, 446676598784
  %208 = ashr exact i64 %sext120, 32
  %209 = getelementptr inbounds float, float* %12, i64 %208
  %210 = bitcast float* %209 to <8 x float>*
  store <8 x float> %206, <8 x float>* %210, align 32, !tbaa !198
  %211 = getelementptr inbounds i8, i8* %57, i64 448
  %212 = bitcast i8* %211 to <8 x float>*
  %213 = load <8 x float>, <8 x float>* %212, align 32, !tbaa !195
  %214 = fadd <8 x float> %73, %213
  %215 = fcmp ogt <8 x float> %214, zeroinitializer
  %216 = select <8 x i1> %215, <8 x float> %214, <8 x float> zeroinitializer
  %217 = mul i64 %indvars.iv107, 1889785610240
  %sext121 = add i64 %217, 481036337152
  %218 = ashr exact i64 %sext121, 32
  %219 = getelementptr inbounds float, float* %12, i64 %218
  %220 = bitcast float* %219 to <8 x float>*
  store <8 x float> %216, <8 x float>* %220, align 32, !tbaa !198
  %221 = getelementptr inbounds i8, i8* %57, i64 480
  %222 = bitcast i8* %221 to <8 x float>*
  %223 = load <8 x float>, <8 x float>* %222, align 32, !tbaa !195
  %224 = fadd <8 x float> %73, %223
  %225 = fcmp ogt <8 x float> %224, zeroinitializer
  %226 = select <8 x i1> %225, <8 x float> %224, <8 x float> zeroinitializer
  %227 = mul i64 %indvars.iv107, 1889785610240
  %sext122 = add i64 %227, 515396075520
  %228 = ashr exact i64 %sext122, 32
  %229 = getelementptr inbounds float, float* %12, i64 %228
  %230 = bitcast float* %229 to <8 x float>*
  store <8 x float> %226, <8 x float>* %230, align 32, !tbaa !198
  %231 = getelementptr inbounds i8, i8* %57, i64 512
  %232 = bitcast i8* %231 to <8 x float>*
  %233 = load <8 x float>, <8 x float>* %232, align 32, !tbaa !195
  %234 = fadd <8 x float> %73, %233
  %235 = fcmp ogt <8 x float> %234, zeroinitializer
  %236 = select <8 x i1> %235, <8 x float> %234, <8 x float> zeroinitializer
  %237 = mul i64 %indvars.iv107, 1889785610240
  %sext123 = add i64 %237, 549755813888
  %238 = ashr exact i64 %sext123, 32
  %239 = getelementptr inbounds float, float* %12, i64 %238
  %240 = bitcast float* %239 to <8 x float>*
  store <8 x float> %236, <8 x float>* %240, align 32, !tbaa !198
  %241 = getelementptr inbounds i8, i8* %57, i64 544
  %242 = bitcast i8* %241 to <8 x float>*
  %243 = load <8 x float>, <8 x float>* %242, align 32, !tbaa !195
  %244 = fadd <8 x float> %73, %243
  %245 = fcmp ogt <8 x float> %244, zeroinitializer
  %246 = select <8 x i1> %245, <8 x float> %244, <8 x float> zeroinitializer
  %247 = mul i64 %indvars.iv107, 1889785610240
  %sext124 = add i64 %247, 584115552256
  %248 = ashr exact i64 %sext124, 32
  %249 = getelementptr inbounds float, float* %12, i64 %248
  %250 = bitcast float* %249 to <8 x float>*
  store <8 x float> %246, <8 x float>* %250, align 32, !tbaa !198
  %251 = getelementptr inbounds i8, i8* %57, i64 576
  %252 = bitcast i8* %251 to <8 x float>*
  %253 = load <8 x float>, <8 x float>* %252, align 32, !tbaa !195
  %254 = fadd <8 x float> %73, %253
  %255 = fcmp ogt <8 x float> %254, zeroinitializer
  %256 = select <8 x i1> %255, <8 x float> %254, <8 x float> zeroinitializer
  %257 = mul i64 %indvars.iv107, 1889785610240
  %sext125 = add i64 %257, 618475290624
  %258 = ashr exact i64 %sext125, 32
  %259 = getelementptr inbounds float, float* %12, i64 %258
  %260 = bitcast float* %259 to <8 x float>*
  store <8 x float> %256, <8 x float>* %260, align 32, !tbaa !198
  %261 = getelementptr inbounds i8, i8* %57, i64 608
  %262 = bitcast i8* %261 to <8 x float>*
  %263 = load <8 x float>, <8 x float>* %262, align 32, !tbaa !195
  %264 = fadd <8 x float> %73, %263
  %265 = fcmp ogt <8 x float> %264, zeroinitializer
  %266 = select <8 x i1> %265, <8 x float> %264, <8 x float> zeroinitializer
  %267 = mul i64 %indvars.iv107, 1889785610240
  %sext126 = add i64 %267, 652835028992
  %268 = ashr exact i64 %sext126, 32
  %269 = getelementptr inbounds float, float* %12, i64 %268
  %270 = bitcast float* %269 to <8 x float>*
  store <8 x float> %266, <8 x float>* %270, align 32, !tbaa !198
  %271 = getelementptr inbounds i8, i8* %57, i64 640
  %272 = bitcast i8* %271 to <8 x float>*
  %273 = load <8 x float>, <8 x float>* %272, align 32, !tbaa !195
  %274 = fadd <8 x float> %73, %273
  %275 = fcmp ogt <8 x float> %274, zeroinitializer
  %276 = select <8 x i1> %275, <8 x float> %274, <8 x float> zeroinitializer
  %277 = mul i64 %indvars.iv107, 1889785610240
  %sext127 = add i64 %277, 687194767360
  %278 = ashr exact i64 %sext127, 32
  %279 = getelementptr inbounds float, float* %12, i64 %278
  %280 = bitcast float* %279 to <8 x float>*
  store <8 x float> %276, <8 x float>* %280, align 32, !tbaa !198
  %281 = getelementptr inbounds i8, i8* %57, i64 672
  %282 = bitcast i8* %281 to <8 x float>*
  %283 = load <8 x float>, <8 x float>* %282, align 32, !tbaa !195
  %284 = fadd <8 x float> %73, %283
  %285 = fcmp ogt <8 x float> %284, zeroinitializer
  %286 = select <8 x i1> %285, <8 x float> %284, <8 x float> zeroinitializer
  %287 = mul i64 %indvars.iv107, 1889785610240
  %sext128 = add i64 %287, 721554505728
  %288 = ashr exact i64 %sext128, 32
  %289 = getelementptr inbounds float, float* %12, i64 %288
  %290 = bitcast float* %289 to <8 x float>*
  store <8 x float> %286, <8 x float>* %290, align 32, !tbaa !198
  %291 = getelementptr inbounds i8, i8* %57, i64 704
  %292 = bitcast i8* %291 to <8 x float>*
  %293 = load <8 x float>, <8 x float>* %292, align 32, !tbaa !195
  %294 = fadd <8 x float> %73, %293
  %295 = fcmp ogt <8 x float> %294, zeroinitializer
  %296 = select <8 x i1> %295, <8 x float> %294, <8 x float> zeroinitializer
  %297 = mul i64 %indvars.iv107, 1889785610240
  %sext129 = add i64 %297, 755914244096
  %298 = ashr exact i64 %sext129, 32
  %299 = getelementptr inbounds float, float* %12, i64 %298
  %300 = bitcast float* %299 to <8 x float>*
  store <8 x float> %296, <8 x float>* %300, align 32, !tbaa !198
  %301 = getelementptr inbounds i8, i8* %57, i64 736
  %302 = bitcast i8* %301 to <8 x float>*
  %303 = load <8 x float>, <8 x float>* %302, align 32, !tbaa !195
  %304 = fadd <8 x float> %73, %303
  %305 = fcmp ogt <8 x float> %304, zeroinitializer
  %306 = select <8 x i1> %305, <8 x float> %304, <8 x float> zeroinitializer
  %307 = mul i64 %indvars.iv107, 1889785610240
  %sext130 = add i64 %307, 790273982464
  %308 = ashr exact i64 %sext130, 32
  %309 = getelementptr inbounds float, float* %12, i64 %308
  %310 = bitcast float* %309 to <8 x float>*
  store <8 x float> %306, <8 x float>* %310, align 32, !tbaa !198
  %311 = getelementptr inbounds i8, i8* %57, i64 768
  %312 = bitcast i8* %311 to <8 x float>*
  %313 = load <8 x float>, <8 x float>* %312, align 32, !tbaa !195
  %314 = fadd <8 x float> %73, %313
  %315 = fcmp ogt <8 x float> %314, zeroinitializer
  %316 = select <8 x i1> %315, <8 x float> %314, <8 x float> zeroinitializer
  %317 = mul i64 %indvars.iv107, 1889785610240
  %sext131 = add i64 %317, 824633720832
  %318 = ashr exact i64 %sext131, 32
  %319 = getelementptr inbounds float, float* %12, i64 %318
  %320 = bitcast float* %319 to <8 x float>*
  store <8 x float> %316, <8 x float>* %320, align 32, !tbaa !198
  %321 = getelementptr inbounds i8, i8* %57, i64 800
  %322 = bitcast i8* %321 to <8 x float>*
  %323 = load <8 x float>, <8 x float>* %322, align 32, !tbaa !195
  %324 = fadd <8 x float> %73, %323
  %325 = fcmp ogt <8 x float> %324, zeroinitializer
  %326 = select <8 x i1> %325, <8 x float> %324, <8 x float> zeroinitializer
  %327 = mul i64 %indvars.iv107, 1889785610240
  %sext132 = add i64 %327, 858993459200
  %328 = ashr exact i64 %sext132, 32
  %329 = getelementptr inbounds float, float* %12, i64 %328
  %330 = bitcast float* %329 to <8 x float>*
  store <8 x float> %326, <8 x float>* %330, align 32, !tbaa !198
  %331 = getelementptr inbounds i8, i8* %57, i64 832
  %332 = bitcast i8* %331 to <8 x float>*
  %333 = load <8 x float>, <8 x float>* %332, align 32, !tbaa !195
  %334 = fadd <8 x float> %73, %333
  %335 = fcmp ogt <8 x float> %334, zeroinitializer
  %336 = select <8 x i1> %335, <8 x float> %334, <8 x float> zeroinitializer
  %337 = mul i64 %indvars.iv107, 1889785610240
  %sext133 = add i64 %337, 893353197568
  %338 = ashr exact i64 %sext133, 32
  %339 = getelementptr inbounds float, float* %12, i64 %338
  %340 = bitcast float* %339 to <8 x float>*
  store <8 x float> %336, <8 x float>* %340, align 32, !tbaa !198
  %341 = getelementptr inbounds i8, i8* %57, i64 864
  %342 = bitcast i8* %341 to <8 x float>*
  %343 = load <8 x float>, <8 x float>* %342, align 32, !tbaa !195
  %344 = fadd <8 x float> %73, %343
  %345 = fcmp ogt <8 x float> %344, zeroinitializer
  %346 = select <8 x i1> %345, <8 x float> %344, <8 x float> zeroinitializer
  %347 = mul i64 %indvars.iv107, 1889785610240
  %sext134 = add i64 %347, 927712935936
  %348 = ashr exact i64 %sext134, 32
  %349 = getelementptr inbounds float, float* %12, i64 %348
  %350 = bitcast float* %349 to <8 x float>*
  store <8 x float> %346, <8 x float>* %350, align 32, !tbaa !198
  %351 = getelementptr inbounds i8, i8* %57, i64 896
  %352 = bitcast i8* %351 to <8 x float>*
  %353 = load <8 x float>, <8 x float>* %352, align 32, !tbaa !195
  %354 = fadd <8 x float> %73, %353
  %355 = fcmp ogt <8 x float> %354, zeroinitializer
  %356 = select <8 x i1> %355, <8 x float> %354, <8 x float> zeroinitializer
  %357 = mul i64 %indvars.iv107, 1889785610240
  %sext135 = add i64 %357, 962072674304
  %358 = ashr exact i64 %sext135, 32
  %359 = getelementptr inbounds float, float* %12, i64 %358
  %360 = bitcast float* %359 to <8 x float>*
  store <8 x float> %356, <8 x float>* %360, align 32, !tbaa !198
  %361 = getelementptr inbounds i8, i8* %57, i64 928
  %362 = bitcast i8* %361 to <8 x float>*
  %363 = load <8 x float>, <8 x float>* %362, align 32, !tbaa !195
  %364 = fadd <8 x float> %73, %363
  %365 = fcmp ogt <8 x float> %364, zeroinitializer
  %366 = select <8 x i1> %365, <8 x float> %364, <8 x float> zeroinitializer
  %367 = mul i64 %indvars.iv107, 1889785610240
  %sext136 = add i64 %367, 996432412672
  %368 = ashr exact i64 %sext136, 32
  %369 = getelementptr inbounds float, float* %12, i64 %368
  %370 = bitcast float* %369 to <8 x float>*
  store <8 x float> %366, <8 x float>* %370, align 32, !tbaa !198
  %371 = getelementptr inbounds i8, i8* %57, i64 960
  %372 = bitcast i8* %371 to <8 x float>*
  %373 = load <8 x float>, <8 x float>* %372, align 32, !tbaa !195
  %374 = fadd <8 x float> %73, %373
  %375 = fcmp ogt <8 x float> %374, zeroinitializer
  %376 = select <8 x i1> %375, <8 x float> %374, <8 x float> zeroinitializer
  %377 = mul i64 %indvars.iv107, 1889785610240
  %sext137 = add i64 %377, 1030792151040
  %378 = ashr exact i64 %sext137, 32
  %379 = getelementptr inbounds float, float* %12, i64 %378
  %380 = bitcast float* %379 to <8 x float>*
  store <8 x float> %376, <8 x float>* %380, align 32, !tbaa !198
  %381 = getelementptr inbounds i8, i8* %57, i64 992
  %382 = bitcast i8* %381 to <8 x float>*
  %383 = load <8 x float>, <8 x float>* %382, align 32, !tbaa !195
  %384 = fadd <8 x float> %73, %383
  %385 = fcmp ogt <8 x float> %384, zeroinitializer
  %386 = select <8 x i1> %385, <8 x float> %384, <8 x float> zeroinitializer
  %387 = mul i64 %indvars.iv107, 1889785610240
  %sext138 = add i64 %387, 1065151889408
  %388 = ashr exact i64 %sext138, 32
  %389 = getelementptr inbounds float, float* %12, i64 %388
  %390 = bitcast float* %389 to <8 x float>*
  store <8 x float> %386, <8 x float>* %390, align 32, !tbaa !198
  %391 = getelementptr inbounds i8, i8* %57, i64 1024
  %392 = bitcast i8* %391 to <8 x float>*
  %393 = load <8 x float>, <8 x float>* %392, align 32, !tbaa !195
  %394 = fadd <8 x float> %73, %393
  %395 = fcmp ogt <8 x float> %394, zeroinitializer
  %396 = select <8 x i1> %395, <8 x float> %394, <8 x float> zeroinitializer
  %397 = mul i64 %indvars.iv107, 1889785610240
  %sext139 = add i64 %397, 1099511627776
  %398 = ashr exact i64 %sext139, 32
  %399 = getelementptr inbounds float, float* %12, i64 %398
  %400 = bitcast float* %399 to <8 x float>*
  store <8 x float> %396, <8 x float>* %400, align 32, !tbaa !198
  %401 = getelementptr inbounds i8, i8* %57, i64 1056
  %402 = bitcast i8* %401 to <8 x float>*
  %403 = load <8 x float>, <8 x float>* %402, align 32, !tbaa !195
  %404 = fadd <8 x float> %73, %403
  %405 = fcmp ogt <8 x float> %404, zeroinitializer
  %406 = select <8 x i1> %405, <8 x float> %404, <8 x float> zeroinitializer
  %407 = mul i64 %indvars.iv107, 1889785610240
  %sext140 = add i64 %407, 1133871366144
  %408 = ashr exact i64 %sext140, 32
  %409 = getelementptr inbounds float, float* %12, i64 %408
  %410 = bitcast float* %409 to <8 x float>*
  store <8 x float> %406, <8 x float>* %410, align 32, !tbaa !198
  %411 = getelementptr inbounds i8, i8* %57, i64 1088
  %412 = bitcast i8* %411 to <8 x float>*
  %413 = load <8 x float>, <8 x float>* %412, align 32, !tbaa !195
  %414 = fadd <8 x float> %73, %413
  %415 = fcmp ogt <8 x float> %414, zeroinitializer
  %416 = select <8 x i1> %415, <8 x float> %414, <8 x float> zeroinitializer
  %417 = mul i64 %indvars.iv107, 1889785610240
  %sext141 = add i64 %417, 1168231104512
  %418 = ashr exact i64 %sext141, 32
  %419 = getelementptr inbounds float, float* %12, i64 %418
  %420 = bitcast float* %419 to <8 x float>*
  store <8 x float> %416, <8 x float>* %420, align 32, !tbaa !198
  %421 = getelementptr inbounds i8, i8* %57, i64 1120
  %422 = bitcast i8* %421 to <8 x float>*
  %423 = load <8 x float>, <8 x float>* %422, align 32, !tbaa !195
  %424 = fadd <8 x float> %73, %423
  %425 = fcmp ogt <8 x float> %424, zeroinitializer
  %426 = select <8 x i1> %425, <8 x float> %424, <8 x float> zeroinitializer
  %427 = mul i64 %indvars.iv107, 1889785610240
  %sext142 = add i64 %427, 1202590842880
  %428 = ashr exact i64 %sext142, 32
  %429 = getelementptr inbounds float, float* %12, i64 %428
  %430 = bitcast float* %429 to <8 x float>*
  store <8 x float> %426, <8 x float>* %430, align 32, !tbaa !198
  %431 = getelementptr inbounds i8, i8* %57, i64 1152
  %432 = bitcast i8* %431 to <8 x float>*
  %433 = load <8 x float>, <8 x float>* %432, align 32, !tbaa !195
  %434 = fadd <8 x float> %73, %433
  %435 = fcmp ogt <8 x float> %434, zeroinitializer
  %436 = select <8 x i1> %435, <8 x float> %434, <8 x float> zeroinitializer
  %437 = mul i64 %indvars.iv107, 1889785610240
  %sext143 = add i64 %437, 1236950581248
  %438 = ashr exact i64 %sext143, 32
  %439 = getelementptr inbounds float, float* %12, i64 %438
  %440 = bitcast float* %439 to <8 x float>*
  store <8 x float> %436, <8 x float>* %440, align 32, !tbaa !198
  %441 = getelementptr inbounds i8, i8* %57, i64 1184
  %442 = bitcast i8* %441 to <8 x float>*
  %443 = load <8 x float>, <8 x float>* %442, align 32, !tbaa !195
  %444 = fadd <8 x float> %73, %443
  %445 = fcmp ogt <8 x float> %444, zeroinitializer
  %446 = select <8 x i1> %445, <8 x float> %444, <8 x float> zeroinitializer
  %447 = mul i64 %indvars.iv107, 1889785610240
  %sext144 = add i64 %447, 1271310319616
  %448 = ashr exact i64 %sext144, 32
  %449 = getelementptr inbounds float, float* %12, i64 %448
  %450 = bitcast float* %449 to <8 x float>*
  store <8 x float> %446, <8 x float>* %450, align 32, !tbaa !198
  %451 = getelementptr inbounds i8, i8* %57, i64 1216
  %452 = bitcast i8* %451 to <8 x float>*
  %453 = load <8 x float>, <8 x float>* %452, align 32, !tbaa !195
  %454 = fadd <8 x float> %73, %453
  %455 = fcmp ogt <8 x float> %454, zeroinitializer
  %456 = select <8 x i1> %455, <8 x float> %454, <8 x float> zeroinitializer
  %457 = mul i64 %indvars.iv107, 1889785610240
  %sext145 = add i64 %457, 1305670057984
  %458 = ashr exact i64 %sext145, 32
  %459 = getelementptr inbounds float, float* %12, i64 %458
  %460 = bitcast float* %459 to <8 x float>*
  store <8 x float> %456, <8 x float>* %460, align 32, !tbaa !198
  %461 = getelementptr inbounds i8, i8* %57, i64 1248
  %462 = bitcast i8* %461 to <8 x float>*
  %463 = load <8 x float>, <8 x float>* %462, align 32, !tbaa !195
  %464 = fadd <8 x float> %73, %463
  %465 = fcmp ogt <8 x float> %464, zeroinitializer
  %466 = select <8 x i1> %465, <8 x float> %464, <8 x float> zeroinitializer
  %467 = mul i64 %indvars.iv107, 1889785610240
  %sext146 = add i64 %467, 1340029796352
  %468 = ashr exact i64 %sext146, 32
  %469 = getelementptr inbounds float, float* %12, i64 %468
  %470 = bitcast float* %469 to <8 x float>*
  store <8 x float> %466, <8 x float>* %470, align 32, !tbaa !198
  %471 = getelementptr inbounds i8, i8* %57, i64 1280
  %472 = bitcast i8* %471 to <8 x float>*
  %473 = load <8 x float>, <8 x float>* %472, align 32, !tbaa !195
  %474 = fadd <8 x float> %73, %473
  %475 = fcmp ogt <8 x float> %474, zeroinitializer
  %476 = select <8 x i1> %475, <8 x float> %474, <8 x float> zeroinitializer
  %477 = mul i64 %indvars.iv107, 1889785610240
  %sext147 = add i64 %477, 1374389534720
  %478 = ashr exact i64 %sext147, 32
  %479 = getelementptr inbounds float, float* %12, i64 %478
  %480 = bitcast float* %479 to <8 x float>*
  store <8 x float> %476, <8 x float>* %480, align 32, !tbaa !198
  %481 = getelementptr inbounds i8, i8* %57, i64 1312
  %482 = bitcast i8* %481 to <8 x float>*
  %483 = load <8 x float>, <8 x float>* %482, align 32, !tbaa !195
  %484 = fadd <8 x float> %73, %483
  %485 = fcmp ogt <8 x float> %484, zeroinitializer
  %486 = select <8 x i1> %485, <8 x float> %484, <8 x float> zeroinitializer
  %487 = mul i64 %indvars.iv107, 1889785610240
  %sext148 = add i64 %487, 1408749273088
  %488 = ashr exact i64 %sext148, 32
  %489 = getelementptr inbounds float, float* %12, i64 %488
  %490 = bitcast float* %489 to <8 x float>*
  store <8 x float> %486, <8 x float>* %490, align 32, !tbaa !198
  %491 = getelementptr inbounds i8, i8* %57, i64 1344
  %492 = bitcast i8* %491 to <8 x float>*
  %493 = load <8 x float>, <8 x float>* %492, align 32, !tbaa !195
  %494 = fadd <8 x float> %73, %493
  %495 = fcmp ogt <8 x float> %494, zeroinitializer
  %496 = select <8 x i1> %495, <8 x float> %494, <8 x float> zeroinitializer
  %497 = mul i64 %indvars.iv107, 1889785610240
  %sext149 = add i64 %497, 1443109011456
  %498 = ashr exact i64 %sext149, 32
  %499 = getelementptr inbounds float, float* %12, i64 %498
  %500 = bitcast float* %499 to <8 x float>*
  store <8 x float> %496, <8 x float>* %500, align 32, !tbaa !198
  %501 = getelementptr inbounds i8, i8* %57, i64 1376
  %502 = bitcast i8* %501 to <8 x float>*
  %503 = load <8 x float>, <8 x float>* %502, align 32, !tbaa !195
  %504 = fadd <8 x float> %73, %503
  %505 = fcmp ogt <8 x float> %504, zeroinitializer
  %506 = select <8 x i1> %505, <8 x float> %504, <8 x float> zeroinitializer
  %507 = mul i64 %indvars.iv107, 1889785610240
  %sext150 = add i64 %507, 1477468749824
  %508 = ashr exact i64 %sext150, 32
  %509 = getelementptr inbounds float, float* %12, i64 %508
  %510 = bitcast float* %509 to <8 x float>*
  store <8 x float> %506, <8 x float>* %510, align 32, !tbaa !198
  %511 = getelementptr inbounds i8, i8* %57, i64 1408
  %512 = bitcast i8* %511 to <8 x float>*
  %513 = load <8 x float>, <8 x float>* %512, align 32, !tbaa !195
  %514 = fadd <8 x float> %73, %513
  %515 = fcmp ogt <8 x float> %514, zeroinitializer
  %516 = select <8 x i1> %515, <8 x float> %514, <8 x float> zeroinitializer
  %517 = mul i64 %indvars.iv107, 1889785610240
  %sext151 = add i64 %517, 1511828488192
  %518 = ashr exact i64 %sext151, 32
  %519 = getelementptr inbounds float, float* %12, i64 %518
  %520 = bitcast float* %519 to <8 x float>*
  store <8 x float> %516, <8 x float>* %520, align 32, !tbaa !198
  %521 = getelementptr inbounds i8, i8* %57, i64 1440
  %522 = bitcast i8* %521 to <8 x float>*
  %523 = load <8 x float>, <8 x float>* %522, align 32, !tbaa !195
  %524 = fadd <8 x float> %73, %523
  %525 = fcmp ogt <8 x float> %524, zeroinitializer
  %526 = select <8 x i1> %525, <8 x float> %524, <8 x float> zeroinitializer
  %527 = mul i64 %indvars.iv107, 1889785610240
  %sext152 = add i64 %527, 1546188226560
  %528 = ashr exact i64 %sext152, 32
  %529 = getelementptr inbounds float, float* %12, i64 %528
  %530 = bitcast float* %529 to <8 x float>*
  store <8 x float> %526, <8 x float>* %530, align 32, !tbaa !198
  %531 = getelementptr inbounds i8, i8* %57, i64 1472
  %532 = bitcast i8* %531 to <8 x float>*
  %533 = load <8 x float>, <8 x float>* %532, align 32, !tbaa !195
  %534 = fadd <8 x float> %73, %533
  %535 = fcmp ogt <8 x float> %534, zeroinitializer
  %536 = select <8 x i1> %535, <8 x float> %534, <8 x float> zeroinitializer
  %537 = mul i64 %indvars.iv107, 1889785610240
  %sext153 = add i64 %537, 1580547964928
  %538 = ashr exact i64 %sext153, 32
  %539 = getelementptr inbounds float, float* %12, i64 %538
  %540 = bitcast float* %539 to <8 x float>*
  store <8 x float> %536, <8 x float>* %540, align 32, !tbaa !198
  %541 = getelementptr inbounds i8, i8* %57, i64 1504
  %542 = bitcast i8* %541 to <8 x float>*
  %543 = load <8 x float>, <8 x float>* %542, align 32, !tbaa !195
  %544 = fadd <8 x float> %73, %543
  %545 = fcmp ogt <8 x float> %544, zeroinitializer
  %546 = select <8 x i1> %545, <8 x float> %544, <8 x float> zeroinitializer
  %547 = mul i64 %indvars.iv107, 1889785610240
  %sext154 = add i64 %547, 1614907703296
  %548 = ashr exact i64 %sext154, 32
  %549 = getelementptr inbounds float, float* %12, i64 %548
  %550 = bitcast float* %549 to <8 x float>*
  store <8 x float> %546, <8 x float>* %550, align 32, !tbaa !198
  %551 = getelementptr inbounds i8, i8* %57, i64 1536
  %552 = bitcast i8* %551 to <8 x float>*
  %553 = load <8 x float>, <8 x float>* %552, align 32, !tbaa !195
  %554 = fadd <8 x float> %73, %553
  %555 = fcmp ogt <8 x float> %554, zeroinitializer
  %556 = select <8 x i1> %555, <8 x float> %554, <8 x float> zeroinitializer
  %557 = mul i64 %indvars.iv107, 1889785610240
  %sext155 = add i64 %557, 1649267441664
  %558 = ashr exact i64 %sext155, 32
  %559 = getelementptr inbounds float, float* %12, i64 %558
  %560 = bitcast float* %559 to <8 x float>*
  store <8 x float> %556, <8 x float>* %560, align 32, !tbaa !198
  %561 = getelementptr inbounds i8, i8* %57, i64 1568
  %562 = bitcast i8* %561 to <8 x float>*
  %563 = load <8 x float>, <8 x float>* %562, align 32, !tbaa !195
  %564 = fadd <8 x float> %73, %563
  %565 = fcmp ogt <8 x float> %564, zeroinitializer
  %566 = select <8 x i1> %565, <8 x float> %564, <8 x float> zeroinitializer
  %567 = mul i64 %indvars.iv107, 1889785610240
  %sext156 = add i64 %567, 1683627180032
  %568 = ashr exact i64 %sext156, 32
  %569 = getelementptr inbounds float, float* %12, i64 %568
  %570 = bitcast float* %569 to <8 x float>*
  store <8 x float> %566, <8 x float>* %570, align 32, !tbaa !198
  %571 = getelementptr inbounds i8, i8* %57, i64 1600
  %572 = bitcast i8* %571 to <8 x float>*
  %573 = load <8 x float>, <8 x float>* %572, align 32, !tbaa !195
  %574 = fadd <8 x float> %73, %573
  %575 = fcmp ogt <8 x float> %574, zeroinitializer
  %576 = select <8 x i1> %575, <8 x float> %574, <8 x float> zeroinitializer
  %577 = mul i64 %indvars.iv107, 1889785610240
  %sext157 = add i64 %577, 1717986918400
  %578 = ashr exact i64 %sext157, 32
  %579 = getelementptr inbounds float, float* %12, i64 %578
  %580 = bitcast float* %579 to <8 x float>*
  store <8 x float> %576, <8 x float>* %580, align 32, !tbaa !198
  %581 = getelementptr inbounds i8, i8* %57, i64 1632
  %582 = bitcast i8* %581 to <8 x float>*
  %583 = load <8 x float>, <8 x float>* %582, align 32, !tbaa !195
  %584 = fadd <8 x float> %73, %583
  %585 = fcmp ogt <8 x float> %584, zeroinitializer
  %586 = select <8 x i1> %585, <8 x float> %584, <8 x float> zeroinitializer
  %587 = mul i64 %indvars.iv107, 1889785610240
  %sext158 = add i64 %587, 1752346656768
  %588 = ashr exact i64 %sext158, 32
  %589 = getelementptr inbounds float, float* %12, i64 %588
  %590 = bitcast float* %589 to <8 x float>*
  store <8 x float> %586, <8 x float>* %590, align 32, !tbaa !198
  %591 = getelementptr inbounds i8, i8* %57, i64 1664
  %592 = bitcast i8* %591 to <8 x float>*
  %593 = load <8 x float>, <8 x float>* %592, align 32, !tbaa !195
  %594 = fadd <8 x float> %73, %593
  %595 = fcmp ogt <8 x float> %594, zeroinitializer
  %596 = select <8 x i1> %595, <8 x float> %594, <8 x float> zeroinitializer
  %597 = mul i64 %indvars.iv107, 1889785610240
  %sext159 = add i64 %597, 1786706395136
  %598 = ashr exact i64 %sext159, 32
  %599 = getelementptr inbounds float, float* %12, i64 %598
  %600 = bitcast float* %599 to <8 x float>*
  store <8 x float> %596, <8 x float>* %600, align 32, !tbaa !198
  %601 = getelementptr inbounds i8, i8* %57, i64 1696
  %602 = bitcast i8* %601 to <8 x float>*
  %603 = load <8 x float>, <8 x float>* %602, align 32, !tbaa !195
  %604 = fadd <8 x float> %73, %603
  %605 = fcmp ogt <8 x float> %604, zeroinitializer
  %606 = select <8 x i1> %605, <8 x float> %604, <8 x float> zeroinitializer
  %607 = mul i64 %indvars.iv107, 1889785610240
  %sext160 = add i64 %607, 1821066133504
  %608 = ashr exact i64 %sext160, 32
  %609 = getelementptr inbounds float, float* %12, i64 %608
  %610 = bitcast float* %609 to <8 x float>*
  store <8 x float> %606, <8 x float>* %610, align 32, !tbaa !198
  %611 = getelementptr inbounds i8, i8* %57, i64 1728
  %612 = bitcast i8* %611 to <8 x float>*
  %613 = load <8 x float>, <8 x float>* %612, align 32, !tbaa !195
  %614 = fadd <8 x float> %73, %613
  %615 = fcmp ogt <8 x float> %614, zeroinitializer
  %616 = select <8 x i1> %615, <8 x float> %614, <8 x float> zeroinitializer
  %617 = mul i64 %indvars.iv107, 1889785610240
  %sext161 = add i64 %617, 1855425871872
  %618 = ashr exact i64 %sext161, 32
  %619 = getelementptr inbounds float, float* %12, i64 %618
  %620 = bitcast float* %619 to <8 x float>*
  store <8 x float> %616, <8 x float>* %620, align 32, !tbaa !198
  %621 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %622 = tail call i32 %621(i32 1, i32 %18, i8* nonnull %57)
  %indvars.iv.next108 = add nsw i64 %indvars.iv107, 1
  %623 = icmp slt i64 %indvars.iv.next108, %54
  br i1 %623, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_end9, %for_body2
  %indvars.iv94 = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next95, %for_end9 ]
  %.lcssa42.lcssa83 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %839, %for_end9 ]
  %.lcssa40.lcssa81 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %833, %for_end9 ]
  %.lcssa38.lcssa79 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %827, %for_end9 ]
  %.lcssa36.lcssa77 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %821, %for_end9 ]
  %.lcssa34.lcssa75 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %815, %for_end9 ]
  %.lcssa32.lcssa73 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %809, %for_end9 ]
  %.lcssa30.lcssa71 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %803, %for_end9 ]
  %.lcssa28.lcssa69 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %797, %for_end9 ]
  %.lcssa26.lcssa68 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %791, %for_end9 ]
  %.lcssa24.lcssa66 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %785, %for_end9 ]
  %.lcssa.lcssa64 = phi <8 x float> [ zeroinitializer, %for_body2 ], [ %779, %for_end9 ]
  %624 = mul nuw nsw i64 %indvars.iv94, 681
  %625 = add nsw i64 %67, %624
  %626 = mul nuw nsw i64 %indvars.iv94, 264
  %627 = add nsw i64 %626, %63
  br label %for_body8

for_end6:                                         ; preds = %for_end9
  store <8 x float> %779, <8 x float>* %.sub, align 32, !tbaa !201
  store <8 x float> %785, <8 x float>* %32, align 32, !tbaa !201
  store <8 x float> %791, <8 x float>* %34, align 32, !tbaa !201
  store <8 x float> %797, <8 x float>* %36, align 32, !tbaa !201
  store <8 x float> %803, <8 x float>* %38, align 32, !tbaa !201
  store <8 x float> %809, <8 x float>* %40, align 32, !tbaa !201
  store <8 x float> %815, <8 x float>* %42, align 32, !tbaa !201
  store <8 x float> %821, <8 x float>* %44, align 32, !tbaa !201
  store <8 x float> %827, <8 x float>* %46, align 32, !tbaa !201
  store <8 x float> %833, <8 x float>* %48, align 32, !tbaa !201
  store <8 x float> %839, <8 x float>* %50, align 32, !tbaa !201
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep, i8* nonnull %4, i64 352, i32 32, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond100 = icmp eq i64 %indvar.next, 5
  br i1 %exitcond100, label %for_end3, label %for_body2, !prof !28

for_body8:                                        ; preds = %for_body8, %for_body5
  %indvars.iv = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next, %for_body8 ]
  %.lcssa4263 = phi <8 x float> [ %.lcssa42.lcssa83, %for_body5 ], [ %839, %for_body8 ]
  %.lcssa4061 = phi <8 x float> [ %.lcssa40.lcssa81, %for_body5 ], [ %833, %for_body8 ]
  %.lcssa3859 = phi <8 x float> [ %.lcssa38.lcssa79, %for_body5 ], [ %827, %for_body8 ]
  %.lcssa3657 = phi <8 x float> [ %.lcssa36.lcssa77, %for_body5 ], [ %821, %for_body8 ]
  %.lcssa3455 = phi <8 x float> [ %.lcssa34.lcssa75, %for_body5 ], [ %815, %for_body8 ]
  %.lcssa3253 = phi <8 x float> [ %.lcssa32.lcssa73, %for_body5 ], [ %809, %for_body8 ]
  %.lcssa3051 = phi <8 x float> [ %.lcssa30.lcssa71, %for_body5 ], [ %803, %for_body8 ]
  %.lcssa2849 = phi <8 x float> [ %.lcssa28.lcssa69, %for_body5 ], [ %797, %for_body8 ]
  %.lcssa2647 = phi <8 x float> [ %.lcssa26.lcssa68, %for_body5 ], [ %791, %for_body8 ]
  %.lcssa2446 = phi <8 x float> [ %.lcssa24.lcssa66, %for_body5 ], [ %785, %for_body8 ]
  %.lcssa44 = phi <8 x float> [ %.lcssa.lcssa64, %for_body5 ], [ %779, %for_body8 ]
  %628 = mul nuw nsw i64 %indvars.iv, 3
  %629 = add nsw i64 %625, %628
  %630 = mul nuw nsw i64 %indvars.iv, 24
  %631 = add nsw i64 %627, %630
  %632 = getelementptr inbounds float, float* %6, i64 %629
  %633 = load float, float* %632, align 4, !tbaa !189
  %634 = insertelement <8 x float> undef, float %633, i32 0
  %635 = shufflevector <8 x float> %634, <8 x float> undef, <8 x i32> zeroinitializer
  %636 = getelementptr inbounds float, float* %9, i64 %631
  %637 = bitcast float* %636 to <8 x float>*
  %638 = load <8 x float>, <8 x float>* %637, align 32, !tbaa !212
  %639 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %635, <8 x float> %638, <8 x float> %.lcssa44)
  %640 = add nsw i64 %629, 12
  %641 = getelementptr inbounds float, float* %6, i64 %640
  %642 = load float, float* %641, align 4, !tbaa !189
  %643 = insertelement <8 x float> undef, float %642, i32 0
  %644 = shufflevector <8 x float> %643, <8 x float> undef, <8 x i32> zeroinitializer
  %645 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %644, <8 x float> %638, <8 x float> %.lcssa2446)
  %646 = add nsw i64 %629, 24
  %647 = getelementptr inbounds float, float* %6, i64 %646
  %648 = load float, float* %647, align 4, !tbaa !189
  %649 = insertelement <8 x float> undef, float %648, i32 0
  %650 = shufflevector <8 x float> %649, <8 x float> undef, <8 x i32> zeroinitializer
  %651 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %650, <8 x float> %638, <8 x float> %.lcssa2647)
  %652 = add nsw i64 %629, 36
  %653 = getelementptr inbounds float, float* %6, i64 %652
  %654 = load float, float* %653, align 4, !tbaa !189
  %655 = insertelement <8 x float> undef, float %654, i32 0
  %656 = shufflevector <8 x float> %655, <8 x float> undef, <8 x i32> zeroinitializer
  %657 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %656, <8 x float> %638, <8 x float> %.lcssa2849)
  %658 = add nsw i64 %629, 48
  %659 = getelementptr inbounds float, float* %6, i64 %658
  %660 = load float, float* %659, align 4, !tbaa !189
  %661 = insertelement <8 x float> undef, float %660, i32 0
  %662 = shufflevector <8 x float> %661, <8 x float> undef, <8 x i32> zeroinitializer
  %663 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %662, <8 x float> %638, <8 x float> %.lcssa3051)
  %664 = add nsw i64 %629, 60
  %665 = getelementptr inbounds float, float* %6, i64 %664
  %666 = load float, float* %665, align 4, !tbaa !189
  %667 = insertelement <8 x float> undef, float %666, i32 0
  %668 = shufflevector <8 x float> %667, <8 x float> undef, <8 x i32> zeroinitializer
  %669 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %668, <8 x float> %638, <8 x float> %.lcssa3253)
  %670 = add nsw i64 %629, 72
  %671 = getelementptr inbounds float, float* %6, i64 %670
  %672 = load float, float* %671, align 4, !tbaa !189
  %673 = insertelement <8 x float> undef, float %672, i32 0
  %674 = shufflevector <8 x float> %673, <8 x float> undef, <8 x i32> zeroinitializer
  %675 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %674, <8 x float> %638, <8 x float> %.lcssa3455)
  %676 = add nsw i64 %629, 84
  %677 = getelementptr inbounds float, float* %6, i64 %676
  %678 = load float, float* %677, align 4, !tbaa !189
  %679 = insertelement <8 x float> undef, float %678, i32 0
  %680 = shufflevector <8 x float> %679, <8 x float> undef, <8 x i32> zeroinitializer
  %681 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %680, <8 x float> %638, <8 x float> %.lcssa3657)
  %682 = add nsw i64 %629, 96
  %683 = getelementptr inbounds float, float* %6, i64 %682
  %684 = load float, float* %683, align 4, !tbaa !189
  %685 = insertelement <8 x float> undef, float %684, i32 0
  %686 = shufflevector <8 x float> %685, <8 x float> undef, <8 x i32> zeroinitializer
  %687 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %686, <8 x float> %638, <8 x float> %.lcssa3859)
  %688 = add nsw i64 %629, 108
  %689 = getelementptr inbounds float, float* %6, i64 %688
  %690 = load float, float* %689, align 4, !tbaa !189
  %691 = insertelement <8 x float> undef, float %690, i32 0
  %692 = shufflevector <8 x float> %691, <8 x float> undef, <8 x i32> zeroinitializer
  %693 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %692, <8 x float> %638, <8 x float> %.lcssa4061)
  %694 = add nsw i64 %629, 120
  %695 = getelementptr inbounds float, float* %6, i64 %694
  %696 = load float, float* %695, align 4, !tbaa !189
  %697 = insertelement <8 x float> undef, float %696, i32 0
  %698 = shufflevector <8 x float> %697, <8 x float> undef, <8 x i32> zeroinitializer
  %699 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %698, <8 x float> %638, <8 x float> %.lcssa4263)
  %700 = add nsw i64 %629, 1
  %701 = getelementptr inbounds float, float* %6, i64 %700
  %702 = load float, float* %701, align 4, !tbaa !189
  %703 = insertelement <8 x float> undef, float %702, i32 0
  %704 = shufflevector <8 x float> %703, <8 x float> undef, <8 x i32> zeroinitializer
  %705 = add nsw i64 %631, 8
  %706 = getelementptr inbounds float, float* %9, i64 %705
  %707 = bitcast float* %706 to <8 x float>*
  %708 = load <8 x float>, <8 x float>* %707, align 32, !tbaa !212
  %709 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %704, <8 x float> %708, <8 x float> %639)
  %710 = add nsw i64 %629, 13
  %711 = getelementptr inbounds float, float* %6, i64 %710
  %712 = load float, float* %711, align 4, !tbaa !189
  %713 = insertelement <8 x float> undef, float %712, i32 0
  %714 = shufflevector <8 x float> %713, <8 x float> undef, <8 x i32> zeroinitializer
  %715 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %714, <8 x float> %708, <8 x float> %645)
  %716 = add nsw i64 %629, 25
  %717 = getelementptr inbounds float, float* %6, i64 %716
  %718 = load float, float* %717, align 4, !tbaa !189
  %719 = insertelement <8 x float> undef, float %718, i32 0
  %720 = shufflevector <8 x float> %719, <8 x float> undef, <8 x i32> zeroinitializer
  %721 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %720, <8 x float> %708, <8 x float> %651)
  %722 = add nsw i64 %629, 37
  %723 = getelementptr inbounds float, float* %6, i64 %722
  %724 = load float, float* %723, align 4, !tbaa !189
  %725 = insertelement <8 x float> undef, float %724, i32 0
  %726 = shufflevector <8 x float> %725, <8 x float> undef, <8 x i32> zeroinitializer
  %727 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %726, <8 x float> %708, <8 x float> %657)
  %728 = add nsw i64 %629, 49
  %729 = getelementptr inbounds float, float* %6, i64 %728
  %730 = load float, float* %729, align 4, !tbaa !189
  %731 = insertelement <8 x float> undef, float %730, i32 0
  %732 = shufflevector <8 x float> %731, <8 x float> undef, <8 x i32> zeroinitializer
  %733 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %732, <8 x float> %708, <8 x float> %663)
  %734 = add nsw i64 %629, 61
  %735 = getelementptr inbounds float, float* %6, i64 %734
  %736 = load float, float* %735, align 4, !tbaa !189
  %737 = insertelement <8 x float> undef, float %736, i32 0
  %738 = shufflevector <8 x float> %737, <8 x float> undef, <8 x i32> zeroinitializer
  %739 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %738, <8 x float> %708, <8 x float> %669)
  %740 = add nsw i64 %629, 73
  %741 = getelementptr inbounds float, float* %6, i64 %740
  %742 = load float, float* %741, align 4, !tbaa !189
  %743 = insertelement <8 x float> undef, float %742, i32 0
  %744 = shufflevector <8 x float> %743, <8 x float> undef, <8 x i32> zeroinitializer
  %745 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %744, <8 x float> %708, <8 x float> %675)
  %746 = add nsw i64 %629, 85
  %747 = getelementptr inbounds float, float* %6, i64 %746
  %748 = load float, float* %747, align 4, !tbaa !189
  %749 = insertelement <8 x float> undef, float %748, i32 0
  %750 = shufflevector <8 x float> %749, <8 x float> undef, <8 x i32> zeroinitializer
  %751 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %750, <8 x float> %708, <8 x float> %681)
  %752 = add nsw i64 %629, 97
  %753 = getelementptr inbounds float, float* %6, i64 %752
  %754 = load float, float* %753, align 4, !tbaa !189
  %755 = insertelement <8 x float> undef, float %754, i32 0
  %756 = shufflevector <8 x float> %755, <8 x float> undef, <8 x i32> zeroinitializer
  %757 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %756, <8 x float> %708, <8 x float> %687)
  %758 = add nsw i64 %629, 109
  %759 = getelementptr inbounds float, float* %6, i64 %758
  %760 = load float, float* %759, align 4, !tbaa !189
  %761 = insertelement <8 x float> undef, float %760, i32 0
  %762 = shufflevector <8 x float> %761, <8 x float> undef, <8 x i32> zeroinitializer
  %763 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %762, <8 x float> %708, <8 x float> %693)
  %764 = add nsw i64 %629, 121
  %765 = getelementptr inbounds float, float* %6, i64 %764
  %766 = load float, float* %765, align 4, !tbaa !189
  %767 = insertelement <8 x float> undef, float %766, i32 0
  %768 = shufflevector <8 x float> %767, <8 x float> undef, <8 x i32> zeroinitializer
  %769 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %768, <8 x float> %708, <8 x float> %699)
  %770 = add nsw i64 %629, 2
  %771 = getelementptr inbounds float, float* %6, i64 %770
  %772 = load float, float* %771, align 4, !tbaa !189
  %773 = insertelement <8 x float> undef, float %772, i32 0
  %774 = shufflevector <8 x float> %773, <8 x float> undef, <8 x i32> zeroinitializer
  %775 = add nsw i64 %631, 16
  %776 = getelementptr inbounds float, float* %9, i64 %775
  %777 = bitcast float* %776 to <8 x float>*
  %778 = load <8 x float>, <8 x float>* %777, align 32, !tbaa !212
  %779 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %774, <8 x float> %778, <8 x float> %709)
  %780 = add nsw i64 %629, 14
  %781 = getelementptr inbounds float, float* %6, i64 %780
  %782 = load float, float* %781, align 4, !tbaa !189
  %783 = insertelement <8 x float> undef, float %782, i32 0
  %784 = shufflevector <8 x float> %783, <8 x float> undef, <8 x i32> zeroinitializer
  %785 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %784, <8 x float> %778, <8 x float> %715)
  %786 = add nsw i64 %629, 26
  %787 = getelementptr inbounds float, float* %6, i64 %786
  %788 = load float, float* %787, align 4, !tbaa !189
  %789 = insertelement <8 x float> undef, float %788, i32 0
  %790 = shufflevector <8 x float> %789, <8 x float> undef, <8 x i32> zeroinitializer
  %791 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %790, <8 x float> %778, <8 x float> %721)
  %792 = add nsw i64 %629, 38
  %793 = getelementptr inbounds float, float* %6, i64 %792
  %794 = load float, float* %793, align 4, !tbaa !189
  %795 = insertelement <8 x float> undef, float %794, i32 0
  %796 = shufflevector <8 x float> %795, <8 x float> undef, <8 x i32> zeroinitializer
  %797 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %796, <8 x float> %778, <8 x float> %727)
  %798 = add nsw i64 %629, 50
  %799 = getelementptr inbounds float, float* %6, i64 %798
  %800 = load float, float* %799, align 4, !tbaa !189
  %801 = insertelement <8 x float> undef, float %800, i32 0
  %802 = shufflevector <8 x float> %801, <8 x float> undef, <8 x i32> zeroinitializer
  %803 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %802, <8 x float> %778, <8 x float> %733)
  %804 = add nsw i64 %629, 62
  %805 = getelementptr inbounds float, float* %6, i64 %804
  %806 = load float, float* %805, align 4, !tbaa !189
  %807 = insertelement <8 x float> undef, float %806, i32 0
  %808 = shufflevector <8 x float> %807, <8 x float> undef, <8 x i32> zeroinitializer
  %809 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %808, <8 x float> %778, <8 x float> %739)
  %810 = add nsw i64 %629, 74
  %811 = getelementptr inbounds float, float* %6, i64 %810
  %812 = load float, float* %811, align 4, !tbaa !189
  %813 = insertelement <8 x float> undef, float %812, i32 0
  %814 = shufflevector <8 x float> %813, <8 x float> undef, <8 x i32> zeroinitializer
  %815 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %814, <8 x float> %778, <8 x float> %745)
  %816 = add nsw i64 %629, 86
  %817 = getelementptr inbounds float, float* %6, i64 %816
  %818 = load float, float* %817, align 4, !tbaa !189
  %819 = insertelement <8 x float> undef, float %818, i32 0
  %820 = shufflevector <8 x float> %819, <8 x float> undef, <8 x i32> zeroinitializer
  %821 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %820, <8 x float> %778, <8 x float> %751)
  %822 = add nsw i64 %629, 98
  %823 = getelementptr inbounds float, float* %6, i64 %822
  %824 = load float, float* %823, align 4, !tbaa !189
  %825 = insertelement <8 x float> undef, float %824, i32 0
  %826 = shufflevector <8 x float> %825, <8 x float> undef, <8 x i32> zeroinitializer
  %827 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %826, <8 x float> %778, <8 x float> %757)
  %828 = add nsw i64 %629, 110
  %829 = getelementptr inbounds float, float* %6, i64 %828
  %830 = load float, float* %829, align 4, !tbaa !189
  %831 = insertelement <8 x float> undef, float %830, i32 0
  %832 = shufflevector <8 x float> %831, <8 x float> undef, <8 x i32> zeroinitializer
  %833 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %832, <8 x float> %778, <8 x float> %763)
  %834 = add nsw i64 %629, 122
  %835 = getelementptr inbounds float, float* %6, i64 %834
  %836 = load float, float* %835, align 4, !tbaa !189
  %837 = insertelement <8 x float> undef, float %836, i32 0
  %838 = shufflevector <8 x float> %837, <8 x float> undef, <8 x i32> zeroinitializer
  %839 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %838, <8 x float> %778, <8 x float> %769)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 11
  br i1 %exitcond, label %for_end9, label %for_body8, !prof !28

for_end9:                                         ; preds = %for_body8
  %indvars.iv.next95 = add nuw nsw i64 %indvars.iv94, 1
  %exitcond96 = icmp eq i64 %indvars.iv.next95, 11
  br i1 %exitcond96, label %for_end6, label %for_body5, !prof !28
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !215 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !217, metadata !DIExpression()), !dbg !220
  call void @llvm.dbg.value(metadata i8* %1, metadata !218, metadata !DIExpression()), !dbg !220
  call void @llvm.dbg.value(metadata i32 %2, metadata !219, metadata !DIExpression()), !dbg !220
  %3 = bitcast i8* %0 to %1**, !dbg !220
  %4 = load %1*, %1** %3, align 8, !dbg !220
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !220
  %6 = bitcast i8* %5 to %1**, !dbg !220
  %7 = load %1*, %1** %6, align 8, !dbg !220
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !220
  %9 = bitcast i8* %8 to %1**, !dbg !220
  %10 = load %1*, %1** %9, align 8, !dbg !220
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !220
  %12 = bitcast i8* %11 to %1**, !dbg !220
  %13 = load %1*, %1** %12, align 8, !dbg !220
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !220
  %15 = load i8*, i8** %14, align 8, !dbg !220
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !220
  %17 = load i32, i32* %16, align 4, !dbg !220
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !220
  %19 = load i8*, i8** %18, align 8, !dbg !220
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !220
  %21 = load i8*, i8** %20, align 8, !dbg !220
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !220
  %23 = load i8*, i8** %22, align 8, !dbg !220
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !220
  ret i32 %24, !dbg !220
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 246016, i32 2, i32 32)
  %7 = alloca %16, align 8
  %8 = getelementptr inbounds %16, %16* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %16, %16* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %16* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.12, i8* nonnull %10, i32 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %21, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %17, align 8
  %15 = getelementptr inbounds %17, %17* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %17, %17* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %17, %17* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %17, %17* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = bitcast %17* %14 to i8*
  %20 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %21 = call i32 %20(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.13, i8* nonnull %19, i32 5)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %23 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %24 = call i32 %23(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.12(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 247
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 248
  %15 = select i1 %14, i32 %13, i32 248
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 248
  %18 = select i1 %17, i32 %16, i32 248
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %219, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 248
  %22 = srem i32 %20, 31
  %.off = add nsw i32 %22, -2
  %23 = icmp ult i32 %.off, 27
  %24 = sdiv i32 %20, 31
  %25 = mul nsw i32 %24, 5832
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 216
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 3
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -2
  %30 = icmp ult i32 %29, 27
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 31
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !28

for_body2.split.us.us:                            ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -448
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to i32*
  %40 = load i32, i32* %39, align 4, !tbaa !221
  %41 = sext i32 %32 to i64
  %42 = getelementptr inbounds float, float* %4, i64 %41
  %43 = bitcast float* %42 to i32*
  store i32 %40, i32* %43, align 4, !tbaa !224
  %44 = or i64 %27, 1
  %45 = trunc i64 %44 to i32
  %46 = add i32 %21, %45
  %47 = trunc i64 %44 to i32
  %48 = add i32 %47, -448
  %49 = add i32 %26, %48
  %50 = add i32 %49, %25
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to i32*
  %54 = load i32, i32* %53, align 4, !tbaa !221
  %55 = sext i32 %46 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to i32*
  store i32 %54, i32* %57, align 4, !tbaa !224
  %58 = or i64 %27, 2
  %59 = trunc i64 %58 to i32
  %60 = add i32 %21, %59
  %61 = trunc i64 %58 to i32
  %62 = add i32 %61, -448
  %63 = add i32 %26, %62
  %64 = add i32 %63, %25
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to i32*
  %68 = load i32, i32* %67, align 4, !tbaa !221
  %69 = sext i32 %60 to i64
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = bitcast float* %70 to i32*
  store i32 %68, i32* %71, align 4, !tbaa !224
  %72 = or i64 %27, 3
  %73 = trunc i64 %72 to i32
  %74 = add i32 %21, %73
  %75 = trunc i64 %72 to i32
  %76 = add i32 %75, -448
  %77 = add i32 %26, %76
  %78 = add i32 %77, %25
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !221
  %83 = sext i32 %74 to i64
  %84 = getelementptr inbounds float, float* %4, i64 %83
  %85 = bitcast float* %84 to i32*
  store i32 %82, i32* %85, align 4, !tbaa !224
  %86 = or i64 %27, 4
  %87 = trunc i64 %86 to i32
  %88 = add i32 %21, %87
  %89 = trunc i64 %86 to i32
  %90 = add i32 %89, -448
  %91 = add i32 %26, %90
  %92 = add i32 %91, %25
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to i32*
  %96 = load i32, i32* %95, align 4, !tbaa !221
  %97 = sext i32 %88 to i64
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = bitcast float* %98 to i32*
  store i32 %96, i32* %99, align 4, !tbaa !224
  %100 = or i64 %27, 5
  %101 = trunc i64 %100 to i32
  %102 = add i32 %21, %101
  %103 = trunc i64 %100 to i32
  %104 = add i32 %103, -448
  %105 = add i32 %26, %104
  %106 = add i32 %105, %25
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to i32*
  %110 = load i32, i32* %109, align 4, !tbaa !221
  %111 = sext i32 %102 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to i32*
  store i32 %110, i32* %113, align 4, !tbaa !224
  %114 = or i64 %27, 6
  %115 = trunc i64 %114 to i32
  %116 = add i32 %21, %115
  %117 = trunc i64 %114 to i32
  %118 = add i32 %117, -448
  %119 = add i32 %26, %118
  %120 = add i32 %119, %25
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to i32*
  %124 = load i32, i32* %123, align 4, !tbaa !221
  %125 = sext i32 %116 to i64
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = bitcast float* %126 to i32*
  store i32 %124, i32* %127, align 4, !tbaa !224
  %128 = or i64 %27, 7
  %129 = trunc i64 %128 to i32
  %130 = add i32 %21, %129
  %131 = trunc i64 %128 to i32
  %132 = add i32 %131, -448
  %133 = add i32 %26, %132
  %134 = add i32 %133, %25
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %7, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !221
  %139 = sext i32 %130 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to i32*
  store i32 %138, i32* %141, align 4, !tbaa !224
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %142 = sext i32 %32 to i64
  %143 = getelementptr inbounds float, float* %4, i64 %142
  store float 0.000000e+00, float* %143, align 4, !tbaa !224
  %144 = trunc i64 %27 to i32
  %145 = or i32 %144, 1
  %146 = add i32 %145, %21
  %147 = sext i32 %146 to i64
  %148 = getelementptr inbounds float, float* %4, i64 %147
  store float 0.000000e+00, float* %148, align 4, !tbaa !224
  %149 = trunc i64 %27 to i32
  %150 = or i32 %149, 2
  %151 = add i32 %150, %21
  %152 = sext i32 %151 to i64
  %153 = getelementptr inbounds float, float* %4, i64 %152
  store float 0.000000e+00, float* %153, align 4, !tbaa !224
  %154 = trunc i64 %27 to i32
  %155 = or i32 %154, 3
  %156 = add i32 %155, %21
  %157 = sext i32 %156 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  store float 0.000000e+00, float* %158, align 4, !tbaa !224
  %159 = trunc i64 %27 to i32
  %160 = or i32 %159, 4
  %161 = add i32 %160, %21
  %162 = sext i32 %161 to i64
  %163 = getelementptr inbounds float, float* %4, i64 %162
  store float 0.000000e+00, float* %163, align 4, !tbaa !224
  %164 = trunc i64 %27 to i32
  %165 = or i32 %164, 5
  %166 = add i32 %165, %21
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  store float 0.000000e+00, float* %168, align 4, !tbaa !224
  %169 = trunc i64 %27 to i32
  %170 = or i32 %169, 6
  %171 = add i32 %170, %21
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds float, float* %4, i64 %172
  store float 0.000000e+00, float* %173, align 4, !tbaa !224
  %174 = trunc i64 %27 to i32
  %175 = or i32 %174, 7
  %176 = add i32 %175, %21
  %177 = sext i32 %176 to i64
  %178 = getelementptr inbounds float, float* %4, i64 %177
  store float 0.000000e+00, float* %178, align 4, !tbaa !224
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %179 = shl nsw i64 %indvars.iv, 3
  %180 = trunc i64 %179 to i32
  %181 = add i32 %21, %180
  %182 = sext i32 %181 to i64
  %183 = getelementptr inbounds float, float* %4, i64 %182
  store float 0.000000e+00, float* %183, align 4, !tbaa !224
  %184 = trunc i64 %179 to i32
  %185 = or i32 %184, 1
  %186 = add i32 %185, %21
  %187 = sext i32 %186 to i64
  %188 = getelementptr inbounds float, float* %4, i64 %187
  store float 0.000000e+00, float* %188, align 4, !tbaa !224
  %189 = trunc i64 %179 to i32
  %190 = or i32 %189, 2
  %191 = add i32 %190, %21
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %4, i64 %192
  store float 0.000000e+00, float* %193, align 4, !tbaa !224
  %194 = trunc i64 %179 to i32
  %195 = or i32 %194, 3
  %196 = add i32 %195, %21
  %197 = sext i32 %196 to i64
  %198 = getelementptr inbounds float, float* %4, i64 %197
  store float 0.000000e+00, float* %198, align 4, !tbaa !224
  %199 = trunc i64 %179 to i32
  %200 = or i32 %199, 4
  %201 = add i32 %200, %21
  %202 = sext i32 %201 to i64
  %203 = getelementptr inbounds float, float* %4, i64 %202
  store float 0.000000e+00, float* %203, align 4, !tbaa !224
  %204 = trunc i64 %179 to i32
  %205 = or i32 %204, 5
  %206 = add i32 %205, %21
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  store float 0.000000e+00, float* %208, align 4, !tbaa !224
  %209 = trunc i64 %179 to i32
  %210 = or i32 %209, 6
  %211 = add i32 %210, %21
  %212 = sext i32 %211 to i64
  %213 = getelementptr inbounds float, float* %4, i64 %212
  store float 0.000000e+00, float* %213, align 4, !tbaa !224
  %214 = trunc i64 %179 to i32
  %215 = or i32 %214, 7
  %216 = add i32 %215, %21
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds float, float* %4, i64 %217
  store float 0.000000e+00, float* %218, align 4, !tbaa !224
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 31
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %219 = add nsw i32 %20, 1
  %220 = icmp slt i32 %219, %15
  br i1 %220, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.13(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = alloca [27 x <8 x float>], align 32
  %.sub = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0
  %4 = bitcast i8* %2 to float**
  %5 = load float*, float** %4, align 8
  %6 = getelementptr inbounds i8, i8* %2, i64 8
  %7 = bitcast i8* %6 to float**
  %8 = load float*, float** %7, align 8
  %9 = getelementptr inbounds i8, i8* %2, i64 16
  %10 = bitcast i8* %9 to float**
  %11 = load float*, float** %10, align 8
  %12 = getelementptr inbounds i8, i8* %2, i64 24
  %13 = bitcast i8* %12 to float**
  %14 = load float*, float** %13, align 8
  %15 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %16 = load i32, i32* %15, align 4
  %17 = add nsw i32 %16, 647
  %18 = sdiv i32 %17, %16
  %19 = add nsw i32 %0, 1
  %20 = mul nsw i32 %18, %19
  %21 = icmp slt i32 %20, 648
  %22 = select i1 %21, i32 %20, i32 648
  %23 = mul nsw i32 %18, %0
  %24 = icmp slt i32 %23, 648
  %25 = select i1 %24, i32 %23, i32 648
  %26 = icmp slt i32 %25, %22
  br i1 %26, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %27 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %28 = bitcast float* %27 to <8 x float>*
  %29 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %30 = bitcast float* %29 to <8 x float>*
  %31 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %32 = bitcast float* %31 to <8 x float>*
  %33 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %34 = bitcast float* %33 to <8 x float>*
  %35 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %36 = bitcast float* %35 to <8 x float>*
  %37 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %38 = bitcast float* %37 to <8 x float>*
  %39 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %40 = bitcast float* %39 to <8 x float>*
  %41 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %42 = bitcast float* %41 to <8 x float>*
  %43 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %44 = bitcast float* %43 to <8 x float>*
  %45 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %46 = bitcast float* %45 to <8 x float>*
  %47 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %48 = bitcast float* %47 to <8 x float>*
  %49 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 96
  %50 = bitcast float* %49 to <8 x float>*
  %51 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 104
  %52 = bitcast float* %51 to <8 x float>*
  %53 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 112
  %54 = bitcast float* %53 to <8 x float>*
  %55 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 120
  %56 = bitcast float* %55 to <8 x float>*
  %57 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 128
  %58 = bitcast float* %57 to <8 x float>*
  %59 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 136
  %60 = bitcast float* %59 to <8 x float>*
  %61 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 144
  %62 = bitcast float* %61 to <8 x float>*
  %63 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 152
  %64 = bitcast float* %63 to <8 x float>*
  %65 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 160
  %66 = bitcast float* %65 to <8 x float>*
  %67 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 168
  %68 = bitcast float* %67 to <8 x float>*
  %69 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 176
  %70 = bitcast float* %69 to <8 x float>*
  %71 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 184
  %72 = bitcast float* %71 to <8 x float>*
  %73 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 192
  %74 = bitcast float* %73 to <8 x float>*
  %75 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 200
  %76 = bitcast float* %75 to <8 x float>*
  %77 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 208
  %78 = bitcast float* %77 to <8 x float>*
  %79 = add i32 %25, 1
  %80 = sext i32 %79 to i64
  %81 = add nsw i64 %80, -1
  %82 = sext i32 %22 to i64
  %83 = bitcast [27 x <8 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv264 = phi i64 [ %81, %for_body.lr.ph ], [ %indvars.iv.next265, %for_end3 ]
  %84 = trunc i64 %indvars.iv264 to i32
  %85 = srem i32 %84, 27
  %86 = sdiv i32 %84, 27
  %87 = mul nsw i32 %86, 12800
  %88 = sext i32 %87 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull %83, i8 0, i64 864, i32 32, i1 false)
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv258 = phi i64 [ 0, %for_body ], [ %indvars.iv.next259, %for_end6 ]
  %.lcssa66.lcssa.lcssa224 = phi <8 x float> [ zeroinitializer, %for_body ], [ %479, %for_end6 ]
  %.lcssa64.lcssa.lcssa222 = phi <8 x float> [ zeroinitializer, %for_body ], [ %473, %for_end6 ]
  %.lcssa62.lcssa.lcssa220 = phi <8 x float> [ zeroinitializer, %for_body ], [ %467, %for_end6 ]
  %.lcssa60.lcssa.lcssa218 = phi <8 x float> [ zeroinitializer, %for_body ], [ %461, %for_end6 ]
  %.lcssa58.lcssa.lcssa216 = phi <8 x float> [ zeroinitializer, %for_body ], [ %455, %for_end6 ]
  %.lcssa56.lcssa.lcssa214 = phi <8 x float> [ zeroinitializer, %for_body ], [ %449, %for_end6 ]
  %.lcssa54.lcssa.lcssa212 = phi <8 x float> [ zeroinitializer, %for_body ], [ %443, %for_end6 ]
  %.lcssa52.lcssa.lcssa210 = phi <8 x float> [ zeroinitializer, %for_body ], [ %437, %for_end6 ]
  %.lcssa50.lcssa.lcssa208 = phi <8 x float> [ zeroinitializer, %for_body ], [ %431, %for_end6 ]
  %.lcssa48.lcssa.lcssa206 = phi <8 x float> [ zeroinitializer, %for_body ], [ %425, %for_end6 ]
  %.lcssa46.lcssa.lcssa204 = phi <8 x float> [ zeroinitializer, %for_body ], [ %419, %for_end6 ]
  %.lcssa44.lcssa.lcssa202 = phi <8 x float> [ zeroinitializer, %for_body ], [ %413, %for_end6 ]
  %.lcssa42.lcssa.lcssa200 = phi <8 x float> [ zeroinitializer, %for_body ], [ %407, %for_end6 ]
  %.lcssa40.lcssa.lcssa198 = phi <8 x float> [ zeroinitializer, %for_body ], [ %401, %for_end6 ]
  %.lcssa38.lcssa.lcssa196 = phi <8 x float> [ zeroinitializer, %for_body ], [ %395, %for_end6 ]
  %.lcssa36.lcssa.lcssa194 = phi <8 x float> [ zeroinitializer, %for_body ], [ %389, %for_end6 ]
  %.lcssa34.lcssa.lcssa192 = phi <8 x float> [ zeroinitializer, %for_body ], [ %383, %for_end6 ]
  %.lcssa32.lcssa.lcssa190 = phi <8 x float> [ zeroinitializer, %for_body ], [ %377, %for_end6 ]
  %.lcssa30.lcssa.lcssa188 = phi <8 x float> [ zeroinitializer, %for_body ], [ %371, %for_end6 ]
  %.lcssa28.lcssa.lcssa186 = phi <8 x float> [ zeroinitializer, %for_body ], [ %365, %for_end6 ]
  %.lcssa26.lcssa.lcssa184 = phi <8 x float> [ zeroinitializer, %for_body ], [ %359, %for_end6 ]
  %.lcssa24.lcssa.lcssa182 = phi <8 x float> [ zeroinitializer, %for_body ], [ %353, %for_end6 ]
  %.lcssa22.lcssa.lcssa180 = phi <8 x float> [ zeroinitializer, %for_body ], [ %347, %for_end6 ]
  %.lcssa20.lcssa.lcssa179 = phi <8 x float> [ zeroinitializer, %for_body ], [ %341, %for_end6 ]
  %.lcssa18.lcssa.lcssa177 = phi <8 x float> [ zeroinitializer, %for_body ], [ %335, %for_end6 ]
  %.lcssa16.lcssa.lcssa175 = phi <8 x float> [ zeroinitializer, %for_body ], [ %329, %for_end6 ]
  %.lcssa.lcssa.lcssa173 = phi <8 x float> [ zeroinitializer, %for_body ], [ %323, %for_end6 ]
  %89 = mul nuw nsw i64 %indvars.iv258, 1600
  %90 = add nsw i64 %89, %88
  %91 = trunc i64 %indvars.iv258 to i32
  %92 = mul i32 %91, 7688
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  store <8 x float> %323, <8 x float>* %.sub, align 32, !tbaa !227
  store <8 x float> %329, <8 x float>* %28, align 32, !tbaa !227
  store <8 x float> %335, <8 x float>* %30, align 32, !tbaa !227
  store <8 x float> %341, <8 x float>* %32, align 32, !tbaa !227
  store <8 x float> %347, <8 x float>* %34, align 32, !tbaa !227
  store <8 x float> %353, <8 x float>* %36, align 32, !tbaa !227
  store <8 x float> %359, <8 x float>* %38, align 32, !tbaa !227
  store <8 x float> %365, <8 x float>* %40, align 32, !tbaa !227
  store <8 x float> %371, <8 x float>* %42, align 32, !tbaa !227
  store <8 x float> %377, <8 x float>* %44, align 32, !tbaa !227
  store <8 x float> %383, <8 x float>* %46, align 32, !tbaa !227
  store <8 x float> %389, <8 x float>* %48, align 32, !tbaa !227
  store <8 x float> %395, <8 x float>* %50, align 32, !tbaa !227
  store <8 x float> %401, <8 x float>* %52, align 32, !tbaa !227
  store <8 x float> %407, <8 x float>* %54, align 32, !tbaa !227
  store <8 x float> %413, <8 x float>* %56, align 32, !tbaa !227
  store <8 x float> %419, <8 x float>* %58, align 32, !tbaa !227
  store <8 x float> %425, <8 x float>* %60, align 32, !tbaa !227
  store <8 x float> %431, <8 x float>* %62, align 32, !tbaa !227
  store <8 x float> %437, <8 x float>* %64, align 32, !tbaa !227
  store <8 x float> %443, <8 x float>* %66, align 32, !tbaa !227
  store <8 x float> %449, <8 x float>* %68, align 32, !tbaa !227
  store <8 x float> %455, <8 x float>* %70, align 32, !tbaa !227
  store <8 x float> %461, <8 x float>* %72, align 32, !tbaa !227
  store <8 x float> %467, <8 x float>* %74, align 32, !tbaa !227
  store <8 x float> %473, <8 x float>* %76, align 32, !tbaa !227
  store <8 x float> %479, <8 x float>* %78, align 32, !tbaa !227
  %93 = mul nsw i64 %indvars.iv264, 216
  %94 = shl nsw i32 %86, 3
  %95 = sext i32 %94 to i64
  %96 = getelementptr inbounds float, float* %14, i64 %95
  %97 = bitcast float* %96 to <8 x float>*
  %98 = load <8 x float>, <8 x float>* %97, align 32, !tbaa !238
  %99 = fadd <8 x float> %98, %323
  %100 = fcmp ogt <8 x float> %99, zeroinitializer
  %101 = select <8 x i1> %100, <8 x float> %99, <8 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %11, i64 %93
  %103 = bitcast float* %102 to <8 x float>*
  store <8 x float> %101, <8 x float>* %103, align 32, !tbaa !241
  %104 = add nsw i64 %93, 8
  %105 = fadd <8 x float> %98, %329
  %106 = fcmp ogt <8 x float> %105, zeroinitializer
  %107 = select <8 x i1> %106, <8 x float> %105, <8 x float> zeroinitializer
  %108 = getelementptr inbounds float, float* %11, i64 %104
  %109 = bitcast float* %108 to <8 x float>*
  store <8 x float> %107, <8 x float>* %109, align 32, !tbaa !241
  %110 = add nsw i64 %93, 16
  %111 = fadd <8 x float> %98, %335
  %112 = fcmp ogt <8 x float> %111, zeroinitializer
  %113 = select <8 x i1> %112, <8 x float> %111, <8 x float> zeroinitializer
  %114 = getelementptr inbounds float, float* %11, i64 %110
  %115 = bitcast float* %114 to <8 x float>*
  store <8 x float> %113, <8 x float>* %115, align 32, !tbaa !241
  %116 = add nsw i64 %93, 24
  %117 = fadd <8 x float> %98, %341
  %118 = fcmp ogt <8 x float> %117, zeroinitializer
  %119 = select <8 x i1> %118, <8 x float> %117, <8 x float> zeroinitializer
  %120 = getelementptr inbounds float, float* %11, i64 %116
  %121 = bitcast float* %120 to <8 x float>*
  store <8 x float> %119, <8 x float>* %121, align 32, !tbaa !241
  %122 = add nsw i64 %93, 32
  %123 = fadd <8 x float> %98, %347
  %124 = fcmp ogt <8 x float> %123, zeroinitializer
  %125 = select <8 x i1> %124, <8 x float> %123, <8 x float> zeroinitializer
  %126 = getelementptr inbounds float, float* %11, i64 %122
  %127 = bitcast float* %126 to <8 x float>*
  store <8 x float> %125, <8 x float>* %127, align 32, !tbaa !241
  %128 = add nsw i64 %93, 40
  %129 = fadd <8 x float> %98, %353
  %130 = fcmp ogt <8 x float> %129, zeroinitializer
  %131 = select <8 x i1> %130, <8 x float> %129, <8 x float> zeroinitializer
  %132 = getelementptr inbounds float, float* %11, i64 %128
  %133 = bitcast float* %132 to <8 x float>*
  store <8 x float> %131, <8 x float>* %133, align 32, !tbaa !241
  %134 = add nsw i64 %93, 48
  %135 = fadd <8 x float> %98, %359
  %136 = fcmp ogt <8 x float> %135, zeroinitializer
  %137 = select <8 x i1> %136, <8 x float> %135, <8 x float> zeroinitializer
  %138 = getelementptr inbounds float, float* %11, i64 %134
  %139 = bitcast float* %138 to <8 x float>*
  store <8 x float> %137, <8 x float>* %139, align 32, !tbaa !241
  %140 = add nsw i64 %93, 56
  %141 = fadd <8 x float> %98, %365
  %142 = fcmp ogt <8 x float> %141, zeroinitializer
  %143 = select <8 x i1> %142, <8 x float> %141, <8 x float> zeroinitializer
  %144 = getelementptr inbounds float, float* %11, i64 %140
  %145 = bitcast float* %144 to <8 x float>*
  store <8 x float> %143, <8 x float>* %145, align 32, !tbaa !241
  %146 = add nsw i64 %93, 64
  %147 = fadd <8 x float> %98, %371
  %148 = fcmp ogt <8 x float> %147, zeroinitializer
  %149 = select <8 x i1> %148, <8 x float> %147, <8 x float> zeroinitializer
  %150 = getelementptr inbounds float, float* %11, i64 %146
  %151 = bitcast float* %150 to <8 x float>*
  store <8 x float> %149, <8 x float>* %151, align 32, !tbaa !241
  %152 = add nsw i64 %93, 72
  %153 = fadd <8 x float> %98, %377
  %154 = fcmp ogt <8 x float> %153, zeroinitializer
  %155 = select <8 x i1> %154, <8 x float> %153, <8 x float> zeroinitializer
  %156 = getelementptr inbounds float, float* %11, i64 %152
  %157 = bitcast float* %156 to <8 x float>*
  store <8 x float> %155, <8 x float>* %157, align 32, !tbaa !241
  %158 = add nsw i64 %93, 80
  %159 = fadd <8 x float> %98, %383
  %160 = fcmp ogt <8 x float> %159, zeroinitializer
  %161 = select <8 x i1> %160, <8 x float> %159, <8 x float> zeroinitializer
  %162 = getelementptr inbounds float, float* %11, i64 %158
  %163 = bitcast float* %162 to <8 x float>*
  store <8 x float> %161, <8 x float>* %163, align 32, !tbaa !241
  %164 = add nsw i64 %93, 88
  %165 = fadd <8 x float> %98, %389
  %166 = fcmp ogt <8 x float> %165, zeroinitializer
  %167 = select <8 x i1> %166, <8 x float> %165, <8 x float> zeroinitializer
  %168 = getelementptr inbounds float, float* %11, i64 %164
  %169 = bitcast float* %168 to <8 x float>*
  store <8 x float> %167, <8 x float>* %169, align 32, !tbaa !241
  %170 = add nsw i64 %93, 96
  %171 = load <8 x float>, <8 x float>* %50, align 32, !tbaa !244
  %172 = fadd <8 x float> %98, %171
  %173 = fcmp ogt <8 x float> %172, zeroinitializer
  %174 = select <8 x i1> %173, <8 x float> %172, <8 x float> zeroinitializer
  %175 = getelementptr inbounds float, float* %11, i64 %170
  %176 = bitcast float* %175 to <8 x float>*
  store <8 x float> %174, <8 x float>* %176, align 32, !tbaa !241
  %177 = add nsw i64 %93, 104
  %178 = load <8 x float>, <8 x float>* %52, align 32, !tbaa !244
  %179 = fadd <8 x float> %98, %178
  %180 = fcmp ogt <8 x float> %179, zeroinitializer
  %181 = select <8 x i1> %180, <8 x float> %179, <8 x float> zeroinitializer
  %182 = getelementptr inbounds float, float* %11, i64 %177
  %183 = bitcast float* %182 to <8 x float>*
  store <8 x float> %181, <8 x float>* %183, align 32, !tbaa !241
  %184 = add nsw i64 %93, 112
  %185 = load <8 x float>, <8 x float>* %54, align 32, !tbaa !244
  %186 = fadd <8 x float> %98, %185
  %187 = fcmp ogt <8 x float> %186, zeroinitializer
  %188 = select <8 x i1> %187, <8 x float> %186, <8 x float> zeroinitializer
  %189 = getelementptr inbounds float, float* %11, i64 %184
  %190 = bitcast float* %189 to <8 x float>*
  store <8 x float> %188, <8 x float>* %190, align 32, !tbaa !241
  %191 = add nsw i64 %93, 120
  %192 = load <8 x float>, <8 x float>* %56, align 32, !tbaa !244
  %193 = fadd <8 x float> %98, %192
  %194 = fcmp ogt <8 x float> %193, zeroinitializer
  %195 = select <8 x i1> %194, <8 x float> %193, <8 x float> zeroinitializer
  %196 = getelementptr inbounds float, float* %11, i64 %191
  %197 = bitcast float* %196 to <8 x float>*
  store <8 x float> %195, <8 x float>* %197, align 32, !tbaa !241
  %198 = add nsw i64 %93, 128
  %199 = load <8 x float>, <8 x float>* %58, align 32, !tbaa !244
  %200 = fadd <8 x float> %98, %199
  %201 = fcmp ogt <8 x float> %200, zeroinitializer
  %202 = select <8 x i1> %201, <8 x float> %200, <8 x float> zeroinitializer
  %203 = getelementptr inbounds float, float* %11, i64 %198
  %204 = bitcast float* %203 to <8 x float>*
  store <8 x float> %202, <8 x float>* %204, align 32, !tbaa !241
  %205 = add nsw i64 %93, 136
  %206 = load <8 x float>, <8 x float>* %60, align 32, !tbaa !244
  %207 = fadd <8 x float> %98, %206
  %208 = fcmp ogt <8 x float> %207, zeroinitializer
  %209 = select <8 x i1> %208, <8 x float> %207, <8 x float> zeroinitializer
  %210 = getelementptr inbounds float, float* %11, i64 %205
  %211 = bitcast float* %210 to <8 x float>*
  store <8 x float> %209, <8 x float>* %211, align 32, !tbaa !241
  %212 = add nsw i64 %93, 144
  %213 = load <8 x float>, <8 x float>* %62, align 32, !tbaa !244
  %214 = fadd <8 x float> %98, %213
  %215 = fcmp ogt <8 x float> %214, zeroinitializer
  %216 = select <8 x i1> %215, <8 x float> %214, <8 x float> zeroinitializer
  %217 = getelementptr inbounds float, float* %11, i64 %212
  %218 = bitcast float* %217 to <8 x float>*
  store <8 x float> %216, <8 x float>* %218, align 32, !tbaa !241
  %219 = add nsw i64 %93, 152
  %220 = load <8 x float>, <8 x float>* %64, align 32, !tbaa !244
  %221 = fadd <8 x float> %98, %220
  %222 = fcmp ogt <8 x float> %221, zeroinitializer
  %223 = select <8 x i1> %222, <8 x float> %221, <8 x float> zeroinitializer
  %224 = getelementptr inbounds float, float* %11, i64 %219
  %225 = bitcast float* %224 to <8 x float>*
  store <8 x float> %223, <8 x float>* %225, align 32, !tbaa !241
  %226 = add nsw i64 %93, 160
  %227 = load <8 x float>, <8 x float>* %66, align 32, !tbaa !244
  %228 = fadd <8 x float> %98, %227
  %229 = fcmp ogt <8 x float> %228, zeroinitializer
  %230 = select <8 x i1> %229, <8 x float> %228, <8 x float> zeroinitializer
  %231 = getelementptr inbounds float, float* %11, i64 %226
  %232 = bitcast float* %231 to <8 x float>*
  store <8 x float> %230, <8 x float>* %232, align 32, !tbaa !241
  %233 = add nsw i64 %93, 168
  %234 = load <8 x float>, <8 x float>* %68, align 32, !tbaa !244
  %235 = fadd <8 x float> %98, %234
  %236 = fcmp ogt <8 x float> %235, zeroinitializer
  %237 = select <8 x i1> %236, <8 x float> %235, <8 x float> zeroinitializer
  %238 = getelementptr inbounds float, float* %11, i64 %233
  %239 = bitcast float* %238 to <8 x float>*
  store <8 x float> %237, <8 x float>* %239, align 32, !tbaa !241
  %240 = add nsw i64 %93, 176
  %241 = load <8 x float>, <8 x float>* %70, align 32, !tbaa !244
  %242 = fadd <8 x float> %98, %241
  %243 = fcmp ogt <8 x float> %242, zeroinitializer
  %244 = select <8 x i1> %243, <8 x float> %242, <8 x float> zeroinitializer
  %245 = getelementptr inbounds float, float* %11, i64 %240
  %246 = bitcast float* %245 to <8 x float>*
  store <8 x float> %244, <8 x float>* %246, align 32, !tbaa !241
  %247 = add nsw i64 %93, 184
  %248 = load <8 x float>, <8 x float>* %72, align 32, !tbaa !244
  %249 = fadd <8 x float> %98, %248
  %250 = fcmp ogt <8 x float> %249, zeroinitializer
  %251 = select <8 x i1> %250, <8 x float> %249, <8 x float> zeroinitializer
  %252 = getelementptr inbounds float, float* %11, i64 %247
  %253 = bitcast float* %252 to <8 x float>*
  store <8 x float> %251, <8 x float>* %253, align 32, !tbaa !241
  %254 = add nsw i64 %93, 192
  %255 = load <8 x float>, <8 x float>* %74, align 32, !tbaa !244
  %256 = fadd <8 x float> %98, %255
  %257 = fcmp ogt <8 x float> %256, zeroinitializer
  %258 = select <8 x i1> %257, <8 x float> %256, <8 x float> zeroinitializer
  %259 = getelementptr inbounds float, float* %11, i64 %254
  %260 = bitcast float* %259 to <8 x float>*
  store <8 x float> %258, <8 x float>* %260, align 32, !tbaa !241
  %261 = add nsw i64 %93, 200
  %262 = load <8 x float>, <8 x float>* %76, align 32, !tbaa !244
  %263 = fadd <8 x float> %98, %262
  %264 = fcmp ogt <8 x float> %263, zeroinitializer
  %265 = select <8 x i1> %264, <8 x float> %263, <8 x float> zeroinitializer
  %266 = getelementptr inbounds float, float* %11, i64 %261
  %267 = bitcast float* %266 to <8 x float>*
  store <8 x float> %265, <8 x float>* %267, align 32, !tbaa !241
  %268 = add nsw i64 %93, 208
  %269 = load <8 x float>, <8 x float>* %78, align 32, !tbaa !244
  %270 = fadd <8 x float> %98, %269
  %271 = fcmp ogt <8 x float> %270, zeroinitializer
  %272 = select <8 x i1> %271, <8 x float> %270, <8 x float> zeroinitializer
  %273 = getelementptr inbounds float, float* %11, i64 %268
  %274 = bitcast float* %273 to <8 x float>*
  store <8 x float> %272, <8 x float>* %274, align 32, !tbaa !241
  %indvars.iv.next265 = add nsw i64 %indvars.iv264, 1
  %275 = icmp slt i64 %indvars.iv.next265, %82
  br i1 %275, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_end9, %for_body2
  %indvars.iv254 = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next255, %for_end9 ]
  %.lcssa66.lcssa171 = phi <8 x float> [ %.lcssa66.lcssa.lcssa224, %for_body2 ], [ %479, %for_end9 ]
  %.lcssa64.lcssa169 = phi <8 x float> [ %.lcssa64.lcssa.lcssa222, %for_body2 ], [ %473, %for_end9 ]
  %.lcssa62.lcssa167 = phi <8 x float> [ %.lcssa62.lcssa.lcssa220, %for_body2 ], [ %467, %for_end9 ]
  %.lcssa60.lcssa165 = phi <8 x float> [ %.lcssa60.lcssa.lcssa218, %for_body2 ], [ %461, %for_end9 ]
  %.lcssa58.lcssa163 = phi <8 x float> [ %.lcssa58.lcssa.lcssa216, %for_body2 ], [ %455, %for_end9 ]
  %.lcssa56.lcssa161 = phi <8 x float> [ %.lcssa56.lcssa.lcssa214, %for_body2 ], [ %449, %for_end9 ]
  %.lcssa54.lcssa159 = phi <8 x float> [ %.lcssa54.lcssa.lcssa212, %for_body2 ], [ %443, %for_end9 ]
  %.lcssa52.lcssa157 = phi <8 x float> [ %.lcssa52.lcssa.lcssa210, %for_body2 ], [ %437, %for_end9 ]
  %.lcssa50.lcssa155 = phi <8 x float> [ %.lcssa50.lcssa.lcssa208, %for_body2 ], [ %431, %for_end9 ]
  %.lcssa48.lcssa153 = phi <8 x float> [ %.lcssa48.lcssa.lcssa206, %for_body2 ], [ %425, %for_end9 ]
  %.lcssa46.lcssa151 = phi <8 x float> [ %.lcssa46.lcssa.lcssa204, %for_body2 ], [ %419, %for_end9 ]
  %.lcssa44.lcssa149 = phi <8 x float> [ %.lcssa44.lcssa.lcssa202, %for_body2 ], [ %413, %for_end9 ]
  %.lcssa42.lcssa147 = phi <8 x float> [ %.lcssa42.lcssa.lcssa200, %for_body2 ], [ %407, %for_end9 ]
  %.lcssa40.lcssa145 = phi <8 x float> [ %.lcssa40.lcssa.lcssa198, %for_body2 ], [ %401, %for_end9 ]
  %.lcssa38.lcssa143 = phi <8 x float> [ %.lcssa38.lcssa.lcssa196, %for_body2 ], [ %395, %for_end9 ]
  %.lcssa36.lcssa141 = phi <8 x float> [ %.lcssa36.lcssa.lcssa194, %for_body2 ], [ %389, %for_end9 ]
  %.lcssa34.lcssa139 = phi <8 x float> [ %.lcssa34.lcssa.lcssa192, %for_body2 ], [ %383, %for_end9 ]
  %.lcssa32.lcssa137 = phi <8 x float> [ %.lcssa32.lcssa.lcssa190, %for_body2 ], [ %377, %for_end9 ]
  %.lcssa30.lcssa135 = phi <8 x float> [ %.lcssa30.lcssa.lcssa188, %for_body2 ], [ %371, %for_end9 ]
  %.lcssa28.lcssa133 = phi <8 x float> [ %.lcssa28.lcssa.lcssa186, %for_body2 ], [ %365, %for_end9 ]
  %.lcssa26.lcssa131 = phi <8 x float> [ %.lcssa26.lcssa.lcssa184, %for_body2 ], [ %359, %for_end9 ]
  %.lcssa24.lcssa129 = phi <8 x float> [ %.lcssa24.lcssa.lcssa182, %for_body2 ], [ %353, %for_end9 ]
  %.lcssa22.lcssa127 = phi <8 x float> [ %.lcssa22.lcssa.lcssa180, %for_body2 ], [ %347, %for_end9 ]
  %.lcssa20.lcssa125 = phi <8 x float> [ %.lcssa20.lcssa.lcssa179, %for_body2 ], [ %341, %for_end9 ]
  %.lcssa18.lcssa124 = phi <8 x float> [ %.lcssa18.lcssa.lcssa177, %for_body2 ], [ %335, %for_end9 ]
  %.lcssa16.lcssa122 = phi <8 x float> [ %.lcssa16.lcssa.lcssa175, %for_body2 ], [ %329, %for_end9 ]
  %.lcssa.lcssa120 = phi <8 x float> [ %.lcssa.lcssa.lcssa173, %for_body2 ], [ %323, %for_end9 ]
  %276 = phi i32 [ 0, %for_body2 ], [ %285, %for_end9 ]
  %reass.add = add nsw i32 %276, %85
  %reass.mul = mul i32 %reass.add, 248
  %277 = add nsw i32 %reass.mul, %92
  %278 = mul nuw nsw i64 %indvars.iv254, 320
  %279 = add nsw i64 %90, %278
  %280 = sext i32 %277 to i64
  br label %for_body8

for_end6:                                         ; preds = %for_end9
  %indvars.iv.next259 = add nuw nsw i64 %indvars.iv258, 1
  %exitcond260 = icmp eq i64 %indvars.iv.next259, 8
  br i1 %exitcond260, label %for_end3, label %for_body2, !prof !28

for_body8:                                        ; preds = %for_end12, %for_body5
  %indvars.iv251 = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next252, %for_end12 ]
  %.lcssa66119 = phi <8 x float> [ %.lcssa66.lcssa171, %for_body5 ], [ %479, %for_end12 ]
  %.lcssa64117 = phi <8 x float> [ %.lcssa64.lcssa169, %for_body5 ], [ %473, %for_end12 ]
  %.lcssa62115 = phi <8 x float> [ %.lcssa62.lcssa167, %for_body5 ], [ %467, %for_end12 ]
  %.lcssa60113 = phi <8 x float> [ %.lcssa60.lcssa165, %for_body5 ], [ %461, %for_end12 ]
  %.lcssa58111 = phi <8 x float> [ %.lcssa58.lcssa163, %for_body5 ], [ %455, %for_end12 ]
  %.lcssa56109 = phi <8 x float> [ %.lcssa56.lcssa161, %for_body5 ], [ %449, %for_end12 ]
  %.lcssa54107 = phi <8 x float> [ %.lcssa54.lcssa159, %for_body5 ], [ %443, %for_end12 ]
  %.lcssa52105 = phi <8 x float> [ %.lcssa52.lcssa157, %for_body5 ], [ %437, %for_end12 ]
  %.lcssa50103 = phi <8 x float> [ %.lcssa50.lcssa155, %for_body5 ], [ %431, %for_end12 ]
  %.lcssa48101 = phi <8 x float> [ %.lcssa48.lcssa153, %for_body5 ], [ %425, %for_end12 ]
  %.lcssa4699 = phi <8 x float> [ %.lcssa46.lcssa151, %for_body5 ], [ %419, %for_end12 ]
  %.lcssa4497 = phi <8 x float> [ %.lcssa44.lcssa149, %for_body5 ], [ %413, %for_end12 ]
  %.lcssa4295 = phi <8 x float> [ %.lcssa42.lcssa147, %for_body5 ], [ %407, %for_end12 ]
  %.lcssa4093 = phi <8 x float> [ %.lcssa40.lcssa145, %for_body5 ], [ %401, %for_end12 ]
  %.lcssa3891 = phi <8 x float> [ %.lcssa38.lcssa143, %for_body5 ], [ %395, %for_end12 ]
  %.lcssa3689 = phi <8 x float> [ %.lcssa36.lcssa141, %for_body5 ], [ %389, %for_end12 ]
  %.lcssa3487 = phi <8 x float> [ %.lcssa34.lcssa139, %for_body5 ], [ %383, %for_end12 ]
  %.lcssa3285 = phi <8 x float> [ %.lcssa32.lcssa137, %for_body5 ], [ %377, %for_end12 ]
  %.lcssa3083 = phi <8 x float> [ %.lcssa30.lcssa135, %for_body5 ], [ %371, %for_end12 ]
  %.lcssa2881 = phi <8 x float> [ %.lcssa28.lcssa133, %for_body5 ], [ %365, %for_end12 ]
  %.lcssa2679 = phi <8 x float> [ %.lcssa26.lcssa131, %for_body5 ], [ %359, %for_end12 ]
  %.lcssa2477 = phi <8 x float> [ %.lcssa24.lcssa129, %for_body5 ], [ %353, %for_end12 ]
  %.lcssa2275 = phi <8 x float> [ %.lcssa22.lcssa127, %for_body5 ], [ %347, %for_end12 ]
  %.lcssa2073 = phi <8 x float> [ %.lcssa20.lcssa125, %for_body5 ], [ %341, %for_end12 ]
  %.lcssa1871 = phi <8 x float> [ %.lcssa18.lcssa124, %for_body5 ], [ %335, %for_end12 ]
  %.lcssa1670 = phi <8 x float> [ %.lcssa16.lcssa122, %for_body5 ], [ %329, %for_end12 ]
  %.lcssa68 = phi <8 x float> [ %.lcssa.lcssa120, %for_body5 ], [ %323, %for_end12 ]
  %281 = shl i64 %indvars.iv251, 3
  %282 = add nsw i64 %281, %280
  %283 = shl i64 %indvars.iv251, 6
  %284 = add nsw i64 %279, %283
  br label %for_body11

for_end9:                                         ; preds = %for_end12
  %indvars.iv.next255 = add nuw nsw i64 %indvars.iv254, 1
  %285 = add nuw nsw i32 %276, 1
  %exitcond257 = icmp eq i64 %indvars.iv.next255, 5
  br i1 %exitcond257, label %for_end6, label %for_body5, !prof !28

for_body11:                                       ; preds = %for_body11, %for_body8
  %indvars.iv = phi i64 [ 0, %for_body8 ], [ %indvars.iv.next, %for_body11 ]
  %286 = phi <8 x float> [ %.lcssa66119, %for_body8 ], [ %479, %for_body11 ]
  %287 = phi <8 x float> [ %.lcssa64117, %for_body8 ], [ %473, %for_body11 ]
  %288 = phi <8 x float> [ %.lcssa62115, %for_body8 ], [ %467, %for_body11 ]
  %289 = phi <8 x float> [ %.lcssa60113, %for_body8 ], [ %461, %for_body11 ]
  %290 = phi <8 x float> [ %.lcssa58111, %for_body8 ], [ %455, %for_body11 ]
  %291 = phi <8 x float> [ %.lcssa56109, %for_body8 ], [ %449, %for_body11 ]
  %292 = phi <8 x float> [ %.lcssa54107, %for_body8 ], [ %443, %for_body11 ]
  %293 = phi <8 x float> [ %.lcssa52105, %for_body8 ], [ %437, %for_body11 ]
  %294 = phi <8 x float> [ %.lcssa50103, %for_body8 ], [ %431, %for_body11 ]
  %295 = phi <8 x float> [ %.lcssa48101, %for_body8 ], [ %425, %for_body11 ]
  %296 = phi <8 x float> [ %.lcssa4699, %for_body8 ], [ %419, %for_body11 ]
  %297 = phi <8 x float> [ %.lcssa4497, %for_body8 ], [ %413, %for_body11 ]
  %298 = phi <8 x float> [ %.lcssa4295, %for_body8 ], [ %407, %for_body11 ]
  %299 = phi <8 x float> [ %.lcssa4093, %for_body8 ], [ %401, %for_body11 ]
  %300 = phi <8 x float> [ %.lcssa3891, %for_body8 ], [ %395, %for_body11 ]
  %301 = phi <8 x float> [ %.lcssa3689, %for_body8 ], [ %389, %for_body11 ]
  %302 = phi <8 x float> [ %.lcssa3487, %for_body8 ], [ %383, %for_body11 ]
  %303 = phi <8 x float> [ %.lcssa3285, %for_body8 ], [ %377, %for_body11 ]
  %304 = phi <8 x float> [ %.lcssa3083, %for_body8 ], [ %371, %for_body11 ]
  %305 = phi <8 x float> [ %.lcssa2881, %for_body8 ], [ %365, %for_body11 ]
  %306 = phi <8 x float> [ %.lcssa2679, %for_body8 ], [ %359, %for_body11 ]
  %307 = phi <8 x float> [ %.lcssa2477, %for_body8 ], [ %353, %for_body11 ]
  %308 = phi <8 x float> [ %.lcssa2275, %for_body8 ], [ %347, %for_body11 ]
  %309 = phi <8 x float> [ %.lcssa2073, %for_body8 ], [ %341, %for_body11 ]
  %310 = phi <8 x float> [ %.lcssa1871, %for_body8 ], [ %335, %for_body11 ]
  %311 = phi <8 x float> [ %.lcssa1670, %for_body8 ], [ %329, %for_body11 ]
  %312 = phi <8 x float> [ %.lcssa68, %for_body8 ], [ %323, %for_body11 ]
  %313 = add nsw i64 %282, %indvars.iv
  %314 = getelementptr inbounds float, float* %5, i64 %313
  %315 = load float, float* %314, align 4, !tbaa !224
  %316 = insertelement <8 x float> undef, float %315, i32 0
  %317 = shufflevector <8 x float> %316, <8 x float> undef, <8 x i32> zeroinitializer
  %318 = shl i64 %indvars.iv, 3
  %319 = add nsw i64 %284, %318
  %320 = getelementptr inbounds float, float* %8, i64 %319
  %321 = bitcast float* %320 to <8 x float>*
  %322 = load <8 x float>, <8 x float>* %321, align 32, !tbaa !245
  %323 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %317, <8 x float> %322, <8 x float> %312)
  %324 = add nsw i64 %313, 8
  %325 = getelementptr inbounds float, float* %5, i64 %324
  %326 = load float, float* %325, align 4, !tbaa !224
  %327 = insertelement <8 x float> undef, float %326, i32 0
  %328 = shufflevector <8 x float> %327, <8 x float> undef, <8 x i32> zeroinitializer
  %329 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %328, <8 x float> %322, <8 x float> %311)
  %330 = add nsw i64 %313, 16
  %331 = getelementptr inbounds float, float* %5, i64 %330
  %332 = load float, float* %331, align 4, !tbaa !224
  %333 = insertelement <8 x float> undef, float %332, i32 0
  %334 = shufflevector <8 x float> %333, <8 x float> undef, <8 x i32> zeroinitializer
  %335 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %334, <8 x float> %322, <8 x float> %310)
  %336 = add nsw i64 %313, 24
  %337 = getelementptr inbounds float, float* %5, i64 %336
  %338 = load float, float* %337, align 4, !tbaa !224
  %339 = insertelement <8 x float> undef, float %338, i32 0
  %340 = shufflevector <8 x float> %339, <8 x float> undef, <8 x i32> zeroinitializer
  %341 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %340, <8 x float> %322, <8 x float> %309)
  %342 = add nsw i64 %313, 32
  %343 = getelementptr inbounds float, float* %5, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !224
  %345 = insertelement <8 x float> undef, float %344, i32 0
  %346 = shufflevector <8 x float> %345, <8 x float> undef, <8 x i32> zeroinitializer
  %347 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %346, <8 x float> %322, <8 x float> %308)
  %348 = add nsw i64 %313, 40
  %349 = getelementptr inbounds float, float* %5, i64 %348
  %350 = load float, float* %349, align 4, !tbaa !224
  %351 = insertelement <8 x float> undef, float %350, i32 0
  %352 = shufflevector <8 x float> %351, <8 x float> undef, <8 x i32> zeroinitializer
  %353 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %352, <8 x float> %322, <8 x float> %307)
  %354 = add nsw i64 %313, 48
  %355 = getelementptr inbounds float, float* %5, i64 %354
  %356 = load float, float* %355, align 4, !tbaa !224
  %357 = insertelement <8 x float> undef, float %356, i32 0
  %358 = shufflevector <8 x float> %357, <8 x float> undef, <8 x i32> zeroinitializer
  %359 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %358, <8 x float> %322, <8 x float> %306)
  %360 = add nsw i64 %313, 56
  %361 = getelementptr inbounds float, float* %5, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !224
  %363 = insertelement <8 x float> undef, float %362, i32 0
  %364 = shufflevector <8 x float> %363, <8 x float> undef, <8 x i32> zeroinitializer
  %365 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %364, <8 x float> %322, <8 x float> %305)
  %366 = add nsw i64 %313, 64
  %367 = getelementptr inbounds float, float* %5, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !224
  %369 = insertelement <8 x float> undef, float %368, i32 0
  %370 = shufflevector <8 x float> %369, <8 x float> undef, <8 x i32> zeroinitializer
  %371 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %370, <8 x float> %322, <8 x float> %304)
  %372 = add nsw i64 %313, 72
  %373 = getelementptr inbounds float, float* %5, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !224
  %375 = insertelement <8 x float> undef, float %374, i32 0
  %376 = shufflevector <8 x float> %375, <8 x float> undef, <8 x i32> zeroinitializer
  %377 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %376, <8 x float> %322, <8 x float> %303)
  %378 = add nsw i64 %313, 80
  %379 = getelementptr inbounds float, float* %5, i64 %378
  %380 = load float, float* %379, align 4, !tbaa !224
  %381 = insertelement <8 x float> undef, float %380, i32 0
  %382 = shufflevector <8 x float> %381, <8 x float> undef, <8 x i32> zeroinitializer
  %383 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %382, <8 x float> %322, <8 x float> %302)
  %384 = add nsw i64 %313, 88
  %385 = getelementptr inbounds float, float* %5, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !224
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shufflevector <8 x float> %387, <8 x float> undef, <8 x i32> zeroinitializer
  %389 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %388, <8 x float> %322, <8 x float> %301)
  %390 = add nsw i64 %313, 96
  %391 = getelementptr inbounds float, float* %5, i64 %390
  %392 = load float, float* %391, align 4, !tbaa !224
  %393 = insertelement <8 x float> undef, float %392, i32 0
  %394 = shufflevector <8 x float> %393, <8 x float> undef, <8 x i32> zeroinitializer
  %395 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %394, <8 x float> %322, <8 x float> %300)
  %396 = add nsw i64 %313, 104
  %397 = getelementptr inbounds float, float* %5, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !224
  %399 = insertelement <8 x float> undef, float %398, i32 0
  %400 = shufflevector <8 x float> %399, <8 x float> undef, <8 x i32> zeroinitializer
  %401 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %400, <8 x float> %322, <8 x float> %299)
  %402 = add nsw i64 %313, 112
  %403 = getelementptr inbounds float, float* %5, i64 %402
  %404 = load float, float* %403, align 4, !tbaa !224
  %405 = insertelement <8 x float> undef, float %404, i32 0
  %406 = shufflevector <8 x float> %405, <8 x float> undef, <8 x i32> zeroinitializer
  %407 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %406, <8 x float> %322, <8 x float> %298)
  %408 = add nsw i64 %313, 120
  %409 = getelementptr inbounds float, float* %5, i64 %408
  %410 = load float, float* %409, align 4, !tbaa !224
  %411 = insertelement <8 x float> undef, float %410, i32 0
  %412 = shufflevector <8 x float> %411, <8 x float> undef, <8 x i32> zeroinitializer
  %413 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %412, <8 x float> %322, <8 x float> %297)
  %414 = add nsw i64 %313, 128
  %415 = getelementptr inbounds float, float* %5, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !224
  %417 = insertelement <8 x float> undef, float %416, i32 0
  %418 = shufflevector <8 x float> %417, <8 x float> undef, <8 x i32> zeroinitializer
  %419 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %418, <8 x float> %322, <8 x float> %296)
  %420 = add nsw i64 %313, 136
  %421 = getelementptr inbounds float, float* %5, i64 %420
  %422 = load float, float* %421, align 4, !tbaa !224
  %423 = insertelement <8 x float> undef, float %422, i32 0
  %424 = shufflevector <8 x float> %423, <8 x float> undef, <8 x i32> zeroinitializer
  %425 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %424, <8 x float> %322, <8 x float> %295)
  %426 = add nsw i64 %313, 144
  %427 = getelementptr inbounds float, float* %5, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !224
  %429 = insertelement <8 x float> undef, float %428, i32 0
  %430 = shufflevector <8 x float> %429, <8 x float> undef, <8 x i32> zeroinitializer
  %431 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %430, <8 x float> %322, <8 x float> %294)
  %432 = add nsw i64 %313, 152
  %433 = getelementptr inbounds float, float* %5, i64 %432
  %434 = load float, float* %433, align 4, !tbaa !224
  %435 = insertelement <8 x float> undef, float %434, i32 0
  %436 = shufflevector <8 x float> %435, <8 x float> undef, <8 x i32> zeroinitializer
  %437 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %436, <8 x float> %322, <8 x float> %293)
  %438 = add nsw i64 %313, 160
  %439 = getelementptr inbounds float, float* %5, i64 %438
  %440 = load float, float* %439, align 4, !tbaa !224
  %441 = insertelement <8 x float> undef, float %440, i32 0
  %442 = shufflevector <8 x float> %441, <8 x float> undef, <8 x i32> zeroinitializer
  %443 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %442, <8 x float> %322, <8 x float> %292)
  %444 = add nsw i64 %313, 168
  %445 = getelementptr inbounds float, float* %5, i64 %444
  %446 = load float, float* %445, align 4, !tbaa !224
  %447 = insertelement <8 x float> undef, float %446, i32 0
  %448 = shufflevector <8 x float> %447, <8 x float> undef, <8 x i32> zeroinitializer
  %449 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %448, <8 x float> %322, <8 x float> %291)
  %450 = add nsw i64 %313, 176
  %451 = getelementptr inbounds float, float* %5, i64 %450
  %452 = load float, float* %451, align 4, !tbaa !224
  %453 = insertelement <8 x float> undef, float %452, i32 0
  %454 = shufflevector <8 x float> %453, <8 x float> undef, <8 x i32> zeroinitializer
  %455 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %454, <8 x float> %322, <8 x float> %290)
  %456 = add nsw i64 %313, 184
  %457 = getelementptr inbounds float, float* %5, i64 %456
  %458 = load float, float* %457, align 4, !tbaa !224
  %459 = insertelement <8 x float> undef, float %458, i32 0
  %460 = shufflevector <8 x float> %459, <8 x float> undef, <8 x i32> zeroinitializer
  %461 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %460, <8 x float> %322, <8 x float> %289)
  %462 = add nsw i64 %313, 192
  %463 = getelementptr inbounds float, float* %5, i64 %462
  %464 = load float, float* %463, align 4, !tbaa !224
  %465 = insertelement <8 x float> undef, float %464, i32 0
  %466 = shufflevector <8 x float> %465, <8 x float> undef, <8 x i32> zeroinitializer
  %467 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %466, <8 x float> %322, <8 x float> %288)
  %468 = add nsw i64 %313, 200
  %469 = getelementptr inbounds float, float* %5, i64 %468
  %470 = load float, float* %469, align 4, !tbaa !224
  %471 = insertelement <8 x float> undef, float %470, i32 0
  %472 = shufflevector <8 x float> %471, <8 x float> undef, <8 x i32> zeroinitializer
  %473 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %472, <8 x float> %322, <8 x float> %287)
  %474 = add nsw i64 %313, 208
  %475 = getelementptr inbounds float, float* %5, i64 %474
  %476 = load float, float* %475, align 4, !tbaa !224
  %477 = insertelement <8 x float> undef, float %476, i32 0
  %478 = shufflevector <8 x float> %477, <8 x float> undef, <8 x i32> zeroinitializer
  %479 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %478, <8 x float> %322, <8 x float> %286)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end12, label %for_body11, !prof !28

for_end12:                                        ; preds = %for_body11
  %indvars.iv.next252 = add nuw nsw i64 %indvars.iv251, 1
  %exitcond253 = icmp eq i64 %indvars.iv.next252, 5
  br i1 %exitcond253, label %for_end9, label %for_body8, !prof !28
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !248 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !250, metadata !DIExpression()), !dbg !253
  call void @llvm.dbg.value(metadata i8* %1, metadata !251, metadata !DIExpression()), !dbg !253
  call void @llvm.dbg.value(metadata i32 %2, metadata !252, metadata !DIExpression()), !dbg !253
  %3 = bitcast i8* %0 to %1**, !dbg !253
  %4 = load %1*, %1** %3, align 8, !dbg !253
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !253
  %6 = bitcast i8* %5 to %1**, !dbg !253
  %7 = load %1*, %1** %6, align 8, !dbg !253
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !253
  %9 = bitcast i8* %8 to %1**, !dbg !253
  %10 = load %1*, %1** %9, align 8, !dbg !253
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !253
  %12 = bitcast i8* %11 to %1**, !dbg !253
  %13 = load %1*, %1** %12, align 8, !dbg !253
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !253
  %15 = load i8*, i8** %14, align 8, !dbg !253
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !253
  %17 = load i32, i32* %16, align 4, !dbg !253
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !253
  %19 = load i8*, i8** %18, align 8, !dbg !253
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !253
  %21 = load i8*, i8** %20, align 8, !dbg !253
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !253
  %23 = load i8*, i8** %22, align 8, !dbg !253
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !253
  ret i32 %24, !dbg !253
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 345600, i32 2, i32 32)
  %7 = alloca %18, align 8
  %8 = getelementptr inbounds %18, %18* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %18, %18* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %18* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.14, i8* nonnull %10, i32 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %21, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %19, align 8
  %15 = getelementptr inbounds %19, %19* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %19, %19* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %19, %19* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %19, %19* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = bitcast %19* %14 to i8*
  %20 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %21 = call i32 %20(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.15, i8* nonnull %19, i32 5)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %23 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %24 = call i32 %23(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.14(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 719
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 720
  %15 = select i1 %14, i32 %13, i32 720
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 720
  %18 = select i1 %17, i32 %16, i32 720
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %219, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 120
  %22 = srem i32 %20, 15
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 13
  %24 = sdiv i32 %20, 15
  %25 = mul nsw i32 %24, 1352
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 104
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 3
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 13
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 15
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !28

for_body2.split.us.us:                            ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -112
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to i32*
  %40 = load i32, i32* %39, align 4, !tbaa !254
  %41 = sext i32 %32 to i64
  %42 = getelementptr inbounds float, float* %4, i64 %41
  %43 = bitcast float* %42 to i32*
  store i32 %40, i32* %43, align 4, !tbaa !257
  %44 = or i64 %27, 1
  %45 = trunc i64 %44 to i32
  %46 = add i32 %21, %45
  %47 = trunc i64 %44 to i32
  %48 = add i32 %47, -112
  %49 = add i32 %26, %48
  %50 = add i32 %49, %25
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to i32*
  %54 = load i32, i32* %53, align 4, !tbaa !254
  %55 = sext i32 %46 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to i32*
  store i32 %54, i32* %57, align 4, !tbaa !257
  %58 = or i64 %27, 2
  %59 = trunc i64 %58 to i32
  %60 = add i32 %21, %59
  %61 = trunc i64 %58 to i32
  %62 = add i32 %61, -112
  %63 = add i32 %26, %62
  %64 = add i32 %63, %25
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to i32*
  %68 = load i32, i32* %67, align 4, !tbaa !254
  %69 = sext i32 %60 to i64
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = bitcast float* %70 to i32*
  store i32 %68, i32* %71, align 4, !tbaa !257
  %72 = or i64 %27, 3
  %73 = trunc i64 %72 to i32
  %74 = add i32 %21, %73
  %75 = trunc i64 %72 to i32
  %76 = add i32 %75, -112
  %77 = add i32 %26, %76
  %78 = add i32 %77, %25
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !254
  %83 = sext i32 %74 to i64
  %84 = getelementptr inbounds float, float* %4, i64 %83
  %85 = bitcast float* %84 to i32*
  store i32 %82, i32* %85, align 4, !tbaa !257
  %86 = or i64 %27, 4
  %87 = trunc i64 %86 to i32
  %88 = add i32 %21, %87
  %89 = trunc i64 %86 to i32
  %90 = add i32 %89, -112
  %91 = add i32 %26, %90
  %92 = add i32 %91, %25
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to i32*
  %96 = load i32, i32* %95, align 4, !tbaa !254
  %97 = sext i32 %88 to i64
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = bitcast float* %98 to i32*
  store i32 %96, i32* %99, align 4, !tbaa !257
  %100 = or i64 %27, 5
  %101 = trunc i64 %100 to i32
  %102 = add i32 %21, %101
  %103 = trunc i64 %100 to i32
  %104 = add i32 %103, -112
  %105 = add i32 %26, %104
  %106 = add i32 %105, %25
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to i32*
  %110 = load i32, i32* %109, align 4, !tbaa !254
  %111 = sext i32 %102 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to i32*
  store i32 %110, i32* %113, align 4, !tbaa !257
  %114 = or i64 %27, 6
  %115 = trunc i64 %114 to i32
  %116 = add i32 %21, %115
  %117 = trunc i64 %114 to i32
  %118 = add i32 %117, -112
  %119 = add i32 %26, %118
  %120 = add i32 %119, %25
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to i32*
  %124 = load i32, i32* %123, align 4, !tbaa !254
  %125 = sext i32 %116 to i64
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = bitcast float* %126 to i32*
  store i32 %124, i32* %127, align 4, !tbaa !257
  %128 = or i64 %27, 7
  %129 = trunc i64 %128 to i32
  %130 = add i32 %21, %129
  %131 = trunc i64 %128 to i32
  %132 = add i32 %131, -112
  %133 = add i32 %26, %132
  %134 = add i32 %133, %25
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %7, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !254
  %139 = sext i32 %130 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to i32*
  store i32 %138, i32* %141, align 4, !tbaa !257
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %142 = sext i32 %32 to i64
  %143 = getelementptr inbounds float, float* %4, i64 %142
  store float 0.000000e+00, float* %143, align 4, !tbaa !257
  %144 = trunc i64 %27 to i32
  %145 = or i32 %144, 1
  %146 = add i32 %145, %21
  %147 = sext i32 %146 to i64
  %148 = getelementptr inbounds float, float* %4, i64 %147
  store float 0.000000e+00, float* %148, align 4, !tbaa !257
  %149 = trunc i64 %27 to i32
  %150 = or i32 %149, 2
  %151 = add i32 %150, %21
  %152 = sext i32 %151 to i64
  %153 = getelementptr inbounds float, float* %4, i64 %152
  store float 0.000000e+00, float* %153, align 4, !tbaa !257
  %154 = trunc i64 %27 to i32
  %155 = or i32 %154, 3
  %156 = add i32 %155, %21
  %157 = sext i32 %156 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  store float 0.000000e+00, float* %158, align 4, !tbaa !257
  %159 = trunc i64 %27 to i32
  %160 = or i32 %159, 4
  %161 = add i32 %160, %21
  %162 = sext i32 %161 to i64
  %163 = getelementptr inbounds float, float* %4, i64 %162
  store float 0.000000e+00, float* %163, align 4, !tbaa !257
  %164 = trunc i64 %27 to i32
  %165 = or i32 %164, 5
  %166 = add i32 %165, %21
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  store float 0.000000e+00, float* %168, align 4, !tbaa !257
  %169 = trunc i64 %27 to i32
  %170 = or i32 %169, 6
  %171 = add i32 %170, %21
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds float, float* %4, i64 %172
  store float 0.000000e+00, float* %173, align 4, !tbaa !257
  %174 = trunc i64 %27 to i32
  %175 = or i32 %174, 7
  %176 = add i32 %175, %21
  %177 = sext i32 %176 to i64
  %178 = getelementptr inbounds float, float* %4, i64 %177
  store float 0.000000e+00, float* %178, align 4, !tbaa !257
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %179 = shl nsw i64 %indvars.iv, 3
  %180 = trunc i64 %179 to i32
  %181 = add i32 %21, %180
  %182 = sext i32 %181 to i64
  %183 = getelementptr inbounds float, float* %4, i64 %182
  store float 0.000000e+00, float* %183, align 4, !tbaa !257
  %184 = trunc i64 %179 to i32
  %185 = or i32 %184, 1
  %186 = add i32 %185, %21
  %187 = sext i32 %186 to i64
  %188 = getelementptr inbounds float, float* %4, i64 %187
  store float 0.000000e+00, float* %188, align 4, !tbaa !257
  %189 = trunc i64 %179 to i32
  %190 = or i32 %189, 2
  %191 = add i32 %190, %21
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %4, i64 %192
  store float 0.000000e+00, float* %193, align 4, !tbaa !257
  %194 = trunc i64 %179 to i32
  %195 = or i32 %194, 3
  %196 = add i32 %195, %21
  %197 = sext i32 %196 to i64
  %198 = getelementptr inbounds float, float* %4, i64 %197
  store float 0.000000e+00, float* %198, align 4, !tbaa !257
  %199 = trunc i64 %179 to i32
  %200 = or i32 %199, 4
  %201 = add i32 %200, %21
  %202 = sext i32 %201 to i64
  %203 = getelementptr inbounds float, float* %4, i64 %202
  store float 0.000000e+00, float* %203, align 4, !tbaa !257
  %204 = trunc i64 %179 to i32
  %205 = or i32 %204, 5
  %206 = add i32 %205, %21
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  store float 0.000000e+00, float* %208, align 4, !tbaa !257
  %209 = trunc i64 %179 to i32
  %210 = or i32 %209, 6
  %211 = add i32 %210, %21
  %212 = sext i32 %211 to i64
  %213 = getelementptr inbounds float, float* %4, i64 %212
  store float 0.000000e+00, float* %213, align 4, !tbaa !257
  %214 = trunc i64 %179 to i32
  %215 = or i32 %214, 7
  %216 = add i32 %215, %21
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds float, float* %4, i64 %217
  store float 0.000000e+00, float* %218, align 4, !tbaa !257
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 15
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %219 = add nsw i32 %20, 1
  %220 = icmp slt i32 %219, %15
  br i1 %220, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.15(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds i8, i8* %2, i64 16
  %9 = bitcast i8* %8 to float**
  %10 = load float*, float** %9, align 8
  %11 = getelementptr inbounds i8, i8* %2, i64 24
  %12 = bitcast i8* %11 to float**
  %13 = load float*, float** %12, align 8
  %14 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %15 = load i32, i32* %14, align 4
  %16 = add nsw i32 %15, 415
  %17 = sdiv i32 %16, %15
  %18 = add nsw i32 %0, 1
  %19 = mul nsw i32 %17, %18
  %20 = icmp slt i32 %19, 416
  %21 = select i1 %20, i32 %19, i32 416
  %22 = mul nsw i32 %17, %0
  %23 = icmp slt i32 %22, 416
  %24 = select i1 %23, i32 %22, i32 416
  %25 = icmp slt i32 %24, %21
  br i1 %25, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %26 = add i32 %24, 1
  %27 = sext i32 %26 to i64
  %28 = add nsw i64 %27, -1
  %29 = sext i32 %21 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv138 = phi i64 [ %28, %for_body.lr.ph ], [ %indvars.iv.next139, %for_end3 ]
  %30 = trunc i64 %indvars.iv138 to i32
  %31 = srem i32 %30, 13
  %32 = sdiv i32 %30, 13
  %33 = mul nsw i32 %32, 27648
  %34 = sext i32 %33 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv132 = phi i64 [ 0, %for_body ], [ %indvars.iv.next133, %for_end6 ]
  %.lcssa38.lcssa.lcssa112 = phi <8 x float> [ zeroinitializer, %for_body ], [ %419, %for_end6 ]
  %.lcssa36.lcssa.lcssa110 = phi <8 x float> [ zeroinitializer, %for_body ], [ %413, %for_end6 ]
  %.lcssa34.lcssa.lcssa108 = phi <8 x float> [ zeroinitializer, %for_body ], [ %407, %for_end6 ]
  %.lcssa32.lcssa.lcssa106 = phi <8 x float> [ zeroinitializer, %for_body ], [ %401, %for_end6 ]
  %.lcssa30.lcssa.lcssa104 = phi <8 x float> [ zeroinitializer, %for_body ], [ %395, %for_end6 ]
  %.lcssa28.lcssa.lcssa102 = phi <8 x float> [ zeroinitializer, %for_body ], [ %389, %for_end6 ]
  %.lcssa26.lcssa.lcssa100 = phi <8 x float> [ zeroinitializer, %for_body ], [ %383, %for_end6 ]
  %.lcssa24.lcssa.lcssa98 = phi <8 x float> [ zeroinitializer, %for_body ], [ %377, %for_end6 ]
  %.lcssa22.lcssa.lcssa96 = phi <8 x float> [ zeroinitializer, %for_body ], [ %371, %for_end6 ]
  %.lcssa20.lcssa.lcssa95 = phi <8 x float> [ zeroinitializer, %for_body ], [ %365, %for_end6 ]
  %.lcssa18.lcssa.lcssa93 = phi <8 x float> [ zeroinitializer, %for_body ], [ %359, %for_end6 ]
  %.lcssa16.lcssa.lcssa91 = phi <8 x float> [ zeroinitializer, %for_body ], [ %353, %for_end6 ]
  %.lcssa.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body ], [ %347, %for_end6 ]
  %35 = mul nuw nsw i64 %indvars.iv132, 576
  %36 = add nsw i64 %35, %34
  %37 = trunc i64 %indvars.iv132 to i32
  %38 = mul i32 %37, 1800
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %39 = mul nsw i64 %indvars.iv138, 104
  %40 = shl nsw i32 %32, 3
  %41 = sext i32 %40 to i64
  %42 = getelementptr inbounds float, float* %13, i64 %41
  %43 = bitcast float* %42 to <8 x float>*
  %44 = load <8 x float>, <8 x float>* %43, align 32, !tbaa !260
  %45 = fadd <8 x float> %44, %347
  %46 = fcmp ogt <8 x float> %45, zeroinitializer
  %47 = select <8 x i1> %46, <8 x float> %45, <8 x float> zeroinitializer
  %48 = getelementptr inbounds float, float* %10, i64 %39
  %49 = bitcast float* %48 to <8 x float>*
  store <8 x float> %47, <8 x float>* %49, align 32, !tbaa !263
  %50 = add nsw i64 %39, 8
  %51 = fadd <8 x float> %44, %353
  %52 = fcmp ogt <8 x float> %51, zeroinitializer
  %53 = select <8 x i1> %52, <8 x float> %51, <8 x float> zeroinitializer
  %54 = getelementptr inbounds float, float* %10, i64 %50
  %55 = bitcast float* %54 to <8 x float>*
  store <8 x float> %53, <8 x float>* %55, align 32, !tbaa !263
  %56 = add nsw i64 %39, 16
  %57 = fadd <8 x float> %44, %359
  %58 = fcmp ogt <8 x float> %57, zeroinitializer
  %59 = select <8 x i1> %58, <8 x float> %57, <8 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %10, i64 %56
  %61 = bitcast float* %60 to <8 x float>*
  store <8 x float> %59, <8 x float>* %61, align 32, !tbaa !263
  %62 = add nsw i64 %39, 24
  %63 = fadd <8 x float> %44, %365
  %64 = fcmp ogt <8 x float> %63, zeroinitializer
  %65 = select <8 x i1> %64, <8 x float> %63, <8 x float> zeroinitializer
  %66 = getelementptr inbounds float, float* %10, i64 %62
  %67 = bitcast float* %66 to <8 x float>*
  store <8 x float> %65, <8 x float>* %67, align 32, !tbaa !263
  %68 = add nsw i64 %39, 32
  %69 = fadd <8 x float> %44, %371
  %70 = fcmp ogt <8 x float> %69, zeroinitializer
  %71 = select <8 x i1> %70, <8 x float> %69, <8 x float> zeroinitializer
  %72 = getelementptr inbounds float, float* %10, i64 %68
  %73 = bitcast float* %72 to <8 x float>*
  store <8 x float> %71, <8 x float>* %73, align 32, !tbaa !263
  %74 = add nsw i64 %39, 40
  %75 = fadd <8 x float> %44, %377
  %76 = fcmp ogt <8 x float> %75, zeroinitializer
  %77 = select <8 x i1> %76, <8 x float> %75, <8 x float> zeroinitializer
  %78 = getelementptr inbounds float, float* %10, i64 %74
  %79 = bitcast float* %78 to <8 x float>*
  store <8 x float> %77, <8 x float>* %79, align 32, !tbaa !263
  %80 = add nsw i64 %39, 48
  %81 = fadd <8 x float> %44, %383
  %82 = fcmp ogt <8 x float> %81, zeroinitializer
  %83 = select <8 x i1> %82, <8 x float> %81, <8 x float> zeroinitializer
  %84 = getelementptr inbounds float, float* %10, i64 %80
  %85 = bitcast float* %84 to <8 x float>*
  store <8 x float> %83, <8 x float>* %85, align 32, !tbaa !263
  %86 = add nsw i64 %39, 56
  %87 = fadd <8 x float> %44, %389
  %88 = fcmp ogt <8 x float> %87, zeroinitializer
  %89 = select <8 x i1> %88, <8 x float> %87, <8 x float> zeroinitializer
  %90 = getelementptr inbounds float, float* %10, i64 %86
  %91 = bitcast float* %90 to <8 x float>*
  store <8 x float> %89, <8 x float>* %91, align 32, !tbaa !263
  %92 = add nsw i64 %39, 64
  %93 = fadd <8 x float> %44, %395
  %94 = fcmp ogt <8 x float> %93, zeroinitializer
  %95 = select <8 x i1> %94, <8 x float> %93, <8 x float> zeroinitializer
  %96 = getelementptr inbounds float, float* %10, i64 %92
  %97 = bitcast float* %96 to <8 x float>*
  store <8 x float> %95, <8 x float>* %97, align 32, !tbaa !263
  %98 = add nsw i64 %39, 72
  %99 = fadd <8 x float> %44, %401
  %100 = fcmp ogt <8 x float> %99, zeroinitializer
  %101 = select <8 x i1> %100, <8 x float> %99, <8 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %10, i64 %98
  %103 = bitcast float* %102 to <8 x float>*
  store <8 x float> %101, <8 x float>* %103, align 32, !tbaa !263
  %104 = add nsw i64 %39, 80
  %105 = fadd <8 x float> %44, %407
  %106 = fcmp ogt <8 x float> %105, zeroinitializer
  %107 = select <8 x i1> %106, <8 x float> %105, <8 x float> zeroinitializer
  %108 = getelementptr inbounds float, float* %10, i64 %104
  %109 = bitcast float* %108 to <8 x float>*
  store <8 x float> %107, <8 x float>* %109, align 32, !tbaa !263
  %110 = add nsw i64 %39, 88
  %111 = fadd <8 x float> %44, %413
  %112 = fcmp ogt <8 x float> %111, zeroinitializer
  %113 = select <8 x i1> %112, <8 x float> %111, <8 x float> zeroinitializer
  %114 = getelementptr inbounds float, float* %10, i64 %110
  %115 = bitcast float* %114 to <8 x float>*
  store <8 x float> %113, <8 x float>* %115, align 32, !tbaa !263
  %116 = add nsw i64 %39, 96
  %117 = fadd <8 x float> %44, %419
  %118 = fcmp ogt <8 x float> %117, zeroinitializer
  %119 = select <8 x i1> %118, <8 x float> %117, <8 x float> zeroinitializer
  %120 = getelementptr inbounds float, float* %10, i64 %116
  %121 = bitcast float* %120 to <8 x float>*
  store <8 x float> %119, <8 x float>* %121, align 32, !tbaa !263
  %indvars.iv.next139 = add nsw i64 %indvars.iv138, 1
  %122 = icmp slt i64 %indvars.iv.next139, %29
  br i1 %122, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_end12.2, %for_body2
  %indvars.iv128 = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next129, %for_end12.2 ]
  %.lcssa38.lcssa87 = phi <8 x float> [ %.lcssa38.lcssa.lcssa112, %for_body2 ], [ %419, %for_end12.2 ]
  %.lcssa36.lcssa85 = phi <8 x float> [ %.lcssa36.lcssa.lcssa110, %for_body2 ], [ %413, %for_end12.2 ]
  %.lcssa34.lcssa83 = phi <8 x float> [ %.lcssa34.lcssa.lcssa108, %for_body2 ], [ %407, %for_end12.2 ]
  %.lcssa32.lcssa81 = phi <8 x float> [ %.lcssa32.lcssa.lcssa106, %for_body2 ], [ %401, %for_end12.2 ]
  %.lcssa30.lcssa79 = phi <8 x float> [ %.lcssa30.lcssa.lcssa104, %for_body2 ], [ %395, %for_end12.2 ]
  %.lcssa28.lcssa77 = phi <8 x float> [ %.lcssa28.lcssa.lcssa102, %for_body2 ], [ %389, %for_end12.2 ]
  %.lcssa26.lcssa75 = phi <8 x float> [ %.lcssa26.lcssa.lcssa100, %for_body2 ], [ %383, %for_end12.2 ]
  %.lcssa24.lcssa73 = phi <8 x float> [ %.lcssa24.lcssa.lcssa98, %for_body2 ], [ %377, %for_end12.2 ]
  %.lcssa22.lcssa71 = phi <8 x float> [ %.lcssa22.lcssa.lcssa96, %for_body2 ], [ %371, %for_end12.2 ]
  %.lcssa20.lcssa69 = phi <8 x float> [ %.lcssa20.lcssa.lcssa95, %for_body2 ], [ %365, %for_end12.2 ]
  %.lcssa18.lcssa68 = phi <8 x float> [ %.lcssa18.lcssa.lcssa93, %for_body2 ], [ %359, %for_end12.2 ]
  %.lcssa16.lcssa66 = phi <8 x float> [ %.lcssa16.lcssa.lcssa91, %for_body2 ], [ %353, %for_end12.2 ]
  %.lcssa.lcssa64 = phi <8 x float> [ %.lcssa.lcssa.lcssa89, %for_body2 ], [ %347, %for_end12.2 ]
  %123 = phi i32 [ 0, %for_body2 ], [ %420, %for_end12.2 ]
  %reass.add = add nsw i32 %123, %31
  %reass.mul = mul i32 %reass.add, 120
  %124 = add nsw i32 %reass.mul, %38
  %125 = mul nuw nsw i64 %indvars.iv128, 192
  %126 = add nsw i64 %36, %125
  %127 = sext i32 %124 to i64
  br label %for_body11

for_end6:                                         ; preds = %for_end12.2
  %indvars.iv.next133 = add nuw nsw i64 %indvars.iv132, 1
  %exitcond134 = icmp eq i64 %indvars.iv.next133, 48
  br i1 %exitcond134, label %for_end3, label %for_body2, !prof !28

for_body11:                                       ; preds = %for_body11, %for_body5
  %indvars.iv = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next, %for_body11 ]
  %128 = phi <8 x float> [ %.lcssa38.lcssa87, %for_body5 ], [ %223, %for_body11 ]
  %129 = phi <8 x float> [ %.lcssa36.lcssa85, %for_body5 ], [ %217, %for_body11 ]
  %130 = phi <8 x float> [ %.lcssa34.lcssa83, %for_body5 ], [ %211, %for_body11 ]
  %131 = phi <8 x float> [ %.lcssa32.lcssa81, %for_body5 ], [ %205, %for_body11 ]
  %132 = phi <8 x float> [ %.lcssa30.lcssa79, %for_body5 ], [ %199, %for_body11 ]
  %133 = phi <8 x float> [ %.lcssa28.lcssa77, %for_body5 ], [ %193, %for_body11 ]
  %134 = phi <8 x float> [ %.lcssa26.lcssa75, %for_body5 ], [ %187, %for_body11 ]
  %135 = phi <8 x float> [ %.lcssa24.lcssa73, %for_body5 ], [ %181, %for_body11 ]
  %136 = phi <8 x float> [ %.lcssa22.lcssa71, %for_body5 ], [ %175, %for_body11 ]
  %137 = phi <8 x float> [ %.lcssa20.lcssa69, %for_body5 ], [ %169, %for_body11 ]
  %138 = phi <8 x float> [ %.lcssa18.lcssa68, %for_body5 ], [ %163, %for_body11 ]
  %139 = phi <8 x float> [ %.lcssa16.lcssa66, %for_body5 ], [ %157, %for_body11 ]
  %140 = phi <8 x float> [ %.lcssa.lcssa64, %for_body5 ], [ %151, %for_body11 ]
  %141 = add nsw i64 %indvars.iv, %127
  %142 = getelementptr inbounds float, float* %4, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !257
  %144 = insertelement <8 x float> undef, float %143, i32 0
  %145 = shufflevector <8 x float> %144, <8 x float> undef, <8 x i32> zeroinitializer
  %146 = shl i64 %indvars.iv, 3
  %147 = add nsw i64 %126, %146
  %148 = getelementptr inbounds float, float* %7, i64 %147
  %149 = bitcast float* %148 to <8 x float>*
  %150 = load <8 x float>, <8 x float>* %149, align 32, !tbaa !266
  %151 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %145, <8 x float> %150, <8 x float> %140)
  %152 = add nsw i64 %141, 8
  %153 = getelementptr inbounds float, float* %4, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !257
  %155 = insertelement <8 x float> undef, float %154, i32 0
  %156 = shufflevector <8 x float> %155, <8 x float> undef, <8 x i32> zeroinitializer
  %157 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %156, <8 x float> %150, <8 x float> %139)
  %158 = add nsw i64 %141, 16
  %159 = getelementptr inbounds float, float* %4, i64 %158
  %160 = load float, float* %159, align 4, !tbaa !257
  %161 = insertelement <8 x float> undef, float %160, i32 0
  %162 = shufflevector <8 x float> %161, <8 x float> undef, <8 x i32> zeroinitializer
  %163 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %162, <8 x float> %150, <8 x float> %138)
  %164 = add nsw i64 %141, 24
  %165 = getelementptr inbounds float, float* %4, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !257
  %167 = insertelement <8 x float> undef, float %166, i32 0
  %168 = shufflevector <8 x float> %167, <8 x float> undef, <8 x i32> zeroinitializer
  %169 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %168, <8 x float> %150, <8 x float> %137)
  %170 = add nsw i64 %141, 32
  %171 = getelementptr inbounds float, float* %4, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !257
  %173 = insertelement <8 x float> undef, float %172, i32 0
  %174 = shufflevector <8 x float> %173, <8 x float> undef, <8 x i32> zeroinitializer
  %175 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %174, <8 x float> %150, <8 x float> %136)
  %176 = add nsw i64 %141, 40
  %177 = getelementptr inbounds float, float* %4, i64 %176
  %178 = load float, float* %177, align 4, !tbaa !257
  %179 = insertelement <8 x float> undef, float %178, i32 0
  %180 = shufflevector <8 x float> %179, <8 x float> undef, <8 x i32> zeroinitializer
  %181 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %180, <8 x float> %150, <8 x float> %135)
  %182 = add nsw i64 %141, 48
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !257
  %185 = insertelement <8 x float> undef, float %184, i32 0
  %186 = shufflevector <8 x float> %185, <8 x float> undef, <8 x i32> zeroinitializer
  %187 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %186, <8 x float> %150, <8 x float> %134)
  %188 = add nsw i64 %141, 56
  %189 = getelementptr inbounds float, float* %4, i64 %188
  %190 = load float, float* %189, align 4, !tbaa !257
  %191 = insertelement <8 x float> undef, float %190, i32 0
  %192 = shufflevector <8 x float> %191, <8 x float> undef, <8 x i32> zeroinitializer
  %193 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %192, <8 x float> %150, <8 x float> %133)
  %194 = add nsw i64 %141, 64
  %195 = getelementptr inbounds float, float* %4, i64 %194
  %196 = load float, float* %195, align 4, !tbaa !257
  %197 = insertelement <8 x float> undef, float %196, i32 0
  %198 = shufflevector <8 x float> %197, <8 x float> undef, <8 x i32> zeroinitializer
  %199 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %198, <8 x float> %150, <8 x float> %132)
  %200 = add nsw i64 %141, 72
  %201 = getelementptr inbounds float, float* %4, i64 %200
  %202 = load float, float* %201, align 4, !tbaa !257
  %203 = insertelement <8 x float> undef, float %202, i32 0
  %204 = shufflevector <8 x float> %203, <8 x float> undef, <8 x i32> zeroinitializer
  %205 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %204, <8 x float> %150, <8 x float> %131)
  %206 = add nsw i64 %141, 80
  %207 = getelementptr inbounds float, float* %4, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !257
  %209 = insertelement <8 x float> undef, float %208, i32 0
  %210 = shufflevector <8 x float> %209, <8 x float> undef, <8 x i32> zeroinitializer
  %211 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %210, <8 x float> %150, <8 x float> %130)
  %212 = add nsw i64 %141, 88
  %213 = getelementptr inbounds float, float* %4, i64 %212
  %214 = load float, float* %213, align 4, !tbaa !257
  %215 = insertelement <8 x float> undef, float %214, i32 0
  %216 = shufflevector <8 x float> %215, <8 x float> undef, <8 x i32> zeroinitializer
  %217 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %216, <8 x float> %150, <8 x float> %129)
  %218 = add nsw i64 %141, 96
  %219 = getelementptr inbounds float, float* %4, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !257
  %221 = insertelement <8 x float> undef, float %220, i32 0
  %222 = shufflevector <8 x float> %221, <8 x float> undef, <8 x i32> zeroinitializer
  %223 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %222, <8 x float> %150, <8 x float> %128)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end12, label %for_body11, !prof !28

for_end12:                                        ; preds = %for_body11
  %224 = add nsw i64 %127, 8
  %225 = add nsw i64 %126, 64
  br label %for_body11.1

for_body11.1:                                     ; preds = %for_body11.1, %for_end12
  %indvars.iv.1 = phi i64 [ 0, %for_end12 ], [ %indvars.iv.next.1, %for_body11.1 ]
  %226 = phi <8 x float> [ %223, %for_end12 ], [ %321, %for_body11.1 ]
  %227 = phi <8 x float> [ %217, %for_end12 ], [ %315, %for_body11.1 ]
  %228 = phi <8 x float> [ %211, %for_end12 ], [ %309, %for_body11.1 ]
  %229 = phi <8 x float> [ %205, %for_end12 ], [ %303, %for_body11.1 ]
  %230 = phi <8 x float> [ %199, %for_end12 ], [ %297, %for_body11.1 ]
  %231 = phi <8 x float> [ %193, %for_end12 ], [ %291, %for_body11.1 ]
  %232 = phi <8 x float> [ %187, %for_end12 ], [ %285, %for_body11.1 ]
  %233 = phi <8 x float> [ %181, %for_end12 ], [ %279, %for_body11.1 ]
  %234 = phi <8 x float> [ %175, %for_end12 ], [ %273, %for_body11.1 ]
  %235 = phi <8 x float> [ %169, %for_end12 ], [ %267, %for_body11.1 ]
  %236 = phi <8 x float> [ %163, %for_end12 ], [ %261, %for_body11.1 ]
  %237 = phi <8 x float> [ %157, %for_end12 ], [ %255, %for_body11.1 ]
  %238 = phi <8 x float> [ %151, %for_end12 ], [ %249, %for_body11.1 ]
  %239 = add nsw i64 %224, %indvars.iv.1
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !257
  %242 = insertelement <8 x float> undef, float %241, i32 0
  %243 = shufflevector <8 x float> %242, <8 x float> undef, <8 x i32> zeroinitializer
  %244 = shl i64 %indvars.iv.1, 3
  %245 = add nsw i64 %225, %244
  %246 = getelementptr inbounds float, float* %7, i64 %245
  %247 = bitcast float* %246 to <8 x float>*
  %248 = load <8 x float>, <8 x float>* %247, align 32, !tbaa !266
  %249 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %243, <8 x float> %248, <8 x float> %238)
  %250 = add nsw i64 %239, 8
  %251 = getelementptr inbounds float, float* %4, i64 %250
  %252 = load float, float* %251, align 4, !tbaa !257
  %253 = insertelement <8 x float> undef, float %252, i32 0
  %254 = shufflevector <8 x float> %253, <8 x float> undef, <8 x i32> zeroinitializer
  %255 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %254, <8 x float> %248, <8 x float> %237)
  %256 = add nsw i64 %239, 16
  %257 = getelementptr inbounds float, float* %4, i64 %256
  %258 = load float, float* %257, align 4, !tbaa !257
  %259 = insertelement <8 x float> undef, float %258, i32 0
  %260 = shufflevector <8 x float> %259, <8 x float> undef, <8 x i32> zeroinitializer
  %261 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %260, <8 x float> %248, <8 x float> %236)
  %262 = add nsw i64 %239, 24
  %263 = getelementptr inbounds float, float* %4, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !257
  %265 = insertelement <8 x float> undef, float %264, i32 0
  %266 = shufflevector <8 x float> %265, <8 x float> undef, <8 x i32> zeroinitializer
  %267 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %266, <8 x float> %248, <8 x float> %235)
  %268 = add nsw i64 %239, 32
  %269 = getelementptr inbounds float, float* %4, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !257
  %271 = insertelement <8 x float> undef, float %270, i32 0
  %272 = shufflevector <8 x float> %271, <8 x float> undef, <8 x i32> zeroinitializer
  %273 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %272, <8 x float> %248, <8 x float> %234)
  %274 = add nsw i64 %239, 40
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !257
  %277 = insertelement <8 x float> undef, float %276, i32 0
  %278 = shufflevector <8 x float> %277, <8 x float> undef, <8 x i32> zeroinitializer
  %279 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %278, <8 x float> %248, <8 x float> %233)
  %280 = add nsw i64 %239, 48
  %281 = getelementptr inbounds float, float* %4, i64 %280
  %282 = load float, float* %281, align 4, !tbaa !257
  %283 = insertelement <8 x float> undef, float %282, i32 0
  %284 = shufflevector <8 x float> %283, <8 x float> undef, <8 x i32> zeroinitializer
  %285 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %284, <8 x float> %248, <8 x float> %232)
  %286 = add nsw i64 %239, 56
  %287 = getelementptr inbounds float, float* %4, i64 %286
  %288 = load float, float* %287, align 4, !tbaa !257
  %289 = insertelement <8 x float> undef, float %288, i32 0
  %290 = shufflevector <8 x float> %289, <8 x float> undef, <8 x i32> zeroinitializer
  %291 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %290, <8 x float> %248, <8 x float> %231)
  %292 = add nsw i64 %239, 64
  %293 = getelementptr inbounds float, float* %4, i64 %292
  %294 = load float, float* %293, align 4, !tbaa !257
  %295 = insertelement <8 x float> undef, float %294, i32 0
  %296 = shufflevector <8 x float> %295, <8 x float> undef, <8 x i32> zeroinitializer
  %297 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %296, <8 x float> %248, <8 x float> %230)
  %298 = add nsw i64 %239, 72
  %299 = getelementptr inbounds float, float* %4, i64 %298
  %300 = load float, float* %299, align 4, !tbaa !257
  %301 = insertelement <8 x float> undef, float %300, i32 0
  %302 = shufflevector <8 x float> %301, <8 x float> undef, <8 x i32> zeroinitializer
  %303 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %302, <8 x float> %248, <8 x float> %229)
  %304 = add nsw i64 %239, 80
  %305 = getelementptr inbounds float, float* %4, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !257
  %307 = insertelement <8 x float> undef, float %306, i32 0
  %308 = shufflevector <8 x float> %307, <8 x float> undef, <8 x i32> zeroinitializer
  %309 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %308, <8 x float> %248, <8 x float> %228)
  %310 = add nsw i64 %239, 88
  %311 = getelementptr inbounds float, float* %4, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !257
  %313 = insertelement <8 x float> undef, float %312, i32 0
  %314 = shufflevector <8 x float> %313, <8 x float> undef, <8 x i32> zeroinitializer
  %315 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %314, <8 x float> %248, <8 x float> %227)
  %316 = add nsw i64 %239, 96
  %317 = getelementptr inbounds float, float* %4, i64 %316
  %318 = load float, float* %317, align 4, !tbaa !257
  %319 = insertelement <8 x float> undef, float %318, i32 0
  %320 = shufflevector <8 x float> %319, <8 x float> undef, <8 x i32> zeroinitializer
  %321 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %320, <8 x float> %248, <8 x float> %226)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 8
  br i1 %exitcond.1, label %for_end12.1, label %for_body11.1, !prof !28

for_end12.1:                                      ; preds = %for_body11.1
  %322 = add nsw i64 %127, 16
  %323 = add nsw i64 %126, 128
  br label %for_body11.2

for_body11.2:                                     ; preds = %for_body11.2, %for_end12.1
  %indvars.iv.2 = phi i64 [ 0, %for_end12.1 ], [ %indvars.iv.next.2, %for_body11.2 ]
  %324 = phi <8 x float> [ %321, %for_end12.1 ], [ %419, %for_body11.2 ]
  %325 = phi <8 x float> [ %315, %for_end12.1 ], [ %413, %for_body11.2 ]
  %326 = phi <8 x float> [ %309, %for_end12.1 ], [ %407, %for_body11.2 ]
  %327 = phi <8 x float> [ %303, %for_end12.1 ], [ %401, %for_body11.2 ]
  %328 = phi <8 x float> [ %297, %for_end12.1 ], [ %395, %for_body11.2 ]
  %329 = phi <8 x float> [ %291, %for_end12.1 ], [ %389, %for_body11.2 ]
  %330 = phi <8 x float> [ %285, %for_end12.1 ], [ %383, %for_body11.2 ]
  %331 = phi <8 x float> [ %279, %for_end12.1 ], [ %377, %for_body11.2 ]
  %332 = phi <8 x float> [ %273, %for_end12.1 ], [ %371, %for_body11.2 ]
  %333 = phi <8 x float> [ %267, %for_end12.1 ], [ %365, %for_body11.2 ]
  %334 = phi <8 x float> [ %261, %for_end12.1 ], [ %359, %for_body11.2 ]
  %335 = phi <8 x float> [ %255, %for_end12.1 ], [ %353, %for_body11.2 ]
  %336 = phi <8 x float> [ %249, %for_end12.1 ], [ %347, %for_body11.2 ]
  %337 = add nsw i64 %322, %indvars.iv.2
  %338 = getelementptr inbounds float, float* %4, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !257
  %340 = insertelement <8 x float> undef, float %339, i32 0
  %341 = shufflevector <8 x float> %340, <8 x float> undef, <8 x i32> zeroinitializer
  %342 = shl i64 %indvars.iv.2, 3
  %343 = add nsw i64 %323, %342
  %344 = getelementptr inbounds float, float* %7, i64 %343
  %345 = bitcast float* %344 to <8 x float>*
  %346 = load <8 x float>, <8 x float>* %345, align 32, !tbaa !266
  %347 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %341, <8 x float> %346, <8 x float> %336)
  %348 = add nsw i64 %337, 8
  %349 = getelementptr inbounds float, float* %4, i64 %348
  %350 = load float, float* %349, align 4, !tbaa !257
  %351 = insertelement <8 x float> undef, float %350, i32 0
  %352 = shufflevector <8 x float> %351, <8 x float> undef, <8 x i32> zeroinitializer
  %353 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %352, <8 x float> %346, <8 x float> %335)
  %354 = add nsw i64 %337, 16
  %355 = getelementptr inbounds float, float* %4, i64 %354
  %356 = load float, float* %355, align 4, !tbaa !257
  %357 = insertelement <8 x float> undef, float %356, i32 0
  %358 = shufflevector <8 x float> %357, <8 x float> undef, <8 x i32> zeroinitializer
  %359 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %358, <8 x float> %346, <8 x float> %334)
  %360 = add nsw i64 %337, 24
  %361 = getelementptr inbounds float, float* %4, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !257
  %363 = insertelement <8 x float> undef, float %362, i32 0
  %364 = shufflevector <8 x float> %363, <8 x float> undef, <8 x i32> zeroinitializer
  %365 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %364, <8 x float> %346, <8 x float> %333)
  %366 = add nsw i64 %337, 32
  %367 = getelementptr inbounds float, float* %4, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !257
  %369 = insertelement <8 x float> undef, float %368, i32 0
  %370 = shufflevector <8 x float> %369, <8 x float> undef, <8 x i32> zeroinitializer
  %371 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %370, <8 x float> %346, <8 x float> %332)
  %372 = add nsw i64 %337, 40
  %373 = getelementptr inbounds float, float* %4, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !257
  %375 = insertelement <8 x float> undef, float %374, i32 0
  %376 = shufflevector <8 x float> %375, <8 x float> undef, <8 x i32> zeroinitializer
  %377 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %376, <8 x float> %346, <8 x float> %331)
  %378 = add nsw i64 %337, 48
  %379 = getelementptr inbounds float, float* %4, i64 %378
  %380 = load float, float* %379, align 4, !tbaa !257
  %381 = insertelement <8 x float> undef, float %380, i32 0
  %382 = shufflevector <8 x float> %381, <8 x float> undef, <8 x i32> zeroinitializer
  %383 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %382, <8 x float> %346, <8 x float> %330)
  %384 = add nsw i64 %337, 56
  %385 = getelementptr inbounds float, float* %4, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !257
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shufflevector <8 x float> %387, <8 x float> undef, <8 x i32> zeroinitializer
  %389 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %388, <8 x float> %346, <8 x float> %329)
  %390 = add nsw i64 %337, 64
  %391 = getelementptr inbounds float, float* %4, i64 %390
  %392 = load float, float* %391, align 4, !tbaa !257
  %393 = insertelement <8 x float> undef, float %392, i32 0
  %394 = shufflevector <8 x float> %393, <8 x float> undef, <8 x i32> zeroinitializer
  %395 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %394, <8 x float> %346, <8 x float> %328)
  %396 = add nsw i64 %337, 72
  %397 = getelementptr inbounds float, float* %4, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !257
  %399 = insertelement <8 x float> undef, float %398, i32 0
  %400 = shufflevector <8 x float> %399, <8 x float> undef, <8 x i32> zeroinitializer
  %401 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %400, <8 x float> %346, <8 x float> %327)
  %402 = add nsw i64 %337, 80
  %403 = getelementptr inbounds float, float* %4, i64 %402
  %404 = load float, float* %403, align 4, !tbaa !257
  %405 = insertelement <8 x float> undef, float %404, i32 0
  %406 = shufflevector <8 x float> %405, <8 x float> undef, <8 x i32> zeroinitializer
  %407 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %406, <8 x float> %346, <8 x float> %326)
  %408 = add nsw i64 %337, 88
  %409 = getelementptr inbounds float, float* %4, i64 %408
  %410 = load float, float* %409, align 4, !tbaa !257
  %411 = insertelement <8 x float> undef, float %410, i32 0
  %412 = shufflevector <8 x float> %411, <8 x float> undef, <8 x i32> zeroinitializer
  %413 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %412, <8 x float> %346, <8 x float> %325)
  %414 = add nsw i64 %337, 96
  %415 = getelementptr inbounds float, float* %4, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !257
  %417 = insertelement <8 x float> undef, float %416, i32 0
  %418 = shufflevector <8 x float> %417, <8 x float> undef, <8 x i32> zeroinitializer
  %419 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %418, <8 x float> %346, <8 x float> %324)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 8
  br i1 %exitcond.2, label %for_end12.2, label %for_body11.2, !prof !28

for_end12.2:                                      ; preds = %for_body11.2
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %420 = add nuw nsw i32 %123, 1
  %exitcond131 = icmp eq i64 %indvars.iv.next129, 3
  br i1 %exitcond131, label %for_end6, label %for_body5, !prof !28
}

define dllexport i32 @fused_nn_max_pool2d_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !269 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !271, metadata !DIExpression()), !dbg !274
  call void @llvm.dbg.value(metadata i8* %1, metadata !272, metadata !DIExpression()), !dbg !274
  call void @llvm.dbg.value(metadata i32 %2, metadata !273, metadata !DIExpression()), !dbg !274
  %3 = bitcast i8* %0 to %1**, !dbg !274
  %4 = load %1*, %1** %3, align 8, !dbg !274
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !274
  %6 = bitcast i8* %5 to %1**, !dbg !274
  %7 = load %1*, %1** %6, align 8, !dbg !274
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !274
  %9 = load i8*, i8** %8, align 8, !dbg !274
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !274
  %11 = load i8*, i8** %10, align 8, !dbg !274
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_1_compute_(i8* %11, i8* %9), !dbg !274
  ret i32 %12, !dbg !274
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_1_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %20, align 8
  %3 = getelementptr inbounds %20, %20* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %20, %20* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %20* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.16, i8* nonnull %5, i32 5)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.16(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 311
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 312
  %15 = select i1 %14, i32 %13, i32 312
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 312
  %18 = select i1 %17, i32 %16, i32 312
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv7 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next8, %for_end3 ]
  %24 = mul nsw i64 %indvars.iv7, 104
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = srem i32 %25, 13
  %27 = mul nsw i32 %26, 432
  %28 = sdiv i32 %25, 13
  %29 = mul nsw i32 %28, 5832
  %30 = add nsw i32 %29, %27
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %31 = shl i64 %indvars.iv, 3
  %32 = add nsw i64 %31, %24
  %33 = getelementptr inbounds float, float* %4, i64 %32
  %34 = bitcast float* %33 to <8 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %35 = shl i32 %indvars.iv.tr, 4
  %36 = add i32 %30, %35
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to <8 x float>*
  %40 = load <8 x float>, <8 x float>* %39, align 32, !tbaa !275
  %41 = fcmp olt <8 x float> %40, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %42 = select <8 x i1> %41, <8 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <8 x float> %40
  %43 = add i32 %36, 8
  %44 = sext i32 %43 to i64
  %45 = getelementptr inbounds float, float* %7, i64 %44
  %46 = bitcast float* %45 to <8 x float>*
  %47 = load <8 x float>, <8 x float>* %46, align 32, !tbaa !275
  %48 = fcmp ogt <8 x float> %42, %47
  %49 = select <8 x i1> %48, <8 x float> %42, <8 x float> %47
  %50 = add i32 %36, 16
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to <8 x float>*
  %54 = load <8 x float>, <8 x float>* %53, align 32, !tbaa !275
  %55 = fcmp ogt <8 x float> %49, %54
  %56 = select <8 x i1> %55, <8 x float> %49, <8 x float> %54
  %57 = add i32 %36, 216
  %58 = sext i32 %57 to i64
  %59 = getelementptr inbounds float, float* %7, i64 %58
  %60 = bitcast float* %59 to <8 x float>*
  %61 = load <8 x float>, <8 x float>* %60, align 32, !tbaa !275
  %62 = fcmp ogt <8 x float> %56, %61
  %63 = select <8 x i1> %62, <8 x float> %56, <8 x float> %61
  %64 = add i32 %36, 224
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to <8 x float>*
  %68 = load <8 x float>, <8 x float>* %67, align 32, !tbaa !275
  %69 = fcmp ogt <8 x float> %63, %68
  %70 = select <8 x i1> %69, <8 x float> %63, <8 x float> %68
  %71 = add i32 %36, 232
  %72 = sext i32 %71 to i64
  %73 = getelementptr inbounds float, float* %7, i64 %72
  %74 = bitcast float* %73 to <8 x float>*
  %75 = load <8 x float>, <8 x float>* %74, align 32, !tbaa !275
  %76 = fcmp ogt <8 x float> %70, %75
  %77 = select <8 x i1> %76, <8 x float> %70, <8 x float> %75
  %78 = add i32 %36, 432
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to <8 x float>*
  %82 = load <8 x float>, <8 x float>* %81, align 32, !tbaa !275
  %83 = fcmp ogt <8 x float> %77, %82
  %84 = select <8 x i1> %83, <8 x float> %77, <8 x float> %82
  %85 = add i32 %36, 440
  %86 = sext i32 %85 to i64
  %87 = getelementptr inbounds float, float* %7, i64 %86
  %88 = bitcast float* %87 to <8 x float>*
  %89 = load <8 x float>, <8 x float>* %88, align 32, !tbaa !275
  %90 = fcmp ogt <8 x float> %84, %89
  %91 = select <8 x i1> %90, <8 x float> %84, <8 x float> %89
  %92 = add i32 %36, 448
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to <8 x float>*
  %96 = load <8 x float>, <8 x float>* %95, align 32, !tbaa !275
  %97 = fcmp ogt <8 x float> %91, %96
  %98 = select <8 x i1> %97, <8 x float> %91, <8 x float> %96
  store <8 x float> %98, <8 x float>* %34, align 32, !tbaa !278
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 13
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %99 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %99, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i32, i1) #5

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #5

attributes #0 = { noinline }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone speculatable }
attributes #4 = { noinline norecurse nounwind }
attributes #5 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_layout_transform_9", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!22 = !{!"0x559a9587c6a0", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x559a9587c6f0", !18, i64 0}
!26 = distinct !{!26, !27}
!27 = !{!"llvm.loop.isvectorized", i32 1}
!28 = !{!"branch_weights", i32 1, i32 1048576}
!29 = distinct !{!29, !27}
!30 = distinct !DISubprogram(name: "fused_nn_max_pool2d_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !31)
!31 = !{!32, !33, !34}
!32 = !DILocalVariable(name: "arg1", arg: 1, scope: !30, file: !1, type: !9)
!33 = !DILocalVariable(name: "arg2", arg: 2, scope: !30, file: !1, type: !9)
!34 = !DILocalVariable(name: "arg3", arg: 3, scope: !30, file: !1, type: !8)
!35 = !DILocation(line: 0, scope: !30)
!36 = !{!37, !37, i64 0}
!37 = !{!"float32", !38, i64 0}
!38 = !{!"0x559a95422d40", !18, i64 0}
!39 = !{!40, !40, i64 0}
!40 = !{!"float32", !41, i64 0}
!41 = !{!"0x559a95423520", !18, i64 0}
!42 = distinct !DISubprogram(name: "fused_nn_dense_add_nn_relu_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !43)
!43 = !{!44, !45, !46}
!44 = !DILocalVariable(name: "arg1", arg: 1, scope: !42, file: !1, type: !9)
!45 = !DILocalVariable(name: "arg2", arg: 2, scope: !42, file: !1, type: !9)
!46 = !DILocalVariable(name: "arg3", arg: 3, scope: !42, file: !1, type: !8)
!47 = !DILocation(line: 0, scope: !42)
!48 = !{!49, !49, i64 0}
!49 = !{!"float32", !50, i64 0}
!50 = !{!"0x559a949a5a80", !18, i64 0}
!51 = !{!52, !52, i64 0}
!52 = !{!"float32", !53, i64 0}
!53 = !{!"0x559a95900620", !18, i64 0}
!54 = !{!55, !55, i64 0}
!55 = !{!"float32", !56, i64 0}
!56 = !{!"0x559a94126260", !18, i64 0}
!57 = distinct !{!57, !27}
!58 = !{!59, !59, i64 0}
!59 = !{!"float32", !60, i64 0}
!60 = !{!"0x559a94437fa0", !18, i64 0}
!61 = !{!62, !62, i64 0}
!62 = !{!"float32", !63, i64 0}
!63 = !{!"0x559a958ef220", !18, i64 0}
!64 = distinct !DISubprogram(name: "fused_nn_dense_add", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !65)
!65 = !{!66, !67, !68}
!66 = !DILocalVariable(name: "arg1", arg: 1, scope: !64, file: !1, type: !9)
!67 = !DILocalVariable(name: "arg2", arg: 2, scope: !64, file: !1, type: !9)
!68 = !DILocalVariable(name: "arg3", arg: 3, scope: !64, file: !1, type: !8)
!69 = !DILocation(line: 0, scope: !64)
!70 = !{!71, !71, i64 0}
!71 = !{!"float32", !72, i64 0}
!72 = !{!"0x559a958387b0", !18, i64 0}
!73 = !{!74, !74, i64 0}
!74 = !{!"float32", !75, i64 0}
!75 = !{!"0x559a953c84f0", !18, i64 0}
!76 = !{!77, !77, i64 0}
!77 = !{!"float32", !78, i64 0}
!78 = !{!"0x559a9583e7e0", !18, i64 0}
!79 = distinct !{!79, !27}
!80 = !{!81, !81, i64 0}
!81 = !{!"float32", !82, i64 0}
!82 = !{!"0x559a956a7010", !18, i64 0}
!83 = !{!84, !84, i64 0}
!84 = !{!"float32", !85, i64 0}
!85 = !{!"0x559a958415e0", !18, i64 0}
!86 = distinct !DISubprogram(name: "fused_nn_dense_add_nn_relu", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !87)
!87 = !{!88, !89, !90}
!88 = !DILocalVariable(name: "arg1", arg: 1, scope: !86, file: !1, type: !9)
!89 = !DILocalVariable(name: "arg2", arg: 2, scope: !86, file: !1, type: !9)
!90 = !DILocalVariable(name: "arg3", arg: 3, scope: !86, file: !1, type: !8)
!91 = !DILocation(line: 0, scope: !86)
!92 = !{!93, !93, i64 0}
!93 = !{!"float32", !94, i64 0}
!94 = !{!"0x559a958eb1b0", !18, i64 0}
!95 = !{!96, !96, i64 0}
!96 = !{!"float32", !97, i64 0}
!97 = !{!"0x559a95a11f70", !18, i64 0}
!98 = !{!99, !99, i64 0}
!99 = !{!"float32", !100, i64 0}
!100 = !{!"0x559a958eb200", !18, i64 0}
!101 = distinct !{!101, !27}
!102 = !{!103, !103, i64 0}
!103 = !{!"float32", !104, i64 0}
!104 = !{!"0x559a958eb110", !18, i64 0}
!105 = !{!106, !106, i64 0}
!106 = !{!"float32", !107, i64 0}
!107 = !{!"0x559a958eb160", !18, i64 0}
!108 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !109)
!109 = !{!110, !111, !112}
!110 = !DILocalVariable(name: "arg1", arg: 1, scope: !108, file: !1, type: !9)
!111 = !DILocalVariable(name: "arg2", arg: 2, scope: !108, file: !1, type: !9)
!112 = !DILocalVariable(name: "arg3", arg: 3, scope: !108, file: !1, type: !8)
!113 = !DILocation(line: 0, scope: !108)
!114 = !{!115, !115, i64 0}
!115 = !{!"float32", !116, i64 0}
!116 = !{!"0x559a9593ccc0", !18, i64 0}
!117 = !{!118, !118, i64 0}
!118 = !{!"float32", !119, i64 0}
!119 = !{!"0x559a94b8cd40", !18, i64 0}
!120 = !{!121, !121, i64 0}
!121 = !{!"float32", !122, i64 0}
!122 = !{!"0x559a9593d990", !18, i64 0}
!123 = !{!124, !124, i64 0}
!124 = !{!"float32", !125, i64 0}
!125 = !{!"0x559a9593cee0", !18, i64 0}
!126 = !{!127, !127, i64 0}
!127 = !{!"float32", !128, i64 0}
!128 = !{!"0x559a9593ca40", !18, i64 0}
!129 = distinct !DISubprogram(name: "fused_layout_transform_nn_batch_flatten_nn_batch_flatten", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !130)
!130 = !{!131, !132, !133}
!131 = !DILocalVariable(name: "arg1", arg: 1, scope: !129, file: !1, type: !9)
!132 = !DILocalVariable(name: "arg2", arg: 2, scope: !129, file: !1, type: !9)
!133 = !DILocalVariable(name: "arg3", arg: 3, scope: !129, file: !1, type: !8)
!134 = !DILocation(line: 0, scope: !129)
!135 = !{!136, !136, i64 0}
!136 = !{!"float32", !137, i64 0}
!137 = !{!"0x559a9448e890", !18, i64 0}
!138 = !{!139, !139, i64 0}
!139 = !{!"float32", !140, i64 0}
!140 = !{!"0x559a9448e8e0", !18, i64 0}
!141 = distinct !DISubprogram(name: "fused_nn_batch_flatten", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !142)
!142 = !{!143, !144, !145}
!143 = !DILocalVariable(name: "arg1", arg: 1, scope: !141, file: !1, type: !9)
!144 = !DILocalVariable(name: "arg2", arg: 2, scope: !141, file: !1, type: !9)
!145 = !DILocalVariable(name: "arg3", arg: 3, scope: !141, file: !1, type: !8)
!146 = !DILocation(line: 0, scope: !141)
!147 = distinct !DISubprogram(name: "fused_nn_max_pool2d", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !148)
!148 = !{!149, !150, !151}
!149 = !DILocalVariable(name: "arg1", arg: 1, scope: !147, file: !1, type: !9)
!150 = !DILocalVariable(name: "arg2", arg: 2, scope: !147, file: !1, type: !9)
!151 = !DILocalVariable(name: "arg3", arg: 3, scope: !147, file: !1, type: !8)
!152 = !DILocation(line: 0, scope: !147)
!153 = !{!154, !154, i64 0}
!154 = !{!"float32", !155, i64 0}
!155 = !{!"0x559a9587f190", !18, i64 0}
!156 = !{!157, !157, i64 0}
!157 = !{!"float32", !158, i64 0}
!158 = !{!"0x559a95946d80", !18, i64 0}
!159 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !160)
!160 = !{!161, !162, !163}
!161 = !DILocalVariable(name: "arg1", arg: 1, scope: !159, file: !1, type: !9)
!162 = !DILocalVariable(name: "arg2", arg: 2, scope: !159, file: !1, type: !9)
!163 = !DILocalVariable(name: "arg3", arg: 3, scope: !159, file: !1, type: !8)
!164 = !DILocation(line: 0, scope: !159)
!165 = !{!166, !166, i64 0}
!166 = !{!"float32", !167, i64 0}
!167 = !{!"0x559a94b8ef20", !18, i64 0}
!168 = !{!169, !169, i64 0}
!169 = !{!"float32", !170, i64 0}
!170 = !{!"0x559a94baabb0", !18, i64 0}
!171 = !{!172, !172, i64 0}
!172 = !{!"float32", !173, i64 0}
!173 = !{!"0x559a94b8e9d0", !18, i64 0}
!174 = !{!175, !175, i64 0}
!175 = !{!"float32", !176, i64 0}
!176 = !{!"0x559a9568c3a0", !18, i64 0}
!177 = !{!178, !178, i64 0}
!178 = !{!"float32", !179, i64 0}
!179 = !{!"0x559a958eea60", !18, i64 0}
!180 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !181)
!181 = !{!182, !183, !184}
!182 = !DILocalVariable(name: "arg1", arg: 1, scope: !180, file: !1, type: !9)
!183 = !DILocalVariable(name: "arg2", arg: 2, scope: !180, file: !1, type: !9)
!184 = !DILocalVariable(name: "arg3", arg: 3, scope: !180, file: !1, type: !8)
!185 = !DILocation(line: 0, scope: !180)
!186 = !{!187, !187, i64 0}
!187 = !{!"float32", !188, i64 0}
!188 = !{!"0x559a95fe54d0", !18, i64 0}
!189 = !{!190, !190, i64 0}
!190 = !{!"float32", !191, i64 0}
!191 = !{!"0x559a95752c70", !18, i64 0}
!192 = !{!193, !193, i64 0}
!193 = !{!"float32", !194, i64 0}
!194 = !{!"0x559a95fe38f0", !18, i64 0}
!195 = !{!196, !196, i64 0}
!196 = !{!"float32", !197, i64 0}
!197 = !{!"0x559a95fe3f20", !18, i64 0}
!198 = !{!199, !199, i64 0}
!199 = !{!"float32", !200, i64 0}
!200 = !{!"0x559a95fe42b0", !18, i64 0}
!201 = !{!202, !202, i64 0}
!202 = !{!"0x559a95886ed0.w8.b0", !203, i64 0}
!203 = !{!"0x559a95886ed0.w16.b0", !204, i64 0}
!204 = !{!"0x559a95886ed0.w32.b0", !205, i64 0}
!205 = !{!"0x559a95886ed0.w64.b0", !206, i64 0}
!206 = !{!"0x559a95886ed0.w128.b0", !207, i64 0}
!207 = !{!"0x559a95886ed0.w256.b0", !208, i64 0}
!208 = !{!"0x559a95886ed0.w512.b0", !209, i64 0}
!209 = !{!"0x559a95886ed0.w1024.b0", !210, i64 0}
!210 = !{!"float32", !211, i64 0}
!211 = !{!"0x559a95886ed0", !18, i64 0}
!212 = !{!213, !213, i64 0}
!213 = !{!"float32", !214, i64 0}
!214 = !{!"0x559a95fe4f80", !18, i64 0}
!215 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !216)
!216 = !{!217, !218, !219}
!217 = !DILocalVariable(name: "arg1", arg: 1, scope: !215, file: !1, type: !9)
!218 = !DILocalVariable(name: "arg2", arg: 2, scope: !215, file: !1, type: !9)
!219 = !DILocalVariable(name: "arg3", arg: 3, scope: !215, file: !1, type: !8)
!220 = !DILocation(line: 0, scope: !215)
!221 = !{!222, !222, i64 0}
!222 = !{!"float32", !223, i64 0}
!223 = !{!"0x559a94463680", !18, i64 0}
!224 = !{!225, !225, i64 0}
!225 = !{!"float32", !226, i64 0}
!226 = !{!"0x559a957d9a40", !18, i64 0}
!227 = !{!228, !228, i64 0}
!228 = !{!"0x559a956bf0a0.w8.b0", !229, i64 0}
!229 = !{!"0x559a956bf0a0.w16.b0", !230, i64 0}
!230 = !{!"0x559a956bf0a0.w32.b0", !231, i64 0}
!231 = !{!"0x559a956bf0a0.w64.b0", !232, i64 0}
!232 = !{!"0x559a956bf0a0.w128.b0", !233, i64 0}
!233 = !{!"0x559a956bf0a0.w256.b0", !234, i64 0}
!234 = !{!"0x559a956bf0a0.w512.b0", !235, i64 0}
!235 = !{!"0x559a956bf0a0.w1024.b0", !236, i64 0}
!236 = !{!"float32", !237, i64 0}
!237 = !{!"0x559a956bf0a0", !18, i64 0}
!238 = !{!239, !239, i64 0}
!239 = !{!"float32", !240, i64 0}
!240 = !{!"0x559a95863880", !18, i64 0}
!241 = !{!242, !242, i64 0}
!242 = !{!"float32", !243, i64 0}
!243 = !{!"0x559a94463a10", !18, i64 0}
!244 = !{!236, !236, i64 0}
!245 = !{!246, !246, i64 0}
!246 = !{!"float32", !247, i64 0}
!247 = !{!"0x559a94463050", !18, i64 0}
!248 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !249)
!249 = !{!250, !251, !252}
!250 = !DILocalVariable(name: "arg1", arg: 1, scope: !248, file: !1, type: !9)
!251 = !DILocalVariable(name: "arg2", arg: 2, scope: !248, file: !1, type: !9)
!252 = !DILocalVariable(name: "arg3", arg: 3, scope: !248, file: !1, type: !8)
!253 = !DILocation(line: 0, scope: !248)
!254 = !{!255, !255, i64 0}
!255 = !{!"float32", !256, i64 0}
!256 = !{!"0x559a9411e480", !18, i64 0}
!257 = !{!258, !258, i64 0}
!258 = !{!"float32", !259, i64 0}
!259 = !{!"0x559a95929560", !18, i64 0}
!260 = !{!261, !261, i64 0}
!261 = !{!"float32", !262, i64 0}
!262 = !{!"0x559a95736340", !18, i64 0}
!263 = !{!264, !264, i64 0}
!264 = !{!"float32", !265, i64 0}
!265 = !{!"0x559a9411e6a0", !18, i64 0}
!266 = !{!267, !267, i64 0}
!267 = !{!"float32", !268, i64 0}
!268 = !{!"0x559a9411e200", !18, i64 0}
!269 = distinct !DISubprogram(name: "fused_nn_max_pool2d_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !270)
!270 = !{!271, !272, !273}
!271 = !DILocalVariable(name: "arg1", arg: 1, scope: !269, file: !1, type: !9)
!272 = !DILocalVariable(name: "arg2", arg: 2, scope: !269, file: !1, type: !9)
!273 = !DILocalVariable(name: "arg3", arg: 3, scope: !269, file: !1, type: !8)
!274 = !DILocation(line: 0, scope: !269)
!275 = !{!276, !276, i64 0}
!276 = !{!"float32", !277, i64 0}
!277 = !{!"0x559a95733b90", !18, i64 0}
!278 = !{!279, !279, i64 0}
!279 = !{!"float32", !280, i64 0}
!280 = !{!"0x559a95734370", !18, i64 0}
