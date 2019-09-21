; ModuleID = 'fused_layout_transform_2'
source_filename = "fused_layout_transform_2"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i32*, i32 }
%1 = type { i8*, %2, i32, %3, i64*, i64*, i64 }
%2 = type { i32, i32 }
%3 = type { i8, i8, i16 }
%4 = type { i8*, i8* }
%5 = type { i8*, i8* }
%6 = type { i8*, i8* }
%7 = type { i8*, i8*, i8* }

@__TVMBackendParallelLaunch = linkonce dllexport local_unnamed_addr global i32 (i32 (i32, %0*, i8*)*, i8*, i32)* null, align 8
@__TVMBackendAllocWorkspace = linkonce dllexport local_unnamed_addr global i8* (i32, i32, i64, i32, i32)* null, align 8
@__TVMBackendFreeWorkspace = linkonce dllexport local_unnamed_addr global i32 (i32, i32, i8*)* null, align 8
@__tvm_main__ = weak local_unnamed_addr constant [25 x i8] c"fused_layout_transform_2\00", align 1

define dllexport i32 @fused_layout_transform_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !5 {
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
  %12 = tail call fastcc i32 @fused_layout_transform_2_compute_(i8* %11, i8* %9), !dbg !15
  ret i32 %12, !dbg !15
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_2_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %4, align 8
  %3 = getelementptr inbounds %4, %4* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %4, %4* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %4* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda, i8* nonnull %5, i32 0)
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
  %10 = add nsw i32 %9, 63
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 64
  %15 = select i1 %14, i32 %13, i32 64
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 64
  %18 = select i1 %17, i32 %16, i32 64
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
  %24 = trunc i64 %indvars.iv to i32
  %25 = shl i32 %24, 6
  %26 = sext i32 %25 to i64
  %27 = getelementptr inbounds float, float* %7, i64 %26
  %28 = getelementptr inbounds float, float* %4, i64 %26
  %29 = bitcast float* %27 to <4 x i32>*
  %30 = load <4 x i32>, <4 x i32>* %29, align 4, !tbaa !20
  %31 = bitcast float* %28 to <4 x i32>*
  store <4 x i32> %30, <4 x i32>* %31, align 4, !tbaa !23
  %32 = or i64 %26, 4
  %33 = getelementptr inbounds float, float* %7, i64 %32
  %34 = getelementptr inbounds float, float* %4, i64 %32
  %35 = bitcast float* %33 to <4 x i32>*
  %36 = load <4 x i32>, <4 x i32>* %35, align 4, !tbaa !20
  %37 = bitcast float* %34 to <4 x i32>*
  store <4 x i32> %36, <4 x i32>* %37, align 4, !tbaa !23
  %38 = or i64 %26, 8
  %39 = getelementptr inbounds float, float* %7, i64 %38
  %40 = getelementptr inbounds float, float* %4, i64 %38
  %41 = bitcast float* %39 to <4 x i32>*
  %42 = load <4 x i32>, <4 x i32>* %41, align 4, !tbaa !20
  %43 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %42, <4 x i32>* %43, align 4, !tbaa !23
  %44 = or i64 %26, 12
  %45 = getelementptr inbounds float, float* %7, i64 %44
  %46 = getelementptr inbounds float, float* %4, i64 %44
  %47 = bitcast float* %45 to <4 x i32>*
  %48 = load <4 x i32>, <4 x i32>* %47, align 4, !tbaa !20
  %49 = bitcast float* %46 to <4 x i32>*
  store <4 x i32> %48, <4 x i32>* %49, align 4, !tbaa !23
  %50 = or i64 %26, 16
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = getelementptr inbounds float, float* %4, i64 %50
  %53 = bitcast float* %51 to <4 x i32>*
  %54 = load <4 x i32>, <4 x i32>* %53, align 4, !tbaa !20
  %55 = bitcast float* %52 to <4 x i32>*
  store <4 x i32> %54, <4 x i32>* %55, align 4, !tbaa !23
  %56 = or i64 %26, 20
  %57 = getelementptr inbounds float, float* %7, i64 %56
  %58 = getelementptr inbounds float, float* %4, i64 %56
  %59 = bitcast float* %57 to <4 x i32>*
  %60 = load <4 x i32>, <4 x i32>* %59, align 4, !tbaa !20
  %61 = bitcast float* %58 to <4 x i32>*
  store <4 x i32> %60, <4 x i32>* %61, align 4, !tbaa !23
  %62 = or i64 %26, 24
  %63 = getelementptr inbounds float, float* %7, i64 %62
  %64 = getelementptr inbounds float, float* %4, i64 %62
  %65 = bitcast float* %63 to <4 x i32>*
  %66 = load <4 x i32>, <4 x i32>* %65, align 4, !tbaa !20
  %67 = bitcast float* %64 to <4 x i32>*
  store <4 x i32> %66, <4 x i32>* %67, align 4, !tbaa !23
  %68 = or i64 %26, 28
  %69 = getelementptr inbounds float, float* %7, i64 %68
  %70 = getelementptr inbounds float, float* %4, i64 %68
  %71 = bitcast float* %69 to <4 x i32>*
  %72 = load <4 x i32>, <4 x i32>* %71, align 4, !tbaa !20
  %73 = bitcast float* %70 to <4 x i32>*
  store <4 x i32> %72, <4 x i32>* %73, align 4, !tbaa !23
  %74 = or i64 %26, 32
  %75 = getelementptr inbounds float, float* %7, i64 %74
  %76 = getelementptr inbounds float, float* %4, i64 %74
  %77 = bitcast float* %75 to <4 x i32>*
  %78 = load <4 x i32>, <4 x i32>* %77, align 4, !tbaa !20
  %79 = bitcast float* %76 to <4 x i32>*
  store <4 x i32> %78, <4 x i32>* %79, align 4, !tbaa !23
  %80 = or i64 %26, 36
  %81 = getelementptr inbounds float, float* %7, i64 %80
  %82 = getelementptr inbounds float, float* %4, i64 %80
  %83 = bitcast float* %81 to <4 x i32>*
  %84 = load <4 x i32>, <4 x i32>* %83, align 4, !tbaa !20
  %85 = bitcast float* %82 to <4 x i32>*
  store <4 x i32> %84, <4 x i32>* %85, align 4, !tbaa !23
  %86 = or i64 %26, 40
  %87 = getelementptr inbounds float, float* %7, i64 %86
  %88 = getelementptr inbounds float, float* %4, i64 %86
  %89 = bitcast float* %87 to <4 x i32>*
  %90 = load <4 x i32>, <4 x i32>* %89, align 4, !tbaa !20
  %91 = bitcast float* %88 to <4 x i32>*
  store <4 x i32> %90, <4 x i32>* %91, align 4, !tbaa !23
  %92 = or i64 %26, 44
  %93 = getelementptr inbounds float, float* %7, i64 %92
  %94 = getelementptr inbounds float, float* %4, i64 %92
  %95 = bitcast float* %93 to <4 x i32>*
  %96 = load <4 x i32>, <4 x i32>* %95, align 4, !tbaa !20
  %97 = bitcast float* %94 to <4 x i32>*
  store <4 x i32> %96, <4 x i32>* %97, align 4, !tbaa !23
  %98 = or i64 %26, 48
  %99 = getelementptr inbounds float, float* %7, i64 %98
  %100 = getelementptr inbounds float, float* %4, i64 %98
  %101 = bitcast float* %99 to <4 x i32>*
  %102 = load <4 x i32>, <4 x i32>* %101, align 4, !tbaa !20
  %103 = bitcast float* %100 to <4 x i32>*
  store <4 x i32> %102, <4 x i32>* %103, align 4, !tbaa !23
  %104 = or i64 %26, 52
  %105 = getelementptr inbounds float, float* %7, i64 %104
  %106 = getelementptr inbounds float, float* %4, i64 %104
  %107 = bitcast float* %105 to <4 x i32>*
  %108 = load <4 x i32>, <4 x i32>* %107, align 4, !tbaa !20
  %109 = bitcast float* %106 to <4 x i32>*
  store <4 x i32> %108, <4 x i32>* %109, align 4, !tbaa !23
  %110 = or i64 %26, 56
  %111 = getelementptr inbounds float, float* %7, i64 %110
  %112 = getelementptr inbounds float, float* %4, i64 %110
  %113 = bitcast float* %111 to <4 x i32>*
  %114 = load <4 x i32>, <4 x i32>* %113, align 4, !tbaa !20
  %115 = bitcast float* %112 to <4 x i32>*
  store <4 x i32> %114, <4 x i32>* %115, align 4, !tbaa !23
  %116 = or i64 %26, 60
  %117 = getelementptr inbounds float, float* %7, i64 %116
  %118 = getelementptr inbounds float, float* %4, i64 %116
  %119 = bitcast float* %117 to <4 x i32>*
  %120 = load <4 x i32>, <4 x i32>* %119, align 4, !tbaa !20
  %121 = bitcast float* %118 to <4 x i32>*
  store <4 x i32> %120, <4 x i32>* %121, align 4, !tbaa !23
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %122 = icmp slt i64 %indvars.iv.next, %23
  br i1 %122, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
}

define dllexport i32 @fused_layout_transform_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !26 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !28, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.value(metadata i8* %1, metadata !29, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.value(metadata i32 %2, metadata !30, metadata !DIExpression()), !dbg !31
  %3 = bitcast i8* %0 to %1**, !dbg !31
  %4 = load %1*, %1** %3, align 8, !dbg !31
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !31
  %6 = bitcast i8* %5 to %1**, !dbg !31
  %7 = load %1*, %1** %6, align 8, !dbg !31
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !31
  %9 = load i8*, i8** %8, align 8, !dbg !31
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !31
  %11 = load i8*, i8** %10, align 8, !dbg !31
  %12 = tail call fastcc i32 @fused_layout_transform_1_compute_(i8* %11, i8* %9), !dbg !31
  ret i32 %12, !dbg !31
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_1_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %5, align 8
  %3 = getelementptr inbounds %5, %5* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %5, %5* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %5* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.1, i8* nonnull %5, i32 0)
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
  %10 = add nsw i32 %9, 1
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 2
  %15 = select i1 %14, i32 %13, i32 2
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 2
  %18 = select i1 %17, i32 %16, i32 2
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv10 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next11, %for_end3 ]
  %24 = trunc i64 %indvars.iv10 to i32
  %25 = shl i32 %24, 12
  %26 = sext i32 %25 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %27 = shl i64 %indvars.iv7, 6
  %28 = add nsw i64 %27, %26
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %29 = shl i32 %indvars.iv7.tr, 7
  %30 = add i32 %29, %24
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %31 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %31, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %32 = add nsw i64 %28, %indvars.iv
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %33 = shl i32 %indvars.iv.tr, 1
  %34 = add i32 %30, %33
  %35 = sext i32 %34 to i64
  %36 = getelementptr inbounds float, float* %7, i64 %35
  %37 = bitcast float* %36 to i32*
  %38 = load i32, i32* %37, align 4, !tbaa !32
  %39 = getelementptr inbounds float, float* %4, i64 %32
  %40 = bitcast float* %39 to i32*
  store i32 %38, i32* %40, align 4, !tbaa !35
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 64
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !38

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 64
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !38
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !39 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !41, metadata !DIExpression()), !dbg !44
  call void @llvm.dbg.value(metadata i8* %1, metadata !42, metadata !DIExpression()), !dbg !44
  call void @llvm.dbg.value(metadata i32 %2, metadata !43, metadata !DIExpression()), !dbg !44
  %3 = bitcast i8* %0 to %1**, !dbg !44
  %4 = load %1*, %1** %3, align 8, !dbg !44
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !44
  %6 = bitcast i8* %5 to %1**, !dbg !44
  %7 = load %1*, %1** %6, align 8, !dbg !44
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !44
  %9 = bitcast i8* %8 to %1**, !dbg !44
  %10 = load %1*, %1** %9, align 8, !dbg !44
  %11 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !44
  %12 = load i8*, i8** %11, align 8, !dbg !44
  %13 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !44
  %14 = load i32, i32* %13, align 4, !dbg !44
  %15 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !44
  %16 = load i8*, i8** %15, align 8, !dbg !44
  %17 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !44
  %18 = load i8*, i8** %17, align 8, !dbg !44
  %19 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_compute_(i8* %12, i8* %16, i8* %18, i32 %14), !dbg !44
  ret i32 %19, !dbg !44
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_compute_(i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %4 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %5 = tail call i8* %4(i32 1, i32 %3, i64 17424, i32 2, i32 32)
  %6 = alloca %6, align 8
  %7 = getelementptr inbounds %6, %6* %6, i64 0, i32 0
  store i8* %5, i8** %7, align 8
  %8 = getelementptr inbounds %6, %6* %6, i64 0, i32 1
  store i8* %0, i8** %8, align 8
  %9 = bitcast %6* %6 to i8*
  %10 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %11 = call i32 %10(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.2, i8* nonnull %9, i32 0)
  %12 = icmp eq i32 %11, 0
  br i1 %12, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %11, %entry ], [ 0, %call_end2 ], [ %19, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %13 = alloca %7, align 8
  %14 = getelementptr inbounds %7, %7* %13, i64 0, i32 0
  store i8* %5, i8** %14, align 8
  %15 = getelementptr inbounds %7, %7* %13, i64 0, i32 1
  store i8* %1, i8** %15, align 8
  %16 = getelementptr inbounds %7, %7* %13, i64 0, i32 2
  store i8* %2, i8** %16, align 8
  %17 = bitcast %7* %13 to i8*
  %18 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %19 = call i32 %18(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.3, i8* nonnull %17, i32 0)
  %20 = icmp eq i32 %19, 0
  br i1 %20, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %21 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %22 = call i32 %21(i32 1, i32 %3, i8* %5)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.2(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 65
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 66
  %15 = select i1 %14, i32 %13, i32 66
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 66
  %18 = select i1 %17, i32 %16, i32 66
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = xor i32 %16, -1
  %21 = icmp sgt i32 %20, -67
  %smax = select i1 %21, i32 %20, i32 -67
  %22 = mul i32 %smax, -66
  %23 = add i32 %22, -66
  %24 = add i32 %18, 1
  %25 = sext i32 %24 to i64
  %26 = add nsw i64 %25, -1
  %27 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv6 = phi i64 [ %26, %for_body.lr.ph ], [ %indvars.iv.next7, %for_end3 ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %28 = mul nsw i64 %indvars.iv6, 66
  %29 = trunc i64 %indvars.iv6 to i32
  %.off = add i32 %29, -1
  %30 = icmp ult i32 %.off, 64
  %31 = shl i32 %29, 6
  %32 = add i32 %31, -65
  br i1 %30, label %for_body2.us.preheader, label %for_body.split

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %if_end.us
  %indvars.iv = phi i64 [ %indvars.iv.next, %if_end.us ], [ 0, %for_body2.us.preheader ]
  %33 = phi i32 [ %42, %if_end.us ], [ 0, %for_body2.us.preheader ]
  %34 = add nsw i64 %indvars.iv, %28
  %trunc.us = trunc i32 %33 to i31
  switch i31 %trunc.us, label %if_then.us [
    i31 65, label %if_end.us
    i31 0, label %if_end.us
  ]

if_then.us:                                       ; preds = %for_body2.us
  %35 = trunc i64 %indvars.iv to i32
  %36 = add i32 %32, %35
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !45
  br label %if_end.us

if_end.us:                                        ; preds = %if_then.us, %for_body2.us, %for_body2.us
  %40 = phi float [ %39, %if_then.us ], [ 0.000000e+00, %for_body2.us ], [ 0.000000e+00, %for_body2.us ]
  %41 = getelementptr inbounds float, float* %4, i64 %34
  store float %40, float* %41, align 4, !tbaa !48
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %42 = add nuw nsw i32 %33, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 66
  br i1 %exitcond, label %for_end3, label %for_body2.us, !prof !38

for_body.split:                                   ; preds = %for_body
  %43 = mul i32 %indvar, 66
  %44 = add i32 %23, %43
  %45 = sext i32 %44 to i64
  %scevgep = getelementptr float, float* %4, i64 %45
  %scevgep5 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep5, i8 0, i64 264, i32 4, i1 false)
  br label %for_end3

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_end3:                                         ; preds = %if_end.us, %for_body.split
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %46 = icmp slt i64 %indvars.iv.next7, %27
  %indvar.next = add nuw i32 %indvar, 1
  br i1 %46, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.3(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = alloca [16 x <2 x float>], align 16
  %4 = bitcast [16 x <2 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0
  %5 = bitcast i8* %2 to float**
  %6 = load float*, float** %5, align 8
  %7 = getelementptr inbounds i8, i8* %2, i64 8
  %8 = bitcast i8* %7 to float**
  %9 = load float*, float** %8, align 8
  %10 = getelementptr inbounds i8, i8* %2, i64 16
  %11 = bitcast i8* %10 to float**
  %12 = load float*, float** %11, align 8
  %13 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %14 = load i32, i32* %13, align 4
  %15 = add nsw i32 %14, 63
  %16 = sdiv i32 %15, %14
  %17 = add nsw i32 %0, 1
  %18 = mul nsw i32 %16, %17
  %19 = icmp slt i32 %18, 64
  %20 = select i1 %19, i32 %18, i32 64
  %21 = mul nsw i32 %16, %0
  %22 = icmp slt i32 %21, 64
  %23 = select i1 %22, i32 %21, i32 64
  %24 = icmp slt i32 %23, %20
  br i1 %24, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %25 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 2
  %26 = bitcast float* %25 to <2 x float>*
  %27 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 4
  %28 = bitcast float* %27 to <2 x float>*
  %29 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 6
  %30 = bitcast float* %29 to <2 x float>*
  %31 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 8
  %32 = bitcast float* %31 to <2 x float>*
  %33 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 10
  %34 = bitcast float* %33 to <2 x float>*
  %35 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 12
  %36 = bitcast float* %35 to <2 x float>*
  %37 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 14
  %38 = bitcast float* %37 to <2 x float>*
  %39 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 16
  %40 = bitcast float* %39 to <2 x float>*
  %41 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 18
  %42 = bitcast float* %41 to <2 x float>*
  %43 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 20
  %44 = bitcast float* %43 to <2 x float>*
  %45 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 22
  %46 = bitcast float* %45 to <2 x float>*
  %47 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 24
  %48 = bitcast float* %47 to <2 x float>*
  %49 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 26
  %50 = bitcast float* %49 to <2 x float>*
  %51 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 28
  %52 = bitcast float* %51 to <2 x float>*
  %53 = getelementptr inbounds [16 x <2 x float>], [16 x <2 x float>]* %3, i64 0, i64 0, i64 30
  %54 = bitcast float* %53 to <2 x float>*
  %55 = xor i32 %21, -1
  %56 = icmp sgt i32 %55, -65
  %smax = select i1 %56, i32 %55, i32 -65
  %57 = shl i32 %smax, 7
  %58 = sub i32 -128, %57
  %59 = add i32 %23, 1
  %60 = sext i32 %59 to i64
  %61 = add nsw i64 %60, -1
  %62 = sext i32 %20 to i64
  %63 = bitcast [16 x <2 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv95 = phi i64 [ %61, %for_body.lr.ph ], [ %indvars.iv.next96, %for_end3 ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %64 = shl i32 %indvar, 7
  %65 = add i32 %58, %64
  %66 = trunc i64 %indvars.iv95 to i32
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv92 = phi i64 [ 0, %for_body ], [ %indvars.iv.next93, %for_end6 ]
  %indvars.iv92.tr = trunc i64 %indvars.iv92 to i32
  %67 = shl i32 %indvars.iv92.tr, 5
  %68 = add i32 %65, %67
  %69 = sext i32 %68 to i64
  %scevgep = getelementptr float, float* %12, i64 %69
  %scevgep91 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull %63, i8 0, i64 128, i32 16, i1 false)
  %indvars.iv92.tr97 = trunc i64 %indvars.iv92 to i32
  %70 = shl i32 %indvars.iv92.tr97, 4
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next96 = add nsw i64 %indvars.iv95, 1
  %71 = icmp slt i64 %indvars.iv.next96, %62
  %indvar.next = add nuw i32 %indvar, 1
  br i1 %71, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %.lcssa4172 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %293, %for_body5 ]
  %.lcssa3970 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %287, %for_body5 ]
  %.lcssa3768 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %286, %for_body5 ]
  %.lcssa3566 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %280, %for_body5 ]
  %.lcssa3364 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %274, %for_body5 ]
  %.lcssa3162 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %268, %for_body5 ]
  %.lcssa2960 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %262, %for_body5 ]
  %.lcssa2758 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %256, %for_body5 ]
  %.lcssa2556 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %250, %for_body5 ]
  %.lcssa2354 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %244, %for_body5 ]
  %.lcssa2152 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %238, %for_body5 ]
  %.lcssa1950 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %232, %for_body5 ]
  %.lcssa1748 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %226, %for_body5 ]
  %.lcssa1546 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %220, %for_body5 ]
  %.lcssa1345 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %214, %for_body5 ]
  %.lcssa43 = phi <2 x float> [ zeroinitializer, %for_body2 ], [ %208, %for_body5 ]
  %72 = trunc i64 %indvars.iv to i32
  %73 = add i32 %72, %66
  %74 = mul i32 %73, 66
  %75 = add nsw i32 %74, %70
  %76 = mul nuw nsw i64 %indvars.iv, 6
  %77 = sext i32 %75 to i64
  %78 = getelementptr inbounds float, float* %6, i64 %77
  %79 = load float, float* %78, align 4, !tbaa !48
  %80 = insertelement <2 x float> undef, float %79, i32 0
  %81 = shufflevector <2 x float> %80, <2 x float> undef, <2 x i32> zeroinitializer
  %82 = getelementptr inbounds float, float* %9, i64 %76
  %83 = bitcast float* %82 to <2 x float>*
  %84 = load <2 x float>, <2 x float>* %83, align 8, !tbaa !51
  %85 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %81, <2 x float> %84, <2 x float> %.lcssa43)
  %86 = or i64 %77, 1
  %87 = getelementptr inbounds float, float* %6, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !48
  %89 = insertelement <2 x float> undef, float %88, i32 0
  %90 = shufflevector <2 x float> %89, <2 x float> undef, <2 x i32> zeroinitializer
  %91 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %90, <2 x float> %84, <2 x float> %.lcssa1345)
  %92 = add nsw i64 %77, 2
  %93 = getelementptr inbounds float, float* %6, i64 %92
  %94 = load float, float* %93, align 4, !tbaa !48
  %95 = insertelement <2 x float> undef, float %94, i32 0
  %96 = shufflevector <2 x float> %95, <2 x float> undef, <2 x i32> zeroinitializer
  %97 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %96, <2 x float> %84, <2 x float> %.lcssa1546)
  %98 = add nsw i64 %77, 3
  %99 = getelementptr inbounds float, float* %6, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !48
  %101 = insertelement <2 x float> undef, float %100, i32 0
  %102 = shufflevector <2 x float> %101, <2 x float> undef, <2 x i32> zeroinitializer
  %103 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %102, <2 x float> %84, <2 x float> %.lcssa1748)
  %104 = add nsw i64 %77, 4
  %105 = getelementptr inbounds float, float* %6, i64 %104
  %106 = load float, float* %105, align 4, !tbaa !48
  %107 = insertelement <2 x float> undef, float %106, i32 0
  %108 = shufflevector <2 x float> %107, <2 x float> undef, <2 x i32> zeroinitializer
  %109 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %108, <2 x float> %84, <2 x float> %.lcssa1950)
  %110 = add nsw i64 %77, 5
  %111 = getelementptr inbounds float, float* %6, i64 %110
  %112 = load float, float* %111, align 4, !tbaa !48
  %113 = insertelement <2 x float> undef, float %112, i32 0
  %114 = shufflevector <2 x float> %113, <2 x float> undef, <2 x i32> zeroinitializer
  %115 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %114, <2 x float> %84, <2 x float> %.lcssa2152)
  %116 = add nsw i64 %77, 6
  %117 = getelementptr inbounds float, float* %6, i64 %116
  %118 = load float, float* %117, align 4, !tbaa !48
  %119 = insertelement <2 x float> undef, float %118, i32 0
  %120 = shufflevector <2 x float> %119, <2 x float> undef, <2 x i32> zeroinitializer
  %121 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %120, <2 x float> %84, <2 x float> %.lcssa2354)
  %122 = add nsw i64 %77, 7
  %123 = getelementptr inbounds float, float* %6, i64 %122
  %124 = load float, float* %123, align 4, !tbaa !48
  %125 = insertelement <2 x float> undef, float %124, i32 0
  %126 = shufflevector <2 x float> %125, <2 x float> undef, <2 x i32> zeroinitializer
  %127 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %126, <2 x float> %84, <2 x float> %.lcssa2556)
  %128 = add nsw i64 %77, 8
  %129 = getelementptr inbounds float, float* %6, i64 %128
  %130 = load float, float* %129, align 4, !tbaa !48
  %131 = insertelement <2 x float> undef, float %130, i32 0
  %132 = shufflevector <2 x float> %131, <2 x float> undef, <2 x i32> zeroinitializer
  %133 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %132, <2 x float> %84, <2 x float> %.lcssa2758)
  %134 = add nsw i64 %77, 9
  %135 = getelementptr inbounds float, float* %6, i64 %134
  %136 = load float, float* %135, align 4, !tbaa !48
  %137 = insertelement <2 x float> undef, float %136, i32 0
  %138 = shufflevector <2 x float> %137, <2 x float> undef, <2 x i32> zeroinitializer
  %139 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %138, <2 x float> %84, <2 x float> %.lcssa2960)
  %140 = add nsw i64 %77, 10
  %141 = getelementptr inbounds float, float* %6, i64 %140
  %142 = load float, float* %141, align 4, !tbaa !48
  %143 = insertelement <2 x float> undef, float %142, i32 0
  %144 = shufflevector <2 x float> %143, <2 x float> undef, <2 x i32> zeroinitializer
  %145 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %144, <2 x float> %84, <2 x float> %.lcssa3162)
  %146 = add nsw i64 %77, 11
  %147 = getelementptr inbounds float, float* %6, i64 %146
  %148 = load float, float* %147, align 4, !tbaa !48
  %149 = insertelement <2 x float> undef, float %148, i32 0
  %150 = shufflevector <2 x float> %149, <2 x float> undef, <2 x i32> zeroinitializer
  %151 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %150, <2 x float> %84, <2 x float> %.lcssa3364)
  %152 = add nsw i64 %77, 12
  %153 = getelementptr inbounds float, float* %6, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !48
  %155 = insertelement <2 x float> undef, float %154, i32 0
  %156 = shufflevector <2 x float> %155, <2 x float> undef, <2 x i32> zeroinitializer
  %157 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %156, <2 x float> %84, <2 x float> %.lcssa3566)
  %158 = add nsw i64 %77, 13
  %159 = getelementptr inbounds float, float* %6, i64 %158
  %160 = load float, float* %159, align 4, !tbaa !48
  %161 = insertelement <2 x float> undef, float %160, i32 0
  %162 = shufflevector <2 x float> %161, <2 x float> undef, <2 x i32> zeroinitializer
  %163 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %162, <2 x float> %84, <2 x float> %.lcssa3768)
  %164 = add nsw i64 %77, 14
  %165 = getelementptr inbounds float, float* %6, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !48
  %167 = insertelement <2 x float> undef, float %166, i32 0
  %168 = shufflevector <2 x float> %167, <2 x float> undef, <2 x i32> zeroinitializer
  %169 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %168, <2 x float> %84, <2 x float> %.lcssa3970)
  %170 = add nsw i64 %77, 15
  %171 = getelementptr inbounds float, float* %6, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !48
  %173 = insertelement <2 x float> undef, float %172, i32 0
  %174 = shufflevector <2 x float> %173, <2 x float> undef, <2 x i32> zeroinitializer
  %175 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %174, <2 x float> %84, <2 x float> %.lcssa4172)
  %176 = add nuw nsw i64 %76, 2
  %177 = getelementptr inbounds float, float* %9, i64 %176
  %178 = bitcast float* %177 to <2 x float>*
  %179 = load <2 x float>, <2 x float>* %178, align 8, !tbaa !51
  %180 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %90, <2 x float> %179, <2 x float> %85)
  %181 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %96, <2 x float> %179, <2 x float> %91)
  %182 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %102, <2 x float> %179, <2 x float> %97)
  %183 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %108, <2 x float> %179, <2 x float> %103)
  %184 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %114, <2 x float> %179, <2 x float> %109)
  %185 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %120, <2 x float> %179, <2 x float> %115)
  %186 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %126, <2 x float> %179, <2 x float> %121)
  %187 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %132, <2 x float> %179, <2 x float> %127)
  %188 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %138, <2 x float> %179, <2 x float> %133)
  %189 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %144, <2 x float> %179, <2 x float> %139)
  %190 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %150, <2 x float> %179, <2 x float> %145)
  %191 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %156, <2 x float> %179, <2 x float> %151)
  %192 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %162, <2 x float> %179, <2 x float> %157)
  %193 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %168, <2 x float> %179, <2 x float> %163)
  %194 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %174, <2 x float> %179, <2 x float> %169)
  %195 = add nsw i64 %86, 15
  %196 = getelementptr inbounds float, float* %6, i64 %195
  %197 = load float, float* %196, align 4, !tbaa !48
  %198 = insertelement <2 x float> undef, float %197, i32 0
  %199 = shufflevector <2 x float> %198, <2 x float> undef, <2 x i32> zeroinitializer
  %200 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %199, <2 x float> %179, <2 x float> %175)
  %201 = load float, float* %93, align 4, !tbaa !48
  %202 = insertelement <2 x float> undef, float %201, i32 0
  %203 = shufflevector <2 x float> %202, <2 x float> undef, <2 x i32> zeroinitializer
  %204 = add nuw nsw i64 %76, 4
  %205 = getelementptr inbounds float, float* %9, i64 %204
  %206 = bitcast float* %205 to <2 x float>*
  %207 = load <2 x float>, <2 x float>* %206, align 8, !tbaa !51
  %208 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %203, <2 x float> %207, <2 x float> %180)
  %209 = add nsw i64 %77, 3
  %210 = getelementptr inbounds float, float* %6, i64 %209
  %211 = load float, float* %210, align 4, !tbaa !48
  %212 = insertelement <2 x float> undef, float %211, i32 0
  %213 = shufflevector <2 x float> %212, <2 x float> undef, <2 x i32> zeroinitializer
  %214 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %213, <2 x float> %207, <2 x float> %181)
  %215 = add nsw i64 %77, 4
  %216 = getelementptr inbounds float, float* %6, i64 %215
  %217 = load float, float* %216, align 4, !tbaa !48
  %218 = insertelement <2 x float> undef, float %217, i32 0
  %219 = shufflevector <2 x float> %218, <2 x float> undef, <2 x i32> zeroinitializer
  %220 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %219, <2 x float> %207, <2 x float> %182)
  %221 = add nsw i64 %77, 5
  %222 = getelementptr inbounds float, float* %6, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !48
  %224 = insertelement <2 x float> undef, float %223, i32 0
  %225 = shufflevector <2 x float> %224, <2 x float> undef, <2 x i32> zeroinitializer
  %226 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %225, <2 x float> %207, <2 x float> %183)
  %227 = add nsw i64 %77, 6
  %228 = getelementptr inbounds float, float* %6, i64 %227
  %229 = load float, float* %228, align 4, !tbaa !48
  %230 = insertelement <2 x float> undef, float %229, i32 0
  %231 = shufflevector <2 x float> %230, <2 x float> undef, <2 x i32> zeroinitializer
  %232 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %231, <2 x float> %207, <2 x float> %184)
  %233 = add nsw i64 %77, 7
  %234 = getelementptr inbounds float, float* %6, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !48
  %236 = insertelement <2 x float> undef, float %235, i32 0
  %237 = shufflevector <2 x float> %236, <2 x float> undef, <2 x i32> zeroinitializer
  %238 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %237, <2 x float> %207, <2 x float> %185)
  %239 = add nsw i64 %77, 8
  %240 = getelementptr inbounds float, float* %6, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !48
  %242 = insertelement <2 x float> undef, float %241, i32 0
  %243 = shufflevector <2 x float> %242, <2 x float> undef, <2 x i32> zeroinitializer
  %244 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %243, <2 x float> %207, <2 x float> %186)
  %245 = add nsw i64 %77, 9
  %246 = getelementptr inbounds float, float* %6, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !48
  %248 = insertelement <2 x float> undef, float %247, i32 0
  %249 = shufflevector <2 x float> %248, <2 x float> undef, <2 x i32> zeroinitializer
  %250 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %249, <2 x float> %207, <2 x float> %187)
  %251 = add nsw i64 %77, 10
  %252 = getelementptr inbounds float, float* %6, i64 %251
  %253 = load float, float* %252, align 4, !tbaa !48
  %254 = insertelement <2 x float> undef, float %253, i32 0
  %255 = shufflevector <2 x float> %254, <2 x float> undef, <2 x i32> zeroinitializer
  %256 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %255, <2 x float> %207, <2 x float> %188)
  %257 = add nsw i64 %77, 11
  %258 = getelementptr inbounds float, float* %6, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !48
  %260 = insertelement <2 x float> undef, float %259, i32 0
  %261 = shufflevector <2 x float> %260, <2 x float> undef, <2 x i32> zeroinitializer
  %262 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %261, <2 x float> %207, <2 x float> %189)
  %263 = add nsw i64 %77, 12
  %264 = getelementptr inbounds float, float* %6, i64 %263
  %265 = load float, float* %264, align 4, !tbaa !48
  %266 = insertelement <2 x float> undef, float %265, i32 0
  %267 = shufflevector <2 x float> %266, <2 x float> undef, <2 x i32> zeroinitializer
  %268 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %267, <2 x float> %207, <2 x float> %190)
  %269 = add nsw i64 %77, 13
  %270 = getelementptr inbounds float, float* %6, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !48
  %272 = insertelement <2 x float> undef, float %271, i32 0
  %273 = shufflevector <2 x float> %272, <2 x float> undef, <2 x i32> zeroinitializer
  %274 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %273, <2 x float> %207, <2 x float> %191)
  %275 = add nsw i64 %77, 14
  %276 = getelementptr inbounds float, float* %6, i64 %275
  %277 = load float, float* %276, align 4, !tbaa !48
  %278 = insertelement <2 x float> undef, float %277, i32 0
  %279 = shufflevector <2 x float> %278, <2 x float> undef, <2 x i32> zeroinitializer
  %280 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %279, <2 x float> %207, <2 x float> %192)
  %281 = add nsw i64 %77, 15
  %282 = getelementptr inbounds float, float* %6, i64 %281
  %283 = load float, float* %282, align 4, !tbaa !48
  %284 = insertelement <2 x float> undef, float %283, i32 0
  %285 = shufflevector <2 x float> %284, <2 x float> undef, <2 x i32> zeroinitializer
  %286 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %285, <2 x float> %207, <2 x float> %193)
  %287 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %199, <2 x float> %207, <2 x float> %194)
  %288 = add nsw i64 %77, 17
  %289 = getelementptr inbounds float, float* %6, i64 %288
  %290 = load float, float* %289, align 4, !tbaa !48
  %291 = insertelement <2 x float> undef, float %290, i32 0
  %292 = shufflevector <2 x float> %291, <2 x float> undef, <2 x i32> zeroinitializer
  %293 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %292, <2 x float> %207, <2 x float> %200)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !38

for_end6:                                         ; preds = %for_body5
  store <2 x float> %208, <2 x float>* %.sub, align 16, !tbaa !54
  store <2 x float> %214, <2 x float>* %26, align 8, !tbaa !54
  store <2 x float> %220, <2 x float>* %28, align 16, !tbaa !54
  store <2 x float> %226, <2 x float>* %30, align 8, !tbaa !54
  store <2 x float> %232, <2 x float>* %32, align 16, !tbaa !54
  store <2 x float> %238, <2 x float>* %34, align 8, !tbaa !54
  store <2 x float> %244, <2 x float>* %36, align 16, !tbaa !54
  store <2 x float> %250, <2 x float>* %38, align 8, !tbaa !54
  store <2 x float> %256, <2 x float>* %40, align 16, !tbaa !54
  store <2 x float> %262, <2 x float>* %42, align 8, !tbaa !54
  store <2 x float> %268, <2 x float>* %44, align 16, !tbaa !54
  store <2 x float> %274, <2 x float>* %46, align 8, !tbaa !54
  store <2 x float> %280, <2 x float>* %48, align 16, !tbaa !54
  store <2 x float> %286, <2 x float>* %50, align 8, !tbaa !54
  store <2 x float> %287, <2 x float>* %52, align 16, !tbaa !54
  store <2 x float> %293, <2 x float>* %54, align 8, !tbaa !54
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep91, i8* nonnull %4, i64 128, i32 8, i1 false)
  %indvars.iv.next93 = add nuw nsw i64 %indvars.iv92, 1
  %exitcond94 = icmp eq i64 %indvars.iv.next93, 4
  br i1 %exitcond94, label %for_end3, label %for_body2, !prof !38
}

; Function Attrs: nounwind readnone speculatable
declare <2 x float> @llvm.fmuladd.v2f32(<2 x float>, <2 x float>, <2 x float>) #3

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i32, i1) #4

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #4

attributes #0 = { noinline }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone speculatable }
attributes #4 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_layout_transform_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!22 = !{!"0x557f3773eee0", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x557f3773ef30", !18, i64 0}
!26 = distinct !DISubprogram(name: "fused_layout_transform_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !27)
!27 = !{!28, !29, !30}
!28 = !DILocalVariable(name: "arg1", arg: 1, scope: !26, file: !1, type: !9)
!29 = !DILocalVariable(name: "arg2", arg: 2, scope: !26, file: !1, type: !9)
!30 = !DILocalVariable(name: "arg3", arg: 3, scope: !26, file: !1, type: !8)
!31 = !DILocation(line: 0, scope: !26)
!32 = !{!33, !33, i64 0}
!33 = !{!"float32", !34, i64 0}
!34 = !{!"0x557f3772c6c0", !18, i64 0}
!35 = !{!36, !36, i64 0}
!36 = !{!"float32", !37, i64 0}
!37 = !{!"0x557f37714610", !18, i64 0}
!38 = !{!"branch_weights", i32 1, i32 1048576}
!39 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !40)
!40 = !{!41, !42, !43}
!41 = !DILocalVariable(name: "arg1", arg: 1, scope: !39, file: !1, type: !9)
!42 = !DILocalVariable(name: "arg2", arg: 2, scope: !39, file: !1, type: !9)
!43 = !DILocalVariable(name: "arg3", arg: 3, scope: !39, file: !1, type: !8)
!44 = !DILocation(line: 0, scope: !39)
!45 = !{!46, !46, i64 0}
!46 = !{!"float32", !47, i64 0}
!47 = !{!"0x557f376bf940", !18, i64 0}
!48 = !{!49, !49, i64 0}
!49 = !{!"float32", !50, i64 0}
!50 = !{!"0x557f37721350", !18, i64 0}
!51 = !{!52, !52, i64 0}
!52 = !{!"float32", !53, i64 0}
!53 = !{!"0x557f37720970", !18, i64 0}
!54 = !{!55, !55, i64 0}
!55 = !{!"0x557f37733050.w2.b0", !56, i64 0}
!56 = !{!"0x557f37733050.w4.b0", !57, i64 0}
!57 = !{!"0x557f37733050.w8.b0", !58, i64 0}
!58 = !{!"0x557f37733050.w16.b0", !59, i64 0}
!59 = !{!"0x557f37733050.w32.b0", !60, i64 0}
!60 = !{!"0x557f37733050.w64.b0", !61, i64 0}
!61 = !{!"0x557f37733050.w128.b0", !62, i64 0}
!62 = !{!"0x557f37733050.w256.b0", !63, i64 0}
!63 = !{!"0x557f37733050.w512.b0", !64, i64 0}
!64 = !{!"0x557f37733050.w1024.b0", !65, i64 0}
!65 = !{!"float32", !66, i64 0}
!66 = !{!"0x557f37733050", !18, i64 0}
