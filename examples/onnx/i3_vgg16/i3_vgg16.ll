; ModuleID = 'fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8'
source_filename = "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i32*, i32 }
%1 = type { i8*, %2, i32, %3, i64*, i64*, i64 }
%2 = type { i32, i32 }
%3 = type { i8, i8, i16 }
%4 = type { i8*, i8* }
%5 = type { i8*, i8*, i8*, i8*, i32 }
%6 = type { i8*, i8* }
%7 = type { i8*, i8* }
%8 = type { i8*, i8*, i8*, i8*, i32 }
%9 = type { i8*, i8* }
%10 = type { i8*, i8*, i8*, i8*, i32 }
%11 = type { i8*, i8* }
%12 = type { i8*, i8* }
%13 = type { i8*, i8* }
%14 = type { i8*, i8*, i8*, i8*, i32 }
%15 = type { i8*, i8* }
%16 = type { i8*, i8*, i8* }
%17 = type { i8*, i8* }
%18 = type { i8*, i8* }
%19 = type { i8*, i8*, i8*, i8*, i32 }
%20 = type { i8*, i8* }
%21 = type { i8*, i8* }
%22 = type { i8*, i8* }
%23 = type { i8*, i8* }
%24 = type { i8*, i8*, i8* }
%25 = type { i8*, i8* }
%26 = type { i8*, i8*, i8*, i8*, i32 }
%27 = type { i8*, i8* }
%28 = type { i8*, i8*, i8* }
%29 = type { i8*, i8* }
%30 = type { i8*, i8*, i8*, i8*, i32 }
%31 = type { i8*, i8* }
%32 = type { i8*, i8*, i8*, i8*, i32 }
%33 = type { i8*, i8* }
%34 = type { i8*, i8* }
%35 = type { i8*, i8* }
%36 = type { i8*, i8*, i8*, i8* }
%37 = type { i8*, i8* }

@__TVMBackendParallelLaunch = linkonce dllexport local_unnamed_addr global i32 (i32 (i32, %0*, i8*)*, i8*, i32)* null, align 8
@__TVMBackendAllocWorkspace = linkonce dllexport local_unnamed_addr global i8* (i32, i32, i64, i32, i32)* null, align 8
@__TVMBackendFreeWorkspace = linkonce dllexport local_unnamed_addr global i32 (i32, i32, i8*)* null, align 8
@__tvm_main__ = weak local_unnamed_addr constant [44 x i8] c"fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8\00", align 1

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !5 {
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
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !15
  %17 = load i32, i32* %16, align 4, !dbg !15
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !15
  %19 = load i8*, i8** %18, align 8, !dbg !15
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !15
  %21 = load i8*, i8** %20, align 8, !dbg !15
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !15
  %23 = load i8*, i8** %22, align 8, !dbg !15
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !15
  ret i32 %24, !dbg !15
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 612912, i32 2, i32 32)
  %7 = alloca %4, align 8
  %8 = getelementptr inbounds %4, %4* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %4, %4* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %4* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %5, align 8
  %15 = getelementptr inbounds %5, %5* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %5, %5* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %5, %5* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %5, %5* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %5, %5* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %5* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.1, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
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
  %10 = add nsw i32 %9, 225
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 226
  %15 = select i1 %14, i32 %13, i32 226
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 226
  %18 = select i1 %17, i32 %16, i32 226
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %93, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 678
  %.off = add i32 %20, -1
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
  %26 = add i32 %25, -1
  %27 = icmp ult i32 %26, 224
  %28 = trunc i64 %24 to i32
  %29 = add i32 %21, %28
  br i1 %27, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 226
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !20

for_body2.split.us.us:                            ; preds = %for_body2.us
  %30 = trunc i64 %24 to i32
  %31 = add i32 %30, -675
  %32 = add i32 %31, %23
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to i32*
  %36 = load i32, i32* %35, align 4, !tbaa !21
  %37 = sext i32 %29 to i64
  %38 = getelementptr inbounds float, float* %4, i64 %37
  %39 = bitcast float* %38 to i32*
  store i32 %36, i32* %39, align 4, !tbaa !24
  %40 = trunc i64 %24 to i32
  %41 = add i32 %40, 1
  %42 = add i32 %21, %41
  %43 = trunc i64 %24 to i32
  %44 = add i32 %43, -674
  %45 = add i32 %44, %23
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds float, float* %7, i64 %46
  %48 = bitcast float* %47 to i32*
  %49 = load i32, i32* %48, align 4, !tbaa !21
  %50 = sext i32 %42 to i64
  %51 = getelementptr inbounds float, float* %4, i64 %50
  %52 = bitcast float* %51 to i32*
  store i32 %49, i32* %52, align 4, !tbaa !24
  %53 = trunc i64 %24 to i32
  %54 = add i32 %53, 2
  %55 = add i32 %21, %54
  %56 = trunc i64 %24 to i32
  %57 = add i32 %56, -673
  %58 = add i32 %57, %23
  %59 = sext i32 %58 to i64
  %60 = getelementptr inbounds float, float* %7, i64 %59
  %61 = bitcast float* %60 to i32*
  %62 = load i32, i32* %61, align 4, !tbaa !21
  %63 = sext i32 %55 to i64
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = bitcast float* %64 to i32*
  store i32 %62, i32* %65, align 4, !tbaa !24
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %66 = sext i32 %29 to i64
  %67 = getelementptr inbounds float, float* %4, i64 %66
  store float 0.000000e+00, float* %67, align 4, !tbaa !24
  %68 = trunc i64 %24 to i32
  %69 = add i32 %68, 1
  %70 = add i32 %69, %21
  %71 = sext i32 %70 to i64
  %72 = getelementptr inbounds float, float* %4, i64 %71
  store float 0.000000e+00, float* %72, align 4, !tbaa !24
  %73 = trunc i64 %24 to i32
  %74 = add i32 %73, 2
  %75 = add i32 %74, %21
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %4, i64 %76
  store float 0.000000e+00, float* %77, align 4, !tbaa !24
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
  store float 0.000000e+00, float* %82, align 4, !tbaa !24
  %83 = trunc i64 %78 to i32
  %84 = add i32 %83, 1
  %85 = add i32 %84, %21
  %86 = sext i32 %85 to i64
  %87 = getelementptr inbounds float, float* %4, i64 %86
  store float 0.000000e+00, float* %87, align 4, !tbaa !24
  %88 = trunc i64 %78 to i32
  %89 = add i32 %88, 2
  %90 = add i32 %89, %21
  %91 = sext i32 %90 to i64
  %92 = getelementptr inbounds float, float* %4, i64 %91
  store float 0.000000e+00, float* %92, align 4, !tbaa !24
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 226
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %93 = add nsw i32 %20, 1
  %94 = icmp slt i32 %93, %15
  br i1 %94, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.1(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 447
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 448
  %24 = select i1 %23, i32 %22, i32 448
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 448
  %27 = select i1 %26, i32 %25, i32 448
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end20
  %29 = phi i32 [ %352, %for_end20 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 28672, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <32 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 128
  %36 = bitcast i8* %35 to <32 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 256
  %38 = bitcast i8* %37 to <32 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 384
  %40 = bitcast i8* %39 to <32 x float>*
  %41 = getelementptr inbounds i8, i8* %33, i64 512
  %42 = bitcast i8* %41 to <32 x float>*
  %43 = getelementptr inbounds i8, i8* %33, i64 640
  %44 = bitcast i8* %43 to <32 x float>*
  %45 = getelementptr inbounds i8, i8* %33, i64 768
  %46 = bitcast i8* %45 to <32 x float>*
  %47 = getelementptr inbounds i8, i8* %33, i64 896
  %48 = bitcast i8* %47 to <32 x float>*
  %49 = srem i32 %29, 224
  %50 = sdiv i32 %29, 224
  %51 = mul nsw i32 %50, 864
  %52 = bitcast i8* %31 to float*
  %53 = sext i32 %51 to i64
  br label %for_body4

for_end:                                          ; preds = %for_end20, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end8, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end8 ]
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  %54 = trunc i64 %indvar to i32
  %55 = mul i32 %54, 24
  br label %for_body7

for_end5:                                         ; preds = %for_end8
  %56 = mul nsw i32 %29, 7168
  %57 = shl nsw i32 %50, 5
  %58 = sext i32 %57 to i64
  %59 = getelementptr inbounds float, float* %13, i64 %58
  %60 = bitcast float* %59 to <32 x float>*
  %61 = load <32 x float>, <32 x float>* %60, align 64, !tbaa !27
  br label %for_body19

for_body7:                                        ; preds = %for_end11, %for_body4
  %indvars.iv79 = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next80, %for_end11 ]
  %.lcssa41.lcssa71 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %250, %for_end11 ]
  %.lcssa39.lcssa69 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %244, %for_end11 ]
  %.lcssa37.lcssa67 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %238, %for_end11 ]
  %.lcssa35.lcssa65 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %232, %for_end11 ]
  %.lcssa33.lcssa63 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %226, %for_end11 ]
  %.lcssa31.lcssa61 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %220, %for_end11 ]
  %.lcssa29.lcssa60 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %214, %for_end11 ]
  %.lcssa.lcssa58 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %208, %for_end11 ]
  %62 = phi i32 [ 0, %for_body4 ], [ %251, %for_end11 ]
  %63 = add nsw i32 %62, %49
  %64 = mul i32 %63, 678
  %65 = add nsw i32 %64, %55
  %66 = mul nuw nsw i64 %indvars.iv79, 288
  %67 = add nsw i64 %66, %53
  %68 = sext i32 %65 to i64
  br label %for_body10

for_end8:                                         ; preds = %for_end11
  store <32 x float> %208, <32 x float>* %34, align 64, !tbaa !30
  store <32 x float> %214, <32 x float>* %36, align 64, !tbaa !30
  store <32 x float> %220, <32 x float>* %38, align 64, !tbaa !30
  store <32 x float> %226, <32 x float>* %40, align 64, !tbaa !30
  store <32 x float> %232, <32 x float>* %42, align 64, !tbaa !30
  store <32 x float> %238, <32 x float>* %44, align 64, !tbaa !30
  store <32 x float> %244, <32 x float>* %46, align 64, !tbaa !30
  store <32 x float> %250, <32 x float>* %48, align 64, !tbaa !30
  %69 = shl i64 %indvar, 8
  %70 = getelementptr inbounds float, float* %52, i64 %69
  %71 = bitcast float* %70 to <32 x float>*
  store <32 x float> %208, <32 x float>* %71, align 64, !tbaa !39
  %72 = or i64 %69, 32
  %73 = getelementptr inbounds float, float* %52, i64 %72
  %74 = bitcast float* %73 to <32 x float>*
  store <32 x float> %214, <32 x float>* %74, align 64, !tbaa !39
  %75 = or i64 %69, 64
  %76 = getelementptr inbounds float, float* %52, i64 %75
  %77 = bitcast float* %76 to <32 x float>*
  store <32 x float> %220, <32 x float>* %77, align 64, !tbaa !39
  %78 = or i64 %69, 96
  %79 = getelementptr inbounds float, float* %52, i64 %78
  %80 = bitcast float* %79 to <32 x float>*
  store <32 x float> %226, <32 x float>* %80, align 64, !tbaa !39
  %81 = or i64 %69, 128
  %82 = getelementptr inbounds float, float* %52, i64 %81
  %83 = bitcast float* %82 to <32 x float>*
  store <32 x float> %232, <32 x float>* %83, align 64, !tbaa !39
  %84 = or i64 %69, 160
  %85 = getelementptr inbounds float, float* %52, i64 %84
  %86 = bitcast float* %85 to <32 x float>*
  store <32 x float> %238, <32 x float>* %86, align 64, !tbaa !39
  %87 = or i64 %69, 192
  %88 = getelementptr inbounds float, float* %52, i64 %87
  %89 = bitcast float* %88 to <32 x float>*
  store <32 x float> %244, <32 x float>* %89, align 64, !tbaa !39
  %90 = or i64 %69, 224
  %91 = getelementptr inbounds float, float* %52, i64 %90
  %92 = bitcast float* %91 to <32 x float>*
  store <32 x float> %250, <32 x float>* %92, align 64, !tbaa !39
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond85 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond85, label %for_end5, label %for_body4, !prof !20

for_body10:                                       ; preds = %for_body10, %for_body7
  %indvars.iv = phi i64 [ 0, %for_body7 ], [ %indvars.iv.next, %for_body10 ]
  %.lcssa4156 = phi <32 x float> [ %.lcssa41.lcssa71, %for_body7 ], [ %250, %for_body10 ]
  %.lcssa3954 = phi <32 x float> [ %.lcssa39.lcssa69, %for_body7 ], [ %244, %for_body10 ]
  %.lcssa3752 = phi <32 x float> [ %.lcssa37.lcssa67, %for_body7 ], [ %238, %for_body10 ]
  %.lcssa3550 = phi <32 x float> [ %.lcssa35.lcssa65, %for_body7 ], [ %232, %for_body10 ]
  %.lcssa3348 = phi <32 x float> [ %.lcssa33.lcssa63, %for_body7 ], [ %226, %for_body10 ]
  %.lcssa3146 = phi <32 x float> [ %.lcssa31.lcssa61, %for_body7 ], [ %220, %for_body10 ]
  %.lcssa2944 = phi <32 x float> [ %.lcssa29.lcssa60, %for_body7 ], [ %214, %for_body10 ]
  %.lcssa43 = phi <32 x float> [ %.lcssa.lcssa58, %for_body7 ], [ %208, %for_body10 ]
  %93 = mul nuw nsw i64 %indvars.iv, 3
  %94 = add nsw i64 %93, %68
  %95 = mul nuw nsw i64 %indvars.iv, 96
  %96 = add nsw i64 %67, %95
  %97 = getelementptr inbounds float, float* %4, i64 %94
  %98 = load float, float* %97, align 4, !tbaa !24
  %99 = insertelement <32 x float> undef, float %98, i32 0
  %100 = shufflevector <32 x float> %99, <32 x float> undef, <32 x i32> zeroinitializer
  %101 = getelementptr inbounds float, float* %7, i64 %96
  %102 = bitcast float* %101 to <32 x float>*
  %103 = load <32 x float>, <32 x float>* %102, align 64, !tbaa !42
  %104 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %100, <32 x float> %103, <32 x float> %.lcssa43)
  %105 = add nsw i64 %94, 3
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = load float, float* %106, align 4, !tbaa !24
  %108 = insertelement <32 x float> undef, float %107, i32 0
  %109 = shufflevector <32 x float> %108, <32 x float> undef, <32 x i32> zeroinitializer
  %110 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %109, <32 x float> %103, <32 x float> %.lcssa2944)
  %111 = add nsw i64 %94, 6
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = load float, float* %112, align 4, !tbaa !24
  %114 = insertelement <32 x float> undef, float %113, i32 0
  %115 = shufflevector <32 x float> %114, <32 x float> undef, <32 x i32> zeroinitializer
  %116 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %115, <32 x float> %103, <32 x float> %.lcssa3146)
  %117 = add nsw i64 %94, 9
  %118 = getelementptr inbounds float, float* %4, i64 %117
  %119 = load float, float* %118, align 4, !tbaa !24
  %120 = insertelement <32 x float> undef, float %119, i32 0
  %121 = shufflevector <32 x float> %120, <32 x float> undef, <32 x i32> zeroinitializer
  %122 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %121, <32 x float> %103, <32 x float> %.lcssa3348)
  %123 = add nsw i64 %94, 12
  %124 = getelementptr inbounds float, float* %4, i64 %123
  %125 = load float, float* %124, align 4, !tbaa !24
  %126 = insertelement <32 x float> undef, float %125, i32 0
  %127 = shufflevector <32 x float> %126, <32 x float> undef, <32 x i32> zeroinitializer
  %128 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %127, <32 x float> %103, <32 x float> %.lcssa3550)
  %129 = add nsw i64 %94, 15
  %130 = getelementptr inbounds float, float* %4, i64 %129
  %131 = load float, float* %130, align 4, !tbaa !24
  %132 = insertelement <32 x float> undef, float %131, i32 0
  %133 = shufflevector <32 x float> %132, <32 x float> undef, <32 x i32> zeroinitializer
  %134 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %133, <32 x float> %103, <32 x float> %.lcssa3752)
  %135 = add nsw i64 %94, 18
  %136 = getelementptr inbounds float, float* %4, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !24
  %138 = insertelement <32 x float> undef, float %137, i32 0
  %139 = shufflevector <32 x float> %138, <32 x float> undef, <32 x i32> zeroinitializer
  %140 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %139, <32 x float> %103, <32 x float> %.lcssa3954)
  %141 = add nsw i64 %94, 21
  %142 = getelementptr inbounds float, float* %4, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !24
  %144 = insertelement <32 x float> undef, float %143, i32 0
  %145 = shufflevector <32 x float> %144, <32 x float> undef, <32 x i32> zeroinitializer
  %146 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %145, <32 x float> %103, <32 x float> %.lcssa4156)
  %147 = add nsw i64 %94, 1
  %148 = getelementptr inbounds float, float* %4, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !24
  %150 = insertelement <32 x float> undef, float %149, i32 0
  %151 = shufflevector <32 x float> %150, <32 x float> undef, <32 x i32> zeroinitializer
  %152 = add nsw i64 %96, 32
  %153 = getelementptr inbounds float, float* %7, i64 %152
  %154 = bitcast float* %153 to <32 x float>*
  %155 = load <32 x float>, <32 x float>* %154, align 64, !tbaa !42
  %156 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %151, <32 x float> %155, <32 x float> %104)
  %157 = add nsw i64 %94, 4
  %158 = getelementptr inbounds float, float* %4, i64 %157
  %159 = load float, float* %158, align 4, !tbaa !24
  %160 = insertelement <32 x float> undef, float %159, i32 0
  %161 = shufflevector <32 x float> %160, <32 x float> undef, <32 x i32> zeroinitializer
  %162 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %161, <32 x float> %155, <32 x float> %110)
  %163 = add nsw i64 %94, 7
  %164 = getelementptr inbounds float, float* %4, i64 %163
  %165 = load float, float* %164, align 4, !tbaa !24
  %166 = insertelement <32 x float> undef, float %165, i32 0
  %167 = shufflevector <32 x float> %166, <32 x float> undef, <32 x i32> zeroinitializer
  %168 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %167, <32 x float> %155, <32 x float> %116)
  %169 = add nsw i64 %94, 10
  %170 = getelementptr inbounds float, float* %4, i64 %169
  %171 = load float, float* %170, align 4, !tbaa !24
  %172 = insertelement <32 x float> undef, float %171, i32 0
  %173 = shufflevector <32 x float> %172, <32 x float> undef, <32 x i32> zeroinitializer
  %174 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %173, <32 x float> %155, <32 x float> %122)
  %175 = add nsw i64 %94, 13
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = load float, float* %176, align 4, !tbaa !24
  %178 = insertelement <32 x float> undef, float %177, i32 0
  %179 = shufflevector <32 x float> %178, <32 x float> undef, <32 x i32> zeroinitializer
  %180 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %179, <32 x float> %155, <32 x float> %128)
  %181 = add nsw i64 %94, 16
  %182 = getelementptr inbounds float, float* %4, i64 %181
  %183 = load float, float* %182, align 4, !tbaa !24
  %184 = insertelement <32 x float> undef, float %183, i32 0
  %185 = shufflevector <32 x float> %184, <32 x float> undef, <32 x i32> zeroinitializer
  %186 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %185, <32 x float> %155, <32 x float> %134)
  %187 = add nsw i64 %94, 19
  %188 = getelementptr inbounds float, float* %4, i64 %187
  %189 = load float, float* %188, align 4, !tbaa !24
  %190 = insertelement <32 x float> undef, float %189, i32 0
  %191 = shufflevector <32 x float> %190, <32 x float> undef, <32 x i32> zeroinitializer
  %192 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %191, <32 x float> %155, <32 x float> %140)
  %193 = add nsw i64 %94, 22
  %194 = getelementptr inbounds float, float* %4, i64 %193
  %195 = load float, float* %194, align 4, !tbaa !24
  %196 = insertelement <32 x float> undef, float %195, i32 0
  %197 = shufflevector <32 x float> %196, <32 x float> undef, <32 x i32> zeroinitializer
  %198 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %197, <32 x float> %155, <32 x float> %146)
  %199 = add nsw i64 %94, 2
  %200 = getelementptr inbounds float, float* %4, i64 %199
  %201 = load float, float* %200, align 4, !tbaa !24
  %202 = insertelement <32 x float> undef, float %201, i32 0
  %203 = shufflevector <32 x float> %202, <32 x float> undef, <32 x i32> zeroinitializer
  %204 = add nsw i64 %96, 64
  %205 = getelementptr inbounds float, float* %7, i64 %204
  %206 = bitcast float* %205 to <32 x float>*
  %207 = load <32 x float>, <32 x float>* %206, align 64, !tbaa !42
  %208 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %203, <32 x float> %207, <32 x float> %156)
  %209 = add nsw i64 %94, 5
  %210 = getelementptr inbounds float, float* %4, i64 %209
  %211 = load float, float* %210, align 4, !tbaa !24
  %212 = insertelement <32 x float> undef, float %211, i32 0
  %213 = shufflevector <32 x float> %212, <32 x float> undef, <32 x i32> zeroinitializer
  %214 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %213, <32 x float> %207, <32 x float> %162)
  %215 = add nsw i64 %94, 8
  %216 = getelementptr inbounds float, float* %4, i64 %215
  %217 = load float, float* %216, align 4, !tbaa !24
  %218 = insertelement <32 x float> undef, float %217, i32 0
  %219 = shufflevector <32 x float> %218, <32 x float> undef, <32 x i32> zeroinitializer
  %220 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %219, <32 x float> %207, <32 x float> %168)
  %221 = add nsw i64 %94, 11
  %222 = getelementptr inbounds float, float* %4, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !24
  %224 = insertelement <32 x float> undef, float %223, i32 0
  %225 = shufflevector <32 x float> %224, <32 x float> undef, <32 x i32> zeroinitializer
  %226 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %225, <32 x float> %207, <32 x float> %174)
  %227 = add nsw i64 %94, 14
  %228 = getelementptr inbounds float, float* %4, i64 %227
  %229 = load float, float* %228, align 4, !tbaa !24
  %230 = insertelement <32 x float> undef, float %229, i32 0
  %231 = shufflevector <32 x float> %230, <32 x float> undef, <32 x i32> zeroinitializer
  %232 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %231, <32 x float> %207, <32 x float> %180)
  %233 = add nsw i64 %94, 17
  %234 = getelementptr inbounds float, float* %4, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !24
  %236 = insertelement <32 x float> undef, float %235, i32 0
  %237 = shufflevector <32 x float> %236, <32 x float> undef, <32 x i32> zeroinitializer
  %238 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %237, <32 x float> %207, <32 x float> %186)
  %239 = add nsw i64 %94, 20
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !24
  %242 = insertelement <32 x float> undef, float %241, i32 0
  %243 = shufflevector <32 x float> %242, <32 x float> undef, <32 x i32> zeroinitializer
  %244 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %243, <32 x float> %207, <32 x float> %192)
  %245 = add nsw i64 %94, 23
  %246 = getelementptr inbounds float, float* %4, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !24
  %248 = insertelement <32 x float> undef, float %247, i32 0
  %249 = shufflevector <32 x float> %248, <32 x float> undef, <32 x i32> zeroinitializer
  %250 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %249, <32 x float> %207, <32 x float> %198)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for_end11, label %for_body10, !prof !20

for_end11:                                        ; preds = %for_body10
  %indvars.iv.next80 = add nuw nsw i64 %indvars.iv79, 1
  %251 = add nuw nsw i32 %62, 1
  %exitcond81 = icmp eq i64 %indvars.iv.next80, 3
  br i1 %exitcond81, label %for_end8, label %for_body7, !prof !20

for_body19:                                       ; preds = %for_body19, %for_end5
  %indvars.iv89 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next90, %for_body19 ]
  %252 = shl nsw i64 %indvars.iv89, 8
  %253 = trunc i64 %252 to i32
  %254 = add i32 %56, %253
  %255 = getelementptr inbounds float, float* %52, i64 %252
  %256 = bitcast float* %255 to <32 x float>*
  %257 = load <32 x float>, <32 x float>* %256, align 64, !tbaa !39
  %258 = fadd <32 x float> %61, %257
  %259 = fcmp ogt <32 x float> %258, zeroinitializer
  %260 = select <32 x i1> %259, <32 x float> %258, <32 x float> zeroinitializer
  %261 = sext i32 %254 to i64
  %262 = getelementptr inbounds float, float* %10, i64 %261
  %263 = bitcast float* %262 to <32 x float>*
  store <32 x float> %260, <32 x float>* %263, align 64, !tbaa !45
  %264 = or i64 %252, 32
  %265 = trunc i64 %264 to i32
  %266 = add i32 %56, %265
  %267 = getelementptr inbounds float, float* %52, i64 %264
  %268 = bitcast float* %267 to <32 x float>*
  %269 = load <32 x float>, <32 x float>* %268, align 64, !tbaa !39
  %270 = fadd <32 x float> %61, %269
  %271 = fcmp ogt <32 x float> %270, zeroinitializer
  %272 = select <32 x i1> %271, <32 x float> %270, <32 x float> zeroinitializer
  %273 = sext i32 %266 to i64
  %274 = getelementptr inbounds float, float* %10, i64 %273
  %275 = bitcast float* %274 to <32 x float>*
  store <32 x float> %272, <32 x float>* %275, align 64, !tbaa !45
  %276 = or i64 %252, 64
  %277 = trunc i64 %276 to i32
  %278 = add i32 %56, %277
  %279 = getelementptr inbounds float, float* %52, i64 %276
  %280 = bitcast float* %279 to <32 x float>*
  %281 = load <32 x float>, <32 x float>* %280, align 64, !tbaa !39
  %282 = fadd <32 x float> %61, %281
  %283 = fcmp ogt <32 x float> %282, zeroinitializer
  %284 = select <32 x i1> %283, <32 x float> %282, <32 x float> zeroinitializer
  %285 = sext i32 %278 to i64
  %286 = getelementptr inbounds float, float* %10, i64 %285
  %287 = bitcast float* %286 to <32 x float>*
  store <32 x float> %284, <32 x float>* %287, align 64, !tbaa !45
  %288 = or i64 %252, 96
  %289 = trunc i64 %288 to i32
  %290 = add i32 %56, %289
  %291 = getelementptr inbounds float, float* %52, i64 %288
  %292 = bitcast float* %291 to <32 x float>*
  %293 = load <32 x float>, <32 x float>* %292, align 64, !tbaa !39
  %294 = fadd <32 x float> %61, %293
  %295 = fcmp ogt <32 x float> %294, zeroinitializer
  %296 = select <32 x i1> %295, <32 x float> %294, <32 x float> zeroinitializer
  %297 = sext i32 %290 to i64
  %298 = getelementptr inbounds float, float* %10, i64 %297
  %299 = bitcast float* %298 to <32 x float>*
  store <32 x float> %296, <32 x float>* %299, align 64, !tbaa !45
  %300 = or i64 %252, 128
  %301 = trunc i64 %300 to i32
  %302 = add i32 %56, %301
  %303 = getelementptr inbounds float, float* %52, i64 %300
  %304 = bitcast float* %303 to <32 x float>*
  %305 = load <32 x float>, <32 x float>* %304, align 64, !tbaa !39
  %306 = fadd <32 x float> %61, %305
  %307 = fcmp ogt <32 x float> %306, zeroinitializer
  %308 = select <32 x i1> %307, <32 x float> %306, <32 x float> zeroinitializer
  %309 = sext i32 %302 to i64
  %310 = getelementptr inbounds float, float* %10, i64 %309
  %311 = bitcast float* %310 to <32 x float>*
  store <32 x float> %308, <32 x float>* %311, align 64, !tbaa !45
  %312 = or i64 %252, 160
  %313 = trunc i64 %312 to i32
  %314 = add i32 %56, %313
  %315 = getelementptr inbounds float, float* %52, i64 %312
  %316 = bitcast float* %315 to <32 x float>*
  %317 = load <32 x float>, <32 x float>* %316, align 64, !tbaa !39
  %318 = fadd <32 x float> %61, %317
  %319 = fcmp ogt <32 x float> %318, zeroinitializer
  %320 = select <32 x i1> %319, <32 x float> %318, <32 x float> zeroinitializer
  %321 = sext i32 %314 to i64
  %322 = getelementptr inbounds float, float* %10, i64 %321
  %323 = bitcast float* %322 to <32 x float>*
  store <32 x float> %320, <32 x float>* %323, align 64, !tbaa !45
  %324 = or i64 %252, 192
  %325 = trunc i64 %324 to i32
  %326 = add i32 %56, %325
  %327 = getelementptr inbounds float, float* %52, i64 %324
  %328 = bitcast float* %327 to <32 x float>*
  %329 = load <32 x float>, <32 x float>* %328, align 64, !tbaa !39
  %330 = fadd <32 x float> %61, %329
  %331 = fcmp ogt <32 x float> %330, zeroinitializer
  %332 = select <32 x i1> %331, <32 x float> %330, <32 x float> zeroinitializer
  %333 = sext i32 %326 to i64
  %334 = getelementptr inbounds float, float* %10, i64 %333
  %335 = bitcast float* %334 to <32 x float>*
  store <32 x float> %332, <32 x float>* %335, align 64, !tbaa !45
  %336 = or i64 %252, 224
  %337 = trunc i64 %336 to i32
  %338 = add i32 %56, %337
  %339 = getelementptr inbounds float, float* %52, i64 %336
  %340 = bitcast float* %339 to <32 x float>*
  %341 = load <32 x float>, <32 x float>* %340, align 64, !tbaa !39
  %342 = fadd <32 x float> %61, %341
  %343 = fcmp ogt <32 x float> %342, zeroinitializer
  %344 = select <32 x i1> %343, <32 x float> %342, <32 x float> zeroinitializer
  %345 = sext i32 %338 to i64
  %346 = getelementptr inbounds float, float* %10, i64 %345
  %347 = bitcast float* %346 to <32 x float>*
  store <32 x float> %344, <32 x float>* %347, align 64, !tbaa !45
  %indvars.iv.next90 = add nuw nsw i64 %indvars.iv89, 1
  %exitcond91 = icmp eq i64 %indvars.iv.next90, 28
  br i1 %exitcond91, label %for_end20, label %for_body19, !prof !20

for_end20:                                        ; preds = %for_body19
  %348 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %349 = tail call i32 %348(i32 1, i32 %16, i8* %33)
  %350 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %351 = tail call i32 %350(i32 1, i32 %16, i8* nonnull %31)
  %352 = add nsw i32 %29, 1
  %353 = icmp slt i32 %352, %24
  br i1 %353, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind readnone speculatable
declare <32 x float> @llvm.fmuladd.v32f32(<32 x float>, <32 x float>, <32 x float>) #2

define dllexport i32 @fused_layout_transform_19(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !48 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !50, metadata !DIExpression()), !dbg !53
  call void @llvm.dbg.value(metadata i8* %1, metadata !51, metadata !DIExpression()), !dbg !53
  call void @llvm.dbg.value(metadata i32 %2, metadata !52, metadata !DIExpression()), !dbg !53
  %3 = bitcast i8* %0 to %1**, !dbg !53
  %4 = load %1*, %1** %3, align 8, !dbg !53
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !53
  %6 = bitcast i8* %5 to %1**, !dbg !53
  %7 = load %1*, %1** %6, align 8, !dbg !53
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !53
  %9 = load i8*, i8** %8, align 8, !dbg !53
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !53
  %11 = load i8*, i8** %10, align 8, !dbg !53
  %12 = tail call fastcc i32 @fused_layout_transform_19_compute_(i8* %11, i8* %9), !dbg !53
  ret i32 %12, !dbg !53
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_19_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %6, align 8
  %3 = getelementptr inbounds %6, %6* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %6, %6* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %6* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.2, i8* nonnull %5, i32 0)
  ret i32 %7
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
  %10 = add nsw i32 %9, 895
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 896
  %15 = select i1 %14, i32 %13, i32 896
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 896
  %18 = select i1 %17, i32 %16, i32 896
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
  %24 = mul nsw i64 %indvars.iv7, 3584
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = sdiv i32 %25, 224
  %27 = shl nsw i32 %26, 4
  %28 = srem i32 %25, 224
  %29 = mul nsw i32 %28, 7168
  %30 = srem i32 %27, 32
  %31 = sdiv i32 %25, 448
  %32 = mul nsw i32 %31, 1605632
  %33 = insertelement <8 x i32> undef, i32 %27, i32 0
  %34 = shufflevector <8 x i32> %33, <8 x i32> undef, <8 x i32> zeroinitializer
  %35 = or <8 x i32> %34, <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %36 = sdiv <8 x i32> %35, <i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32>
  %37 = mul <8 x i32> %36, <i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32>
  %38 = sub <8 x i32> %35, %37
  %39 = mul nsw <8 x i32> %36, <i32 1605632, i32 1605632, i32 1605632, i32 1605632, i32 1605632, i32 1605632, i32 1605632, i32 1605632>
  %40 = insertelement <4 x i32> undef, i32 %27, i32 0
  %41 = shufflevector <4 x i32> %40, <4 x i32> undef, <4 x i32> zeroinitializer
  %42 = or <4 x i32> %41, <i32 9, i32 10, i32 11, i32 12>
  %43 = sdiv <4 x i32> %42, <i32 32, i32 32, i32 32, i32 32>
  %44 = mul <4 x i32> %43, <i32 32, i32 32, i32 32, i32 32>
  %45 = sub <4 x i32> %42, %44
  %46 = mul nsw <4 x i32> %43, <i32 1605632, i32 1605632, i32 1605632, i32 1605632>
  %47 = or i32 %27, 13
  %48 = srem i32 %47, 32
  %49 = sdiv i32 %47, 32
  %50 = mul nsw i32 %49, 1605632
  %51 = or i32 %27, 14
  %52 = srem i32 %51, 32
  %53 = sdiv i32 %51, 32
  %54 = mul nsw i32 %53, 1605632
  %55 = or i32 %27, 15
  %56 = srem i32 %55, 32
  %57 = sdiv i32 %55, 32
  %58 = mul nsw i32 %57, 1605632
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %59 = phi i32 [ 0, %for_body ], [ %205, %for_body2 ]
  %60 = shl i64 %indvars.iv, 4
  %61 = add nsw i64 %60, %24
  %62 = shl i32 %59, 5
  %63 = add nsw i32 %62, %29
  %64 = add i32 %63, %30
  %65 = add i32 %64, %32
  %66 = sext i32 %65 to i64
  %67 = getelementptr inbounds float, float* %7, i64 %66
  %68 = bitcast float* %67 to i32*
  %69 = load i32, i32* %68, align 4, !tbaa !54
  %70 = getelementptr inbounds float, float* %4, i64 %61
  %71 = bitcast float* %70 to i32*
  store i32 %69, i32* %71, align 4, !tbaa !57
  %72 = or i64 %61, 1
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %73 = shl i32 %indvars.iv.tr, 5
  %74 = add i32 %29, %73
  %75 = insertelement <8 x i32> undef, i32 %74, i32 0
  %76 = shufflevector <8 x i32> %75, <8 x i32> undef, <8 x i32> zeroinitializer
  %77 = add <8 x i32> %76, %38
  %78 = add <8 x i32> %77, %39
  %79 = extractelement <8 x i32> %78, i32 0
  %80 = sext i32 %79 to i64
  %81 = getelementptr inbounds float, float* %7, i64 %80
  %82 = bitcast float* %81 to i32*
  %83 = load i32, i32* %82, align 4, !tbaa !54
  %84 = getelementptr inbounds float, float* %4, i64 %72
  %85 = bitcast float* %84 to i32*
  store i32 %83, i32* %85, align 4, !tbaa !57
  %86 = or i64 %61, 2
  %87 = extractelement <8 x i32> %78, i32 1
  %88 = sext i32 %87 to i64
  %89 = getelementptr inbounds float, float* %7, i64 %88
  %90 = bitcast float* %89 to i32*
  %91 = load i32, i32* %90, align 4, !tbaa !54
  %92 = getelementptr inbounds float, float* %4, i64 %86
  %93 = bitcast float* %92 to i32*
  store i32 %91, i32* %93, align 4, !tbaa !57
  %94 = or i64 %61, 3
  %95 = extractelement <8 x i32> %78, i32 2
  %96 = sext i32 %95 to i64
  %97 = getelementptr inbounds float, float* %7, i64 %96
  %98 = bitcast float* %97 to i32*
  %99 = load i32, i32* %98, align 4, !tbaa !54
  %100 = getelementptr inbounds float, float* %4, i64 %94
  %101 = bitcast float* %100 to i32*
  store i32 %99, i32* %101, align 4, !tbaa !57
  %102 = or i64 %61, 4
  %103 = extractelement <8 x i32> %78, i32 3
  %104 = sext i32 %103 to i64
  %105 = getelementptr inbounds float, float* %7, i64 %104
  %106 = bitcast float* %105 to i32*
  %107 = load i32, i32* %106, align 4, !tbaa !54
  %108 = getelementptr inbounds float, float* %4, i64 %102
  %109 = bitcast float* %108 to i32*
  store i32 %107, i32* %109, align 4, !tbaa !57
  %110 = or i64 %61, 5
  %111 = extractelement <8 x i32> %78, i32 4
  %112 = sext i32 %111 to i64
  %113 = getelementptr inbounds float, float* %7, i64 %112
  %114 = bitcast float* %113 to i32*
  %115 = load i32, i32* %114, align 4, !tbaa !54
  %116 = getelementptr inbounds float, float* %4, i64 %110
  %117 = bitcast float* %116 to i32*
  store i32 %115, i32* %117, align 4, !tbaa !57
  %118 = or i64 %61, 6
  %119 = extractelement <8 x i32> %78, i32 5
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds float, float* %7, i64 %120
  %122 = bitcast float* %121 to i32*
  %123 = load i32, i32* %122, align 4, !tbaa !54
  %124 = getelementptr inbounds float, float* %4, i64 %118
  %125 = bitcast float* %124 to i32*
  store i32 %123, i32* %125, align 4, !tbaa !57
  %126 = or i64 %61, 7
  %127 = extractelement <8 x i32> %78, i32 6
  %128 = sext i32 %127 to i64
  %129 = getelementptr inbounds float, float* %7, i64 %128
  %130 = bitcast float* %129 to i32*
  %131 = load i32, i32* %130, align 4, !tbaa !54
  %132 = getelementptr inbounds float, float* %4, i64 %126
  %133 = bitcast float* %132 to i32*
  store i32 %131, i32* %133, align 4, !tbaa !57
  %134 = or i64 %61, 8
  %135 = extractelement <8 x i32> %78, i32 7
  %136 = sext i32 %135 to i64
  %137 = getelementptr inbounds float, float* %7, i64 %136
  %138 = bitcast float* %137 to i32*
  %139 = load i32, i32* %138, align 4, !tbaa !54
  %140 = getelementptr inbounds float, float* %4, i64 %134
  %141 = bitcast float* %140 to i32*
  store i32 %139, i32* %141, align 4, !tbaa !57
  %142 = or i64 %61, 9
  %143 = insertelement <4 x i32> undef, i32 %74, i32 0
  %144 = shufflevector <4 x i32> %143, <4 x i32> undef, <4 x i32> zeroinitializer
  %145 = add <4 x i32> %144, %45
  %146 = add <4 x i32> %145, %46
  %147 = extractelement <4 x i32> %146, i32 0
  %148 = sext i32 %147 to i64
  %149 = getelementptr inbounds float, float* %7, i64 %148
  %150 = bitcast float* %149 to i32*
  %151 = load i32, i32* %150, align 4, !tbaa !54
  %152 = getelementptr inbounds float, float* %4, i64 %142
  %153 = bitcast float* %152 to i32*
  store i32 %151, i32* %153, align 4, !tbaa !57
  %154 = or i64 %61, 10
  %155 = extractelement <4 x i32> %146, i32 1
  %156 = sext i32 %155 to i64
  %157 = getelementptr inbounds float, float* %7, i64 %156
  %158 = bitcast float* %157 to i32*
  %159 = load i32, i32* %158, align 4, !tbaa !54
  %160 = getelementptr inbounds float, float* %4, i64 %154
  %161 = bitcast float* %160 to i32*
  store i32 %159, i32* %161, align 4, !tbaa !57
  %162 = or i64 %61, 11
  %163 = extractelement <4 x i32> %146, i32 2
  %164 = sext i32 %163 to i64
  %165 = getelementptr inbounds float, float* %7, i64 %164
  %166 = bitcast float* %165 to i32*
  %167 = load i32, i32* %166, align 4, !tbaa !54
  %168 = getelementptr inbounds float, float* %4, i64 %162
  %169 = bitcast float* %168 to i32*
  store i32 %167, i32* %169, align 4, !tbaa !57
  %170 = or i64 %61, 12
  %171 = extractelement <4 x i32> %146, i32 3
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds float, float* %7, i64 %172
  %174 = bitcast float* %173 to i32*
  %175 = load i32, i32* %174, align 4, !tbaa !54
  %176 = getelementptr inbounds float, float* %4, i64 %170
  %177 = bitcast float* %176 to i32*
  store i32 %175, i32* %177, align 4, !tbaa !57
  %178 = or i64 %61, 13
  %179 = add i32 %74, %48
  %180 = add i32 %179, %50
  %181 = sext i32 %180 to i64
  %182 = getelementptr inbounds float, float* %7, i64 %181
  %183 = bitcast float* %182 to i32*
  %184 = load i32, i32* %183, align 4, !tbaa !54
  %185 = getelementptr inbounds float, float* %4, i64 %178
  %186 = bitcast float* %185 to i32*
  store i32 %184, i32* %186, align 4, !tbaa !57
  %187 = or i64 %61, 14
  %188 = add i32 %74, %52
  %189 = add i32 %188, %54
  %190 = sext i32 %189 to i64
  %191 = getelementptr inbounds float, float* %7, i64 %190
  %192 = bitcast float* %191 to i32*
  %193 = load i32, i32* %192, align 4, !tbaa !54
  %194 = getelementptr inbounds float, float* %4, i64 %187
  %195 = bitcast float* %194 to i32*
  store i32 %193, i32* %195, align 4, !tbaa !57
  %196 = or i64 %61, 15
  %197 = add i32 %74, %56
  %198 = add i32 %197, %58
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %7, i64 %199
  %201 = bitcast float* %200 to i32*
  %202 = load i32, i32* %201, align 4, !tbaa !54
  %203 = getelementptr inbounds float, float* %4, i64 %196
  %204 = bitcast float* %203 to i32*
  store i32 %202, i32* %204, align 4, !tbaa !57
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %205 = add nuw nsw i32 %59, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 224
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %206 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %206, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_7(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !60 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !62, metadata !DIExpression()), !dbg !65
  call void @llvm.dbg.value(metadata i8* %1, metadata !63, metadata !DIExpression()), !dbg !65
  call void @llvm.dbg.value(metadata i32 %2, metadata !64, metadata !DIExpression()), !dbg !65
  %3 = bitcast i8* %0 to %1**, !dbg !65
  %4 = load %1*, %1** %3, align 8, !dbg !65
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !65
  %6 = bitcast i8* %5 to %1**, !dbg !65
  %7 = load %1*, %1** %6, align 8, !dbg !65
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !65
  %9 = bitcast i8* %8 to %1**, !dbg !65
  %10 = load %1*, %1** %9, align 8, !dbg !65
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !65
  %12 = bitcast i8* %11 to %1**, !dbg !65
  %13 = load %1*, %1** %12, align 8, !dbg !65
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !65
  %15 = load i8*, i8** %14, align 8, !dbg !65
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !65
  %17 = load i32, i32* %16, align 4, !dbg !65
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !65
  %19 = load i8*, i8** %18, align 8, !dbg !65
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !65
  %21 = load i8*, i8** %20, align 8, !dbg !65
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !65
  %23 = load i8*, i8** %22, align 8, !dbg !65
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_7_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !65
  ret i32 %24, !dbg !65
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_7_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 13075456, i32 2, i32 32)
  %7 = alloca %7, align 8
  %8 = getelementptr inbounds %7, %7* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %7, %7* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %7* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.3, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %8, align 8
  %15 = getelementptr inbounds %8, %8* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %8, %8* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %8, %8* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %8, %8* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %8, %8* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %8* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.4, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.3(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 903
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 904
  %15 = select i1 %14, i32 %13, i32 904
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 904
  %18 = select i1 %17, i32 %16, i32 904
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %411, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 3616
  %22 = srem i32 %20, 226
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 224
  %24 = sdiv i32 %20, 226
  %25 = mul nsw i32 %24, 802816
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 3584
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 4
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 224
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %for_body2.split.us.us, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %for_body2.for_body2.split_crit_edge.us, %for_body2.split.us.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 226
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !20

for_body2.split.us.us:                            ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -3600
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to i32*
  %40 = load i32, i32* %39, align 4, !tbaa !66
  %41 = sext i32 %32 to i64
  %42 = getelementptr inbounds float, float* %4, i64 %41
  %43 = bitcast float* %42 to i32*
  store i32 %40, i32* %43, align 4, !tbaa !69
  %44 = or i64 %27, 1
  %45 = trunc i64 %44 to i32
  %46 = add i32 %21, %45
  %47 = trunc i64 %44 to i32
  %48 = add i32 %47, -3600
  %49 = add i32 %26, %48
  %50 = add i32 %49, %25
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to i32*
  %54 = load i32, i32* %53, align 4, !tbaa !66
  %55 = sext i32 %46 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to i32*
  store i32 %54, i32* %57, align 4, !tbaa !69
  %58 = or i64 %27, 2
  %59 = trunc i64 %58 to i32
  %60 = add i32 %21, %59
  %61 = trunc i64 %58 to i32
  %62 = add i32 %61, -3600
  %63 = add i32 %26, %62
  %64 = add i32 %63, %25
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds float, float* %7, i64 %65
  %67 = bitcast float* %66 to i32*
  %68 = load i32, i32* %67, align 4, !tbaa !66
  %69 = sext i32 %60 to i64
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = bitcast float* %70 to i32*
  store i32 %68, i32* %71, align 4, !tbaa !69
  %72 = or i64 %27, 3
  %73 = trunc i64 %72 to i32
  %74 = add i32 %21, %73
  %75 = trunc i64 %72 to i32
  %76 = add i32 %75, -3600
  %77 = add i32 %26, %76
  %78 = add i32 %77, %25
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !66
  %83 = sext i32 %74 to i64
  %84 = getelementptr inbounds float, float* %4, i64 %83
  %85 = bitcast float* %84 to i32*
  store i32 %82, i32* %85, align 4, !tbaa !69
  %86 = or i64 %27, 4
  %87 = trunc i64 %86 to i32
  %88 = add i32 %21, %87
  %89 = trunc i64 %86 to i32
  %90 = add i32 %89, -3600
  %91 = add i32 %26, %90
  %92 = add i32 %91, %25
  %93 = sext i32 %92 to i64
  %94 = getelementptr inbounds float, float* %7, i64 %93
  %95 = bitcast float* %94 to i32*
  %96 = load i32, i32* %95, align 4, !tbaa !66
  %97 = sext i32 %88 to i64
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = bitcast float* %98 to i32*
  store i32 %96, i32* %99, align 4, !tbaa !69
  %100 = or i64 %27, 5
  %101 = trunc i64 %100 to i32
  %102 = add i32 %21, %101
  %103 = trunc i64 %100 to i32
  %104 = add i32 %103, -3600
  %105 = add i32 %26, %104
  %106 = add i32 %105, %25
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to i32*
  %110 = load i32, i32* %109, align 4, !tbaa !66
  %111 = sext i32 %102 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to i32*
  store i32 %110, i32* %113, align 4, !tbaa !69
  %114 = or i64 %27, 6
  %115 = trunc i64 %114 to i32
  %116 = add i32 %21, %115
  %117 = trunc i64 %114 to i32
  %118 = add i32 %117, -3600
  %119 = add i32 %26, %118
  %120 = add i32 %119, %25
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to i32*
  %124 = load i32, i32* %123, align 4, !tbaa !66
  %125 = sext i32 %116 to i64
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = bitcast float* %126 to i32*
  store i32 %124, i32* %127, align 4, !tbaa !69
  %128 = or i64 %27, 7
  %129 = trunc i64 %128 to i32
  %130 = add i32 %21, %129
  %131 = trunc i64 %128 to i32
  %132 = add i32 %131, -3600
  %133 = add i32 %26, %132
  %134 = add i32 %133, %25
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %7, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !66
  %139 = sext i32 %130 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to i32*
  store i32 %138, i32* %141, align 4, !tbaa !69
  %142 = or i64 %27, 8
  %143 = trunc i64 %142 to i32
  %144 = add i32 %21, %143
  %145 = trunc i64 %142 to i32
  %146 = add i32 %145, -3600
  %147 = add i32 %26, %146
  %148 = add i32 %147, %25
  %149 = sext i32 %148 to i64
  %150 = getelementptr inbounds float, float* %7, i64 %149
  %151 = bitcast float* %150 to i32*
  %152 = load i32, i32* %151, align 4, !tbaa !66
  %153 = sext i32 %144 to i64
  %154 = getelementptr inbounds float, float* %4, i64 %153
  %155 = bitcast float* %154 to i32*
  store i32 %152, i32* %155, align 4, !tbaa !69
  %156 = or i64 %27, 9
  %157 = trunc i64 %156 to i32
  %158 = add i32 %21, %157
  %159 = trunc i64 %156 to i32
  %160 = add i32 %159, -3600
  %161 = add i32 %26, %160
  %162 = add i32 %161, %25
  %163 = sext i32 %162 to i64
  %164 = getelementptr inbounds float, float* %7, i64 %163
  %165 = bitcast float* %164 to i32*
  %166 = load i32, i32* %165, align 4, !tbaa !66
  %167 = sext i32 %158 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  %169 = bitcast float* %168 to i32*
  store i32 %166, i32* %169, align 4, !tbaa !69
  %170 = or i64 %27, 10
  %171 = trunc i64 %170 to i32
  %172 = add i32 %21, %171
  %173 = trunc i64 %170 to i32
  %174 = add i32 %173, -3600
  %175 = add i32 %26, %174
  %176 = add i32 %175, %25
  %177 = sext i32 %176 to i64
  %178 = getelementptr inbounds float, float* %7, i64 %177
  %179 = bitcast float* %178 to i32*
  %180 = load i32, i32* %179, align 4, !tbaa !66
  %181 = sext i32 %172 to i64
  %182 = getelementptr inbounds float, float* %4, i64 %181
  %183 = bitcast float* %182 to i32*
  store i32 %180, i32* %183, align 4, !tbaa !69
  %184 = or i64 %27, 11
  %185 = trunc i64 %184 to i32
  %186 = add i32 %21, %185
  %187 = trunc i64 %184 to i32
  %188 = add i32 %187, -3600
  %189 = add i32 %26, %188
  %190 = add i32 %189, %25
  %191 = sext i32 %190 to i64
  %192 = getelementptr inbounds float, float* %7, i64 %191
  %193 = bitcast float* %192 to i32*
  %194 = load i32, i32* %193, align 4, !tbaa !66
  %195 = sext i32 %186 to i64
  %196 = getelementptr inbounds float, float* %4, i64 %195
  %197 = bitcast float* %196 to i32*
  store i32 %194, i32* %197, align 4, !tbaa !69
  %198 = or i64 %27, 12
  %199 = trunc i64 %198 to i32
  %200 = add i32 %21, %199
  %201 = trunc i64 %198 to i32
  %202 = add i32 %201, -3600
  %203 = add i32 %26, %202
  %204 = add i32 %203, %25
  %205 = sext i32 %204 to i64
  %206 = getelementptr inbounds float, float* %7, i64 %205
  %207 = bitcast float* %206 to i32*
  %208 = load i32, i32* %207, align 4, !tbaa !66
  %209 = sext i32 %200 to i64
  %210 = getelementptr inbounds float, float* %4, i64 %209
  %211 = bitcast float* %210 to i32*
  store i32 %208, i32* %211, align 4, !tbaa !69
  %212 = or i64 %27, 13
  %213 = trunc i64 %212 to i32
  %214 = add i32 %21, %213
  %215 = trunc i64 %212 to i32
  %216 = add i32 %215, -3600
  %217 = add i32 %26, %216
  %218 = add i32 %217, %25
  %219 = sext i32 %218 to i64
  %220 = getelementptr inbounds float, float* %7, i64 %219
  %221 = bitcast float* %220 to i32*
  %222 = load i32, i32* %221, align 4, !tbaa !66
  %223 = sext i32 %214 to i64
  %224 = getelementptr inbounds float, float* %4, i64 %223
  %225 = bitcast float* %224 to i32*
  store i32 %222, i32* %225, align 4, !tbaa !69
  %226 = or i64 %27, 14
  %227 = trunc i64 %226 to i32
  %228 = add i32 %21, %227
  %229 = trunc i64 %226 to i32
  %230 = add i32 %229, -3600
  %231 = add i32 %26, %230
  %232 = add i32 %231, %25
  %233 = sext i32 %232 to i64
  %234 = getelementptr inbounds float, float* %7, i64 %233
  %235 = bitcast float* %234 to i32*
  %236 = load i32, i32* %235, align 4, !tbaa !66
  %237 = sext i32 %228 to i64
  %238 = getelementptr inbounds float, float* %4, i64 %237
  %239 = bitcast float* %238 to i32*
  store i32 %236, i32* %239, align 4, !tbaa !69
  %240 = or i64 %27, 15
  %241 = trunc i64 %240 to i32
  %242 = add i32 %21, %241
  %243 = trunc i64 %240 to i32
  %244 = add i32 %243, -3600
  %245 = add i32 %26, %244
  %246 = add i32 %245, %25
  %247 = sext i32 %246 to i64
  %248 = getelementptr inbounds float, float* %7, i64 %247
  %249 = bitcast float* %248 to i32*
  %250 = load i32, i32* %249, align 4, !tbaa !66
  %251 = sext i32 %242 to i64
  %252 = getelementptr inbounds float, float* %4, i64 %251
  %253 = bitcast float* %252 to i32*
  store i32 %250, i32* %253, align 4, !tbaa !69
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %254 = sext i32 %32 to i64
  %255 = getelementptr inbounds float, float* %4, i64 %254
  store float 0.000000e+00, float* %255, align 4, !tbaa !69
  %256 = trunc i64 %27 to i32
  %257 = or i32 %256, 1
  %258 = add i32 %257, %21
  %259 = sext i32 %258 to i64
  %260 = getelementptr inbounds float, float* %4, i64 %259
  store float 0.000000e+00, float* %260, align 4, !tbaa !69
  %261 = trunc i64 %27 to i32
  %262 = or i32 %261, 2
  %263 = add i32 %262, %21
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds float, float* %4, i64 %264
  store float 0.000000e+00, float* %265, align 4, !tbaa !69
  %266 = trunc i64 %27 to i32
  %267 = or i32 %266, 3
  %268 = add i32 %267, %21
  %269 = sext i32 %268 to i64
  %270 = getelementptr inbounds float, float* %4, i64 %269
  store float 0.000000e+00, float* %270, align 4, !tbaa !69
  %271 = trunc i64 %27 to i32
  %272 = or i32 %271, 4
  %273 = add i32 %272, %21
  %274 = sext i32 %273 to i64
  %275 = getelementptr inbounds float, float* %4, i64 %274
  store float 0.000000e+00, float* %275, align 4, !tbaa !69
  %276 = trunc i64 %27 to i32
  %277 = or i32 %276, 5
  %278 = add i32 %277, %21
  %279 = sext i32 %278 to i64
  %280 = getelementptr inbounds float, float* %4, i64 %279
  store float 0.000000e+00, float* %280, align 4, !tbaa !69
  %281 = trunc i64 %27 to i32
  %282 = or i32 %281, 6
  %283 = add i32 %282, %21
  %284 = sext i32 %283 to i64
  %285 = getelementptr inbounds float, float* %4, i64 %284
  store float 0.000000e+00, float* %285, align 4, !tbaa !69
  %286 = trunc i64 %27 to i32
  %287 = or i32 %286, 7
  %288 = add i32 %287, %21
  %289 = sext i32 %288 to i64
  %290 = getelementptr inbounds float, float* %4, i64 %289
  store float 0.000000e+00, float* %290, align 4, !tbaa !69
  %291 = trunc i64 %27 to i32
  %292 = or i32 %291, 8
  %293 = add i32 %292, %21
  %294 = sext i32 %293 to i64
  %295 = getelementptr inbounds float, float* %4, i64 %294
  store float 0.000000e+00, float* %295, align 4, !tbaa !69
  %296 = trunc i64 %27 to i32
  %297 = or i32 %296, 9
  %298 = add i32 %297, %21
  %299 = sext i32 %298 to i64
  %300 = getelementptr inbounds float, float* %4, i64 %299
  store float 0.000000e+00, float* %300, align 4, !tbaa !69
  %301 = trunc i64 %27 to i32
  %302 = or i32 %301, 10
  %303 = add i32 %302, %21
  %304 = sext i32 %303 to i64
  %305 = getelementptr inbounds float, float* %4, i64 %304
  store float 0.000000e+00, float* %305, align 4, !tbaa !69
  %306 = trunc i64 %27 to i32
  %307 = or i32 %306, 11
  %308 = add i32 %307, %21
  %309 = sext i32 %308 to i64
  %310 = getelementptr inbounds float, float* %4, i64 %309
  store float 0.000000e+00, float* %310, align 4, !tbaa !69
  %311 = trunc i64 %27 to i32
  %312 = or i32 %311, 12
  %313 = add i32 %312, %21
  %314 = sext i32 %313 to i64
  %315 = getelementptr inbounds float, float* %4, i64 %314
  store float 0.000000e+00, float* %315, align 4, !tbaa !69
  %316 = trunc i64 %27 to i32
  %317 = or i32 %316, 13
  %318 = add i32 %317, %21
  %319 = sext i32 %318 to i64
  %320 = getelementptr inbounds float, float* %4, i64 %319
  store float 0.000000e+00, float* %320, align 4, !tbaa !69
  %321 = trunc i64 %27 to i32
  %322 = or i32 %321, 14
  %323 = add i32 %322, %21
  %324 = sext i32 %323 to i64
  %325 = getelementptr inbounds float, float* %4, i64 %324
  store float 0.000000e+00, float* %325, align 4, !tbaa !69
  %326 = trunc i64 %27 to i32
  %327 = or i32 %326, 15
  %328 = add i32 %327, %21
  %329 = sext i32 %328 to i64
  %330 = getelementptr inbounds float, float* %4, i64 %329
  store float 0.000000e+00, float* %330, align 4, !tbaa !69
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %331 = shl nsw i64 %indvars.iv, 4
  %332 = trunc i64 %331 to i32
  %333 = add i32 %21, %332
  %334 = sext i32 %333 to i64
  %335 = getelementptr inbounds float, float* %4, i64 %334
  store float 0.000000e+00, float* %335, align 4, !tbaa !69
  %336 = trunc i64 %331 to i32
  %337 = or i32 %336, 1
  %338 = add i32 %337, %21
  %339 = sext i32 %338 to i64
  %340 = getelementptr inbounds float, float* %4, i64 %339
  store float 0.000000e+00, float* %340, align 4, !tbaa !69
  %341 = trunc i64 %331 to i32
  %342 = or i32 %341, 2
  %343 = add i32 %342, %21
  %344 = sext i32 %343 to i64
  %345 = getelementptr inbounds float, float* %4, i64 %344
  store float 0.000000e+00, float* %345, align 4, !tbaa !69
  %346 = trunc i64 %331 to i32
  %347 = or i32 %346, 3
  %348 = add i32 %347, %21
  %349 = sext i32 %348 to i64
  %350 = getelementptr inbounds float, float* %4, i64 %349
  store float 0.000000e+00, float* %350, align 4, !tbaa !69
  %351 = trunc i64 %331 to i32
  %352 = or i32 %351, 4
  %353 = add i32 %352, %21
  %354 = sext i32 %353 to i64
  %355 = getelementptr inbounds float, float* %4, i64 %354
  store float 0.000000e+00, float* %355, align 4, !tbaa !69
  %356 = trunc i64 %331 to i32
  %357 = or i32 %356, 5
  %358 = add i32 %357, %21
  %359 = sext i32 %358 to i64
  %360 = getelementptr inbounds float, float* %4, i64 %359
  store float 0.000000e+00, float* %360, align 4, !tbaa !69
  %361 = trunc i64 %331 to i32
  %362 = or i32 %361, 6
  %363 = add i32 %362, %21
  %364 = sext i32 %363 to i64
  %365 = getelementptr inbounds float, float* %4, i64 %364
  store float 0.000000e+00, float* %365, align 4, !tbaa !69
  %366 = trunc i64 %331 to i32
  %367 = or i32 %366, 7
  %368 = add i32 %367, %21
  %369 = sext i32 %368 to i64
  %370 = getelementptr inbounds float, float* %4, i64 %369
  store float 0.000000e+00, float* %370, align 4, !tbaa !69
  %371 = trunc i64 %331 to i32
  %372 = or i32 %371, 8
  %373 = add i32 %372, %21
  %374 = sext i32 %373 to i64
  %375 = getelementptr inbounds float, float* %4, i64 %374
  store float 0.000000e+00, float* %375, align 4, !tbaa !69
  %376 = trunc i64 %331 to i32
  %377 = or i32 %376, 9
  %378 = add i32 %377, %21
  %379 = sext i32 %378 to i64
  %380 = getelementptr inbounds float, float* %4, i64 %379
  store float 0.000000e+00, float* %380, align 4, !tbaa !69
  %381 = trunc i64 %331 to i32
  %382 = or i32 %381, 10
  %383 = add i32 %382, %21
  %384 = sext i32 %383 to i64
  %385 = getelementptr inbounds float, float* %4, i64 %384
  store float 0.000000e+00, float* %385, align 4, !tbaa !69
  %386 = trunc i64 %331 to i32
  %387 = or i32 %386, 11
  %388 = add i32 %387, %21
  %389 = sext i32 %388 to i64
  %390 = getelementptr inbounds float, float* %4, i64 %389
  store float 0.000000e+00, float* %390, align 4, !tbaa !69
  %391 = trunc i64 %331 to i32
  %392 = or i32 %391, 12
  %393 = add i32 %392, %21
  %394 = sext i32 %393 to i64
  %395 = getelementptr inbounds float, float* %4, i64 %394
  store float 0.000000e+00, float* %395, align 4, !tbaa !69
  %396 = trunc i64 %331 to i32
  %397 = or i32 %396, 13
  %398 = add i32 %397, %21
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds float, float* %4, i64 %399
  store float 0.000000e+00, float* %400, align 4, !tbaa !69
  %401 = trunc i64 %331 to i32
  %402 = or i32 %401, 14
  %403 = add i32 %402, %21
  %404 = sext i32 %403 to i64
  %405 = getelementptr inbounds float, float* %4, i64 %404
  store float 0.000000e+00, float* %405, align 4, !tbaa !69
  %406 = trunc i64 %331 to i32
  %407 = or i32 %406, 15
  %408 = add i32 %407, %21
  %409 = sext i32 %408 to i64
  %410 = getelementptr inbounds float, float* %4, i64 %409
  store float 0.000000e+00, float* %410, align 4, !tbaa !69
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 226
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %411 = add nsw i32 %20, 1
  %412 = icmp slt i32 %411, %15
  br i1 %412, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.4(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 447
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 448
  %24 = select i1 %23, i32 %22, i32 448
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 448
  %27 = select i1 %26, i32 %25, i32 448
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end20
  %29 = phi i32 [ %292, %for_end20 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 28672, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <32 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 128
  %36 = bitcast i8* %35 to <32 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 256
  %38 = bitcast i8* %37 to <32 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 384
  %40 = bitcast i8* %39 to <32 x float>*
  %41 = getelementptr inbounds i8, i8* %33, i64 512
  %42 = bitcast i8* %41 to <32 x float>*
  %43 = getelementptr inbounds i8, i8* %33, i64 640
  %44 = bitcast i8* %43 to <32 x float>*
  %45 = getelementptr inbounds i8, i8* %33, i64 768
  %46 = bitcast i8* %45 to <32 x float>*
  %47 = getelementptr inbounds i8, i8* %33, i64 896
  %48 = bitcast i8* %47 to <32 x float>*
  %49 = srem i32 %29, 224
  %50 = sdiv i32 %29, 224
  %51 = mul nsw i32 %50, 18432
  %52 = bitcast i8* %31 to float*
  %53 = sext i32 %51 to i64
  %reass.mul = mul nsw i32 %49, 3616
  %54 = sext i32 %reass.mul to i64
  %55 = mul nsw i32 %49, 3616
  %reass.mul.1 = add nsw i32 %55, 3616
  %56 = sext i32 %reass.mul.1 to i64
  %57 = mul nsw i32 %49, 3616
  %reass.mul.2 = add nsw i32 %57, 7232
  %58 = sext i32 %reass.mul.2 to i64
  br label %for_body4

for_end:                                          ; preds = %for_end20, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end8, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end8 ]
  %59 = shl i64 %indvar, 7
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  br label %for_body7

for_end5:                                         ; preds = %for_end8
  %60 = mul nsw i32 %29, 7168
  %61 = shl nsw i32 %50, 5
  %62 = sext i32 %61 to i64
  %63 = getelementptr inbounds float, float* %13, i64 %62
  %64 = bitcast float* %63 to <32 x float>*
  %65 = load <32 x float>, <32 x float>* %64, align 64, !tbaa !72
  br label %for_body19

for_body7:                                        ; preds = %for_end14.2, %for_body4
  %indvars.iv83 = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next84, %for_end14.2 ]
  %.lcssa41.lcssa71 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %485, %for_end14.2 ]
  %.lcssa39.lcssa69 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %479, %for_end14.2 ]
  %.lcssa37.lcssa67 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %478, %for_end14.2 ]
  %.lcssa35.lcssa65 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %477, %for_end14.2 ]
  %.lcssa33.lcssa63 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %476, %for_end14.2 ]
  %.lcssa31.lcssa61 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %475, %for_end14.2 ]
  %.lcssa29.lcssa60 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %474, %for_end14.2 ]
  %.lcssa.lcssa58 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %473, %for_end14.2 ]
  %66 = mul nuw nsw i64 %indvars.iv83, 817216
  %67 = add nuw nsw i64 %66, %59
  %68 = mul nuw nsw i64 %indvars.iv83, 4608
  %69 = add nsw i64 %68, %53
  %70 = add nsw i64 %67, %54
  br label %for_body13

for_end8:                                         ; preds = %for_end14.2
  store <32 x float> %473, <32 x float>* %34, align 64, !tbaa !75
  store <32 x float> %474, <32 x float>* %36, align 64, !tbaa !75
  store <32 x float> %475, <32 x float>* %38, align 64, !tbaa !75
  store <32 x float> %476, <32 x float>* %40, align 64, !tbaa !75
  store <32 x float> %477, <32 x float>* %42, align 64, !tbaa !75
  store <32 x float> %478, <32 x float>* %44, align 64, !tbaa !75
  store <32 x float> %479, <32 x float>* %46, align 64, !tbaa !75
  store <32 x float> %485, <32 x float>* %48, align 64, !tbaa !75
  %71 = shl i64 %indvar, 8
  %72 = getelementptr inbounds float, float* %52, i64 %71
  %73 = bitcast float* %72 to <32 x float>*
  store <32 x float> %473, <32 x float>* %73, align 64, !tbaa !84
  %74 = or i64 %71, 32
  %75 = getelementptr inbounds float, float* %52, i64 %74
  %76 = bitcast float* %75 to <32 x float>*
  store <32 x float> %474, <32 x float>* %76, align 64, !tbaa !84
  %77 = or i64 %71, 64
  %78 = getelementptr inbounds float, float* %52, i64 %77
  %79 = bitcast float* %78 to <32 x float>*
  store <32 x float> %475, <32 x float>* %79, align 64, !tbaa !84
  %80 = or i64 %71, 96
  %81 = getelementptr inbounds float, float* %52, i64 %80
  %82 = bitcast float* %81 to <32 x float>*
  store <32 x float> %476, <32 x float>* %82, align 64, !tbaa !84
  %83 = or i64 %71, 128
  %84 = getelementptr inbounds float, float* %52, i64 %83
  %85 = bitcast float* %84 to <32 x float>*
  store <32 x float> %477, <32 x float>* %85, align 64, !tbaa !84
  %86 = or i64 %71, 160
  %87 = getelementptr inbounds float, float* %52, i64 %86
  %88 = bitcast float* %87 to <32 x float>*
  store <32 x float> %478, <32 x float>* %88, align 64, !tbaa !84
  %89 = or i64 %71, 192
  %90 = getelementptr inbounds float, float* %52, i64 %89
  %91 = bitcast float* %90 to <32 x float>*
  store <32 x float> %479, <32 x float>* %91, align 64, !tbaa !84
  %92 = or i64 %71, 224
  %93 = getelementptr inbounds float, float* %52, i64 %92
  %94 = bitcast float* %93 to <32 x float>*
  store <32 x float> %485, <32 x float>* %94, align 64, !tbaa !84
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond89 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond89, label %for_end5, label %for_body4, !prof !20

for_body13:                                       ; preds = %for_body13, %for_body7
  %indvars.iv = phi i64 [ 0, %for_body7 ], [ %indvars.iv.next, %for_body13 ]
  %95 = phi <32 x float> [ %.lcssa41.lcssa71, %for_body7 ], [ %189, %for_body13 ]
  %96 = phi <32 x float> [ %.lcssa39.lcssa69, %for_body7 ], [ %183, %for_body13 ]
  %97 = phi <32 x float> [ %.lcssa37.lcssa67, %for_body7 ], [ %182, %for_body13 ]
  %98 = phi <32 x float> [ %.lcssa35.lcssa65, %for_body7 ], [ %181, %for_body13 ]
  %99 = phi <32 x float> [ %.lcssa33.lcssa63, %for_body7 ], [ %180, %for_body13 ]
  %100 = phi <32 x float> [ %.lcssa31.lcssa61, %for_body7 ], [ %179, %for_body13 ]
  %101 = phi <32 x float> [ %.lcssa29.lcssa60, %for_body7 ], [ %178, %for_body13 ]
  %102 = phi <32 x float> [ %.lcssa.lcssa58, %for_body7 ], [ %177, %for_body13 ]
  %103 = add nsw i64 %70, %indvars.iv
  %104 = getelementptr inbounds float, float* %4, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !69
  %106 = insertelement <32 x float> undef, float %105, i32 0
  %107 = shufflevector <32 x float> %106, <32 x float> undef, <32 x i32> zeroinitializer
  %108 = shl nsw i64 %indvars.iv, 5
  %109 = add nsw i64 %69, %108
  %110 = getelementptr inbounds float, float* %7, i64 %109
  %111 = bitcast float* %110 to <32 x float>*
  %112 = load <32 x float>, <32 x float>* %111, align 64, !tbaa !87
  %113 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %107, <32 x float> %112, <32 x float> %102)
  %114 = add nsw i64 %103, 16
  %115 = getelementptr inbounds float, float* %4, i64 %114
  %116 = load float, float* %115, align 4, !tbaa !69
  %117 = insertelement <32 x float> undef, float %116, i32 0
  %118 = shufflevector <32 x float> %117, <32 x float> undef, <32 x i32> zeroinitializer
  %119 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %118, <32 x float> %112, <32 x float> %101)
  %120 = add nsw i64 %103, 32
  %121 = getelementptr inbounds float, float* %4, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !69
  %123 = insertelement <32 x float> undef, float %122, i32 0
  %124 = shufflevector <32 x float> %123, <32 x float> undef, <32 x i32> zeroinitializer
  %125 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %124, <32 x float> %112, <32 x float> %100)
  %126 = add nsw i64 %103, 48
  %127 = getelementptr inbounds float, float* %4, i64 %126
  %128 = load float, float* %127, align 4, !tbaa !69
  %129 = insertelement <32 x float> undef, float %128, i32 0
  %130 = shufflevector <32 x float> %129, <32 x float> undef, <32 x i32> zeroinitializer
  %131 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %130, <32 x float> %112, <32 x float> %99)
  %132 = add nsw i64 %103, 64
  %133 = getelementptr inbounds float, float* %4, i64 %132
  %134 = load float, float* %133, align 4, !tbaa !69
  %135 = insertelement <32 x float> undef, float %134, i32 0
  %136 = shufflevector <32 x float> %135, <32 x float> undef, <32 x i32> zeroinitializer
  %137 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %136, <32 x float> %112, <32 x float> %98)
  %138 = add nsw i64 %103, 80
  %139 = getelementptr inbounds float, float* %4, i64 %138
  %140 = load float, float* %139, align 4, !tbaa !69
  %141 = insertelement <32 x float> undef, float %140, i32 0
  %142 = shufflevector <32 x float> %141, <32 x float> undef, <32 x i32> zeroinitializer
  %143 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %142, <32 x float> %112, <32 x float> %97)
  %144 = add nsw i64 %103, 96
  %145 = getelementptr inbounds float, float* %4, i64 %144
  %146 = load float, float* %145, align 4, !tbaa !69
  %147 = insertelement <32 x float> undef, float %146, i32 0
  %148 = shufflevector <32 x float> %147, <32 x float> undef, <32 x i32> zeroinitializer
  %149 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %148, <32 x float> %112, <32 x float> %96)
  %150 = add nsw i64 %103, 112
  %151 = getelementptr inbounds float, float* %4, i64 %150
  %152 = load float, float* %151, align 4, !tbaa !69
  %153 = insertelement <32 x float> undef, float %152, i32 0
  %154 = shufflevector <32 x float> %153, <32 x float> undef, <32 x i32> zeroinitializer
  %155 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %154, <32 x float> %112, <32 x float> %95)
  %156 = add nsw i64 %109, 512
  %157 = getelementptr inbounds float, float* %7, i64 %156
  %158 = bitcast float* %157 to <32 x float>*
  %159 = load <32 x float>, <32 x float>* %158, align 64, !tbaa !87
  %160 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %118, <32 x float> %159, <32 x float> %113)
  %161 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %124, <32 x float> %159, <32 x float> %119)
  %162 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %130, <32 x float> %159, <32 x float> %125)
  %163 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %136, <32 x float> %159, <32 x float> %131)
  %164 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %142, <32 x float> %159, <32 x float> %137)
  %165 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %148, <32 x float> %159, <32 x float> %143)
  %166 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %154, <32 x float> %159, <32 x float> %149)
  %167 = add nsw i64 %103, 128
  %168 = getelementptr inbounds float, float* %4, i64 %167
  %169 = load float, float* %168, align 4, !tbaa !69
  %170 = insertelement <32 x float> undef, float %169, i32 0
  %171 = shufflevector <32 x float> %170, <32 x float> undef, <32 x i32> zeroinitializer
  %172 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %171, <32 x float> %159, <32 x float> %155)
  %173 = add nsw i64 %109, 1024
  %174 = getelementptr inbounds float, float* %7, i64 %173
  %175 = bitcast float* %174 to <32 x float>*
  %176 = load <32 x float>, <32 x float>* %175, align 64, !tbaa !87
  %177 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %124, <32 x float> %176, <32 x float> %160)
  %178 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %130, <32 x float> %176, <32 x float> %161)
  %179 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %136, <32 x float> %176, <32 x float> %162)
  %180 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %142, <32 x float> %176, <32 x float> %163)
  %181 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %148, <32 x float> %176, <32 x float> %164)
  %182 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %154, <32 x float> %176, <32 x float> %165)
  %183 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %171, <32 x float> %176, <32 x float> %166)
  %184 = add nsw i64 %103, 144
  %185 = getelementptr inbounds float, float* %4, i64 %184
  %186 = load float, float* %185, align 4, !tbaa !69
  %187 = insertelement <32 x float> undef, float %186, i32 0
  %188 = shufflevector <32 x float> %187, <32 x float> undef, <32 x i32> zeroinitializer
  %189 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %188, <32 x float> %176, <32 x float> %172)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 16
  br i1 %exitcond, label %for_end14, label %for_body13, !prof !20

for_end14:                                        ; preds = %for_body13
  %190 = add nsw i64 %67, %56
  %191 = add nsw i64 %69, 1536
  br label %for_body13.1

for_body19:                                       ; preds = %for_body19, %for_end5
  %indvars.iv93 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next94, %for_body19 ]
  %192 = shl nsw i64 %indvars.iv93, 8
  %193 = trunc i64 %192 to i32
  %194 = add i32 %60, %193
  %195 = getelementptr inbounds float, float* %52, i64 %192
  %196 = bitcast float* %195 to <32 x float>*
  %197 = load <32 x float>, <32 x float>* %196, align 64, !tbaa !84
  %198 = fadd <32 x float> %65, %197
  %199 = fcmp ogt <32 x float> %198, zeroinitializer
  %200 = select <32 x i1> %199, <32 x float> %198, <32 x float> zeroinitializer
  %201 = sext i32 %194 to i64
  %202 = getelementptr inbounds float, float* %10, i64 %201
  %203 = bitcast float* %202 to <32 x float>*
  store <32 x float> %200, <32 x float>* %203, align 64, !tbaa !90
  %204 = or i64 %192, 32
  %205 = trunc i64 %204 to i32
  %206 = add i32 %60, %205
  %207 = getelementptr inbounds float, float* %52, i64 %204
  %208 = bitcast float* %207 to <32 x float>*
  %209 = load <32 x float>, <32 x float>* %208, align 64, !tbaa !84
  %210 = fadd <32 x float> %65, %209
  %211 = fcmp ogt <32 x float> %210, zeroinitializer
  %212 = select <32 x i1> %211, <32 x float> %210, <32 x float> zeroinitializer
  %213 = sext i32 %206 to i64
  %214 = getelementptr inbounds float, float* %10, i64 %213
  %215 = bitcast float* %214 to <32 x float>*
  store <32 x float> %212, <32 x float>* %215, align 64, !tbaa !90
  %216 = or i64 %192, 64
  %217 = trunc i64 %216 to i32
  %218 = add i32 %60, %217
  %219 = getelementptr inbounds float, float* %52, i64 %216
  %220 = bitcast float* %219 to <32 x float>*
  %221 = load <32 x float>, <32 x float>* %220, align 64, !tbaa !84
  %222 = fadd <32 x float> %65, %221
  %223 = fcmp ogt <32 x float> %222, zeroinitializer
  %224 = select <32 x i1> %223, <32 x float> %222, <32 x float> zeroinitializer
  %225 = sext i32 %218 to i64
  %226 = getelementptr inbounds float, float* %10, i64 %225
  %227 = bitcast float* %226 to <32 x float>*
  store <32 x float> %224, <32 x float>* %227, align 64, !tbaa !90
  %228 = or i64 %192, 96
  %229 = trunc i64 %228 to i32
  %230 = add i32 %60, %229
  %231 = getelementptr inbounds float, float* %52, i64 %228
  %232 = bitcast float* %231 to <32 x float>*
  %233 = load <32 x float>, <32 x float>* %232, align 64, !tbaa !84
  %234 = fadd <32 x float> %65, %233
  %235 = fcmp ogt <32 x float> %234, zeroinitializer
  %236 = select <32 x i1> %235, <32 x float> %234, <32 x float> zeroinitializer
  %237 = sext i32 %230 to i64
  %238 = getelementptr inbounds float, float* %10, i64 %237
  %239 = bitcast float* %238 to <32 x float>*
  store <32 x float> %236, <32 x float>* %239, align 64, !tbaa !90
  %240 = or i64 %192, 128
  %241 = trunc i64 %240 to i32
  %242 = add i32 %60, %241
  %243 = getelementptr inbounds float, float* %52, i64 %240
  %244 = bitcast float* %243 to <32 x float>*
  %245 = load <32 x float>, <32 x float>* %244, align 64, !tbaa !84
  %246 = fadd <32 x float> %65, %245
  %247 = fcmp ogt <32 x float> %246, zeroinitializer
  %248 = select <32 x i1> %247, <32 x float> %246, <32 x float> zeroinitializer
  %249 = sext i32 %242 to i64
  %250 = getelementptr inbounds float, float* %10, i64 %249
  %251 = bitcast float* %250 to <32 x float>*
  store <32 x float> %248, <32 x float>* %251, align 64, !tbaa !90
  %252 = or i64 %192, 160
  %253 = trunc i64 %252 to i32
  %254 = add i32 %60, %253
  %255 = getelementptr inbounds float, float* %52, i64 %252
  %256 = bitcast float* %255 to <32 x float>*
  %257 = load <32 x float>, <32 x float>* %256, align 64, !tbaa !84
  %258 = fadd <32 x float> %65, %257
  %259 = fcmp ogt <32 x float> %258, zeroinitializer
  %260 = select <32 x i1> %259, <32 x float> %258, <32 x float> zeroinitializer
  %261 = sext i32 %254 to i64
  %262 = getelementptr inbounds float, float* %10, i64 %261
  %263 = bitcast float* %262 to <32 x float>*
  store <32 x float> %260, <32 x float>* %263, align 64, !tbaa !90
  %264 = or i64 %192, 192
  %265 = trunc i64 %264 to i32
  %266 = add i32 %60, %265
  %267 = getelementptr inbounds float, float* %52, i64 %264
  %268 = bitcast float* %267 to <32 x float>*
  %269 = load <32 x float>, <32 x float>* %268, align 64, !tbaa !84
  %270 = fadd <32 x float> %65, %269
  %271 = fcmp ogt <32 x float> %270, zeroinitializer
  %272 = select <32 x i1> %271, <32 x float> %270, <32 x float> zeroinitializer
  %273 = sext i32 %266 to i64
  %274 = getelementptr inbounds float, float* %10, i64 %273
  %275 = bitcast float* %274 to <32 x float>*
  store <32 x float> %272, <32 x float>* %275, align 64, !tbaa !90
  %276 = or i64 %192, 224
  %277 = trunc i64 %276 to i32
  %278 = add i32 %60, %277
  %279 = getelementptr inbounds float, float* %52, i64 %276
  %280 = bitcast float* %279 to <32 x float>*
  %281 = load <32 x float>, <32 x float>* %280, align 64, !tbaa !84
  %282 = fadd <32 x float> %65, %281
  %283 = fcmp ogt <32 x float> %282, zeroinitializer
  %284 = select <32 x i1> %283, <32 x float> %282, <32 x float> zeroinitializer
  %285 = sext i32 %278 to i64
  %286 = getelementptr inbounds float, float* %10, i64 %285
  %287 = bitcast float* %286 to <32 x float>*
  store <32 x float> %284, <32 x float>* %287, align 64, !tbaa !90
  %indvars.iv.next94 = add nuw nsw i64 %indvars.iv93, 1
  %exitcond95 = icmp eq i64 %indvars.iv.next94, 28
  br i1 %exitcond95, label %for_end20, label %for_body19, !prof !20

for_end20:                                        ; preds = %for_body19
  %288 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %289 = tail call i32 %288(i32 1, i32 %16, i8* %33)
  %290 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %291 = tail call i32 %290(i32 1, i32 %16, i8* nonnull %31)
  %292 = add nsw i32 %29, 1
  %293 = icmp slt i32 %292, %24
  br i1 %293, label %for_body, label %for_end, !prof !19

for_body13.1:                                     ; preds = %for_body13.1, %for_end14
  %indvars.iv.1 = phi i64 [ 0, %for_end14 ], [ %indvars.iv.next.1, %for_body13.1 ]
  %294 = phi <32 x float> [ %189, %for_end14 ], [ %388, %for_body13.1 ]
  %295 = phi <32 x float> [ %183, %for_end14 ], [ %382, %for_body13.1 ]
  %296 = phi <32 x float> [ %182, %for_end14 ], [ %381, %for_body13.1 ]
  %297 = phi <32 x float> [ %181, %for_end14 ], [ %380, %for_body13.1 ]
  %298 = phi <32 x float> [ %180, %for_end14 ], [ %379, %for_body13.1 ]
  %299 = phi <32 x float> [ %179, %for_end14 ], [ %378, %for_body13.1 ]
  %300 = phi <32 x float> [ %178, %for_end14 ], [ %377, %for_body13.1 ]
  %301 = phi <32 x float> [ %177, %for_end14 ], [ %376, %for_body13.1 ]
  %302 = add nsw i64 %190, %indvars.iv.1
  %303 = getelementptr inbounds float, float* %4, i64 %302
  %304 = load float, float* %303, align 4, !tbaa !69
  %305 = insertelement <32 x float> undef, float %304, i32 0
  %306 = shufflevector <32 x float> %305, <32 x float> undef, <32 x i32> zeroinitializer
  %307 = shl nsw i64 %indvars.iv.1, 5
  %308 = add nsw i64 %191, %307
  %309 = getelementptr inbounds float, float* %7, i64 %308
  %310 = bitcast float* %309 to <32 x float>*
  %311 = load <32 x float>, <32 x float>* %310, align 64, !tbaa !87
  %312 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %306, <32 x float> %311, <32 x float> %301)
  %313 = add nsw i64 %302, 16
  %314 = getelementptr inbounds float, float* %4, i64 %313
  %315 = load float, float* %314, align 4, !tbaa !69
  %316 = insertelement <32 x float> undef, float %315, i32 0
  %317 = shufflevector <32 x float> %316, <32 x float> undef, <32 x i32> zeroinitializer
  %318 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %311, <32 x float> %300)
  %319 = add nsw i64 %302, 32
  %320 = getelementptr inbounds float, float* %4, i64 %319
  %321 = load float, float* %320, align 4, !tbaa !69
  %322 = insertelement <32 x float> undef, float %321, i32 0
  %323 = shufflevector <32 x float> %322, <32 x float> undef, <32 x i32> zeroinitializer
  %324 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %311, <32 x float> %299)
  %325 = add nsw i64 %302, 48
  %326 = getelementptr inbounds float, float* %4, i64 %325
  %327 = load float, float* %326, align 4, !tbaa !69
  %328 = insertelement <32 x float> undef, float %327, i32 0
  %329 = shufflevector <32 x float> %328, <32 x float> undef, <32 x i32> zeroinitializer
  %330 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %311, <32 x float> %298)
  %331 = add nsw i64 %302, 64
  %332 = getelementptr inbounds float, float* %4, i64 %331
  %333 = load float, float* %332, align 4, !tbaa !69
  %334 = insertelement <32 x float> undef, float %333, i32 0
  %335 = shufflevector <32 x float> %334, <32 x float> undef, <32 x i32> zeroinitializer
  %336 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %335, <32 x float> %311, <32 x float> %297)
  %337 = add nsw i64 %302, 80
  %338 = getelementptr inbounds float, float* %4, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !69
  %340 = insertelement <32 x float> undef, float %339, i32 0
  %341 = shufflevector <32 x float> %340, <32 x float> undef, <32 x i32> zeroinitializer
  %342 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %341, <32 x float> %311, <32 x float> %296)
  %343 = add nsw i64 %302, 96
  %344 = getelementptr inbounds float, float* %4, i64 %343
  %345 = load float, float* %344, align 4, !tbaa !69
  %346 = insertelement <32 x float> undef, float %345, i32 0
  %347 = shufflevector <32 x float> %346, <32 x float> undef, <32 x i32> zeroinitializer
  %348 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %347, <32 x float> %311, <32 x float> %295)
  %349 = add nsw i64 %302, 112
  %350 = getelementptr inbounds float, float* %4, i64 %349
  %351 = load float, float* %350, align 4, !tbaa !69
  %352 = insertelement <32 x float> undef, float %351, i32 0
  %353 = shufflevector <32 x float> %352, <32 x float> undef, <32 x i32> zeroinitializer
  %354 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %353, <32 x float> %311, <32 x float> %294)
  %355 = add nsw i64 %308, 512
  %356 = getelementptr inbounds float, float* %7, i64 %355
  %357 = bitcast float* %356 to <32 x float>*
  %358 = load <32 x float>, <32 x float>* %357, align 64, !tbaa !87
  %359 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %358, <32 x float> %312)
  %360 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %358, <32 x float> %318)
  %361 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %358, <32 x float> %324)
  %362 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %335, <32 x float> %358, <32 x float> %330)
  %363 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %341, <32 x float> %358, <32 x float> %336)
  %364 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %347, <32 x float> %358, <32 x float> %342)
  %365 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %353, <32 x float> %358, <32 x float> %348)
  %366 = add nsw i64 %302, 128
  %367 = getelementptr inbounds float, float* %4, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !69
  %369 = insertelement <32 x float> undef, float %368, i32 0
  %370 = shufflevector <32 x float> %369, <32 x float> undef, <32 x i32> zeroinitializer
  %371 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %370, <32 x float> %358, <32 x float> %354)
  %372 = add nsw i64 %308, 1024
  %373 = getelementptr inbounds float, float* %7, i64 %372
  %374 = bitcast float* %373 to <32 x float>*
  %375 = load <32 x float>, <32 x float>* %374, align 64, !tbaa !87
  %376 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %375, <32 x float> %359)
  %377 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %375, <32 x float> %360)
  %378 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %335, <32 x float> %375, <32 x float> %361)
  %379 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %341, <32 x float> %375, <32 x float> %362)
  %380 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %347, <32 x float> %375, <32 x float> %363)
  %381 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %353, <32 x float> %375, <32 x float> %364)
  %382 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %370, <32 x float> %375, <32 x float> %365)
  %383 = add nsw i64 %302, 144
  %384 = getelementptr inbounds float, float* %4, i64 %383
  %385 = load float, float* %384, align 4, !tbaa !69
  %386 = insertelement <32 x float> undef, float %385, i32 0
  %387 = shufflevector <32 x float> %386, <32 x float> undef, <32 x i32> zeroinitializer
  %388 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %387, <32 x float> %375, <32 x float> %371)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 16
  br i1 %exitcond.1, label %for_end14.1, label %for_body13.1, !prof !20

for_end14.1:                                      ; preds = %for_body13.1
  %389 = add nsw i64 %67, %58
  %390 = add nsw i64 %69, 3072
  br label %for_body13.2

for_body13.2:                                     ; preds = %for_body13.2, %for_end14.1
  %indvars.iv.2 = phi i64 [ 0, %for_end14.1 ], [ %indvars.iv.next.2, %for_body13.2 ]
  %391 = phi <32 x float> [ %388, %for_end14.1 ], [ %485, %for_body13.2 ]
  %392 = phi <32 x float> [ %382, %for_end14.1 ], [ %479, %for_body13.2 ]
  %393 = phi <32 x float> [ %381, %for_end14.1 ], [ %478, %for_body13.2 ]
  %394 = phi <32 x float> [ %380, %for_end14.1 ], [ %477, %for_body13.2 ]
  %395 = phi <32 x float> [ %379, %for_end14.1 ], [ %476, %for_body13.2 ]
  %396 = phi <32 x float> [ %378, %for_end14.1 ], [ %475, %for_body13.2 ]
  %397 = phi <32 x float> [ %377, %for_end14.1 ], [ %474, %for_body13.2 ]
  %398 = phi <32 x float> [ %376, %for_end14.1 ], [ %473, %for_body13.2 ]
  %399 = add nsw i64 %389, %indvars.iv.2
  %400 = getelementptr inbounds float, float* %4, i64 %399
  %401 = load float, float* %400, align 4, !tbaa !69
  %402 = insertelement <32 x float> undef, float %401, i32 0
  %403 = shufflevector <32 x float> %402, <32 x float> undef, <32 x i32> zeroinitializer
  %404 = shl nsw i64 %indvars.iv.2, 5
  %405 = add nsw i64 %390, %404
  %406 = getelementptr inbounds float, float* %7, i64 %405
  %407 = bitcast float* %406 to <32 x float>*
  %408 = load <32 x float>, <32 x float>* %407, align 64, !tbaa !87
  %409 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %403, <32 x float> %408, <32 x float> %398)
  %410 = add nsw i64 %399, 16
  %411 = getelementptr inbounds float, float* %4, i64 %410
  %412 = load float, float* %411, align 4, !tbaa !69
  %413 = insertelement <32 x float> undef, float %412, i32 0
  %414 = shufflevector <32 x float> %413, <32 x float> undef, <32 x i32> zeroinitializer
  %415 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %414, <32 x float> %408, <32 x float> %397)
  %416 = add nsw i64 %399, 32
  %417 = getelementptr inbounds float, float* %4, i64 %416
  %418 = load float, float* %417, align 4, !tbaa !69
  %419 = insertelement <32 x float> undef, float %418, i32 0
  %420 = shufflevector <32 x float> %419, <32 x float> undef, <32 x i32> zeroinitializer
  %421 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %420, <32 x float> %408, <32 x float> %396)
  %422 = add nsw i64 %399, 48
  %423 = getelementptr inbounds float, float* %4, i64 %422
  %424 = load float, float* %423, align 4, !tbaa !69
  %425 = insertelement <32 x float> undef, float %424, i32 0
  %426 = shufflevector <32 x float> %425, <32 x float> undef, <32 x i32> zeroinitializer
  %427 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %426, <32 x float> %408, <32 x float> %395)
  %428 = add nsw i64 %399, 64
  %429 = getelementptr inbounds float, float* %4, i64 %428
  %430 = load float, float* %429, align 4, !tbaa !69
  %431 = insertelement <32 x float> undef, float %430, i32 0
  %432 = shufflevector <32 x float> %431, <32 x float> undef, <32 x i32> zeroinitializer
  %433 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %432, <32 x float> %408, <32 x float> %394)
  %434 = add nsw i64 %399, 80
  %435 = getelementptr inbounds float, float* %4, i64 %434
  %436 = load float, float* %435, align 4, !tbaa !69
  %437 = insertelement <32 x float> undef, float %436, i32 0
  %438 = shufflevector <32 x float> %437, <32 x float> undef, <32 x i32> zeroinitializer
  %439 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %438, <32 x float> %408, <32 x float> %393)
  %440 = add nsw i64 %399, 96
  %441 = getelementptr inbounds float, float* %4, i64 %440
  %442 = load float, float* %441, align 4, !tbaa !69
  %443 = insertelement <32 x float> undef, float %442, i32 0
  %444 = shufflevector <32 x float> %443, <32 x float> undef, <32 x i32> zeroinitializer
  %445 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %444, <32 x float> %408, <32 x float> %392)
  %446 = add nsw i64 %399, 112
  %447 = getelementptr inbounds float, float* %4, i64 %446
  %448 = load float, float* %447, align 4, !tbaa !69
  %449 = insertelement <32 x float> undef, float %448, i32 0
  %450 = shufflevector <32 x float> %449, <32 x float> undef, <32 x i32> zeroinitializer
  %451 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %450, <32 x float> %408, <32 x float> %391)
  %452 = add nsw i64 %405, 512
  %453 = getelementptr inbounds float, float* %7, i64 %452
  %454 = bitcast float* %453 to <32 x float>*
  %455 = load <32 x float>, <32 x float>* %454, align 64, !tbaa !87
  %456 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %414, <32 x float> %455, <32 x float> %409)
  %457 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %420, <32 x float> %455, <32 x float> %415)
  %458 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %426, <32 x float> %455, <32 x float> %421)
  %459 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %432, <32 x float> %455, <32 x float> %427)
  %460 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %438, <32 x float> %455, <32 x float> %433)
  %461 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %444, <32 x float> %455, <32 x float> %439)
  %462 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %450, <32 x float> %455, <32 x float> %445)
  %463 = add nsw i64 %399, 128
  %464 = getelementptr inbounds float, float* %4, i64 %463
  %465 = load float, float* %464, align 4, !tbaa !69
  %466 = insertelement <32 x float> undef, float %465, i32 0
  %467 = shufflevector <32 x float> %466, <32 x float> undef, <32 x i32> zeroinitializer
  %468 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %467, <32 x float> %455, <32 x float> %451)
  %469 = add nsw i64 %405, 1024
  %470 = getelementptr inbounds float, float* %7, i64 %469
  %471 = bitcast float* %470 to <32 x float>*
  %472 = load <32 x float>, <32 x float>* %471, align 64, !tbaa !87
  %473 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %420, <32 x float> %472, <32 x float> %456)
  %474 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %426, <32 x float> %472, <32 x float> %457)
  %475 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %432, <32 x float> %472, <32 x float> %458)
  %476 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %438, <32 x float> %472, <32 x float> %459)
  %477 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %444, <32 x float> %472, <32 x float> %460)
  %478 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %450, <32 x float> %472, <32 x float> %461)
  %479 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %467, <32 x float> %472, <32 x float> %462)
  %480 = add nsw i64 %399, 144
  %481 = getelementptr inbounds float, float* %4, i64 %480
  %482 = load float, float* %481, align 4, !tbaa !69
  %483 = insertelement <32 x float> undef, float %482, i32 0
  %484 = shufflevector <32 x float> %483, <32 x float> undef, <32 x i32> zeroinitializer
  %485 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %484, <32 x float> %472, <32 x float> %468)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 16
  br i1 %exitcond.2, label %for_end14.2, label %for_body13.2, !prof !20

for_end14.2:                                      ; preds = %for_body13.2
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 4
  br i1 %exitcond85, label %for_end8, label %for_body7, !prof !20
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_5(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !93 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !95, metadata !DIExpression()), !dbg !98
  call void @llvm.dbg.value(metadata i8* %1, metadata !96, metadata !DIExpression()), !dbg !98
  call void @llvm.dbg.value(metadata i32 %2, metadata !97, metadata !DIExpression()), !dbg !98
  %3 = bitcast i8* %0 to %1**, !dbg !98
  %4 = load %1*, %1** %3, align 8, !dbg !98
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !98
  %6 = bitcast i8* %5 to %1**, !dbg !98
  %7 = load %1*, %1** %6, align 8, !dbg !98
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !98
  %9 = bitcast i8* %8 to %1**, !dbg !98
  %10 = load %1*, %1** %9, align 8, !dbg !98
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !98
  %12 = bitcast i8* %11 to %1**, !dbg !98
  %13 = load %1*, %1** %12, align 8, !dbg !98
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !98
  %15 = load i8*, i8** %14, align 8, !dbg !98
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !98
  %17 = load i32, i32* %16, align 4, !dbg !98
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !98
  %19 = load i8*, i8** %18, align 8, !dbg !98
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !98
  %21 = load i8*, i8** %20, align 8, !dbg !98
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !98
  %23 = load i8*, i8** %22, align 8, !dbg !98
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_5_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !98
  ret i32 %24, !dbg !98
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_5_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 6653952, i32 2, i32 32)
  %7 = alloca %9, align 8
  %8 = getelementptr inbounds %9, %9* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %9, %9* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %9* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.5, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
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
  %19 = getelementptr inbounds %10, %10* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %10* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.6, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
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
  %10 = add nsw i32 %9, 227
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 228
  %15 = select i1 %14, i32 %13, i32 228
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 228
  %18 = select i1 %17, i32 %16, i32 228
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %427, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 7296
  %22 = srem i32 %20, 114
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 112
  %24 = sdiv i32 %20, 114
  %25 = mul nsw i32 %24, 802816
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 7168
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv24 = phi i64 [ %indvars.iv.next25, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv24, 6
  %28 = trunc i64 %indvars.iv24 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 112
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %vector.body, label %vector.body35

for_end6.us:                                      ; preds = %vector.body35, %vector.body
  %indvars.iv.next25 = add nuw nsw i64 %indvars.iv24, 1
  %exitcond27 = icmp eq i64 %indvars.iv.next25, 114
  br i1 %exitcond27, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -7232
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %39, align 4, !tbaa !99
  %40 = sext i32 %32 to i64
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %42, align 4, !tbaa !102
  %43 = or i64 %27, 4
  %44 = trunc i64 %43 to i32
  %45 = add i32 %21, %44
  %46 = trunc i64 %43 to i32
  %47 = add i32 %46, -7232
  %48 = add i32 %26, %47
  %49 = add i32 %48, %25
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to <4 x i32>*
  %wide.load.1 = load <4 x i32>, <4 x i32>* %52, align 4, !tbaa !99
  %53 = sext i32 %45 to i64
  %54 = getelementptr inbounds float, float* %4, i64 %53
  %55 = bitcast float* %54 to <4 x i32>*
  store <4 x i32> %wide.load.1, <4 x i32>* %55, align 4, !tbaa !102
  %56 = or i64 %27, 8
  %57 = trunc i64 %56 to i32
  %58 = add i32 %21, %57
  %59 = trunc i64 %56 to i32
  %60 = add i32 %59, -7232
  %61 = add i32 %26, %60
  %62 = add i32 %61, %25
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %7, i64 %63
  %65 = bitcast float* %64 to <4 x i32>*
  %wide.load.2 = load <4 x i32>, <4 x i32>* %65, align 4, !tbaa !99
  %66 = sext i32 %58 to i64
  %67 = getelementptr inbounds float, float* %4, i64 %66
  %68 = bitcast float* %67 to <4 x i32>*
  store <4 x i32> %wide.load.2, <4 x i32>* %68, align 4, !tbaa !102
  %69 = or i64 %27, 12
  %70 = trunc i64 %69 to i32
  %71 = add i32 %21, %70
  %72 = trunc i64 %69 to i32
  %73 = add i32 %72, -7232
  %74 = add i32 %26, %73
  %75 = add i32 %74, %25
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %7, i64 %76
  %78 = bitcast float* %77 to <4 x i32>*
  %wide.load.3 = load <4 x i32>, <4 x i32>* %78, align 4, !tbaa !99
  %79 = sext i32 %71 to i64
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = bitcast float* %80 to <4 x i32>*
  store <4 x i32> %wide.load.3, <4 x i32>* %81, align 4, !tbaa !102
  %82 = or i64 %27, 16
  %83 = trunc i64 %82 to i32
  %84 = add i32 %21, %83
  %85 = trunc i64 %82 to i32
  %86 = add i32 %85, -7232
  %87 = add i32 %26, %86
  %88 = add i32 %87, %25
  %89 = sext i32 %88 to i64
  %90 = getelementptr inbounds float, float* %7, i64 %89
  %91 = bitcast float* %90 to <4 x i32>*
  %wide.load.4 = load <4 x i32>, <4 x i32>* %91, align 4, !tbaa !99
  %92 = sext i32 %84 to i64
  %93 = getelementptr inbounds float, float* %4, i64 %92
  %94 = bitcast float* %93 to <4 x i32>*
  store <4 x i32> %wide.load.4, <4 x i32>* %94, align 4, !tbaa !102
  %95 = or i64 %27, 20
  %96 = trunc i64 %95 to i32
  %97 = add i32 %21, %96
  %98 = trunc i64 %95 to i32
  %99 = add i32 %98, -7232
  %100 = add i32 %26, %99
  %101 = add i32 %100, %25
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds float, float* %7, i64 %102
  %104 = bitcast float* %103 to <4 x i32>*
  %wide.load.5 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !99
  %105 = sext i32 %97 to i64
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = bitcast float* %106 to <4 x i32>*
  store <4 x i32> %wide.load.5, <4 x i32>* %107, align 4, !tbaa !102
  %108 = or i64 %27, 24
  %109 = trunc i64 %108 to i32
  %110 = add i32 %21, %109
  %111 = trunc i64 %108 to i32
  %112 = add i32 %111, -7232
  %113 = add i32 %26, %112
  %114 = add i32 %113, %25
  %115 = sext i32 %114 to i64
  %116 = getelementptr inbounds float, float* %7, i64 %115
  %117 = bitcast float* %116 to <4 x i32>*
  %wide.load.6 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !99
  %118 = sext i32 %110 to i64
  %119 = getelementptr inbounds float, float* %4, i64 %118
  %120 = bitcast float* %119 to <4 x i32>*
  store <4 x i32> %wide.load.6, <4 x i32>* %120, align 4, !tbaa !102
  %121 = or i64 %27, 28
  %122 = trunc i64 %121 to i32
  %123 = add i32 %21, %122
  %124 = trunc i64 %121 to i32
  %125 = add i32 %124, -7232
  %126 = add i32 %26, %125
  %127 = add i32 %126, %25
  %128 = sext i32 %127 to i64
  %129 = getelementptr inbounds float, float* %7, i64 %128
  %130 = bitcast float* %129 to <4 x i32>*
  %wide.load.7 = load <4 x i32>, <4 x i32>* %130, align 4, !tbaa !99
  %131 = sext i32 %123 to i64
  %132 = getelementptr inbounds float, float* %4, i64 %131
  %133 = bitcast float* %132 to <4 x i32>*
  store <4 x i32> %wide.load.7, <4 x i32>* %133, align 4, !tbaa !102
  %134 = or i64 %27, 32
  %135 = trunc i64 %134 to i32
  %136 = add i32 %21, %135
  %137 = trunc i64 %134 to i32
  %138 = add i32 %137, -7232
  %139 = add i32 %26, %138
  %140 = add i32 %139, %25
  %141 = sext i32 %140 to i64
  %142 = getelementptr inbounds float, float* %7, i64 %141
  %143 = bitcast float* %142 to <4 x i32>*
  %wide.load.8 = load <4 x i32>, <4 x i32>* %143, align 4, !tbaa !99
  %144 = sext i32 %136 to i64
  %145 = getelementptr inbounds float, float* %4, i64 %144
  %146 = bitcast float* %145 to <4 x i32>*
  store <4 x i32> %wide.load.8, <4 x i32>* %146, align 4, !tbaa !102
  %147 = or i64 %27, 36
  %148 = trunc i64 %147 to i32
  %149 = add i32 %21, %148
  %150 = trunc i64 %147 to i32
  %151 = add i32 %150, -7232
  %152 = add i32 %26, %151
  %153 = add i32 %152, %25
  %154 = sext i32 %153 to i64
  %155 = getelementptr inbounds float, float* %7, i64 %154
  %156 = bitcast float* %155 to <4 x i32>*
  %wide.load.9 = load <4 x i32>, <4 x i32>* %156, align 4, !tbaa !99
  %157 = sext i32 %149 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  %159 = bitcast float* %158 to <4 x i32>*
  store <4 x i32> %wide.load.9, <4 x i32>* %159, align 4, !tbaa !102
  %160 = or i64 %27, 40
  %161 = trunc i64 %160 to i32
  %162 = add i32 %21, %161
  %163 = trunc i64 %160 to i32
  %164 = add i32 %163, -7232
  %165 = add i32 %26, %164
  %166 = add i32 %165, %25
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %7, i64 %167
  %169 = bitcast float* %168 to <4 x i32>*
  %wide.load.10 = load <4 x i32>, <4 x i32>* %169, align 4, !tbaa !99
  %170 = sext i32 %162 to i64
  %171 = getelementptr inbounds float, float* %4, i64 %170
  %172 = bitcast float* %171 to <4 x i32>*
  store <4 x i32> %wide.load.10, <4 x i32>* %172, align 4, !tbaa !102
  %173 = or i64 %27, 44
  %174 = trunc i64 %173 to i32
  %175 = add i32 %21, %174
  %176 = trunc i64 %173 to i32
  %177 = add i32 %176, -7232
  %178 = add i32 %26, %177
  %179 = add i32 %178, %25
  %180 = sext i32 %179 to i64
  %181 = getelementptr inbounds float, float* %7, i64 %180
  %182 = bitcast float* %181 to <4 x i32>*
  %wide.load.11 = load <4 x i32>, <4 x i32>* %182, align 4, !tbaa !99
  %183 = sext i32 %175 to i64
  %184 = getelementptr inbounds float, float* %4, i64 %183
  %185 = bitcast float* %184 to <4 x i32>*
  store <4 x i32> %wide.load.11, <4 x i32>* %185, align 4, !tbaa !102
  %186 = or i64 %27, 48
  %187 = trunc i64 %186 to i32
  %188 = add i32 %21, %187
  %189 = trunc i64 %186 to i32
  %190 = add i32 %189, -7232
  %191 = add i32 %26, %190
  %192 = add i32 %191, %25
  %193 = sext i32 %192 to i64
  %194 = getelementptr inbounds float, float* %7, i64 %193
  %195 = bitcast float* %194 to <4 x i32>*
  %wide.load.12 = load <4 x i32>, <4 x i32>* %195, align 4, !tbaa !99
  %196 = sext i32 %188 to i64
  %197 = getelementptr inbounds float, float* %4, i64 %196
  %198 = bitcast float* %197 to <4 x i32>*
  store <4 x i32> %wide.load.12, <4 x i32>* %198, align 4, !tbaa !102
  %199 = or i64 %27, 52
  %200 = trunc i64 %199 to i32
  %201 = add i32 %21, %200
  %202 = trunc i64 %199 to i32
  %203 = add i32 %202, -7232
  %204 = add i32 %26, %203
  %205 = add i32 %204, %25
  %206 = sext i32 %205 to i64
  %207 = getelementptr inbounds float, float* %7, i64 %206
  %208 = bitcast float* %207 to <4 x i32>*
  %wide.load.13 = load <4 x i32>, <4 x i32>* %208, align 4, !tbaa !99
  %209 = sext i32 %201 to i64
  %210 = getelementptr inbounds float, float* %4, i64 %209
  %211 = bitcast float* %210 to <4 x i32>*
  store <4 x i32> %wide.load.13, <4 x i32>* %211, align 4, !tbaa !102
  %212 = or i64 %27, 56
  %213 = trunc i64 %212 to i32
  %214 = add i32 %21, %213
  %215 = trunc i64 %212 to i32
  %216 = add i32 %215, -7232
  %217 = add i32 %26, %216
  %218 = add i32 %217, %25
  %219 = sext i32 %218 to i64
  %220 = getelementptr inbounds float, float* %7, i64 %219
  %221 = bitcast float* %220 to <4 x i32>*
  %wide.load.14 = load <4 x i32>, <4 x i32>* %221, align 4, !tbaa !99
  %222 = sext i32 %214 to i64
  %223 = getelementptr inbounds float, float* %4, i64 %222
  %224 = bitcast float* %223 to <4 x i32>*
  store <4 x i32> %wide.load.14, <4 x i32>* %224, align 4, !tbaa !102
  %225 = or i64 %27, 60
  %226 = trunc i64 %225 to i32
  %227 = add i32 %21, %226
  %228 = trunc i64 %225 to i32
  %229 = add i32 %228, -7232
  %230 = add i32 %26, %229
  %231 = add i32 %230, %25
  %232 = sext i32 %231 to i64
  %233 = getelementptr inbounds float, float* %7, i64 %232
  %234 = bitcast float* %233 to <4 x i32>*
  %wide.load.15 = load <4 x i32>, <4 x i32>* %234, align 4, !tbaa !99
  %235 = sext i32 %227 to i64
  %236 = getelementptr inbounds float, float* %4, i64 %235
  %237 = bitcast float* %236 to <4 x i32>*
  store <4 x i32> %wide.load.15, <4 x i32>* %237, align 4, !tbaa !102
  br label %for_end6.us

vector.body35:                                    ; preds = %for_body2.us
  %238 = sext i32 %32 to i64
  %239 = getelementptr inbounds float, float* %4, i64 %238
  %240 = bitcast float* %239 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %240, align 4, !tbaa !102
  %241 = trunc i64 %27 to i32
  %242 = or i32 %241, 4
  %243 = add i32 %21, %242
  %244 = sext i32 %243 to i64
  %245 = getelementptr inbounds float, float* %4, i64 %244
  %246 = bitcast float* %245 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %246, align 4, !tbaa !102
  %247 = trunc i64 %27 to i32
  %248 = or i32 %247, 8
  %249 = add i32 %21, %248
  %250 = sext i32 %249 to i64
  %251 = getelementptr inbounds float, float* %4, i64 %250
  %252 = bitcast float* %251 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %252, align 4, !tbaa !102
  %253 = trunc i64 %27 to i32
  %254 = or i32 %253, 12
  %255 = add i32 %21, %254
  %256 = sext i32 %255 to i64
  %257 = getelementptr inbounds float, float* %4, i64 %256
  %258 = bitcast float* %257 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %258, align 4, !tbaa !102
  %259 = trunc i64 %27 to i32
  %260 = or i32 %259, 16
  %261 = add i32 %21, %260
  %262 = sext i32 %261 to i64
  %263 = getelementptr inbounds float, float* %4, i64 %262
  %264 = bitcast float* %263 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %264, align 4, !tbaa !102
  %265 = trunc i64 %27 to i32
  %266 = or i32 %265, 20
  %267 = add i32 %21, %266
  %268 = sext i32 %267 to i64
  %269 = getelementptr inbounds float, float* %4, i64 %268
  %270 = bitcast float* %269 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %270, align 4, !tbaa !102
  %271 = trunc i64 %27 to i32
  %272 = or i32 %271, 24
  %273 = add i32 %21, %272
  %274 = sext i32 %273 to i64
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = bitcast float* %275 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %276, align 4, !tbaa !102
  %277 = trunc i64 %27 to i32
  %278 = or i32 %277, 28
  %279 = add i32 %21, %278
  %280 = sext i32 %279 to i64
  %281 = getelementptr inbounds float, float* %4, i64 %280
  %282 = bitcast float* %281 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %282, align 4, !tbaa !102
  %283 = trunc i64 %27 to i32
  %284 = or i32 %283, 32
  %285 = add i32 %21, %284
  %286 = sext i32 %285 to i64
  %287 = getelementptr inbounds float, float* %4, i64 %286
  %288 = bitcast float* %287 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %288, align 4, !tbaa !102
  %289 = trunc i64 %27 to i32
  %290 = or i32 %289, 36
  %291 = add i32 %21, %290
  %292 = sext i32 %291 to i64
  %293 = getelementptr inbounds float, float* %4, i64 %292
  %294 = bitcast float* %293 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %294, align 4, !tbaa !102
  %295 = trunc i64 %27 to i32
  %296 = or i32 %295, 40
  %297 = add i32 %21, %296
  %298 = sext i32 %297 to i64
  %299 = getelementptr inbounds float, float* %4, i64 %298
  %300 = bitcast float* %299 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %300, align 4, !tbaa !102
  %301 = trunc i64 %27 to i32
  %302 = or i32 %301, 44
  %303 = add i32 %21, %302
  %304 = sext i32 %303 to i64
  %305 = getelementptr inbounds float, float* %4, i64 %304
  %306 = bitcast float* %305 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %306, align 4, !tbaa !102
  %307 = trunc i64 %27 to i32
  %308 = or i32 %307, 48
  %309 = add i32 %21, %308
  %310 = sext i32 %309 to i64
  %311 = getelementptr inbounds float, float* %4, i64 %310
  %312 = bitcast float* %311 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %312, align 4, !tbaa !102
  %313 = trunc i64 %27 to i32
  %314 = or i32 %313, 52
  %315 = add i32 %21, %314
  %316 = sext i32 %315 to i64
  %317 = getelementptr inbounds float, float* %4, i64 %316
  %318 = bitcast float* %317 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %318, align 4, !tbaa !102
  %319 = trunc i64 %27 to i32
  %320 = or i32 %319, 56
  %321 = add i32 %21, %320
  %322 = sext i32 %321 to i64
  %323 = getelementptr inbounds float, float* %4, i64 %322
  %324 = bitcast float* %323 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %324, align 4, !tbaa !102
  %325 = trunc i64 %27 to i32
  %326 = or i32 %325, 60
  %327 = add i32 %21, %326
  %328 = sext i32 %327 to i64
  %329 = getelementptr inbounds float, float* %4, i64 %328
  %330 = bitcast float* %329 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %330, align 4, !tbaa !102
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv15 = phi i64 [ %indvars.iv.next16, %for_body2 ], [ 0, %for_body2.preheader ]
  %331 = shl nsw i64 %indvars.iv15, 6
  %332 = trunc i64 %331 to i32
  %333 = add i32 %21, %332
  %334 = sext i32 %333 to i64
  %335 = getelementptr inbounds float, float* %4, i64 %334
  %336 = bitcast float* %335 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %336, align 4, !tbaa !102
  %337 = trunc i64 %331 to i32
  %338 = or i32 %337, 4
  %339 = add i32 %21, %338
  %340 = sext i32 %339 to i64
  %341 = getelementptr inbounds float, float* %4, i64 %340
  %342 = bitcast float* %341 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %342, align 4, !tbaa !102
  %343 = trunc i64 %331 to i32
  %344 = or i32 %343, 8
  %345 = add i32 %21, %344
  %346 = sext i32 %345 to i64
  %347 = getelementptr inbounds float, float* %4, i64 %346
  %348 = bitcast float* %347 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %348, align 4, !tbaa !102
  %349 = trunc i64 %331 to i32
  %350 = or i32 %349, 12
  %351 = add i32 %21, %350
  %352 = sext i32 %351 to i64
  %353 = getelementptr inbounds float, float* %4, i64 %352
  %354 = bitcast float* %353 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %354, align 4, !tbaa !102
  %355 = trunc i64 %331 to i32
  %356 = or i32 %355, 16
  %357 = add i32 %21, %356
  %358 = sext i32 %357 to i64
  %359 = getelementptr inbounds float, float* %4, i64 %358
  %360 = bitcast float* %359 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %360, align 4, !tbaa !102
  %361 = trunc i64 %331 to i32
  %362 = or i32 %361, 20
  %363 = add i32 %21, %362
  %364 = sext i32 %363 to i64
  %365 = getelementptr inbounds float, float* %4, i64 %364
  %366 = bitcast float* %365 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %366, align 4, !tbaa !102
  %367 = trunc i64 %331 to i32
  %368 = or i32 %367, 24
  %369 = add i32 %21, %368
  %370 = sext i32 %369 to i64
  %371 = getelementptr inbounds float, float* %4, i64 %370
  %372 = bitcast float* %371 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %372, align 4, !tbaa !102
  %373 = trunc i64 %331 to i32
  %374 = or i32 %373, 28
  %375 = add i32 %21, %374
  %376 = sext i32 %375 to i64
  %377 = getelementptr inbounds float, float* %4, i64 %376
  %378 = bitcast float* %377 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %378, align 4, !tbaa !102
  %379 = trunc i64 %331 to i32
  %380 = or i32 %379, 32
  %381 = add i32 %21, %380
  %382 = sext i32 %381 to i64
  %383 = getelementptr inbounds float, float* %4, i64 %382
  %384 = bitcast float* %383 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %384, align 4, !tbaa !102
  %385 = trunc i64 %331 to i32
  %386 = or i32 %385, 36
  %387 = add i32 %21, %386
  %388 = sext i32 %387 to i64
  %389 = getelementptr inbounds float, float* %4, i64 %388
  %390 = bitcast float* %389 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %390, align 4, !tbaa !102
  %391 = trunc i64 %331 to i32
  %392 = or i32 %391, 40
  %393 = add i32 %21, %392
  %394 = sext i32 %393 to i64
  %395 = getelementptr inbounds float, float* %4, i64 %394
  %396 = bitcast float* %395 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %396, align 4, !tbaa !102
  %397 = trunc i64 %331 to i32
  %398 = or i32 %397, 44
  %399 = add i32 %21, %398
  %400 = sext i32 %399 to i64
  %401 = getelementptr inbounds float, float* %4, i64 %400
  %402 = bitcast float* %401 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %402, align 4, !tbaa !102
  %403 = trunc i64 %331 to i32
  %404 = or i32 %403, 48
  %405 = add i32 %21, %404
  %406 = sext i32 %405 to i64
  %407 = getelementptr inbounds float, float* %4, i64 %406
  %408 = bitcast float* %407 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %408, align 4, !tbaa !102
  %409 = trunc i64 %331 to i32
  %410 = or i32 %409, 52
  %411 = add i32 %21, %410
  %412 = sext i32 %411 to i64
  %413 = getelementptr inbounds float, float* %4, i64 %412
  %414 = bitcast float* %413 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %414, align 4, !tbaa !102
  %415 = trunc i64 %331 to i32
  %416 = or i32 %415, 56
  %417 = add i32 %21, %416
  %418 = sext i32 %417 to i64
  %419 = getelementptr inbounds float, float* %4, i64 %418
  %420 = bitcast float* %419 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %420, align 4, !tbaa !102
  %421 = trunc i64 %331 to i32
  %422 = or i32 %421, 60
  %423 = add i32 %21, %422
  %424 = sext i32 %423 to i64
  %425 = getelementptr inbounds float, float* %4, i64 %424
  %426 = bitcast float* %425 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %426, align 4, !tbaa !102
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp eq i64 %indvars.iv.next16, 114
  br i1 %exitcond17, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %427 = add nsw i32 %20, 1
  %428 = icmp slt i32 %427, %15
  br i1 %428, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.6(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 223
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 224
  %24 = select i1 %23, i32 %22, i32 224
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 224
  %27 = select i1 %26, i32 %25, i32 224
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end20
  %29 = phi i32 [ %187, %for_end20 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 28672, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <64 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 256
  %36 = bitcast i8* %35 to <64 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 512
  %38 = bitcast i8* %37 to <64 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 768
  %40 = bitcast i8* %39 to <64 x float>*
  %41 = srem i32 %29, 112
  %42 = sdiv i32 %29, 112
  %43 = mul nsw i32 %42, 73728
  %44 = bitcast i8* %31 to float*
  %45 = sext i32 %43 to i64
  %reass.mul = mul nsw i32 %41, 7296
  %46 = sext i32 %reass.mul to i64
  %47 = mul nsw i32 %41, 7296
  %reass.mul.1 = add nsw i32 %47, 7296
  %48 = sext i32 %reass.mul.1 to i64
  %49 = mul nsw i32 %41, 7296
  %reass.mul.2 = add nsw i32 %49, 14592
  %50 = sext i32 %reass.mul.2 to i64
  br label %for_body4

for_end:                                          ; preds = %for_end20, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end8, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end8 ]
  %51 = shl i64 %indvar, 8
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  br label %for_body7

for_end5:                                         ; preds = %for_end8
  %52 = mul nsw i32 %29, 7168
  %53 = shl nsw i32 %42, 6
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %13, i64 %54
  %56 = bitcast float* %55 to <64 x float>*
  %57 = load <64 x float>, <64 x float>* %56, align 64, !tbaa !105
  br label %for_body19

for_body7:                                        ; preds = %for_end14.2, %for_body4
  %indvars.iv55 = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next56, %for_end14.2 ]
  %.lcssa33.lcssa47 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %308, %for_end14.2 ]
  %.lcssa31.lcssa45 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %302, %for_end14.2 ]
  %.lcssa29.lcssa44 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %301, %for_end14.2 ]
  %.lcssa.lcssa42 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %300, %for_end14.2 ]
  %58 = mul nuw nsw i64 %indvars.iv55, 831744
  %59 = add nuw nsw i64 %58, %51
  %60 = mul nuw nsw i64 %indvars.iv55, 36864
  %61 = add nsw i64 %60, %45
  %62 = add nsw i64 %59, %46
  br label %for_body13

for_end8:                                         ; preds = %for_end14.2
  store <64 x float> %300, <64 x float>* %34, align 64, !tbaa !108
  store <64 x float> %301, <64 x float>* %36, align 64, !tbaa !108
  store <64 x float> %302, <64 x float>* %38, align 64, !tbaa !108
  store <64 x float> %308, <64 x float>* %40, align 64, !tbaa !108
  %63 = getelementptr inbounds float, float* %44, i64 %51
  %64 = bitcast float* %63 to <64 x float>*
  store <64 x float> %300, <64 x float>* %64, align 64, !tbaa !116
  %65 = or i64 %51, 64
  %66 = getelementptr inbounds float, float* %44, i64 %65
  %67 = bitcast float* %66 to <64 x float>*
  store <64 x float> %301, <64 x float>* %67, align 64, !tbaa !116
  %68 = or i64 %51, 128
  %69 = getelementptr inbounds float, float* %44, i64 %68
  %70 = bitcast float* %69 to <64 x float>*
  store <64 x float> %302, <64 x float>* %70, align 64, !tbaa !116
  %71 = or i64 %51, 192
  %72 = getelementptr inbounds float, float* %44, i64 %71
  %73 = bitcast float* %72 to <64 x float>*
  store <64 x float> %308, <64 x float>* %73, align 64, !tbaa !116
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond61 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond61, label %for_end5, label %for_body4, !prof !20

for_body13:                                       ; preds = %for_body13, %for_body7
  %indvars.iv = phi i64 [ 0, %for_body7 ], [ %indvars.iv.next, %for_body13 ]
  %74 = phi <64 x float> [ %.lcssa33.lcssa47, %for_body7 ], [ %132, %for_body13 ]
  %75 = phi <64 x float> [ %.lcssa31.lcssa45, %for_body7 ], [ %126, %for_body13 ]
  %76 = phi <64 x float> [ %.lcssa29.lcssa44, %for_body7 ], [ %125, %for_body13 ]
  %77 = phi <64 x float> [ %.lcssa.lcssa42, %for_body7 ], [ %124, %for_body13 ]
  %78 = add nsw i64 %62, %indvars.iv
  %79 = getelementptr inbounds float, float* %4, i64 %78
  %80 = load float, float* %79, align 4, !tbaa !102
  %81 = insertelement <64 x float> undef, float %80, i32 0
  %82 = shufflevector <64 x float> %81, <64 x float> undef, <64 x i32> zeroinitializer
  %83 = shl nsw i64 %indvars.iv, 6
  %84 = add nsw i64 %61, %83
  %85 = getelementptr inbounds float, float* %7, i64 %84
  %86 = bitcast float* %85 to <64 x float>*
  %87 = load <64 x float>, <64 x float>* %86, align 64, !tbaa !119
  %88 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %82, <64 x float> %87, <64 x float> %77)
  %89 = add nsw i64 %78, 64
  %90 = getelementptr inbounds float, float* %4, i64 %89
  %91 = load float, float* %90, align 4, !tbaa !102
  %92 = insertelement <64 x float> undef, float %91, i32 0
  %93 = shufflevector <64 x float> %92, <64 x float> undef, <64 x i32> zeroinitializer
  %94 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %93, <64 x float> %87, <64 x float> %76)
  %95 = add nsw i64 %78, 128
  %96 = getelementptr inbounds float, float* %4, i64 %95
  %97 = load float, float* %96, align 4, !tbaa !102
  %98 = insertelement <64 x float> undef, float %97, i32 0
  %99 = shufflevector <64 x float> %98, <64 x float> undef, <64 x i32> zeroinitializer
  %100 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %99, <64 x float> %87, <64 x float> %75)
  %101 = add nsw i64 %78, 192
  %102 = getelementptr inbounds float, float* %4, i64 %101
  %103 = load float, float* %102, align 4, !tbaa !102
  %104 = insertelement <64 x float> undef, float %103, i32 0
  %105 = shufflevector <64 x float> %104, <64 x float> undef, <64 x i32> zeroinitializer
  %106 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %105, <64 x float> %87, <64 x float> %74)
  %107 = add nsw i64 %84, 4096
  %108 = getelementptr inbounds float, float* %7, i64 %107
  %109 = bitcast float* %108 to <64 x float>*
  %110 = load <64 x float>, <64 x float>* %109, align 64, !tbaa !119
  %111 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %93, <64 x float> %110, <64 x float> %88)
  %112 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %99, <64 x float> %110, <64 x float> %94)
  %113 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %105, <64 x float> %110, <64 x float> %100)
  %114 = add nsw i64 %78, 256
  %115 = getelementptr inbounds float, float* %4, i64 %114
  %116 = load float, float* %115, align 4, !tbaa !102
  %117 = insertelement <64 x float> undef, float %116, i32 0
  %118 = shufflevector <64 x float> %117, <64 x float> undef, <64 x i32> zeroinitializer
  %119 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %118, <64 x float> %110, <64 x float> %106)
  %120 = add nsw i64 %84, 8192
  %121 = getelementptr inbounds float, float* %7, i64 %120
  %122 = bitcast float* %121 to <64 x float>*
  %123 = load <64 x float>, <64 x float>* %122, align 64, !tbaa !119
  %124 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %99, <64 x float> %123, <64 x float> %111)
  %125 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %105, <64 x float> %123, <64 x float> %112)
  %126 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %118, <64 x float> %123, <64 x float> %113)
  %127 = add nsw i64 %78, 320
  %128 = getelementptr inbounds float, float* %4, i64 %127
  %129 = load float, float* %128, align 4, !tbaa !102
  %130 = insertelement <64 x float> undef, float %129, i32 0
  %131 = shufflevector <64 x float> %130, <64 x float> undef, <64 x i32> zeroinitializer
  %132 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %131, <64 x float> %123, <64 x float> %119)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 64
  br i1 %exitcond, label %for_end14, label %for_body13, !prof !20

for_end14:                                        ; preds = %for_body13
  %133 = add nsw i64 %59, %48
  %134 = add nsw i64 %61, 12288
  br label %for_body13.1

for_body19:                                       ; preds = %for_body19, %for_end5
  %indvars.iv65 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next66, %for_body19 ]
  %135 = shl nsw i64 %indvars.iv65, 8
  %136 = trunc i64 %135 to i32
  %137 = add i32 %52, %136
  %138 = getelementptr inbounds float, float* %44, i64 %135
  %139 = bitcast float* %138 to <64 x float>*
  %140 = load <64 x float>, <64 x float>* %139, align 64, !tbaa !116
  %141 = fadd <64 x float> %57, %140
  %142 = fcmp ogt <64 x float> %141, zeroinitializer
  %143 = select <64 x i1> %142, <64 x float> %141, <64 x float> zeroinitializer
  %144 = sext i32 %137 to i64
  %145 = getelementptr inbounds float, float* %10, i64 %144
  %146 = bitcast float* %145 to <64 x float>*
  store <64 x float> %143, <64 x float>* %146, align 64, !tbaa !122
  %147 = or i64 %135, 64
  %148 = trunc i64 %147 to i32
  %149 = add i32 %52, %148
  %150 = getelementptr inbounds float, float* %44, i64 %147
  %151 = bitcast float* %150 to <64 x float>*
  %152 = load <64 x float>, <64 x float>* %151, align 64, !tbaa !116
  %153 = fadd <64 x float> %57, %152
  %154 = fcmp ogt <64 x float> %153, zeroinitializer
  %155 = select <64 x i1> %154, <64 x float> %153, <64 x float> zeroinitializer
  %156 = sext i32 %149 to i64
  %157 = getelementptr inbounds float, float* %10, i64 %156
  %158 = bitcast float* %157 to <64 x float>*
  store <64 x float> %155, <64 x float>* %158, align 64, !tbaa !122
  %159 = or i64 %135, 128
  %160 = trunc i64 %159 to i32
  %161 = add i32 %52, %160
  %162 = getelementptr inbounds float, float* %44, i64 %159
  %163 = bitcast float* %162 to <64 x float>*
  %164 = load <64 x float>, <64 x float>* %163, align 64, !tbaa !116
  %165 = fadd <64 x float> %57, %164
  %166 = fcmp ogt <64 x float> %165, zeroinitializer
  %167 = select <64 x i1> %166, <64 x float> %165, <64 x float> zeroinitializer
  %168 = sext i32 %161 to i64
  %169 = getelementptr inbounds float, float* %10, i64 %168
  %170 = bitcast float* %169 to <64 x float>*
  store <64 x float> %167, <64 x float>* %170, align 64, !tbaa !122
  %171 = or i64 %135, 192
  %172 = trunc i64 %171 to i32
  %173 = add i32 %52, %172
  %174 = getelementptr inbounds float, float* %44, i64 %171
  %175 = bitcast float* %174 to <64 x float>*
  %176 = load <64 x float>, <64 x float>* %175, align 64, !tbaa !116
  %177 = fadd <64 x float> %57, %176
  %178 = fcmp ogt <64 x float> %177, zeroinitializer
  %179 = select <64 x i1> %178, <64 x float> %177, <64 x float> zeroinitializer
  %180 = sext i32 %173 to i64
  %181 = getelementptr inbounds float, float* %10, i64 %180
  %182 = bitcast float* %181 to <64 x float>*
  store <64 x float> %179, <64 x float>* %182, align 64, !tbaa !122
  %indvars.iv.next66 = add nuw nsw i64 %indvars.iv65, 1
  %exitcond67 = icmp eq i64 %indvars.iv.next66, 28
  br i1 %exitcond67, label %for_end20, label %for_body19, !prof !20

for_end20:                                        ; preds = %for_body19
  %183 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %184 = tail call i32 %183(i32 1, i32 %16, i8* %33)
  %185 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %186 = tail call i32 %185(i32 1, i32 %16, i8* nonnull %31)
  %187 = add nsw i32 %29, 1
  %188 = icmp slt i32 %187, %24
  br i1 %188, label %for_body, label %for_end, !prof !19

for_body13.1:                                     ; preds = %for_body13.1, %for_end14
  %indvars.iv.1 = phi i64 [ 0, %for_end14 ], [ %indvars.iv.next.1, %for_body13.1 ]
  %189 = phi <64 x float> [ %132, %for_end14 ], [ %247, %for_body13.1 ]
  %190 = phi <64 x float> [ %126, %for_end14 ], [ %241, %for_body13.1 ]
  %191 = phi <64 x float> [ %125, %for_end14 ], [ %240, %for_body13.1 ]
  %192 = phi <64 x float> [ %124, %for_end14 ], [ %239, %for_body13.1 ]
  %193 = add nsw i64 %133, %indvars.iv.1
  %194 = getelementptr inbounds float, float* %4, i64 %193
  %195 = load float, float* %194, align 4, !tbaa !102
  %196 = insertelement <64 x float> undef, float %195, i32 0
  %197 = shufflevector <64 x float> %196, <64 x float> undef, <64 x i32> zeroinitializer
  %198 = shl nsw i64 %indvars.iv.1, 6
  %199 = add nsw i64 %134, %198
  %200 = getelementptr inbounds float, float* %7, i64 %199
  %201 = bitcast float* %200 to <64 x float>*
  %202 = load <64 x float>, <64 x float>* %201, align 64, !tbaa !119
  %203 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %197, <64 x float> %202, <64 x float> %192)
  %204 = add nsw i64 %193, 64
  %205 = getelementptr inbounds float, float* %4, i64 %204
  %206 = load float, float* %205, align 4, !tbaa !102
  %207 = insertelement <64 x float> undef, float %206, i32 0
  %208 = shufflevector <64 x float> %207, <64 x float> undef, <64 x i32> zeroinitializer
  %209 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %208, <64 x float> %202, <64 x float> %191)
  %210 = add nsw i64 %193, 128
  %211 = getelementptr inbounds float, float* %4, i64 %210
  %212 = load float, float* %211, align 4, !tbaa !102
  %213 = insertelement <64 x float> undef, float %212, i32 0
  %214 = shufflevector <64 x float> %213, <64 x float> undef, <64 x i32> zeroinitializer
  %215 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %214, <64 x float> %202, <64 x float> %190)
  %216 = add nsw i64 %193, 192
  %217 = getelementptr inbounds float, float* %4, i64 %216
  %218 = load float, float* %217, align 4, !tbaa !102
  %219 = insertelement <64 x float> undef, float %218, i32 0
  %220 = shufflevector <64 x float> %219, <64 x float> undef, <64 x i32> zeroinitializer
  %221 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %220, <64 x float> %202, <64 x float> %189)
  %222 = add nsw i64 %199, 4096
  %223 = getelementptr inbounds float, float* %7, i64 %222
  %224 = bitcast float* %223 to <64 x float>*
  %225 = load <64 x float>, <64 x float>* %224, align 64, !tbaa !119
  %226 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %208, <64 x float> %225, <64 x float> %203)
  %227 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %214, <64 x float> %225, <64 x float> %209)
  %228 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %220, <64 x float> %225, <64 x float> %215)
  %229 = add nsw i64 %193, 256
  %230 = getelementptr inbounds float, float* %4, i64 %229
  %231 = load float, float* %230, align 4, !tbaa !102
  %232 = insertelement <64 x float> undef, float %231, i32 0
  %233 = shufflevector <64 x float> %232, <64 x float> undef, <64 x i32> zeroinitializer
  %234 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %233, <64 x float> %225, <64 x float> %221)
  %235 = add nsw i64 %199, 8192
  %236 = getelementptr inbounds float, float* %7, i64 %235
  %237 = bitcast float* %236 to <64 x float>*
  %238 = load <64 x float>, <64 x float>* %237, align 64, !tbaa !119
  %239 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %214, <64 x float> %238, <64 x float> %226)
  %240 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %220, <64 x float> %238, <64 x float> %227)
  %241 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %233, <64 x float> %238, <64 x float> %228)
  %242 = add nsw i64 %193, 320
  %243 = getelementptr inbounds float, float* %4, i64 %242
  %244 = load float, float* %243, align 4, !tbaa !102
  %245 = insertelement <64 x float> undef, float %244, i32 0
  %246 = shufflevector <64 x float> %245, <64 x float> undef, <64 x i32> zeroinitializer
  %247 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %246, <64 x float> %238, <64 x float> %234)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 64
  br i1 %exitcond.1, label %for_end14.1, label %for_body13.1, !prof !20

for_end14.1:                                      ; preds = %for_body13.1
  %248 = add nsw i64 %59, %50
  %249 = add nsw i64 %61, 24576
  br label %for_body13.2

for_body13.2:                                     ; preds = %for_body13.2, %for_end14.1
  %indvars.iv.2 = phi i64 [ 0, %for_end14.1 ], [ %indvars.iv.next.2, %for_body13.2 ]
  %250 = phi <64 x float> [ %247, %for_end14.1 ], [ %308, %for_body13.2 ]
  %251 = phi <64 x float> [ %241, %for_end14.1 ], [ %302, %for_body13.2 ]
  %252 = phi <64 x float> [ %240, %for_end14.1 ], [ %301, %for_body13.2 ]
  %253 = phi <64 x float> [ %239, %for_end14.1 ], [ %300, %for_body13.2 ]
  %254 = add nsw i64 %248, %indvars.iv.2
  %255 = getelementptr inbounds float, float* %4, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !102
  %257 = insertelement <64 x float> undef, float %256, i32 0
  %258 = shufflevector <64 x float> %257, <64 x float> undef, <64 x i32> zeroinitializer
  %259 = shl nsw i64 %indvars.iv.2, 6
  %260 = add nsw i64 %249, %259
  %261 = getelementptr inbounds float, float* %7, i64 %260
  %262 = bitcast float* %261 to <64 x float>*
  %263 = load <64 x float>, <64 x float>* %262, align 64, !tbaa !119
  %264 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %258, <64 x float> %263, <64 x float> %253)
  %265 = add nsw i64 %254, 64
  %266 = getelementptr inbounds float, float* %4, i64 %265
  %267 = load float, float* %266, align 4, !tbaa !102
  %268 = insertelement <64 x float> undef, float %267, i32 0
  %269 = shufflevector <64 x float> %268, <64 x float> undef, <64 x i32> zeroinitializer
  %270 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %269, <64 x float> %263, <64 x float> %252)
  %271 = add nsw i64 %254, 128
  %272 = getelementptr inbounds float, float* %4, i64 %271
  %273 = load float, float* %272, align 4, !tbaa !102
  %274 = insertelement <64 x float> undef, float %273, i32 0
  %275 = shufflevector <64 x float> %274, <64 x float> undef, <64 x i32> zeroinitializer
  %276 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %275, <64 x float> %263, <64 x float> %251)
  %277 = add nsw i64 %254, 192
  %278 = getelementptr inbounds float, float* %4, i64 %277
  %279 = load float, float* %278, align 4, !tbaa !102
  %280 = insertelement <64 x float> undef, float %279, i32 0
  %281 = shufflevector <64 x float> %280, <64 x float> undef, <64 x i32> zeroinitializer
  %282 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %281, <64 x float> %263, <64 x float> %250)
  %283 = add nsw i64 %260, 4096
  %284 = getelementptr inbounds float, float* %7, i64 %283
  %285 = bitcast float* %284 to <64 x float>*
  %286 = load <64 x float>, <64 x float>* %285, align 64, !tbaa !119
  %287 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %269, <64 x float> %286, <64 x float> %264)
  %288 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %275, <64 x float> %286, <64 x float> %270)
  %289 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %281, <64 x float> %286, <64 x float> %276)
  %290 = add nsw i64 %254, 256
  %291 = getelementptr inbounds float, float* %4, i64 %290
  %292 = load float, float* %291, align 4, !tbaa !102
  %293 = insertelement <64 x float> undef, float %292, i32 0
  %294 = shufflevector <64 x float> %293, <64 x float> undef, <64 x i32> zeroinitializer
  %295 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %294, <64 x float> %286, <64 x float> %282)
  %296 = add nsw i64 %260, 8192
  %297 = getelementptr inbounds float, float* %7, i64 %296
  %298 = bitcast float* %297 to <64 x float>*
  %299 = load <64 x float>, <64 x float>* %298, align 64, !tbaa !119
  %300 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %275, <64 x float> %299, <64 x float> %287)
  %301 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %281, <64 x float> %299, <64 x float> %288)
  %302 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %294, <64 x float> %299, <64 x float> %289)
  %303 = add nsw i64 %254, 320
  %304 = getelementptr inbounds float, float* %4, i64 %303
  %305 = load float, float* %304, align 4, !tbaa !102
  %306 = insertelement <64 x float> undef, float %305, i32 0
  %307 = shufflevector <64 x float> %306, <64 x float> undef, <64 x i32> zeroinitializer
  %308 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %307, <64 x float> %299, <64 x float> %295)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 64
  br i1 %exitcond.2, label %for_end14.2, label %for_body13.2, !prof !20

for_end14.2:                                      ; preds = %for_body13.2
  %indvars.iv.next56 = add nuw nsw i64 %indvars.iv55, 1
  %exitcond57 = icmp eq i64 %indvars.iv.next56, 2
  br i1 %exitcond57, label %for_end8, label %for_body7, !prof !20
}

; Function Attrs: nounwind readnone speculatable
declare <64 x float> @llvm.fmuladd.v64f32(<64 x float>, <64 x float>, <64 x float>) #2

define dllexport i32 @fused_nn_max_pool2d_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !125 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !127, metadata !DIExpression()), !dbg !130
  call void @llvm.dbg.value(metadata i8* %1, metadata !128, metadata !DIExpression()), !dbg !130
  call void @llvm.dbg.value(metadata i32 %2, metadata !129, metadata !DIExpression()), !dbg !130
  %3 = bitcast i8* %0 to %1**, !dbg !130
  %4 = load %1*, %1** %3, align 8, !dbg !130
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !130
  %6 = bitcast i8* %5 to %1**, !dbg !130
  %7 = load %1*, %1** %6, align 8, !dbg !130
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !130
  %9 = load i8*, i8** %8, align 8, !dbg !130
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !130
  %11 = load i8*, i8** %10, align 8, !dbg !130
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_3_compute_(i8* %11, i8* %9), !dbg !130
  ret i32 %12, !dbg !130
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_3_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %11, align 8
  %3 = getelementptr inbounds %11, %11* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %11, %11* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %11* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.7, i8* nonnull %5, i32 0)
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
  %10 = add nsw i32 %9, 111
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 112
  %15 = select i1 %14, i32 %13, i32 112
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 112
  %18 = select i1 %17, i32 %16, i32 112
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
  %24 = mul nsw i64 %indvars.iv7, 3584
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = mul i32 %25, 14336
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %27 = shl i64 %indvars.iv, 6
  %28 = add nsw i64 %27, %24
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <64 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %31 = shl i32 %indvars.iv.tr, 7
  %32 = add i32 %31, %26
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to <64 x float>*
  %36 = load <64 x float>, <64 x float>* %35, align 64, !tbaa !131
  %37 = fcmp olt <64 x float> %36, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %38 = select <64 x i1> %37, <64 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <64 x float> %36
  %39 = or i32 %32, 64
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to <64 x float>*
  %43 = load <64 x float>, <64 x float>* %42, align 64, !tbaa !131
  %44 = fcmp ogt <64 x float> %38, %43
  %45 = select <64 x i1> %44, <64 x float> %38, <64 x float> %43
  %46 = add i32 %32, 7168
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to <64 x float>*
  %50 = load <64 x float>, <64 x float>* %49, align 64, !tbaa !131
  %51 = fcmp ogt <64 x float> %45, %50
  %52 = select <64 x i1> %51, <64 x float> %45, <64 x float> %50
  %53 = or i32 %46, 64
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %7, i64 %54
  %56 = bitcast float* %55 to <64 x float>*
  %57 = load <64 x float>, <64 x float>* %56, align 64, !tbaa !131
  %58 = fcmp ogt <64 x float> %52, %57
  %59 = select <64 x i1> %58, <64 x float> %52, <64 x float> %57
  store <64 x float> %59, <64 x float>* %30, align 64, !tbaa !134
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 56
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %60 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %60, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_layout_transform_18(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !137 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !139, metadata !DIExpression()), !dbg !142
  call void @llvm.dbg.value(metadata i8* %1, metadata !140, metadata !DIExpression()), !dbg !142
  call void @llvm.dbg.value(metadata i32 %2, metadata !141, metadata !DIExpression()), !dbg !142
  %3 = bitcast i8* %0 to %1**, !dbg !142
  %4 = load %1*, %1** %3, align 8, !dbg !142
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !142
  %6 = bitcast i8* %5 to %1**, !dbg !142
  %7 = load %1*, %1** %6, align 8, !dbg !142
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !142
  %9 = load i8*, i8** %8, align 8, !dbg !142
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !142
  %11 = load i8*, i8** %10, align 8, !dbg !142
  %12 = tail call fastcc i32 @fused_layout_transform_18_compute_(i8* %11, i8* %9), !dbg !142
  ret i32 %12, !dbg !142
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_18_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %12, align 8
  %3 = getelementptr inbounds %12, %12* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %12, %12* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %12* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.8, i8* nonnull %5, i32 0)
  ret i32 %7
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
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv10 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next11, %for_end3 ]
  %24 = mul nsw i64 %indvars.iv10, 1792
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = sdiv i32 %25, 56
  %27 = shl nsw i32 %26, 5
  %28 = srem i32 %25, 56
  %29 = mul nsw i32 %28, 3584
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %30 = shl i64 %indvars.iv7, 5
  %31 = add nsw i64 %30, %24
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %32 = shl i32 %indvars.iv7.tr, 6
  %33 = add i32 %29, %32
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %34 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %34, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %35 = add nsw i64 %31, %indvars.iv
  %36 = trunc i64 %indvars.iv to i32
  %37 = add i32 %27, %36
  %38 = srem i32 %37, 64
  %39 = sdiv i32 %37, 64
  %40 = mul nsw i32 %39, 200704
  %41 = add i32 %33, %38
  %42 = add i32 %41, %40
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds float, float* %7, i64 %43
  %45 = bitcast float* %44 to i32*
  %46 = load i32, i32* %45, align 4, !tbaa !143
  %47 = getelementptr inbounds float, float* %4, i64 %35
  %48 = bitcast float* %47 to i32*
  store i32 %46, i32* %48, align 4, !tbaa !146
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 56
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !149 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !151, metadata !DIExpression()), !dbg !154
  call void @llvm.dbg.value(metadata i8* %1, metadata !152, metadata !DIExpression()), !dbg !154
  call void @llvm.dbg.value(metadata i32 %2, metadata !153, metadata !DIExpression()), !dbg !154
  %3 = bitcast i8* %0 to %1**, !dbg !154
  %4 = load %1*, %1** %3, align 8, !dbg !154
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !154
  %6 = bitcast i8* %5 to %1**, !dbg !154
  %7 = load %1*, %1** %6, align 8, !dbg !154
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !154
  %9 = bitcast i8* %8 to %1**, !dbg !154
  %10 = load %1*, %1** %9, align 8, !dbg !154
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !154
  %12 = bitcast i8* %11 to %1**, !dbg !154
  %13 = load %1*, %1** %12, align 8, !dbg !154
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !154
  %15 = load i8*, i8** %14, align 8, !dbg !154
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !154
  %17 = load i32, i32* %16, align 4, !dbg !154
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !154
  %19 = load i8*, i8** %18, align 8, !dbg !154
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !154
  %21 = load i8*, i8** %20, align 8, !dbg !154
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !154
  %23 = load i8*, i8** %22, align 8, !dbg !154
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !154
  ret i32 %24, !dbg !154
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 1722368, i32 2, i32 32)
  %7 = alloca %13, align 8
  %8 = getelementptr inbounds %13, %13* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %13, %13* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %13* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.9, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %14, align 8
  %15 = getelementptr inbounds %14, %14* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %14, %14* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %14, %14* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %14, %14* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %14, %14* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %14* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.10, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.9(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 231
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 232
  %15 = select i1 %14, i32 %13, i32 232
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 232
  %18 = select i1 %17, i32 %16, i32 232
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %451, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 1856
  %22 = srem i32 %20, 58
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 56
  %24 = sdiv i32 %20, 58
  %25 = mul nsw i32 %24, 100352
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 1792
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 5
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 56
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %vector.body, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %vector.body, %for_body2.for_body2.split_crit_edge.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 58
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -1824
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %39, align 4, !tbaa !155
  %40 = sext i32 %32 to i64
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %42, align 4, !tbaa !158
  %43 = or i64 %27, 4
  %44 = trunc i64 %43 to i32
  %45 = add i32 %21, %44
  %46 = trunc i64 %43 to i32
  %47 = add i32 %46, -1824
  %48 = add i32 %26, %47
  %49 = add i32 %48, %25
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to <4 x i32>*
  %wide.load.1 = load <4 x i32>, <4 x i32>* %52, align 4, !tbaa !155
  %53 = sext i32 %45 to i64
  %54 = getelementptr inbounds float, float* %4, i64 %53
  %55 = bitcast float* %54 to <4 x i32>*
  store <4 x i32> %wide.load.1, <4 x i32>* %55, align 4, !tbaa !158
  %56 = or i64 %27, 8
  %57 = trunc i64 %56 to i32
  %58 = add i32 %21, %57
  %59 = trunc i64 %56 to i32
  %60 = add i32 %59, -1824
  %61 = add i32 %26, %60
  %62 = add i32 %61, %25
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %7, i64 %63
  %65 = bitcast float* %64 to <4 x i32>*
  %wide.load.2 = load <4 x i32>, <4 x i32>* %65, align 4, !tbaa !155
  %66 = sext i32 %58 to i64
  %67 = getelementptr inbounds float, float* %4, i64 %66
  %68 = bitcast float* %67 to <4 x i32>*
  store <4 x i32> %wide.load.2, <4 x i32>* %68, align 4, !tbaa !158
  %69 = or i64 %27, 12
  %70 = trunc i64 %69 to i32
  %71 = add i32 %21, %70
  %72 = trunc i64 %69 to i32
  %73 = add i32 %72, -1824
  %74 = add i32 %26, %73
  %75 = add i32 %74, %25
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %7, i64 %76
  %78 = bitcast float* %77 to <4 x i32>*
  %wide.load.3 = load <4 x i32>, <4 x i32>* %78, align 4, !tbaa !155
  %79 = sext i32 %71 to i64
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = bitcast float* %80 to <4 x i32>*
  store <4 x i32> %wide.load.3, <4 x i32>* %81, align 4, !tbaa !158
  %82 = or i64 %27, 16
  %83 = trunc i64 %82 to i32
  %84 = add i32 %21, %83
  %85 = trunc i64 %82 to i32
  %86 = add i32 %85, -1824
  %87 = add i32 %26, %86
  %88 = add i32 %87, %25
  %89 = sext i32 %88 to i64
  %90 = getelementptr inbounds float, float* %7, i64 %89
  %91 = bitcast float* %90 to <4 x i32>*
  %wide.load.4 = load <4 x i32>, <4 x i32>* %91, align 4, !tbaa !155
  %92 = sext i32 %84 to i64
  %93 = getelementptr inbounds float, float* %4, i64 %92
  %94 = bitcast float* %93 to <4 x i32>*
  store <4 x i32> %wide.load.4, <4 x i32>* %94, align 4, !tbaa !158
  %95 = or i64 %27, 20
  %96 = trunc i64 %95 to i32
  %97 = add i32 %21, %96
  %98 = trunc i64 %95 to i32
  %99 = add i32 %98, -1824
  %100 = add i32 %26, %99
  %101 = add i32 %100, %25
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds float, float* %7, i64 %102
  %104 = bitcast float* %103 to <4 x i32>*
  %wide.load.5 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !155
  %105 = sext i32 %97 to i64
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = bitcast float* %106 to <4 x i32>*
  store <4 x i32> %wide.load.5, <4 x i32>* %107, align 4, !tbaa !158
  %108 = or i64 %27, 24
  %109 = trunc i64 %108 to i32
  %110 = add i32 %21, %109
  %111 = trunc i64 %108 to i32
  %112 = add i32 %111, -1824
  %113 = add i32 %26, %112
  %114 = add i32 %113, %25
  %115 = sext i32 %114 to i64
  %116 = getelementptr inbounds float, float* %7, i64 %115
  %117 = bitcast float* %116 to <4 x i32>*
  %wide.load.6 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !155
  %118 = sext i32 %110 to i64
  %119 = getelementptr inbounds float, float* %4, i64 %118
  %120 = bitcast float* %119 to <4 x i32>*
  store <4 x i32> %wide.load.6, <4 x i32>* %120, align 4, !tbaa !158
  %121 = or i64 %27, 28
  %122 = trunc i64 %121 to i32
  %123 = add i32 %21, %122
  %124 = trunc i64 %121 to i32
  %125 = add i32 %124, -1824
  %126 = add i32 %26, %125
  %127 = add i32 %126, %25
  %128 = sext i32 %127 to i64
  %129 = getelementptr inbounds float, float* %7, i64 %128
  %130 = bitcast float* %129 to <4 x i32>*
  %wide.load.7 = load <4 x i32>, <4 x i32>* %130, align 4, !tbaa !155
  %131 = sext i32 %123 to i64
  %132 = getelementptr inbounds float, float* %4, i64 %131
  %133 = bitcast float* %132 to <4 x i32>*
  store <4 x i32> %wide.load.7, <4 x i32>* %133, align 4, !tbaa !158
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %134 = sext i32 %32 to i64
  %135 = getelementptr inbounds float, float* %4, i64 %134
  store float 0.000000e+00, float* %135, align 4, !tbaa !158
  %136 = trunc i64 %27 to i32
  %137 = or i32 %136, 1
  %138 = add i32 %137, %21
  %139 = sext i32 %138 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  store float 0.000000e+00, float* %140, align 4, !tbaa !158
  %141 = trunc i64 %27 to i32
  %142 = or i32 %141, 2
  %143 = add i32 %142, %21
  %144 = sext i32 %143 to i64
  %145 = getelementptr inbounds float, float* %4, i64 %144
  store float 0.000000e+00, float* %145, align 4, !tbaa !158
  %146 = trunc i64 %27 to i32
  %147 = or i32 %146, 3
  %148 = add i32 %147, %21
  %149 = sext i32 %148 to i64
  %150 = getelementptr inbounds float, float* %4, i64 %149
  store float 0.000000e+00, float* %150, align 4, !tbaa !158
  %151 = trunc i64 %27 to i32
  %152 = or i32 %151, 4
  %153 = add i32 %152, %21
  %154 = sext i32 %153 to i64
  %155 = getelementptr inbounds float, float* %4, i64 %154
  store float 0.000000e+00, float* %155, align 4, !tbaa !158
  %156 = trunc i64 %27 to i32
  %157 = or i32 %156, 5
  %158 = add i32 %157, %21
  %159 = sext i32 %158 to i64
  %160 = getelementptr inbounds float, float* %4, i64 %159
  store float 0.000000e+00, float* %160, align 4, !tbaa !158
  %161 = trunc i64 %27 to i32
  %162 = or i32 %161, 6
  %163 = add i32 %162, %21
  %164 = sext i32 %163 to i64
  %165 = getelementptr inbounds float, float* %4, i64 %164
  store float 0.000000e+00, float* %165, align 4, !tbaa !158
  %166 = trunc i64 %27 to i32
  %167 = or i32 %166, 7
  %168 = add i32 %167, %21
  %169 = sext i32 %168 to i64
  %170 = getelementptr inbounds float, float* %4, i64 %169
  store float 0.000000e+00, float* %170, align 4, !tbaa !158
  %171 = trunc i64 %27 to i32
  %172 = or i32 %171, 8
  %173 = add i32 %172, %21
  %174 = sext i32 %173 to i64
  %175 = getelementptr inbounds float, float* %4, i64 %174
  store float 0.000000e+00, float* %175, align 4, !tbaa !158
  %176 = trunc i64 %27 to i32
  %177 = or i32 %176, 9
  %178 = add i32 %177, %21
  %179 = sext i32 %178 to i64
  %180 = getelementptr inbounds float, float* %4, i64 %179
  store float 0.000000e+00, float* %180, align 4, !tbaa !158
  %181 = trunc i64 %27 to i32
  %182 = or i32 %181, 10
  %183 = add i32 %182, %21
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds float, float* %4, i64 %184
  store float 0.000000e+00, float* %185, align 4, !tbaa !158
  %186 = trunc i64 %27 to i32
  %187 = or i32 %186, 11
  %188 = add i32 %187, %21
  %189 = sext i32 %188 to i64
  %190 = getelementptr inbounds float, float* %4, i64 %189
  store float 0.000000e+00, float* %190, align 4, !tbaa !158
  %191 = trunc i64 %27 to i32
  %192 = or i32 %191, 12
  %193 = add i32 %192, %21
  %194 = sext i32 %193 to i64
  %195 = getelementptr inbounds float, float* %4, i64 %194
  store float 0.000000e+00, float* %195, align 4, !tbaa !158
  %196 = trunc i64 %27 to i32
  %197 = or i32 %196, 13
  %198 = add i32 %197, %21
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %4, i64 %199
  store float 0.000000e+00, float* %200, align 4, !tbaa !158
  %201 = trunc i64 %27 to i32
  %202 = or i32 %201, 14
  %203 = add i32 %202, %21
  %204 = sext i32 %203 to i64
  %205 = getelementptr inbounds float, float* %4, i64 %204
  store float 0.000000e+00, float* %205, align 4, !tbaa !158
  %206 = trunc i64 %27 to i32
  %207 = or i32 %206, 15
  %208 = add i32 %207, %21
  %209 = sext i32 %208 to i64
  %210 = getelementptr inbounds float, float* %4, i64 %209
  store float 0.000000e+00, float* %210, align 4, !tbaa !158
  %211 = trunc i64 %27 to i32
  %212 = or i32 %211, 16
  %213 = add i32 %212, %21
  %214 = sext i32 %213 to i64
  %215 = getelementptr inbounds float, float* %4, i64 %214
  store float 0.000000e+00, float* %215, align 4, !tbaa !158
  %216 = trunc i64 %27 to i32
  %217 = or i32 %216, 17
  %218 = add i32 %217, %21
  %219 = sext i32 %218 to i64
  %220 = getelementptr inbounds float, float* %4, i64 %219
  store float 0.000000e+00, float* %220, align 4, !tbaa !158
  %221 = trunc i64 %27 to i32
  %222 = or i32 %221, 18
  %223 = add i32 %222, %21
  %224 = sext i32 %223 to i64
  %225 = getelementptr inbounds float, float* %4, i64 %224
  store float 0.000000e+00, float* %225, align 4, !tbaa !158
  %226 = trunc i64 %27 to i32
  %227 = or i32 %226, 19
  %228 = add i32 %227, %21
  %229 = sext i32 %228 to i64
  %230 = getelementptr inbounds float, float* %4, i64 %229
  store float 0.000000e+00, float* %230, align 4, !tbaa !158
  %231 = trunc i64 %27 to i32
  %232 = or i32 %231, 20
  %233 = add i32 %232, %21
  %234 = sext i32 %233 to i64
  %235 = getelementptr inbounds float, float* %4, i64 %234
  store float 0.000000e+00, float* %235, align 4, !tbaa !158
  %236 = trunc i64 %27 to i32
  %237 = or i32 %236, 21
  %238 = add i32 %237, %21
  %239 = sext i32 %238 to i64
  %240 = getelementptr inbounds float, float* %4, i64 %239
  store float 0.000000e+00, float* %240, align 4, !tbaa !158
  %241 = trunc i64 %27 to i32
  %242 = or i32 %241, 22
  %243 = add i32 %242, %21
  %244 = sext i32 %243 to i64
  %245 = getelementptr inbounds float, float* %4, i64 %244
  store float 0.000000e+00, float* %245, align 4, !tbaa !158
  %246 = trunc i64 %27 to i32
  %247 = or i32 %246, 23
  %248 = add i32 %247, %21
  %249 = sext i32 %248 to i64
  %250 = getelementptr inbounds float, float* %4, i64 %249
  store float 0.000000e+00, float* %250, align 4, !tbaa !158
  %251 = trunc i64 %27 to i32
  %252 = or i32 %251, 24
  %253 = add i32 %252, %21
  %254 = sext i32 %253 to i64
  %255 = getelementptr inbounds float, float* %4, i64 %254
  store float 0.000000e+00, float* %255, align 4, !tbaa !158
  %256 = trunc i64 %27 to i32
  %257 = or i32 %256, 25
  %258 = add i32 %257, %21
  %259 = sext i32 %258 to i64
  %260 = getelementptr inbounds float, float* %4, i64 %259
  store float 0.000000e+00, float* %260, align 4, !tbaa !158
  %261 = trunc i64 %27 to i32
  %262 = or i32 %261, 26
  %263 = add i32 %262, %21
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds float, float* %4, i64 %264
  store float 0.000000e+00, float* %265, align 4, !tbaa !158
  %266 = trunc i64 %27 to i32
  %267 = or i32 %266, 27
  %268 = add i32 %267, %21
  %269 = sext i32 %268 to i64
  %270 = getelementptr inbounds float, float* %4, i64 %269
  store float 0.000000e+00, float* %270, align 4, !tbaa !158
  %271 = trunc i64 %27 to i32
  %272 = or i32 %271, 28
  %273 = add i32 %272, %21
  %274 = sext i32 %273 to i64
  %275 = getelementptr inbounds float, float* %4, i64 %274
  store float 0.000000e+00, float* %275, align 4, !tbaa !158
  %276 = trunc i64 %27 to i32
  %277 = or i32 %276, 29
  %278 = add i32 %277, %21
  %279 = sext i32 %278 to i64
  %280 = getelementptr inbounds float, float* %4, i64 %279
  store float 0.000000e+00, float* %280, align 4, !tbaa !158
  %281 = trunc i64 %27 to i32
  %282 = or i32 %281, 30
  %283 = add i32 %282, %21
  %284 = sext i32 %283 to i64
  %285 = getelementptr inbounds float, float* %4, i64 %284
  store float 0.000000e+00, float* %285, align 4, !tbaa !158
  %286 = trunc i64 %27 to i32
  %287 = or i32 %286, 31
  %288 = add i32 %287, %21
  %289 = sext i32 %288 to i64
  %290 = getelementptr inbounds float, float* %4, i64 %289
  store float 0.000000e+00, float* %290, align 4, !tbaa !158
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %291 = shl nsw i64 %indvars.iv, 5
  %292 = trunc i64 %291 to i32
  %293 = add i32 %21, %292
  %294 = sext i32 %293 to i64
  %295 = getelementptr inbounds float, float* %4, i64 %294
  store float 0.000000e+00, float* %295, align 4, !tbaa !158
  %296 = trunc i64 %291 to i32
  %297 = or i32 %296, 1
  %298 = add i32 %297, %21
  %299 = sext i32 %298 to i64
  %300 = getelementptr inbounds float, float* %4, i64 %299
  store float 0.000000e+00, float* %300, align 4, !tbaa !158
  %301 = trunc i64 %291 to i32
  %302 = or i32 %301, 2
  %303 = add i32 %302, %21
  %304 = sext i32 %303 to i64
  %305 = getelementptr inbounds float, float* %4, i64 %304
  store float 0.000000e+00, float* %305, align 4, !tbaa !158
  %306 = trunc i64 %291 to i32
  %307 = or i32 %306, 3
  %308 = add i32 %307, %21
  %309 = sext i32 %308 to i64
  %310 = getelementptr inbounds float, float* %4, i64 %309
  store float 0.000000e+00, float* %310, align 4, !tbaa !158
  %311 = trunc i64 %291 to i32
  %312 = or i32 %311, 4
  %313 = add i32 %312, %21
  %314 = sext i32 %313 to i64
  %315 = getelementptr inbounds float, float* %4, i64 %314
  store float 0.000000e+00, float* %315, align 4, !tbaa !158
  %316 = trunc i64 %291 to i32
  %317 = or i32 %316, 5
  %318 = add i32 %317, %21
  %319 = sext i32 %318 to i64
  %320 = getelementptr inbounds float, float* %4, i64 %319
  store float 0.000000e+00, float* %320, align 4, !tbaa !158
  %321 = trunc i64 %291 to i32
  %322 = or i32 %321, 6
  %323 = add i32 %322, %21
  %324 = sext i32 %323 to i64
  %325 = getelementptr inbounds float, float* %4, i64 %324
  store float 0.000000e+00, float* %325, align 4, !tbaa !158
  %326 = trunc i64 %291 to i32
  %327 = or i32 %326, 7
  %328 = add i32 %327, %21
  %329 = sext i32 %328 to i64
  %330 = getelementptr inbounds float, float* %4, i64 %329
  store float 0.000000e+00, float* %330, align 4, !tbaa !158
  %331 = trunc i64 %291 to i32
  %332 = or i32 %331, 8
  %333 = add i32 %332, %21
  %334 = sext i32 %333 to i64
  %335 = getelementptr inbounds float, float* %4, i64 %334
  store float 0.000000e+00, float* %335, align 4, !tbaa !158
  %336 = trunc i64 %291 to i32
  %337 = or i32 %336, 9
  %338 = add i32 %337, %21
  %339 = sext i32 %338 to i64
  %340 = getelementptr inbounds float, float* %4, i64 %339
  store float 0.000000e+00, float* %340, align 4, !tbaa !158
  %341 = trunc i64 %291 to i32
  %342 = or i32 %341, 10
  %343 = add i32 %342, %21
  %344 = sext i32 %343 to i64
  %345 = getelementptr inbounds float, float* %4, i64 %344
  store float 0.000000e+00, float* %345, align 4, !tbaa !158
  %346 = trunc i64 %291 to i32
  %347 = or i32 %346, 11
  %348 = add i32 %347, %21
  %349 = sext i32 %348 to i64
  %350 = getelementptr inbounds float, float* %4, i64 %349
  store float 0.000000e+00, float* %350, align 4, !tbaa !158
  %351 = trunc i64 %291 to i32
  %352 = or i32 %351, 12
  %353 = add i32 %352, %21
  %354 = sext i32 %353 to i64
  %355 = getelementptr inbounds float, float* %4, i64 %354
  store float 0.000000e+00, float* %355, align 4, !tbaa !158
  %356 = trunc i64 %291 to i32
  %357 = or i32 %356, 13
  %358 = add i32 %357, %21
  %359 = sext i32 %358 to i64
  %360 = getelementptr inbounds float, float* %4, i64 %359
  store float 0.000000e+00, float* %360, align 4, !tbaa !158
  %361 = trunc i64 %291 to i32
  %362 = or i32 %361, 14
  %363 = add i32 %362, %21
  %364 = sext i32 %363 to i64
  %365 = getelementptr inbounds float, float* %4, i64 %364
  store float 0.000000e+00, float* %365, align 4, !tbaa !158
  %366 = trunc i64 %291 to i32
  %367 = or i32 %366, 15
  %368 = add i32 %367, %21
  %369 = sext i32 %368 to i64
  %370 = getelementptr inbounds float, float* %4, i64 %369
  store float 0.000000e+00, float* %370, align 4, !tbaa !158
  %371 = trunc i64 %291 to i32
  %372 = or i32 %371, 16
  %373 = add i32 %372, %21
  %374 = sext i32 %373 to i64
  %375 = getelementptr inbounds float, float* %4, i64 %374
  store float 0.000000e+00, float* %375, align 4, !tbaa !158
  %376 = trunc i64 %291 to i32
  %377 = or i32 %376, 17
  %378 = add i32 %377, %21
  %379 = sext i32 %378 to i64
  %380 = getelementptr inbounds float, float* %4, i64 %379
  store float 0.000000e+00, float* %380, align 4, !tbaa !158
  %381 = trunc i64 %291 to i32
  %382 = or i32 %381, 18
  %383 = add i32 %382, %21
  %384 = sext i32 %383 to i64
  %385 = getelementptr inbounds float, float* %4, i64 %384
  store float 0.000000e+00, float* %385, align 4, !tbaa !158
  %386 = trunc i64 %291 to i32
  %387 = or i32 %386, 19
  %388 = add i32 %387, %21
  %389 = sext i32 %388 to i64
  %390 = getelementptr inbounds float, float* %4, i64 %389
  store float 0.000000e+00, float* %390, align 4, !tbaa !158
  %391 = trunc i64 %291 to i32
  %392 = or i32 %391, 20
  %393 = add i32 %392, %21
  %394 = sext i32 %393 to i64
  %395 = getelementptr inbounds float, float* %4, i64 %394
  store float 0.000000e+00, float* %395, align 4, !tbaa !158
  %396 = trunc i64 %291 to i32
  %397 = or i32 %396, 21
  %398 = add i32 %397, %21
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds float, float* %4, i64 %399
  store float 0.000000e+00, float* %400, align 4, !tbaa !158
  %401 = trunc i64 %291 to i32
  %402 = or i32 %401, 22
  %403 = add i32 %402, %21
  %404 = sext i32 %403 to i64
  %405 = getelementptr inbounds float, float* %4, i64 %404
  store float 0.000000e+00, float* %405, align 4, !tbaa !158
  %406 = trunc i64 %291 to i32
  %407 = or i32 %406, 23
  %408 = add i32 %407, %21
  %409 = sext i32 %408 to i64
  %410 = getelementptr inbounds float, float* %4, i64 %409
  store float 0.000000e+00, float* %410, align 4, !tbaa !158
  %411 = trunc i64 %291 to i32
  %412 = or i32 %411, 24
  %413 = add i32 %412, %21
  %414 = sext i32 %413 to i64
  %415 = getelementptr inbounds float, float* %4, i64 %414
  store float 0.000000e+00, float* %415, align 4, !tbaa !158
  %416 = trunc i64 %291 to i32
  %417 = or i32 %416, 25
  %418 = add i32 %417, %21
  %419 = sext i32 %418 to i64
  %420 = getelementptr inbounds float, float* %4, i64 %419
  store float 0.000000e+00, float* %420, align 4, !tbaa !158
  %421 = trunc i64 %291 to i32
  %422 = or i32 %421, 26
  %423 = add i32 %422, %21
  %424 = sext i32 %423 to i64
  %425 = getelementptr inbounds float, float* %4, i64 %424
  store float 0.000000e+00, float* %425, align 4, !tbaa !158
  %426 = trunc i64 %291 to i32
  %427 = or i32 %426, 27
  %428 = add i32 %427, %21
  %429 = sext i32 %428 to i64
  %430 = getelementptr inbounds float, float* %4, i64 %429
  store float 0.000000e+00, float* %430, align 4, !tbaa !158
  %431 = trunc i64 %291 to i32
  %432 = or i32 %431, 28
  %433 = add i32 %432, %21
  %434 = sext i32 %433 to i64
  %435 = getelementptr inbounds float, float* %4, i64 %434
  store float 0.000000e+00, float* %435, align 4, !tbaa !158
  %436 = trunc i64 %291 to i32
  %437 = or i32 %436, 29
  %438 = add i32 %437, %21
  %439 = sext i32 %438 to i64
  %440 = getelementptr inbounds float, float* %4, i64 %439
  store float 0.000000e+00, float* %440, align 4, !tbaa !158
  %441 = trunc i64 %291 to i32
  %442 = or i32 %441, 30
  %443 = add i32 %442, %21
  %444 = sext i32 %443 to i64
  %445 = getelementptr inbounds float, float* %4, i64 %444
  store float 0.000000e+00, float* %445, align 4, !tbaa !158
  %446 = trunc i64 %291 to i32
  %447 = or i32 %446, 31
  %448 = add i32 %447, %21
  %449 = sext i32 %448 to i64
  %450 = getelementptr inbounds float, float* %4, i64 %449
  store float 0.000000e+00, float* %450, align 4, !tbaa !158
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 58
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %451 = add nsw i32 %20, 1
  %452 = icmp slt i32 %451, %15
  br i1 %452, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.10(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 447
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 448
  %24 = select i1 %23, i32 %22, i32 448
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 448
  %27 = select i1 %26, i32 %25, i32 448
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end20
  %29 = phi i32 [ %291, %for_end20 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 7168, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <32 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 128
  %36 = bitcast i8* %35 to <32 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 256
  %38 = bitcast i8* %37 to <32 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 384
  %40 = bitcast i8* %39 to <32 x float>*
  %41 = getelementptr inbounds i8, i8* %33, i64 512
  %42 = bitcast i8* %41 to <32 x float>*
  %43 = getelementptr inbounds i8, i8* %33, i64 640
  %44 = bitcast i8* %43 to <32 x float>*
  %45 = getelementptr inbounds i8, i8* %33, i64 768
  %46 = bitcast i8* %45 to <32 x float>*
  %47 = getelementptr inbounds i8, i8* %33, i64 896
  %48 = bitcast i8* %47 to <32 x float>*
  %49 = srem i32 %29, 56
  %50 = sdiv i32 %29, 56
  %51 = mul nsw i32 %50, 36864
  %52 = bitcast i8* %31 to float*
  %53 = sext i32 %51 to i64
  %reass.mul = mul nsw i32 %49, 1856
  %54 = sext i32 %reass.mul to i64
  %55 = mul nsw i32 %49, 1856
  %reass.mul.1 = add nsw i32 %55, 1856
  %56 = sext i32 %reass.mul.1 to i64
  %57 = mul nsw i32 %49, 1856
  %reass.mul.2 = add nsw i32 %57, 3712
  %58 = sext i32 %reass.mul.2 to i64
  br label %for_body4

for_end:                                          ; preds = %for_end20, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end8, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end8 ]
  %59 = shl i64 %indvar, 8
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  br label %for_body7

for_end5:                                         ; preds = %for_end8
  %60 = mul nsw i32 %29, 1792
  %61 = shl nsw i32 %50, 5
  %62 = sext i32 %61 to i64
  %63 = getelementptr inbounds float, float* %13, i64 %62
  %64 = bitcast float* %63 to <32 x float>*
  %65 = load <32 x float>, <32 x float>* %64, align 64, !tbaa !161
  br label %for_body19

for_body7:                                        ; preds = %for_end14.2, %for_body4
  %indvars.iv83 = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next84, %for_end14.2 ]
  %.lcssa41.lcssa71 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %484, %for_end14.2 ]
  %.lcssa39.lcssa69 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %478, %for_end14.2 ]
  %.lcssa37.lcssa67 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %477, %for_end14.2 ]
  %.lcssa35.lcssa65 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %476, %for_end14.2 ]
  %.lcssa33.lcssa63 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %475, %for_end14.2 ]
  %.lcssa31.lcssa61 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %474, %for_end14.2 ]
  %.lcssa29.lcssa60 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %473, %for_end14.2 ]
  %.lcssa.lcssa58 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %472, %for_end14.2 ]
  %66 = mul nuw nsw i64 %indvars.iv83, 107648
  %67 = add nuw nsw i64 %66, %59
  %68 = mul nuw nsw i64 %indvars.iv83, 9216
  %69 = add nsw i64 %68, %53
  %70 = add nsw i64 %67, %54
  br label %for_body13

for_end8:                                         ; preds = %for_end14.2
  store <32 x float> %472, <32 x float>* %34, align 64, !tbaa !164
  store <32 x float> %473, <32 x float>* %36, align 64, !tbaa !164
  store <32 x float> %474, <32 x float>* %38, align 64, !tbaa !164
  store <32 x float> %475, <32 x float>* %40, align 64, !tbaa !164
  store <32 x float> %476, <32 x float>* %42, align 64, !tbaa !164
  store <32 x float> %477, <32 x float>* %44, align 64, !tbaa !164
  store <32 x float> %478, <32 x float>* %46, align 64, !tbaa !164
  store <32 x float> %484, <32 x float>* %48, align 64, !tbaa !164
  %71 = getelementptr inbounds float, float* %52, i64 %59
  %72 = bitcast float* %71 to <32 x float>*
  store <32 x float> %472, <32 x float>* %72, align 64, !tbaa !173
  %73 = or i64 %59, 32
  %74 = getelementptr inbounds float, float* %52, i64 %73
  %75 = bitcast float* %74 to <32 x float>*
  store <32 x float> %473, <32 x float>* %75, align 64, !tbaa !173
  %76 = or i64 %59, 64
  %77 = getelementptr inbounds float, float* %52, i64 %76
  %78 = bitcast float* %77 to <32 x float>*
  store <32 x float> %474, <32 x float>* %78, align 64, !tbaa !173
  %79 = or i64 %59, 96
  %80 = getelementptr inbounds float, float* %52, i64 %79
  %81 = bitcast float* %80 to <32 x float>*
  store <32 x float> %475, <32 x float>* %81, align 64, !tbaa !173
  %82 = or i64 %59, 128
  %83 = getelementptr inbounds float, float* %52, i64 %82
  %84 = bitcast float* %83 to <32 x float>*
  store <32 x float> %476, <32 x float>* %84, align 64, !tbaa !173
  %85 = or i64 %59, 160
  %86 = getelementptr inbounds float, float* %52, i64 %85
  %87 = bitcast float* %86 to <32 x float>*
  store <32 x float> %477, <32 x float>* %87, align 64, !tbaa !173
  %88 = or i64 %59, 192
  %89 = getelementptr inbounds float, float* %52, i64 %88
  %90 = bitcast float* %89 to <32 x float>*
  store <32 x float> %478, <32 x float>* %90, align 64, !tbaa !173
  %91 = or i64 %59, 224
  %92 = getelementptr inbounds float, float* %52, i64 %91
  %93 = bitcast float* %92 to <32 x float>*
  store <32 x float> %484, <32 x float>* %93, align 64, !tbaa !173
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond89 = icmp eq i64 %indvar.next, 7
  br i1 %exitcond89, label %for_end5, label %for_body4, !prof !20

for_body13:                                       ; preds = %for_body13, %for_body7
  %indvars.iv = phi i64 [ 0, %for_body7 ], [ %indvars.iv.next, %for_body13 ]
  %94 = phi <32 x float> [ %.lcssa41.lcssa71, %for_body7 ], [ %188, %for_body13 ]
  %95 = phi <32 x float> [ %.lcssa39.lcssa69, %for_body7 ], [ %182, %for_body13 ]
  %96 = phi <32 x float> [ %.lcssa37.lcssa67, %for_body7 ], [ %181, %for_body13 ]
  %97 = phi <32 x float> [ %.lcssa35.lcssa65, %for_body7 ], [ %180, %for_body13 ]
  %98 = phi <32 x float> [ %.lcssa33.lcssa63, %for_body7 ], [ %179, %for_body13 ]
  %99 = phi <32 x float> [ %.lcssa31.lcssa61, %for_body7 ], [ %178, %for_body13 ]
  %100 = phi <32 x float> [ %.lcssa29.lcssa60, %for_body7 ], [ %177, %for_body13 ]
  %101 = phi <32 x float> [ %.lcssa.lcssa58, %for_body7 ], [ %176, %for_body13 ]
  %102 = add nsw i64 %70, %indvars.iv
  %103 = getelementptr inbounds float, float* %4, i64 %102
  %104 = load float, float* %103, align 4, !tbaa !158
  %105 = insertelement <32 x float> undef, float %104, i32 0
  %106 = shufflevector <32 x float> %105, <32 x float> undef, <32 x i32> zeroinitializer
  %107 = shl nsw i64 %indvars.iv, 5
  %108 = add nsw i64 %69, %107
  %109 = getelementptr inbounds float, float* %7, i64 %108
  %110 = bitcast float* %109 to <32 x float>*
  %111 = load <32 x float>, <32 x float>* %110, align 64, !tbaa !176
  %112 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %106, <32 x float> %111, <32 x float> %101)
  %113 = add nsw i64 %102, 32
  %114 = getelementptr inbounds float, float* %4, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !158
  %116 = insertelement <32 x float> undef, float %115, i32 0
  %117 = shufflevector <32 x float> %116, <32 x float> undef, <32 x i32> zeroinitializer
  %118 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %117, <32 x float> %111, <32 x float> %100)
  %119 = add nsw i64 %102, 64
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = load float, float* %120, align 4, !tbaa !158
  %122 = insertelement <32 x float> undef, float %121, i32 0
  %123 = shufflevector <32 x float> %122, <32 x float> undef, <32 x i32> zeroinitializer
  %124 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %123, <32 x float> %111, <32 x float> %99)
  %125 = add nsw i64 %102, 96
  %126 = getelementptr inbounds float, float* %4, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !158
  %128 = insertelement <32 x float> undef, float %127, i32 0
  %129 = shufflevector <32 x float> %128, <32 x float> undef, <32 x i32> zeroinitializer
  %130 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %129, <32 x float> %111, <32 x float> %98)
  %131 = add nsw i64 %102, 128
  %132 = getelementptr inbounds float, float* %4, i64 %131
  %133 = load float, float* %132, align 4, !tbaa !158
  %134 = insertelement <32 x float> undef, float %133, i32 0
  %135 = shufflevector <32 x float> %134, <32 x float> undef, <32 x i32> zeroinitializer
  %136 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %135, <32 x float> %111, <32 x float> %97)
  %137 = add nsw i64 %102, 160
  %138 = getelementptr inbounds float, float* %4, i64 %137
  %139 = load float, float* %138, align 4, !tbaa !158
  %140 = insertelement <32 x float> undef, float %139, i32 0
  %141 = shufflevector <32 x float> %140, <32 x float> undef, <32 x i32> zeroinitializer
  %142 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %141, <32 x float> %111, <32 x float> %96)
  %143 = add nsw i64 %102, 192
  %144 = getelementptr inbounds float, float* %4, i64 %143
  %145 = load float, float* %144, align 4, !tbaa !158
  %146 = insertelement <32 x float> undef, float %145, i32 0
  %147 = shufflevector <32 x float> %146, <32 x float> undef, <32 x i32> zeroinitializer
  %148 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %147, <32 x float> %111, <32 x float> %95)
  %149 = add nsw i64 %102, 224
  %150 = getelementptr inbounds float, float* %4, i64 %149
  %151 = load float, float* %150, align 4, !tbaa !158
  %152 = insertelement <32 x float> undef, float %151, i32 0
  %153 = shufflevector <32 x float> %152, <32 x float> undef, <32 x i32> zeroinitializer
  %154 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %153, <32 x float> %111, <32 x float> %94)
  %155 = add nsw i64 %108, 1024
  %156 = getelementptr inbounds float, float* %7, i64 %155
  %157 = bitcast float* %156 to <32 x float>*
  %158 = load <32 x float>, <32 x float>* %157, align 64, !tbaa !176
  %159 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %117, <32 x float> %158, <32 x float> %112)
  %160 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %123, <32 x float> %158, <32 x float> %118)
  %161 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %129, <32 x float> %158, <32 x float> %124)
  %162 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %135, <32 x float> %158, <32 x float> %130)
  %163 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %141, <32 x float> %158, <32 x float> %136)
  %164 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %147, <32 x float> %158, <32 x float> %142)
  %165 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %153, <32 x float> %158, <32 x float> %148)
  %166 = add nsw i64 %102, 256
  %167 = getelementptr inbounds float, float* %4, i64 %166
  %168 = load float, float* %167, align 4, !tbaa !158
  %169 = insertelement <32 x float> undef, float %168, i32 0
  %170 = shufflevector <32 x float> %169, <32 x float> undef, <32 x i32> zeroinitializer
  %171 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %170, <32 x float> %158, <32 x float> %154)
  %172 = add nsw i64 %108, 2048
  %173 = getelementptr inbounds float, float* %7, i64 %172
  %174 = bitcast float* %173 to <32 x float>*
  %175 = load <32 x float>, <32 x float>* %174, align 64, !tbaa !176
  %176 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %123, <32 x float> %175, <32 x float> %159)
  %177 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %129, <32 x float> %175, <32 x float> %160)
  %178 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %135, <32 x float> %175, <32 x float> %161)
  %179 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %141, <32 x float> %175, <32 x float> %162)
  %180 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %147, <32 x float> %175, <32 x float> %163)
  %181 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %153, <32 x float> %175, <32 x float> %164)
  %182 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %170, <32 x float> %175, <32 x float> %165)
  %183 = add nsw i64 %102, 288
  %184 = getelementptr inbounds float, float* %4, i64 %183
  %185 = load float, float* %184, align 4, !tbaa !158
  %186 = insertelement <32 x float> undef, float %185, i32 0
  %187 = shufflevector <32 x float> %186, <32 x float> undef, <32 x i32> zeroinitializer
  %188 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %187, <32 x float> %175, <32 x float> %171)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end14, label %for_body13, !prof !20

for_end14:                                        ; preds = %for_body13
  %189 = add nsw i64 %67, %56
  %190 = add nsw i64 %69, 3072
  br label %for_body13.1

for_body19:                                       ; preds = %for_body19, %for_end5
  %indvars.iv93 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next94, %for_body19 ]
  %191 = shl nsw i64 %indvars.iv93, 8
  %192 = trunc i64 %191 to i32
  %193 = add i32 %60, %192
  %194 = getelementptr inbounds float, float* %52, i64 %191
  %195 = bitcast float* %194 to <32 x float>*
  %196 = load <32 x float>, <32 x float>* %195, align 64, !tbaa !173
  %197 = fadd <32 x float> %65, %196
  %198 = fcmp ogt <32 x float> %197, zeroinitializer
  %199 = select <32 x i1> %198, <32 x float> %197, <32 x float> zeroinitializer
  %200 = sext i32 %193 to i64
  %201 = getelementptr inbounds float, float* %10, i64 %200
  %202 = bitcast float* %201 to <32 x float>*
  store <32 x float> %199, <32 x float>* %202, align 64, !tbaa !179
  %203 = or i64 %191, 32
  %204 = trunc i64 %203 to i32
  %205 = add i32 %60, %204
  %206 = getelementptr inbounds float, float* %52, i64 %203
  %207 = bitcast float* %206 to <32 x float>*
  %208 = load <32 x float>, <32 x float>* %207, align 64, !tbaa !173
  %209 = fadd <32 x float> %65, %208
  %210 = fcmp ogt <32 x float> %209, zeroinitializer
  %211 = select <32 x i1> %210, <32 x float> %209, <32 x float> zeroinitializer
  %212 = sext i32 %205 to i64
  %213 = getelementptr inbounds float, float* %10, i64 %212
  %214 = bitcast float* %213 to <32 x float>*
  store <32 x float> %211, <32 x float>* %214, align 64, !tbaa !179
  %215 = or i64 %191, 64
  %216 = trunc i64 %215 to i32
  %217 = add i32 %60, %216
  %218 = getelementptr inbounds float, float* %52, i64 %215
  %219 = bitcast float* %218 to <32 x float>*
  %220 = load <32 x float>, <32 x float>* %219, align 64, !tbaa !173
  %221 = fadd <32 x float> %65, %220
  %222 = fcmp ogt <32 x float> %221, zeroinitializer
  %223 = select <32 x i1> %222, <32 x float> %221, <32 x float> zeroinitializer
  %224 = sext i32 %217 to i64
  %225 = getelementptr inbounds float, float* %10, i64 %224
  %226 = bitcast float* %225 to <32 x float>*
  store <32 x float> %223, <32 x float>* %226, align 64, !tbaa !179
  %227 = or i64 %191, 96
  %228 = trunc i64 %227 to i32
  %229 = add i32 %60, %228
  %230 = getelementptr inbounds float, float* %52, i64 %227
  %231 = bitcast float* %230 to <32 x float>*
  %232 = load <32 x float>, <32 x float>* %231, align 64, !tbaa !173
  %233 = fadd <32 x float> %65, %232
  %234 = fcmp ogt <32 x float> %233, zeroinitializer
  %235 = select <32 x i1> %234, <32 x float> %233, <32 x float> zeroinitializer
  %236 = sext i32 %229 to i64
  %237 = getelementptr inbounds float, float* %10, i64 %236
  %238 = bitcast float* %237 to <32 x float>*
  store <32 x float> %235, <32 x float>* %238, align 64, !tbaa !179
  %239 = or i64 %191, 128
  %240 = trunc i64 %239 to i32
  %241 = add i32 %60, %240
  %242 = getelementptr inbounds float, float* %52, i64 %239
  %243 = bitcast float* %242 to <32 x float>*
  %244 = load <32 x float>, <32 x float>* %243, align 64, !tbaa !173
  %245 = fadd <32 x float> %65, %244
  %246 = fcmp ogt <32 x float> %245, zeroinitializer
  %247 = select <32 x i1> %246, <32 x float> %245, <32 x float> zeroinitializer
  %248 = sext i32 %241 to i64
  %249 = getelementptr inbounds float, float* %10, i64 %248
  %250 = bitcast float* %249 to <32 x float>*
  store <32 x float> %247, <32 x float>* %250, align 64, !tbaa !179
  %251 = or i64 %191, 160
  %252 = trunc i64 %251 to i32
  %253 = add i32 %60, %252
  %254 = getelementptr inbounds float, float* %52, i64 %251
  %255 = bitcast float* %254 to <32 x float>*
  %256 = load <32 x float>, <32 x float>* %255, align 64, !tbaa !173
  %257 = fadd <32 x float> %65, %256
  %258 = fcmp ogt <32 x float> %257, zeroinitializer
  %259 = select <32 x i1> %258, <32 x float> %257, <32 x float> zeroinitializer
  %260 = sext i32 %253 to i64
  %261 = getelementptr inbounds float, float* %10, i64 %260
  %262 = bitcast float* %261 to <32 x float>*
  store <32 x float> %259, <32 x float>* %262, align 64, !tbaa !179
  %263 = or i64 %191, 192
  %264 = trunc i64 %263 to i32
  %265 = add i32 %60, %264
  %266 = getelementptr inbounds float, float* %52, i64 %263
  %267 = bitcast float* %266 to <32 x float>*
  %268 = load <32 x float>, <32 x float>* %267, align 64, !tbaa !173
  %269 = fadd <32 x float> %65, %268
  %270 = fcmp ogt <32 x float> %269, zeroinitializer
  %271 = select <32 x i1> %270, <32 x float> %269, <32 x float> zeroinitializer
  %272 = sext i32 %265 to i64
  %273 = getelementptr inbounds float, float* %10, i64 %272
  %274 = bitcast float* %273 to <32 x float>*
  store <32 x float> %271, <32 x float>* %274, align 64, !tbaa !179
  %275 = or i64 %191, 224
  %276 = trunc i64 %275 to i32
  %277 = add i32 %60, %276
  %278 = getelementptr inbounds float, float* %52, i64 %275
  %279 = bitcast float* %278 to <32 x float>*
  %280 = load <32 x float>, <32 x float>* %279, align 64, !tbaa !173
  %281 = fadd <32 x float> %65, %280
  %282 = fcmp ogt <32 x float> %281, zeroinitializer
  %283 = select <32 x i1> %282, <32 x float> %281, <32 x float> zeroinitializer
  %284 = sext i32 %277 to i64
  %285 = getelementptr inbounds float, float* %10, i64 %284
  %286 = bitcast float* %285 to <32 x float>*
  store <32 x float> %283, <32 x float>* %286, align 64, !tbaa !179
  %indvars.iv.next94 = add nuw nsw i64 %indvars.iv93, 1
  %exitcond95 = icmp eq i64 %indvars.iv.next94, 7
  br i1 %exitcond95, label %for_end20, label %for_body19, !prof !20

for_end20:                                        ; preds = %for_body19
  %287 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %288 = tail call i32 %287(i32 1, i32 %16, i8* %33)
  %289 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %290 = tail call i32 %289(i32 1, i32 %16, i8* nonnull %31)
  %291 = add nsw i32 %29, 1
  %292 = icmp slt i32 %291, %24
  br i1 %292, label %for_body, label %for_end, !prof !19

for_body13.1:                                     ; preds = %for_body13.1, %for_end14
  %indvars.iv.1 = phi i64 [ 0, %for_end14 ], [ %indvars.iv.next.1, %for_body13.1 ]
  %293 = phi <32 x float> [ %188, %for_end14 ], [ %387, %for_body13.1 ]
  %294 = phi <32 x float> [ %182, %for_end14 ], [ %381, %for_body13.1 ]
  %295 = phi <32 x float> [ %181, %for_end14 ], [ %380, %for_body13.1 ]
  %296 = phi <32 x float> [ %180, %for_end14 ], [ %379, %for_body13.1 ]
  %297 = phi <32 x float> [ %179, %for_end14 ], [ %378, %for_body13.1 ]
  %298 = phi <32 x float> [ %178, %for_end14 ], [ %377, %for_body13.1 ]
  %299 = phi <32 x float> [ %177, %for_end14 ], [ %376, %for_body13.1 ]
  %300 = phi <32 x float> [ %176, %for_end14 ], [ %375, %for_body13.1 ]
  %301 = add nsw i64 %189, %indvars.iv.1
  %302 = getelementptr inbounds float, float* %4, i64 %301
  %303 = load float, float* %302, align 4, !tbaa !158
  %304 = insertelement <32 x float> undef, float %303, i32 0
  %305 = shufflevector <32 x float> %304, <32 x float> undef, <32 x i32> zeroinitializer
  %306 = shl nsw i64 %indvars.iv.1, 5
  %307 = add nsw i64 %190, %306
  %308 = getelementptr inbounds float, float* %7, i64 %307
  %309 = bitcast float* %308 to <32 x float>*
  %310 = load <32 x float>, <32 x float>* %309, align 64, !tbaa !176
  %311 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %305, <32 x float> %310, <32 x float> %300)
  %312 = add nsw i64 %301, 32
  %313 = getelementptr inbounds float, float* %4, i64 %312
  %314 = load float, float* %313, align 4, !tbaa !158
  %315 = insertelement <32 x float> undef, float %314, i32 0
  %316 = shufflevector <32 x float> %315, <32 x float> undef, <32 x i32> zeroinitializer
  %317 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %316, <32 x float> %310, <32 x float> %299)
  %318 = add nsw i64 %301, 64
  %319 = getelementptr inbounds float, float* %4, i64 %318
  %320 = load float, float* %319, align 4, !tbaa !158
  %321 = insertelement <32 x float> undef, float %320, i32 0
  %322 = shufflevector <32 x float> %321, <32 x float> undef, <32 x i32> zeroinitializer
  %323 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %322, <32 x float> %310, <32 x float> %298)
  %324 = add nsw i64 %301, 96
  %325 = getelementptr inbounds float, float* %4, i64 %324
  %326 = load float, float* %325, align 4, !tbaa !158
  %327 = insertelement <32 x float> undef, float %326, i32 0
  %328 = shufflevector <32 x float> %327, <32 x float> undef, <32 x i32> zeroinitializer
  %329 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %328, <32 x float> %310, <32 x float> %297)
  %330 = add nsw i64 %301, 128
  %331 = getelementptr inbounds float, float* %4, i64 %330
  %332 = load float, float* %331, align 4, !tbaa !158
  %333 = insertelement <32 x float> undef, float %332, i32 0
  %334 = shufflevector <32 x float> %333, <32 x float> undef, <32 x i32> zeroinitializer
  %335 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %334, <32 x float> %310, <32 x float> %296)
  %336 = add nsw i64 %301, 160
  %337 = getelementptr inbounds float, float* %4, i64 %336
  %338 = load float, float* %337, align 4, !tbaa !158
  %339 = insertelement <32 x float> undef, float %338, i32 0
  %340 = shufflevector <32 x float> %339, <32 x float> undef, <32 x i32> zeroinitializer
  %341 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %340, <32 x float> %310, <32 x float> %295)
  %342 = add nsw i64 %301, 192
  %343 = getelementptr inbounds float, float* %4, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !158
  %345 = insertelement <32 x float> undef, float %344, i32 0
  %346 = shufflevector <32 x float> %345, <32 x float> undef, <32 x i32> zeroinitializer
  %347 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %346, <32 x float> %310, <32 x float> %294)
  %348 = add nsw i64 %301, 224
  %349 = getelementptr inbounds float, float* %4, i64 %348
  %350 = load float, float* %349, align 4, !tbaa !158
  %351 = insertelement <32 x float> undef, float %350, i32 0
  %352 = shufflevector <32 x float> %351, <32 x float> undef, <32 x i32> zeroinitializer
  %353 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %352, <32 x float> %310, <32 x float> %293)
  %354 = add nsw i64 %307, 1024
  %355 = getelementptr inbounds float, float* %7, i64 %354
  %356 = bitcast float* %355 to <32 x float>*
  %357 = load <32 x float>, <32 x float>* %356, align 64, !tbaa !176
  %358 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %316, <32 x float> %357, <32 x float> %311)
  %359 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %322, <32 x float> %357, <32 x float> %317)
  %360 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %328, <32 x float> %357, <32 x float> %323)
  %361 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %334, <32 x float> %357, <32 x float> %329)
  %362 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %340, <32 x float> %357, <32 x float> %335)
  %363 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %346, <32 x float> %357, <32 x float> %341)
  %364 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %352, <32 x float> %357, <32 x float> %347)
  %365 = add nsw i64 %301, 256
  %366 = getelementptr inbounds float, float* %4, i64 %365
  %367 = load float, float* %366, align 4, !tbaa !158
  %368 = insertelement <32 x float> undef, float %367, i32 0
  %369 = shufflevector <32 x float> %368, <32 x float> undef, <32 x i32> zeroinitializer
  %370 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %369, <32 x float> %357, <32 x float> %353)
  %371 = add nsw i64 %307, 2048
  %372 = getelementptr inbounds float, float* %7, i64 %371
  %373 = bitcast float* %372 to <32 x float>*
  %374 = load <32 x float>, <32 x float>* %373, align 64, !tbaa !176
  %375 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %322, <32 x float> %374, <32 x float> %358)
  %376 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %328, <32 x float> %374, <32 x float> %359)
  %377 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %334, <32 x float> %374, <32 x float> %360)
  %378 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %340, <32 x float> %374, <32 x float> %361)
  %379 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %346, <32 x float> %374, <32 x float> %362)
  %380 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %352, <32 x float> %374, <32 x float> %363)
  %381 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %369, <32 x float> %374, <32 x float> %364)
  %382 = add nsw i64 %301, 288
  %383 = getelementptr inbounds float, float* %4, i64 %382
  %384 = load float, float* %383, align 4, !tbaa !158
  %385 = insertelement <32 x float> undef, float %384, i32 0
  %386 = shufflevector <32 x float> %385, <32 x float> undef, <32 x i32> zeroinitializer
  %387 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %386, <32 x float> %374, <32 x float> %370)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 32
  br i1 %exitcond.1, label %for_end14.1, label %for_body13.1, !prof !20

for_end14.1:                                      ; preds = %for_body13.1
  %388 = add nsw i64 %67, %58
  %389 = add nsw i64 %69, 6144
  br label %for_body13.2

for_body13.2:                                     ; preds = %for_body13.2, %for_end14.1
  %indvars.iv.2 = phi i64 [ 0, %for_end14.1 ], [ %indvars.iv.next.2, %for_body13.2 ]
  %390 = phi <32 x float> [ %387, %for_end14.1 ], [ %484, %for_body13.2 ]
  %391 = phi <32 x float> [ %381, %for_end14.1 ], [ %478, %for_body13.2 ]
  %392 = phi <32 x float> [ %380, %for_end14.1 ], [ %477, %for_body13.2 ]
  %393 = phi <32 x float> [ %379, %for_end14.1 ], [ %476, %for_body13.2 ]
  %394 = phi <32 x float> [ %378, %for_end14.1 ], [ %475, %for_body13.2 ]
  %395 = phi <32 x float> [ %377, %for_end14.1 ], [ %474, %for_body13.2 ]
  %396 = phi <32 x float> [ %376, %for_end14.1 ], [ %473, %for_body13.2 ]
  %397 = phi <32 x float> [ %375, %for_end14.1 ], [ %472, %for_body13.2 ]
  %398 = add nsw i64 %388, %indvars.iv.2
  %399 = getelementptr inbounds float, float* %4, i64 %398
  %400 = load float, float* %399, align 4, !tbaa !158
  %401 = insertelement <32 x float> undef, float %400, i32 0
  %402 = shufflevector <32 x float> %401, <32 x float> undef, <32 x i32> zeroinitializer
  %403 = shl nsw i64 %indvars.iv.2, 5
  %404 = add nsw i64 %389, %403
  %405 = getelementptr inbounds float, float* %7, i64 %404
  %406 = bitcast float* %405 to <32 x float>*
  %407 = load <32 x float>, <32 x float>* %406, align 64, !tbaa !176
  %408 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %402, <32 x float> %407, <32 x float> %397)
  %409 = add nsw i64 %398, 32
  %410 = getelementptr inbounds float, float* %4, i64 %409
  %411 = load float, float* %410, align 4, !tbaa !158
  %412 = insertelement <32 x float> undef, float %411, i32 0
  %413 = shufflevector <32 x float> %412, <32 x float> undef, <32 x i32> zeroinitializer
  %414 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %413, <32 x float> %407, <32 x float> %396)
  %415 = add nsw i64 %398, 64
  %416 = getelementptr inbounds float, float* %4, i64 %415
  %417 = load float, float* %416, align 4, !tbaa !158
  %418 = insertelement <32 x float> undef, float %417, i32 0
  %419 = shufflevector <32 x float> %418, <32 x float> undef, <32 x i32> zeroinitializer
  %420 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %407, <32 x float> %395)
  %421 = add nsw i64 %398, 96
  %422 = getelementptr inbounds float, float* %4, i64 %421
  %423 = load float, float* %422, align 4, !tbaa !158
  %424 = insertelement <32 x float> undef, float %423, i32 0
  %425 = shufflevector <32 x float> %424, <32 x float> undef, <32 x i32> zeroinitializer
  %426 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %407, <32 x float> %394)
  %427 = add nsw i64 %398, 128
  %428 = getelementptr inbounds float, float* %4, i64 %427
  %429 = load float, float* %428, align 4, !tbaa !158
  %430 = insertelement <32 x float> undef, float %429, i32 0
  %431 = shufflevector <32 x float> %430, <32 x float> undef, <32 x i32> zeroinitializer
  %432 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %431, <32 x float> %407, <32 x float> %393)
  %433 = add nsw i64 %398, 160
  %434 = getelementptr inbounds float, float* %4, i64 %433
  %435 = load float, float* %434, align 4, !tbaa !158
  %436 = insertelement <32 x float> undef, float %435, i32 0
  %437 = shufflevector <32 x float> %436, <32 x float> undef, <32 x i32> zeroinitializer
  %438 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %437, <32 x float> %407, <32 x float> %392)
  %439 = add nsw i64 %398, 192
  %440 = getelementptr inbounds float, float* %4, i64 %439
  %441 = load float, float* %440, align 4, !tbaa !158
  %442 = insertelement <32 x float> undef, float %441, i32 0
  %443 = shufflevector <32 x float> %442, <32 x float> undef, <32 x i32> zeroinitializer
  %444 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %443, <32 x float> %407, <32 x float> %391)
  %445 = add nsw i64 %398, 224
  %446 = getelementptr inbounds float, float* %4, i64 %445
  %447 = load float, float* %446, align 4, !tbaa !158
  %448 = insertelement <32 x float> undef, float %447, i32 0
  %449 = shufflevector <32 x float> %448, <32 x float> undef, <32 x i32> zeroinitializer
  %450 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %449, <32 x float> %407, <32 x float> %390)
  %451 = add nsw i64 %404, 1024
  %452 = getelementptr inbounds float, float* %7, i64 %451
  %453 = bitcast float* %452 to <32 x float>*
  %454 = load <32 x float>, <32 x float>* %453, align 64, !tbaa !176
  %455 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %413, <32 x float> %454, <32 x float> %408)
  %456 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %454, <32 x float> %414)
  %457 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %454, <32 x float> %420)
  %458 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %431, <32 x float> %454, <32 x float> %426)
  %459 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %437, <32 x float> %454, <32 x float> %432)
  %460 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %443, <32 x float> %454, <32 x float> %438)
  %461 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %449, <32 x float> %454, <32 x float> %444)
  %462 = add nsw i64 %398, 256
  %463 = getelementptr inbounds float, float* %4, i64 %462
  %464 = load float, float* %463, align 4, !tbaa !158
  %465 = insertelement <32 x float> undef, float %464, i32 0
  %466 = shufflevector <32 x float> %465, <32 x float> undef, <32 x i32> zeroinitializer
  %467 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %466, <32 x float> %454, <32 x float> %450)
  %468 = add nsw i64 %404, 2048
  %469 = getelementptr inbounds float, float* %7, i64 %468
  %470 = bitcast float* %469 to <32 x float>*
  %471 = load <32 x float>, <32 x float>* %470, align 64, !tbaa !176
  %472 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %471, <32 x float> %455)
  %473 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %471, <32 x float> %456)
  %474 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %431, <32 x float> %471, <32 x float> %457)
  %475 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %437, <32 x float> %471, <32 x float> %458)
  %476 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %443, <32 x float> %471, <32 x float> %459)
  %477 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %449, <32 x float> %471, <32 x float> %460)
  %478 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %466, <32 x float> %471, <32 x float> %461)
  %479 = add nsw i64 %398, 288
  %480 = getelementptr inbounds float, float* %4, i64 %479
  %481 = load float, float* %480, align 4, !tbaa !158
  %482 = insertelement <32 x float> undef, float %481, i32 0
  %483 = shufflevector <32 x float> %482, <32 x float> undef, <32 x i32> zeroinitializer
  %484 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %483, <32 x float> %471, <32 x float> %467)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 32
  br i1 %exitcond.2, label %for_end14.2, label %for_body13.2, !prof !20

for_end14.2:                                      ; preds = %for_body13.2
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 4
  br i1 %exitcond85, label %for_end8, label %for_body7, !prof !20
}

define dllexport i32 @fused_layout_transform_17(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !182 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !184, metadata !DIExpression()), !dbg !187
  call void @llvm.dbg.value(metadata i8* %1, metadata !185, metadata !DIExpression()), !dbg !187
  call void @llvm.dbg.value(metadata i32 %2, metadata !186, metadata !DIExpression()), !dbg !187
  %3 = bitcast i8* %0 to %1**, !dbg !187
  %4 = load %1*, %1** %3, align 8, !dbg !187
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !187
  %6 = bitcast i8* %5 to %1**, !dbg !187
  %7 = load %1*, %1** %6, align 8, !dbg !187
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !187
  %9 = load i8*, i8** %8, align 8, !dbg !187
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !187
  %11 = load i8*, i8** %10, align 8, !dbg !187
  %12 = tail call fastcc i32 @fused_layout_transform_17_compute_(i8* %11, i8* %9), !dbg !187
  ret i32 %12, !dbg !187
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_17_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %15, align 8
  %3 = getelementptr inbounds %15, %15* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %15, %15* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %15* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.11, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.11(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 55
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 56
  %15 = select i1 %14, i32 %13, i32 56
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 56
  %18 = select i1 %17, i32 %16, i32 56
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
  %24 = mul nsw i64 %indvars.iv10, 14336
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = mul i32 %25, 1792
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %27 = shl i64 %indvars.iv7, 8
  %28 = add nsw i64 %27, %24
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %29 = shl i32 %indvars.iv7.tr, 5
  %30 = add i32 %29, %26
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %31 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %31, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %32 = add nsw i64 %28, %indvars.iv
  %33 = trunc i64 %indvars.iv to i32
  %34 = and i32 %33, 31
  %35 = lshr i32 %33, 5
  %36 = mul nsw i32 %35, 100352
  %37 = add i32 %30, %36
  %38 = or i32 %37, %34
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, float* %7, i64 %39
  %41 = bitcast float* %40 to i32*
  %42 = load i32, i32* %41, align 4, !tbaa !188
  %43 = getelementptr inbounds float, float* %4, i64 %32
  %44 = bitcast float* %43 to i32*
  store i32 %42, i32* %44, align 4, !tbaa !191
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 56
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_nn_dense_add_nn_relu_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !194 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !196, metadata !DIExpression()), !dbg !199
  call void @llvm.dbg.value(metadata i8* %1, metadata !197, metadata !DIExpression()), !dbg !199
  call void @llvm.dbg.value(metadata i32 %2, metadata !198, metadata !DIExpression()), !dbg !199
  %3 = bitcast i8* %0 to %1**, !dbg !199
  %4 = load %1*, %1** %3, align 8, !dbg !199
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !199
  %6 = bitcast i8* %5 to %1**, !dbg !199
  %7 = load %1*, %1** %6, align 8, !dbg !199
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !199
  %9 = bitcast i8* %8 to %1**, !dbg !199
  %10 = load %1*, %1** %9, align 8, !dbg !199
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !199
  %12 = bitcast i8* %11 to %1**, !dbg !199
  %13 = load %1*, %1** %12, align 8, !dbg !199
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !199
  %15 = load i8*, i8** %14, align 8, !dbg !199
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !199
  %17 = load i32, i32* %16, align 4, !dbg !199
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !199
  %19 = load i8*, i8** %18, align 8, !dbg !199
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !199
  %21 = load i8*, i8** %20, align 8, !dbg !199
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !199
  %23 = load i8*, i8** %22, align 8, !dbg !199
  %24 = tail call fastcc i32 @fused_nn_dense_add_nn_relu_1_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !199
  ret i32 %24, !dbg !199
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_nn_relu_1_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 16384, i32 2, i32 32)
  %7 = alloca %16, align 8
  %8 = getelementptr inbounds %16, %16* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %16, %16* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %16, %16* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %16* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.12, i8* nonnull %11, i32 0)
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
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !200
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !200
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !203
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !203
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = fcmp ogt <4 x float> %26, zeroinitializer
  %29 = fcmp ogt <4 x float> %27, zeroinitializer
  %30 = select <4 x i1> %28, <4 x float> %26, <4 x float> zeroinitializer
  %31 = select <4 x i1> %29, <4 x float> %27, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %17, i64 %index
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %30, <4 x float>* %33, align 4, !tbaa !206
  %34 = getelementptr float, float* %32, i64 4
  %35 = bitcast float* %34 to <4 x float>*
  store <4 x float> %31, <4 x float>* %35, align 4, !tbaa !206
  %index.next = add i64 %index, 8
  %36 = icmp eq i64 %index.next, 4096
  br i1 %36, label %for_end, label %vector.body, !llvm.loop !209

for_end:                                          ; preds = %vector.body
  %37 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %38 = call i32 %37(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.12(i32, %0* nocapture readonly, i8* nocapture readonly) #3 {
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
  %27 = mul nsw i64 %indvars.iv6, 25088
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %.05 = phi <16 x float> [ zeroinitializer, %for_body ], [ %36, %for_body2 ]
  %28 = shl nsw i64 %indvars.iv, 4
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <16 x float>*
  %31 = load <16 x float>, <16 x float>* %30, align 64, !tbaa !211
  %32 = add nsw i64 %28, %27
  %33 = getelementptr inbounds float, float* %7, i64 %32
  %34 = bitcast float* %33 to <16 x float>*
  %35 = load <16 x float>, <16 x float>* %34, align 64, !tbaa !214
  %36 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %31, <16 x float> %35, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 1568
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

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
  store float %53, float* %37, align 4, !tbaa !203
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %54 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %54, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #2

define dllexport i32 @fused_nn_max_pool2d_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !217 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !219, metadata !DIExpression()), !dbg !222
  call void @llvm.dbg.value(metadata i8* %1, metadata !220, metadata !DIExpression()), !dbg !222
  call void @llvm.dbg.value(metadata i32 %2, metadata !221, metadata !DIExpression()), !dbg !222
  %3 = bitcast i8* %0 to %1**, !dbg !222
  %4 = load %1*, %1** %3, align 8, !dbg !222
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !222
  %6 = bitcast i8* %5 to %1**, !dbg !222
  %7 = load %1*, %1** %6, align 8, !dbg !222
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !222
  %9 = load i8*, i8** %8, align 8, !dbg !222
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !222
  %11 = load i8*, i8** %10, align 8, !dbg !222
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_4_compute_(i8* %11, i8* %9), !dbg !222
  ret i32 %12, !dbg !222
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_4_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %17, align 8
  %3 = getelementptr inbounds %17, %17* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %17, %17* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %17* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.13, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.13(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
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
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv7 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next8, %for_end3 ]
  %24 = mul nsw i64 %indvars.iv7, 3584
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = mul i32 %25, 14336
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %27 = shl i64 %indvars.iv, 5
  %28 = add nsw i64 %27, %24
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <32 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %31 = shl i32 %indvars.iv.tr, 6
  %32 = add i32 %31, %26
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to <32 x float>*
  %36 = load <32 x float>, <32 x float>* %35, align 64, !tbaa !223
  %37 = fcmp olt <32 x float> %36, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %38 = select <32 x i1> %37, <32 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <32 x float> %36
  %39 = or i32 %32, 32
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to <32 x float>*
  %43 = load <32 x float>, <32 x float>* %42, align 64, !tbaa !223
  %44 = fcmp ogt <32 x float> %38, %43
  %45 = select <32 x i1> %44, <32 x float> %38, <32 x float> %43
  %46 = add i32 %32, 7168
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to <32 x float>*
  %50 = load <32 x float>, <32 x float>* %49, align 64, !tbaa !223
  %51 = fcmp ogt <32 x float> %45, %50
  %52 = select <32 x i1> %51, <32 x float> %45, <32 x float> %50
  %53 = or i32 %46, 32
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %7, i64 %54
  %56 = bitcast float* %55 to <32 x float>*
  %57 = load <32 x float>, <32 x float>* %56, align 64, !tbaa !223
  %58 = fcmp ogt <32 x float> %52, %57
  %59 = select <32 x i1> %58, <32 x float> %52, <32 x float> %57
  store <32 x float> %59, <32 x float>* %30, align 64, !tbaa !226
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 112
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %60 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %60, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_6(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !229 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !231, metadata !DIExpression()), !dbg !234
  call void @llvm.dbg.value(metadata i8* %1, metadata !232, metadata !DIExpression()), !dbg !234
  call void @llvm.dbg.value(metadata i32 %2, metadata !233, metadata !DIExpression()), !dbg !234
  %3 = bitcast i8* %0 to %1**, !dbg !234
  %4 = load %1*, %1** %3, align 8, !dbg !234
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !234
  %6 = bitcast i8* %5 to %1**, !dbg !234
  %7 = load %1*, %1** %6, align 8, !dbg !234
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !234
  %9 = bitcast i8* %8 to %1**, !dbg !234
  %10 = load %1*, %1** %9, align 8, !dbg !234
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !234
  %12 = bitcast i8* %11 to %1**, !dbg !234
  %13 = load %1*, %1** %12, align 8, !dbg !234
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !234
  %15 = load i8*, i8** %14, align 8, !dbg !234
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !234
  %17 = load i32, i32* %16, align 4, !dbg !234
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !234
  %19 = load i8*, i8** %18, align 8, !dbg !234
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !234
  %21 = load i8*, i8** %20, align 8, !dbg !234
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !234
  %23 = load i8*, i8** %22, align 8, !dbg !234
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_6_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !234
  ret i32 %24, !dbg !234
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_6_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 3326976, i32 2, i32 32)
  %7 = alloca %18, align 8
  %8 = getelementptr inbounds %18, %18* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %18, %18* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %18* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.14, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
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
  %19 = getelementptr inbounds %19, %19* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %19* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.15, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
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
  %10 = add nsw i32 %9, 227
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 228
  %15 = select i1 %14, i32 %13, i32 228
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 228
  %18 = select i1 %17, i32 %16, i32 228
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %451, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 3648
  %22 = srem i32 %20, 114
  %.off = add nsw i32 %22, -1
  %23 = icmp ult i32 %.off, 112
  %24 = sdiv i32 %20, 114
  %25 = mul nsw i32 %24, 401408
  br i1 %23, label %for_body.split.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body.split.us:                                ; preds = %for_body
  %26 = mul nsw i32 %22, 3584
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_end6.us, %for_body.split.us
  %indvars.iv21 = phi i64 [ %indvars.iv.next22, %for_end6.us ], [ 0, %for_body.split.us ]
  %27 = shl nsw i64 %indvars.iv21, 5
  %28 = trunc i64 %indvars.iv21 to i32
  %29 = add i32 %28, -1
  %30 = icmp ult i32 %29, 112
  %31 = trunc i64 %27 to i32
  %32 = add i32 %21, %31
  br i1 %30, label %vector.body, label %for_body2.for_body2.split_crit_edge.us

for_end6.us:                                      ; preds = %vector.body, %for_body2.for_body2.split_crit_edge.us
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond24 = icmp eq i64 %indvars.iv.next22, 114
  br i1 %exitcond24, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %for_body2.us
  %33 = trunc i64 %27 to i32
  %34 = add i32 %33, -3616
  %35 = add i32 %26, %34
  %36 = add i32 %35, %25
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds float, float* %7, i64 %37
  %39 = bitcast float* %38 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %39, align 4, !tbaa !235
  %40 = sext i32 %32 to i64
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %42, align 4, !tbaa !238
  %43 = or i64 %27, 4
  %44 = trunc i64 %43 to i32
  %45 = add i32 %21, %44
  %46 = trunc i64 %43 to i32
  %47 = add i32 %46, -3616
  %48 = add i32 %26, %47
  %49 = add i32 %48, %25
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to <4 x i32>*
  %wide.load.1 = load <4 x i32>, <4 x i32>* %52, align 4, !tbaa !235
  %53 = sext i32 %45 to i64
  %54 = getelementptr inbounds float, float* %4, i64 %53
  %55 = bitcast float* %54 to <4 x i32>*
  store <4 x i32> %wide.load.1, <4 x i32>* %55, align 4, !tbaa !238
  %56 = or i64 %27, 8
  %57 = trunc i64 %56 to i32
  %58 = add i32 %21, %57
  %59 = trunc i64 %56 to i32
  %60 = add i32 %59, -3616
  %61 = add i32 %26, %60
  %62 = add i32 %61, %25
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %7, i64 %63
  %65 = bitcast float* %64 to <4 x i32>*
  %wide.load.2 = load <4 x i32>, <4 x i32>* %65, align 4, !tbaa !235
  %66 = sext i32 %58 to i64
  %67 = getelementptr inbounds float, float* %4, i64 %66
  %68 = bitcast float* %67 to <4 x i32>*
  store <4 x i32> %wide.load.2, <4 x i32>* %68, align 4, !tbaa !238
  %69 = or i64 %27, 12
  %70 = trunc i64 %69 to i32
  %71 = add i32 %21, %70
  %72 = trunc i64 %69 to i32
  %73 = add i32 %72, -3616
  %74 = add i32 %26, %73
  %75 = add i32 %74, %25
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %7, i64 %76
  %78 = bitcast float* %77 to <4 x i32>*
  %wide.load.3 = load <4 x i32>, <4 x i32>* %78, align 4, !tbaa !235
  %79 = sext i32 %71 to i64
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = bitcast float* %80 to <4 x i32>*
  store <4 x i32> %wide.load.3, <4 x i32>* %81, align 4, !tbaa !238
  %82 = or i64 %27, 16
  %83 = trunc i64 %82 to i32
  %84 = add i32 %21, %83
  %85 = trunc i64 %82 to i32
  %86 = add i32 %85, -3616
  %87 = add i32 %26, %86
  %88 = add i32 %87, %25
  %89 = sext i32 %88 to i64
  %90 = getelementptr inbounds float, float* %7, i64 %89
  %91 = bitcast float* %90 to <4 x i32>*
  %wide.load.4 = load <4 x i32>, <4 x i32>* %91, align 4, !tbaa !235
  %92 = sext i32 %84 to i64
  %93 = getelementptr inbounds float, float* %4, i64 %92
  %94 = bitcast float* %93 to <4 x i32>*
  store <4 x i32> %wide.load.4, <4 x i32>* %94, align 4, !tbaa !238
  %95 = or i64 %27, 20
  %96 = trunc i64 %95 to i32
  %97 = add i32 %21, %96
  %98 = trunc i64 %95 to i32
  %99 = add i32 %98, -3616
  %100 = add i32 %26, %99
  %101 = add i32 %100, %25
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds float, float* %7, i64 %102
  %104 = bitcast float* %103 to <4 x i32>*
  %wide.load.5 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !235
  %105 = sext i32 %97 to i64
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = bitcast float* %106 to <4 x i32>*
  store <4 x i32> %wide.load.5, <4 x i32>* %107, align 4, !tbaa !238
  %108 = or i64 %27, 24
  %109 = trunc i64 %108 to i32
  %110 = add i32 %21, %109
  %111 = trunc i64 %108 to i32
  %112 = add i32 %111, -3616
  %113 = add i32 %26, %112
  %114 = add i32 %113, %25
  %115 = sext i32 %114 to i64
  %116 = getelementptr inbounds float, float* %7, i64 %115
  %117 = bitcast float* %116 to <4 x i32>*
  %wide.load.6 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !235
  %118 = sext i32 %110 to i64
  %119 = getelementptr inbounds float, float* %4, i64 %118
  %120 = bitcast float* %119 to <4 x i32>*
  store <4 x i32> %wide.load.6, <4 x i32>* %120, align 4, !tbaa !238
  %121 = or i64 %27, 28
  %122 = trunc i64 %121 to i32
  %123 = add i32 %21, %122
  %124 = trunc i64 %121 to i32
  %125 = add i32 %124, -3616
  %126 = add i32 %26, %125
  %127 = add i32 %126, %25
  %128 = sext i32 %127 to i64
  %129 = getelementptr inbounds float, float* %7, i64 %128
  %130 = bitcast float* %129 to <4 x i32>*
  %wide.load.7 = load <4 x i32>, <4 x i32>* %130, align 4, !tbaa !235
  %131 = sext i32 %123 to i64
  %132 = getelementptr inbounds float, float* %4, i64 %131
  %133 = bitcast float* %132 to <4 x i32>*
  store <4 x i32> %wide.load.7, <4 x i32>* %133, align 4, !tbaa !238
  br label %for_end6.us

for_body2.for_body2.split_crit_edge.us:           ; preds = %for_body2.us
  %134 = sext i32 %32 to i64
  %135 = getelementptr inbounds float, float* %4, i64 %134
  store float 0.000000e+00, float* %135, align 4, !tbaa !238
  %136 = trunc i64 %27 to i32
  %137 = or i32 %136, 1
  %138 = add i32 %137, %21
  %139 = sext i32 %138 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  store float 0.000000e+00, float* %140, align 4, !tbaa !238
  %141 = trunc i64 %27 to i32
  %142 = or i32 %141, 2
  %143 = add i32 %142, %21
  %144 = sext i32 %143 to i64
  %145 = getelementptr inbounds float, float* %4, i64 %144
  store float 0.000000e+00, float* %145, align 4, !tbaa !238
  %146 = trunc i64 %27 to i32
  %147 = or i32 %146, 3
  %148 = add i32 %147, %21
  %149 = sext i32 %148 to i64
  %150 = getelementptr inbounds float, float* %4, i64 %149
  store float 0.000000e+00, float* %150, align 4, !tbaa !238
  %151 = trunc i64 %27 to i32
  %152 = or i32 %151, 4
  %153 = add i32 %152, %21
  %154 = sext i32 %153 to i64
  %155 = getelementptr inbounds float, float* %4, i64 %154
  store float 0.000000e+00, float* %155, align 4, !tbaa !238
  %156 = trunc i64 %27 to i32
  %157 = or i32 %156, 5
  %158 = add i32 %157, %21
  %159 = sext i32 %158 to i64
  %160 = getelementptr inbounds float, float* %4, i64 %159
  store float 0.000000e+00, float* %160, align 4, !tbaa !238
  %161 = trunc i64 %27 to i32
  %162 = or i32 %161, 6
  %163 = add i32 %162, %21
  %164 = sext i32 %163 to i64
  %165 = getelementptr inbounds float, float* %4, i64 %164
  store float 0.000000e+00, float* %165, align 4, !tbaa !238
  %166 = trunc i64 %27 to i32
  %167 = or i32 %166, 7
  %168 = add i32 %167, %21
  %169 = sext i32 %168 to i64
  %170 = getelementptr inbounds float, float* %4, i64 %169
  store float 0.000000e+00, float* %170, align 4, !tbaa !238
  %171 = trunc i64 %27 to i32
  %172 = or i32 %171, 8
  %173 = add i32 %172, %21
  %174 = sext i32 %173 to i64
  %175 = getelementptr inbounds float, float* %4, i64 %174
  store float 0.000000e+00, float* %175, align 4, !tbaa !238
  %176 = trunc i64 %27 to i32
  %177 = or i32 %176, 9
  %178 = add i32 %177, %21
  %179 = sext i32 %178 to i64
  %180 = getelementptr inbounds float, float* %4, i64 %179
  store float 0.000000e+00, float* %180, align 4, !tbaa !238
  %181 = trunc i64 %27 to i32
  %182 = or i32 %181, 10
  %183 = add i32 %182, %21
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds float, float* %4, i64 %184
  store float 0.000000e+00, float* %185, align 4, !tbaa !238
  %186 = trunc i64 %27 to i32
  %187 = or i32 %186, 11
  %188 = add i32 %187, %21
  %189 = sext i32 %188 to i64
  %190 = getelementptr inbounds float, float* %4, i64 %189
  store float 0.000000e+00, float* %190, align 4, !tbaa !238
  %191 = trunc i64 %27 to i32
  %192 = or i32 %191, 12
  %193 = add i32 %192, %21
  %194 = sext i32 %193 to i64
  %195 = getelementptr inbounds float, float* %4, i64 %194
  store float 0.000000e+00, float* %195, align 4, !tbaa !238
  %196 = trunc i64 %27 to i32
  %197 = or i32 %196, 13
  %198 = add i32 %197, %21
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %4, i64 %199
  store float 0.000000e+00, float* %200, align 4, !tbaa !238
  %201 = trunc i64 %27 to i32
  %202 = or i32 %201, 14
  %203 = add i32 %202, %21
  %204 = sext i32 %203 to i64
  %205 = getelementptr inbounds float, float* %4, i64 %204
  store float 0.000000e+00, float* %205, align 4, !tbaa !238
  %206 = trunc i64 %27 to i32
  %207 = or i32 %206, 15
  %208 = add i32 %207, %21
  %209 = sext i32 %208 to i64
  %210 = getelementptr inbounds float, float* %4, i64 %209
  store float 0.000000e+00, float* %210, align 4, !tbaa !238
  %211 = trunc i64 %27 to i32
  %212 = or i32 %211, 16
  %213 = add i32 %212, %21
  %214 = sext i32 %213 to i64
  %215 = getelementptr inbounds float, float* %4, i64 %214
  store float 0.000000e+00, float* %215, align 4, !tbaa !238
  %216 = trunc i64 %27 to i32
  %217 = or i32 %216, 17
  %218 = add i32 %217, %21
  %219 = sext i32 %218 to i64
  %220 = getelementptr inbounds float, float* %4, i64 %219
  store float 0.000000e+00, float* %220, align 4, !tbaa !238
  %221 = trunc i64 %27 to i32
  %222 = or i32 %221, 18
  %223 = add i32 %222, %21
  %224 = sext i32 %223 to i64
  %225 = getelementptr inbounds float, float* %4, i64 %224
  store float 0.000000e+00, float* %225, align 4, !tbaa !238
  %226 = trunc i64 %27 to i32
  %227 = or i32 %226, 19
  %228 = add i32 %227, %21
  %229 = sext i32 %228 to i64
  %230 = getelementptr inbounds float, float* %4, i64 %229
  store float 0.000000e+00, float* %230, align 4, !tbaa !238
  %231 = trunc i64 %27 to i32
  %232 = or i32 %231, 20
  %233 = add i32 %232, %21
  %234 = sext i32 %233 to i64
  %235 = getelementptr inbounds float, float* %4, i64 %234
  store float 0.000000e+00, float* %235, align 4, !tbaa !238
  %236 = trunc i64 %27 to i32
  %237 = or i32 %236, 21
  %238 = add i32 %237, %21
  %239 = sext i32 %238 to i64
  %240 = getelementptr inbounds float, float* %4, i64 %239
  store float 0.000000e+00, float* %240, align 4, !tbaa !238
  %241 = trunc i64 %27 to i32
  %242 = or i32 %241, 22
  %243 = add i32 %242, %21
  %244 = sext i32 %243 to i64
  %245 = getelementptr inbounds float, float* %4, i64 %244
  store float 0.000000e+00, float* %245, align 4, !tbaa !238
  %246 = trunc i64 %27 to i32
  %247 = or i32 %246, 23
  %248 = add i32 %247, %21
  %249 = sext i32 %248 to i64
  %250 = getelementptr inbounds float, float* %4, i64 %249
  store float 0.000000e+00, float* %250, align 4, !tbaa !238
  %251 = trunc i64 %27 to i32
  %252 = or i32 %251, 24
  %253 = add i32 %252, %21
  %254 = sext i32 %253 to i64
  %255 = getelementptr inbounds float, float* %4, i64 %254
  store float 0.000000e+00, float* %255, align 4, !tbaa !238
  %256 = trunc i64 %27 to i32
  %257 = or i32 %256, 25
  %258 = add i32 %257, %21
  %259 = sext i32 %258 to i64
  %260 = getelementptr inbounds float, float* %4, i64 %259
  store float 0.000000e+00, float* %260, align 4, !tbaa !238
  %261 = trunc i64 %27 to i32
  %262 = or i32 %261, 26
  %263 = add i32 %262, %21
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds float, float* %4, i64 %264
  store float 0.000000e+00, float* %265, align 4, !tbaa !238
  %266 = trunc i64 %27 to i32
  %267 = or i32 %266, 27
  %268 = add i32 %267, %21
  %269 = sext i32 %268 to i64
  %270 = getelementptr inbounds float, float* %4, i64 %269
  store float 0.000000e+00, float* %270, align 4, !tbaa !238
  %271 = trunc i64 %27 to i32
  %272 = or i32 %271, 28
  %273 = add i32 %272, %21
  %274 = sext i32 %273 to i64
  %275 = getelementptr inbounds float, float* %4, i64 %274
  store float 0.000000e+00, float* %275, align 4, !tbaa !238
  %276 = trunc i64 %27 to i32
  %277 = or i32 %276, 29
  %278 = add i32 %277, %21
  %279 = sext i32 %278 to i64
  %280 = getelementptr inbounds float, float* %4, i64 %279
  store float 0.000000e+00, float* %280, align 4, !tbaa !238
  %281 = trunc i64 %27 to i32
  %282 = or i32 %281, 30
  %283 = add i32 %282, %21
  %284 = sext i32 %283 to i64
  %285 = getelementptr inbounds float, float* %4, i64 %284
  store float 0.000000e+00, float* %285, align 4, !tbaa !238
  %286 = trunc i64 %27 to i32
  %287 = or i32 %286, 31
  %288 = add i32 %287, %21
  %289 = sext i32 %288 to i64
  %290 = getelementptr inbounds float, float* %4, i64 %289
  store float 0.000000e+00, float* %290, align 4, !tbaa !238
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_body2 ], [ 0, %for_body2.preheader ]
  %291 = shl nsw i64 %indvars.iv, 5
  %292 = trunc i64 %291 to i32
  %293 = add i32 %21, %292
  %294 = sext i32 %293 to i64
  %295 = getelementptr inbounds float, float* %4, i64 %294
  store float 0.000000e+00, float* %295, align 4, !tbaa !238
  %296 = trunc i64 %291 to i32
  %297 = or i32 %296, 1
  %298 = add i32 %297, %21
  %299 = sext i32 %298 to i64
  %300 = getelementptr inbounds float, float* %4, i64 %299
  store float 0.000000e+00, float* %300, align 4, !tbaa !238
  %301 = trunc i64 %291 to i32
  %302 = or i32 %301, 2
  %303 = add i32 %302, %21
  %304 = sext i32 %303 to i64
  %305 = getelementptr inbounds float, float* %4, i64 %304
  store float 0.000000e+00, float* %305, align 4, !tbaa !238
  %306 = trunc i64 %291 to i32
  %307 = or i32 %306, 3
  %308 = add i32 %307, %21
  %309 = sext i32 %308 to i64
  %310 = getelementptr inbounds float, float* %4, i64 %309
  store float 0.000000e+00, float* %310, align 4, !tbaa !238
  %311 = trunc i64 %291 to i32
  %312 = or i32 %311, 4
  %313 = add i32 %312, %21
  %314 = sext i32 %313 to i64
  %315 = getelementptr inbounds float, float* %4, i64 %314
  store float 0.000000e+00, float* %315, align 4, !tbaa !238
  %316 = trunc i64 %291 to i32
  %317 = or i32 %316, 5
  %318 = add i32 %317, %21
  %319 = sext i32 %318 to i64
  %320 = getelementptr inbounds float, float* %4, i64 %319
  store float 0.000000e+00, float* %320, align 4, !tbaa !238
  %321 = trunc i64 %291 to i32
  %322 = or i32 %321, 6
  %323 = add i32 %322, %21
  %324 = sext i32 %323 to i64
  %325 = getelementptr inbounds float, float* %4, i64 %324
  store float 0.000000e+00, float* %325, align 4, !tbaa !238
  %326 = trunc i64 %291 to i32
  %327 = or i32 %326, 7
  %328 = add i32 %327, %21
  %329 = sext i32 %328 to i64
  %330 = getelementptr inbounds float, float* %4, i64 %329
  store float 0.000000e+00, float* %330, align 4, !tbaa !238
  %331 = trunc i64 %291 to i32
  %332 = or i32 %331, 8
  %333 = add i32 %332, %21
  %334 = sext i32 %333 to i64
  %335 = getelementptr inbounds float, float* %4, i64 %334
  store float 0.000000e+00, float* %335, align 4, !tbaa !238
  %336 = trunc i64 %291 to i32
  %337 = or i32 %336, 9
  %338 = add i32 %337, %21
  %339 = sext i32 %338 to i64
  %340 = getelementptr inbounds float, float* %4, i64 %339
  store float 0.000000e+00, float* %340, align 4, !tbaa !238
  %341 = trunc i64 %291 to i32
  %342 = or i32 %341, 10
  %343 = add i32 %342, %21
  %344 = sext i32 %343 to i64
  %345 = getelementptr inbounds float, float* %4, i64 %344
  store float 0.000000e+00, float* %345, align 4, !tbaa !238
  %346 = trunc i64 %291 to i32
  %347 = or i32 %346, 11
  %348 = add i32 %347, %21
  %349 = sext i32 %348 to i64
  %350 = getelementptr inbounds float, float* %4, i64 %349
  store float 0.000000e+00, float* %350, align 4, !tbaa !238
  %351 = trunc i64 %291 to i32
  %352 = or i32 %351, 12
  %353 = add i32 %352, %21
  %354 = sext i32 %353 to i64
  %355 = getelementptr inbounds float, float* %4, i64 %354
  store float 0.000000e+00, float* %355, align 4, !tbaa !238
  %356 = trunc i64 %291 to i32
  %357 = or i32 %356, 13
  %358 = add i32 %357, %21
  %359 = sext i32 %358 to i64
  %360 = getelementptr inbounds float, float* %4, i64 %359
  store float 0.000000e+00, float* %360, align 4, !tbaa !238
  %361 = trunc i64 %291 to i32
  %362 = or i32 %361, 14
  %363 = add i32 %362, %21
  %364 = sext i32 %363 to i64
  %365 = getelementptr inbounds float, float* %4, i64 %364
  store float 0.000000e+00, float* %365, align 4, !tbaa !238
  %366 = trunc i64 %291 to i32
  %367 = or i32 %366, 15
  %368 = add i32 %367, %21
  %369 = sext i32 %368 to i64
  %370 = getelementptr inbounds float, float* %4, i64 %369
  store float 0.000000e+00, float* %370, align 4, !tbaa !238
  %371 = trunc i64 %291 to i32
  %372 = or i32 %371, 16
  %373 = add i32 %372, %21
  %374 = sext i32 %373 to i64
  %375 = getelementptr inbounds float, float* %4, i64 %374
  store float 0.000000e+00, float* %375, align 4, !tbaa !238
  %376 = trunc i64 %291 to i32
  %377 = or i32 %376, 17
  %378 = add i32 %377, %21
  %379 = sext i32 %378 to i64
  %380 = getelementptr inbounds float, float* %4, i64 %379
  store float 0.000000e+00, float* %380, align 4, !tbaa !238
  %381 = trunc i64 %291 to i32
  %382 = or i32 %381, 18
  %383 = add i32 %382, %21
  %384 = sext i32 %383 to i64
  %385 = getelementptr inbounds float, float* %4, i64 %384
  store float 0.000000e+00, float* %385, align 4, !tbaa !238
  %386 = trunc i64 %291 to i32
  %387 = or i32 %386, 19
  %388 = add i32 %387, %21
  %389 = sext i32 %388 to i64
  %390 = getelementptr inbounds float, float* %4, i64 %389
  store float 0.000000e+00, float* %390, align 4, !tbaa !238
  %391 = trunc i64 %291 to i32
  %392 = or i32 %391, 20
  %393 = add i32 %392, %21
  %394 = sext i32 %393 to i64
  %395 = getelementptr inbounds float, float* %4, i64 %394
  store float 0.000000e+00, float* %395, align 4, !tbaa !238
  %396 = trunc i64 %291 to i32
  %397 = or i32 %396, 21
  %398 = add i32 %397, %21
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds float, float* %4, i64 %399
  store float 0.000000e+00, float* %400, align 4, !tbaa !238
  %401 = trunc i64 %291 to i32
  %402 = or i32 %401, 22
  %403 = add i32 %402, %21
  %404 = sext i32 %403 to i64
  %405 = getelementptr inbounds float, float* %4, i64 %404
  store float 0.000000e+00, float* %405, align 4, !tbaa !238
  %406 = trunc i64 %291 to i32
  %407 = or i32 %406, 23
  %408 = add i32 %407, %21
  %409 = sext i32 %408 to i64
  %410 = getelementptr inbounds float, float* %4, i64 %409
  store float 0.000000e+00, float* %410, align 4, !tbaa !238
  %411 = trunc i64 %291 to i32
  %412 = or i32 %411, 24
  %413 = add i32 %412, %21
  %414 = sext i32 %413 to i64
  %415 = getelementptr inbounds float, float* %4, i64 %414
  store float 0.000000e+00, float* %415, align 4, !tbaa !238
  %416 = trunc i64 %291 to i32
  %417 = or i32 %416, 25
  %418 = add i32 %417, %21
  %419 = sext i32 %418 to i64
  %420 = getelementptr inbounds float, float* %4, i64 %419
  store float 0.000000e+00, float* %420, align 4, !tbaa !238
  %421 = trunc i64 %291 to i32
  %422 = or i32 %421, 26
  %423 = add i32 %422, %21
  %424 = sext i32 %423 to i64
  %425 = getelementptr inbounds float, float* %4, i64 %424
  store float 0.000000e+00, float* %425, align 4, !tbaa !238
  %426 = trunc i64 %291 to i32
  %427 = or i32 %426, 27
  %428 = add i32 %427, %21
  %429 = sext i32 %428 to i64
  %430 = getelementptr inbounds float, float* %4, i64 %429
  store float 0.000000e+00, float* %430, align 4, !tbaa !238
  %431 = trunc i64 %291 to i32
  %432 = or i32 %431, 28
  %433 = add i32 %432, %21
  %434 = sext i32 %433 to i64
  %435 = getelementptr inbounds float, float* %4, i64 %434
  store float 0.000000e+00, float* %435, align 4, !tbaa !238
  %436 = trunc i64 %291 to i32
  %437 = or i32 %436, 29
  %438 = add i32 %437, %21
  %439 = sext i32 %438 to i64
  %440 = getelementptr inbounds float, float* %4, i64 %439
  store float 0.000000e+00, float* %440, align 4, !tbaa !238
  %441 = trunc i64 %291 to i32
  %442 = or i32 %441, 30
  %443 = add i32 %442, %21
  %444 = sext i32 %443 to i64
  %445 = getelementptr inbounds float, float* %4, i64 %444
  store float 0.000000e+00, float* %445, align 4, !tbaa !238
  %446 = trunc i64 %291 to i32
  %447 = or i32 %446, 31
  %448 = add i32 %447, %21
  %449 = sext i32 %448 to i64
  %450 = getelementptr inbounds float, float* %4, i64 %449
  store float 0.000000e+00, float* %450, align 4, !tbaa !238
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 114
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %451 = add nsw i32 %20, 1
  %452 = icmp slt i32 %451, %15
  br i1 %452, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.15(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 223
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 224
  %24 = select i1 %23, i32 %22, i32 224
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 224
  %27 = select i1 %26, i32 %25, i32 224
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end20
  %29 = phi i32 [ %188, %for_end20 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 28672, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <64 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 256
  %36 = bitcast i8* %35 to <64 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 512
  %38 = bitcast i8* %37 to <64 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 768
  %40 = bitcast i8* %39 to <64 x float>*
  %41 = srem i32 %29, 112
  %42 = sdiv i32 %29, 112
  %43 = mul nsw i32 %42, 36864
  %44 = bitcast i8* %31 to float*
  %45 = sext i32 %43 to i64
  %reass.mul = mul nsw i32 %41, 3648
  %46 = sext i32 %reass.mul to i64
  %47 = mul nsw i32 %41, 3648
  %reass.mul.1 = add nsw i32 %47, 3648
  %48 = sext i32 %reass.mul.1 to i64
  %49 = mul nsw i32 %41, 3648
  %reass.mul.2 = add nsw i32 %49, 7296
  %50 = sext i32 %reass.mul.2 to i64
  br label %for_body4

for_end:                                          ; preds = %for_end20, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end8, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end8 ]
  %51 = shl i64 %indvar, 7
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  br label %for_body7

for_end5:                                         ; preds = %for_end8
  %52 = mul nsw i32 %29, 7168
  %53 = shl nsw i32 %42, 6
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %13, i64 %54
  %56 = bitcast float* %55 to <64 x float>*
  %57 = load <64 x float>, <64 x float>* %56, align 64, !tbaa !241
  br label %for_body19

for_body7:                                        ; preds = %for_end14.2, %for_body4
  %indvars.iv55 = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next56, %for_end14.2 ]
  %.lcssa33.lcssa47 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %309, %for_end14.2 ]
  %.lcssa31.lcssa45 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %303, %for_end14.2 ]
  %.lcssa29.lcssa44 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %302, %for_end14.2 ]
  %.lcssa.lcssa42 = phi <64 x float> [ zeroinitializer, %for_body4 ], [ %301, %for_end14.2 ]
  %58 = mul nuw nsw i64 %indvars.iv55, 415872
  %59 = add nuw nsw i64 %58, %51
  %60 = mul nuw nsw i64 %indvars.iv55, 18432
  %61 = add nsw i64 %60, %45
  %62 = add nsw i64 %59, %46
  br label %for_body13

for_end8:                                         ; preds = %for_end14.2
  store <64 x float> %301, <64 x float>* %34, align 64, !tbaa !244
  store <64 x float> %302, <64 x float>* %36, align 64, !tbaa !244
  store <64 x float> %303, <64 x float>* %38, align 64, !tbaa !244
  store <64 x float> %309, <64 x float>* %40, align 64, !tbaa !244
  %63 = shl i64 %indvar, 8
  %64 = getelementptr inbounds float, float* %44, i64 %63
  %65 = bitcast float* %64 to <64 x float>*
  store <64 x float> %301, <64 x float>* %65, align 64, !tbaa !252
  %66 = or i64 %63, 64
  %67 = getelementptr inbounds float, float* %44, i64 %66
  %68 = bitcast float* %67 to <64 x float>*
  store <64 x float> %302, <64 x float>* %68, align 64, !tbaa !252
  %69 = or i64 %63, 128
  %70 = getelementptr inbounds float, float* %44, i64 %69
  %71 = bitcast float* %70 to <64 x float>*
  store <64 x float> %303, <64 x float>* %71, align 64, !tbaa !252
  %72 = or i64 %63, 192
  %73 = getelementptr inbounds float, float* %44, i64 %72
  %74 = bitcast float* %73 to <64 x float>*
  store <64 x float> %309, <64 x float>* %74, align 64, !tbaa !252
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond61 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond61, label %for_end5, label %for_body4, !prof !20

for_body13:                                       ; preds = %for_body13, %for_body7
  %indvars.iv = phi i64 [ 0, %for_body7 ], [ %indvars.iv.next, %for_body13 ]
  %75 = phi <64 x float> [ %.lcssa33.lcssa47, %for_body7 ], [ %133, %for_body13 ]
  %76 = phi <64 x float> [ %.lcssa31.lcssa45, %for_body7 ], [ %127, %for_body13 ]
  %77 = phi <64 x float> [ %.lcssa29.lcssa44, %for_body7 ], [ %126, %for_body13 ]
  %78 = phi <64 x float> [ %.lcssa.lcssa42, %for_body7 ], [ %125, %for_body13 ]
  %79 = add nsw i64 %62, %indvars.iv
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = load float, float* %80, align 4, !tbaa !238
  %82 = insertelement <64 x float> undef, float %81, i32 0
  %83 = shufflevector <64 x float> %82, <64 x float> undef, <64 x i32> zeroinitializer
  %84 = shl nsw i64 %indvars.iv, 6
  %85 = add nsw i64 %61, %84
  %86 = getelementptr inbounds float, float* %7, i64 %85
  %87 = bitcast float* %86 to <64 x float>*
  %88 = load <64 x float>, <64 x float>* %87, align 64, !tbaa !255
  %89 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %83, <64 x float> %88, <64 x float> %78)
  %90 = add nsw i64 %79, 32
  %91 = getelementptr inbounds float, float* %4, i64 %90
  %92 = load float, float* %91, align 4, !tbaa !238
  %93 = insertelement <64 x float> undef, float %92, i32 0
  %94 = shufflevector <64 x float> %93, <64 x float> undef, <64 x i32> zeroinitializer
  %95 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %94, <64 x float> %88, <64 x float> %77)
  %96 = add nsw i64 %79, 64
  %97 = getelementptr inbounds float, float* %4, i64 %96
  %98 = load float, float* %97, align 4, !tbaa !238
  %99 = insertelement <64 x float> undef, float %98, i32 0
  %100 = shufflevector <64 x float> %99, <64 x float> undef, <64 x i32> zeroinitializer
  %101 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %100, <64 x float> %88, <64 x float> %76)
  %102 = add nsw i64 %79, 96
  %103 = getelementptr inbounds float, float* %4, i64 %102
  %104 = load float, float* %103, align 4, !tbaa !238
  %105 = insertelement <64 x float> undef, float %104, i32 0
  %106 = shufflevector <64 x float> %105, <64 x float> undef, <64 x i32> zeroinitializer
  %107 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %106, <64 x float> %88, <64 x float> %75)
  %108 = add nsw i64 %85, 2048
  %109 = getelementptr inbounds float, float* %7, i64 %108
  %110 = bitcast float* %109 to <64 x float>*
  %111 = load <64 x float>, <64 x float>* %110, align 64, !tbaa !255
  %112 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %94, <64 x float> %111, <64 x float> %89)
  %113 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %100, <64 x float> %111, <64 x float> %95)
  %114 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %106, <64 x float> %111, <64 x float> %101)
  %115 = add nsw i64 %79, 128
  %116 = getelementptr inbounds float, float* %4, i64 %115
  %117 = load float, float* %116, align 4, !tbaa !238
  %118 = insertelement <64 x float> undef, float %117, i32 0
  %119 = shufflevector <64 x float> %118, <64 x float> undef, <64 x i32> zeroinitializer
  %120 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %119, <64 x float> %111, <64 x float> %107)
  %121 = add nsw i64 %85, 4096
  %122 = getelementptr inbounds float, float* %7, i64 %121
  %123 = bitcast float* %122 to <64 x float>*
  %124 = load <64 x float>, <64 x float>* %123, align 64, !tbaa !255
  %125 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %100, <64 x float> %124, <64 x float> %112)
  %126 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %106, <64 x float> %124, <64 x float> %113)
  %127 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %119, <64 x float> %124, <64 x float> %114)
  %128 = add nsw i64 %79, 160
  %129 = getelementptr inbounds float, float* %4, i64 %128
  %130 = load float, float* %129, align 4, !tbaa !238
  %131 = insertelement <64 x float> undef, float %130, i32 0
  %132 = shufflevector <64 x float> %131, <64 x float> undef, <64 x i32> zeroinitializer
  %133 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %132, <64 x float> %124, <64 x float> %120)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end14, label %for_body13, !prof !20

for_end14:                                        ; preds = %for_body13
  %134 = add nsw i64 %59, %48
  %135 = add nsw i64 %61, 6144
  br label %for_body13.1

for_body19:                                       ; preds = %for_body19, %for_end5
  %indvars.iv65 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next66, %for_body19 ]
  %136 = shl nsw i64 %indvars.iv65, 8
  %137 = trunc i64 %136 to i32
  %138 = add i32 %52, %137
  %139 = getelementptr inbounds float, float* %44, i64 %136
  %140 = bitcast float* %139 to <64 x float>*
  %141 = load <64 x float>, <64 x float>* %140, align 64, !tbaa !252
  %142 = fadd <64 x float> %57, %141
  %143 = fcmp ogt <64 x float> %142, zeroinitializer
  %144 = select <64 x i1> %143, <64 x float> %142, <64 x float> zeroinitializer
  %145 = sext i32 %138 to i64
  %146 = getelementptr inbounds float, float* %10, i64 %145
  %147 = bitcast float* %146 to <64 x float>*
  store <64 x float> %144, <64 x float>* %147, align 64, !tbaa !258
  %148 = or i64 %136, 64
  %149 = trunc i64 %148 to i32
  %150 = add i32 %52, %149
  %151 = getelementptr inbounds float, float* %44, i64 %148
  %152 = bitcast float* %151 to <64 x float>*
  %153 = load <64 x float>, <64 x float>* %152, align 64, !tbaa !252
  %154 = fadd <64 x float> %57, %153
  %155 = fcmp ogt <64 x float> %154, zeroinitializer
  %156 = select <64 x i1> %155, <64 x float> %154, <64 x float> zeroinitializer
  %157 = sext i32 %150 to i64
  %158 = getelementptr inbounds float, float* %10, i64 %157
  %159 = bitcast float* %158 to <64 x float>*
  store <64 x float> %156, <64 x float>* %159, align 64, !tbaa !258
  %160 = or i64 %136, 128
  %161 = trunc i64 %160 to i32
  %162 = add i32 %52, %161
  %163 = getelementptr inbounds float, float* %44, i64 %160
  %164 = bitcast float* %163 to <64 x float>*
  %165 = load <64 x float>, <64 x float>* %164, align 64, !tbaa !252
  %166 = fadd <64 x float> %57, %165
  %167 = fcmp ogt <64 x float> %166, zeroinitializer
  %168 = select <64 x i1> %167, <64 x float> %166, <64 x float> zeroinitializer
  %169 = sext i32 %162 to i64
  %170 = getelementptr inbounds float, float* %10, i64 %169
  %171 = bitcast float* %170 to <64 x float>*
  store <64 x float> %168, <64 x float>* %171, align 64, !tbaa !258
  %172 = or i64 %136, 192
  %173 = trunc i64 %172 to i32
  %174 = add i32 %52, %173
  %175 = getelementptr inbounds float, float* %44, i64 %172
  %176 = bitcast float* %175 to <64 x float>*
  %177 = load <64 x float>, <64 x float>* %176, align 64, !tbaa !252
  %178 = fadd <64 x float> %57, %177
  %179 = fcmp ogt <64 x float> %178, zeroinitializer
  %180 = select <64 x i1> %179, <64 x float> %178, <64 x float> zeroinitializer
  %181 = sext i32 %174 to i64
  %182 = getelementptr inbounds float, float* %10, i64 %181
  %183 = bitcast float* %182 to <64 x float>*
  store <64 x float> %180, <64 x float>* %183, align 64, !tbaa !258
  %indvars.iv.next66 = add nuw nsw i64 %indvars.iv65, 1
  %exitcond67 = icmp eq i64 %indvars.iv.next66, 28
  br i1 %exitcond67, label %for_end20, label %for_body19, !prof !20

for_end20:                                        ; preds = %for_body19
  %184 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %185 = tail call i32 %184(i32 1, i32 %16, i8* %33)
  %186 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %187 = tail call i32 %186(i32 1, i32 %16, i8* nonnull %31)
  %188 = add nsw i32 %29, 1
  %189 = icmp slt i32 %188, %24
  br i1 %189, label %for_body, label %for_end, !prof !19

for_body13.1:                                     ; preds = %for_body13.1, %for_end14
  %indvars.iv.1 = phi i64 [ 0, %for_end14 ], [ %indvars.iv.next.1, %for_body13.1 ]
  %190 = phi <64 x float> [ %133, %for_end14 ], [ %248, %for_body13.1 ]
  %191 = phi <64 x float> [ %127, %for_end14 ], [ %242, %for_body13.1 ]
  %192 = phi <64 x float> [ %126, %for_end14 ], [ %241, %for_body13.1 ]
  %193 = phi <64 x float> [ %125, %for_end14 ], [ %240, %for_body13.1 ]
  %194 = add nsw i64 %134, %indvars.iv.1
  %195 = getelementptr inbounds float, float* %4, i64 %194
  %196 = load float, float* %195, align 4, !tbaa !238
  %197 = insertelement <64 x float> undef, float %196, i32 0
  %198 = shufflevector <64 x float> %197, <64 x float> undef, <64 x i32> zeroinitializer
  %199 = shl nsw i64 %indvars.iv.1, 6
  %200 = add nsw i64 %135, %199
  %201 = getelementptr inbounds float, float* %7, i64 %200
  %202 = bitcast float* %201 to <64 x float>*
  %203 = load <64 x float>, <64 x float>* %202, align 64, !tbaa !255
  %204 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %198, <64 x float> %203, <64 x float> %193)
  %205 = add nsw i64 %194, 32
  %206 = getelementptr inbounds float, float* %4, i64 %205
  %207 = load float, float* %206, align 4, !tbaa !238
  %208 = insertelement <64 x float> undef, float %207, i32 0
  %209 = shufflevector <64 x float> %208, <64 x float> undef, <64 x i32> zeroinitializer
  %210 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %209, <64 x float> %203, <64 x float> %192)
  %211 = add nsw i64 %194, 64
  %212 = getelementptr inbounds float, float* %4, i64 %211
  %213 = load float, float* %212, align 4, !tbaa !238
  %214 = insertelement <64 x float> undef, float %213, i32 0
  %215 = shufflevector <64 x float> %214, <64 x float> undef, <64 x i32> zeroinitializer
  %216 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %215, <64 x float> %203, <64 x float> %191)
  %217 = add nsw i64 %194, 96
  %218 = getelementptr inbounds float, float* %4, i64 %217
  %219 = load float, float* %218, align 4, !tbaa !238
  %220 = insertelement <64 x float> undef, float %219, i32 0
  %221 = shufflevector <64 x float> %220, <64 x float> undef, <64 x i32> zeroinitializer
  %222 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %221, <64 x float> %203, <64 x float> %190)
  %223 = add nsw i64 %200, 2048
  %224 = getelementptr inbounds float, float* %7, i64 %223
  %225 = bitcast float* %224 to <64 x float>*
  %226 = load <64 x float>, <64 x float>* %225, align 64, !tbaa !255
  %227 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %209, <64 x float> %226, <64 x float> %204)
  %228 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %215, <64 x float> %226, <64 x float> %210)
  %229 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %221, <64 x float> %226, <64 x float> %216)
  %230 = add nsw i64 %194, 128
  %231 = getelementptr inbounds float, float* %4, i64 %230
  %232 = load float, float* %231, align 4, !tbaa !238
  %233 = insertelement <64 x float> undef, float %232, i32 0
  %234 = shufflevector <64 x float> %233, <64 x float> undef, <64 x i32> zeroinitializer
  %235 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %234, <64 x float> %226, <64 x float> %222)
  %236 = add nsw i64 %200, 4096
  %237 = getelementptr inbounds float, float* %7, i64 %236
  %238 = bitcast float* %237 to <64 x float>*
  %239 = load <64 x float>, <64 x float>* %238, align 64, !tbaa !255
  %240 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %215, <64 x float> %239, <64 x float> %227)
  %241 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %221, <64 x float> %239, <64 x float> %228)
  %242 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %234, <64 x float> %239, <64 x float> %229)
  %243 = add nsw i64 %194, 160
  %244 = getelementptr inbounds float, float* %4, i64 %243
  %245 = load float, float* %244, align 4, !tbaa !238
  %246 = insertelement <64 x float> undef, float %245, i32 0
  %247 = shufflevector <64 x float> %246, <64 x float> undef, <64 x i32> zeroinitializer
  %248 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %247, <64 x float> %239, <64 x float> %235)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 32
  br i1 %exitcond.1, label %for_end14.1, label %for_body13.1, !prof !20

for_end14.1:                                      ; preds = %for_body13.1
  %249 = add nsw i64 %59, %50
  %250 = add nsw i64 %61, 12288
  br label %for_body13.2

for_body13.2:                                     ; preds = %for_body13.2, %for_end14.1
  %indvars.iv.2 = phi i64 [ 0, %for_end14.1 ], [ %indvars.iv.next.2, %for_body13.2 ]
  %251 = phi <64 x float> [ %248, %for_end14.1 ], [ %309, %for_body13.2 ]
  %252 = phi <64 x float> [ %242, %for_end14.1 ], [ %303, %for_body13.2 ]
  %253 = phi <64 x float> [ %241, %for_end14.1 ], [ %302, %for_body13.2 ]
  %254 = phi <64 x float> [ %240, %for_end14.1 ], [ %301, %for_body13.2 ]
  %255 = add nsw i64 %249, %indvars.iv.2
  %256 = getelementptr inbounds float, float* %4, i64 %255
  %257 = load float, float* %256, align 4, !tbaa !238
  %258 = insertelement <64 x float> undef, float %257, i32 0
  %259 = shufflevector <64 x float> %258, <64 x float> undef, <64 x i32> zeroinitializer
  %260 = shl nsw i64 %indvars.iv.2, 6
  %261 = add nsw i64 %250, %260
  %262 = getelementptr inbounds float, float* %7, i64 %261
  %263 = bitcast float* %262 to <64 x float>*
  %264 = load <64 x float>, <64 x float>* %263, align 64, !tbaa !255
  %265 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %259, <64 x float> %264, <64 x float> %254)
  %266 = add nsw i64 %255, 32
  %267 = getelementptr inbounds float, float* %4, i64 %266
  %268 = load float, float* %267, align 4, !tbaa !238
  %269 = insertelement <64 x float> undef, float %268, i32 0
  %270 = shufflevector <64 x float> %269, <64 x float> undef, <64 x i32> zeroinitializer
  %271 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %270, <64 x float> %264, <64 x float> %253)
  %272 = add nsw i64 %255, 64
  %273 = getelementptr inbounds float, float* %4, i64 %272
  %274 = load float, float* %273, align 4, !tbaa !238
  %275 = insertelement <64 x float> undef, float %274, i32 0
  %276 = shufflevector <64 x float> %275, <64 x float> undef, <64 x i32> zeroinitializer
  %277 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %276, <64 x float> %264, <64 x float> %252)
  %278 = add nsw i64 %255, 96
  %279 = getelementptr inbounds float, float* %4, i64 %278
  %280 = load float, float* %279, align 4, !tbaa !238
  %281 = insertelement <64 x float> undef, float %280, i32 0
  %282 = shufflevector <64 x float> %281, <64 x float> undef, <64 x i32> zeroinitializer
  %283 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %282, <64 x float> %264, <64 x float> %251)
  %284 = add nsw i64 %261, 2048
  %285 = getelementptr inbounds float, float* %7, i64 %284
  %286 = bitcast float* %285 to <64 x float>*
  %287 = load <64 x float>, <64 x float>* %286, align 64, !tbaa !255
  %288 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %270, <64 x float> %287, <64 x float> %265)
  %289 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %276, <64 x float> %287, <64 x float> %271)
  %290 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %282, <64 x float> %287, <64 x float> %277)
  %291 = add nsw i64 %255, 128
  %292 = getelementptr inbounds float, float* %4, i64 %291
  %293 = load float, float* %292, align 4, !tbaa !238
  %294 = insertelement <64 x float> undef, float %293, i32 0
  %295 = shufflevector <64 x float> %294, <64 x float> undef, <64 x i32> zeroinitializer
  %296 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %295, <64 x float> %287, <64 x float> %283)
  %297 = add nsw i64 %261, 4096
  %298 = getelementptr inbounds float, float* %7, i64 %297
  %299 = bitcast float* %298 to <64 x float>*
  %300 = load <64 x float>, <64 x float>* %299, align 64, !tbaa !255
  %301 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %276, <64 x float> %300, <64 x float> %288)
  %302 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %282, <64 x float> %300, <64 x float> %289)
  %303 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %295, <64 x float> %300, <64 x float> %290)
  %304 = add nsw i64 %255, 160
  %305 = getelementptr inbounds float, float* %4, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !238
  %307 = insertelement <64 x float> undef, float %306, i32 0
  %308 = shufflevector <64 x float> %307, <64 x float> undef, <64 x i32> zeroinitializer
  %309 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %308, <64 x float> %300, <64 x float> %296)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 32
  br i1 %exitcond.2, label %for_end14.2, label %for_body13.2, !prof !20

for_end14.2:                                      ; preds = %for_body13.2
  %indvars.iv.next56 = add nuw nsw i64 %indvars.iv55, 1
  %exitcond57 = icmp eq i64 %indvars.iv.next56, 2
  br i1 %exitcond57, label %for_end8, label %for_body7, !prof !20
}

define dllexport i32 @fused_nn_max_pool2d_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !261 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !263, metadata !DIExpression()), !dbg !266
  call void @llvm.dbg.value(metadata i8* %1, metadata !264, metadata !DIExpression()), !dbg !266
  call void @llvm.dbg.value(metadata i32 %2, metadata !265, metadata !DIExpression()), !dbg !266
  %3 = bitcast i8* %0 to %1**, !dbg !266
  %4 = load %1*, %1** %3, align 8, !dbg !266
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !266
  %6 = bitcast i8* %5 to %1**, !dbg !266
  %7 = load %1*, %1** %6, align 8, !dbg !266
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !266
  %9 = load i8*, i8** %8, align 8, !dbg !266
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !266
  %11 = load i8*, i8** %10, align 8, !dbg !266
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_1_compute_(i8* %11, i8* %9), !dbg !266
  ret i32 %12, !dbg !266
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
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.16, i8* nonnull %5, i32 0)
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
  %10 = add nsw i32 %9, 447
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 448
  %15 = select i1 %14, i32 %13, i32 448
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 448
  %18 = select i1 %17, i32 %16, i32 448
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
  %24 = mul nsw i64 %indvars.iv7, 224
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = mul i32 %25, 896
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %27 = shl i64 %indvars.iv, 4
  %28 = add nsw i64 %27, %24
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <16 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %31 = shl i32 %indvars.iv.tr, 5
  %32 = add i32 %31, %26
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to <16 x float>*
  %36 = load <16 x float>, <16 x float>* %35, align 64, !tbaa !267
  %37 = fcmp olt <16 x float> %36, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %38 = select <16 x i1> %37, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %36
  %39 = or i32 %32, 16
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to <16 x float>*
  %43 = load <16 x float>, <16 x float>* %42, align 64, !tbaa !267
  %44 = fcmp ogt <16 x float> %38, %43
  %45 = select <16 x i1> %44, <16 x float> %38, <16 x float> %43
  %46 = add i32 %32, 448
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to <16 x float>*
  %50 = load <16 x float>, <16 x float>* %49, align 64, !tbaa !267
  %51 = fcmp ogt <16 x float> %45, %50
  %52 = select <16 x i1> %51, <16 x float> %45, <16 x float> %50
  %53 = or i32 %46, 16
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %7, i64 %54
  %56 = bitcast float* %55 to <16 x float>*
  %57 = load <16 x float>, <16 x float>* %56, align 64, !tbaa !267
  %58 = fcmp ogt <16 x float> %52, %57
  %59 = select <16 x i1> %58, <16 x float> %52, <16 x float> %57
  store <16 x float> %59, <16 x float>* %30, align 64, !tbaa !270
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %60 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %60, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_layout_transform_16(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !273 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !275, metadata !DIExpression()), !dbg !278
  call void @llvm.dbg.value(metadata i8* %1, metadata !276, metadata !DIExpression()), !dbg !278
  call void @llvm.dbg.value(metadata i32 %2, metadata !277, metadata !DIExpression()), !dbg !278
  %3 = bitcast i8* %0 to %1**, !dbg !278
  %4 = load %1*, %1** %3, align 8, !dbg !278
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !278
  %6 = bitcast i8* %5 to %1**, !dbg !278
  %7 = load %1*, %1** %6, align 8, !dbg !278
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !278
  %9 = load i8*, i8** %8, align 8, !dbg !278
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !278
  %11 = load i8*, i8** %10, align 8, !dbg !278
  %12 = tail call fastcc i32 @fused_layout_transform_16_compute_(i8* %11, i8* %9), !dbg !278
  ret i32 %12, !dbg !278
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_16_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %21, align 8
  %3 = getelementptr inbounds %21, %21* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %21, %21* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %21* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.17, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.17(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 27
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 28
  %15 = select i1 %14, i32 %13, i32 28
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 28
  %18 = select i1 %17, i32 %16, i32 28
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
  %24 = mul nsw i64 %indvars.iv10, 7168
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = mul i32 %25, 896
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %27 = shl i64 %indvars.iv7, 8
  %28 = add nsw i64 %27, %24
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %29 = shl i32 %indvars.iv7.tr, 5
  %30 = add i32 %29, %26
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %31 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %31, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %32 = add nsw i64 %28, %indvars.iv
  %33 = trunc i64 %indvars.iv to i32
  %34 = and i32 %33, 31
  %35 = lshr i32 %33, 5
  %36 = mul nsw i32 %35, 25088
  %37 = add i32 %30, %36
  %38 = or i32 %37, %34
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, float* %7, i64 %39
  %41 = bitcast float* %40 to i32*
  %42 = load i32, i32* %41, align 4, !tbaa !279
  %43 = getelementptr inbounds float, float* %4, i64 %32
  %44 = bitcast float* %43 to i32*
  store i32 %42, i32* %44, align 4, !tbaa !282
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 28
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_layout_transform_14(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !285 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !287, metadata !DIExpression()), !dbg !290
  call void @llvm.dbg.value(metadata i8* %1, metadata !288, metadata !DIExpression()), !dbg !290
  call void @llvm.dbg.value(metadata i32 %2, metadata !289, metadata !DIExpression()), !dbg !290
  %3 = bitcast i8* %0 to %1**, !dbg !290
  %4 = load %1*, %1** %3, align 8, !dbg !290
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !290
  %6 = bitcast i8* %5 to %1**, !dbg !290
  %7 = load %1*, %1** %6, align 8, !dbg !290
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !290
  %9 = load i8*, i8** %8, align 8, !dbg !290
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !290
  %11 = load i8*, i8** %10, align 8, !dbg !290
  %12 = tail call fastcc i32 @fused_layout_transform_14_compute_(i8* %11, i8* %9), !dbg !290
  ret i32 %12, !dbg !290
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_14_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %22, align 8
  %3 = getelementptr inbounds %22, %22* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %22, %22* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %22* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.18, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.18(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 27
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 28
  %15 = select i1 %14, i32 %13, i32 28
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 28
  %18 = select i1 %17, i32 %16, i32 28
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
  %24 = mul nsw i64 %indvars.iv10, 14336
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = mul i32 %25, 448
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %27 = shl i64 %indvars.iv7, 9
  %28 = add nsw i64 %27, %24
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %29 = shl i32 %indvars.iv7.tr, 4
  %30 = add i32 %29, %26
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %31 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %31, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %32 = add nsw i64 %28, %indvars.iv
  %33 = trunc i64 %indvars.iv to i32
  %34 = and i32 %33, 15
  %35 = lshr i32 %33, 4
  %36 = mul nsw i32 %35, 12544
  %37 = add i32 %30, %36
  %38 = or i32 %37, %34
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, float* %7, i64 %39
  %41 = bitcast float* %40 to i32*
  %42 = load i32, i32* %41, align 4, !tbaa !291
  %43 = getelementptr inbounds float, float* %4, i64 %32
  %44 = bitcast float* %43 to i32*
  store i32 %42, i32* %44, align 4, !tbaa !294
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 28
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_layout_transform_20(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !297 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !299, metadata !DIExpression()), !dbg !302
  call void @llvm.dbg.value(metadata i8* %1, metadata !300, metadata !DIExpression()), !dbg !302
  call void @llvm.dbg.value(metadata i32 %2, metadata !301, metadata !DIExpression()), !dbg !302
  %3 = bitcast i8* %0 to %1**, !dbg !302
  %4 = load %1*, %1** %3, align 8, !dbg !302
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !302
  %6 = bitcast i8* %5 to %1**, !dbg !302
  %7 = load %1*, %1** %6, align 8, !dbg !302
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !302
  %9 = load i8*, i8** %8, align 8, !dbg !302
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !302
  %11 = load i8*, i8** %10, align 8, !dbg !302
  %12 = tail call fastcc i32 @fused_layout_transform_20_compute_(i8* %11, i8* %9), !dbg !302
  ret i32 %12, !dbg !302
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_20_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %23, align 8
  %3 = getelementptr inbounds %23, %23* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %23, %23* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %23* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.19, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.19(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
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
  %wide.load = load <4 x i32>, <4 x i32>* %48, align 4, !tbaa !303
  %49 = add i32 %45, 50176
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to <4 x i32>*
  %wide.load18 = load <4 x i32>, <4 x i32>* %52, align 4, !tbaa !303
  %53 = or i64 %43, 2
  %54 = add i32 %45, 100352
  %55 = sext i32 %54 to i64
  %56 = getelementptr inbounds float, float* %7, i64 %55
  %57 = bitcast float* %56 to <4 x i32>*
  %wide.load19 = load <4 x i32>, <4 x i32>* %57, align 4, !tbaa !303
  %58 = getelementptr inbounds float, float* %4, i64 %53
  %59 = getelementptr float, float* %58, i64 -2
  %60 = bitcast float* %59 to <12 x i32>*
  %61 = shufflevector <4 x i32> %wide.load, <4 x i32> %wide.load18, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %62 = shufflevector <4 x i32> %wide.load19, <4 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef>
  %interleaved.vec = shufflevector <8 x i32> %61, <8 x i32> %62, <12 x i32> <i32 0, i32 4, i32 8, i32 1, i32 5, i32 9, i32 2, i32 6, i32 10, i32 3, i32 7, i32 11>
  store <12 x i32> %interleaved.vec, <12 x i32>* %60, align 4, !tbaa !306
  %index.next = add i64 %index, 4
  %63 = icmp eq i64 %index.next, 224
  br i1 %63, label %for_end3, label %vector.body, !llvm.loop !309

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
  %71 = load i32, i32* %70, align 4, !tbaa !303
  %72 = getelementptr inbounds float, float* %4, i64 %65
  %73 = bitcast float* %72 to i32*
  store i32 %71, i32* %73, align 4, !tbaa !306
  %74 = add nsw i64 %65, 1
  %75 = add i32 %67, 50176
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %7, i64 %76
  %78 = bitcast float* %77 to i32*
  %79 = load i32, i32* %78, align 4, !tbaa !303
  %80 = getelementptr inbounds float, float* %4, i64 %74
  %81 = bitcast float* %80 to i32*
  store i32 %79, i32* %81, align 4, !tbaa !306
  %82 = add nsw i64 %65, 2
  %83 = add i32 %67, 100352
  %84 = sext i32 %83 to i64
  %85 = getelementptr inbounds float, float* %7, i64 %84
  %86 = bitcast float* %85 to i32*
  %87 = load i32, i32* %86, align 4, !tbaa !303
  %88 = getelementptr inbounds float, float* %4, i64 %82
  %89 = bitcast float* %88 to i32*
  store i32 %87, i32* %89, align 4, !tbaa !306
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 224
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20, !llvm.loop !310

for_end3:                                         ; preds = %vector.body, %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %90 = icmp slt i64 %indvars.iv.next8, %23
  %indvar.next = add i32 %indvar, 1
  br i1 %90, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_dense_add(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !311 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !313, metadata !DIExpression()), !dbg !316
  call void @llvm.dbg.value(metadata i8* %1, metadata !314, metadata !DIExpression()), !dbg !316
  call void @llvm.dbg.value(metadata i32 %2, metadata !315, metadata !DIExpression()), !dbg !316
  %3 = bitcast i8* %0 to %1**, !dbg !316
  %4 = load %1*, %1** %3, align 8, !dbg !316
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !316
  %6 = bitcast i8* %5 to %1**, !dbg !316
  %7 = load %1*, %1** %6, align 8, !dbg !316
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !316
  %9 = bitcast i8* %8 to %1**, !dbg !316
  %10 = load %1*, %1** %9, align 8, !dbg !316
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !316
  %12 = bitcast i8* %11 to %1**, !dbg !316
  %13 = load %1*, %1** %12, align 8, !dbg !316
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !316
  %15 = load i8*, i8** %14, align 8, !dbg !316
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !316
  %17 = load i32, i32* %16, align 4, !dbg !316
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !316
  %19 = load i8*, i8** %18, align 8, !dbg !316
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !316
  %21 = load i8*, i8** %20, align 8, !dbg !316
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !316
  %23 = load i8*, i8** %22, align 8, !dbg !316
  %24 = tail call fastcc i32 @fused_nn_dense_add_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !316
  ret i32 %24, !dbg !316
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 4000, i32 2, i32 32)
  %7 = alloca %24, align 8
  %8 = getelementptr inbounds %24, %24* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %24, %24* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %24, %24* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %24* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.20, i8* nonnull %11, i32 0)
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
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !317
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !317
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !320
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !320
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = getelementptr inbounds float, float* %17, i64 %index
  %29 = bitcast float* %28 to <4 x float>*
  store <4 x float> %26, <4 x float>* %29, align 4, !tbaa !323
  %30 = getelementptr float, float* %28, i64 4
  %31 = bitcast float* %30 to <4 x float>*
  store <4 x float> %27, <4 x float>* %31, align 4, !tbaa !323
  %index.next = add i64 %index, 8
  %32 = icmp eq i64 %index.next, 1000
  br i1 %32, label %for_end, label %vector.body, !llvm.loop !326

for_end:                                          ; preds = %vector.body
  %33 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %34 = call i32 %33(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.20(i32, %0* nocapture readonly, i8* nocapture readonly) #3 {
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
  %33 = load <16 x float>, <16 x float>* %32, align 64, !tbaa !327
  %34 = add nsw i64 %30, %29
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <16 x float>*
  %37 = load <16 x float>, <16 x float>* %36, align 64, !tbaa !330
  %38 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %33, <16 x float> %37, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

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
  store float %55, float* %39, align 4, !tbaa !320
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %56 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %56, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !333 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !335, metadata !DIExpression()), !dbg !338
  call void @llvm.dbg.value(metadata i8* %1, metadata !336, metadata !DIExpression()), !dbg !338
  call void @llvm.dbg.value(metadata i32 %2, metadata !337, metadata !DIExpression()), !dbg !338
  %3 = bitcast i8* %0 to %1**, !dbg !338
  %4 = load %1*, %1** %3, align 8, !dbg !338
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !338
  %6 = bitcast i8* %5 to %1**, !dbg !338
  %7 = load %1*, %1** %6, align 8, !dbg !338
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !338
  %9 = bitcast i8* %8 to %1**, !dbg !338
  %10 = load %1*, %1** %9, align 8, !dbg !338
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !338
  %12 = bitcast i8* %11 to %1**, !dbg !338
  %13 = load %1*, %1** %12, align 8, !dbg !338
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !338
  %15 = load i8*, i8** %14, align 8, !dbg !338
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !338
  %17 = load i32, i32* %16, align 4, !dbg !338
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !338
  %19 = load i8*, i8** %18, align 8, !dbg !338
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !338
  %21 = load i8*, i8** %20, align 8, !dbg !338
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !338
  %23 = load i8*, i8** %22, align 8, !dbg !338
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !338
  ret i32 %24, !dbg !338
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 3444736, i32 2, i32 32)
  %7 = alloca %25, align 8
  %8 = getelementptr inbounds %25, %25* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %25, %25* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %25* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.21, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %26, align 8
  %15 = getelementptr inbounds %26, %26* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %26, %26* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %26, %26* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %26, %26* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %26, %26* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %26* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.22, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.21(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 57
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 58
  %15 = select i1 %14, i32 %13, i32 58
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 58
  %18 = select i1 %17, i32 %16, i32 58
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %556, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 14848
  %.off = add i32 %20, -1
  %22 = icmp ult i32 %.off, 56
  %23 = mul nsw i32 %20, 14336
  br i1 %22, label %for_body2.us.preheader, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %for_end6.us
  %indvars.iv24 = phi i64 [ %indvars.iv.next25, %for_end6.us ], [ 0, %for_body2.us.preheader ]
  %24 = shl nsw i64 %indvars.iv24, 8
  %25 = trunc i64 %indvars.iv24 to i32
  %26 = add i32 %25, -1
  %27 = icmp ult i32 %26, 56
  br i1 %27, label %vector.body.preheader, label %vector.body37

vector.body.preheader:                            ; preds = %for_body2.us
  br label %vector.body

for_end6.us:                                      ; preds = %vector.body, %vector.body37
  %indvars.iv.next25 = add nuw nsw i64 %indvars.iv24, 1
  %exitcond27 = icmp eq i64 %indvars.iv.next25, 58
  br i1 %exitcond27, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %vector.body.preheader, %vector.body
  %index = phi i64 [ %index.next, %vector.body ], [ 0, %vector.body.preheader ]
  %28 = add nuw nsw i64 %index, %24
  %29 = trunc i64 %28 to i32
  %30 = add i32 %21, %29
  %31 = trunc i64 %28 to i32
  %32 = add i32 %31, -14592
  %33 = add i32 %32, %23
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !339
  %37 = getelementptr float, float* %35, i64 4
  %38 = bitcast float* %37 to <4 x i32>*
  %wide.load36 = load <4 x i32>, <4 x i32>* %38, align 4, !tbaa !339
  %39 = sext i32 %30 to i64
  %40 = getelementptr inbounds float, float* %4, i64 %39
  %41 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %41, align 4, !tbaa !342
  %42 = getelementptr float, float* %40, i64 4
  %43 = bitcast float* %42 to <4 x i32>*
  store <4 x i32> %wide.load36, <4 x i32>* %43, align 4, !tbaa !342
  %index.next = add i64 %index, 8
  %44 = icmp eq i64 %index.next, 256
  br i1 %44, label %for_end6.us, label %vector.body, !llvm.loop !345

vector.body37:                                    ; preds = %for_body2.us
  %45 = trunc i64 %24 to i32
  %46 = add i32 %21, %45
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %4, i64 %47
  %49 = bitcast float* %48 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %49, align 4, !tbaa !342
  %50 = getelementptr float, float* %48, i64 4
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %51, align 4, !tbaa !342
  %52 = trunc i64 %24 to i32
  %53 = or i32 %52, 8
  %54 = add i32 %21, %53
  %55 = sext i32 %54 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %57, align 4, !tbaa !342
  %58 = getelementptr float, float* %56, i64 4
  %59 = bitcast float* %58 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %59, align 4, !tbaa !342
  %60 = trunc i64 %24 to i32
  %61 = or i32 %60, 16
  %62 = add i32 %21, %61
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = bitcast float* %64 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %65, align 4, !tbaa !342
  %66 = getelementptr float, float* %64, i64 4
  %67 = bitcast float* %66 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %67, align 4, !tbaa !342
  %68 = trunc i64 %24 to i32
  %69 = or i32 %68, 24
  %70 = add i32 %21, %69
  %71 = sext i32 %70 to i64
  %72 = getelementptr inbounds float, float* %4, i64 %71
  %73 = bitcast float* %72 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %73, align 4, !tbaa !342
  %74 = getelementptr float, float* %72, i64 4
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %75, align 4, !tbaa !342
  %76 = trunc i64 %24 to i32
  %77 = or i32 %76, 32
  %78 = add i32 %21, %77
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %81, align 4, !tbaa !342
  %82 = getelementptr float, float* %80, i64 4
  %83 = bitcast float* %82 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %83, align 4, !tbaa !342
  %84 = trunc i64 %24 to i32
  %85 = or i32 %84, 40
  %86 = add i32 %21, %85
  %87 = sext i32 %86 to i64
  %88 = getelementptr inbounds float, float* %4, i64 %87
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %89, align 4, !tbaa !342
  %90 = getelementptr float, float* %88, i64 4
  %91 = bitcast float* %90 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %91, align 4, !tbaa !342
  %92 = trunc i64 %24 to i32
  %93 = or i32 %92, 48
  %94 = add i32 %21, %93
  %95 = sext i32 %94 to i64
  %96 = getelementptr inbounds float, float* %4, i64 %95
  %97 = bitcast float* %96 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %97, align 4, !tbaa !342
  %98 = getelementptr float, float* %96, i64 4
  %99 = bitcast float* %98 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %99, align 4, !tbaa !342
  %100 = trunc i64 %24 to i32
  %101 = or i32 %100, 56
  %102 = add i32 %21, %101
  %103 = sext i32 %102 to i64
  %104 = getelementptr inbounds float, float* %4, i64 %103
  %105 = bitcast float* %104 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %105, align 4, !tbaa !342
  %106 = getelementptr float, float* %104, i64 4
  %107 = bitcast float* %106 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %107, align 4, !tbaa !342
  %108 = trunc i64 %24 to i32
  %109 = or i32 %108, 64
  %110 = add i32 %21, %109
  %111 = sext i32 %110 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %113, align 4, !tbaa !342
  %114 = getelementptr float, float* %112, i64 4
  %115 = bitcast float* %114 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %115, align 4, !tbaa !342
  %116 = trunc i64 %24 to i32
  %117 = or i32 %116, 72
  %118 = add i32 %21, %117
  %119 = sext i32 %118 to i64
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = bitcast float* %120 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %121, align 4, !tbaa !342
  %122 = getelementptr float, float* %120, i64 4
  %123 = bitcast float* %122 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %123, align 4, !tbaa !342
  %124 = trunc i64 %24 to i32
  %125 = or i32 %124, 80
  %126 = add i32 %21, %125
  %127 = sext i32 %126 to i64
  %128 = getelementptr inbounds float, float* %4, i64 %127
  %129 = bitcast float* %128 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %129, align 4, !tbaa !342
  %130 = getelementptr float, float* %128, i64 4
  %131 = bitcast float* %130 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %131, align 4, !tbaa !342
  %132 = trunc i64 %24 to i32
  %133 = or i32 %132, 88
  %134 = add i32 %21, %133
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %4, i64 %135
  %137 = bitcast float* %136 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %137, align 4, !tbaa !342
  %138 = getelementptr float, float* %136, i64 4
  %139 = bitcast float* %138 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %139, align 4, !tbaa !342
  %140 = trunc i64 %24 to i32
  %141 = or i32 %140, 96
  %142 = add i32 %21, %141
  %143 = sext i32 %142 to i64
  %144 = getelementptr inbounds float, float* %4, i64 %143
  %145 = bitcast float* %144 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %145, align 4, !tbaa !342
  %146 = getelementptr float, float* %144, i64 4
  %147 = bitcast float* %146 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %147, align 4, !tbaa !342
  %148 = trunc i64 %24 to i32
  %149 = or i32 %148, 104
  %150 = add i32 %21, %149
  %151 = sext i32 %150 to i64
  %152 = getelementptr inbounds float, float* %4, i64 %151
  %153 = bitcast float* %152 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %153, align 4, !tbaa !342
  %154 = getelementptr float, float* %152, i64 4
  %155 = bitcast float* %154 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %155, align 4, !tbaa !342
  %156 = trunc i64 %24 to i32
  %157 = or i32 %156, 112
  %158 = add i32 %21, %157
  %159 = sext i32 %158 to i64
  %160 = getelementptr inbounds float, float* %4, i64 %159
  %161 = bitcast float* %160 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %161, align 4, !tbaa !342
  %162 = getelementptr float, float* %160, i64 4
  %163 = bitcast float* %162 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %163, align 4, !tbaa !342
  %164 = trunc i64 %24 to i32
  %165 = or i32 %164, 120
  %166 = add i32 %21, %165
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  %169 = bitcast float* %168 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %169, align 4, !tbaa !342
  %170 = getelementptr float, float* %168, i64 4
  %171 = bitcast float* %170 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %171, align 4, !tbaa !342
  %172 = trunc i64 %24 to i32
  %173 = or i32 %172, 128
  %174 = add i32 %21, %173
  %175 = sext i32 %174 to i64
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = bitcast float* %176 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %177, align 4, !tbaa !342
  %178 = getelementptr float, float* %176, i64 4
  %179 = bitcast float* %178 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %179, align 4, !tbaa !342
  %180 = trunc i64 %24 to i32
  %181 = or i32 %180, 136
  %182 = add i32 %21, %181
  %183 = sext i32 %182 to i64
  %184 = getelementptr inbounds float, float* %4, i64 %183
  %185 = bitcast float* %184 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %185, align 4, !tbaa !342
  %186 = getelementptr float, float* %184, i64 4
  %187 = bitcast float* %186 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %187, align 4, !tbaa !342
  %188 = trunc i64 %24 to i32
  %189 = or i32 %188, 144
  %190 = add i32 %21, %189
  %191 = sext i32 %190 to i64
  %192 = getelementptr inbounds float, float* %4, i64 %191
  %193 = bitcast float* %192 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %193, align 4, !tbaa !342
  %194 = getelementptr float, float* %192, i64 4
  %195 = bitcast float* %194 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %195, align 4, !tbaa !342
  %196 = trunc i64 %24 to i32
  %197 = or i32 %196, 152
  %198 = add i32 %21, %197
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %4, i64 %199
  %201 = bitcast float* %200 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %201, align 4, !tbaa !342
  %202 = getelementptr float, float* %200, i64 4
  %203 = bitcast float* %202 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %203, align 4, !tbaa !342
  %204 = trunc i64 %24 to i32
  %205 = or i32 %204, 160
  %206 = add i32 %21, %205
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  %209 = bitcast float* %208 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %209, align 4, !tbaa !342
  %210 = getelementptr float, float* %208, i64 4
  %211 = bitcast float* %210 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %211, align 4, !tbaa !342
  %212 = trunc i64 %24 to i32
  %213 = or i32 %212, 168
  %214 = add i32 %21, %213
  %215 = sext i32 %214 to i64
  %216 = getelementptr inbounds float, float* %4, i64 %215
  %217 = bitcast float* %216 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %217, align 4, !tbaa !342
  %218 = getelementptr float, float* %216, i64 4
  %219 = bitcast float* %218 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %219, align 4, !tbaa !342
  %220 = trunc i64 %24 to i32
  %221 = or i32 %220, 176
  %222 = add i32 %21, %221
  %223 = sext i32 %222 to i64
  %224 = getelementptr inbounds float, float* %4, i64 %223
  %225 = bitcast float* %224 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %225, align 4, !tbaa !342
  %226 = getelementptr float, float* %224, i64 4
  %227 = bitcast float* %226 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %227, align 4, !tbaa !342
  %228 = trunc i64 %24 to i32
  %229 = or i32 %228, 184
  %230 = add i32 %21, %229
  %231 = sext i32 %230 to i64
  %232 = getelementptr inbounds float, float* %4, i64 %231
  %233 = bitcast float* %232 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %233, align 4, !tbaa !342
  %234 = getelementptr float, float* %232, i64 4
  %235 = bitcast float* %234 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %235, align 4, !tbaa !342
  %236 = trunc i64 %24 to i32
  %237 = or i32 %236, 192
  %238 = add i32 %21, %237
  %239 = sext i32 %238 to i64
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = bitcast float* %240 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %241, align 4, !tbaa !342
  %242 = getelementptr float, float* %240, i64 4
  %243 = bitcast float* %242 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %243, align 4, !tbaa !342
  %244 = trunc i64 %24 to i32
  %245 = or i32 %244, 200
  %246 = add i32 %21, %245
  %247 = sext i32 %246 to i64
  %248 = getelementptr inbounds float, float* %4, i64 %247
  %249 = bitcast float* %248 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %249, align 4, !tbaa !342
  %250 = getelementptr float, float* %248, i64 4
  %251 = bitcast float* %250 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %251, align 4, !tbaa !342
  %252 = trunc i64 %24 to i32
  %253 = or i32 %252, 208
  %254 = add i32 %21, %253
  %255 = sext i32 %254 to i64
  %256 = getelementptr inbounds float, float* %4, i64 %255
  %257 = bitcast float* %256 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %257, align 4, !tbaa !342
  %258 = getelementptr float, float* %256, i64 4
  %259 = bitcast float* %258 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %259, align 4, !tbaa !342
  %260 = trunc i64 %24 to i32
  %261 = or i32 %260, 216
  %262 = add i32 %21, %261
  %263 = sext i32 %262 to i64
  %264 = getelementptr inbounds float, float* %4, i64 %263
  %265 = bitcast float* %264 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %265, align 4, !tbaa !342
  %266 = getelementptr float, float* %264, i64 4
  %267 = bitcast float* %266 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %267, align 4, !tbaa !342
  %268 = trunc i64 %24 to i32
  %269 = or i32 %268, 224
  %270 = add i32 %21, %269
  %271 = sext i32 %270 to i64
  %272 = getelementptr inbounds float, float* %4, i64 %271
  %273 = bitcast float* %272 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %273, align 4, !tbaa !342
  %274 = getelementptr float, float* %272, i64 4
  %275 = bitcast float* %274 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %275, align 4, !tbaa !342
  %276 = trunc i64 %24 to i32
  %277 = or i32 %276, 232
  %278 = add i32 %21, %277
  %279 = sext i32 %278 to i64
  %280 = getelementptr inbounds float, float* %4, i64 %279
  %281 = bitcast float* %280 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %281, align 4, !tbaa !342
  %282 = getelementptr float, float* %280, i64 4
  %283 = bitcast float* %282 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %283, align 4, !tbaa !342
  %284 = trunc i64 %24 to i32
  %285 = or i32 %284, 240
  %286 = add i32 %21, %285
  %287 = sext i32 %286 to i64
  %288 = getelementptr inbounds float, float* %4, i64 %287
  %289 = bitcast float* %288 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %289, align 4, !tbaa !342
  %290 = getelementptr float, float* %288, i64 4
  %291 = bitcast float* %290 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %291, align 4, !tbaa !342
  %292 = trunc i64 %24 to i32
  %293 = or i32 %292, 248
  %294 = add i32 %21, %293
  %295 = sext i32 %294 to i64
  %296 = getelementptr inbounds float, float* %4, i64 %295
  %297 = bitcast float* %296 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %297, align 4, !tbaa !342
  %298 = getelementptr float, float* %296, i64 4
  %299 = bitcast float* %298 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %299, align 4, !tbaa !342
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv15 = phi i64 [ %indvars.iv.next16, %for_body2 ], [ 0, %for_body2.preheader ]
  %300 = shl nsw i64 %indvars.iv15, 8
  %301 = trunc i64 %300 to i32
  %302 = add i32 %21, %301
  %303 = sext i32 %302 to i64
  %304 = getelementptr inbounds float, float* %4, i64 %303
  %305 = bitcast float* %304 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %305, align 4, !tbaa !342
  %306 = getelementptr float, float* %304, i64 4
  %307 = bitcast float* %306 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %307, align 4, !tbaa !342
  %308 = trunc i64 %300 to i32
  %309 = or i32 %308, 8
  %310 = add i32 %21, %309
  %311 = sext i32 %310 to i64
  %312 = getelementptr inbounds float, float* %4, i64 %311
  %313 = bitcast float* %312 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %313, align 4, !tbaa !342
  %314 = getelementptr float, float* %312, i64 4
  %315 = bitcast float* %314 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %315, align 4, !tbaa !342
  %316 = trunc i64 %300 to i32
  %317 = or i32 %316, 16
  %318 = add i32 %21, %317
  %319 = sext i32 %318 to i64
  %320 = getelementptr inbounds float, float* %4, i64 %319
  %321 = bitcast float* %320 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %321, align 4, !tbaa !342
  %322 = getelementptr float, float* %320, i64 4
  %323 = bitcast float* %322 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %323, align 4, !tbaa !342
  %324 = trunc i64 %300 to i32
  %325 = or i32 %324, 24
  %326 = add i32 %21, %325
  %327 = sext i32 %326 to i64
  %328 = getelementptr inbounds float, float* %4, i64 %327
  %329 = bitcast float* %328 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %329, align 4, !tbaa !342
  %330 = getelementptr float, float* %328, i64 4
  %331 = bitcast float* %330 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %331, align 4, !tbaa !342
  %332 = trunc i64 %300 to i32
  %333 = or i32 %332, 32
  %334 = add i32 %21, %333
  %335 = sext i32 %334 to i64
  %336 = getelementptr inbounds float, float* %4, i64 %335
  %337 = bitcast float* %336 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %337, align 4, !tbaa !342
  %338 = getelementptr float, float* %336, i64 4
  %339 = bitcast float* %338 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %339, align 4, !tbaa !342
  %340 = trunc i64 %300 to i32
  %341 = or i32 %340, 40
  %342 = add i32 %21, %341
  %343 = sext i32 %342 to i64
  %344 = getelementptr inbounds float, float* %4, i64 %343
  %345 = bitcast float* %344 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %345, align 4, !tbaa !342
  %346 = getelementptr float, float* %344, i64 4
  %347 = bitcast float* %346 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %347, align 4, !tbaa !342
  %348 = trunc i64 %300 to i32
  %349 = or i32 %348, 48
  %350 = add i32 %21, %349
  %351 = sext i32 %350 to i64
  %352 = getelementptr inbounds float, float* %4, i64 %351
  %353 = bitcast float* %352 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %353, align 4, !tbaa !342
  %354 = getelementptr float, float* %352, i64 4
  %355 = bitcast float* %354 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %355, align 4, !tbaa !342
  %356 = trunc i64 %300 to i32
  %357 = or i32 %356, 56
  %358 = add i32 %21, %357
  %359 = sext i32 %358 to i64
  %360 = getelementptr inbounds float, float* %4, i64 %359
  %361 = bitcast float* %360 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %361, align 4, !tbaa !342
  %362 = getelementptr float, float* %360, i64 4
  %363 = bitcast float* %362 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %363, align 4, !tbaa !342
  %364 = trunc i64 %300 to i32
  %365 = or i32 %364, 64
  %366 = add i32 %21, %365
  %367 = sext i32 %366 to i64
  %368 = getelementptr inbounds float, float* %4, i64 %367
  %369 = bitcast float* %368 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %369, align 4, !tbaa !342
  %370 = getelementptr float, float* %368, i64 4
  %371 = bitcast float* %370 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %371, align 4, !tbaa !342
  %372 = trunc i64 %300 to i32
  %373 = or i32 %372, 72
  %374 = add i32 %21, %373
  %375 = sext i32 %374 to i64
  %376 = getelementptr inbounds float, float* %4, i64 %375
  %377 = bitcast float* %376 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %377, align 4, !tbaa !342
  %378 = getelementptr float, float* %376, i64 4
  %379 = bitcast float* %378 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %379, align 4, !tbaa !342
  %380 = trunc i64 %300 to i32
  %381 = or i32 %380, 80
  %382 = add i32 %21, %381
  %383 = sext i32 %382 to i64
  %384 = getelementptr inbounds float, float* %4, i64 %383
  %385 = bitcast float* %384 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %385, align 4, !tbaa !342
  %386 = getelementptr float, float* %384, i64 4
  %387 = bitcast float* %386 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %387, align 4, !tbaa !342
  %388 = trunc i64 %300 to i32
  %389 = or i32 %388, 88
  %390 = add i32 %21, %389
  %391 = sext i32 %390 to i64
  %392 = getelementptr inbounds float, float* %4, i64 %391
  %393 = bitcast float* %392 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %393, align 4, !tbaa !342
  %394 = getelementptr float, float* %392, i64 4
  %395 = bitcast float* %394 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %395, align 4, !tbaa !342
  %396 = trunc i64 %300 to i32
  %397 = or i32 %396, 96
  %398 = add i32 %21, %397
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds float, float* %4, i64 %399
  %401 = bitcast float* %400 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %401, align 4, !tbaa !342
  %402 = getelementptr float, float* %400, i64 4
  %403 = bitcast float* %402 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %403, align 4, !tbaa !342
  %404 = trunc i64 %300 to i32
  %405 = or i32 %404, 104
  %406 = add i32 %21, %405
  %407 = sext i32 %406 to i64
  %408 = getelementptr inbounds float, float* %4, i64 %407
  %409 = bitcast float* %408 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %409, align 4, !tbaa !342
  %410 = getelementptr float, float* %408, i64 4
  %411 = bitcast float* %410 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %411, align 4, !tbaa !342
  %412 = trunc i64 %300 to i32
  %413 = or i32 %412, 112
  %414 = add i32 %21, %413
  %415 = sext i32 %414 to i64
  %416 = getelementptr inbounds float, float* %4, i64 %415
  %417 = bitcast float* %416 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %417, align 4, !tbaa !342
  %418 = getelementptr float, float* %416, i64 4
  %419 = bitcast float* %418 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %419, align 4, !tbaa !342
  %420 = trunc i64 %300 to i32
  %421 = or i32 %420, 120
  %422 = add i32 %21, %421
  %423 = sext i32 %422 to i64
  %424 = getelementptr inbounds float, float* %4, i64 %423
  %425 = bitcast float* %424 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %425, align 4, !tbaa !342
  %426 = getelementptr float, float* %424, i64 4
  %427 = bitcast float* %426 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %427, align 4, !tbaa !342
  %428 = trunc i64 %300 to i32
  %429 = or i32 %428, 128
  %430 = add i32 %21, %429
  %431 = sext i32 %430 to i64
  %432 = getelementptr inbounds float, float* %4, i64 %431
  %433 = bitcast float* %432 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %433, align 4, !tbaa !342
  %434 = getelementptr float, float* %432, i64 4
  %435 = bitcast float* %434 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %435, align 4, !tbaa !342
  %436 = trunc i64 %300 to i32
  %437 = or i32 %436, 136
  %438 = add i32 %21, %437
  %439 = sext i32 %438 to i64
  %440 = getelementptr inbounds float, float* %4, i64 %439
  %441 = bitcast float* %440 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %441, align 4, !tbaa !342
  %442 = getelementptr float, float* %440, i64 4
  %443 = bitcast float* %442 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %443, align 4, !tbaa !342
  %444 = trunc i64 %300 to i32
  %445 = or i32 %444, 144
  %446 = add i32 %21, %445
  %447 = sext i32 %446 to i64
  %448 = getelementptr inbounds float, float* %4, i64 %447
  %449 = bitcast float* %448 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %449, align 4, !tbaa !342
  %450 = getelementptr float, float* %448, i64 4
  %451 = bitcast float* %450 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %451, align 4, !tbaa !342
  %452 = trunc i64 %300 to i32
  %453 = or i32 %452, 152
  %454 = add i32 %21, %453
  %455 = sext i32 %454 to i64
  %456 = getelementptr inbounds float, float* %4, i64 %455
  %457 = bitcast float* %456 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %457, align 4, !tbaa !342
  %458 = getelementptr float, float* %456, i64 4
  %459 = bitcast float* %458 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %459, align 4, !tbaa !342
  %460 = trunc i64 %300 to i32
  %461 = or i32 %460, 160
  %462 = add i32 %21, %461
  %463 = sext i32 %462 to i64
  %464 = getelementptr inbounds float, float* %4, i64 %463
  %465 = bitcast float* %464 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %465, align 4, !tbaa !342
  %466 = getelementptr float, float* %464, i64 4
  %467 = bitcast float* %466 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %467, align 4, !tbaa !342
  %468 = trunc i64 %300 to i32
  %469 = or i32 %468, 168
  %470 = add i32 %21, %469
  %471 = sext i32 %470 to i64
  %472 = getelementptr inbounds float, float* %4, i64 %471
  %473 = bitcast float* %472 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %473, align 4, !tbaa !342
  %474 = getelementptr float, float* %472, i64 4
  %475 = bitcast float* %474 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %475, align 4, !tbaa !342
  %476 = trunc i64 %300 to i32
  %477 = or i32 %476, 176
  %478 = add i32 %21, %477
  %479 = sext i32 %478 to i64
  %480 = getelementptr inbounds float, float* %4, i64 %479
  %481 = bitcast float* %480 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %481, align 4, !tbaa !342
  %482 = getelementptr float, float* %480, i64 4
  %483 = bitcast float* %482 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %483, align 4, !tbaa !342
  %484 = trunc i64 %300 to i32
  %485 = or i32 %484, 184
  %486 = add i32 %21, %485
  %487 = sext i32 %486 to i64
  %488 = getelementptr inbounds float, float* %4, i64 %487
  %489 = bitcast float* %488 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %489, align 4, !tbaa !342
  %490 = getelementptr float, float* %488, i64 4
  %491 = bitcast float* %490 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %491, align 4, !tbaa !342
  %492 = trunc i64 %300 to i32
  %493 = or i32 %492, 192
  %494 = add i32 %21, %493
  %495 = sext i32 %494 to i64
  %496 = getelementptr inbounds float, float* %4, i64 %495
  %497 = bitcast float* %496 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %497, align 4, !tbaa !342
  %498 = getelementptr float, float* %496, i64 4
  %499 = bitcast float* %498 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %499, align 4, !tbaa !342
  %500 = trunc i64 %300 to i32
  %501 = or i32 %500, 200
  %502 = add i32 %21, %501
  %503 = sext i32 %502 to i64
  %504 = getelementptr inbounds float, float* %4, i64 %503
  %505 = bitcast float* %504 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %505, align 4, !tbaa !342
  %506 = getelementptr float, float* %504, i64 4
  %507 = bitcast float* %506 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %507, align 4, !tbaa !342
  %508 = trunc i64 %300 to i32
  %509 = or i32 %508, 208
  %510 = add i32 %21, %509
  %511 = sext i32 %510 to i64
  %512 = getelementptr inbounds float, float* %4, i64 %511
  %513 = bitcast float* %512 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %513, align 4, !tbaa !342
  %514 = getelementptr float, float* %512, i64 4
  %515 = bitcast float* %514 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %515, align 4, !tbaa !342
  %516 = trunc i64 %300 to i32
  %517 = or i32 %516, 216
  %518 = add i32 %21, %517
  %519 = sext i32 %518 to i64
  %520 = getelementptr inbounds float, float* %4, i64 %519
  %521 = bitcast float* %520 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %521, align 4, !tbaa !342
  %522 = getelementptr float, float* %520, i64 4
  %523 = bitcast float* %522 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %523, align 4, !tbaa !342
  %524 = trunc i64 %300 to i32
  %525 = or i32 %524, 224
  %526 = add i32 %21, %525
  %527 = sext i32 %526 to i64
  %528 = getelementptr inbounds float, float* %4, i64 %527
  %529 = bitcast float* %528 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %529, align 4, !tbaa !342
  %530 = getelementptr float, float* %528, i64 4
  %531 = bitcast float* %530 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %531, align 4, !tbaa !342
  %532 = trunc i64 %300 to i32
  %533 = or i32 %532, 232
  %534 = add i32 %21, %533
  %535 = sext i32 %534 to i64
  %536 = getelementptr inbounds float, float* %4, i64 %535
  %537 = bitcast float* %536 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %537, align 4, !tbaa !342
  %538 = getelementptr float, float* %536, i64 4
  %539 = bitcast float* %538 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %539, align 4, !tbaa !342
  %540 = trunc i64 %300 to i32
  %541 = or i32 %540, 240
  %542 = add i32 %21, %541
  %543 = sext i32 %542 to i64
  %544 = getelementptr inbounds float, float* %4, i64 %543
  %545 = bitcast float* %544 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %545, align 4, !tbaa !342
  %546 = getelementptr float, float* %544, i64 4
  %547 = bitcast float* %546 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %547, align 4, !tbaa !342
  %548 = trunc i64 %300 to i32
  %549 = or i32 %548, 248
  %550 = add i32 %21, %549
  %551 = sext i32 %550 to i64
  %552 = getelementptr inbounds float, float* %4, i64 %551
  %553 = bitcast float* %552 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %553, align 4, !tbaa !342
  %554 = getelementptr float, float* %552, i64 4
  %555 = bitcast float* %554 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %555, align 4, !tbaa !342
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp eq i64 %indvars.iv.next16, 58
  br i1 %exitcond17, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %556 = add nsw i32 %20, 1
  %557 = icmp slt i32 %556, %15
  br i1 %557, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.22(i32, %0* nocapture readonly, i8* nocapture readonly) {
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
  %14 = getelementptr inbounds i8, i8* %2, i64 32
  %15 = bitcast i8* %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %18 = load i32, i32* %17, align 4
  %19 = add nsw i32 %18, 447
  %20 = sdiv i32 %19, %18
  %21 = add nsw i32 %0, 1
  %22 = mul nsw i32 %20, %21
  %23 = icmp slt i32 %22, 448
  %24 = select i1 %23, i32 %22, i32 448
  %25 = mul nsw i32 %20, %0
  %26 = icmp slt i32 %25, 448
  %27 = select i1 %26, i32 %25, i32 448
  %28 = icmp slt i32 %27, %24
  br i1 %28, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end17
  %29 = phi i32 [ %268, %for_end17 ], [ %27, %for_body.preheader ]
  %30 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %31 = tail call i8* %30(i32 1, i32 %16, i64 7168, i32 2, i32 32)
  %32 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %33 = tail call i8* %32(i32 1, i32 %16, i64 1024, i32 2, i32 32)
  %34 = bitcast i8* %33 to <32 x float>*
  %35 = getelementptr inbounds i8, i8* %33, i64 128
  %36 = bitcast i8* %35 to <32 x float>*
  %37 = getelementptr inbounds i8, i8* %33, i64 256
  %38 = bitcast i8* %37 to <32 x float>*
  %39 = getelementptr inbounds i8, i8* %33, i64 384
  %40 = bitcast i8* %39 to <32 x float>*
  %41 = getelementptr inbounds i8, i8* %33, i64 512
  %42 = bitcast i8* %41 to <32 x float>*
  %43 = getelementptr inbounds i8, i8* %33, i64 640
  %44 = bitcast i8* %43 to <32 x float>*
  %45 = getelementptr inbounds i8, i8* %33, i64 768
  %46 = bitcast i8* %45 to <32 x float>*
  %47 = getelementptr inbounds i8, i8* %33, i64 896
  %48 = bitcast i8* %47 to <32 x float>*
  %49 = srem i32 %29, 56
  %50 = sdiv i32 %29, 56
  %51 = mul nsw i32 %50, 73728
  %52 = bitcast i8* %31 to float*
  %53 = sext i32 %51 to i64
  %54 = mul nsw i32 %49, 14848
  %55 = sext i32 %54 to i64
  %56 = mul nsw i32 %49, 14848
  %57 = add nsw i32 %56, 14848
  %58 = sext i32 %57 to i64
  %59 = add nsw i64 %53, 24576
  %60 = mul nsw i32 %49, 14848
  %61 = add nsw i32 %60, 29696
  %62 = sext i32 %61 to i64
  %63 = add nsw i64 %53, 49152
  br label %for_body4

for_end:                                          ; preds = %for_end17, %entry
  ret i32 0

for_body4:                                        ; preds = %for_end11.2, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end11.2 ]
  %64 = shl i64 %indvar, 11
  %65 = add nsw i64 %64, %55
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 1024, i32 64, i1 false)
  br label %for_body10

for_end5:                                         ; preds = %for_end11.2
  %66 = mul nsw i32 %29, 1792
  %67 = shl nsw i32 %50, 5
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds float, float* %13, i64 %68
  %70 = bitcast float* %69 to <32 x float>*
  %71 = load <32 x float>, <32 x float>* %70, align 64, !tbaa !346
  br label %for_body16

for_body10:                                       ; preds = %for_body10, %for_body4
  %indvars.iv = phi i64 [ 0, %for_body4 ], [ %indvars.iv.next, %for_body10 ]
  %72 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %166, %for_body10 ]
  %73 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %160, %for_body10 ]
  %74 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %159, %for_body10 ]
  %75 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %158, %for_body10 ]
  %76 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %157, %for_body10 ]
  %77 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %156, %for_body10 ]
  %78 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %155, %for_body10 ]
  %79 = phi <32 x float> [ zeroinitializer, %for_body4 ], [ %154, %for_body10 ]
  %80 = add nsw i64 %65, %indvars.iv
  %81 = getelementptr inbounds float, float* %4, i64 %80
  %82 = load float, float* %81, align 4, !tbaa !342
  %83 = insertelement <32 x float> undef, float %82, i32 0
  %84 = shufflevector <32 x float> %83, <32 x float> undef, <32 x i32> zeroinitializer
  %85 = shl nsw i64 %indvars.iv, 5
  %86 = add nsw i64 %85, %53
  %87 = getelementptr inbounds float, float* %7, i64 %86
  %88 = bitcast float* %87 to <32 x float>*
  %89 = load <32 x float>, <32 x float>* %88, align 64, !tbaa !349
  %90 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %84, <32 x float> %89, <32 x float> %79)
  %91 = add nsw i64 %80, 256
  %92 = getelementptr inbounds float, float* %4, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !342
  %94 = insertelement <32 x float> undef, float %93, i32 0
  %95 = shufflevector <32 x float> %94, <32 x float> undef, <32 x i32> zeroinitializer
  %96 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %95, <32 x float> %89, <32 x float> %78)
  %97 = add nsw i64 %80, 512
  %98 = getelementptr inbounds float, float* %4, i64 %97
  %99 = load float, float* %98, align 4, !tbaa !342
  %100 = insertelement <32 x float> undef, float %99, i32 0
  %101 = shufflevector <32 x float> %100, <32 x float> undef, <32 x i32> zeroinitializer
  %102 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %101, <32 x float> %89, <32 x float> %77)
  %103 = add nsw i64 %80, 768
  %104 = getelementptr inbounds float, float* %4, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !342
  %106 = insertelement <32 x float> undef, float %105, i32 0
  %107 = shufflevector <32 x float> %106, <32 x float> undef, <32 x i32> zeroinitializer
  %108 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %107, <32 x float> %89, <32 x float> %76)
  %109 = add nsw i64 %80, 1024
  %110 = getelementptr inbounds float, float* %4, i64 %109
  %111 = load float, float* %110, align 4, !tbaa !342
  %112 = insertelement <32 x float> undef, float %111, i32 0
  %113 = shufflevector <32 x float> %112, <32 x float> undef, <32 x i32> zeroinitializer
  %114 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %113, <32 x float> %89, <32 x float> %75)
  %115 = add nsw i64 %80, 1280
  %116 = getelementptr inbounds float, float* %4, i64 %115
  %117 = load float, float* %116, align 4, !tbaa !342
  %118 = insertelement <32 x float> undef, float %117, i32 0
  %119 = shufflevector <32 x float> %118, <32 x float> undef, <32 x i32> zeroinitializer
  %120 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %119, <32 x float> %89, <32 x float> %74)
  %121 = add nsw i64 %80, 1536
  %122 = getelementptr inbounds float, float* %4, i64 %121
  %123 = load float, float* %122, align 4, !tbaa !342
  %124 = insertelement <32 x float> undef, float %123, i32 0
  %125 = shufflevector <32 x float> %124, <32 x float> undef, <32 x i32> zeroinitializer
  %126 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %125, <32 x float> %89, <32 x float> %73)
  %127 = add nsw i64 %80, 1792
  %128 = getelementptr inbounds float, float* %4, i64 %127
  %129 = load float, float* %128, align 4, !tbaa !342
  %130 = insertelement <32 x float> undef, float %129, i32 0
  %131 = shufflevector <32 x float> %130, <32 x float> undef, <32 x i32> zeroinitializer
  %132 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %131, <32 x float> %89, <32 x float> %72)
  %133 = add nsw i64 %86, 8192
  %134 = getelementptr inbounds float, float* %7, i64 %133
  %135 = bitcast float* %134 to <32 x float>*
  %136 = load <32 x float>, <32 x float>* %135, align 64, !tbaa !349
  %137 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %95, <32 x float> %136, <32 x float> %90)
  %138 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %101, <32 x float> %136, <32 x float> %96)
  %139 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %107, <32 x float> %136, <32 x float> %102)
  %140 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %113, <32 x float> %136, <32 x float> %108)
  %141 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %119, <32 x float> %136, <32 x float> %114)
  %142 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %125, <32 x float> %136, <32 x float> %120)
  %143 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %131, <32 x float> %136, <32 x float> %126)
  %144 = add nsw i64 %80, 2048
  %145 = getelementptr inbounds float, float* %4, i64 %144
  %146 = load float, float* %145, align 4, !tbaa !342
  %147 = insertelement <32 x float> undef, float %146, i32 0
  %148 = shufflevector <32 x float> %147, <32 x float> undef, <32 x i32> zeroinitializer
  %149 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %148, <32 x float> %136, <32 x float> %132)
  %150 = add nsw i64 %86, 16384
  %151 = getelementptr inbounds float, float* %7, i64 %150
  %152 = bitcast float* %151 to <32 x float>*
  %153 = load <32 x float>, <32 x float>* %152, align 64, !tbaa !349
  %154 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %101, <32 x float> %153, <32 x float> %137)
  %155 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %107, <32 x float> %153, <32 x float> %138)
  %156 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %113, <32 x float> %153, <32 x float> %139)
  %157 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %119, <32 x float> %153, <32 x float> %140)
  %158 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %125, <32 x float> %153, <32 x float> %141)
  %159 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %131, <32 x float> %153, <32 x float> %142)
  %160 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %148, <32 x float> %153, <32 x float> %143)
  %161 = add nsw i64 %80, 2304
  %162 = getelementptr inbounds float, float* %4, i64 %161
  %163 = load float, float* %162, align 4, !tbaa !342
  %164 = insertelement <32 x float> undef, float %163, i32 0
  %165 = shufflevector <32 x float> %164, <32 x float> undef, <32 x i32> zeroinitializer
  %166 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %165, <32 x float> %153, <32 x float> %149)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end11, label %for_body10, !prof !20

for_end11:                                        ; preds = %for_body10
  %167 = add nsw i64 %64, %58
  br label %for_body10.1

for_body16:                                       ; preds = %for_body16, %for_end5
  %indvars.iv71 = phi i64 [ 0, %for_end5 ], [ %indvars.iv.next72, %for_body16 ]
  %168 = shl nsw i64 %indvars.iv71, 8
  %169 = trunc i64 %168 to i32
  %170 = add i32 %66, %169
  %171 = getelementptr inbounds float, float* %52, i64 %168
  %172 = bitcast float* %171 to <32 x float>*
  %173 = load <32 x float>, <32 x float>* %172, align 64, !tbaa !352
  %174 = fadd <32 x float> %71, %173
  %175 = fcmp ogt <32 x float> %174, zeroinitializer
  %176 = select <32 x i1> %175, <32 x float> %174, <32 x float> zeroinitializer
  %177 = sext i32 %170 to i64
  %178 = getelementptr inbounds float, float* %10, i64 %177
  %179 = bitcast float* %178 to <32 x float>*
  store <32 x float> %176, <32 x float>* %179, align 64, !tbaa !355
  %180 = or i64 %168, 32
  %181 = trunc i64 %180 to i32
  %182 = add i32 %66, %181
  %183 = getelementptr inbounds float, float* %52, i64 %180
  %184 = bitcast float* %183 to <32 x float>*
  %185 = load <32 x float>, <32 x float>* %184, align 64, !tbaa !352
  %186 = fadd <32 x float> %71, %185
  %187 = fcmp ogt <32 x float> %186, zeroinitializer
  %188 = select <32 x i1> %187, <32 x float> %186, <32 x float> zeroinitializer
  %189 = sext i32 %182 to i64
  %190 = getelementptr inbounds float, float* %10, i64 %189
  %191 = bitcast float* %190 to <32 x float>*
  store <32 x float> %188, <32 x float>* %191, align 64, !tbaa !355
  %192 = or i64 %168, 64
  %193 = trunc i64 %192 to i32
  %194 = add i32 %66, %193
  %195 = getelementptr inbounds float, float* %52, i64 %192
  %196 = bitcast float* %195 to <32 x float>*
  %197 = load <32 x float>, <32 x float>* %196, align 64, !tbaa !352
  %198 = fadd <32 x float> %71, %197
  %199 = fcmp ogt <32 x float> %198, zeroinitializer
  %200 = select <32 x i1> %199, <32 x float> %198, <32 x float> zeroinitializer
  %201 = sext i32 %194 to i64
  %202 = getelementptr inbounds float, float* %10, i64 %201
  %203 = bitcast float* %202 to <32 x float>*
  store <32 x float> %200, <32 x float>* %203, align 64, !tbaa !355
  %204 = or i64 %168, 96
  %205 = trunc i64 %204 to i32
  %206 = add i32 %66, %205
  %207 = getelementptr inbounds float, float* %52, i64 %204
  %208 = bitcast float* %207 to <32 x float>*
  %209 = load <32 x float>, <32 x float>* %208, align 64, !tbaa !352
  %210 = fadd <32 x float> %71, %209
  %211 = fcmp ogt <32 x float> %210, zeroinitializer
  %212 = select <32 x i1> %211, <32 x float> %210, <32 x float> zeroinitializer
  %213 = sext i32 %206 to i64
  %214 = getelementptr inbounds float, float* %10, i64 %213
  %215 = bitcast float* %214 to <32 x float>*
  store <32 x float> %212, <32 x float>* %215, align 64, !tbaa !355
  %216 = or i64 %168, 128
  %217 = trunc i64 %216 to i32
  %218 = add i32 %66, %217
  %219 = getelementptr inbounds float, float* %52, i64 %216
  %220 = bitcast float* %219 to <32 x float>*
  %221 = load <32 x float>, <32 x float>* %220, align 64, !tbaa !352
  %222 = fadd <32 x float> %71, %221
  %223 = fcmp ogt <32 x float> %222, zeroinitializer
  %224 = select <32 x i1> %223, <32 x float> %222, <32 x float> zeroinitializer
  %225 = sext i32 %218 to i64
  %226 = getelementptr inbounds float, float* %10, i64 %225
  %227 = bitcast float* %226 to <32 x float>*
  store <32 x float> %224, <32 x float>* %227, align 64, !tbaa !355
  %228 = or i64 %168, 160
  %229 = trunc i64 %228 to i32
  %230 = add i32 %66, %229
  %231 = getelementptr inbounds float, float* %52, i64 %228
  %232 = bitcast float* %231 to <32 x float>*
  %233 = load <32 x float>, <32 x float>* %232, align 64, !tbaa !352
  %234 = fadd <32 x float> %71, %233
  %235 = fcmp ogt <32 x float> %234, zeroinitializer
  %236 = select <32 x i1> %235, <32 x float> %234, <32 x float> zeroinitializer
  %237 = sext i32 %230 to i64
  %238 = getelementptr inbounds float, float* %10, i64 %237
  %239 = bitcast float* %238 to <32 x float>*
  store <32 x float> %236, <32 x float>* %239, align 64, !tbaa !355
  %240 = or i64 %168, 192
  %241 = trunc i64 %240 to i32
  %242 = add i32 %66, %241
  %243 = getelementptr inbounds float, float* %52, i64 %240
  %244 = bitcast float* %243 to <32 x float>*
  %245 = load <32 x float>, <32 x float>* %244, align 64, !tbaa !352
  %246 = fadd <32 x float> %71, %245
  %247 = fcmp ogt <32 x float> %246, zeroinitializer
  %248 = select <32 x i1> %247, <32 x float> %246, <32 x float> zeroinitializer
  %249 = sext i32 %242 to i64
  %250 = getelementptr inbounds float, float* %10, i64 %249
  %251 = bitcast float* %250 to <32 x float>*
  store <32 x float> %248, <32 x float>* %251, align 64, !tbaa !355
  %252 = or i64 %168, 224
  %253 = trunc i64 %252 to i32
  %254 = add i32 %66, %253
  %255 = getelementptr inbounds float, float* %52, i64 %252
  %256 = bitcast float* %255 to <32 x float>*
  %257 = load <32 x float>, <32 x float>* %256, align 64, !tbaa !352
  %258 = fadd <32 x float> %71, %257
  %259 = fcmp ogt <32 x float> %258, zeroinitializer
  %260 = select <32 x i1> %259, <32 x float> %258, <32 x float> zeroinitializer
  %261 = sext i32 %254 to i64
  %262 = getelementptr inbounds float, float* %10, i64 %261
  %263 = bitcast float* %262 to <32 x float>*
  store <32 x float> %260, <32 x float>* %263, align 64, !tbaa !355
  %indvars.iv.next72 = add nuw nsw i64 %indvars.iv71, 1
  %exitcond73 = icmp eq i64 %indvars.iv.next72, 7
  br i1 %exitcond73, label %for_end17, label %for_body16, !prof !20

for_end17:                                        ; preds = %for_body16
  %264 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %265 = tail call i32 %264(i32 1, i32 %16, i8* %33)
  %266 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %267 = tail call i32 %266(i32 1, i32 %16, i8* nonnull %31)
  %268 = add nsw i32 %29, 1
  %269 = icmp slt i32 %268, %24
  br i1 %269, label %for_body, label %for_end, !prof !19

for_body10.1:                                     ; preds = %for_body10.1, %for_end11
  %indvars.iv.1 = phi i64 [ 0, %for_end11 ], [ %indvars.iv.next.1, %for_body10.1 ]
  %270 = phi <32 x float> [ %166, %for_end11 ], [ %364, %for_body10.1 ]
  %271 = phi <32 x float> [ %160, %for_end11 ], [ %358, %for_body10.1 ]
  %272 = phi <32 x float> [ %159, %for_end11 ], [ %357, %for_body10.1 ]
  %273 = phi <32 x float> [ %158, %for_end11 ], [ %356, %for_body10.1 ]
  %274 = phi <32 x float> [ %157, %for_end11 ], [ %355, %for_body10.1 ]
  %275 = phi <32 x float> [ %156, %for_end11 ], [ %354, %for_body10.1 ]
  %276 = phi <32 x float> [ %155, %for_end11 ], [ %353, %for_body10.1 ]
  %277 = phi <32 x float> [ %154, %for_end11 ], [ %352, %for_body10.1 ]
  %278 = add nsw i64 %167, %indvars.iv.1
  %279 = getelementptr inbounds float, float* %4, i64 %278
  %280 = load float, float* %279, align 4, !tbaa !342
  %281 = insertelement <32 x float> undef, float %280, i32 0
  %282 = shufflevector <32 x float> %281, <32 x float> undef, <32 x i32> zeroinitializer
  %283 = shl nsw i64 %indvars.iv.1, 5
  %284 = add nsw i64 %59, %283
  %285 = getelementptr inbounds float, float* %7, i64 %284
  %286 = bitcast float* %285 to <32 x float>*
  %287 = load <32 x float>, <32 x float>* %286, align 64, !tbaa !349
  %288 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %282, <32 x float> %287, <32 x float> %277)
  %289 = add nsw i64 %278, 256
  %290 = getelementptr inbounds float, float* %4, i64 %289
  %291 = load float, float* %290, align 4, !tbaa !342
  %292 = insertelement <32 x float> undef, float %291, i32 0
  %293 = shufflevector <32 x float> %292, <32 x float> undef, <32 x i32> zeroinitializer
  %294 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %293, <32 x float> %287, <32 x float> %276)
  %295 = add nsw i64 %278, 512
  %296 = getelementptr inbounds float, float* %4, i64 %295
  %297 = load float, float* %296, align 4, !tbaa !342
  %298 = insertelement <32 x float> undef, float %297, i32 0
  %299 = shufflevector <32 x float> %298, <32 x float> undef, <32 x i32> zeroinitializer
  %300 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %299, <32 x float> %287, <32 x float> %275)
  %301 = add nsw i64 %278, 768
  %302 = getelementptr inbounds float, float* %4, i64 %301
  %303 = load float, float* %302, align 4, !tbaa !342
  %304 = insertelement <32 x float> undef, float %303, i32 0
  %305 = shufflevector <32 x float> %304, <32 x float> undef, <32 x i32> zeroinitializer
  %306 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %305, <32 x float> %287, <32 x float> %274)
  %307 = add nsw i64 %278, 1024
  %308 = getelementptr inbounds float, float* %4, i64 %307
  %309 = load float, float* %308, align 4, !tbaa !342
  %310 = insertelement <32 x float> undef, float %309, i32 0
  %311 = shufflevector <32 x float> %310, <32 x float> undef, <32 x i32> zeroinitializer
  %312 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %311, <32 x float> %287, <32 x float> %273)
  %313 = add nsw i64 %278, 1280
  %314 = getelementptr inbounds float, float* %4, i64 %313
  %315 = load float, float* %314, align 4, !tbaa !342
  %316 = insertelement <32 x float> undef, float %315, i32 0
  %317 = shufflevector <32 x float> %316, <32 x float> undef, <32 x i32> zeroinitializer
  %318 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %287, <32 x float> %272)
  %319 = add nsw i64 %278, 1536
  %320 = getelementptr inbounds float, float* %4, i64 %319
  %321 = load float, float* %320, align 4, !tbaa !342
  %322 = insertelement <32 x float> undef, float %321, i32 0
  %323 = shufflevector <32 x float> %322, <32 x float> undef, <32 x i32> zeroinitializer
  %324 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %287, <32 x float> %271)
  %325 = add nsw i64 %278, 1792
  %326 = getelementptr inbounds float, float* %4, i64 %325
  %327 = load float, float* %326, align 4, !tbaa !342
  %328 = insertelement <32 x float> undef, float %327, i32 0
  %329 = shufflevector <32 x float> %328, <32 x float> undef, <32 x i32> zeroinitializer
  %330 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %287, <32 x float> %270)
  %331 = add nsw i64 %284, 8192
  %332 = getelementptr inbounds float, float* %7, i64 %331
  %333 = bitcast float* %332 to <32 x float>*
  %334 = load <32 x float>, <32 x float>* %333, align 64, !tbaa !349
  %335 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %293, <32 x float> %334, <32 x float> %288)
  %336 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %299, <32 x float> %334, <32 x float> %294)
  %337 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %305, <32 x float> %334, <32 x float> %300)
  %338 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %311, <32 x float> %334, <32 x float> %306)
  %339 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %334, <32 x float> %312)
  %340 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %334, <32 x float> %318)
  %341 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %334, <32 x float> %324)
  %342 = add nsw i64 %278, 2048
  %343 = getelementptr inbounds float, float* %4, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !342
  %345 = insertelement <32 x float> undef, float %344, i32 0
  %346 = shufflevector <32 x float> %345, <32 x float> undef, <32 x i32> zeroinitializer
  %347 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %346, <32 x float> %334, <32 x float> %330)
  %348 = add nsw i64 %284, 16384
  %349 = getelementptr inbounds float, float* %7, i64 %348
  %350 = bitcast float* %349 to <32 x float>*
  %351 = load <32 x float>, <32 x float>* %350, align 64, !tbaa !349
  %352 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %299, <32 x float> %351, <32 x float> %335)
  %353 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %305, <32 x float> %351, <32 x float> %336)
  %354 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %311, <32 x float> %351, <32 x float> %337)
  %355 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %351, <32 x float> %338)
  %356 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %323, <32 x float> %351, <32 x float> %339)
  %357 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %329, <32 x float> %351, <32 x float> %340)
  %358 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %346, <32 x float> %351, <32 x float> %341)
  %359 = add nsw i64 %278, 2304
  %360 = getelementptr inbounds float, float* %4, i64 %359
  %361 = load float, float* %360, align 4, !tbaa !342
  %362 = insertelement <32 x float> undef, float %361, i32 0
  %363 = shufflevector <32 x float> %362, <32 x float> undef, <32 x i32> zeroinitializer
  %364 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %363, <32 x float> %351, <32 x float> %347)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 256
  br i1 %exitcond.1, label %for_end11.1, label %for_body10.1, !prof !20

for_end11.1:                                      ; preds = %for_body10.1
  %365 = add nsw i64 %64, %62
  br label %for_body10.2

for_body10.2:                                     ; preds = %for_body10.2, %for_end11.1
  %indvars.iv.2 = phi i64 [ 0, %for_end11.1 ], [ %indvars.iv.next.2, %for_body10.2 ]
  %366 = phi <32 x float> [ %364, %for_end11.1 ], [ %460, %for_body10.2 ]
  %367 = phi <32 x float> [ %358, %for_end11.1 ], [ %454, %for_body10.2 ]
  %368 = phi <32 x float> [ %357, %for_end11.1 ], [ %453, %for_body10.2 ]
  %369 = phi <32 x float> [ %356, %for_end11.1 ], [ %452, %for_body10.2 ]
  %370 = phi <32 x float> [ %355, %for_end11.1 ], [ %451, %for_body10.2 ]
  %371 = phi <32 x float> [ %354, %for_end11.1 ], [ %450, %for_body10.2 ]
  %372 = phi <32 x float> [ %353, %for_end11.1 ], [ %449, %for_body10.2 ]
  %373 = phi <32 x float> [ %352, %for_end11.1 ], [ %448, %for_body10.2 ]
  %374 = add nsw i64 %365, %indvars.iv.2
  %375 = getelementptr inbounds float, float* %4, i64 %374
  %376 = load float, float* %375, align 4, !tbaa !342
  %377 = insertelement <32 x float> undef, float %376, i32 0
  %378 = shufflevector <32 x float> %377, <32 x float> undef, <32 x i32> zeroinitializer
  %379 = shl nsw i64 %indvars.iv.2, 5
  %380 = add nsw i64 %63, %379
  %381 = getelementptr inbounds float, float* %7, i64 %380
  %382 = bitcast float* %381 to <32 x float>*
  %383 = load <32 x float>, <32 x float>* %382, align 64, !tbaa !349
  %384 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %378, <32 x float> %383, <32 x float> %373)
  %385 = add nsw i64 %374, 256
  %386 = getelementptr inbounds float, float* %4, i64 %385
  %387 = load float, float* %386, align 4, !tbaa !342
  %388 = insertelement <32 x float> undef, float %387, i32 0
  %389 = shufflevector <32 x float> %388, <32 x float> undef, <32 x i32> zeroinitializer
  %390 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %389, <32 x float> %383, <32 x float> %372)
  %391 = add nsw i64 %374, 512
  %392 = getelementptr inbounds float, float* %4, i64 %391
  %393 = load float, float* %392, align 4, !tbaa !342
  %394 = insertelement <32 x float> undef, float %393, i32 0
  %395 = shufflevector <32 x float> %394, <32 x float> undef, <32 x i32> zeroinitializer
  %396 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %395, <32 x float> %383, <32 x float> %371)
  %397 = add nsw i64 %374, 768
  %398 = getelementptr inbounds float, float* %4, i64 %397
  %399 = load float, float* %398, align 4, !tbaa !342
  %400 = insertelement <32 x float> undef, float %399, i32 0
  %401 = shufflevector <32 x float> %400, <32 x float> undef, <32 x i32> zeroinitializer
  %402 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %401, <32 x float> %383, <32 x float> %370)
  %403 = add nsw i64 %374, 1024
  %404 = getelementptr inbounds float, float* %4, i64 %403
  %405 = load float, float* %404, align 4, !tbaa !342
  %406 = insertelement <32 x float> undef, float %405, i32 0
  %407 = shufflevector <32 x float> %406, <32 x float> undef, <32 x i32> zeroinitializer
  %408 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %407, <32 x float> %383, <32 x float> %369)
  %409 = add nsw i64 %374, 1280
  %410 = getelementptr inbounds float, float* %4, i64 %409
  %411 = load float, float* %410, align 4, !tbaa !342
  %412 = insertelement <32 x float> undef, float %411, i32 0
  %413 = shufflevector <32 x float> %412, <32 x float> undef, <32 x i32> zeroinitializer
  %414 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %413, <32 x float> %383, <32 x float> %368)
  %415 = add nsw i64 %374, 1536
  %416 = getelementptr inbounds float, float* %4, i64 %415
  %417 = load float, float* %416, align 4, !tbaa !342
  %418 = insertelement <32 x float> undef, float %417, i32 0
  %419 = shufflevector <32 x float> %418, <32 x float> undef, <32 x i32> zeroinitializer
  %420 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %383, <32 x float> %367)
  %421 = add nsw i64 %374, 1792
  %422 = getelementptr inbounds float, float* %4, i64 %421
  %423 = load float, float* %422, align 4, !tbaa !342
  %424 = insertelement <32 x float> undef, float %423, i32 0
  %425 = shufflevector <32 x float> %424, <32 x float> undef, <32 x i32> zeroinitializer
  %426 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %383, <32 x float> %366)
  %427 = add nsw i64 %380, 8192
  %428 = getelementptr inbounds float, float* %7, i64 %427
  %429 = bitcast float* %428 to <32 x float>*
  %430 = load <32 x float>, <32 x float>* %429, align 64, !tbaa !349
  %431 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %389, <32 x float> %430, <32 x float> %384)
  %432 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %395, <32 x float> %430, <32 x float> %390)
  %433 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %401, <32 x float> %430, <32 x float> %396)
  %434 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %407, <32 x float> %430, <32 x float> %402)
  %435 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %413, <32 x float> %430, <32 x float> %408)
  %436 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %430, <32 x float> %414)
  %437 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %430, <32 x float> %420)
  %438 = add nsw i64 %374, 2048
  %439 = getelementptr inbounds float, float* %4, i64 %438
  %440 = load float, float* %439, align 4, !tbaa !342
  %441 = insertelement <32 x float> undef, float %440, i32 0
  %442 = shufflevector <32 x float> %441, <32 x float> undef, <32 x i32> zeroinitializer
  %443 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %442, <32 x float> %430, <32 x float> %426)
  %444 = add nsw i64 %380, 16384
  %445 = getelementptr inbounds float, float* %7, i64 %444
  %446 = bitcast float* %445 to <32 x float>*
  %447 = load <32 x float>, <32 x float>* %446, align 64, !tbaa !349
  %448 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %395, <32 x float> %447, <32 x float> %431)
  %449 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %401, <32 x float> %447, <32 x float> %432)
  %450 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %407, <32 x float> %447, <32 x float> %433)
  %451 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %413, <32 x float> %447, <32 x float> %434)
  %452 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %419, <32 x float> %447, <32 x float> %435)
  %453 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %425, <32 x float> %447, <32 x float> %436)
  %454 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %442, <32 x float> %447, <32 x float> %437)
  %455 = add nsw i64 %374, 2304
  %456 = getelementptr inbounds float, float* %4, i64 %455
  %457 = load float, float* %456, align 4, !tbaa !342
  %458 = insertelement <32 x float> undef, float %457, i32 0
  %459 = shufflevector <32 x float> %458, <32 x float> undef, <32 x i32> zeroinitializer
  %460 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %459, <32 x float> %447, <32 x float> %443)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 256
  br i1 %exitcond.2, label %for_end11.2, label %for_body10.2, !prof !20

for_end11.2:                                      ; preds = %for_body10.2
  store <32 x float> %448, <32 x float>* %34, align 64, !tbaa !358
  store <32 x float> %449, <32 x float>* %36, align 64, !tbaa !358
  store <32 x float> %450, <32 x float>* %38, align 64, !tbaa !358
  store <32 x float> %451, <32 x float>* %40, align 64, !tbaa !358
  store <32 x float> %452, <32 x float>* %42, align 64, !tbaa !358
  store <32 x float> %453, <32 x float>* %44, align 64, !tbaa !358
  store <32 x float> %454, <32 x float>* %46, align 64, !tbaa !358
  store <32 x float> %460, <32 x float>* %48, align 64, !tbaa !358
  %461 = shl i64 %indvar, 8
  %462 = getelementptr inbounds float, float* %52, i64 %461
  %463 = bitcast float* %462 to <32 x float>*
  store <32 x float> %448, <32 x float>* %463, align 64, !tbaa !352
  %464 = or i64 %461, 32
  %465 = getelementptr inbounds float, float* %52, i64 %464
  %466 = bitcast float* %465 to <32 x float>*
  store <32 x float> %449, <32 x float>* %466, align 64, !tbaa !352
  %467 = or i64 %461, 64
  %468 = getelementptr inbounds float, float* %52, i64 %467
  %469 = bitcast float* %468 to <32 x float>*
  store <32 x float> %450, <32 x float>* %469, align 64, !tbaa !352
  %470 = or i64 %461, 96
  %471 = getelementptr inbounds float, float* %52, i64 %470
  %472 = bitcast float* %471 to <32 x float>*
  store <32 x float> %451, <32 x float>* %472, align 64, !tbaa !352
  %473 = or i64 %461, 128
  %474 = getelementptr inbounds float, float* %52, i64 %473
  %475 = bitcast float* %474 to <32 x float>*
  store <32 x float> %452, <32 x float>* %475, align 64, !tbaa !352
  %476 = or i64 %461, 160
  %477 = getelementptr inbounds float, float* %52, i64 %476
  %478 = bitcast float* %477 to <32 x float>*
  store <32 x float> %453, <32 x float>* %478, align 64, !tbaa !352
  %479 = or i64 %461, 192
  %480 = getelementptr inbounds float, float* %52, i64 %479
  %481 = bitcast float* %480 to <32 x float>*
  store <32 x float> %454, <32 x float>* %481, align 64, !tbaa !352
  %482 = or i64 %461, 224
  %483 = getelementptr inbounds float, float* %52, i64 %482
  %484 = bitcast float* %483 to <32 x float>*
  store <32 x float> %460, <32 x float>* %484, align 64, !tbaa !352
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond67 = icmp eq i64 %indvar.next, 7
  br i1 %exitcond67, label %for_end5, label %for_body4, !prof !20
}

define dllexport i32 @fused_nn_max_pool2d(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !367 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !369, metadata !DIExpression()), !dbg !372
  call void @llvm.dbg.value(metadata i8* %1, metadata !370, metadata !DIExpression()), !dbg !372
  call void @llvm.dbg.value(metadata i32 %2, metadata !371, metadata !DIExpression()), !dbg !372
  %3 = bitcast i8* %0 to %1**, !dbg !372
  %4 = load %1*, %1** %3, align 8, !dbg !372
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !372
  %6 = bitcast i8* %5 to %1**, !dbg !372
  %7 = load %1*, %1** %6, align 8, !dbg !372
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !372
  %9 = load i8*, i8** %8, align 8, !dbg !372
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !372
  %11 = load i8*, i8** %10, align 8, !dbg !372
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_compute_(i8* %11, i8* %9), !dbg !372
  ret i32 %12, !dbg !372
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %27, align 8
  %3 = getelementptr inbounds %27, %27* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %27, %27* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %27* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.23, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.23(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
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
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_body
  %indvars.iv = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next, %for_body ]
  %24 = phi i32 [ %18, %for_body.lr.ph ], [ %243, %for_body ]
  %25 = mul nsw i64 %indvars.iv, 112
  %26 = mul nsw i64 %indvars.iv, 448
  %27 = mul nsw i32 %24, 448
  %28 = getelementptr inbounds float, float* %4, i64 %25
  %29 = bitcast float* %28 to <16 x float>*
  %30 = getelementptr inbounds float, float* %7, i64 %26
  %31 = bitcast float* %30 to <16 x float>*
  %32 = load <16 x float>, <16 x float>* %31, align 64, !tbaa !373
  %33 = fcmp olt <16 x float> %32, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %34 = select <16 x i1> %33, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %32
  %35 = or i32 %27, 16
  %36 = sext i32 %35 to i64
  %37 = getelementptr inbounds float, float* %7, i64 %36
  %38 = bitcast float* %37 to <16 x float>*
  %39 = load <16 x float>, <16 x float>* %38, align 64, !tbaa !373
  %40 = fcmp ogt <16 x float> %34, %39
  %41 = select <16 x i1> %40, <16 x float> %34, <16 x float> %39
  %42 = trunc i64 %26 to i32
  %43 = add i32 %42, 224
  %44 = sext i32 %43 to i64
  %45 = getelementptr inbounds float, float* %7, i64 %44
  %46 = bitcast float* %45 to <16 x float>*
  %47 = load <16 x float>, <16 x float>* %46, align 64, !tbaa !373
  %48 = fcmp ogt <16 x float> %41, %47
  %49 = select <16 x i1> %48, <16 x float> %41, <16 x float> %47
  %50 = or i32 %43, 16
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds float, float* %7, i64 %51
  %53 = bitcast float* %52 to <16 x float>*
  %54 = load <16 x float>, <16 x float>* %53, align 64, !tbaa !373
  %55 = fcmp ogt <16 x float> %49, %54
  %56 = select <16 x i1> %55, <16 x float> %49, <16 x float> %54
  store <16 x float> %56, <16 x float>* %29, align 64, !tbaa !376
  %57 = add nsw i64 %25, 16
  %58 = getelementptr inbounds float, float* %4, i64 %57
  %59 = bitcast float* %58 to <16 x float>*
  %60 = or i32 %27, 32
  %61 = sext i32 %60 to i64
  %62 = getelementptr inbounds float, float* %7, i64 %61
  %63 = bitcast float* %62 to <16 x float>*
  %64 = load <16 x float>, <16 x float>* %63, align 64, !tbaa !373
  %65 = fcmp olt <16 x float> %64, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %66 = select <16 x i1> %65, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %64
  %67 = or i32 %27, 48
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds float, float* %7, i64 %68
  %70 = bitcast float* %69 to <16 x float>*
  %71 = load <16 x float>, <16 x float>* %70, align 64, !tbaa !373
  %72 = fcmp ogt <16 x float> %66, %71
  %73 = select <16 x i1> %72, <16 x float> %66, <16 x float> %71
  %74 = add i32 %60, 224
  %75 = sext i32 %74 to i64
  %76 = getelementptr inbounds float, float* %7, i64 %75
  %77 = bitcast float* %76 to <16 x float>*
  %78 = load <16 x float>, <16 x float>* %77, align 64, !tbaa !373
  %79 = fcmp ogt <16 x float> %73, %78
  %80 = select <16 x i1> %79, <16 x float> %73, <16 x float> %78
  %81 = or i32 %74, 16
  %82 = sext i32 %81 to i64
  %83 = getelementptr inbounds float, float* %7, i64 %82
  %84 = bitcast float* %83 to <16 x float>*
  %85 = load <16 x float>, <16 x float>* %84, align 64, !tbaa !373
  %86 = fcmp ogt <16 x float> %80, %85
  %87 = select <16 x i1> %86, <16 x float> %80, <16 x float> %85
  store <16 x float> %87, <16 x float>* %59, align 64, !tbaa !376
  %88 = add nsw i64 %25, 32
  %89 = getelementptr inbounds float, float* %4, i64 %88
  %90 = bitcast float* %89 to <16 x float>*
  %91 = add i32 %42, 64
  %92 = sext i32 %91 to i64
  %93 = getelementptr inbounds float, float* %7, i64 %92
  %94 = bitcast float* %93 to <16 x float>*
  %95 = load <16 x float>, <16 x float>* %94, align 64, !tbaa !373
  %96 = fcmp olt <16 x float> %95, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %97 = select <16 x i1> %96, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %95
  %98 = or i32 %91, 16
  %99 = sext i32 %98 to i64
  %100 = getelementptr inbounds float, float* %7, i64 %99
  %101 = bitcast float* %100 to <16 x float>*
  %102 = load <16 x float>, <16 x float>* %101, align 64, !tbaa !373
  %103 = fcmp ogt <16 x float> %97, %102
  %104 = select <16 x i1> %103, <16 x float> %97, <16 x float> %102
  %105 = add i32 %42, 288
  %106 = sext i32 %105 to i64
  %107 = getelementptr inbounds float, float* %7, i64 %106
  %108 = bitcast float* %107 to <16 x float>*
  %109 = load <16 x float>, <16 x float>* %108, align 64, !tbaa !373
  %110 = fcmp ogt <16 x float> %104, %109
  %111 = select <16 x i1> %110, <16 x float> %104, <16 x float> %109
  %112 = or i32 %105, 16
  %113 = sext i32 %112 to i64
  %114 = getelementptr inbounds float, float* %7, i64 %113
  %115 = bitcast float* %114 to <16 x float>*
  %116 = load <16 x float>, <16 x float>* %115, align 64, !tbaa !373
  %117 = fcmp ogt <16 x float> %111, %116
  %118 = select <16 x i1> %117, <16 x float> %111, <16 x float> %116
  store <16 x float> %118, <16 x float>* %90, align 64, !tbaa !376
  %119 = add nsw i64 %25, 48
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = bitcast float* %120 to <16 x float>*
  %122 = add i32 %42, 96
  %123 = sext i32 %122 to i64
  %124 = getelementptr inbounds float, float* %7, i64 %123
  %125 = bitcast float* %124 to <16 x float>*
  %126 = load <16 x float>, <16 x float>* %125, align 64, !tbaa !373
  %127 = fcmp olt <16 x float> %126, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %128 = select <16 x i1> %127, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %126
  %129 = or i32 %122, 16
  %130 = sext i32 %129 to i64
  %131 = getelementptr inbounds float, float* %7, i64 %130
  %132 = bitcast float* %131 to <16 x float>*
  %133 = load <16 x float>, <16 x float>* %132, align 64, !tbaa !373
  %134 = fcmp ogt <16 x float> %128, %133
  %135 = select <16 x i1> %134, <16 x float> %128, <16 x float> %133
  %136 = add i32 %42, 320
  %137 = sext i32 %136 to i64
  %138 = getelementptr inbounds float, float* %7, i64 %137
  %139 = bitcast float* %138 to <16 x float>*
  %140 = load <16 x float>, <16 x float>* %139, align 64, !tbaa !373
  %141 = fcmp ogt <16 x float> %135, %140
  %142 = select <16 x i1> %141, <16 x float> %135, <16 x float> %140
  %143 = or i32 %136, 16
  %144 = sext i32 %143 to i64
  %145 = getelementptr inbounds float, float* %7, i64 %144
  %146 = bitcast float* %145 to <16 x float>*
  %147 = load <16 x float>, <16 x float>* %146, align 64, !tbaa !373
  %148 = fcmp ogt <16 x float> %142, %147
  %149 = select <16 x i1> %148, <16 x float> %142, <16 x float> %147
  store <16 x float> %149, <16 x float>* %121, align 64, !tbaa !376
  %150 = add nsw i64 %25, 64
  %151 = getelementptr inbounds float, float* %4, i64 %150
  %152 = bitcast float* %151 to <16 x float>*
  %153 = add i32 %42, 128
  %154 = sext i32 %153 to i64
  %155 = getelementptr inbounds float, float* %7, i64 %154
  %156 = bitcast float* %155 to <16 x float>*
  %157 = load <16 x float>, <16 x float>* %156, align 64, !tbaa !373
  %158 = fcmp olt <16 x float> %157, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %159 = select <16 x i1> %158, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %157
  %160 = or i32 %153, 16
  %161 = sext i32 %160 to i64
  %162 = getelementptr inbounds float, float* %7, i64 %161
  %163 = bitcast float* %162 to <16 x float>*
  %164 = load <16 x float>, <16 x float>* %163, align 64, !tbaa !373
  %165 = fcmp ogt <16 x float> %159, %164
  %166 = select <16 x i1> %165, <16 x float> %159, <16 x float> %164
  %167 = add i32 %42, 352
  %168 = sext i32 %167 to i64
  %169 = getelementptr inbounds float, float* %7, i64 %168
  %170 = bitcast float* %169 to <16 x float>*
  %171 = load <16 x float>, <16 x float>* %170, align 64, !tbaa !373
  %172 = fcmp ogt <16 x float> %166, %171
  %173 = select <16 x i1> %172, <16 x float> %166, <16 x float> %171
  %174 = or i32 %167, 16
  %175 = sext i32 %174 to i64
  %176 = getelementptr inbounds float, float* %7, i64 %175
  %177 = bitcast float* %176 to <16 x float>*
  %178 = load <16 x float>, <16 x float>* %177, align 64, !tbaa !373
  %179 = fcmp ogt <16 x float> %173, %178
  %180 = select <16 x i1> %179, <16 x float> %173, <16 x float> %178
  store <16 x float> %180, <16 x float>* %152, align 64, !tbaa !376
  %181 = add nsw i64 %25, 80
  %182 = getelementptr inbounds float, float* %4, i64 %181
  %183 = bitcast float* %182 to <16 x float>*
  %184 = add i32 %42, 160
  %185 = sext i32 %184 to i64
  %186 = getelementptr inbounds float, float* %7, i64 %185
  %187 = bitcast float* %186 to <16 x float>*
  %188 = load <16 x float>, <16 x float>* %187, align 64, !tbaa !373
  %189 = fcmp olt <16 x float> %188, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %190 = select <16 x i1> %189, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %188
  %191 = or i32 %184, 16
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %7, i64 %192
  %194 = bitcast float* %193 to <16 x float>*
  %195 = load <16 x float>, <16 x float>* %194, align 64, !tbaa !373
  %196 = fcmp ogt <16 x float> %190, %195
  %197 = select <16 x i1> %196, <16 x float> %190, <16 x float> %195
  %198 = add i32 %42, 384
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %7, i64 %199
  %201 = bitcast float* %200 to <16 x float>*
  %202 = load <16 x float>, <16 x float>* %201, align 64, !tbaa !373
  %203 = fcmp ogt <16 x float> %197, %202
  %204 = select <16 x i1> %203, <16 x float> %197, <16 x float> %202
  %205 = or i32 %198, 16
  %206 = sext i32 %205 to i64
  %207 = getelementptr inbounds float, float* %7, i64 %206
  %208 = bitcast float* %207 to <16 x float>*
  %209 = load <16 x float>, <16 x float>* %208, align 64, !tbaa !373
  %210 = fcmp ogt <16 x float> %204, %209
  %211 = select <16 x i1> %210, <16 x float> %204, <16 x float> %209
  store <16 x float> %211, <16 x float>* %183, align 64, !tbaa !376
  %212 = add nsw i64 %25, 96
  %213 = getelementptr inbounds float, float* %4, i64 %212
  %214 = bitcast float* %213 to <16 x float>*
  %215 = add i32 %42, 192
  %216 = sext i32 %215 to i64
  %217 = getelementptr inbounds float, float* %7, i64 %216
  %218 = bitcast float* %217 to <16 x float>*
  %219 = load <16 x float>, <16 x float>* %218, align 64, !tbaa !373
  %220 = fcmp olt <16 x float> %219, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %221 = select <16 x i1> %220, <16 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <16 x float> %219
  %222 = or i32 %215, 16
  %223 = sext i32 %222 to i64
  %224 = getelementptr inbounds float, float* %7, i64 %223
  %225 = bitcast float* %224 to <16 x float>*
  %226 = load <16 x float>, <16 x float>* %225, align 64, !tbaa !373
  %227 = fcmp ogt <16 x float> %221, %226
  %228 = select <16 x i1> %227, <16 x float> %221, <16 x float> %226
  %229 = add i32 %42, 416
  %230 = sext i32 %229 to i64
  %231 = getelementptr inbounds float, float* %7, i64 %230
  %232 = bitcast float* %231 to <16 x float>*
  %233 = load <16 x float>, <16 x float>* %232, align 64, !tbaa !373
  %234 = fcmp ogt <16 x float> %228, %233
  %235 = select <16 x i1> %234, <16 x float> %228, <16 x float> %233
  %236 = or i32 %229, 16
  %237 = sext i32 %236 to i64
  %238 = getelementptr inbounds float, float* %7, i64 %237
  %239 = bitcast float* %238 to <16 x float>*
  %240 = load <16 x float>, <16 x float>* %239, align 64, !tbaa !373
  %241 = fcmp ogt <16 x float> %235, %240
  %242 = select <16 x i1> %241, <16 x float> %235, <16 x float> %240
  store <16 x float> %242, <16 x float>* %214, align 64, !tbaa !376
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %243 = add nsw i32 %24, 1
  %244 = icmp slt i64 %indvars.iv.next, %23
  br i1 %244, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
}

define dllexport i32 @fused_nn_dense_add_nn_relu(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !379 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !381, metadata !DIExpression()), !dbg !384
  call void @llvm.dbg.value(metadata i8* %1, metadata !382, metadata !DIExpression()), !dbg !384
  call void @llvm.dbg.value(metadata i32 %2, metadata !383, metadata !DIExpression()), !dbg !384
  %3 = bitcast i8* %0 to %1**, !dbg !384
  %4 = load %1*, %1** %3, align 8, !dbg !384
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !384
  %6 = bitcast i8* %5 to %1**, !dbg !384
  %7 = load %1*, %1** %6, align 8, !dbg !384
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !384
  %9 = bitcast i8* %8 to %1**, !dbg !384
  %10 = load %1*, %1** %9, align 8, !dbg !384
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !384
  %12 = bitcast i8* %11 to %1**, !dbg !384
  %13 = load %1*, %1** %12, align 8, !dbg !384
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !384
  %15 = load i8*, i8** %14, align 8, !dbg !384
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !384
  %17 = load i32, i32* %16, align 4, !dbg !384
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !384
  %19 = load i8*, i8** %18, align 8, !dbg !384
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !384
  %21 = load i8*, i8** %20, align 8, !dbg !384
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !384
  %23 = load i8*, i8** %22, align 8, !dbg !384
  %24 = tail call fastcc i32 @fused_nn_dense_add_nn_relu_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !384
  ret i32 %24, !dbg !384
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_dense_add_nn_relu_compute_(i8* noalias, i8* noalias, i8* noalias nocapture, i8* noalias nocapture readonly, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 16384, i32 2, i32 32)
  %7 = alloca %28, align 8
  %8 = getelementptr inbounds %28, %28* %7, i64 0, i32 0
  store i8* %0, i8** %8, align 8
  %9 = getelementptr inbounds %28, %28* %7, i64 0, i32 1
  store i8* %1, i8** %9, align 8
  %10 = getelementptr inbounds %28, %28* %7, i64 0, i32 2
  store i8* %6, i8** %10, align 8
  %11 = bitcast %28* %7 to i8*
  %12 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %13 = call i32 %12(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.24, i8* nonnull %11, i32 0)
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
  %wide.load = load <4 x float>, <4 x float>* %19, align 4, !tbaa !385
  %20 = getelementptr float, float* %18, i64 4
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !385
  %22 = getelementptr inbounds float, float* %16, i64 %index
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load5 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !388
  %24 = getelementptr float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load6 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !388
  %26 = fadd <4 x float> %wide.load, %wide.load5
  %27 = fadd <4 x float> %wide.load4, %wide.load6
  %28 = fcmp ogt <4 x float> %26, zeroinitializer
  %29 = fcmp ogt <4 x float> %27, zeroinitializer
  %30 = select <4 x i1> %28, <4 x float> %26, <4 x float> zeroinitializer
  %31 = select <4 x i1> %29, <4 x float> %27, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %17, i64 %index
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %30, <4 x float>* %33, align 4, !tbaa !391
  %34 = getelementptr float, float* %32, i64 4
  %35 = bitcast float* %34 to <4 x float>*
  store <4 x float> %31, <4 x float>* %35, align 4, !tbaa !391
  %index.next = add i64 %index, 8
  %36 = icmp eq i64 %index.next, 4096
  br i1 %36, label %for_end, label %vector.body, !llvm.loop !394

for_end:                                          ; preds = %vector.body
  %37 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %38 = call i32 %37(i32 1, i32 %4, i8* nonnull %6)
  br label %call_fail
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.24(i32, %0* nocapture readonly, i8* nocapture readonly) #3 {
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
  %33 = load <16 x float>, <16 x float>* %32, align 64, !tbaa !395
  %34 = add nsw i64 %30, %29
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <16 x float>*
  %37 = load <16 x float>, <16 x float>* %36, align 64, !tbaa !398
  %38 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %33, <16 x float> %37, <16 x float> %.05)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

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
  store float %55, float* %39, align 4, !tbaa !388
  %indvars.iv.next7 = add nsw i64 %indvars.iv6, 1
  %56 = icmp slt i64 %indvars.iv.next7, %26
  br i1 %56, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !401 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !403, metadata !DIExpression()), !dbg !406
  call void @llvm.dbg.value(metadata i8* %1, metadata !404, metadata !DIExpression()), !dbg !406
  call void @llvm.dbg.value(metadata i32 %2, metadata !405, metadata !DIExpression()), !dbg !406
  %3 = bitcast i8* %0 to %1**, !dbg !406
  %4 = load %1*, %1** %3, align 8, !dbg !406
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !406
  %6 = bitcast i8* %5 to %1**, !dbg !406
  %7 = load %1*, %1** %6, align 8, !dbg !406
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !406
  %9 = bitcast i8* %8 to %1**, !dbg !406
  %10 = load %1*, %1** %9, align 8, !dbg !406
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !406
  %12 = bitcast i8* %11 to %1**, !dbg !406
  %13 = load %1*, %1** %12, align 8, !dbg !406
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !406
  %15 = load i8*, i8** %14, align 8, !dbg !406
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !406
  %17 = load i32, i32* %16, align 4, !dbg !406
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !406
  %19 = load i8*, i8** %18, align 8, !dbg !406
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !406
  %21 = load i8*, i8** %20, align 8, !dbg !406
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !406
  %23 = load i8*, i8** %22, align 8, !dbg !406
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !406
  ret i32 %24, !dbg !406
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 1843200, i32 2, i32 32)
  %7 = alloca %29, align 8
  %8 = getelementptr inbounds %29, %29* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %29, %29* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %29* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.25, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %30, align 8
  %15 = getelementptr inbounds %30, %30* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %30, %30* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %30, %30* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %30, %30* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %30, %30* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %30* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.26, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.25(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 29
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 30
  %15 = select i1 %14, i32 %13, i32 30
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 30
  %18 = select i1 %17, i32 %16, i32 30
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %64, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 15360
  %.off = add i32 %20, -1
  %22 = icmp ult i32 %.off, 28
  %23 = mul nsw i32 %20, 14336
  br i1 %22, label %for_body2.us.preheader, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %for_end6.us
  %indvars.iv24 = phi i64 [ %indvars.iv.next25, %for_end6.us ], [ 0, %for_body2.us.preheader ]
  %24 = shl nsw i64 %indvars.iv24, 9
  %25 = trunc i64 %indvars.iv24 to i32
  %26 = add i32 %25, -1
  %27 = icmp ult i32 %26, 28
  br i1 %27, label %vector.body.preheader, label %vector.body37.preheader

vector.body37.preheader:                          ; preds = %for_body2.us
  br label %vector.body37

vector.body.preheader:                            ; preds = %for_body2.us
  br label %vector.body

for_end6.us:                                      ; preds = %vector.body37, %vector.body
  %indvars.iv.next25 = add nuw nsw i64 %indvars.iv24, 1
  %exitcond27 = icmp eq i64 %indvars.iv.next25, 30
  br i1 %exitcond27, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %vector.body.preheader, %vector.body
  %index = phi i64 [ %index.next, %vector.body ], [ 0, %vector.body.preheader ]
  %28 = add nuw nsw i64 %index, %24
  %29 = trunc i64 %28 to i32
  %30 = add i32 %21, %29
  %31 = trunc i64 %28 to i32
  %32 = add i32 %31, -14848
  %33 = add i32 %32, %23
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !407
  %37 = getelementptr float, float* %35, i64 4
  %38 = bitcast float* %37 to <4 x i32>*
  %wide.load36 = load <4 x i32>, <4 x i32>* %38, align 4, !tbaa !407
  %39 = sext i32 %30 to i64
  %40 = getelementptr inbounds float, float* %4, i64 %39
  %41 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %41, align 4, !tbaa !410
  %42 = getelementptr float, float* %40, i64 4
  %43 = bitcast float* %42 to <4 x i32>*
  store <4 x i32> %wide.load36, <4 x i32>* %43, align 4, !tbaa !410
  %index.next = add i64 %index, 8
  %44 = icmp eq i64 %index.next, 512
  br i1 %44, label %for_end6.us, label %vector.body, !llvm.loop !413

vector.body37:                                    ; preds = %vector.body37.preheader, %vector.body37
  %index47 = phi i64 [ %index.next48, %vector.body37 ], [ 0, %vector.body37.preheader ]
  %45 = add nuw nsw i64 %index47, %24
  %46 = trunc i64 %45 to i32
  %47 = add i32 %21, %46
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds float, float* %4, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %50, align 4, !tbaa !410
  %51 = getelementptr float, float* %49, i64 4
  %52 = bitcast float* %51 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %52, align 4, !tbaa !410
  %index.next48 = add i64 %index47, 8
  %53 = icmp eq i64 %index.next48, 512
  br i1 %53, label %for_end6.us, label %vector.body37, !llvm.loop !414

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_end6
  %indvars.iv15 = phi i64 [ %indvars.iv.next16, %for_end6 ], [ 0, %for_body2.preheader ]
  %54 = shl nsw i64 %indvars.iv15, 9
  br label %vector.body55

vector.body55:                                    ; preds = %vector.body55, %for_body2
  %index65 = phi i64 [ 0, %for_body2 ], [ %index.next66, %vector.body55 ]
  %55 = add nuw nsw i64 %index65, %54
  %56 = trunc i64 %55 to i32
  %57 = add i32 %21, %56
  %58 = sext i32 %57 to i64
  %59 = getelementptr inbounds float, float* %4, i64 %58
  %60 = bitcast float* %59 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %60, align 4, !tbaa !410
  %61 = getelementptr float, float* %59, i64 4
  %62 = bitcast float* %61 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %62, align 4, !tbaa !410
  %index.next66 = add i64 %index65, 8
  %63 = icmp eq i64 %index.next66, 512
  br i1 %63, label %for_end6, label %vector.body55, !llvm.loop !415

for_end3:                                         ; preds = %for_end6, %for_end6.us
  %64 = add nsw i32 %20, 1
  %65 = icmp slt i32 %64, %15
  br i1 %65, label %for_body, label %for_end, !prof !19

for_end6:                                         ; preds = %vector.body55
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp eq i64 %indvars.iv.next16, 30
  br i1 %exitcond17, label %for_end3, label %for_body2, !prof !20
}

define private i32 @__tvm_parallel_lambda.26(i32, %0* nocapture readonly, i8* nocapture readonly) {
entry:
  %3 = alloca [14 x <16 x float>], align 64
  %4 = bitcast [14 x <16 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0
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
  %21 = add nsw i32 %20, 895
  %22 = sdiv i32 %21, %20
  %23 = add nsw i32 %0, 1
  %24 = mul nsw i32 %22, %23
  %25 = icmp slt i32 %24, 896
  %26 = select i1 %25, i32 %24, i32 896
  %27 = mul nsw i32 %22, %0
  %28 = icmp slt i32 %27, 896
  %29 = select i1 %28, i32 %27, i32 896
  %30 = icmp slt i32 %29, %26
  br i1 %30, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %31 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 16
  %32 = bitcast float* %31 to <16 x float>*
  %33 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 32
  %34 = bitcast float* %33 to <16 x float>*
  %35 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 48
  %36 = bitcast float* %35 to <16 x float>*
  %37 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 64
  %38 = bitcast float* %37 to <16 x float>*
  %39 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 80
  %40 = bitcast float* %39 to <16 x float>*
  %41 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 96
  %42 = bitcast float* %41 to <16 x float>*
  %43 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 112
  %44 = bitcast float* %43 to <16 x float>*
  %45 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 128
  %46 = bitcast float* %45 to <16 x float>*
  %47 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 144
  %48 = bitcast float* %47 to <16 x float>*
  %49 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 160
  %50 = bitcast float* %49 to <16 x float>*
  %51 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 176
  %52 = bitcast float* %51 to <16 x float>*
  %53 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 192
  %54 = bitcast float* %53 to <16 x float>*
  %55 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 208
  %56 = bitcast float* %55 to <16 x float>*
  %57 = add i32 %29, 1
  %58 = sext i32 %57 to i64
  %59 = add nsw i64 %58, -1
  %60 = sext i32 %26 to i64
  %61 = bitcast [14 x <16 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end6.1
  %indvars.iv99 = phi i64 [ %59, %for_body.lr.ph ], [ %indvars.iv.next100, %for_end6.1 ]
  %62 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %63 = tail call i8* %62(i32 1, i32 %18, i64 1792, i32 2, i32 32)
  %64 = trunc i64 %indvars.iv99 to i32
  %65 = srem i32 %64, 28
  %66 = sdiv i32 %64, 28
  %67 = mul nsw i32 %66, 73728
  %68 = sext i32 %67 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull %61, i8 0, i64 896, i32 64, i1 false)
  br label %for_body5

for_end:                                          ; preds = %for_end6.1, %entry
  ret i32 0

for_body5:                                        ; preds = %for_end9, %for_body
  %indvars.iv86 = phi i64 [ 0, %for_body ], [ %indvars.iv.next87, %for_end9 ]
  %.lcssa4572 = phi <16 x float> [ zeroinitializer, %for_body ], [ %223, %for_end9 ]
  %.lcssa4370 = phi <16 x float> [ zeroinitializer, %for_body ], [ %217, %for_end9 ]
  %.lcssa4168 = phi <16 x float> [ zeroinitializer, %for_body ], [ %216, %for_end9 ]
  %.lcssa3966 = phi <16 x float> [ zeroinitializer, %for_body ], [ %215, %for_end9 ]
  %.lcssa3764 = phi <16 x float> [ zeroinitializer, %for_body ], [ %214, %for_end9 ]
  %.lcssa3562 = phi <16 x float> [ zeroinitializer, %for_body ], [ %213, %for_end9 ]
  %.lcssa3360 = phi <16 x float> [ zeroinitializer, %for_body ], [ %212, %for_end9 ]
  %.lcssa3158 = phi <16 x float> [ zeroinitializer, %for_body ], [ %211, %for_end9 ]
  %.lcssa2956 = phi <16 x float> [ zeroinitializer, %for_body ], [ %210, %for_end9 ]
  %.lcssa2754 = phi <16 x float> [ zeroinitializer, %for_body ], [ %209, %for_end9 ]
  %.lcssa2552 = phi <16 x float> [ zeroinitializer, %for_body ], [ %208, %for_end9 ]
  %.lcssa2350 = phi <16 x float> [ zeroinitializer, %for_body ], [ %207, %for_end9 ]
  %.lcssa2149 = phi <16 x float> [ zeroinitializer, %for_body ], [ %206, %for_end9 ]
  %.lcssa47 = phi <16 x float> [ zeroinitializer, %for_body ], [ %205, %for_end9 ]
  %69 = phi i32 [ 0, %for_body ], [ %224, %for_end9 ]
  %70 = add nsw i32 %69, %65
  %71 = mul i32 %70, 15360
  %72 = mul nuw nsw i64 %indvars.iv86, 24576
  %73 = add nsw i64 %72, %68
  %74 = sext i32 %71 to i64
  br label %for_body8

for_end6:                                         ; preds = %for_end9
  store <16 x float> %205, <16 x float>* %.sub, align 64, !tbaa !416
  store <16 x float> %206, <16 x float>* %32, align 64, !tbaa !416
  store <16 x float> %207, <16 x float>* %34, align 64, !tbaa !416
  store <16 x float> %208, <16 x float>* %36, align 64, !tbaa !416
  store <16 x float> %209, <16 x float>* %38, align 64, !tbaa !416
  store <16 x float> %210, <16 x float>* %40, align 64, !tbaa !416
  store <16 x float> %211, <16 x float>* %42, align 64, !tbaa !416
  store <16 x float> %212, <16 x float>* %44, align 64, !tbaa !416
  store <16 x float> %213, <16 x float>* %46, align 64, !tbaa !416
  store <16 x float> %214, <16 x float>* %48, align 64, !tbaa !416
  store <16 x float> %215, <16 x float>* %50, align 64, !tbaa !416
  store <16 x float> %216, <16 x float>* %52, align 64, !tbaa !416
  store <16 x float> %217, <16 x float>* %54, align 64, !tbaa !416
  store <16 x float> %223, <16 x float>* %56, align 64, !tbaa !416
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %63, i8* nonnull %4, i64 896, i32 64, i1 false)
  %scevgep.1 = getelementptr i8, i8* %63, i64 896
  call void @llvm.memset.p0i8.i64(i8* nonnull %61, i8 0, i64 896, i32 64, i1 false)
  br label %for_body5.1

for_body8:                                        ; preds = %for_body8, %for_body5
  %indvars.iv = phi i64 [ 0, %for_body5 ], [ %indvars.iv.next, %for_body8 ]
  %75 = phi <16 x float> [ %.lcssa4572, %for_body5 ], [ %223, %for_body8 ]
  %76 = phi <16 x float> [ %.lcssa4370, %for_body5 ], [ %217, %for_body8 ]
  %77 = phi <16 x float> [ %.lcssa4168, %for_body5 ], [ %216, %for_body8 ]
  %78 = phi <16 x float> [ %.lcssa3966, %for_body5 ], [ %215, %for_body8 ]
  %79 = phi <16 x float> [ %.lcssa3764, %for_body5 ], [ %214, %for_body8 ]
  %80 = phi <16 x float> [ %.lcssa3562, %for_body5 ], [ %213, %for_body8 ]
  %81 = phi <16 x float> [ %.lcssa3360, %for_body5 ], [ %212, %for_body8 ]
  %82 = phi <16 x float> [ %.lcssa3158, %for_body5 ], [ %211, %for_body8 ]
  %83 = phi <16 x float> [ %.lcssa2956, %for_body5 ], [ %210, %for_body8 ]
  %84 = phi <16 x float> [ %.lcssa2754, %for_body5 ], [ %209, %for_body8 ]
  %85 = phi <16 x float> [ %.lcssa2552, %for_body5 ], [ %208, %for_body8 ]
  %86 = phi <16 x float> [ %.lcssa2350, %for_body5 ], [ %207, %for_body8 ]
  %87 = phi <16 x float> [ %.lcssa2149, %for_body5 ], [ %206, %for_body8 ]
  %88 = phi <16 x float> [ %.lcssa47, %for_body5 ], [ %205, %for_body8 ]
  %89 = add nsw i64 %indvars.iv, %74
  %90 = getelementptr inbounds float, float* %6, i64 %89
  %91 = load float, float* %90, align 4, !tbaa !410
  %92 = insertelement <16 x float> undef, float %91, i32 0
  %93 = shufflevector <16 x float> %92, <16 x float> undef, <16 x i32> zeroinitializer
  %94 = shl nsw i64 %indvars.iv, 4
  %95 = add nsw i64 %73, %94
  %96 = getelementptr inbounds float, float* %9, i64 %95
  %97 = bitcast float* %96 to <16 x float>*
  %98 = load <16 x float>, <16 x float>* %97, align 64, !tbaa !426
  %99 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %93, <16 x float> %98, <16 x float> %88)
  %100 = add nsw i64 %89, 512
  %101 = getelementptr inbounds float, float* %6, i64 %100
  %102 = load float, float* %101, align 4, !tbaa !410
  %103 = insertelement <16 x float> undef, float %102, i32 0
  %104 = shufflevector <16 x float> %103, <16 x float> undef, <16 x i32> zeroinitializer
  %105 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %104, <16 x float> %98, <16 x float> %87)
  %106 = add nsw i64 %89, 1024
  %107 = getelementptr inbounds float, float* %6, i64 %106
  %108 = load float, float* %107, align 4, !tbaa !410
  %109 = insertelement <16 x float> undef, float %108, i32 0
  %110 = shufflevector <16 x float> %109, <16 x float> undef, <16 x i32> zeroinitializer
  %111 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %110, <16 x float> %98, <16 x float> %86)
  %112 = add nsw i64 %89, 1536
  %113 = getelementptr inbounds float, float* %6, i64 %112
  %114 = load float, float* %113, align 4, !tbaa !410
  %115 = insertelement <16 x float> undef, float %114, i32 0
  %116 = shufflevector <16 x float> %115, <16 x float> undef, <16 x i32> zeroinitializer
  %117 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %116, <16 x float> %98, <16 x float> %85)
  %118 = add nsw i64 %89, 2048
  %119 = getelementptr inbounds float, float* %6, i64 %118
  %120 = load float, float* %119, align 4, !tbaa !410
  %121 = insertelement <16 x float> undef, float %120, i32 0
  %122 = shufflevector <16 x float> %121, <16 x float> undef, <16 x i32> zeroinitializer
  %123 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %122, <16 x float> %98, <16 x float> %84)
  %124 = add nsw i64 %89, 2560
  %125 = getelementptr inbounds float, float* %6, i64 %124
  %126 = load float, float* %125, align 4, !tbaa !410
  %127 = insertelement <16 x float> undef, float %126, i32 0
  %128 = shufflevector <16 x float> %127, <16 x float> undef, <16 x i32> zeroinitializer
  %129 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %128, <16 x float> %98, <16 x float> %83)
  %130 = add nsw i64 %89, 3072
  %131 = getelementptr inbounds float, float* %6, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !410
  %133 = insertelement <16 x float> undef, float %132, i32 0
  %134 = shufflevector <16 x float> %133, <16 x float> undef, <16 x i32> zeroinitializer
  %135 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %134, <16 x float> %98, <16 x float> %82)
  %136 = add nsw i64 %89, 3584
  %137 = getelementptr inbounds float, float* %6, i64 %136
  %138 = load float, float* %137, align 4, !tbaa !410
  %139 = insertelement <16 x float> undef, float %138, i32 0
  %140 = shufflevector <16 x float> %139, <16 x float> undef, <16 x i32> zeroinitializer
  %141 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %140, <16 x float> %98, <16 x float> %81)
  %142 = add nsw i64 %89, 4096
  %143 = getelementptr inbounds float, float* %6, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !410
  %145 = insertelement <16 x float> undef, float %144, i32 0
  %146 = shufflevector <16 x float> %145, <16 x float> undef, <16 x i32> zeroinitializer
  %147 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %146, <16 x float> %98, <16 x float> %80)
  %148 = add nsw i64 %89, 4608
  %149 = getelementptr inbounds float, float* %6, i64 %148
  %150 = load float, float* %149, align 4, !tbaa !410
  %151 = insertelement <16 x float> undef, float %150, i32 0
  %152 = shufflevector <16 x float> %151, <16 x float> undef, <16 x i32> zeroinitializer
  %153 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %152, <16 x float> %98, <16 x float> %79)
  %154 = add nsw i64 %89, 5120
  %155 = getelementptr inbounds float, float* %6, i64 %154
  %156 = load float, float* %155, align 4, !tbaa !410
  %157 = insertelement <16 x float> undef, float %156, i32 0
  %158 = shufflevector <16 x float> %157, <16 x float> undef, <16 x i32> zeroinitializer
  %159 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %158, <16 x float> %98, <16 x float> %78)
  %160 = add nsw i64 %89, 5632
  %161 = getelementptr inbounds float, float* %6, i64 %160
  %162 = load float, float* %161, align 4, !tbaa !410
  %163 = insertelement <16 x float> undef, float %162, i32 0
  %164 = shufflevector <16 x float> %163, <16 x float> undef, <16 x i32> zeroinitializer
  %165 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %164, <16 x float> %98, <16 x float> %77)
  %166 = add nsw i64 %89, 6144
  %167 = getelementptr inbounds float, float* %6, i64 %166
  %168 = load float, float* %167, align 4, !tbaa !410
  %169 = insertelement <16 x float> undef, float %168, i32 0
  %170 = shufflevector <16 x float> %169, <16 x float> undef, <16 x i32> zeroinitializer
  %171 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %170, <16 x float> %98, <16 x float> %76)
  %172 = add nsw i64 %89, 6656
  %173 = getelementptr inbounds float, float* %6, i64 %172
  %174 = load float, float* %173, align 4, !tbaa !410
  %175 = insertelement <16 x float> undef, float %174, i32 0
  %176 = shufflevector <16 x float> %175, <16 x float> undef, <16 x i32> zeroinitializer
  %177 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %98, <16 x float> %75)
  %178 = add nsw i64 %95, 8192
  %179 = getelementptr inbounds float, float* %9, i64 %178
  %180 = bitcast float* %179 to <16 x float>*
  %181 = load <16 x float>, <16 x float>* %180, align 64, !tbaa !426
  %182 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %104, <16 x float> %181, <16 x float> %99)
  %183 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %110, <16 x float> %181, <16 x float> %105)
  %184 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %116, <16 x float> %181, <16 x float> %111)
  %185 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %122, <16 x float> %181, <16 x float> %117)
  %186 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %128, <16 x float> %181, <16 x float> %123)
  %187 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %134, <16 x float> %181, <16 x float> %129)
  %188 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %140, <16 x float> %181, <16 x float> %135)
  %189 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %146, <16 x float> %181, <16 x float> %141)
  %190 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %152, <16 x float> %181, <16 x float> %147)
  %191 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %158, <16 x float> %181, <16 x float> %153)
  %192 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %164, <16 x float> %181, <16 x float> %159)
  %193 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %170, <16 x float> %181, <16 x float> %165)
  %194 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %181, <16 x float> %171)
  %195 = add nsw i64 %89, 7168
  %196 = getelementptr inbounds float, float* %6, i64 %195
  %197 = load float, float* %196, align 4, !tbaa !410
  %198 = insertelement <16 x float> undef, float %197, i32 0
  %199 = shufflevector <16 x float> %198, <16 x float> undef, <16 x i32> zeroinitializer
  %200 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %199, <16 x float> %181, <16 x float> %177)
  %201 = add nsw i64 %95, 16384
  %202 = getelementptr inbounds float, float* %9, i64 %201
  %203 = bitcast float* %202 to <16 x float>*
  %204 = load <16 x float>, <16 x float>* %203, align 64, !tbaa !426
  %205 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %110, <16 x float> %204, <16 x float> %182)
  %206 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %116, <16 x float> %204, <16 x float> %183)
  %207 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %122, <16 x float> %204, <16 x float> %184)
  %208 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %128, <16 x float> %204, <16 x float> %185)
  %209 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %134, <16 x float> %204, <16 x float> %186)
  %210 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %140, <16 x float> %204, <16 x float> %187)
  %211 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %146, <16 x float> %204, <16 x float> %188)
  %212 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %152, <16 x float> %204, <16 x float> %189)
  %213 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %158, <16 x float> %204, <16 x float> %190)
  %214 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %164, <16 x float> %204, <16 x float> %191)
  %215 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %170, <16 x float> %204, <16 x float> %192)
  %216 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %204, <16 x float> %193)
  %217 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %199, <16 x float> %204, <16 x float> %194)
  %218 = add nsw i64 %89, 7680
  %219 = getelementptr inbounds float, float* %6, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !410
  %221 = insertelement <16 x float> undef, float %220, i32 0
  %222 = shufflevector <16 x float> %221, <16 x float> undef, <16 x i32> zeroinitializer
  %223 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %222, <16 x float> %204, <16 x float> %200)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end9, label %for_body8, !prof !20

for_end9:                                         ; preds = %for_body8
  %indvars.iv.next87 = add nuw nsw i64 %indvars.iv86, 1
  %224 = add nuw nsw i32 %69, 1
  %exitcond88 = icmp eq i64 %indvars.iv.next87, 3
  br i1 %exitcond88, label %for_end6, label %for_body5, !prof !20

for_body5.1:                                      ; preds = %for_end9.1, %for_end6
  %indvars.iv86.1 = phi i64 [ 0, %for_end6 ], [ %indvars.iv.next87.1, %for_end9.1 ]
  %.lcssa4572.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %380, %for_end9.1 ]
  %.lcssa4370.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %374, %for_end9.1 ]
  %.lcssa4168.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %373, %for_end9.1 ]
  %.lcssa3966.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %372, %for_end9.1 ]
  %.lcssa3764.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %371, %for_end9.1 ]
  %.lcssa3562.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %370, %for_end9.1 ]
  %.lcssa3360.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %369, %for_end9.1 ]
  %.lcssa3158.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %368, %for_end9.1 ]
  %.lcssa2956.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %367, %for_end9.1 ]
  %.lcssa2754.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %366, %for_end9.1 ]
  %.lcssa2552.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %365, %for_end9.1 ]
  %.lcssa2350.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %364, %for_end9.1 ]
  %.lcssa2149.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %363, %for_end9.1 ]
  %.lcssa47.1 = phi <16 x float> [ zeroinitializer, %for_end6 ], [ %362, %for_end9.1 ]
  %225 = phi i32 [ 0, %for_end6 ], [ %381, %for_end9.1 ]
  %226 = add nsw i32 %225, %65
  %227 = mul i32 %226, 15360
  %228 = add nsw i32 %227, 7168
  %229 = mul nuw nsw i64 %indvars.iv86.1, 24576
  %230 = add nsw i64 %229, %68
  %231 = sext i32 %228 to i64
  br label %for_body8.1

for_body8.1:                                      ; preds = %for_body8.1, %for_body5.1
  %indvars.iv.1 = phi i64 [ 0, %for_body5.1 ], [ %indvars.iv.next.1, %for_body8.1 ]
  %232 = phi <16 x float> [ %.lcssa4572.1, %for_body5.1 ], [ %380, %for_body8.1 ]
  %233 = phi <16 x float> [ %.lcssa4370.1, %for_body5.1 ], [ %374, %for_body8.1 ]
  %234 = phi <16 x float> [ %.lcssa4168.1, %for_body5.1 ], [ %373, %for_body8.1 ]
  %235 = phi <16 x float> [ %.lcssa3966.1, %for_body5.1 ], [ %372, %for_body8.1 ]
  %236 = phi <16 x float> [ %.lcssa3764.1, %for_body5.1 ], [ %371, %for_body8.1 ]
  %237 = phi <16 x float> [ %.lcssa3562.1, %for_body5.1 ], [ %370, %for_body8.1 ]
  %238 = phi <16 x float> [ %.lcssa3360.1, %for_body5.1 ], [ %369, %for_body8.1 ]
  %239 = phi <16 x float> [ %.lcssa3158.1, %for_body5.1 ], [ %368, %for_body8.1 ]
  %240 = phi <16 x float> [ %.lcssa2956.1, %for_body5.1 ], [ %367, %for_body8.1 ]
  %241 = phi <16 x float> [ %.lcssa2754.1, %for_body5.1 ], [ %366, %for_body8.1 ]
  %242 = phi <16 x float> [ %.lcssa2552.1, %for_body5.1 ], [ %365, %for_body8.1 ]
  %243 = phi <16 x float> [ %.lcssa2350.1, %for_body5.1 ], [ %364, %for_body8.1 ]
  %244 = phi <16 x float> [ %.lcssa2149.1, %for_body5.1 ], [ %363, %for_body8.1 ]
  %245 = phi <16 x float> [ %.lcssa47.1, %for_body5.1 ], [ %362, %for_body8.1 ]
  %246 = add nsw i64 %indvars.iv.1, %231
  %247 = getelementptr inbounds float, float* %6, i64 %246
  %248 = load float, float* %247, align 4, !tbaa !410
  %249 = insertelement <16 x float> undef, float %248, i32 0
  %250 = shufflevector <16 x float> %249, <16 x float> undef, <16 x i32> zeroinitializer
  %251 = shl nsw i64 %indvars.iv.1, 4
  %252 = add nsw i64 %230, %251
  %253 = getelementptr inbounds float, float* %9, i64 %252
  %254 = bitcast float* %253 to <16 x float>*
  %255 = load <16 x float>, <16 x float>* %254, align 64, !tbaa !426
  %256 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %250, <16 x float> %255, <16 x float> %245)
  %257 = add nsw i64 %246, 512
  %258 = getelementptr inbounds float, float* %6, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !410
  %260 = insertelement <16 x float> undef, float %259, i32 0
  %261 = shufflevector <16 x float> %260, <16 x float> undef, <16 x i32> zeroinitializer
  %262 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %261, <16 x float> %255, <16 x float> %244)
  %263 = add nsw i64 %246, 1024
  %264 = getelementptr inbounds float, float* %6, i64 %263
  %265 = load float, float* %264, align 4, !tbaa !410
  %266 = insertelement <16 x float> undef, float %265, i32 0
  %267 = shufflevector <16 x float> %266, <16 x float> undef, <16 x i32> zeroinitializer
  %268 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %267, <16 x float> %255, <16 x float> %243)
  %269 = add nsw i64 %246, 1536
  %270 = getelementptr inbounds float, float* %6, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !410
  %272 = insertelement <16 x float> undef, float %271, i32 0
  %273 = shufflevector <16 x float> %272, <16 x float> undef, <16 x i32> zeroinitializer
  %274 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %273, <16 x float> %255, <16 x float> %242)
  %275 = add nsw i64 %246, 2048
  %276 = getelementptr inbounds float, float* %6, i64 %275
  %277 = load float, float* %276, align 4, !tbaa !410
  %278 = insertelement <16 x float> undef, float %277, i32 0
  %279 = shufflevector <16 x float> %278, <16 x float> undef, <16 x i32> zeroinitializer
  %280 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %279, <16 x float> %255, <16 x float> %241)
  %281 = add nsw i64 %246, 2560
  %282 = getelementptr inbounds float, float* %6, i64 %281
  %283 = load float, float* %282, align 4, !tbaa !410
  %284 = insertelement <16 x float> undef, float %283, i32 0
  %285 = shufflevector <16 x float> %284, <16 x float> undef, <16 x i32> zeroinitializer
  %286 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %285, <16 x float> %255, <16 x float> %240)
  %287 = add nsw i64 %246, 3072
  %288 = getelementptr inbounds float, float* %6, i64 %287
  %289 = load float, float* %288, align 4, !tbaa !410
  %290 = insertelement <16 x float> undef, float %289, i32 0
  %291 = shufflevector <16 x float> %290, <16 x float> undef, <16 x i32> zeroinitializer
  %292 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %291, <16 x float> %255, <16 x float> %239)
  %293 = add nsw i64 %246, 3584
  %294 = getelementptr inbounds float, float* %6, i64 %293
  %295 = load float, float* %294, align 4, !tbaa !410
  %296 = insertelement <16 x float> undef, float %295, i32 0
  %297 = shufflevector <16 x float> %296, <16 x float> undef, <16 x i32> zeroinitializer
  %298 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %297, <16 x float> %255, <16 x float> %238)
  %299 = add nsw i64 %246, 4096
  %300 = getelementptr inbounds float, float* %6, i64 %299
  %301 = load float, float* %300, align 4, !tbaa !410
  %302 = insertelement <16 x float> undef, float %301, i32 0
  %303 = shufflevector <16 x float> %302, <16 x float> undef, <16 x i32> zeroinitializer
  %304 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %303, <16 x float> %255, <16 x float> %237)
  %305 = add nsw i64 %246, 4608
  %306 = getelementptr inbounds float, float* %6, i64 %305
  %307 = load float, float* %306, align 4, !tbaa !410
  %308 = insertelement <16 x float> undef, float %307, i32 0
  %309 = shufflevector <16 x float> %308, <16 x float> undef, <16 x i32> zeroinitializer
  %310 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %309, <16 x float> %255, <16 x float> %236)
  %311 = add nsw i64 %246, 5120
  %312 = getelementptr inbounds float, float* %6, i64 %311
  %313 = load float, float* %312, align 4, !tbaa !410
  %314 = insertelement <16 x float> undef, float %313, i32 0
  %315 = shufflevector <16 x float> %314, <16 x float> undef, <16 x i32> zeroinitializer
  %316 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %315, <16 x float> %255, <16 x float> %235)
  %317 = add nsw i64 %246, 5632
  %318 = getelementptr inbounds float, float* %6, i64 %317
  %319 = load float, float* %318, align 4, !tbaa !410
  %320 = insertelement <16 x float> undef, float %319, i32 0
  %321 = shufflevector <16 x float> %320, <16 x float> undef, <16 x i32> zeroinitializer
  %322 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %321, <16 x float> %255, <16 x float> %234)
  %323 = add nsw i64 %246, 6144
  %324 = getelementptr inbounds float, float* %6, i64 %323
  %325 = load float, float* %324, align 4, !tbaa !410
  %326 = insertelement <16 x float> undef, float %325, i32 0
  %327 = shufflevector <16 x float> %326, <16 x float> undef, <16 x i32> zeroinitializer
  %328 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %327, <16 x float> %255, <16 x float> %233)
  %329 = add nsw i64 %246, 6656
  %330 = getelementptr inbounds float, float* %6, i64 %329
  %331 = load float, float* %330, align 4, !tbaa !410
  %332 = insertelement <16 x float> undef, float %331, i32 0
  %333 = shufflevector <16 x float> %332, <16 x float> undef, <16 x i32> zeroinitializer
  %334 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %333, <16 x float> %255, <16 x float> %232)
  %335 = add nsw i64 %252, 8192
  %336 = getelementptr inbounds float, float* %9, i64 %335
  %337 = bitcast float* %336 to <16 x float>*
  %338 = load <16 x float>, <16 x float>* %337, align 64, !tbaa !426
  %339 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %261, <16 x float> %338, <16 x float> %256)
  %340 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %267, <16 x float> %338, <16 x float> %262)
  %341 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %273, <16 x float> %338, <16 x float> %268)
  %342 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %279, <16 x float> %338, <16 x float> %274)
  %343 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %285, <16 x float> %338, <16 x float> %280)
  %344 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %291, <16 x float> %338, <16 x float> %286)
  %345 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %297, <16 x float> %338, <16 x float> %292)
  %346 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %303, <16 x float> %338, <16 x float> %298)
  %347 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %309, <16 x float> %338, <16 x float> %304)
  %348 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %315, <16 x float> %338, <16 x float> %310)
  %349 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %321, <16 x float> %338, <16 x float> %316)
  %350 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %327, <16 x float> %338, <16 x float> %322)
  %351 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %333, <16 x float> %338, <16 x float> %328)
  %352 = add nsw i64 %246, 7168
  %353 = getelementptr inbounds float, float* %6, i64 %352
  %354 = load float, float* %353, align 4, !tbaa !410
  %355 = insertelement <16 x float> undef, float %354, i32 0
  %356 = shufflevector <16 x float> %355, <16 x float> undef, <16 x i32> zeroinitializer
  %357 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %356, <16 x float> %338, <16 x float> %334)
  %358 = add nsw i64 %252, 16384
  %359 = getelementptr inbounds float, float* %9, i64 %358
  %360 = bitcast float* %359 to <16 x float>*
  %361 = load <16 x float>, <16 x float>* %360, align 64, !tbaa !426
  %362 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %267, <16 x float> %361, <16 x float> %339)
  %363 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %273, <16 x float> %361, <16 x float> %340)
  %364 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %279, <16 x float> %361, <16 x float> %341)
  %365 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %285, <16 x float> %361, <16 x float> %342)
  %366 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %291, <16 x float> %361, <16 x float> %343)
  %367 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %297, <16 x float> %361, <16 x float> %344)
  %368 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %303, <16 x float> %361, <16 x float> %345)
  %369 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %309, <16 x float> %361, <16 x float> %346)
  %370 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %315, <16 x float> %361, <16 x float> %347)
  %371 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %321, <16 x float> %361, <16 x float> %348)
  %372 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %327, <16 x float> %361, <16 x float> %349)
  %373 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %333, <16 x float> %361, <16 x float> %350)
  %374 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %356, <16 x float> %361, <16 x float> %351)
  %375 = add nsw i64 %246, 7680
  %376 = getelementptr inbounds float, float* %6, i64 %375
  %377 = load float, float* %376, align 4, !tbaa !410
  %378 = insertelement <16 x float> undef, float %377, i32 0
  %379 = shufflevector <16 x float> %378, <16 x float> undef, <16 x i32> zeroinitializer
  %380 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %379, <16 x float> %361, <16 x float> %357)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 512
  br i1 %exitcond.1, label %for_end9.1, label %for_body8.1, !prof !20

for_end9.1:                                       ; preds = %for_body8.1
  %indvars.iv.next87.1 = add nuw nsw i64 %indvars.iv86.1, 1
  %381 = add nuw nsw i32 %225, 1
  %exitcond88.1 = icmp eq i64 %indvars.iv.next87.1, 3
  br i1 %exitcond88.1, label %for_end6.1, label %for_body5.1, !prof !20

for_end6.1:                                       ; preds = %for_end9.1
  store <16 x float> %362, <16 x float>* %.sub, align 64, !tbaa !416
  store <16 x float> %363, <16 x float>* %32, align 64, !tbaa !416
  store <16 x float> %364, <16 x float>* %34, align 64, !tbaa !416
  store <16 x float> %365, <16 x float>* %36, align 64, !tbaa !416
  store <16 x float> %366, <16 x float>* %38, align 64, !tbaa !416
  store <16 x float> %367, <16 x float>* %40, align 64, !tbaa !416
  store <16 x float> %368, <16 x float>* %42, align 64, !tbaa !416
  store <16 x float> %369, <16 x float>* %44, align 64, !tbaa !416
  store <16 x float> %370, <16 x float>* %46, align 64, !tbaa !416
  store <16 x float> %371, <16 x float>* %48, align 64, !tbaa !416
  store <16 x float> %372, <16 x float>* %50, align 64, !tbaa !416
  store <16 x float> %373, <16 x float>* %52, align 64, !tbaa !416
  store <16 x float> %374, <16 x float>* %54, align 64, !tbaa !416
  store <16 x float> %380, <16 x float>* %56, align 64, !tbaa !416
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep.1, i8* nonnull %4, i64 896, i32 64, i1 false)
  %382 = mul nsw i64 %indvars.iv99, 448
  %383 = shl nsw i32 %66, 4
  %384 = sext i32 %383 to i64
  %385 = getelementptr inbounds float, float* %15, i64 %384
  %386 = bitcast float* %385 to <16 x float>*
  %387 = load <16 x float>, <16 x float>* %386, align 64, !tbaa !429
  %388 = bitcast i8* %63 to <16 x float>*
  %389 = load <16 x float>, <16 x float>* %388, align 64, !tbaa !432
  %390 = fadd <16 x float> %387, %389
  %391 = fcmp ogt <16 x float> %390, zeroinitializer
  %392 = select <16 x i1> %391, <16 x float> %390, <16 x float> zeroinitializer
  %393 = getelementptr inbounds float, float* %12, i64 %382
  %394 = bitcast float* %393 to <16 x float>*
  store <16 x float> %392, <16 x float>* %394, align 64, !tbaa !435
  %395 = getelementptr inbounds i8, i8* %63, i64 64
  %396 = bitcast i8* %395 to <16 x float>*
  %397 = load <16 x float>, <16 x float>* %396, align 64, !tbaa !432
  %398 = fadd <16 x float> %387, %397
  %399 = fcmp ogt <16 x float> %398, zeroinitializer
  %400 = select <16 x i1> %399, <16 x float> %398, <16 x float> zeroinitializer
  %401 = mul i64 %indvars.iv99, 1924145348608
  %sext = ashr exact i64 %401, 32
  %402 = or i64 %sext, 16
  %403 = getelementptr inbounds float, float* %12, i64 %402
  %404 = bitcast float* %403 to <16 x float>*
  store <16 x float> %400, <16 x float>* %404, align 64, !tbaa !435
  %405 = getelementptr inbounds i8, i8* %63, i64 128
  %406 = bitcast i8* %405 to <16 x float>*
  %407 = load <16 x float>, <16 x float>* %406, align 64, !tbaa !432
  %408 = fadd <16 x float> %387, %407
  %409 = fcmp ogt <16 x float> %408, zeroinitializer
  %410 = select <16 x i1> %409, <16 x float> %408, <16 x float> zeroinitializer
  %411 = mul i64 %indvars.iv99, 1924145348608
  %sext101 = ashr exact i64 %411, 32
  %412 = or i64 %sext101, 32
  %413 = getelementptr inbounds float, float* %12, i64 %412
  %414 = bitcast float* %413 to <16 x float>*
  store <16 x float> %410, <16 x float>* %414, align 64, !tbaa !435
  %415 = getelementptr inbounds i8, i8* %63, i64 192
  %416 = bitcast i8* %415 to <16 x float>*
  %417 = load <16 x float>, <16 x float>* %416, align 64, !tbaa !432
  %418 = fadd <16 x float> %387, %417
  %419 = fcmp ogt <16 x float> %418, zeroinitializer
  %420 = select <16 x i1> %419, <16 x float> %418, <16 x float> zeroinitializer
  %421 = mul i64 %indvars.iv99, 1924145348608
  %sext102 = ashr exact i64 %421, 32
  %422 = or i64 %sext102, 48
  %423 = getelementptr inbounds float, float* %12, i64 %422
  %424 = bitcast float* %423 to <16 x float>*
  store <16 x float> %420, <16 x float>* %424, align 64, !tbaa !435
  %425 = getelementptr inbounds i8, i8* %63, i64 256
  %426 = bitcast i8* %425 to <16 x float>*
  %427 = load <16 x float>, <16 x float>* %426, align 64, !tbaa !432
  %428 = fadd <16 x float> %387, %427
  %429 = fcmp ogt <16 x float> %428, zeroinitializer
  %430 = select <16 x i1> %429, <16 x float> %428, <16 x float> zeroinitializer
  %431 = mul i64 %indvars.iv99, 1924145348608
  %sext103 = add i64 %431, 274877906944
  %432 = ashr exact i64 %sext103, 32
  %433 = getelementptr inbounds float, float* %12, i64 %432
  %434 = bitcast float* %433 to <16 x float>*
  store <16 x float> %430, <16 x float>* %434, align 64, !tbaa !435
  %435 = getelementptr inbounds i8, i8* %63, i64 320
  %436 = bitcast i8* %435 to <16 x float>*
  %437 = load <16 x float>, <16 x float>* %436, align 64, !tbaa !432
  %438 = fadd <16 x float> %387, %437
  %439 = fcmp ogt <16 x float> %438, zeroinitializer
  %440 = select <16 x i1> %439, <16 x float> %438, <16 x float> zeroinitializer
  %441 = mul i64 %indvars.iv99, 1924145348608
  %sext104 = add i64 %441, 343597383680
  %442 = ashr exact i64 %sext104, 32
  %443 = getelementptr inbounds float, float* %12, i64 %442
  %444 = bitcast float* %443 to <16 x float>*
  store <16 x float> %440, <16 x float>* %444, align 64, !tbaa !435
  %445 = getelementptr inbounds i8, i8* %63, i64 384
  %446 = bitcast i8* %445 to <16 x float>*
  %447 = load <16 x float>, <16 x float>* %446, align 64, !tbaa !432
  %448 = fadd <16 x float> %387, %447
  %449 = fcmp ogt <16 x float> %448, zeroinitializer
  %450 = select <16 x i1> %449, <16 x float> %448, <16 x float> zeroinitializer
  %451 = mul i64 %indvars.iv99, 1924145348608
  %sext105 = add i64 %451, 412316860416
  %452 = ashr exact i64 %sext105, 32
  %453 = getelementptr inbounds float, float* %12, i64 %452
  %454 = bitcast float* %453 to <16 x float>*
  store <16 x float> %450, <16 x float>* %454, align 64, !tbaa !435
  %455 = getelementptr inbounds i8, i8* %63, i64 448
  %456 = bitcast i8* %455 to <16 x float>*
  %457 = load <16 x float>, <16 x float>* %456, align 64, !tbaa !432
  %458 = fadd <16 x float> %387, %457
  %459 = fcmp ogt <16 x float> %458, zeroinitializer
  %460 = select <16 x i1> %459, <16 x float> %458, <16 x float> zeroinitializer
  %461 = mul i64 %indvars.iv99, 1924145348608
  %sext106 = add i64 %461, 481036337152
  %462 = ashr exact i64 %sext106, 32
  %463 = getelementptr inbounds float, float* %12, i64 %462
  %464 = bitcast float* %463 to <16 x float>*
  store <16 x float> %460, <16 x float>* %464, align 64, !tbaa !435
  %465 = getelementptr inbounds i8, i8* %63, i64 512
  %466 = bitcast i8* %465 to <16 x float>*
  %467 = load <16 x float>, <16 x float>* %466, align 64, !tbaa !432
  %468 = fadd <16 x float> %387, %467
  %469 = fcmp ogt <16 x float> %468, zeroinitializer
  %470 = select <16 x i1> %469, <16 x float> %468, <16 x float> zeroinitializer
  %471 = mul i64 %indvars.iv99, 1924145348608
  %sext107 = add i64 %471, 549755813888
  %472 = ashr exact i64 %sext107, 32
  %473 = getelementptr inbounds float, float* %12, i64 %472
  %474 = bitcast float* %473 to <16 x float>*
  store <16 x float> %470, <16 x float>* %474, align 64, !tbaa !435
  %475 = getelementptr inbounds i8, i8* %63, i64 576
  %476 = bitcast i8* %475 to <16 x float>*
  %477 = load <16 x float>, <16 x float>* %476, align 64, !tbaa !432
  %478 = fadd <16 x float> %387, %477
  %479 = fcmp ogt <16 x float> %478, zeroinitializer
  %480 = select <16 x i1> %479, <16 x float> %478, <16 x float> zeroinitializer
  %481 = mul i64 %indvars.iv99, 1924145348608
  %sext108 = add i64 %481, 618475290624
  %482 = ashr exact i64 %sext108, 32
  %483 = getelementptr inbounds float, float* %12, i64 %482
  %484 = bitcast float* %483 to <16 x float>*
  store <16 x float> %480, <16 x float>* %484, align 64, !tbaa !435
  %485 = getelementptr inbounds i8, i8* %63, i64 640
  %486 = bitcast i8* %485 to <16 x float>*
  %487 = load <16 x float>, <16 x float>* %486, align 64, !tbaa !432
  %488 = fadd <16 x float> %387, %487
  %489 = fcmp ogt <16 x float> %488, zeroinitializer
  %490 = select <16 x i1> %489, <16 x float> %488, <16 x float> zeroinitializer
  %491 = mul i64 %indvars.iv99, 1924145348608
  %sext109 = add i64 %491, 687194767360
  %492 = ashr exact i64 %sext109, 32
  %493 = getelementptr inbounds float, float* %12, i64 %492
  %494 = bitcast float* %493 to <16 x float>*
  store <16 x float> %490, <16 x float>* %494, align 64, !tbaa !435
  %495 = getelementptr inbounds i8, i8* %63, i64 704
  %496 = bitcast i8* %495 to <16 x float>*
  %497 = load <16 x float>, <16 x float>* %496, align 64, !tbaa !432
  %498 = fadd <16 x float> %387, %497
  %499 = fcmp ogt <16 x float> %498, zeroinitializer
  %500 = select <16 x i1> %499, <16 x float> %498, <16 x float> zeroinitializer
  %501 = mul i64 %indvars.iv99, 1924145348608
  %sext110 = add i64 %501, 755914244096
  %502 = ashr exact i64 %sext110, 32
  %503 = getelementptr inbounds float, float* %12, i64 %502
  %504 = bitcast float* %503 to <16 x float>*
  store <16 x float> %500, <16 x float>* %504, align 64, !tbaa !435
  %505 = getelementptr inbounds i8, i8* %63, i64 768
  %506 = bitcast i8* %505 to <16 x float>*
  %507 = load <16 x float>, <16 x float>* %506, align 64, !tbaa !432
  %508 = fadd <16 x float> %387, %507
  %509 = fcmp ogt <16 x float> %508, zeroinitializer
  %510 = select <16 x i1> %509, <16 x float> %508, <16 x float> zeroinitializer
  %511 = mul i64 %indvars.iv99, 1924145348608
  %sext111 = add i64 %511, 824633720832
  %512 = ashr exact i64 %sext111, 32
  %513 = getelementptr inbounds float, float* %12, i64 %512
  %514 = bitcast float* %513 to <16 x float>*
  store <16 x float> %510, <16 x float>* %514, align 64, !tbaa !435
  %515 = getelementptr inbounds i8, i8* %63, i64 832
  %516 = bitcast i8* %515 to <16 x float>*
  %517 = load <16 x float>, <16 x float>* %516, align 64, !tbaa !432
  %518 = fadd <16 x float> %387, %517
  %519 = fcmp ogt <16 x float> %518, zeroinitializer
  %520 = select <16 x i1> %519, <16 x float> %518, <16 x float> zeroinitializer
  %521 = mul i64 %indvars.iv99, 1924145348608
  %sext112 = add i64 %521, 893353197568
  %522 = ashr exact i64 %sext112, 32
  %523 = getelementptr inbounds float, float* %12, i64 %522
  %524 = bitcast float* %523 to <16 x float>*
  store <16 x float> %520, <16 x float>* %524, align 64, !tbaa !435
  %525 = getelementptr inbounds i8, i8* %63, i64 896
  %526 = bitcast i8* %525 to <16 x float>*
  %527 = load <16 x float>, <16 x float>* %526, align 64, !tbaa !432
  %528 = fadd <16 x float> %387, %527
  %529 = fcmp ogt <16 x float> %528, zeroinitializer
  %530 = select <16 x i1> %529, <16 x float> %528, <16 x float> zeroinitializer
  %531 = mul i64 %indvars.iv99, 1924145348608
  %sext113 = add i64 %531, 962072674304
  %532 = ashr exact i64 %sext113, 32
  %533 = getelementptr inbounds float, float* %12, i64 %532
  %534 = bitcast float* %533 to <16 x float>*
  store <16 x float> %530, <16 x float>* %534, align 64, !tbaa !435
  %535 = getelementptr inbounds i8, i8* %63, i64 960
  %536 = bitcast i8* %535 to <16 x float>*
  %537 = load <16 x float>, <16 x float>* %536, align 64, !tbaa !432
  %538 = fadd <16 x float> %387, %537
  %539 = fcmp ogt <16 x float> %538, zeroinitializer
  %540 = select <16 x i1> %539, <16 x float> %538, <16 x float> zeroinitializer
  %541 = mul i64 %indvars.iv99, 1924145348608
  %sext114 = add i64 %541, 1030792151040
  %542 = ashr exact i64 %sext114, 32
  %543 = getelementptr inbounds float, float* %12, i64 %542
  %544 = bitcast float* %543 to <16 x float>*
  store <16 x float> %540, <16 x float>* %544, align 64, !tbaa !435
  %545 = getelementptr inbounds i8, i8* %63, i64 1024
  %546 = bitcast i8* %545 to <16 x float>*
  %547 = load <16 x float>, <16 x float>* %546, align 64, !tbaa !432
  %548 = fadd <16 x float> %387, %547
  %549 = fcmp ogt <16 x float> %548, zeroinitializer
  %550 = select <16 x i1> %549, <16 x float> %548, <16 x float> zeroinitializer
  %551 = mul i64 %indvars.iv99, 1924145348608
  %sext115 = add i64 %551, 1099511627776
  %552 = ashr exact i64 %sext115, 32
  %553 = getelementptr inbounds float, float* %12, i64 %552
  %554 = bitcast float* %553 to <16 x float>*
  store <16 x float> %550, <16 x float>* %554, align 64, !tbaa !435
  %555 = getelementptr inbounds i8, i8* %63, i64 1088
  %556 = bitcast i8* %555 to <16 x float>*
  %557 = load <16 x float>, <16 x float>* %556, align 64, !tbaa !432
  %558 = fadd <16 x float> %387, %557
  %559 = fcmp ogt <16 x float> %558, zeroinitializer
  %560 = select <16 x i1> %559, <16 x float> %558, <16 x float> zeroinitializer
  %561 = mul i64 %indvars.iv99, 1924145348608
  %sext116 = add i64 %561, 1168231104512
  %562 = ashr exact i64 %sext116, 32
  %563 = getelementptr inbounds float, float* %12, i64 %562
  %564 = bitcast float* %563 to <16 x float>*
  store <16 x float> %560, <16 x float>* %564, align 64, !tbaa !435
  %565 = getelementptr inbounds i8, i8* %63, i64 1152
  %566 = bitcast i8* %565 to <16 x float>*
  %567 = load <16 x float>, <16 x float>* %566, align 64, !tbaa !432
  %568 = fadd <16 x float> %387, %567
  %569 = fcmp ogt <16 x float> %568, zeroinitializer
  %570 = select <16 x i1> %569, <16 x float> %568, <16 x float> zeroinitializer
  %571 = mul i64 %indvars.iv99, 1924145348608
  %sext117 = add i64 %571, 1236950581248
  %572 = ashr exact i64 %sext117, 32
  %573 = getelementptr inbounds float, float* %12, i64 %572
  %574 = bitcast float* %573 to <16 x float>*
  store <16 x float> %570, <16 x float>* %574, align 64, !tbaa !435
  %575 = getelementptr inbounds i8, i8* %63, i64 1216
  %576 = bitcast i8* %575 to <16 x float>*
  %577 = load <16 x float>, <16 x float>* %576, align 64, !tbaa !432
  %578 = fadd <16 x float> %387, %577
  %579 = fcmp ogt <16 x float> %578, zeroinitializer
  %580 = select <16 x i1> %579, <16 x float> %578, <16 x float> zeroinitializer
  %581 = mul i64 %indvars.iv99, 1924145348608
  %sext118 = add i64 %581, 1305670057984
  %582 = ashr exact i64 %sext118, 32
  %583 = getelementptr inbounds float, float* %12, i64 %582
  %584 = bitcast float* %583 to <16 x float>*
  store <16 x float> %580, <16 x float>* %584, align 64, !tbaa !435
  %585 = getelementptr inbounds i8, i8* %63, i64 1280
  %586 = bitcast i8* %585 to <16 x float>*
  %587 = load <16 x float>, <16 x float>* %586, align 64, !tbaa !432
  %588 = fadd <16 x float> %387, %587
  %589 = fcmp ogt <16 x float> %588, zeroinitializer
  %590 = select <16 x i1> %589, <16 x float> %588, <16 x float> zeroinitializer
  %591 = mul i64 %indvars.iv99, 1924145348608
  %sext119 = add i64 %591, 1374389534720
  %592 = ashr exact i64 %sext119, 32
  %593 = getelementptr inbounds float, float* %12, i64 %592
  %594 = bitcast float* %593 to <16 x float>*
  store <16 x float> %590, <16 x float>* %594, align 64, !tbaa !435
  %595 = getelementptr inbounds i8, i8* %63, i64 1344
  %596 = bitcast i8* %595 to <16 x float>*
  %597 = load <16 x float>, <16 x float>* %596, align 64, !tbaa !432
  %598 = fadd <16 x float> %387, %597
  %599 = fcmp ogt <16 x float> %598, zeroinitializer
  %600 = select <16 x i1> %599, <16 x float> %598, <16 x float> zeroinitializer
  %601 = mul i64 %indvars.iv99, 1924145348608
  %sext120 = add i64 %601, 1443109011456
  %602 = ashr exact i64 %sext120, 32
  %603 = getelementptr inbounds float, float* %12, i64 %602
  %604 = bitcast float* %603 to <16 x float>*
  store <16 x float> %600, <16 x float>* %604, align 64, !tbaa !435
  %605 = getelementptr inbounds i8, i8* %63, i64 1408
  %606 = bitcast i8* %605 to <16 x float>*
  %607 = load <16 x float>, <16 x float>* %606, align 64, !tbaa !432
  %608 = fadd <16 x float> %387, %607
  %609 = fcmp ogt <16 x float> %608, zeroinitializer
  %610 = select <16 x i1> %609, <16 x float> %608, <16 x float> zeroinitializer
  %611 = mul i64 %indvars.iv99, 1924145348608
  %sext121 = add i64 %611, 1511828488192
  %612 = ashr exact i64 %sext121, 32
  %613 = getelementptr inbounds float, float* %12, i64 %612
  %614 = bitcast float* %613 to <16 x float>*
  store <16 x float> %610, <16 x float>* %614, align 64, !tbaa !435
  %615 = getelementptr inbounds i8, i8* %63, i64 1472
  %616 = bitcast i8* %615 to <16 x float>*
  %617 = load <16 x float>, <16 x float>* %616, align 64, !tbaa !432
  %618 = fadd <16 x float> %387, %617
  %619 = fcmp ogt <16 x float> %618, zeroinitializer
  %620 = select <16 x i1> %619, <16 x float> %618, <16 x float> zeroinitializer
  %621 = mul i64 %indvars.iv99, 1924145348608
  %sext122 = add i64 %621, 1580547964928
  %622 = ashr exact i64 %sext122, 32
  %623 = getelementptr inbounds float, float* %12, i64 %622
  %624 = bitcast float* %623 to <16 x float>*
  store <16 x float> %620, <16 x float>* %624, align 64, !tbaa !435
  %625 = getelementptr inbounds i8, i8* %63, i64 1536
  %626 = bitcast i8* %625 to <16 x float>*
  %627 = load <16 x float>, <16 x float>* %626, align 64, !tbaa !432
  %628 = fadd <16 x float> %387, %627
  %629 = fcmp ogt <16 x float> %628, zeroinitializer
  %630 = select <16 x i1> %629, <16 x float> %628, <16 x float> zeroinitializer
  %631 = mul i64 %indvars.iv99, 1924145348608
  %sext123 = add i64 %631, 1649267441664
  %632 = ashr exact i64 %sext123, 32
  %633 = getelementptr inbounds float, float* %12, i64 %632
  %634 = bitcast float* %633 to <16 x float>*
  store <16 x float> %630, <16 x float>* %634, align 64, !tbaa !435
  %635 = getelementptr inbounds i8, i8* %63, i64 1600
  %636 = bitcast i8* %635 to <16 x float>*
  %637 = load <16 x float>, <16 x float>* %636, align 64, !tbaa !432
  %638 = fadd <16 x float> %387, %637
  %639 = fcmp ogt <16 x float> %638, zeroinitializer
  %640 = select <16 x i1> %639, <16 x float> %638, <16 x float> zeroinitializer
  %641 = mul i64 %indvars.iv99, 1924145348608
  %sext124 = add i64 %641, 1717986918400
  %642 = ashr exact i64 %sext124, 32
  %643 = getelementptr inbounds float, float* %12, i64 %642
  %644 = bitcast float* %643 to <16 x float>*
  store <16 x float> %640, <16 x float>* %644, align 64, !tbaa !435
  %645 = getelementptr inbounds i8, i8* %63, i64 1664
  %646 = bitcast i8* %645 to <16 x float>*
  %647 = load <16 x float>, <16 x float>* %646, align 64, !tbaa !432
  %648 = fadd <16 x float> %387, %647
  %649 = fcmp ogt <16 x float> %648, zeroinitializer
  %650 = select <16 x i1> %649, <16 x float> %648, <16 x float> zeroinitializer
  %651 = mul i64 %indvars.iv99, 1924145348608
  %sext125 = add i64 %651, 1786706395136
  %652 = ashr exact i64 %sext125, 32
  %653 = getelementptr inbounds float, float* %12, i64 %652
  %654 = bitcast float* %653 to <16 x float>*
  store <16 x float> %650, <16 x float>* %654, align 64, !tbaa !435
  %655 = getelementptr inbounds i8, i8* %63, i64 1728
  %656 = bitcast i8* %655 to <16 x float>*
  %657 = load <16 x float>, <16 x float>* %656, align 64, !tbaa !432
  %658 = fadd <16 x float> %387, %657
  %659 = fcmp ogt <16 x float> %658, zeroinitializer
  %660 = select <16 x i1> %659, <16 x float> %658, <16 x float> zeroinitializer
  %661 = mul i64 %indvars.iv99, 1924145348608
  %sext126 = add i64 %661, 1855425871872
  %662 = ashr exact i64 %sext126, 32
  %663 = getelementptr inbounds float, float* %12, i64 %662
  %664 = bitcast float* %663 to <16 x float>*
  store <16 x float> %660, <16 x float>* %664, align 64, !tbaa !435
  %665 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %666 = tail call i32 %665(i32 1, i32 %18, i8* nonnull %63)
  %indvars.iv.next100 = add nsw i64 %indvars.iv99, 1
  %667 = icmp slt i64 %indvars.iv.next100, %60
  br i1 %667, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define dllexport i32 @fused_layout_transform_nn_batch_flatten_nn_batch_flatten_multiply(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #3 !dbg !438 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !440, metadata !DIExpression()), !dbg !443
  call void @llvm.dbg.value(metadata i8* %1, metadata !441, metadata !DIExpression()), !dbg !443
  call void @llvm.dbg.value(metadata i32 %2, metadata !442, metadata !DIExpression()), !dbg !443
  %3 = bitcast i8* %0 to %1**, !dbg !443
  %4 = load %1*, %1** %3, align 8, !dbg !443
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !443
  %6 = bitcast i8* %5 to %1**, !dbg !443
  %7 = load %1*, %1** %6, align 8, !dbg !443
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !443
  %9 = load i8*, i8** %8, align 8, !dbg !443
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !443
  %11 = load i8*, i8** %10, align 8, !dbg !443
  tail call fastcc void @fused_layout_transform_nn_batch_flatten_nn_batch_flatten_multiply_compute_(i8* %11, i8* %9), !dbg !443
  ret i32 0, !dbg !443
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_layout_transform_nn_batch_flatten_nn_batch_flatten_multiply_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #4 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_body

for_body:                                         ; preds = %for_body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for_body ]
  %4 = trunc i64 %indvars.iv to i32
  %5 = urem i32 %4, 784
  %6 = udiv i32 %5, 49
  %7 = urem i32 %4, 49
  %8 = shl nuw nsw i32 %7, 4
  %9 = sub nsw i32 %4, %5
  %10 = add nsw i32 %9, %8
  %11 = add nsw i32 %10, %6
  %12 = sext i32 %11 to i64
  %13 = getelementptr inbounds float, float* %2, i64 %12
  %14 = bitcast float* %13 to i32*
  %15 = load i32, i32* %14, align 4, !tbaa !444
  %16 = getelementptr inbounds float, float* %3, i64 %indvars.iv
  %17 = bitcast float* %16 to i32*
  store i32 %15, i32* %17, align 4, !tbaa !447
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 25088
  br i1 %exitcond, label %for_end, label %for_body, !prof !20

for_end:                                          ; preds = %for_body
  ret void
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !450 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !452, metadata !DIExpression()), !dbg !455
  call void @llvm.dbg.value(metadata i8* %1, metadata !453, metadata !DIExpression()), !dbg !455
  call void @llvm.dbg.value(metadata i32 %2, metadata !454, metadata !DIExpression()), !dbg !455
  %3 = bitcast i8* %0 to %1**, !dbg !455
  %4 = load %1*, %1** %3, align 8, !dbg !455
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !455
  %6 = bitcast i8* %5 to %1**, !dbg !455
  %7 = load %1*, %1** %6, align 8, !dbg !455
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !455
  %9 = bitcast i8* %8 to %1**, !dbg !455
  %10 = load %1*, %1** %9, align 8, !dbg !455
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !455
  %12 = bitcast i8* %11 to %1**, !dbg !455
  %13 = load %1*, %1** %12, align 8, !dbg !455
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !455
  %15 = load i8*, i8** %14, align 8, !dbg !455
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !455
  %17 = load i32, i32* %16, align 4, !dbg !455
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !455
  %19 = load i8*, i8** %18, align 8, !dbg !455
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !455
  %21 = load i8*, i8** %20, align 8, !dbg !455
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !455
  %23 = load i8*, i8** %22, align 8, !dbg !455
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !455
  ret i32 %24, !dbg !455
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 921600, i32 2, i32 32)
  %7 = alloca %31, align 8
  %8 = getelementptr inbounds %31, %31* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %31, %31* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %31* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.27, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %22, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %32, align 8
  %15 = getelementptr inbounds %32, %32* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %32, %32* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %32, %32* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %32, %32* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = getelementptr inbounds %32, %32* %14, i64 0, i32 4
  store i32 %4, i32* %19, align 8
  %20 = bitcast %32* %14 to i8*
  %21 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %22 = call i32 %21(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.28, i8* nonnull %20, i32 0)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %24 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %25 = call i32 %24(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.27(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 29
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 30
  %15 = select i1 %14, i32 %13, i32 30
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 30
  %18 = select i1 %17, i32 %16, i32 30
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %556, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = mul nsw i32 %20, 7680
  %.off = add i32 %20, -1
  %22 = icmp ult i32 %.off, 28
  %23 = mul nsw i32 %20, 7168
  br i1 %22, label %for_body2.us.preheader, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_body
  br label %for_body2

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %for_end6.us
  %indvars.iv24 = phi i64 [ %indvars.iv.next25, %for_end6.us ], [ 0, %for_body2.us.preheader ]
  %24 = shl nsw i64 %indvars.iv24, 8
  %25 = trunc i64 %indvars.iv24 to i32
  %26 = add i32 %25, -1
  %27 = icmp ult i32 %26, 28
  br i1 %27, label %vector.body.preheader, label %vector.body37

vector.body.preheader:                            ; preds = %for_body2.us
  br label %vector.body

for_end6.us:                                      ; preds = %vector.body, %vector.body37
  %indvars.iv.next25 = add nuw nsw i64 %indvars.iv24, 1
  %exitcond27 = icmp eq i64 %indvars.iv.next25, 30
  br i1 %exitcond27, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %vector.body.preheader, %vector.body
  %index = phi i64 [ %index.next, %vector.body ], [ 0, %vector.body.preheader ]
  %28 = add nuw nsw i64 %index, %24
  %29 = trunc i64 %28 to i32
  %30 = add i32 %21, %29
  %31 = trunc i64 %28 to i32
  %32 = add i32 %31, -7424
  %33 = add i32 %32, %23
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !456
  %37 = getelementptr float, float* %35, i64 4
  %38 = bitcast float* %37 to <4 x i32>*
  %wide.load36 = load <4 x i32>, <4 x i32>* %38, align 4, !tbaa !456
  %39 = sext i32 %30 to i64
  %40 = getelementptr inbounds float, float* %4, i64 %39
  %41 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %41, align 4, !tbaa !459
  %42 = getelementptr float, float* %40, i64 4
  %43 = bitcast float* %42 to <4 x i32>*
  store <4 x i32> %wide.load36, <4 x i32>* %43, align 4, !tbaa !459
  %index.next = add i64 %index, 8
  %44 = icmp eq i64 %index.next, 256
  br i1 %44, label %for_end6.us, label %vector.body, !llvm.loop !462

vector.body37:                                    ; preds = %for_body2.us
  %45 = trunc i64 %24 to i32
  %46 = add i32 %21, %45
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %4, i64 %47
  %49 = bitcast float* %48 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %49, align 4, !tbaa !459
  %50 = getelementptr float, float* %48, i64 4
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %51, align 4, !tbaa !459
  %52 = trunc i64 %24 to i32
  %53 = or i32 %52, 8
  %54 = add i32 %21, %53
  %55 = sext i32 %54 to i64
  %56 = getelementptr inbounds float, float* %4, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %57, align 4, !tbaa !459
  %58 = getelementptr float, float* %56, i64 4
  %59 = bitcast float* %58 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %59, align 4, !tbaa !459
  %60 = trunc i64 %24 to i32
  %61 = or i32 %60, 16
  %62 = add i32 %21, %61
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = bitcast float* %64 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %65, align 4, !tbaa !459
  %66 = getelementptr float, float* %64, i64 4
  %67 = bitcast float* %66 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %67, align 4, !tbaa !459
  %68 = trunc i64 %24 to i32
  %69 = or i32 %68, 24
  %70 = add i32 %21, %69
  %71 = sext i32 %70 to i64
  %72 = getelementptr inbounds float, float* %4, i64 %71
  %73 = bitcast float* %72 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %73, align 4, !tbaa !459
  %74 = getelementptr float, float* %72, i64 4
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %75, align 4, !tbaa !459
  %76 = trunc i64 %24 to i32
  %77 = or i32 %76, 32
  %78 = add i32 %21, %77
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %4, i64 %79
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %81, align 4, !tbaa !459
  %82 = getelementptr float, float* %80, i64 4
  %83 = bitcast float* %82 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %83, align 4, !tbaa !459
  %84 = trunc i64 %24 to i32
  %85 = or i32 %84, 40
  %86 = add i32 %21, %85
  %87 = sext i32 %86 to i64
  %88 = getelementptr inbounds float, float* %4, i64 %87
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %89, align 4, !tbaa !459
  %90 = getelementptr float, float* %88, i64 4
  %91 = bitcast float* %90 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %91, align 4, !tbaa !459
  %92 = trunc i64 %24 to i32
  %93 = or i32 %92, 48
  %94 = add i32 %21, %93
  %95 = sext i32 %94 to i64
  %96 = getelementptr inbounds float, float* %4, i64 %95
  %97 = bitcast float* %96 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %97, align 4, !tbaa !459
  %98 = getelementptr float, float* %96, i64 4
  %99 = bitcast float* %98 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %99, align 4, !tbaa !459
  %100 = trunc i64 %24 to i32
  %101 = or i32 %100, 56
  %102 = add i32 %21, %101
  %103 = sext i32 %102 to i64
  %104 = getelementptr inbounds float, float* %4, i64 %103
  %105 = bitcast float* %104 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %105, align 4, !tbaa !459
  %106 = getelementptr float, float* %104, i64 4
  %107 = bitcast float* %106 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %107, align 4, !tbaa !459
  %108 = trunc i64 %24 to i32
  %109 = or i32 %108, 64
  %110 = add i32 %21, %109
  %111 = sext i32 %110 to i64
  %112 = getelementptr inbounds float, float* %4, i64 %111
  %113 = bitcast float* %112 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %113, align 4, !tbaa !459
  %114 = getelementptr float, float* %112, i64 4
  %115 = bitcast float* %114 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %115, align 4, !tbaa !459
  %116 = trunc i64 %24 to i32
  %117 = or i32 %116, 72
  %118 = add i32 %21, %117
  %119 = sext i32 %118 to i64
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = bitcast float* %120 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %121, align 4, !tbaa !459
  %122 = getelementptr float, float* %120, i64 4
  %123 = bitcast float* %122 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %123, align 4, !tbaa !459
  %124 = trunc i64 %24 to i32
  %125 = or i32 %124, 80
  %126 = add i32 %21, %125
  %127 = sext i32 %126 to i64
  %128 = getelementptr inbounds float, float* %4, i64 %127
  %129 = bitcast float* %128 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %129, align 4, !tbaa !459
  %130 = getelementptr float, float* %128, i64 4
  %131 = bitcast float* %130 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %131, align 4, !tbaa !459
  %132 = trunc i64 %24 to i32
  %133 = or i32 %132, 88
  %134 = add i32 %21, %133
  %135 = sext i32 %134 to i64
  %136 = getelementptr inbounds float, float* %4, i64 %135
  %137 = bitcast float* %136 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %137, align 4, !tbaa !459
  %138 = getelementptr float, float* %136, i64 4
  %139 = bitcast float* %138 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %139, align 4, !tbaa !459
  %140 = trunc i64 %24 to i32
  %141 = or i32 %140, 96
  %142 = add i32 %21, %141
  %143 = sext i32 %142 to i64
  %144 = getelementptr inbounds float, float* %4, i64 %143
  %145 = bitcast float* %144 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %145, align 4, !tbaa !459
  %146 = getelementptr float, float* %144, i64 4
  %147 = bitcast float* %146 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %147, align 4, !tbaa !459
  %148 = trunc i64 %24 to i32
  %149 = or i32 %148, 104
  %150 = add i32 %21, %149
  %151 = sext i32 %150 to i64
  %152 = getelementptr inbounds float, float* %4, i64 %151
  %153 = bitcast float* %152 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %153, align 4, !tbaa !459
  %154 = getelementptr float, float* %152, i64 4
  %155 = bitcast float* %154 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %155, align 4, !tbaa !459
  %156 = trunc i64 %24 to i32
  %157 = or i32 %156, 112
  %158 = add i32 %21, %157
  %159 = sext i32 %158 to i64
  %160 = getelementptr inbounds float, float* %4, i64 %159
  %161 = bitcast float* %160 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %161, align 4, !tbaa !459
  %162 = getelementptr float, float* %160, i64 4
  %163 = bitcast float* %162 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %163, align 4, !tbaa !459
  %164 = trunc i64 %24 to i32
  %165 = or i32 %164, 120
  %166 = add i32 %21, %165
  %167 = sext i32 %166 to i64
  %168 = getelementptr inbounds float, float* %4, i64 %167
  %169 = bitcast float* %168 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %169, align 4, !tbaa !459
  %170 = getelementptr float, float* %168, i64 4
  %171 = bitcast float* %170 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %171, align 4, !tbaa !459
  %172 = trunc i64 %24 to i32
  %173 = or i32 %172, 128
  %174 = add i32 %21, %173
  %175 = sext i32 %174 to i64
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = bitcast float* %176 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %177, align 4, !tbaa !459
  %178 = getelementptr float, float* %176, i64 4
  %179 = bitcast float* %178 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %179, align 4, !tbaa !459
  %180 = trunc i64 %24 to i32
  %181 = or i32 %180, 136
  %182 = add i32 %21, %181
  %183 = sext i32 %182 to i64
  %184 = getelementptr inbounds float, float* %4, i64 %183
  %185 = bitcast float* %184 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %185, align 4, !tbaa !459
  %186 = getelementptr float, float* %184, i64 4
  %187 = bitcast float* %186 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %187, align 4, !tbaa !459
  %188 = trunc i64 %24 to i32
  %189 = or i32 %188, 144
  %190 = add i32 %21, %189
  %191 = sext i32 %190 to i64
  %192 = getelementptr inbounds float, float* %4, i64 %191
  %193 = bitcast float* %192 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %193, align 4, !tbaa !459
  %194 = getelementptr float, float* %192, i64 4
  %195 = bitcast float* %194 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %195, align 4, !tbaa !459
  %196 = trunc i64 %24 to i32
  %197 = or i32 %196, 152
  %198 = add i32 %21, %197
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %4, i64 %199
  %201 = bitcast float* %200 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %201, align 4, !tbaa !459
  %202 = getelementptr float, float* %200, i64 4
  %203 = bitcast float* %202 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %203, align 4, !tbaa !459
  %204 = trunc i64 %24 to i32
  %205 = or i32 %204, 160
  %206 = add i32 %21, %205
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds float, float* %4, i64 %207
  %209 = bitcast float* %208 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %209, align 4, !tbaa !459
  %210 = getelementptr float, float* %208, i64 4
  %211 = bitcast float* %210 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %211, align 4, !tbaa !459
  %212 = trunc i64 %24 to i32
  %213 = or i32 %212, 168
  %214 = add i32 %21, %213
  %215 = sext i32 %214 to i64
  %216 = getelementptr inbounds float, float* %4, i64 %215
  %217 = bitcast float* %216 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %217, align 4, !tbaa !459
  %218 = getelementptr float, float* %216, i64 4
  %219 = bitcast float* %218 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %219, align 4, !tbaa !459
  %220 = trunc i64 %24 to i32
  %221 = or i32 %220, 176
  %222 = add i32 %21, %221
  %223 = sext i32 %222 to i64
  %224 = getelementptr inbounds float, float* %4, i64 %223
  %225 = bitcast float* %224 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %225, align 4, !tbaa !459
  %226 = getelementptr float, float* %224, i64 4
  %227 = bitcast float* %226 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %227, align 4, !tbaa !459
  %228 = trunc i64 %24 to i32
  %229 = or i32 %228, 184
  %230 = add i32 %21, %229
  %231 = sext i32 %230 to i64
  %232 = getelementptr inbounds float, float* %4, i64 %231
  %233 = bitcast float* %232 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %233, align 4, !tbaa !459
  %234 = getelementptr float, float* %232, i64 4
  %235 = bitcast float* %234 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %235, align 4, !tbaa !459
  %236 = trunc i64 %24 to i32
  %237 = or i32 %236, 192
  %238 = add i32 %21, %237
  %239 = sext i32 %238 to i64
  %240 = getelementptr inbounds float, float* %4, i64 %239
  %241 = bitcast float* %240 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %241, align 4, !tbaa !459
  %242 = getelementptr float, float* %240, i64 4
  %243 = bitcast float* %242 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %243, align 4, !tbaa !459
  %244 = trunc i64 %24 to i32
  %245 = or i32 %244, 200
  %246 = add i32 %21, %245
  %247 = sext i32 %246 to i64
  %248 = getelementptr inbounds float, float* %4, i64 %247
  %249 = bitcast float* %248 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %249, align 4, !tbaa !459
  %250 = getelementptr float, float* %248, i64 4
  %251 = bitcast float* %250 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %251, align 4, !tbaa !459
  %252 = trunc i64 %24 to i32
  %253 = or i32 %252, 208
  %254 = add i32 %21, %253
  %255 = sext i32 %254 to i64
  %256 = getelementptr inbounds float, float* %4, i64 %255
  %257 = bitcast float* %256 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %257, align 4, !tbaa !459
  %258 = getelementptr float, float* %256, i64 4
  %259 = bitcast float* %258 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %259, align 4, !tbaa !459
  %260 = trunc i64 %24 to i32
  %261 = or i32 %260, 216
  %262 = add i32 %21, %261
  %263 = sext i32 %262 to i64
  %264 = getelementptr inbounds float, float* %4, i64 %263
  %265 = bitcast float* %264 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %265, align 4, !tbaa !459
  %266 = getelementptr float, float* %264, i64 4
  %267 = bitcast float* %266 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %267, align 4, !tbaa !459
  %268 = trunc i64 %24 to i32
  %269 = or i32 %268, 224
  %270 = add i32 %21, %269
  %271 = sext i32 %270 to i64
  %272 = getelementptr inbounds float, float* %4, i64 %271
  %273 = bitcast float* %272 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %273, align 4, !tbaa !459
  %274 = getelementptr float, float* %272, i64 4
  %275 = bitcast float* %274 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %275, align 4, !tbaa !459
  %276 = trunc i64 %24 to i32
  %277 = or i32 %276, 232
  %278 = add i32 %21, %277
  %279 = sext i32 %278 to i64
  %280 = getelementptr inbounds float, float* %4, i64 %279
  %281 = bitcast float* %280 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %281, align 4, !tbaa !459
  %282 = getelementptr float, float* %280, i64 4
  %283 = bitcast float* %282 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %283, align 4, !tbaa !459
  %284 = trunc i64 %24 to i32
  %285 = or i32 %284, 240
  %286 = add i32 %21, %285
  %287 = sext i32 %286 to i64
  %288 = getelementptr inbounds float, float* %4, i64 %287
  %289 = bitcast float* %288 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %289, align 4, !tbaa !459
  %290 = getelementptr float, float* %288, i64 4
  %291 = bitcast float* %290 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %291, align 4, !tbaa !459
  %292 = trunc i64 %24 to i32
  %293 = or i32 %292, 248
  %294 = add i32 %21, %293
  %295 = sext i32 %294 to i64
  %296 = getelementptr inbounds float, float* %4, i64 %295
  %297 = bitcast float* %296 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %297, align 4, !tbaa !459
  %298 = getelementptr float, float* %296, i64 4
  %299 = bitcast float* %298 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %299, align 4, !tbaa !459
  br label %for_end6.us

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2.preheader, %for_body2
  %indvars.iv15 = phi i64 [ %indvars.iv.next16, %for_body2 ], [ 0, %for_body2.preheader ]
  %300 = shl nsw i64 %indvars.iv15, 8
  %301 = trunc i64 %300 to i32
  %302 = add i32 %21, %301
  %303 = sext i32 %302 to i64
  %304 = getelementptr inbounds float, float* %4, i64 %303
  %305 = bitcast float* %304 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %305, align 4, !tbaa !459
  %306 = getelementptr float, float* %304, i64 4
  %307 = bitcast float* %306 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %307, align 4, !tbaa !459
  %308 = trunc i64 %300 to i32
  %309 = or i32 %308, 8
  %310 = add i32 %21, %309
  %311 = sext i32 %310 to i64
  %312 = getelementptr inbounds float, float* %4, i64 %311
  %313 = bitcast float* %312 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %313, align 4, !tbaa !459
  %314 = getelementptr float, float* %312, i64 4
  %315 = bitcast float* %314 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %315, align 4, !tbaa !459
  %316 = trunc i64 %300 to i32
  %317 = or i32 %316, 16
  %318 = add i32 %21, %317
  %319 = sext i32 %318 to i64
  %320 = getelementptr inbounds float, float* %4, i64 %319
  %321 = bitcast float* %320 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %321, align 4, !tbaa !459
  %322 = getelementptr float, float* %320, i64 4
  %323 = bitcast float* %322 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %323, align 4, !tbaa !459
  %324 = trunc i64 %300 to i32
  %325 = or i32 %324, 24
  %326 = add i32 %21, %325
  %327 = sext i32 %326 to i64
  %328 = getelementptr inbounds float, float* %4, i64 %327
  %329 = bitcast float* %328 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %329, align 4, !tbaa !459
  %330 = getelementptr float, float* %328, i64 4
  %331 = bitcast float* %330 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %331, align 4, !tbaa !459
  %332 = trunc i64 %300 to i32
  %333 = or i32 %332, 32
  %334 = add i32 %21, %333
  %335 = sext i32 %334 to i64
  %336 = getelementptr inbounds float, float* %4, i64 %335
  %337 = bitcast float* %336 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %337, align 4, !tbaa !459
  %338 = getelementptr float, float* %336, i64 4
  %339 = bitcast float* %338 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %339, align 4, !tbaa !459
  %340 = trunc i64 %300 to i32
  %341 = or i32 %340, 40
  %342 = add i32 %21, %341
  %343 = sext i32 %342 to i64
  %344 = getelementptr inbounds float, float* %4, i64 %343
  %345 = bitcast float* %344 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %345, align 4, !tbaa !459
  %346 = getelementptr float, float* %344, i64 4
  %347 = bitcast float* %346 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %347, align 4, !tbaa !459
  %348 = trunc i64 %300 to i32
  %349 = or i32 %348, 48
  %350 = add i32 %21, %349
  %351 = sext i32 %350 to i64
  %352 = getelementptr inbounds float, float* %4, i64 %351
  %353 = bitcast float* %352 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %353, align 4, !tbaa !459
  %354 = getelementptr float, float* %352, i64 4
  %355 = bitcast float* %354 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %355, align 4, !tbaa !459
  %356 = trunc i64 %300 to i32
  %357 = or i32 %356, 56
  %358 = add i32 %21, %357
  %359 = sext i32 %358 to i64
  %360 = getelementptr inbounds float, float* %4, i64 %359
  %361 = bitcast float* %360 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %361, align 4, !tbaa !459
  %362 = getelementptr float, float* %360, i64 4
  %363 = bitcast float* %362 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %363, align 4, !tbaa !459
  %364 = trunc i64 %300 to i32
  %365 = or i32 %364, 64
  %366 = add i32 %21, %365
  %367 = sext i32 %366 to i64
  %368 = getelementptr inbounds float, float* %4, i64 %367
  %369 = bitcast float* %368 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %369, align 4, !tbaa !459
  %370 = getelementptr float, float* %368, i64 4
  %371 = bitcast float* %370 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %371, align 4, !tbaa !459
  %372 = trunc i64 %300 to i32
  %373 = or i32 %372, 72
  %374 = add i32 %21, %373
  %375 = sext i32 %374 to i64
  %376 = getelementptr inbounds float, float* %4, i64 %375
  %377 = bitcast float* %376 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %377, align 4, !tbaa !459
  %378 = getelementptr float, float* %376, i64 4
  %379 = bitcast float* %378 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %379, align 4, !tbaa !459
  %380 = trunc i64 %300 to i32
  %381 = or i32 %380, 80
  %382 = add i32 %21, %381
  %383 = sext i32 %382 to i64
  %384 = getelementptr inbounds float, float* %4, i64 %383
  %385 = bitcast float* %384 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %385, align 4, !tbaa !459
  %386 = getelementptr float, float* %384, i64 4
  %387 = bitcast float* %386 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %387, align 4, !tbaa !459
  %388 = trunc i64 %300 to i32
  %389 = or i32 %388, 88
  %390 = add i32 %21, %389
  %391 = sext i32 %390 to i64
  %392 = getelementptr inbounds float, float* %4, i64 %391
  %393 = bitcast float* %392 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %393, align 4, !tbaa !459
  %394 = getelementptr float, float* %392, i64 4
  %395 = bitcast float* %394 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %395, align 4, !tbaa !459
  %396 = trunc i64 %300 to i32
  %397 = or i32 %396, 96
  %398 = add i32 %21, %397
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds float, float* %4, i64 %399
  %401 = bitcast float* %400 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %401, align 4, !tbaa !459
  %402 = getelementptr float, float* %400, i64 4
  %403 = bitcast float* %402 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %403, align 4, !tbaa !459
  %404 = trunc i64 %300 to i32
  %405 = or i32 %404, 104
  %406 = add i32 %21, %405
  %407 = sext i32 %406 to i64
  %408 = getelementptr inbounds float, float* %4, i64 %407
  %409 = bitcast float* %408 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %409, align 4, !tbaa !459
  %410 = getelementptr float, float* %408, i64 4
  %411 = bitcast float* %410 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %411, align 4, !tbaa !459
  %412 = trunc i64 %300 to i32
  %413 = or i32 %412, 112
  %414 = add i32 %21, %413
  %415 = sext i32 %414 to i64
  %416 = getelementptr inbounds float, float* %4, i64 %415
  %417 = bitcast float* %416 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %417, align 4, !tbaa !459
  %418 = getelementptr float, float* %416, i64 4
  %419 = bitcast float* %418 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %419, align 4, !tbaa !459
  %420 = trunc i64 %300 to i32
  %421 = or i32 %420, 120
  %422 = add i32 %21, %421
  %423 = sext i32 %422 to i64
  %424 = getelementptr inbounds float, float* %4, i64 %423
  %425 = bitcast float* %424 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %425, align 4, !tbaa !459
  %426 = getelementptr float, float* %424, i64 4
  %427 = bitcast float* %426 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %427, align 4, !tbaa !459
  %428 = trunc i64 %300 to i32
  %429 = or i32 %428, 128
  %430 = add i32 %21, %429
  %431 = sext i32 %430 to i64
  %432 = getelementptr inbounds float, float* %4, i64 %431
  %433 = bitcast float* %432 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %433, align 4, !tbaa !459
  %434 = getelementptr float, float* %432, i64 4
  %435 = bitcast float* %434 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %435, align 4, !tbaa !459
  %436 = trunc i64 %300 to i32
  %437 = or i32 %436, 136
  %438 = add i32 %21, %437
  %439 = sext i32 %438 to i64
  %440 = getelementptr inbounds float, float* %4, i64 %439
  %441 = bitcast float* %440 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %441, align 4, !tbaa !459
  %442 = getelementptr float, float* %440, i64 4
  %443 = bitcast float* %442 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %443, align 4, !tbaa !459
  %444 = trunc i64 %300 to i32
  %445 = or i32 %444, 144
  %446 = add i32 %21, %445
  %447 = sext i32 %446 to i64
  %448 = getelementptr inbounds float, float* %4, i64 %447
  %449 = bitcast float* %448 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %449, align 4, !tbaa !459
  %450 = getelementptr float, float* %448, i64 4
  %451 = bitcast float* %450 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %451, align 4, !tbaa !459
  %452 = trunc i64 %300 to i32
  %453 = or i32 %452, 152
  %454 = add i32 %21, %453
  %455 = sext i32 %454 to i64
  %456 = getelementptr inbounds float, float* %4, i64 %455
  %457 = bitcast float* %456 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %457, align 4, !tbaa !459
  %458 = getelementptr float, float* %456, i64 4
  %459 = bitcast float* %458 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %459, align 4, !tbaa !459
  %460 = trunc i64 %300 to i32
  %461 = or i32 %460, 160
  %462 = add i32 %21, %461
  %463 = sext i32 %462 to i64
  %464 = getelementptr inbounds float, float* %4, i64 %463
  %465 = bitcast float* %464 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %465, align 4, !tbaa !459
  %466 = getelementptr float, float* %464, i64 4
  %467 = bitcast float* %466 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %467, align 4, !tbaa !459
  %468 = trunc i64 %300 to i32
  %469 = or i32 %468, 168
  %470 = add i32 %21, %469
  %471 = sext i32 %470 to i64
  %472 = getelementptr inbounds float, float* %4, i64 %471
  %473 = bitcast float* %472 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %473, align 4, !tbaa !459
  %474 = getelementptr float, float* %472, i64 4
  %475 = bitcast float* %474 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %475, align 4, !tbaa !459
  %476 = trunc i64 %300 to i32
  %477 = or i32 %476, 176
  %478 = add i32 %21, %477
  %479 = sext i32 %478 to i64
  %480 = getelementptr inbounds float, float* %4, i64 %479
  %481 = bitcast float* %480 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %481, align 4, !tbaa !459
  %482 = getelementptr float, float* %480, i64 4
  %483 = bitcast float* %482 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %483, align 4, !tbaa !459
  %484 = trunc i64 %300 to i32
  %485 = or i32 %484, 184
  %486 = add i32 %21, %485
  %487 = sext i32 %486 to i64
  %488 = getelementptr inbounds float, float* %4, i64 %487
  %489 = bitcast float* %488 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %489, align 4, !tbaa !459
  %490 = getelementptr float, float* %488, i64 4
  %491 = bitcast float* %490 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %491, align 4, !tbaa !459
  %492 = trunc i64 %300 to i32
  %493 = or i32 %492, 192
  %494 = add i32 %21, %493
  %495 = sext i32 %494 to i64
  %496 = getelementptr inbounds float, float* %4, i64 %495
  %497 = bitcast float* %496 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %497, align 4, !tbaa !459
  %498 = getelementptr float, float* %496, i64 4
  %499 = bitcast float* %498 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %499, align 4, !tbaa !459
  %500 = trunc i64 %300 to i32
  %501 = or i32 %500, 200
  %502 = add i32 %21, %501
  %503 = sext i32 %502 to i64
  %504 = getelementptr inbounds float, float* %4, i64 %503
  %505 = bitcast float* %504 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %505, align 4, !tbaa !459
  %506 = getelementptr float, float* %504, i64 4
  %507 = bitcast float* %506 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %507, align 4, !tbaa !459
  %508 = trunc i64 %300 to i32
  %509 = or i32 %508, 208
  %510 = add i32 %21, %509
  %511 = sext i32 %510 to i64
  %512 = getelementptr inbounds float, float* %4, i64 %511
  %513 = bitcast float* %512 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %513, align 4, !tbaa !459
  %514 = getelementptr float, float* %512, i64 4
  %515 = bitcast float* %514 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %515, align 4, !tbaa !459
  %516 = trunc i64 %300 to i32
  %517 = or i32 %516, 216
  %518 = add i32 %21, %517
  %519 = sext i32 %518 to i64
  %520 = getelementptr inbounds float, float* %4, i64 %519
  %521 = bitcast float* %520 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %521, align 4, !tbaa !459
  %522 = getelementptr float, float* %520, i64 4
  %523 = bitcast float* %522 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %523, align 4, !tbaa !459
  %524 = trunc i64 %300 to i32
  %525 = or i32 %524, 224
  %526 = add i32 %21, %525
  %527 = sext i32 %526 to i64
  %528 = getelementptr inbounds float, float* %4, i64 %527
  %529 = bitcast float* %528 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %529, align 4, !tbaa !459
  %530 = getelementptr float, float* %528, i64 4
  %531 = bitcast float* %530 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %531, align 4, !tbaa !459
  %532 = trunc i64 %300 to i32
  %533 = or i32 %532, 232
  %534 = add i32 %21, %533
  %535 = sext i32 %534 to i64
  %536 = getelementptr inbounds float, float* %4, i64 %535
  %537 = bitcast float* %536 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %537, align 4, !tbaa !459
  %538 = getelementptr float, float* %536, i64 4
  %539 = bitcast float* %538 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %539, align 4, !tbaa !459
  %540 = trunc i64 %300 to i32
  %541 = or i32 %540, 240
  %542 = add i32 %21, %541
  %543 = sext i32 %542 to i64
  %544 = getelementptr inbounds float, float* %4, i64 %543
  %545 = bitcast float* %544 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %545, align 4, !tbaa !459
  %546 = getelementptr float, float* %544, i64 4
  %547 = bitcast float* %546 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %547, align 4, !tbaa !459
  %548 = trunc i64 %300 to i32
  %549 = or i32 %548, 248
  %550 = add i32 %21, %549
  %551 = sext i32 %550 to i64
  %552 = getelementptr inbounds float, float* %4, i64 %551
  %553 = bitcast float* %552 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %553, align 4, !tbaa !459
  %554 = getelementptr float, float* %552, i64 4
  %555 = bitcast float* %554 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %555, align 4, !tbaa !459
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp eq i64 %indvars.iv.next16, 30
  br i1 %exitcond17, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2, %for_end6.us
  %556 = add nsw i32 %20, 1
  %557 = icmp slt i32 %556, %15
  br i1 %557, label %for_body, label %for_end, !prof !19
}

define private i32 @__tvm_parallel_lambda.28(i32, %0* nocapture readonly, i8* nocapture readonly) {
entry:
  %3 = alloca [7 x <32 x float>], align 128
  %4 = bitcast [7 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0
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
  %21 = add nsw i32 %20, 447
  %22 = sdiv i32 %21, %20
  %23 = add nsw i32 %0, 1
  %24 = mul nsw i32 %22, %23
  %25 = icmp slt i32 %24, 448
  %26 = select i1 %25, i32 %24, i32 448
  %27 = mul nsw i32 %22, %0
  %28 = icmp slt i32 %27, 448
  %29 = select i1 %28, i32 %27, i32 448
  %30 = icmp slt i32 %29, %26
  br i1 %30, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %31 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %32 = bitcast float* %31 to <32 x float>*
  %33 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %34 = bitcast float* %33 to <32 x float>*
  %35 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %36 = bitcast float* %35 to <32 x float>*
  %37 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %38 = bitcast float* %37 to <32 x float>*
  %39 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %40 = bitcast float* %39 to <32 x float>*
  %41 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %42 = bitcast float* %41 to <32 x float>*
  %43 = add i32 %29, 1
  %44 = sext i32 %43 to i64
  %45 = add nsw i64 %44, -1
  %46 = sext i32 %26 to i64
  %47 = bitcast [7 x <32 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv64 = phi i64 [ %45, %for_body.lr.ph ], [ %indvars.iv.next65, %for_end3 ]
  %48 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %49 = tail call i8* %48(i32 1, i32 %18, i64 3584, i32 2, i32 32)
  %50 = trunc i64 %indvars.iv64 to i32
  %51 = srem i32 %50, 28
  %52 = sdiv i32 %50, 28
  %53 = mul nsw i32 %52, 73728
  %54 = sext i32 %53 to i64
  %55 = mul nsw i32 %51, 7680
  %56 = sext i32 %55 to i64
  %57 = mul nsw i32 %51, 7680
  %58 = add nsw i32 %57, 7680
  %59 = sext i32 %58 to i64
  %60 = add nsw i64 %54, 24576
  %61 = mul nsw i32 %51, 7680
  %62 = add nsw i32 %61, 15360
  %63 = sext i32 %62 to i64
  %64 = add nsw i64 %54, 49152
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end9.2, %for_body
  %indvar = phi i64 [ 0, %for_body ], [ %indvar.next, %for_end9.2 ]
  %65 = mul nuw nsw i64 %indvar, 896
  %scevgep = getelementptr i8, i8* %49, i64 %65
  %66 = mul nuw nsw i64 %indvar, 1792
  %67 = add nsw i64 %66, %56
  call void @llvm.memset.p0i8.i64(i8* nonnull %47, i8 0, i64 896, i32 128, i1 false)
  br label %for_body8

for_end3:                                         ; preds = %for_end9.2
  %68 = mul nsw i64 %indvars.iv64, 896
  %69 = shl nsw i32 %52, 5
  %70 = sext i32 %69 to i64
  %71 = getelementptr inbounds float, float* %15, i64 %70
  %72 = bitcast float* %71 to <32 x float>*
  %73 = load <32 x float>, <32 x float>* %72, align 64, !tbaa !463
  %74 = bitcast i8* %49 to <32 x float>*
  %75 = load <32 x float>, <32 x float>* %74, align 64, !tbaa !466
  %76 = fadd <32 x float> %73, %75
  %77 = fcmp ogt <32 x float> %76, zeroinitializer
  %78 = select <32 x i1> %77, <32 x float> %76, <32 x float> zeroinitializer
  %79 = getelementptr inbounds float, float* %12, i64 %68
  %80 = bitcast float* %79 to <32 x float>*
  store <32 x float> %78, <32 x float>* %80, align 64, !tbaa !469
  %81 = getelementptr inbounds i8, i8* %49, i64 128
  %82 = bitcast i8* %81 to <32 x float>*
  %83 = load <32 x float>, <32 x float>* %82, align 64, !tbaa !466
  %84 = fadd <32 x float> %73, %83
  %85 = fcmp ogt <32 x float> %84, zeroinitializer
  %86 = select <32 x i1> %85, <32 x float> %84, <32 x float> zeroinitializer
  %87 = mul i64 %indvars.iv64, 3848290697216
  %sext = ashr exact i64 %87, 32
  %88 = or i64 %sext, 32
  %89 = getelementptr inbounds float, float* %12, i64 %88
  %90 = bitcast float* %89 to <32 x float>*
  store <32 x float> %86, <32 x float>* %90, align 64, !tbaa !469
  %91 = getelementptr inbounds i8, i8* %49, i64 256
  %92 = bitcast i8* %91 to <32 x float>*
  %93 = load <32 x float>, <32 x float>* %92, align 64, !tbaa !466
  %94 = fadd <32 x float> %73, %93
  %95 = fcmp ogt <32 x float> %94, zeroinitializer
  %96 = select <32 x i1> %95, <32 x float> %94, <32 x float> zeroinitializer
  %97 = mul i64 %indvars.iv64, 3848290697216
  %sext66 = ashr exact i64 %97, 32
  %98 = or i64 %sext66, 64
  %99 = getelementptr inbounds float, float* %12, i64 %98
  %100 = bitcast float* %99 to <32 x float>*
  store <32 x float> %96, <32 x float>* %100, align 64, !tbaa !469
  %101 = getelementptr inbounds i8, i8* %49, i64 384
  %102 = bitcast i8* %101 to <32 x float>*
  %103 = load <32 x float>, <32 x float>* %102, align 64, !tbaa !466
  %104 = fadd <32 x float> %73, %103
  %105 = fcmp ogt <32 x float> %104, zeroinitializer
  %106 = select <32 x i1> %105, <32 x float> %104, <32 x float> zeroinitializer
  %107 = mul i64 %indvars.iv64, 3848290697216
  %sext67 = ashr exact i64 %107, 32
  %108 = or i64 %sext67, 96
  %109 = getelementptr inbounds float, float* %12, i64 %108
  %110 = bitcast float* %109 to <32 x float>*
  store <32 x float> %106, <32 x float>* %110, align 64, !tbaa !469
  %111 = getelementptr inbounds i8, i8* %49, i64 512
  %112 = bitcast i8* %111 to <32 x float>*
  %113 = load <32 x float>, <32 x float>* %112, align 64, !tbaa !466
  %114 = fadd <32 x float> %73, %113
  %115 = fcmp ogt <32 x float> %114, zeroinitializer
  %116 = select <32 x i1> %115, <32 x float> %114, <32 x float> zeroinitializer
  %117 = mul i64 %indvars.iv64, 3848290697216
  %sext68 = add i64 %117, 549755813888
  %118 = ashr exact i64 %sext68, 32
  %119 = getelementptr inbounds float, float* %12, i64 %118
  %120 = bitcast float* %119 to <32 x float>*
  store <32 x float> %116, <32 x float>* %120, align 64, !tbaa !469
  %121 = getelementptr inbounds i8, i8* %49, i64 640
  %122 = bitcast i8* %121 to <32 x float>*
  %123 = load <32 x float>, <32 x float>* %122, align 64, !tbaa !466
  %124 = fadd <32 x float> %73, %123
  %125 = fcmp ogt <32 x float> %124, zeroinitializer
  %126 = select <32 x i1> %125, <32 x float> %124, <32 x float> zeroinitializer
  %127 = mul i64 %indvars.iv64, 3848290697216
  %sext69 = add i64 %127, 687194767360
  %128 = ashr exact i64 %sext69, 32
  %129 = getelementptr inbounds float, float* %12, i64 %128
  %130 = bitcast float* %129 to <32 x float>*
  store <32 x float> %126, <32 x float>* %130, align 64, !tbaa !469
  %131 = getelementptr inbounds i8, i8* %49, i64 768
  %132 = bitcast i8* %131 to <32 x float>*
  %133 = load <32 x float>, <32 x float>* %132, align 64, !tbaa !466
  %134 = fadd <32 x float> %73, %133
  %135 = fcmp ogt <32 x float> %134, zeroinitializer
  %136 = select <32 x i1> %135, <32 x float> %134, <32 x float> zeroinitializer
  %137 = mul i64 %indvars.iv64, 3848290697216
  %sext70 = add i64 %137, 824633720832
  %138 = ashr exact i64 %sext70, 32
  %139 = getelementptr inbounds float, float* %12, i64 %138
  %140 = bitcast float* %139 to <32 x float>*
  store <32 x float> %136, <32 x float>* %140, align 64, !tbaa !469
  %141 = getelementptr inbounds i8, i8* %49, i64 896
  %142 = bitcast i8* %141 to <32 x float>*
  %143 = load <32 x float>, <32 x float>* %142, align 64, !tbaa !466
  %144 = fadd <32 x float> %73, %143
  %145 = fcmp ogt <32 x float> %144, zeroinitializer
  %146 = select <32 x i1> %145, <32 x float> %144, <32 x float> zeroinitializer
  %147 = mul i64 %indvars.iv64, 3848290697216
  %sext71 = add i64 %147, 962072674304
  %148 = ashr exact i64 %sext71, 32
  %149 = getelementptr inbounds float, float* %12, i64 %148
  %150 = bitcast float* %149 to <32 x float>*
  store <32 x float> %146, <32 x float>* %150, align 64, !tbaa !469
  %151 = getelementptr inbounds i8, i8* %49, i64 1024
  %152 = bitcast i8* %151 to <32 x float>*
  %153 = load <32 x float>, <32 x float>* %152, align 64, !tbaa !466
  %154 = fadd <32 x float> %73, %153
  %155 = fcmp ogt <32 x float> %154, zeroinitializer
  %156 = select <32 x i1> %155, <32 x float> %154, <32 x float> zeroinitializer
  %157 = mul i64 %indvars.iv64, 3848290697216
  %sext72 = add i64 %157, 1099511627776
  %158 = ashr exact i64 %sext72, 32
  %159 = getelementptr inbounds float, float* %12, i64 %158
  %160 = bitcast float* %159 to <32 x float>*
  store <32 x float> %156, <32 x float>* %160, align 64, !tbaa !469
  %161 = getelementptr inbounds i8, i8* %49, i64 1152
  %162 = bitcast i8* %161 to <32 x float>*
  %163 = load <32 x float>, <32 x float>* %162, align 64, !tbaa !466
  %164 = fadd <32 x float> %73, %163
  %165 = fcmp ogt <32 x float> %164, zeroinitializer
  %166 = select <32 x i1> %165, <32 x float> %164, <32 x float> zeroinitializer
  %167 = mul i64 %indvars.iv64, 3848290697216
  %sext73 = add i64 %167, 1236950581248
  %168 = ashr exact i64 %sext73, 32
  %169 = getelementptr inbounds float, float* %12, i64 %168
  %170 = bitcast float* %169 to <32 x float>*
  store <32 x float> %166, <32 x float>* %170, align 64, !tbaa !469
  %171 = getelementptr inbounds i8, i8* %49, i64 1280
  %172 = bitcast i8* %171 to <32 x float>*
  %173 = load <32 x float>, <32 x float>* %172, align 64, !tbaa !466
  %174 = fadd <32 x float> %73, %173
  %175 = fcmp ogt <32 x float> %174, zeroinitializer
  %176 = select <32 x i1> %175, <32 x float> %174, <32 x float> zeroinitializer
  %177 = mul i64 %indvars.iv64, 3848290697216
  %sext74 = add i64 %177, 1374389534720
  %178 = ashr exact i64 %sext74, 32
  %179 = getelementptr inbounds float, float* %12, i64 %178
  %180 = bitcast float* %179 to <32 x float>*
  store <32 x float> %176, <32 x float>* %180, align 64, !tbaa !469
  %181 = getelementptr inbounds i8, i8* %49, i64 1408
  %182 = bitcast i8* %181 to <32 x float>*
  %183 = load <32 x float>, <32 x float>* %182, align 64, !tbaa !466
  %184 = fadd <32 x float> %73, %183
  %185 = fcmp ogt <32 x float> %184, zeroinitializer
  %186 = select <32 x i1> %185, <32 x float> %184, <32 x float> zeroinitializer
  %187 = mul i64 %indvars.iv64, 3848290697216
  %sext75 = add i64 %187, 1511828488192
  %188 = ashr exact i64 %sext75, 32
  %189 = getelementptr inbounds float, float* %12, i64 %188
  %190 = bitcast float* %189 to <32 x float>*
  store <32 x float> %186, <32 x float>* %190, align 64, !tbaa !469
  %191 = getelementptr inbounds i8, i8* %49, i64 1536
  %192 = bitcast i8* %191 to <32 x float>*
  %193 = load <32 x float>, <32 x float>* %192, align 64, !tbaa !466
  %194 = fadd <32 x float> %73, %193
  %195 = fcmp ogt <32 x float> %194, zeroinitializer
  %196 = select <32 x i1> %195, <32 x float> %194, <32 x float> zeroinitializer
  %197 = mul i64 %indvars.iv64, 3848290697216
  %sext76 = add i64 %197, 1649267441664
  %198 = ashr exact i64 %sext76, 32
  %199 = getelementptr inbounds float, float* %12, i64 %198
  %200 = bitcast float* %199 to <32 x float>*
  store <32 x float> %196, <32 x float>* %200, align 64, !tbaa !469
  %201 = getelementptr inbounds i8, i8* %49, i64 1664
  %202 = bitcast i8* %201 to <32 x float>*
  %203 = load <32 x float>, <32 x float>* %202, align 64, !tbaa !466
  %204 = fadd <32 x float> %73, %203
  %205 = fcmp ogt <32 x float> %204, zeroinitializer
  %206 = select <32 x i1> %205, <32 x float> %204, <32 x float> zeroinitializer
  %207 = mul i64 %indvars.iv64, 3848290697216
  %sext77 = add i64 %207, 1786706395136
  %208 = ashr exact i64 %sext77, 32
  %209 = getelementptr inbounds float, float* %12, i64 %208
  %210 = bitcast float* %209 to <32 x float>*
  store <32 x float> %206, <32 x float>* %210, align 64, !tbaa !469
  %211 = getelementptr inbounds i8, i8* %49, i64 1792
  %212 = bitcast i8* %211 to <32 x float>*
  %213 = load <32 x float>, <32 x float>* %212, align 64, !tbaa !466
  %214 = fadd <32 x float> %73, %213
  %215 = fcmp ogt <32 x float> %214, zeroinitializer
  %216 = select <32 x i1> %215, <32 x float> %214, <32 x float> zeroinitializer
  %217 = mul i64 %indvars.iv64, 3848290697216
  %sext78 = add i64 %217, 1924145348608
  %218 = ashr exact i64 %sext78, 32
  %219 = getelementptr inbounds float, float* %12, i64 %218
  %220 = bitcast float* %219 to <32 x float>*
  store <32 x float> %216, <32 x float>* %220, align 64, !tbaa !469
  %221 = getelementptr inbounds i8, i8* %49, i64 1920
  %222 = bitcast i8* %221 to <32 x float>*
  %223 = load <32 x float>, <32 x float>* %222, align 64, !tbaa !466
  %224 = fadd <32 x float> %73, %223
  %225 = fcmp ogt <32 x float> %224, zeroinitializer
  %226 = select <32 x i1> %225, <32 x float> %224, <32 x float> zeroinitializer
  %227 = mul i64 %indvars.iv64, 3848290697216
  %sext79 = add i64 %227, 2061584302080
  %228 = ashr exact i64 %sext79, 32
  %229 = getelementptr inbounds float, float* %12, i64 %228
  %230 = bitcast float* %229 to <32 x float>*
  store <32 x float> %226, <32 x float>* %230, align 64, !tbaa !469
  %231 = getelementptr inbounds i8, i8* %49, i64 2048
  %232 = bitcast i8* %231 to <32 x float>*
  %233 = load <32 x float>, <32 x float>* %232, align 64, !tbaa !466
  %234 = fadd <32 x float> %73, %233
  %235 = fcmp ogt <32 x float> %234, zeroinitializer
  %236 = select <32 x i1> %235, <32 x float> %234, <32 x float> zeroinitializer
  %237 = mul i64 %indvars.iv64, 3848290697216
  %sext80 = add i64 %237, 2199023255552
  %238 = ashr exact i64 %sext80, 32
  %239 = getelementptr inbounds float, float* %12, i64 %238
  %240 = bitcast float* %239 to <32 x float>*
  store <32 x float> %236, <32 x float>* %240, align 64, !tbaa !469
  %241 = getelementptr inbounds i8, i8* %49, i64 2176
  %242 = bitcast i8* %241 to <32 x float>*
  %243 = load <32 x float>, <32 x float>* %242, align 64, !tbaa !466
  %244 = fadd <32 x float> %73, %243
  %245 = fcmp ogt <32 x float> %244, zeroinitializer
  %246 = select <32 x i1> %245, <32 x float> %244, <32 x float> zeroinitializer
  %247 = mul i64 %indvars.iv64, 3848290697216
  %sext81 = add i64 %247, 2336462209024
  %248 = ashr exact i64 %sext81, 32
  %249 = getelementptr inbounds float, float* %12, i64 %248
  %250 = bitcast float* %249 to <32 x float>*
  store <32 x float> %246, <32 x float>* %250, align 64, !tbaa !469
  %251 = getelementptr inbounds i8, i8* %49, i64 2304
  %252 = bitcast i8* %251 to <32 x float>*
  %253 = load <32 x float>, <32 x float>* %252, align 64, !tbaa !466
  %254 = fadd <32 x float> %73, %253
  %255 = fcmp ogt <32 x float> %254, zeroinitializer
  %256 = select <32 x i1> %255, <32 x float> %254, <32 x float> zeroinitializer
  %257 = mul i64 %indvars.iv64, 3848290697216
  %sext82 = add i64 %257, 2473901162496
  %258 = ashr exact i64 %sext82, 32
  %259 = getelementptr inbounds float, float* %12, i64 %258
  %260 = bitcast float* %259 to <32 x float>*
  store <32 x float> %256, <32 x float>* %260, align 64, !tbaa !469
  %261 = getelementptr inbounds i8, i8* %49, i64 2432
  %262 = bitcast i8* %261 to <32 x float>*
  %263 = load <32 x float>, <32 x float>* %262, align 64, !tbaa !466
  %264 = fadd <32 x float> %73, %263
  %265 = fcmp ogt <32 x float> %264, zeroinitializer
  %266 = select <32 x i1> %265, <32 x float> %264, <32 x float> zeroinitializer
  %267 = mul i64 %indvars.iv64, 3848290697216
  %sext83 = add i64 %267, 2611340115968
  %268 = ashr exact i64 %sext83, 32
  %269 = getelementptr inbounds float, float* %12, i64 %268
  %270 = bitcast float* %269 to <32 x float>*
  store <32 x float> %266, <32 x float>* %270, align 64, !tbaa !469
  %271 = getelementptr inbounds i8, i8* %49, i64 2560
  %272 = bitcast i8* %271 to <32 x float>*
  %273 = load <32 x float>, <32 x float>* %272, align 64, !tbaa !466
  %274 = fadd <32 x float> %73, %273
  %275 = fcmp ogt <32 x float> %274, zeroinitializer
  %276 = select <32 x i1> %275, <32 x float> %274, <32 x float> zeroinitializer
  %277 = mul i64 %indvars.iv64, 3848290697216
  %sext84 = add i64 %277, 2748779069440
  %278 = ashr exact i64 %sext84, 32
  %279 = getelementptr inbounds float, float* %12, i64 %278
  %280 = bitcast float* %279 to <32 x float>*
  store <32 x float> %276, <32 x float>* %280, align 64, !tbaa !469
  %281 = getelementptr inbounds i8, i8* %49, i64 2688
  %282 = bitcast i8* %281 to <32 x float>*
  %283 = load <32 x float>, <32 x float>* %282, align 64, !tbaa !466
  %284 = fadd <32 x float> %73, %283
  %285 = fcmp ogt <32 x float> %284, zeroinitializer
  %286 = select <32 x i1> %285, <32 x float> %284, <32 x float> zeroinitializer
  %287 = mul i64 %indvars.iv64, 3848290697216
  %sext85 = add i64 %287, 2886218022912
  %288 = ashr exact i64 %sext85, 32
  %289 = getelementptr inbounds float, float* %12, i64 %288
  %290 = bitcast float* %289 to <32 x float>*
  store <32 x float> %286, <32 x float>* %290, align 64, !tbaa !469
  %291 = getelementptr inbounds i8, i8* %49, i64 2816
  %292 = bitcast i8* %291 to <32 x float>*
  %293 = load <32 x float>, <32 x float>* %292, align 64, !tbaa !466
  %294 = fadd <32 x float> %73, %293
  %295 = fcmp ogt <32 x float> %294, zeroinitializer
  %296 = select <32 x i1> %295, <32 x float> %294, <32 x float> zeroinitializer
  %297 = mul i64 %indvars.iv64, 3848290697216
  %sext86 = add i64 %297, 3023656976384
  %298 = ashr exact i64 %sext86, 32
  %299 = getelementptr inbounds float, float* %12, i64 %298
  %300 = bitcast float* %299 to <32 x float>*
  store <32 x float> %296, <32 x float>* %300, align 64, !tbaa !469
  %301 = getelementptr inbounds i8, i8* %49, i64 2944
  %302 = bitcast i8* %301 to <32 x float>*
  %303 = load <32 x float>, <32 x float>* %302, align 64, !tbaa !466
  %304 = fadd <32 x float> %73, %303
  %305 = fcmp ogt <32 x float> %304, zeroinitializer
  %306 = select <32 x i1> %305, <32 x float> %304, <32 x float> zeroinitializer
  %307 = mul i64 %indvars.iv64, 3848290697216
  %sext87 = add i64 %307, 3161095929856
  %308 = ashr exact i64 %sext87, 32
  %309 = getelementptr inbounds float, float* %12, i64 %308
  %310 = bitcast float* %309 to <32 x float>*
  store <32 x float> %306, <32 x float>* %310, align 64, !tbaa !469
  %311 = getelementptr inbounds i8, i8* %49, i64 3072
  %312 = bitcast i8* %311 to <32 x float>*
  %313 = load <32 x float>, <32 x float>* %312, align 64, !tbaa !466
  %314 = fadd <32 x float> %73, %313
  %315 = fcmp ogt <32 x float> %314, zeroinitializer
  %316 = select <32 x i1> %315, <32 x float> %314, <32 x float> zeroinitializer
  %317 = mul i64 %indvars.iv64, 3848290697216
  %sext88 = add i64 %317, 3298534883328
  %318 = ashr exact i64 %sext88, 32
  %319 = getelementptr inbounds float, float* %12, i64 %318
  %320 = bitcast float* %319 to <32 x float>*
  store <32 x float> %316, <32 x float>* %320, align 64, !tbaa !469
  %321 = getelementptr inbounds i8, i8* %49, i64 3200
  %322 = bitcast i8* %321 to <32 x float>*
  %323 = load <32 x float>, <32 x float>* %322, align 64, !tbaa !466
  %324 = fadd <32 x float> %73, %323
  %325 = fcmp ogt <32 x float> %324, zeroinitializer
  %326 = select <32 x i1> %325, <32 x float> %324, <32 x float> zeroinitializer
  %327 = mul i64 %indvars.iv64, 3848290697216
  %sext89 = add i64 %327, 3435973836800
  %328 = ashr exact i64 %sext89, 32
  %329 = getelementptr inbounds float, float* %12, i64 %328
  %330 = bitcast float* %329 to <32 x float>*
  store <32 x float> %326, <32 x float>* %330, align 64, !tbaa !469
  %331 = getelementptr inbounds i8, i8* %49, i64 3328
  %332 = bitcast i8* %331 to <32 x float>*
  %333 = load <32 x float>, <32 x float>* %332, align 64, !tbaa !466
  %334 = fadd <32 x float> %73, %333
  %335 = fcmp ogt <32 x float> %334, zeroinitializer
  %336 = select <32 x i1> %335, <32 x float> %334, <32 x float> zeroinitializer
  %337 = mul i64 %indvars.iv64, 3848290697216
  %sext90 = add i64 %337, 3573412790272
  %338 = ashr exact i64 %sext90, 32
  %339 = getelementptr inbounds float, float* %12, i64 %338
  %340 = bitcast float* %339 to <32 x float>*
  store <32 x float> %336, <32 x float>* %340, align 64, !tbaa !469
  %341 = getelementptr inbounds i8, i8* %49, i64 3456
  %342 = bitcast i8* %341 to <32 x float>*
  %343 = load <32 x float>, <32 x float>* %342, align 64, !tbaa !466
  %344 = fadd <32 x float> %73, %343
  %345 = fcmp ogt <32 x float> %344, zeroinitializer
  %346 = select <32 x i1> %345, <32 x float> %344, <32 x float> zeroinitializer
  %347 = mul i64 %indvars.iv64, 3848290697216
  %sext91 = add i64 %347, 3710851743744
  %348 = ashr exact i64 %sext91, 32
  %349 = getelementptr inbounds float, float* %12, i64 %348
  %350 = bitcast float* %349 to <32 x float>*
  store <32 x float> %346, <32 x float>* %350, align 64, !tbaa !469
  %351 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %352 = tail call i32 %351(i32 1, i32 %18, i8* nonnull %49)
  %indvars.iv.next65 = add nsw i64 %indvars.iv64, 1
  %353 = icmp slt i64 %indvars.iv.next65, %46
  br i1 %353, label %for_body, label %for_end, !prof !19

for_body8:                                        ; preds = %for_body8, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body8 ]
  %354 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %439, %for_body8 ]
  %355 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %433, %for_body8 ]
  %356 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %432, %for_body8 ]
  %357 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %431, %for_body8 ]
  %358 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %430, %for_body8 ]
  %359 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %429, %for_body8 ]
  %360 = phi <32 x float> [ zeroinitializer, %for_body2 ], [ %428, %for_body8 ]
  %361 = add nsw i64 %67, %indvars.iv
  %362 = getelementptr inbounds float, float* %6, i64 %361
  %363 = load float, float* %362, align 4, !tbaa !459
  %364 = insertelement <32 x float> undef, float %363, i32 0
  %365 = shufflevector <32 x float> %364, <32 x float> undef, <32 x i32> zeroinitializer
  %366 = shl nsw i64 %indvars.iv, 5
  %367 = add nsw i64 %366, %54
  %368 = getelementptr inbounds float, float* %9, i64 %367
  %369 = bitcast float* %368 to <32 x float>*
  %370 = load <32 x float>, <32 x float>* %369, align 64, !tbaa !472
  %371 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %365, <32 x float> %370, <32 x float> %360)
  %372 = add nsw i64 %361, 256
  %373 = getelementptr inbounds float, float* %6, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !459
  %375 = insertelement <32 x float> undef, float %374, i32 0
  %376 = shufflevector <32 x float> %375, <32 x float> undef, <32 x i32> zeroinitializer
  %377 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %376, <32 x float> %370, <32 x float> %359)
  %378 = add nsw i64 %361, 512
  %379 = getelementptr inbounds float, float* %6, i64 %378
  %380 = load float, float* %379, align 4, !tbaa !459
  %381 = insertelement <32 x float> undef, float %380, i32 0
  %382 = shufflevector <32 x float> %381, <32 x float> undef, <32 x i32> zeroinitializer
  %383 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %382, <32 x float> %370, <32 x float> %358)
  %384 = add nsw i64 %361, 768
  %385 = getelementptr inbounds float, float* %6, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !459
  %387 = insertelement <32 x float> undef, float %386, i32 0
  %388 = shufflevector <32 x float> %387, <32 x float> undef, <32 x i32> zeroinitializer
  %389 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %388, <32 x float> %370, <32 x float> %357)
  %390 = add nsw i64 %361, 1024
  %391 = getelementptr inbounds float, float* %6, i64 %390
  %392 = load float, float* %391, align 4, !tbaa !459
  %393 = insertelement <32 x float> undef, float %392, i32 0
  %394 = shufflevector <32 x float> %393, <32 x float> undef, <32 x i32> zeroinitializer
  %395 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %394, <32 x float> %370, <32 x float> %356)
  %396 = add nsw i64 %361, 1280
  %397 = getelementptr inbounds float, float* %6, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !459
  %399 = insertelement <32 x float> undef, float %398, i32 0
  %400 = shufflevector <32 x float> %399, <32 x float> undef, <32 x i32> zeroinitializer
  %401 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %400, <32 x float> %370, <32 x float> %355)
  %402 = add nsw i64 %361, 1536
  %403 = getelementptr inbounds float, float* %6, i64 %402
  %404 = load float, float* %403, align 4, !tbaa !459
  %405 = insertelement <32 x float> undef, float %404, i32 0
  %406 = shufflevector <32 x float> %405, <32 x float> undef, <32 x i32> zeroinitializer
  %407 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %406, <32 x float> %370, <32 x float> %354)
  %408 = add nsw i64 %367, 8192
  %409 = getelementptr inbounds float, float* %9, i64 %408
  %410 = bitcast float* %409 to <32 x float>*
  %411 = load <32 x float>, <32 x float>* %410, align 64, !tbaa !472
  %412 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %376, <32 x float> %411, <32 x float> %371)
  %413 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %382, <32 x float> %411, <32 x float> %377)
  %414 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %388, <32 x float> %411, <32 x float> %383)
  %415 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %394, <32 x float> %411, <32 x float> %389)
  %416 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %400, <32 x float> %411, <32 x float> %395)
  %417 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %406, <32 x float> %411, <32 x float> %401)
  %418 = add nsw i64 %361, 1792
  %419 = getelementptr inbounds float, float* %6, i64 %418
  %420 = load float, float* %419, align 4, !tbaa !459
  %421 = insertelement <32 x float> undef, float %420, i32 0
  %422 = shufflevector <32 x float> %421, <32 x float> undef, <32 x i32> zeroinitializer
  %423 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %422, <32 x float> %411, <32 x float> %407)
  %424 = add nsw i64 %367, 16384
  %425 = getelementptr inbounds float, float* %9, i64 %424
  %426 = bitcast float* %425 to <32 x float>*
  %427 = load <32 x float>, <32 x float>* %426, align 64, !tbaa !472
  %428 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %382, <32 x float> %427, <32 x float> %412)
  %429 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %388, <32 x float> %427, <32 x float> %413)
  %430 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %394, <32 x float> %427, <32 x float> %414)
  %431 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %400, <32 x float> %427, <32 x float> %415)
  %432 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %406, <32 x float> %427, <32 x float> %416)
  %433 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %422, <32 x float> %427, <32 x float> %417)
  %434 = add nsw i64 %361, 2048
  %435 = getelementptr inbounds float, float* %6, i64 %434
  %436 = load float, float* %435, align 4, !tbaa !459
  %437 = insertelement <32 x float> undef, float %436, i32 0
  %438 = shufflevector <32 x float> %437, <32 x float> undef, <32 x i32> zeroinitializer
  %439 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %438, <32 x float> %427, <32 x float> %423)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end9, label %for_body8, !prof !20

for_end9:                                         ; preds = %for_body8
  %440 = add nsw i64 %66, %59
  br label %for_body8.1

for_body8.1:                                      ; preds = %for_body8.1, %for_end9
  %indvars.iv.1 = phi i64 [ 0, %for_end9 ], [ %indvars.iv.next.1, %for_body8.1 ]
  %441 = phi <32 x float> [ %439, %for_end9 ], [ %526, %for_body8.1 ]
  %442 = phi <32 x float> [ %433, %for_end9 ], [ %520, %for_body8.1 ]
  %443 = phi <32 x float> [ %432, %for_end9 ], [ %519, %for_body8.1 ]
  %444 = phi <32 x float> [ %431, %for_end9 ], [ %518, %for_body8.1 ]
  %445 = phi <32 x float> [ %430, %for_end9 ], [ %517, %for_body8.1 ]
  %446 = phi <32 x float> [ %429, %for_end9 ], [ %516, %for_body8.1 ]
  %447 = phi <32 x float> [ %428, %for_end9 ], [ %515, %for_body8.1 ]
  %448 = add nsw i64 %440, %indvars.iv.1
  %449 = getelementptr inbounds float, float* %6, i64 %448
  %450 = load float, float* %449, align 4, !tbaa !459
  %451 = insertelement <32 x float> undef, float %450, i32 0
  %452 = shufflevector <32 x float> %451, <32 x float> undef, <32 x i32> zeroinitializer
  %453 = shl nsw i64 %indvars.iv.1, 5
  %454 = add nsw i64 %60, %453
  %455 = getelementptr inbounds float, float* %9, i64 %454
  %456 = bitcast float* %455 to <32 x float>*
  %457 = load <32 x float>, <32 x float>* %456, align 64, !tbaa !472
  %458 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %452, <32 x float> %457, <32 x float> %447)
  %459 = add nsw i64 %448, 256
  %460 = getelementptr inbounds float, float* %6, i64 %459
  %461 = load float, float* %460, align 4, !tbaa !459
  %462 = insertelement <32 x float> undef, float %461, i32 0
  %463 = shufflevector <32 x float> %462, <32 x float> undef, <32 x i32> zeroinitializer
  %464 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %463, <32 x float> %457, <32 x float> %446)
  %465 = add nsw i64 %448, 512
  %466 = getelementptr inbounds float, float* %6, i64 %465
  %467 = load float, float* %466, align 4, !tbaa !459
  %468 = insertelement <32 x float> undef, float %467, i32 0
  %469 = shufflevector <32 x float> %468, <32 x float> undef, <32 x i32> zeroinitializer
  %470 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %457, <32 x float> %445)
  %471 = add nsw i64 %448, 768
  %472 = getelementptr inbounds float, float* %6, i64 %471
  %473 = load float, float* %472, align 4, !tbaa !459
  %474 = insertelement <32 x float> undef, float %473, i32 0
  %475 = shufflevector <32 x float> %474, <32 x float> undef, <32 x i32> zeroinitializer
  %476 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %457, <32 x float> %444)
  %477 = add nsw i64 %448, 1024
  %478 = getelementptr inbounds float, float* %6, i64 %477
  %479 = load float, float* %478, align 4, !tbaa !459
  %480 = insertelement <32 x float> undef, float %479, i32 0
  %481 = shufflevector <32 x float> %480, <32 x float> undef, <32 x i32> zeroinitializer
  %482 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %457, <32 x float> %443)
  %483 = add nsw i64 %448, 1280
  %484 = getelementptr inbounds float, float* %6, i64 %483
  %485 = load float, float* %484, align 4, !tbaa !459
  %486 = insertelement <32 x float> undef, float %485, i32 0
  %487 = shufflevector <32 x float> %486, <32 x float> undef, <32 x i32> zeroinitializer
  %488 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %457, <32 x float> %442)
  %489 = add nsw i64 %448, 1536
  %490 = getelementptr inbounds float, float* %6, i64 %489
  %491 = load float, float* %490, align 4, !tbaa !459
  %492 = insertelement <32 x float> undef, float %491, i32 0
  %493 = shufflevector <32 x float> %492, <32 x float> undef, <32 x i32> zeroinitializer
  %494 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %457, <32 x float> %441)
  %495 = add nsw i64 %454, 8192
  %496 = getelementptr inbounds float, float* %9, i64 %495
  %497 = bitcast float* %496 to <32 x float>*
  %498 = load <32 x float>, <32 x float>* %497, align 64, !tbaa !472
  %499 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %463, <32 x float> %498, <32 x float> %458)
  %500 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %498, <32 x float> %464)
  %501 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %498, <32 x float> %470)
  %502 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %498, <32 x float> %476)
  %503 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %498, <32 x float> %482)
  %504 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %498, <32 x float> %488)
  %505 = add nsw i64 %448, 1792
  %506 = getelementptr inbounds float, float* %6, i64 %505
  %507 = load float, float* %506, align 4, !tbaa !459
  %508 = insertelement <32 x float> undef, float %507, i32 0
  %509 = shufflevector <32 x float> %508, <32 x float> undef, <32 x i32> zeroinitializer
  %510 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %509, <32 x float> %498, <32 x float> %494)
  %511 = add nsw i64 %454, 16384
  %512 = getelementptr inbounds float, float* %9, i64 %511
  %513 = bitcast float* %512 to <32 x float>*
  %514 = load <32 x float>, <32 x float>* %513, align 64, !tbaa !472
  %515 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %514, <32 x float> %499)
  %516 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %514, <32 x float> %500)
  %517 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %514, <32 x float> %501)
  %518 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %514, <32 x float> %502)
  %519 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %514, <32 x float> %503)
  %520 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %509, <32 x float> %514, <32 x float> %504)
  %521 = add nsw i64 %448, 2048
  %522 = getelementptr inbounds float, float* %6, i64 %521
  %523 = load float, float* %522, align 4, !tbaa !459
  %524 = insertelement <32 x float> undef, float %523, i32 0
  %525 = shufflevector <32 x float> %524, <32 x float> undef, <32 x i32> zeroinitializer
  %526 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %525, <32 x float> %514, <32 x float> %510)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 256
  br i1 %exitcond.1, label %for_end9.1, label %for_body8.1, !prof !20

for_end9.1:                                       ; preds = %for_body8.1
  %527 = add nsw i64 %66, %63
  br label %for_body8.2

for_body8.2:                                      ; preds = %for_body8.2, %for_end9.1
  %indvars.iv.2 = phi i64 [ 0, %for_end9.1 ], [ %indvars.iv.next.2, %for_body8.2 ]
  %528 = phi <32 x float> [ %526, %for_end9.1 ], [ %613, %for_body8.2 ]
  %529 = phi <32 x float> [ %520, %for_end9.1 ], [ %607, %for_body8.2 ]
  %530 = phi <32 x float> [ %519, %for_end9.1 ], [ %606, %for_body8.2 ]
  %531 = phi <32 x float> [ %518, %for_end9.1 ], [ %605, %for_body8.2 ]
  %532 = phi <32 x float> [ %517, %for_end9.1 ], [ %604, %for_body8.2 ]
  %533 = phi <32 x float> [ %516, %for_end9.1 ], [ %603, %for_body8.2 ]
  %534 = phi <32 x float> [ %515, %for_end9.1 ], [ %602, %for_body8.2 ]
  %535 = add nsw i64 %527, %indvars.iv.2
  %536 = getelementptr inbounds float, float* %6, i64 %535
  %537 = load float, float* %536, align 4, !tbaa !459
  %538 = insertelement <32 x float> undef, float %537, i32 0
  %539 = shufflevector <32 x float> %538, <32 x float> undef, <32 x i32> zeroinitializer
  %540 = shl nsw i64 %indvars.iv.2, 5
  %541 = add nsw i64 %64, %540
  %542 = getelementptr inbounds float, float* %9, i64 %541
  %543 = bitcast float* %542 to <32 x float>*
  %544 = load <32 x float>, <32 x float>* %543, align 64, !tbaa !472
  %545 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %539, <32 x float> %544, <32 x float> %534)
  %546 = add nsw i64 %535, 256
  %547 = getelementptr inbounds float, float* %6, i64 %546
  %548 = load float, float* %547, align 4, !tbaa !459
  %549 = insertelement <32 x float> undef, float %548, i32 0
  %550 = shufflevector <32 x float> %549, <32 x float> undef, <32 x i32> zeroinitializer
  %551 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %550, <32 x float> %544, <32 x float> %533)
  %552 = add nsw i64 %535, 512
  %553 = getelementptr inbounds float, float* %6, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !459
  %555 = insertelement <32 x float> undef, float %554, i32 0
  %556 = shufflevector <32 x float> %555, <32 x float> undef, <32 x i32> zeroinitializer
  %557 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %556, <32 x float> %544, <32 x float> %532)
  %558 = add nsw i64 %535, 768
  %559 = getelementptr inbounds float, float* %6, i64 %558
  %560 = load float, float* %559, align 4, !tbaa !459
  %561 = insertelement <32 x float> undef, float %560, i32 0
  %562 = shufflevector <32 x float> %561, <32 x float> undef, <32 x i32> zeroinitializer
  %563 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %562, <32 x float> %544, <32 x float> %531)
  %564 = add nsw i64 %535, 1024
  %565 = getelementptr inbounds float, float* %6, i64 %564
  %566 = load float, float* %565, align 4, !tbaa !459
  %567 = insertelement <32 x float> undef, float %566, i32 0
  %568 = shufflevector <32 x float> %567, <32 x float> undef, <32 x i32> zeroinitializer
  %569 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %544, <32 x float> %530)
  %570 = add nsw i64 %535, 1280
  %571 = getelementptr inbounds float, float* %6, i64 %570
  %572 = load float, float* %571, align 4, !tbaa !459
  %573 = insertelement <32 x float> undef, float %572, i32 0
  %574 = shufflevector <32 x float> %573, <32 x float> undef, <32 x i32> zeroinitializer
  %575 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %544, <32 x float> %529)
  %576 = add nsw i64 %535, 1536
  %577 = getelementptr inbounds float, float* %6, i64 %576
  %578 = load float, float* %577, align 4, !tbaa !459
  %579 = insertelement <32 x float> undef, float %578, i32 0
  %580 = shufflevector <32 x float> %579, <32 x float> undef, <32 x i32> zeroinitializer
  %581 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %544, <32 x float> %528)
  %582 = add nsw i64 %541, 8192
  %583 = getelementptr inbounds float, float* %9, i64 %582
  %584 = bitcast float* %583 to <32 x float>*
  %585 = load <32 x float>, <32 x float>* %584, align 64, !tbaa !472
  %586 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %550, <32 x float> %585, <32 x float> %545)
  %587 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %556, <32 x float> %585, <32 x float> %551)
  %588 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %562, <32 x float> %585, <32 x float> %557)
  %589 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %585, <32 x float> %563)
  %590 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %585, <32 x float> %569)
  %591 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %585, <32 x float> %575)
  %592 = add nsw i64 %535, 1792
  %593 = getelementptr inbounds float, float* %6, i64 %592
  %594 = load float, float* %593, align 4, !tbaa !459
  %595 = insertelement <32 x float> undef, float %594, i32 0
  %596 = shufflevector <32 x float> %595, <32 x float> undef, <32 x i32> zeroinitializer
  %597 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %596, <32 x float> %585, <32 x float> %581)
  %598 = add nsw i64 %541, 16384
  %599 = getelementptr inbounds float, float* %9, i64 %598
  %600 = bitcast float* %599 to <32 x float>*
  %601 = load <32 x float>, <32 x float>* %600, align 64, !tbaa !472
  %602 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %556, <32 x float> %601, <32 x float> %586)
  %603 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %562, <32 x float> %601, <32 x float> %587)
  %604 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %601, <32 x float> %588)
  %605 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %601, <32 x float> %589)
  %606 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %601, <32 x float> %590)
  %607 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %596, <32 x float> %601, <32 x float> %591)
  %608 = add nsw i64 %535, 2048
  %609 = getelementptr inbounds float, float* %6, i64 %608
  %610 = load float, float* %609, align 4, !tbaa !459
  %611 = insertelement <32 x float> undef, float %610, i32 0
  %612 = shufflevector <32 x float> %611, <32 x float> undef, <32 x i32> zeroinitializer
  %613 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %612, <32 x float> %601, <32 x float> %597)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 256
  br i1 %exitcond.2, label %for_end9.2, label %for_body8.2, !prof !20

for_end9.2:                                       ; preds = %for_body8.2
  store <32 x float> %602, <32 x float>* %.sub, align 128, !tbaa !475
  store <32 x float> %603, <32 x float>* %32, align 128, !tbaa !475
  store <32 x float> %604, <32 x float>* %34, align 128, !tbaa !475
  store <32 x float> %605, <32 x float>* %36, align 128, !tbaa !475
  store <32 x float> %606, <32 x float>* %38, align 128, !tbaa !475
  store <32 x float> %607, <32 x float>* %40, align 128, !tbaa !475
  store <32 x float> %613, <32 x float>* %42, align 128, !tbaa !475
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep, i8* nonnull %4, i64 896, i32 64, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond57 = icmp eq i64 %indvar.next, 4
  br i1 %exitcond57, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_nn_max_pool2d_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !484 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !486, metadata !DIExpression()), !dbg !489
  call void @llvm.dbg.value(metadata i8* %1, metadata !487, metadata !DIExpression()), !dbg !489
  call void @llvm.dbg.value(metadata i32 %2, metadata !488, metadata !DIExpression()), !dbg !489
  %3 = bitcast i8* %0 to %1**, !dbg !489
  %4 = load %1*, %1** %3, align 8, !dbg !489
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !489
  %6 = bitcast i8* %5 to %1**, !dbg !489
  %7 = load %1*, %1** %6, align 8, !dbg !489
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !489
  %9 = load i8*, i8** %8, align 8, !dbg !489
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !489
  %11 = load i8*, i8** %10, align 8, !dbg !489
  %12 = tail call fastcc i32 @fused_nn_max_pool2d_2_compute_(i8* %11, i8* %9), !dbg !489
  ret i32 %12, !dbg !489
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_max_pool2d_2_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %33, align 8
  %3 = getelementptr inbounds %33, %33* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %33, %33* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %33* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.29, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.29(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
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
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv7 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next8, %for_end3 ]
  %24 = mul nsw i64 %indvars.iv7, 896
  %25 = trunc i64 %indvars.iv7 to i32
  %26 = mul i32 %25, 3584
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %27 = shl i64 %indvars.iv, 5
  %28 = add nsw i64 %27, %24
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <32 x float>*
  %indvars.iv.tr = trunc i64 %indvars.iv to i32
  %31 = shl i32 %indvars.iv.tr, 6
  %32 = add i32 %31, %26
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, float* %7, i64 %33
  %35 = bitcast float* %34 to <32 x float>*
  %36 = load <32 x float>, <32 x float>* %35, align 64, !tbaa !490
  %37 = fcmp olt <32 x float> %36, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %38 = select <32 x i1> %37, <32 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <32 x float> %36
  %39 = or i32 %32, 32
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to <32 x float>*
  %43 = load <32 x float>, <32 x float>* %42, align 64, !tbaa !490
  %44 = fcmp ogt <32 x float> %38, %43
  %45 = select <32 x i1> %44, <32 x float> %38, <32 x float> %43
  %46 = add i32 %32, 1792
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to <32 x float>*
  %50 = load <32 x float>, <32 x float>* %49, align 64, !tbaa !490
  %51 = fcmp ogt <32 x float> %45, %50
  %52 = select <32 x i1> %51, <32 x float> %45, <32 x float> %50
  %53 = or i32 %46, 32
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, float* %7, i64 %54
  %56 = bitcast float* %55 to <32 x float>*
  %57 = load <32 x float>, <32 x float>* %56, align 64, !tbaa !490
  %58 = fcmp ogt <32 x float> %52, %57
  %59 = select <32 x i1> %58, <32 x float> %52, <32 x float> %57
  store <32 x float> %59, <32 x float>* %30, align 64, !tbaa !493
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 28
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %60 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %60, label %for_body, label %for_end, !prof !19
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_batch_flatten_nn_batch_flatten_multiply(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #3 !dbg !496 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !498, metadata !DIExpression()), !dbg !501
  call void @llvm.dbg.value(metadata i8* %1, metadata !499, metadata !DIExpression()), !dbg !501
  call void @llvm.dbg.value(metadata i32 %2, metadata !500, metadata !DIExpression()), !dbg !501
  %3 = bitcast i8* %0 to %1**, !dbg !501
  %4 = load %1*, %1** %3, align 8, !dbg !501
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !501
  %6 = bitcast i8* %5 to %1**, !dbg !501
  %7 = load %1*, %1** %6, align 8, !dbg !501
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !501
  %9 = load i8*, i8** %8, align 8, !dbg !501
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !501
  %11 = load i8*, i8** %10, align 8, !dbg !501
  tail call fastcc void @fused_nn_batch_flatten_nn_batch_flatten_multiply_compute_(i8* %11, i8* %9), !dbg !501
  ret i32 0, !dbg !501
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_batch_flatten_nn_batch_flatten_multiply_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #4 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16384, i32 4, i1 false)
  ret void
}

define dllexport i32 @fused_layout_transform_13(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !502 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !504, metadata !DIExpression()), !dbg !507
  call void @llvm.dbg.value(metadata i8* %1, metadata !505, metadata !DIExpression()), !dbg !507
  call void @llvm.dbg.value(metadata i32 %2, metadata !506, metadata !DIExpression()), !dbg !507
  %3 = bitcast i8* %0 to %1**, !dbg !507
  %4 = load %1*, %1** %3, align 8, !dbg !507
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !507
  %6 = bitcast i8* %5 to %1**, !dbg !507
  %7 = load %1*, %1** %6, align 8, !dbg !507
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !507
  %9 = load i8*, i8** %8, align 8, !dbg !507
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !507
  %11 = load i8*, i8** %10, align 8, !dbg !507
  %12 = tail call fastcc i32 @fused_layout_transform_13_compute_(i8* %11, i8* %9), !dbg !507
  ret i32 %12, !dbg !507
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_13_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %34, align 8
  %3 = getelementptr inbounds %34, %34* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %34, %34* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %34* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.30, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.30(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 13
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 14
  %15 = select i1 %14, i32 %13, i32 14
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 14
  %18 = select i1 %17, i32 %16, i32 14
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = add i32 %18, 1
  %21 = sext i32 %20 to i64
  %22 = add nsw i64 %21, -1
  %23 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end6.13
  %indvars.iv10 = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next11, %for_end6.13 ]
  %24 = mul nsw i64 %indvars.iv10, 7168
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = mul i32 %25, 224
  br label %for_body5

for_end:                                          ; preds = %for_end6.13, %entry
  ret i32 0

for_body5:                                        ; preds = %for_body5, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body5 ]
  %27 = add nsw i64 %24, %indvars.iv
  %28 = trunc i64 %indvars.iv to i32
  %29 = and i32 %28, 15
  %30 = lshr i32 %28, 4
  %31 = mul nsw i32 %30, 3136
  %32 = add i32 %26, %31
  %33 = or i32 %32, %29
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to i32*
  %37 = load i32, i32* %36, align 4, !tbaa !508
  %38 = getelementptr inbounds float, float* %4, i64 %27
  %39 = bitcast float* %38 to i32*
  store i32 %37, i32* %39, align 4, !tbaa !511
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %40 = or i64 %24, 512
  %41 = or i32 %26, 16
  br label %for_body5.1

for_body5.1:                                      ; preds = %for_body5.1, %for_end6
  %indvars.iv.1 = phi i64 [ 0, %for_end6 ], [ %indvars.iv.next.1, %for_body5.1 ]
  %42 = add nsw i64 %40, %indvars.iv.1
  %43 = trunc i64 %indvars.iv.1 to i32
  %44 = and i32 %43, 15
  %45 = lshr i32 %43, 4
  %46 = mul nsw i32 %45, 3136
  %47 = add i32 %41, %46
  %48 = or i32 %47, %44
  %49 = sext i32 %48 to i64
  %50 = getelementptr inbounds float, float* %7, i64 %49
  %51 = bitcast float* %50 to i32*
  %52 = load i32, i32* %51, align 4, !tbaa !508
  %53 = getelementptr inbounds float, float* %4, i64 %42
  %54 = bitcast float* %53 to i32*
  store i32 %52, i32* %54, align 4, !tbaa !511
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 512
  br i1 %exitcond.1, label %for_end6.1, label %for_body5.1, !prof !20

for_end6.1:                                       ; preds = %for_body5.1
  %55 = add nsw i64 %24, 1024
  %56 = add i32 %26, 32
  br label %for_body5.2

for_body5.2:                                      ; preds = %for_body5.2, %for_end6.1
  %indvars.iv.2 = phi i64 [ 0, %for_end6.1 ], [ %indvars.iv.next.2, %for_body5.2 ]
  %57 = add nsw i64 %55, %indvars.iv.2
  %58 = trunc i64 %indvars.iv.2 to i32
  %59 = and i32 %58, 15
  %60 = lshr i32 %58, 4
  %61 = mul nsw i32 %60, 3136
  %62 = add i32 %56, %61
  %63 = or i32 %62, %59
  %64 = sext i32 %63 to i64
  %65 = getelementptr inbounds float, float* %7, i64 %64
  %66 = bitcast float* %65 to i32*
  %67 = load i32, i32* %66, align 4, !tbaa !508
  %68 = getelementptr inbounds float, float* %4, i64 %57
  %69 = bitcast float* %68 to i32*
  store i32 %67, i32* %69, align 4, !tbaa !511
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 512
  br i1 %exitcond.2, label %for_end6.2, label %for_body5.2, !prof !20

for_end6.2:                                       ; preds = %for_body5.2
  %70 = add nsw i64 %24, 1536
  %71 = add i32 %26, 48
  br label %for_body5.3

for_body5.3:                                      ; preds = %for_body5.3, %for_end6.2
  %indvars.iv.3 = phi i64 [ 0, %for_end6.2 ], [ %indvars.iv.next.3, %for_body5.3 ]
  %72 = add nsw i64 %70, %indvars.iv.3
  %73 = trunc i64 %indvars.iv.3 to i32
  %74 = and i32 %73, 15
  %75 = lshr i32 %73, 4
  %76 = mul nsw i32 %75, 3136
  %77 = add i32 %71, %76
  %78 = or i32 %77, %74
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !508
  %83 = getelementptr inbounds float, float* %4, i64 %72
  %84 = bitcast float* %83 to i32*
  store i32 %82, i32* %84, align 4, !tbaa !511
  %indvars.iv.next.3 = add nuw nsw i64 %indvars.iv.3, 1
  %exitcond.3 = icmp eq i64 %indvars.iv.next.3, 512
  br i1 %exitcond.3, label %for_end6.3, label %for_body5.3, !prof !20

for_end6.3:                                       ; preds = %for_body5.3
  %85 = add nsw i64 %24, 2048
  %86 = add i32 %26, 64
  br label %for_body5.4

for_body5.4:                                      ; preds = %for_body5.4, %for_end6.3
  %indvars.iv.4 = phi i64 [ 0, %for_end6.3 ], [ %indvars.iv.next.4, %for_body5.4 ]
  %87 = add nsw i64 %85, %indvars.iv.4
  %88 = trunc i64 %indvars.iv.4 to i32
  %89 = and i32 %88, 15
  %90 = lshr i32 %88, 4
  %91 = mul nsw i32 %90, 3136
  %92 = add i32 %86, %91
  %93 = or i32 %92, %89
  %94 = sext i32 %93 to i64
  %95 = getelementptr inbounds float, float* %7, i64 %94
  %96 = bitcast float* %95 to i32*
  %97 = load i32, i32* %96, align 4, !tbaa !508
  %98 = getelementptr inbounds float, float* %4, i64 %87
  %99 = bitcast float* %98 to i32*
  store i32 %97, i32* %99, align 4, !tbaa !511
  %indvars.iv.next.4 = add nuw nsw i64 %indvars.iv.4, 1
  %exitcond.4 = icmp eq i64 %indvars.iv.next.4, 512
  br i1 %exitcond.4, label %for_end6.4, label %for_body5.4, !prof !20

for_end6.4:                                       ; preds = %for_body5.4
  %100 = add nsw i64 %24, 2560
  %101 = add i32 %26, 80
  br label %for_body5.5

for_body5.5:                                      ; preds = %for_body5.5, %for_end6.4
  %indvars.iv.5 = phi i64 [ 0, %for_end6.4 ], [ %indvars.iv.next.5, %for_body5.5 ]
  %102 = add nsw i64 %100, %indvars.iv.5
  %103 = trunc i64 %indvars.iv.5 to i32
  %104 = and i32 %103, 15
  %105 = lshr i32 %103, 4
  %106 = mul nsw i32 %105, 3136
  %107 = add i32 %101, %106
  %108 = or i32 %107, %104
  %109 = sext i32 %108 to i64
  %110 = getelementptr inbounds float, float* %7, i64 %109
  %111 = bitcast float* %110 to i32*
  %112 = load i32, i32* %111, align 4, !tbaa !508
  %113 = getelementptr inbounds float, float* %4, i64 %102
  %114 = bitcast float* %113 to i32*
  store i32 %112, i32* %114, align 4, !tbaa !511
  %indvars.iv.next.5 = add nuw nsw i64 %indvars.iv.5, 1
  %exitcond.5 = icmp eq i64 %indvars.iv.next.5, 512
  br i1 %exitcond.5, label %for_end6.5, label %for_body5.5, !prof !20

for_end6.5:                                       ; preds = %for_body5.5
  %115 = add nsw i64 %24, 3072
  %116 = add i32 %26, 96
  br label %for_body5.6

for_body5.6:                                      ; preds = %for_body5.6, %for_end6.5
  %indvars.iv.6 = phi i64 [ 0, %for_end6.5 ], [ %indvars.iv.next.6, %for_body5.6 ]
  %117 = add nsw i64 %115, %indvars.iv.6
  %118 = trunc i64 %indvars.iv.6 to i32
  %119 = and i32 %118, 15
  %120 = lshr i32 %118, 4
  %121 = mul nsw i32 %120, 3136
  %122 = add i32 %116, %121
  %123 = or i32 %122, %119
  %124 = sext i32 %123 to i64
  %125 = getelementptr inbounds float, float* %7, i64 %124
  %126 = bitcast float* %125 to i32*
  %127 = load i32, i32* %126, align 4, !tbaa !508
  %128 = getelementptr inbounds float, float* %4, i64 %117
  %129 = bitcast float* %128 to i32*
  store i32 %127, i32* %129, align 4, !tbaa !511
  %indvars.iv.next.6 = add nuw nsw i64 %indvars.iv.6, 1
  %exitcond.6 = icmp eq i64 %indvars.iv.next.6, 512
  br i1 %exitcond.6, label %for_end6.6, label %for_body5.6, !prof !20

for_end6.6:                                       ; preds = %for_body5.6
  %130 = add nsw i64 %24, 3584
  %131 = add i32 %26, 112
  br label %for_body5.7

for_body5.7:                                      ; preds = %for_body5.7, %for_end6.6
  %indvars.iv.7 = phi i64 [ 0, %for_end6.6 ], [ %indvars.iv.next.7, %for_body5.7 ]
  %132 = add nsw i64 %130, %indvars.iv.7
  %133 = trunc i64 %indvars.iv.7 to i32
  %134 = and i32 %133, 15
  %135 = lshr i32 %133, 4
  %136 = mul nsw i32 %135, 3136
  %137 = add i32 %131, %136
  %138 = or i32 %137, %134
  %139 = sext i32 %138 to i64
  %140 = getelementptr inbounds float, float* %7, i64 %139
  %141 = bitcast float* %140 to i32*
  %142 = load i32, i32* %141, align 4, !tbaa !508
  %143 = getelementptr inbounds float, float* %4, i64 %132
  %144 = bitcast float* %143 to i32*
  store i32 %142, i32* %144, align 4, !tbaa !511
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv.7, 1
  %exitcond.7 = icmp eq i64 %indvars.iv.next.7, 512
  br i1 %exitcond.7, label %for_end6.7, label %for_body5.7, !prof !20

for_end6.7:                                       ; preds = %for_body5.7
  %145 = add nsw i64 %24, 4096
  %146 = add i32 %26, 128
  br label %for_body5.8

for_body5.8:                                      ; preds = %for_body5.8, %for_end6.7
  %indvars.iv.8 = phi i64 [ 0, %for_end6.7 ], [ %indvars.iv.next.8, %for_body5.8 ]
  %147 = add nsw i64 %145, %indvars.iv.8
  %148 = trunc i64 %indvars.iv.8 to i32
  %149 = and i32 %148, 15
  %150 = lshr i32 %148, 4
  %151 = mul nsw i32 %150, 3136
  %152 = add i32 %146, %151
  %153 = or i32 %152, %149
  %154 = sext i32 %153 to i64
  %155 = getelementptr inbounds float, float* %7, i64 %154
  %156 = bitcast float* %155 to i32*
  %157 = load i32, i32* %156, align 4, !tbaa !508
  %158 = getelementptr inbounds float, float* %4, i64 %147
  %159 = bitcast float* %158 to i32*
  store i32 %157, i32* %159, align 4, !tbaa !511
  %indvars.iv.next.8 = add nuw nsw i64 %indvars.iv.8, 1
  %exitcond.8 = icmp eq i64 %indvars.iv.next.8, 512
  br i1 %exitcond.8, label %for_end6.8, label %for_body5.8, !prof !20

for_end6.8:                                       ; preds = %for_body5.8
  %160 = add nsw i64 %24, 4608
  %161 = add i32 %26, 144
  br label %for_body5.9

for_body5.9:                                      ; preds = %for_body5.9, %for_end6.8
  %indvars.iv.9 = phi i64 [ 0, %for_end6.8 ], [ %indvars.iv.next.9, %for_body5.9 ]
  %162 = add nsw i64 %160, %indvars.iv.9
  %163 = trunc i64 %indvars.iv.9 to i32
  %164 = and i32 %163, 15
  %165 = lshr i32 %163, 4
  %166 = mul nsw i32 %165, 3136
  %167 = add i32 %161, %166
  %168 = or i32 %167, %164
  %169 = sext i32 %168 to i64
  %170 = getelementptr inbounds float, float* %7, i64 %169
  %171 = bitcast float* %170 to i32*
  %172 = load i32, i32* %171, align 4, !tbaa !508
  %173 = getelementptr inbounds float, float* %4, i64 %162
  %174 = bitcast float* %173 to i32*
  store i32 %172, i32* %174, align 4, !tbaa !511
  %indvars.iv.next.9 = add nuw nsw i64 %indvars.iv.9, 1
  %exitcond.9 = icmp eq i64 %indvars.iv.next.9, 512
  br i1 %exitcond.9, label %for_end6.9, label %for_body5.9, !prof !20

for_end6.9:                                       ; preds = %for_body5.9
  %175 = add nsw i64 %24, 5120
  %176 = add i32 %26, 160
  br label %for_body5.10

for_body5.10:                                     ; preds = %for_body5.10, %for_end6.9
  %indvars.iv.10 = phi i64 [ 0, %for_end6.9 ], [ %indvars.iv.next.10, %for_body5.10 ]
  %177 = add nsw i64 %175, %indvars.iv.10
  %178 = trunc i64 %indvars.iv.10 to i32
  %179 = and i32 %178, 15
  %180 = lshr i32 %178, 4
  %181 = mul nsw i32 %180, 3136
  %182 = add i32 %176, %181
  %183 = or i32 %182, %179
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds float, float* %7, i64 %184
  %186 = bitcast float* %185 to i32*
  %187 = load i32, i32* %186, align 4, !tbaa !508
  %188 = getelementptr inbounds float, float* %4, i64 %177
  %189 = bitcast float* %188 to i32*
  store i32 %187, i32* %189, align 4, !tbaa !511
  %indvars.iv.next.10 = add nuw nsw i64 %indvars.iv.10, 1
  %exitcond.10 = icmp eq i64 %indvars.iv.next.10, 512
  br i1 %exitcond.10, label %for_end6.10, label %for_body5.10, !prof !20

for_end6.10:                                      ; preds = %for_body5.10
  %190 = add nsw i64 %24, 5632
  %191 = add i32 %26, 176
  br label %for_body5.11

for_body5.11:                                     ; preds = %for_body5.11, %for_end6.10
  %indvars.iv.11 = phi i64 [ 0, %for_end6.10 ], [ %indvars.iv.next.11, %for_body5.11 ]
  %192 = add nsw i64 %190, %indvars.iv.11
  %193 = trunc i64 %indvars.iv.11 to i32
  %194 = and i32 %193, 15
  %195 = lshr i32 %193, 4
  %196 = mul nsw i32 %195, 3136
  %197 = add i32 %191, %196
  %198 = or i32 %197, %194
  %199 = sext i32 %198 to i64
  %200 = getelementptr inbounds float, float* %7, i64 %199
  %201 = bitcast float* %200 to i32*
  %202 = load i32, i32* %201, align 4, !tbaa !508
  %203 = getelementptr inbounds float, float* %4, i64 %192
  %204 = bitcast float* %203 to i32*
  store i32 %202, i32* %204, align 4, !tbaa !511
  %indvars.iv.next.11 = add nuw nsw i64 %indvars.iv.11, 1
  %exitcond.11 = icmp eq i64 %indvars.iv.next.11, 512
  br i1 %exitcond.11, label %for_end6.11, label %for_body5.11, !prof !20

for_end6.11:                                      ; preds = %for_body5.11
  %205 = add nsw i64 %24, 6144
  %206 = add i32 %26, 192
  br label %for_body5.12

for_body5.12:                                     ; preds = %for_body5.12, %for_end6.11
  %indvars.iv.12 = phi i64 [ 0, %for_end6.11 ], [ %indvars.iv.next.12, %for_body5.12 ]
  %207 = add nsw i64 %205, %indvars.iv.12
  %208 = trunc i64 %indvars.iv.12 to i32
  %209 = and i32 %208, 15
  %210 = lshr i32 %208, 4
  %211 = mul nsw i32 %210, 3136
  %212 = add i32 %206, %211
  %213 = or i32 %212, %209
  %214 = sext i32 %213 to i64
  %215 = getelementptr inbounds float, float* %7, i64 %214
  %216 = bitcast float* %215 to i32*
  %217 = load i32, i32* %216, align 4, !tbaa !508
  %218 = getelementptr inbounds float, float* %4, i64 %207
  %219 = bitcast float* %218 to i32*
  store i32 %217, i32* %219, align 4, !tbaa !511
  %indvars.iv.next.12 = add nuw nsw i64 %indvars.iv.12, 1
  %exitcond.12 = icmp eq i64 %indvars.iv.next.12, 512
  br i1 %exitcond.12, label %for_end6.12, label %for_body5.12, !prof !20

for_end6.12:                                      ; preds = %for_body5.12
  %220 = add nsw i64 %24, 6656
  %221 = add i32 %26, 208
  br label %for_body5.13

for_body5.13:                                     ; preds = %for_body5.13, %for_end6.12
  %indvars.iv.13 = phi i64 [ 0, %for_end6.12 ], [ %indvars.iv.next.13, %for_body5.13 ]
  %222 = add nsw i64 %220, %indvars.iv.13
  %223 = trunc i64 %indvars.iv.13 to i32
  %224 = and i32 %223, 15
  %225 = lshr i32 %223, 4
  %226 = mul nsw i32 %225, 3136
  %227 = add i32 %221, %226
  %228 = or i32 %227, %224
  %229 = sext i32 %228 to i64
  %230 = getelementptr inbounds float, float* %7, i64 %229
  %231 = bitcast float* %230 to i32*
  %232 = load i32, i32* %231, align 4, !tbaa !508
  %233 = getelementptr inbounds float, float* %4, i64 %222
  %234 = bitcast float* %233 to i32*
  store i32 %232, i32* %234, align 4, !tbaa !511
  %indvars.iv.next.13 = add nuw nsw i64 %indvars.iv.13, 1
  %exitcond.13 = icmp eq i64 %indvars.iv.next.13, 512
  br i1 %exitcond.13, label %for_end6.13, label %for_body5.13, !prof !20

for_end6.13:                                      ; preds = %for_body5.13
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %235 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %235, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !514 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !516, metadata !DIExpression()), !dbg !519
  call void @llvm.dbg.value(metadata i8* %1, metadata !517, metadata !DIExpression()), !dbg !519
  call void @llvm.dbg.value(metadata i32 %2, metadata !518, metadata !DIExpression()), !dbg !519
  %3 = bitcast i8* %0 to %1**, !dbg !519
  %4 = load %1*, %1** %3, align 8, !dbg !519
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !519
  %6 = bitcast i8* %5 to %1**, !dbg !519
  %7 = load %1*, %1** %6, align 8, !dbg !519
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !519
  %9 = bitcast i8* %8 to %1**, !dbg !519
  %10 = load %1*, %1** %9, align 8, !dbg !519
  %11 = getelementptr inbounds i8, i8* %0, i64 24, !dbg !519
  %12 = bitcast i8* %11 to %1**, !dbg !519
  %13 = load %1*, %1** %12, align 8, !dbg !519
  %14 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !519
  %15 = load i8*, i8** %14, align 8, !dbg !519
  %16 = getelementptr inbounds %1, %1* %4, i64 0, i32 1, i32 1, !dbg !519
  %17 = load i32, i32* %16, align 4, !dbg !519
  %18 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !519
  %19 = load i8*, i8** %18, align 8, !dbg !519
  %20 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !519
  %21 = load i8*, i8** %20, align 8, !dbg !519
  %22 = getelementptr inbounds %1, %1* %13, i64 0, i32 0, !dbg !519
  %23 = load i8*, i8** %22, align 8, !dbg !519
  %24 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_compute_(i8* %15, i8* %19, i8* %23, i8* %21, i32 %17), !dbg !519
  ret i32 %24, !dbg !519
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_add_nn_relu_compute_(i8* noalias, i8* noalias, i8* noalias, i8* noalias, i32) unnamed_addr #0 {
entry:
  %5 = load i8* (i32, i32, i64, i32, i32)*, i8* (i32, i32, i64, i32, i32)** @__TVMBackendAllocWorkspace, align 8, !tbaa !16
  %6 = tail call i8* %5(i32 1, i32 %4, i64 524288, i32 2, i32 32)
  %7 = alloca %35, align 8
  %8 = getelementptr inbounds %35, %35* %7, i64 0, i32 0
  store i8* %6, i8** %8, align 8
  %9 = getelementptr inbounds %35, %35* %7, i64 0, i32 1
  store i8* %0, i8** %9, align 8
  %10 = bitcast %35* %7 to i8*
  %11 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %12 = call i32 %11(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.31, i8* nonnull %10, i32 0)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %call_end2, %entry
  %merge = phi i32 [ %12, %entry ], [ 0, %call_end2 ], [ %21, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %14 = alloca %36, align 8
  %15 = getelementptr inbounds %36, %36* %14, i64 0, i32 0
  store i8* %6, i8** %15, align 8
  %16 = getelementptr inbounds %36, %36* %14, i64 0, i32 1
  store i8* %1, i8** %16, align 8
  %17 = getelementptr inbounds %36, %36* %14, i64 0, i32 2
  store i8* %2, i8** %17, align 8
  %18 = getelementptr inbounds %36, %36* %14, i64 0, i32 3
  store i8* %3, i8** %18, align 8
  %19 = bitcast %36* %14 to i8*
  %20 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %21 = call i32 %20(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.32, i8* nonnull %19, i32 0)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %call_end2, label %call_fail, !prof !19

call_end2:                                        ; preds = %call_end
  %23 = load i32 (i32, i32, i8*)*, i32 (i32, i32, i8*)** @__TVMBackendFreeWorkspace, align 8, !tbaa !16
  %24 = call i32 %23(i32 1, i32 %4, i8* %6)
  br label %call_fail
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.31(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 15
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 16
  %15 = select i1 %14, i32 %13, i32 16
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 16
  %18 = select i1 %17, i32 %16, i32 16
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.preheader, label %for_end, !prof !19

for_body.preheader:                               ; preds = %entry
  br label %for_body

for_body:                                         ; preds = %for_body.preheader, %for_end3
  %20 = phi i32 [ %62, %for_end3 ], [ %18, %for_body.preheader ]
  %21 = shl i32 %20, 13
  %.off = add i32 %20, -1
  %22 = icmp ult i32 %.off, 14
  %23 = mul nsw i32 %20, 7168
  br i1 %22, label %for_body2.us.preheader, label %vector.body385.preheader

vector.body385.preheader:                         ; preds = %for_body
  br label %vector.body385

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %for_end6.us
  %indvars.iv24 = phi i64 [ %indvars.iv.next25, %for_end6.us ], [ 0, %for_body2.us.preheader ]
  %24 = shl nsw i64 %indvars.iv24, 9
  %25 = trunc i64 %indvars.iv24 to i32
  %26 = add i32 %25, -1
  %27 = icmp ult i32 %26, 14
  br i1 %27, label %vector.body.preheader, label %vector.body37.preheader

vector.body37.preheader:                          ; preds = %for_body2.us
  br label %vector.body37

vector.body.preheader:                            ; preds = %for_body2.us
  br label %vector.body

for_end6.us:                                      ; preds = %vector.body37, %vector.body
  %indvars.iv.next25 = add nuw nsw i64 %indvars.iv24, 1
  %exitcond27 = icmp eq i64 %indvars.iv.next25, 16
  br i1 %exitcond27, label %for_end3, label %for_body2.us, !prof !20

vector.body:                                      ; preds = %vector.body.preheader, %vector.body
  %index = phi i64 [ %index.next, %vector.body ], [ 0, %vector.body.preheader ]
  %28 = add nuw nsw i64 %index, %24
  %29 = trunc i64 %28 to i32
  %30 = add i32 %21, %29
  %31 = trunc i64 %28 to i32
  %32 = add i32 %31, -7680
  %33 = add i32 %32, %23
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to <4 x i32>*
  %wide.load = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !520
  %37 = getelementptr float, float* %35, i64 4
  %38 = bitcast float* %37 to <4 x i32>*
  %wide.load36 = load <4 x i32>, <4 x i32>* %38, align 4, !tbaa !520
  %39 = sext i32 %30 to i64
  %40 = getelementptr inbounds float, float* %4, i64 %39
  %41 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %wide.load, <4 x i32>* %41, align 4, !tbaa !523
  %42 = getelementptr float, float* %40, i64 4
  %43 = bitcast float* %42 to <4 x i32>*
  store <4 x i32> %wide.load36, <4 x i32>* %43, align 4, !tbaa !523
  %index.next = add i64 %index, 8
  %44 = icmp eq i64 %index.next, 512
  br i1 %44, label %for_end6.us, label %vector.body, !llvm.loop !526

vector.body37:                                    ; preds = %vector.body37.preheader, %vector.body37
  %index47 = phi i64 [ %index.next48, %vector.body37 ], [ 0, %vector.body37.preheader ]
  %45 = add nuw nsw i64 %index47, %24
  %46 = trunc i64 %45 to i32
  %47 = add i32 %21, %46
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds float, float* %4, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %50, align 4, !tbaa !523
  %51 = getelementptr float, float* %49, i64 4
  %52 = bitcast float* %51 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %52, align 4, !tbaa !523
  %index.next48 = add i64 %index47, 8
  %53 = icmp eq i64 %index.next48, 512
  br i1 %53, label %for_end6.us, label %vector.body37, !llvm.loop !527

vector.body385:                                   ; preds = %vector.body385.preheader, %vector.body385
  %index395 = phi i64 [ %index.next396, %vector.body385 ], [ 0, %vector.body385.preheader ]
  %54 = trunc i64 %index395 to i32
  %55 = add i32 %21, %54
  %56 = sext i32 %55 to i64
  %57 = getelementptr inbounds float, float* %4, i64 %56
  %58 = bitcast float* %57 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %58, align 4, !tbaa !523
  %59 = getelementptr float, float* %57, i64 4
  %60 = bitcast float* %59 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %60, align 4, !tbaa !523
  %index.next396 = add i64 %index395, 8
  %61 = icmp eq i64 %index.next396, 512
  br i1 %61, label %vector.body363.preheader, label %vector.body385, !llvm.loop !528

vector.body363.preheader:                         ; preds = %vector.body385
  br label %vector.body363

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_end3:                                         ; preds = %vector.body55, %for_end6.us
  %62 = add nsw i32 %20, 1
  %63 = icmp slt i32 %62, %15
  br i1 %63, label %for_body, label %for_end, !prof !19

vector.body363:                                   ; preds = %vector.body363.preheader, %vector.body363
  %index373 = phi i64 [ %index.next374, %vector.body363 ], [ 0, %vector.body363.preheader ]
  %64 = trunc i64 %index373 to i32
  %65 = add i32 %64, 512
  %66 = add i32 %65, %21
  %67 = sext i32 %66 to i64
  %68 = getelementptr inbounds float, float* %4, i64 %67
  %69 = bitcast float* %68 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %69, align 4, !tbaa !523
  %70 = getelementptr float, float* %68, i64 4
  %71 = bitcast float* %70 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %71, align 4, !tbaa !523
  %index.next374 = add i64 %index373, 8
  %72 = icmp eq i64 %index.next374, 512
  br i1 %72, label %vector.body341.preheader, label %vector.body363, !llvm.loop !529

vector.body341.preheader:                         ; preds = %vector.body363
  br label %vector.body341

vector.body341:                                   ; preds = %vector.body341.preheader, %vector.body341
  %index351 = phi i64 [ %index.next352, %vector.body341 ], [ 0, %vector.body341.preheader ]
  %73 = trunc i64 %index351 to i32
  %74 = add i32 %73, 1024
  %75 = add i32 %74, %21
  %76 = sext i32 %75 to i64
  %77 = getelementptr inbounds float, float* %4, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %78, align 4, !tbaa !523
  %79 = getelementptr float, float* %77, i64 4
  %80 = bitcast float* %79 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %80, align 4, !tbaa !523
  %index.next352 = add i64 %index351, 8
  %81 = icmp eq i64 %index.next352, 512
  br i1 %81, label %vector.body319.preheader, label %vector.body341, !llvm.loop !530

vector.body319.preheader:                         ; preds = %vector.body341
  br label %vector.body319

vector.body319:                                   ; preds = %vector.body319.preheader, %vector.body319
  %index329 = phi i64 [ %index.next330, %vector.body319 ], [ 0, %vector.body319.preheader ]
  %82 = trunc i64 %index329 to i32
  %83 = add i32 %82, 1536
  %84 = add i32 %83, %21
  %85 = sext i32 %84 to i64
  %86 = getelementptr inbounds float, float* %4, i64 %85
  %87 = bitcast float* %86 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %87, align 4, !tbaa !523
  %88 = getelementptr float, float* %86, i64 4
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %89, align 4, !tbaa !523
  %index.next330 = add i64 %index329, 8
  %90 = icmp eq i64 %index.next330, 512
  br i1 %90, label %vector.body297.preheader, label %vector.body319, !llvm.loop !531

vector.body297.preheader:                         ; preds = %vector.body319
  br label %vector.body297

vector.body297:                                   ; preds = %vector.body297.preheader, %vector.body297
  %index307 = phi i64 [ %index.next308, %vector.body297 ], [ 0, %vector.body297.preheader ]
  %91 = trunc i64 %index307 to i32
  %92 = add i32 %91, 2048
  %93 = add i32 %92, %21
  %94 = sext i32 %93 to i64
  %95 = getelementptr inbounds float, float* %4, i64 %94
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %96, align 4, !tbaa !523
  %97 = getelementptr float, float* %95, i64 4
  %98 = bitcast float* %97 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %98, align 4, !tbaa !523
  %index.next308 = add i64 %index307, 8
  %99 = icmp eq i64 %index.next308, 512
  br i1 %99, label %vector.body275.preheader, label %vector.body297, !llvm.loop !532

vector.body275.preheader:                         ; preds = %vector.body297
  br label %vector.body275

vector.body275:                                   ; preds = %vector.body275.preheader, %vector.body275
  %index285 = phi i64 [ %index.next286, %vector.body275 ], [ 0, %vector.body275.preheader ]
  %100 = trunc i64 %index285 to i32
  %101 = add i32 %100, 2560
  %102 = add i32 %101, %21
  %103 = sext i32 %102 to i64
  %104 = getelementptr inbounds float, float* %4, i64 %103
  %105 = bitcast float* %104 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %105, align 4, !tbaa !523
  %106 = getelementptr float, float* %104, i64 4
  %107 = bitcast float* %106 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %107, align 4, !tbaa !523
  %index.next286 = add i64 %index285, 8
  %108 = icmp eq i64 %index.next286, 512
  br i1 %108, label %vector.body253.preheader, label %vector.body275, !llvm.loop !533

vector.body253.preheader:                         ; preds = %vector.body275
  br label %vector.body253

vector.body253:                                   ; preds = %vector.body253.preheader, %vector.body253
  %index263 = phi i64 [ %index.next264, %vector.body253 ], [ 0, %vector.body253.preheader ]
  %109 = trunc i64 %index263 to i32
  %110 = add i32 %109, 3072
  %111 = add i32 %110, %21
  %112 = sext i32 %111 to i64
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = bitcast float* %113 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %114, align 4, !tbaa !523
  %115 = getelementptr float, float* %113, i64 4
  %116 = bitcast float* %115 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %116, align 4, !tbaa !523
  %index.next264 = add i64 %index263, 8
  %117 = icmp eq i64 %index.next264, 512
  br i1 %117, label %vector.body231.preheader, label %vector.body253, !llvm.loop !534

vector.body231.preheader:                         ; preds = %vector.body253
  br label %vector.body231

vector.body231:                                   ; preds = %vector.body231.preheader, %vector.body231
  %index241 = phi i64 [ %index.next242, %vector.body231 ], [ 0, %vector.body231.preheader ]
  %118 = trunc i64 %index241 to i32
  %119 = add i32 %118, 3584
  %120 = add i32 %119, %21
  %121 = sext i32 %120 to i64
  %122 = getelementptr inbounds float, float* %4, i64 %121
  %123 = bitcast float* %122 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %123, align 4, !tbaa !523
  %124 = getelementptr float, float* %122, i64 4
  %125 = bitcast float* %124 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %125, align 4, !tbaa !523
  %index.next242 = add i64 %index241, 8
  %126 = icmp eq i64 %index.next242, 512
  br i1 %126, label %vector.body209.preheader, label %vector.body231, !llvm.loop !535

vector.body209.preheader:                         ; preds = %vector.body231
  br label %vector.body209

vector.body209:                                   ; preds = %vector.body209.preheader, %vector.body209
  %index219 = phi i64 [ %index.next220, %vector.body209 ], [ 0, %vector.body209.preheader ]
  %127 = trunc i64 %index219 to i32
  %128 = add i32 %127, 4096
  %129 = add i32 %128, %21
  %130 = sext i32 %129 to i64
  %131 = getelementptr inbounds float, float* %4, i64 %130
  %132 = bitcast float* %131 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %132, align 4, !tbaa !523
  %133 = getelementptr float, float* %131, i64 4
  %134 = bitcast float* %133 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %134, align 4, !tbaa !523
  %index.next220 = add i64 %index219, 8
  %135 = icmp eq i64 %index.next220, 512
  br i1 %135, label %vector.body187.preheader, label %vector.body209, !llvm.loop !536

vector.body187.preheader:                         ; preds = %vector.body209
  br label %vector.body187

vector.body187:                                   ; preds = %vector.body187.preheader, %vector.body187
  %index197 = phi i64 [ %index.next198, %vector.body187 ], [ 0, %vector.body187.preheader ]
  %136 = trunc i64 %index197 to i32
  %137 = add i32 %136, 4608
  %138 = add i32 %137, %21
  %139 = sext i32 %138 to i64
  %140 = getelementptr inbounds float, float* %4, i64 %139
  %141 = bitcast float* %140 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %141, align 4, !tbaa !523
  %142 = getelementptr float, float* %140, i64 4
  %143 = bitcast float* %142 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %143, align 4, !tbaa !523
  %index.next198 = add i64 %index197, 8
  %144 = icmp eq i64 %index.next198, 512
  br i1 %144, label %vector.body165.preheader, label %vector.body187, !llvm.loop !537

vector.body165.preheader:                         ; preds = %vector.body187
  br label %vector.body165

vector.body165:                                   ; preds = %vector.body165.preheader, %vector.body165
  %index175 = phi i64 [ %index.next176, %vector.body165 ], [ 0, %vector.body165.preheader ]
  %145 = trunc i64 %index175 to i32
  %146 = add i32 %145, 5120
  %147 = add i32 %146, %21
  %148 = sext i32 %147 to i64
  %149 = getelementptr inbounds float, float* %4, i64 %148
  %150 = bitcast float* %149 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %150, align 4, !tbaa !523
  %151 = getelementptr float, float* %149, i64 4
  %152 = bitcast float* %151 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %152, align 4, !tbaa !523
  %index.next176 = add i64 %index175, 8
  %153 = icmp eq i64 %index.next176, 512
  br i1 %153, label %vector.body143.preheader, label %vector.body165, !llvm.loop !538

vector.body143.preheader:                         ; preds = %vector.body165
  br label %vector.body143

vector.body143:                                   ; preds = %vector.body143.preheader, %vector.body143
  %index153 = phi i64 [ %index.next154, %vector.body143 ], [ 0, %vector.body143.preheader ]
  %154 = trunc i64 %index153 to i32
  %155 = add i32 %154, 5632
  %156 = add i32 %155, %21
  %157 = sext i32 %156 to i64
  %158 = getelementptr inbounds float, float* %4, i64 %157
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %159, align 4, !tbaa !523
  %160 = getelementptr float, float* %158, i64 4
  %161 = bitcast float* %160 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %161, align 4, !tbaa !523
  %index.next154 = add i64 %index153, 8
  %162 = icmp eq i64 %index.next154, 512
  br i1 %162, label %vector.body121.preheader, label %vector.body143, !llvm.loop !539

vector.body121.preheader:                         ; preds = %vector.body143
  br label %vector.body121

vector.body121:                                   ; preds = %vector.body121.preheader, %vector.body121
  %index131 = phi i64 [ %index.next132, %vector.body121 ], [ 0, %vector.body121.preheader ]
  %163 = trunc i64 %index131 to i32
  %164 = add i32 %163, 6144
  %165 = add i32 %164, %21
  %166 = sext i32 %165 to i64
  %167 = getelementptr inbounds float, float* %4, i64 %166
  %168 = bitcast float* %167 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %168, align 4, !tbaa !523
  %169 = getelementptr float, float* %167, i64 4
  %170 = bitcast float* %169 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %170, align 4, !tbaa !523
  %index.next132 = add i64 %index131, 8
  %171 = icmp eq i64 %index.next132, 512
  br i1 %171, label %vector.body99.preheader, label %vector.body121, !llvm.loop !540

vector.body99.preheader:                          ; preds = %vector.body121
  br label %vector.body99

vector.body99:                                    ; preds = %vector.body99.preheader, %vector.body99
  %index109 = phi i64 [ %index.next110, %vector.body99 ], [ 0, %vector.body99.preheader ]
  %172 = trunc i64 %index109 to i32
  %173 = add i32 %172, 6656
  %174 = add i32 %173, %21
  %175 = sext i32 %174 to i64
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = bitcast float* %176 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %177, align 4, !tbaa !523
  %178 = getelementptr float, float* %176, i64 4
  %179 = bitcast float* %178 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %179, align 4, !tbaa !523
  %index.next110 = add i64 %index109, 8
  %180 = icmp eq i64 %index.next110, 512
  br i1 %180, label %vector.body77.preheader, label %vector.body99, !llvm.loop !541

vector.body77.preheader:                          ; preds = %vector.body99
  br label %vector.body77

vector.body77:                                    ; preds = %vector.body77.preheader, %vector.body77
  %index87 = phi i64 [ %index.next88, %vector.body77 ], [ 0, %vector.body77.preheader ]
  %181 = trunc i64 %index87 to i32
  %182 = add i32 %181, 7168
  %183 = add i32 %182, %21
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds float, float* %4, i64 %184
  %186 = bitcast float* %185 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %186, align 4, !tbaa !523
  %187 = getelementptr float, float* %185, i64 4
  %188 = bitcast float* %187 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %188, align 4, !tbaa !523
  %index.next88 = add i64 %index87, 8
  %189 = icmp eq i64 %index.next88, 512
  br i1 %189, label %vector.body55.preheader, label %vector.body77, !llvm.loop !542

vector.body55.preheader:                          ; preds = %vector.body77
  br label %vector.body55

vector.body55:                                    ; preds = %vector.body55.preheader, %vector.body55
  %index65 = phi i64 [ %index.next66, %vector.body55 ], [ 0, %vector.body55.preheader ]
  %190 = trunc i64 %index65 to i32
  %191 = add i32 %190, 7680
  %192 = add i32 %191, %21
  %193 = sext i32 %192 to i64
  %194 = getelementptr inbounds float, float* %4, i64 %193
  %195 = bitcast float* %194 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %195, align 4, !tbaa !523
  %196 = getelementptr float, float* %194, i64 4
  %197 = bitcast float* %196 to <4 x float>*
  store <4 x float> zeroinitializer, <4 x float>* %197, align 4, !tbaa !523
  %index.next66 = add i64 %index65, 8
  %198 = icmp eq i64 %index.next66, 512
  br i1 %198, label %for_end3, label %vector.body55, !llvm.loop !543
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.32(i32, %0* nocapture readonly, i8* nocapture readonly) #3 {
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
  %16 = add nsw i32 %15, 447
  %17 = sdiv i32 %16, %15
  %18 = add nsw i32 %0, 1
  %19 = mul nsw i32 %17, %18
  %20 = icmp slt i32 %19, 448
  %21 = select i1 %20, i32 %19, i32 448
  %22 = mul nsw i32 %17, %0
  %23 = icmp slt i32 %22, 448
  %24 = select i1 %23, i32 %22, i32 448
  %25 = icmp slt i32 %24, %21
  br i1 %25, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %26 = add i32 %24, 1
  %27 = sext i32 %26 to i64
  %28 = add nsw i64 %27, -1
  %29 = sext i32 %21 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv81 = phi i64 [ %28, %for_body.lr.ph ], [ %indvars.iv.next82, %for_end3 ]
  %30 = trunc i64 %indvars.iv81 to i32
  %31 = srem i32 %30, 14
  %32 = sdiv i32 %30, 14
  %33 = mul nsw i32 %32, 73728
  %34 = sext i32 %33 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv75 = phi i64 [ 0, %for_body ], [ %indvars.iv.next76, %for_end6 ]
  %.lcssa3461 = phi <16 x float> [ zeroinitializer, %for_body ], [ %279, %for_end6 ]
  %.lcssa3259 = phi <16 x float> [ zeroinitializer, %for_body ], [ %273, %for_end6 ]
  %.lcssa3057 = phi <16 x float> [ zeroinitializer, %for_body ], [ %272, %for_end6 ]
  %.lcssa2855 = phi <16 x float> [ zeroinitializer, %for_body ], [ %271, %for_end6 ]
  %.lcssa2653 = phi <16 x float> [ zeroinitializer, %for_body ], [ %270, %for_end6 ]
  %.lcssa2451 = phi <16 x float> [ zeroinitializer, %for_body ], [ %269, %for_end6 ]
  %.lcssa2249 = phi <16 x float> [ zeroinitializer, %for_body ], [ %268, %for_end6 ]
  %.lcssa2047 = phi <16 x float> [ zeroinitializer, %for_body ], [ %267, %for_end6 ]
  %.lcssa1845 = phi <16 x float> [ zeroinitializer, %for_body ], [ %266, %for_end6 ]
  %.lcssa1643 = phi <16 x float> [ zeroinitializer, %for_body ], [ %265, %for_end6 ]
  %.lcssa1441 = phi <16 x float> [ zeroinitializer, %for_body ], [ %264, %for_end6 ]
  %.lcssa1239 = phi <16 x float> [ zeroinitializer, %for_body ], [ %263, %for_end6 ]
  %.lcssa1038 = phi <16 x float> [ zeroinitializer, %for_body ], [ %262, %for_end6 ]
  %.lcssa36 = phi <16 x float> [ zeroinitializer, %for_body ], [ %261, %for_end6 ]
  %35 = phi i32 [ 0, %for_body ], [ %280, %for_end6 ]
  %36 = add nsw i32 %35, %31
  %37 = shl i32 %36, 13
  %38 = mul nuw nsw i64 %indvars.iv75, 24576
  %39 = add nsw i64 %38, %34
  %40 = sext i32 %37 to i64
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %41 = mul nsw i64 %indvars.iv81, 224
  %42 = shl nsw i32 %32, 4
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds float, float* %13, i64 %43
  %45 = bitcast float* %44 to <16 x float>*
  %46 = load <16 x float>, <16 x float>* %45, align 64, !tbaa !544
  %47 = fadd <16 x float> %46, %261
  %48 = fcmp ogt <16 x float> %47, zeroinitializer
  %49 = select <16 x i1> %48, <16 x float> %47, <16 x float> zeroinitializer
  %50 = getelementptr inbounds float, float* %10, i64 %41
  %51 = bitcast float* %50 to <16 x float>*
  store <16 x float> %49, <16 x float>* %51, align 64, !tbaa !547
  %52 = or i64 %41, 16
  %53 = fadd <16 x float> %46, %262
  %54 = fcmp ogt <16 x float> %53, zeroinitializer
  %55 = select <16 x i1> %54, <16 x float> %53, <16 x float> zeroinitializer
  %56 = getelementptr inbounds float, float* %10, i64 %52
  %57 = bitcast float* %56 to <16 x float>*
  store <16 x float> %55, <16 x float>* %57, align 64, !tbaa !547
  %58 = add nsw i64 %41, 32
  %59 = fadd <16 x float> %46, %263
  %60 = fcmp ogt <16 x float> %59, zeroinitializer
  %61 = select <16 x i1> %60, <16 x float> %59, <16 x float> zeroinitializer
  %62 = getelementptr inbounds float, float* %10, i64 %58
  %63 = bitcast float* %62 to <16 x float>*
  store <16 x float> %61, <16 x float>* %63, align 64, !tbaa !547
  %64 = add nsw i64 %41, 48
  %65 = fadd <16 x float> %46, %264
  %66 = fcmp ogt <16 x float> %65, zeroinitializer
  %67 = select <16 x i1> %66, <16 x float> %65, <16 x float> zeroinitializer
  %68 = getelementptr inbounds float, float* %10, i64 %64
  %69 = bitcast float* %68 to <16 x float>*
  store <16 x float> %67, <16 x float>* %69, align 64, !tbaa !547
  %70 = add nsw i64 %41, 64
  %71 = fadd <16 x float> %46, %265
  %72 = fcmp ogt <16 x float> %71, zeroinitializer
  %73 = select <16 x i1> %72, <16 x float> %71, <16 x float> zeroinitializer
  %74 = getelementptr inbounds float, float* %10, i64 %70
  %75 = bitcast float* %74 to <16 x float>*
  store <16 x float> %73, <16 x float>* %75, align 64, !tbaa !547
  %76 = add nsw i64 %41, 80
  %77 = fadd <16 x float> %46, %266
  %78 = fcmp ogt <16 x float> %77, zeroinitializer
  %79 = select <16 x i1> %78, <16 x float> %77, <16 x float> zeroinitializer
  %80 = getelementptr inbounds float, float* %10, i64 %76
  %81 = bitcast float* %80 to <16 x float>*
  store <16 x float> %79, <16 x float>* %81, align 64, !tbaa !547
  %82 = add nsw i64 %41, 96
  %83 = fadd <16 x float> %46, %267
  %84 = fcmp ogt <16 x float> %83, zeroinitializer
  %85 = select <16 x i1> %84, <16 x float> %83, <16 x float> zeroinitializer
  %86 = getelementptr inbounds float, float* %10, i64 %82
  %87 = bitcast float* %86 to <16 x float>*
  store <16 x float> %85, <16 x float>* %87, align 64, !tbaa !547
  %88 = add nsw i64 %41, 112
  %89 = fadd <16 x float> %46, %268
  %90 = fcmp ogt <16 x float> %89, zeroinitializer
  %91 = select <16 x i1> %90, <16 x float> %89, <16 x float> zeroinitializer
  %92 = getelementptr inbounds float, float* %10, i64 %88
  %93 = bitcast float* %92 to <16 x float>*
  store <16 x float> %91, <16 x float>* %93, align 64, !tbaa !547
  %94 = add nsw i64 %41, 128
  %95 = fadd <16 x float> %46, %269
  %96 = fcmp ogt <16 x float> %95, zeroinitializer
  %97 = select <16 x i1> %96, <16 x float> %95, <16 x float> zeroinitializer
  %98 = getelementptr inbounds float, float* %10, i64 %94
  %99 = bitcast float* %98 to <16 x float>*
  store <16 x float> %97, <16 x float>* %99, align 64, !tbaa !547
  %100 = add nsw i64 %41, 144
  %101 = fadd <16 x float> %46, %270
  %102 = fcmp ogt <16 x float> %101, zeroinitializer
  %103 = select <16 x i1> %102, <16 x float> %101, <16 x float> zeroinitializer
  %104 = getelementptr inbounds float, float* %10, i64 %100
  %105 = bitcast float* %104 to <16 x float>*
  store <16 x float> %103, <16 x float>* %105, align 64, !tbaa !547
  %106 = add nsw i64 %41, 160
  %107 = fadd <16 x float> %46, %271
  %108 = fcmp ogt <16 x float> %107, zeroinitializer
  %109 = select <16 x i1> %108, <16 x float> %107, <16 x float> zeroinitializer
  %110 = getelementptr inbounds float, float* %10, i64 %106
  %111 = bitcast float* %110 to <16 x float>*
  store <16 x float> %109, <16 x float>* %111, align 64, !tbaa !547
  %112 = add nsw i64 %41, 176
  %113 = fadd <16 x float> %46, %272
  %114 = fcmp ogt <16 x float> %113, zeroinitializer
  %115 = select <16 x i1> %114, <16 x float> %113, <16 x float> zeroinitializer
  %116 = getelementptr inbounds float, float* %10, i64 %112
  %117 = bitcast float* %116 to <16 x float>*
  store <16 x float> %115, <16 x float>* %117, align 64, !tbaa !547
  %118 = add nsw i64 %41, 192
  %119 = fadd <16 x float> %46, %273
  %120 = fcmp ogt <16 x float> %119, zeroinitializer
  %121 = select <16 x i1> %120, <16 x float> %119, <16 x float> zeroinitializer
  %122 = getelementptr inbounds float, float* %10, i64 %118
  %123 = bitcast float* %122 to <16 x float>*
  store <16 x float> %121, <16 x float>* %123, align 64, !tbaa !547
  %124 = add nsw i64 %41, 208
  %125 = fadd <16 x float> %46, %279
  %126 = fcmp ogt <16 x float> %125, zeroinitializer
  %127 = select <16 x i1> %126, <16 x float> %125, <16 x float> zeroinitializer
  %128 = getelementptr inbounds float, float* %10, i64 %124
  %129 = bitcast float* %128 to <16 x float>*
  store <16 x float> %127, <16 x float>* %129, align 64, !tbaa !547
  %indvars.iv.next82 = add nsw i64 %indvars.iv81, 1
  %130 = icmp slt i64 %indvars.iv.next82, %29
  br i1 %130, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %131 = phi <16 x float> [ %.lcssa3461, %for_body2 ], [ %279, %for_body5 ]
  %132 = phi <16 x float> [ %.lcssa3259, %for_body2 ], [ %273, %for_body5 ]
  %133 = phi <16 x float> [ %.lcssa3057, %for_body2 ], [ %272, %for_body5 ]
  %134 = phi <16 x float> [ %.lcssa2855, %for_body2 ], [ %271, %for_body5 ]
  %135 = phi <16 x float> [ %.lcssa2653, %for_body2 ], [ %270, %for_body5 ]
  %136 = phi <16 x float> [ %.lcssa2451, %for_body2 ], [ %269, %for_body5 ]
  %137 = phi <16 x float> [ %.lcssa2249, %for_body2 ], [ %268, %for_body5 ]
  %138 = phi <16 x float> [ %.lcssa2047, %for_body2 ], [ %267, %for_body5 ]
  %139 = phi <16 x float> [ %.lcssa1845, %for_body2 ], [ %266, %for_body5 ]
  %140 = phi <16 x float> [ %.lcssa1643, %for_body2 ], [ %265, %for_body5 ]
  %141 = phi <16 x float> [ %.lcssa1441, %for_body2 ], [ %264, %for_body5 ]
  %142 = phi <16 x float> [ %.lcssa1239, %for_body2 ], [ %263, %for_body5 ]
  %143 = phi <16 x float> [ %.lcssa1038, %for_body2 ], [ %262, %for_body5 ]
  %144 = phi <16 x float> [ %.lcssa36, %for_body2 ], [ %261, %for_body5 ]
  %145 = add nsw i64 %indvars.iv, %40
  %146 = getelementptr inbounds float, float* %4, i64 %145
  %147 = load float, float* %146, align 4, !tbaa !523
  %148 = insertelement <16 x float> undef, float %147, i32 0
  %149 = shufflevector <16 x float> %148, <16 x float> undef, <16 x i32> zeroinitializer
  %150 = shl nsw i64 %indvars.iv, 4
  %151 = add nsw i64 %39, %150
  %152 = getelementptr inbounds float, float* %7, i64 %151
  %153 = bitcast float* %152 to <16 x float>*
  %154 = load <16 x float>, <16 x float>* %153, align 64, !tbaa !550
  %155 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %149, <16 x float> %154, <16 x float> %144)
  %156 = add nsw i64 %145, 512
  %157 = getelementptr inbounds float, float* %4, i64 %156
  %158 = load float, float* %157, align 4, !tbaa !523
  %159 = insertelement <16 x float> undef, float %158, i32 0
  %160 = shufflevector <16 x float> %159, <16 x float> undef, <16 x i32> zeroinitializer
  %161 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %160, <16 x float> %154, <16 x float> %143)
  %162 = add nsw i64 %145, 1024
  %163 = getelementptr inbounds float, float* %4, i64 %162
  %164 = load float, float* %163, align 4, !tbaa !523
  %165 = insertelement <16 x float> undef, float %164, i32 0
  %166 = shufflevector <16 x float> %165, <16 x float> undef, <16 x i32> zeroinitializer
  %167 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %166, <16 x float> %154, <16 x float> %142)
  %168 = add nsw i64 %145, 1536
  %169 = getelementptr inbounds float, float* %4, i64 %168
  %170 = load float, float* %169, align 4, !tbaa !523
  %171 = insertelement <16 x float> undef, float %170, i32 0
  %172 = shufflevector <16 x float> %171, <16 x float> undef, <16 x i32> zeroinitializer
  %173 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %172, <16 x float> %154, <16 x float> %141)
  %174 = add nsw i64 %145, 2048
  %175 = getelementptr inbounds float, float* %4, i64 %174
  %176 = load float, float* %175, align 4, !tbaa !523
  %177 = insertelement <16 x float> undef, float %176, i32 0
  %178 = shufflevector <16 x float> %177, <16 x float> undef, <16 x i32> zeroinitializer
  %179 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %178, <16 x float> %154, <16 x float> %140)
  %180 = add nsw i64 %145, 2560
  %181 = getelementptr inbounds float, float* %4, i64 %180
  %182 = load float, float* %181, align 4, !tbaa !523
  %183 = insertelement <16 x float> undef, float %182, i32 0
  %184 = shufflevector <16 x float> %183, <16 x float> undef, <16 x i32> zeroinitializer
  %185 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %184, <16 x float> %154, <16 x float> %139)
  %186 = add nsw i64 %145, 3072
  %187 = getelementptr inbounds float, float* %4, i64 %186
  %188 = load float, float* %187, align 4, !tbaa !523
  %189 = insertelement <16 x float> undef, float %188, i32 0
  %190 = shufflevector <16 x float> %189, <16 x float> undef, <16 x i32> zeroinitializer
  %191 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %154, <16 x float> %138)
  %192 = add nsw i64 %145, 3584
  %193 = getelementptr inbounds float, float* %4, i64 %192
  %194 = load float, float* %193, align 4, !tbaa !523
  %195 = insertelement <16 x float> undef, float %194, i32 0
  %196 = shufflevector <16 x float> %195, <16 x float> undef, <16 x i32> zeroinitializer
  %197 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %196, <16 x float> %154, <16 x float> %137)
  %198 = add nsw i64 %145, 4096
  %199 = getelementptr inbounds float, float* %4, i64 %198
  %200 = load float, float* %199, align 4, !tbaa !523
  %201 = insertelement <16 x float> undef, float %200, i32 0
  %202 = shufflevector <16 x float> %201, <16 x float> undef, <16 x i32> zeroinitializer
  %203 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %202, <16 x float> %154, <16 x float> %136)
  %204 = add nsw i64 %145, 4608
  %205 = getelementptr inbounds float, float* %4, i64 %204
  %206 = load float, float* %205, align 4, !tbaa !523
  %207 = insertelement <16 x float> undef, float %206, i32 0
  %208 = shufflevector <16 x float> %207, <16 x float> undef, <16 x i32> zeroinitializer
  %209 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %208, <16 x float> %154, <16 x float> %135)
  %210 = add nsw i64 %145, 5120
  %211 = getelementptr inbounds float, float* %4, i64 %210
  %212 = load float, float* %211, align 4, !tbaa !523
  %213 = insertelement <16 x float> undef, float %212, i32 0
  %214 = shufflevector <16 x float> %213, <16 x float> undef, <16 x i32> zeroinitializer
  %215 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %214, <16 x float> %154, <16 x float> %134)
  %216 = add nsw i64 %145, 5632
  %217 = getelementptr inbounds float, float* %4, i64 %216
  %218 = load float, float* %217, align 4, !tbaa !523
  %219 = insertelement <16 x float> undef, float %218, i32 0
  %220 = shufflevector <16 x float> %219, <16 x float> undef, <16 x i32> zeroinitializer
  %221 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %220, <16 x float> %154, <16 x float> %133)
  %222 = add nsw i64 %145, 6144
  %223 = getelementptr inbounds float, float* %4, i64 %222
  %224 = load float, float* %223, align 4, !tbaa !523
  %225 = insertelement <16 x float> undef, float %224, i32 0
  %226 = shufflevector <16 x float> %225, <16 x float> undef, <16 x i32> zeroinitializer
  %227 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %226, <16 x float> %154, <16 x float> %132)
  %228 = add nsw i64 %145, 6656
  %229 = getelementptr inbounds float, float* %4, i64 %228
  %230 = load float, float* %229, align 4, !tbaa !523
  %231 = insertelement <16 x float> undef, float %230, i32 0
  %232 = shufflevector <16 x float> %231, <16 x float> undef, <16 x i32> zeroinitializer
  %233 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %154, <16 x float> %131)
  %234 = add nsw i64 %151, 8192
  %235 = getelementptr inbounds float, float* %7, i64 %234
  %236 = bitcast float* %235 to <16 x float>*
  %237 = load <16 x float>, <16 x float>* %236, align 64, !tbaa !550
  %238 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %160, <16 x float> %237, <16 x float> %155)
  %239 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %166, <16 x float> %237, <16 x float> %161)
  %240 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %172, <16 x float> %237, <16 x float> %167)
  %241 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %178, <16 x float> %237, <16 x float> %173)
  %242 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %184, <16 x float> %237, <16 x float> %179)
  %243 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %237, <16 x float> %185)
  %244 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %196, <16 x float> %237, <16 x float> %191)
  %245 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %202, <16 x float> %237, <16 x float> %197)
  %246 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %208, <16 x float> %237, <16 x float> %203)
  %247 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %214, <16 x float> %237, <16 x float> %209)
  %248 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %220, <16 x float> %237, <16 x float> %215)
  %249 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %226, <16 x float> %237, <16 x float> %221)
  %250 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %237, <16 x float> %227)
  %251 = add nsw i64 %145, 7168
  %252 = getelementptr inbounds float, float* %4, i64 %251
  %253 = load float, float* %252, align 4, !tbaa !523
  %254 = insertelement <16 x float> undef, float %253, i32 0
  %255 = shufflevector <16 x float> %254, <16 x float> undef, <16 x i32> zeroinitializer
  %256 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %255, <16 x float> %237, <16 x float> %233)
  %257 = add nsw i64 %151, 16384
  %258 = getelementptr inbounds float, float* %7, i64 %257
  %259 = bitcast float* %258 to <16 x float>*
  %260 = load <16 x float>, <16 x float>* %259, align 64, !tbaa !550
  %261 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %166, <16 x float> %260, <16 x float> %238)
  %262 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %172, <16 x float> %260, <16 x float> %239)
  %263 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %178, <16 x float> %260, <16 x float> %240)
  %264 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %184, <16 x float> %260, <16 x float> %241)
  %265 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %260, <16 x float> %242)
  %266 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %196, <16 x float> %260, <16 x float> %243)
  %267 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %202, <16 x float> %260, <16 x float> %244)
  %268 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %208, <16 x float> %260, <16 x float> %245)
  %269 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %214, <16 x float> %260, <16 x float> %246)
  %270 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %220, <16 x float> %260, <16 x float> %247)
  %271 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %226, <16 x float> %260, <16 x float> %248)
  %272 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %260, <16 x float> %249)
  %273 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %255, <16 x float> %260, <16 x float> %250)
  %274 = add nsw i64 %145, 7680
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !523
  %277 = insertelement <16 x float> undef, float %276, i32 0
  %278 = shufflevector <16 x float> %277, <16 x float> undef, <16 x i32> zeroinitializer
  %279 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %278, <16 x float> %260, <16 x float> %256)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next76 = add nuw nsw i64 %indvars.iv75, 1
  %280 = add nuw nsw i32 %35, 1
  %exitcond77 = icmp eq i64 %indvars.iv.next76, 3
  br i1 %exitcond77, label %for_end3, label %for_body2, !prof !20
}

define dllexport i32 @fused_layout_transform_15(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !553 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !555, metadata !DIExpression()), !dbg !558
  call void @llvm.dbg.value(metadata i8* %1, metadata !556, metadata !DIExpression()), !dbg !558
  call void @llvm.dbg.value(metadata i32 %2, metadata !557, metadata !DIExpression()), !dbg !558
  %3 = bitcast i8* %0 to %1**, !dbg !558
  %4 = load %1*, %1** %3, align 8, !dbg !558
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !558
  %6 = bitcast i8* %5 to %1**, !dbg !558
  %7 = load %1*, %1** %6, align 8, !dbg !558
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !558
  %9 = load i8*, i8** %8, align 8, !dbg !558
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !558
  %11 = load i8*, i8** %10, align 8, !dbg !558
  %12 = tail call fastcc i32 @fused_layout_transform_15_compute_(i8* %11, i8* %9), !dbg !558
  ret i32 %12, !dbg !558
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_15_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %2 = alloca %37, align 8
  %3 = getelementptr inbounds %37, %37* %2, i64 0, i32 0
  store i8* %0, i8** %3, align 8
  %4 = getelementptr inbounds %37, %37* %2, i64 0, i32 1
  store i8* %1, i8** %4, align 8
  %5 = bitcast %37* %2 to i8*
  %6 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %7 = call i32 %6(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.33, i8* nonnull %5, i32 0)
  ret i32 %7
}

; Function Attrs: norecurse nounwind
define private i32 @__tvm_parallel_lambda.33(i32, %0* nocapture readonly, i8* nocapture readonly) #1 {
entry:
  %3 = bitcast i8* %2 to float**
  %4 = load float*, float** %3, align 8
  %5 = getelementptr inbounds i8, i8* %2, i64 8
  %6 = bitcast i8* %5 to float**
  %7 = load float*, float** %6, align 8
  %8 = getelementptr inbounds %0, %0* %1, i64 0, i32 1
  %9 = load i32, i32* %8, align 4
  %10 = add nsw i32 %9, 27
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 28
  %15 = select i1 %14, i32 %13, i32 28
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 28
  %18 = select i1 %17, i32 %16, i32 28
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
  %24 = mul nsw i64 %indvars.iv10, 14336
  %25 = trunc i64 %indvars.iv10 to i32
  %26 = mul i32 %25, 896
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv7 = phi i64 [ 0, %for_body ], [ %indvars.iv.next8, %for_end6 ]
  %27 = shl i64 %indvars.iv7, 9
  %28 = add nsw i64 %27, %24
  %indvars.iv7.tr = trunc i64 %indvars.iv7 to i32
  %29 = shl i32 %indvars.iv7.tr, 5
  %30 = add i32 %29, %26
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next11 = add nsw i64 %indvars.iv10, 1
  %31 = icmp slt i64 %indvars.iv.next11, %23
  br i1 %31, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %32 = add nsw i64 %28, %indvars.iv
  %33 = trunc i64 %indvars.iv to i32
  %34 = and i32 %33, 31
  %35 = lshr i32 %33, 5
  %36 = mul nsw i32 %35, 25088
  %37 = add i32 %30, %36
  %38 = or i32 %37, %34
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, float* %7, i64 %39
  %41 = bitcast float* %40 to i32*
  %42 = load i32, i32* %41, align 4, !tbaa !559
  %43 = getelementptr inbounds float, float* %4, i64 %32
  %44 = bitcast float* %43 to i32*
  store i32 %42, i32* %44, align 4, !tbaa !562
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !20

for_end6:                                         ; preds = %for_body5
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 28
  br i1 %exitcond9, label %for_end3, label %for_body2, !prof !20
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i32, i1) #5

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #5

attributes #0 = { noinline }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { nounwind }
attributes #4 = { noinline norecurse nounwind }
attributes #5 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_8", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!20 = !{!"branch_weights", i32 1, i32 1048576}
!21 = !{!22, !22, i64 0}
!22 = !{!"float32", !23, i64 0}
!23 = !{!"0x5603afa2cab0", !18, i64 0}
!24 = !{!25, !25, i64 0}
!25 = !{!"float32", !26, i64 0}
!26 = !{!"0x5603afa38d90", !18, i64 0}
!27 = !{!28, !28, i64 0}
!28 = !{!"float32", !29, i64 0}
!29 = !{!"0x5603afa2c4d0", !18, i64 0}
!30 = !{!31, !31, i64 0}
!31 = !{!"0x5603afa2f240.w32.b0", !32, i64 0}
!32 = !{!"0x5603afa2f240.w64.b0", !33, i64 0}
!33 = !{!"0x5603afa2f240.w128.b0", !34, i64 0}
!34 = !{!"0x5603afa2f240.w256.b0", !35, i64 0}
!35 = !{!"0x5603afa2f240.w512.b0", !36, i64 0}
!36 = !{!"0x5603afa2f240.w1024.b0", !37, i64 0}
!37 = !{!"float32", !38, i64 0}
!38 = !{!"0x5603afa2f240", !18, i64 0}
!39 = !{!40, !40, i64 0}
!40 = !{!"float32", !41, i64 0}
!41 = !{!"0x5603afa2b500", !18, i64 0}
!42 = !{!43, !43, i64 0}
!43 = !{!"float32", !44, i64 0}
!44 = !{!"0x5603afa2af50", !18, i64 0}
!45 = !{!46, !46, i64 0}
!46 = !{!"float32", !47, i64 0}
!47 = !{!"0x5603afa2b890", !18, i64 0}
!48 = distinct !DISubprogram(name: "fused_layout_transform_19", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !49)
!49 = !{!50, !51, !52}
!50 = !DILocalVariable(name: "arg1", arg: 1, scope: !48, file: !1, type: !9)
!51 = !DILocalVariable(name: "arg2", arg: 2, scope: !48, file: !1, type: !9)
!52 = !DILocalVariable(name: "arg3", arg: 3, scope: !48, file: !1, type: !8)
!53 = !DILocation(line: 0, scope: !48)
!54 = !{!55, !55, i64 0}
!55 = !{!"float32", !56, i64 0}
!56 = !{!"0x5603afa23e90", !18, i64 0}
!57 = !{!58, !58, i64 0}
!58 = !{!"float32", !59, i64 0}
!59 = !{!"0x5603afa24100", !18, i64 0}
!60 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_7", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !61)
!61 = !{!62, !63, !64}
!62 = !DILocalVariable(name: "arg1", arg: 1, scope: !60, file: !1, type: !9)
!63 = !DILocalVariable(name: "arg2", arg: 2, scope: !60, file: !1, type: !9)
!64 = !DILocalVariable(name: "arg3", arg: 3, scope: !60, file: !1, type: !8)
!65 = !DILocation(line: 0, scope: !60)
!66 = !{!67, !67, i64 0}
!67 = !{!"float32", !68, i64 0}
!68 = !{!"0x5603af9fc7a0", !18, i64 0}
!69 = !{!70, !70, i64 0}
!70 = !{!"float32", !71, i64 0}
!71 = !{!"0x5603afa09290", !18, i64 0}
!72 = !{!73, !73, i64 0}
!73 = !{!"float32", !74, i64 0}
!74 = !{!"0x5603af9fabc0", !18, i64 0}
!75 = !{!76, !76, i64 0}
!76 = !{!"0x5603afa01830.w32.b0", !77, i64 0}
!77 = !{!"0x5603afa01830.w64.b0", !78, i64 0}
!78 = !{!"0x5603afa01830.w128.b0", !79, i64 0}
!79 = !{!"0x5603afa01830.w256.b0", !80, i64 0}
!80 = !{!"0x5603afa01830.w512.b0", !81, i64 0}
!81 = !{!"0x5603afa01830.w1024.b0", !82, i64 0}
!82 = !{!"float32", !83, i64 0}
!83 = !{!"0x5603afa01830", !18, i64 0}
!84 = !{!85, !85, i64 0}
!85 = !{!"float32", !86, i64 0}
!86 = !{!"0x5603af9fb1f0", !18, i64 0}
!87 = !{!88, !88, i64 0}
!88 = !{!"float32", !89, i64 0}
!89 = !{!"0x5603af9fc250", !18, i64 0}
!90 = !{!91, !91, i64 0}
!91 = !{!"float32", !92, i64 0}
!92 = !{!"0x5603af9fb580", !18, i64 0}
!93 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_5", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !94)
!94 = !{!95, !96, !97}
!95 = !DILocalVariable(name: "arg1", arg: 1, scope: !93, file: !1, type: !9)
!96 = !DILocalVariable(name: "arg2", arg: 2, scope: !93, file: !1, type: !9)
!97 = !DILocalVariable(name: "arg3", arg: 3, scope: !93, file: !1, type: !8)
!98 = !DILocation(line: 0, scope: !93)
!99 = !{!100, !100, i64 0}
!100 = !{!"float32", !101, i64 0}
!101 = !{!"0x5603af9b2e50", !18, i64 0}
!102 = !{!103, !103, i64 0}
!103 = !{!"float32", !104, i64 0}
!104 = !{!"0x5603af9bf8c0", !18, i64 0}
!105 = !{!106, !106, i64 0}
!106 = !{!"float32", !107, i64 0}
!107 = !{!"0x5603af9b2900", !18, i64 0}
!108 = !{!109, !109, i64 0}
!109 = !{!"0x5603af9b7e60.w64.b0", !110, i64 0}
!110 = !{!"0x5603af9b7e60.w128.b0", !111, i64 0}
!111 = !{!"0x5603af9b7e60.w256.b0", !112, i64 0}
!112 = !{!"0x5603af9b7e60.w512.b0", !113, i64 0}
!113 = !{!"0x5603af9b7e60.w1024.b0", !114, i64 0}
!114 = !{!"float32", !115, i64 0}
!115 = !{!"0x5603af9b7e60", !18, i64 0}
!116 = !{!117, !117, i64 0}
!117 = !{!"float32", !118, i64 0}
!118 = !{!"0x5603af9b18a0", !18, i64 0}
!119 = !{!120, !120, i64 0}
!120 = !{!"float32", !121, i64 0}
!121 = !{!"0x5603af9b1270", !18, i64 0}
!122 = !{!123, !123, i64 0}
!123 = !{!"float32", !124, i64 0}
!124 = !{!"0x5603af9b1c30", !18, i64 0}
!125 = distinct !DISubprogram(name: "fused_nn_max_pool2d_3", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !126)
!126 = !{!127, !128, !129}
!127 = !DILocalVariable(name: "arg1", arg: 1, scope: !125, file: !1, type: !9)
!128 = !DILocalVariable(name: "arg2", arg: 2, scope: !125, file: !1, type: !9)
!129 = !DILocalVariable(name: "arg3", arg: 3, scope: !125, file: !1, type: !8)
!130 = !DILocation(line: 0, scope: !125)
!131 = !{!132, !132, i64 0}
!132 = !{!"float32", !133, i64 0}
!133 = !{!"0x5603af9a6bb0", !18, i64 0}
!134 = !{!135, !135, i64 0}
!135 = !{!"float32", !136, i64 0}
!136 = !{!"0x5603af9a7150", !18, i64 0}
!137 = distinct !DISubprogram(name: "fused_layout_transform_18", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !138)
!138 = !{!139, !140, !141}
!139 = !DILocalVariable(name: "arg1", arg: 1, scope: !137, file: !1, type: !9)
!140 = !DILocalVariable(name: "arg2", arg: 2, scope: !137, file: !1, type: !9)
!141 = !DILocalVariable(name: "arg3", arg: 3, scope: !137, file: !1, type: !8)
!142 = !DILocation(line: 0, scope: !137)
!143 = !{!144, !144, i64 0}
!144 = !{!"float32", !145, i64 0}
!145 = !{!"0x5603af9a04d0", !18, i64 0}
!146 = !{!147, !147, i64 0}
!147 = !{!"float32", !148, i64 0}
!148 = !{!"0x5603af9a0740", !18, i64 0}
!149 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_4", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !150)
!150 = !{!151, !152, !153}
!151 = !DILocalVariable(name: "arg1", arg: 1, scope: !149, file: !1, type: !9)
!152 = !DILocalVariable(name: "arg2", arg: 2, scope: !149, file: !1, type: !9)
!153 = !DILocalVariable(name: "arg3", arg: 3, scope: !149, file: !1, type: !8)
!154 = !DILocation(line: 0, scope: !149)
!155 = !{!156, !156, i64 0}
!156 = !{!"float32", !157, i64 0}
!157 = !{!"0x5603af9793e0", !18, i64 0}
!158 = !{!159, !159, i64 0}
!159 = !{!"float32", !160, i64 0}
!160 = !{!"0x5603af985ad0", !18, i64 0}
!161 = !{!162, !162, i64 0}
!162 = !{!"float32", !163, i64 0}
!163 = !{!"0x5603af978e00", !18, i64 0}
!164 = !{!165, !165, i64 0}
!165 = !{!"0x5603afecf230.w32.b0", !166, i64 0}
!166 = !{!"0x5603afecf230.w64.b0", !167, i64 0}
!167 = !{!"0x5603afecf230.w128.b0", !168, i64 0}
!168 = !{!"0x5603afecf230.w256.b0", !169, i64 0}
!169 = !{!"0x5603afecf230.w512.b0", !170, i64 0}
!170 = !{!"0x5603afecf230.w1024.b0", !171, i64 0}
!171 = !{!"float32", !172, i64 0}
!172 = !{!"0x5603afecf230", !18, i64 0}
!173 = !{!174, !174, i64 0}
!174 = !{!"float32", !175, i64 0}
!175 = !{!"0x5603af977e30", !18, i64 0}
!176 = !{!177, !177, i64 0}
!177 = !{!"float32", !178, i64 0}
!178 = !{!"0x5603af977840", !18, i64 0}
!179 = !{!180, !180, i64 0}
!180 = !{!"float32", !181, i64 0}
!181 = !{!"0x5603af9781c0", !18, i64 0}
!182 = distinct !DISubprogram(name: "fused_layout_transform_17", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !183)
!183 = !{!184, !185, !186}
!184 = !DILocalVariable(name: "arg1", arg: 1, scope: !182, file: !1, type: !9)
!185 = !DILocalVariable(name: "arg2", arg: 2, scope: !182, file: !1, type: !9)
!186 = !DILocalVariable(name: "arg3", arg: 3, scope: !182, file: !1, type: !8)
!187 = !DILocation(line: 0, scope: !182)
!188 = !{!189, !189, i64 0}
!189 = !{!"float32", !190, i64 0}
!190 = !{!"0x5603af970700", !18, i64 0}
!191 = !{!192, !192, i64 0}
!192 = !{!"float32", !193, i64 0}
!193 = !{!"0x5603af970970", !18, i64 0}
!194 = distinct !DISubprogram(name: "fused_nn_dense_add_nn_relu_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !195)
!195 = !{!196, !197, !198}
!196 = !DILocalVariable(name: "arg1", arg: 1, scope: !194, file: !1, type: !9)
!197 = !DILocalVariable(name: "arg2", arg: 2, scope: !194, file: !1, type: !9)
!198 = !DILocalVariable(name: "arg3", arg: 3, scope: !194, file: !1, type: !8)
!199 = !DILocation(line: 0, scope: !194)
!200 = !{!201, !201, i64 0}
!201 = !{!"float32", !202, i64 0}
!202 = !{!"0x5603afe07330", !18, i64 0}
!203 = !{!204, !204, i64 0}
!204 = !{!"float32", !205, i64 0}
!205 = !{!"0x5603afe979f0", !18, i64 0}
!206 = !{!207, !207, i64 0}
!207 = !{!"float32", !208, i64 0}
!208 = !{!"0x5603acf5b130", !18, i64 0}
!209 = distinct !{!209, !210}
!210 = !{!"llvm.loop.isvectorized", i32 1}
!211 = !{!212, !212, i64 0}
!212 = !{!"float32", !213, i64 0}
!213 = !{!"0x5603afe07290", !18, i64 0}
!214 = !{!215, !215, i64 0}
!215 = !{!"float32", !216, i64 0}
!216 = !{!"0x5603afe072e0", !18, i64 0}
!217 = distinct !DISubprogram(name: "fused_nn_max_pool2d_4", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !218)
!218 = !{!219, !220, !221}
!219 = !DILocalVariable(name: "arg1", arg: 1, scope: !217, file: !1, type: !9)
!220 = !DILocalVariable(name: "arg2", arg: 2, scope: !217, file: !1, type: !9)
!221 = !DILocalVariable(name: "arg3", arg: 3, scope: !217, file: !1, type: !8)
!222 = !DILocation(line: 0, scope: !217)
!223 = !{!224, !224, i64 0}
!224 = !{!"float32", !225, i64 0}
!225 = !{!"0x5603af9f0ec0", !18, i64 0}
!226 = !{!227, !227, i64 0}
!227 = !{!"float32", !228, i64 0}
!228 = !{!"0x5603af9f17c0", !18, i64 0}
!229 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_6", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !230)
!230 = !{!231, !232, !233}
!231 = !DILocalVariable(name: "arg1", arg: 1, scope: !229, file: !1, type: !9)
!232 = !DILocalVariable(name: "arg2", arg: 2, scope: !229, file: !1, type: !9)
!233 = !DILocalVariable(name: "arg3", arg: 3, scope: !229, file: !1, type: !8)
!234 = !DILocation(line: 0, scope: !229)
!235 = !{!236, !236, i64 0}
!236 = !{!"float32", !237, i64 0}
!237 = !{!"0x5603af9d3180", !18, i64 0}
!238 = !{!239, !239, i64 0}
!239 = !{!"float32", !240, i64 0}
!240 = !{!"0x5603af9df690", !18, i64 0}
!241 = !{!242, !242, i64 0}
!242 = !{!"float32", !243, i64 0}
!243 = !{!"0x5603af9d2e20", !18, i64 0}
!244 = !{!245, !245, i64 0}
!245 = !{!"0x5603af9d7ce0.w64.b0", !246, i64 0}
!246 = !{!"0x5603af9d7ce0.w128.b0", !247, i64 0}
!247 = !{!"0x5603af9d7ce0.w256.b0", !248, i64 0}
!248 = !{!"0x5603af9d7ce0.w512.b0", !249, i64 0}
!249 = !{!"0x5603af9d7ce0.w1024.b0", !250, i64 0}
!250 = !{!"float32", !251, i64 0}
!251 = !{!"0x5603af9d7ce0", !18, i64 0}
!252 = !{!253, !253, i64 0}
!253 = !{!"float32", !254, i64 0}
!254 = !{!"0x5603af9d2150", !18, i64 0}
!255 = !{!256, !256, i64 0}
!256 = !{!"float32", !257, i64 0}
!257 = !{!"0x5603af9d1ed0", !18, i64 0}
!258 = !{!259, !259, i64 0}
!259 = !{!"float32", !260, i64 0}
!260 = !{!"0x5603af9d2370", !18, i64 0}
!261 = distinct !DISubprogram(name: "fused_nn_max_pool2d_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !262)
!262 = !{!263, !264, !265}
!263 = !DILocalVariable(name: "arg1", arg: 1, scope: !261, file: !1, type: !9)
!264 = !DILocalVariable(name: "arg2", arg: 2, scope: !261, file: !1, type: !9)
!265 = !DILocalVariable(name: "arg3", arg: 3, scope: !261, file: !1, type: !8)
!266 = !DILocation(line: 0, scope: !261)
!267 = !{!268, !268, i64 0}
!268 = !{!"float32", !269, i64 0}
!269 = !{!"0x5603afe306a0", !18, i64 0}
!270 = !{!271, !271, i64 0}
!271 = !{!"float32", !272, i64 0}
!272 = !{!"0x5603afe30c40", !18, i64 0}
!273 = distinct !DISubprogram(name: "fused_layout_transform_16", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !274)
!274 = !{!275, !276, !277}
!275 = !DILocalVariable(name: "arg1", arg: 1, scope: !273, file: !1, type: !9)
!276 = !DILocalVariable(name: "arg2", arg: 2, scope: !273, file: !1, type: !9)
!277 = !DILocalVariable(name: "arg3", arg: 3, scope: !273, file: !1, type: !8)
!278 = !DILocation(line: 0, scope: !273)
!279 = !{!280, !280, i64 0}
!280 = !{!"float32", !281, i64 0}
!281 = !{!"0x5603af9389e0", !18, i64 0}
!282 = !{!283, !283, i64 0}
!283 = !{!"float32", !284, i64 0}
!284 = !{!"0x5603af938c50", !18, i64 0}
!285 = distinct !DISubprogram(name: "fused_layout_transform_14", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !286)
!286 = !{!287, !288, !289}
!287 = !DILocalVariable(name: "arg1", arg: 1, scope: !285, file: !1, type: !9)
!288 = !DILocalVariable(name: "arg2", arg: 2, scope: !285, file: !1, type: !9)
!289 = !DILocalVariable(name: "arg3", arg: 3, scope: !285, file: !1, type: !8)
!290 = !DILocation(line: 0, scope: !285)
!291 = !{!292, !292, i64 0}
!292 = !{!"float32", !293, i64 0}
!293 = !{!"0x5603afea1ea0", !18, i64 0}
!294 = !{!295, !295, i64 0}
!295 = !{!"float32", !296, i64 0}
!296 = !{!"0x5603afea2110", !18, i64 0}
!297 = distinct !DISubprogram(name: "fused_layout_transform_20", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !298)
!298 = !{!299, !300, !301}
!299 = !DILocalVariable(name: "arg1", arg: 1, scope: !297, file: !1, type: !9)
!300 = !DILocalVariable(name: "arg2", arg: 2, scope: !297, file: !1, type: !9)
!301 = !DILocalVariable(name: "arg3", arg: 3, scope: !297, file: !1, type: !8)
!302 = !DILocation(line: 0, scope: !297)
!303 = !{!304, !304, i64 0}
!304 = !{!"float32", !305, i64 0}
!305 = !{!"0x5603afa48990", !18, i64 0}
!306 = !{!307, !307, i64 0}
!307 = !{!"float32", !308, i64 0}
!308 = !{!"0x5603afa489e0", !18, i64 0}
!309 = distinct !{!309, !210}
!310 = distinct !{!310, !210}
!311 = distinct !DISubprogram(name: "fused_nn_dense_add", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !312)
!312 = !{!313, !314, !315}
!313 = !DILocalVariable(name: "arg1", arg: 1, scope: !311, file: !1, type: !9)
!314 = !DILocalVariable(name: "arg2", arg: 2, scope: !311, file: !1, type: !9)
!315 = !DILocalVariable(name: "arg3", arg: 3, scope: !311, file: !1, type: !8)
!316 = !DILocation(line: 0, scope: !311)
!317 = !{!318, !318, i64 0}
!318 = !{!"float32", !319, i64 0}
!319 = !{!"0x5603aad24d50", !18, i64 0}
!320 = !{!321, !321, i64 0}
!321 = !{!"float32", !322, i64 0}
!322 = !{!"0x5603afe5ac40", !18, i64 0}
!323 = !{!324, !324, i64 0}
!324 = !{!"float32", !325, i64 0}
!325 = !{!"0x5603ab9f39c0", !18, i64 0}
!326 = distinct !{!326, !210}
!327 = !{!328, !328, i64 0}
!328 = !{!"float32", !329, i64 0}
!329 = !{!"0x5603ad6ed8f0", !18, i64 0}
!330 = !{!331, !331, i64 0}
!331 = !{!"float32", !332, i64 0}
!332 = !{!"0x5603acf72ad0", !18, i64 0}
!333 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_3", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !334)
!334 = !{!335, !336, !337}
!335 = !DILocalVariable(name: "arg1", arg: 1, scope: !333, file: !1, type: !9)
!336 = !DILocalVariable(name: "arg2", arg: 2, scope: !333, file: !1, type: !9)
!337 = !DILocalVariable(name: "arg3", arg: 3, scope: !333, file: !1, type: !8)
!338 = !DILocation(line: 0, scope: !333)
!339 = !{!340, !340, i64 0}
!340 = !{!"float32", !341, i64 0}
!341 = !{!"0x5603af94b830", !18, i64 0}
!342 = !{!343, !343, i64 0}
!343 = !{!"float32", !344, i64 0}
!344 = !{!"0x5603af958040", !18, i64 0}
!345 = distinct !{!345, !210}
!346 = !{!347, !347, i64 0}
!347 = !{!"float32", !348, i64 0}
!348 = !{!"0x5603af94b2e0", !18, i64 0}
!349 = !{!350, !350, i64 0}
!350 = !{!"float32", !351, i64 0}
!351 = !{!"0x5603af949c50", !18, i64 0}
!352 = !{!353, !353, i64 0}
!353 = !{!"float32", !354, i64 0}
!354 = !{!"0x5603af94a280", !18, i64 0}
!355 = !{!356, !356, i64 0}
!356 = !{!"float32", !357, i64 0}
!357 = !{!"0x5603af94a610", !18, i64 0}
!358 = !{!359, !359, i64 0}
!359 = !{!"0x5603af950640.w32.b0", !360, i64 0}
!360 = !{!"0x5603af950640.w64.b0", !361, i64 0}
!361 = !{!"0x5603af950640.w128.b0", !362, i64 0}
!362 = !{!"0x5603af950640.w256.b0", !363, i64 0}
!363 = !{!"0x5603af950640.w512.b0", !364, i64 0}
!364 = !{!"0x5603af950640.w1024.b0", !365, i64 0}
!365 = !{!"float32", !366, i64 0}
!366 = !{!"0x5603af950640", !18, i64 0}
!367 = distinct !DISubprogram(name: "fused_nn_max_pool2d", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !368)
!368 = !{!369, !370, !371}
!369 = !DILocalVariable(name: "arg1", arg: 1, scope: !367, file: !1, type: !9)
!370 = !DILocalVariable(name: "arg2", arg: 2, scope: !367, file: !1, type: !9)
!371 = !DILocalVariable(name: "arg3", arg: 3, scope: !367, file: !1, type: !8)
!372 = !DILocation(line: 0, scope: !367)
!373 = !{!374, !374, i64 0}
!374 = !{!"float32", !375, i64 0}
!375 = !{!"0x5603af6a2440", !18, i64 0}
!376 = !{!377, !377, i64 0}
!377 = !{!"float32", !378, i64 0}
!378 = !{!"0x5603afeb89c0", !18, i64 0}
!379 = distinct !DISubprogram(name: "fused_nn_dense_add_nn_relu", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !380)
!380 = !{!381, !382, !383}
!381 = !DILocalVariable(name: "arg1", arg: 1, scope: !379, file: !1, type: !9)
!382 = !DILocalVariable(name: "arg2", arg: 2, scope: !379, file: !1, type: !9)
!383 = !DILocalVariable(name: "arg3", arg: 3, scope: !379, file: !1, type: !8)
!384 = !DILocation(line: 0, scope: !379)
!385 = !{!386, !386, i64 0}
!386 = !{!"float32", !387, i64 0}
!387 = !{!"0x5603afebfa30", !18, i64 0}
!388 = !{!389, !389, i64 0}
!389 = !{!"float32", !390, i64 0}
!390 = !{!"0x5603aab528e0", !18, i64 0}
!391 = !{!392, !392, i64 0}
!392 = !{!"float32", !393, i64 0}
!393 = !{!"0x5603afebfa80", !18, i64 0}
!394 = distinct !{!394, !210}
!395 = !{!396, !396, i64 0}
!396 = !{!"float32", !397, i64 0}
!397 = !{!"0x5603ad02cf90", !18, i64 0}
!398 = !{!399, !399, i64 0}
!399 = !{!"float32", !400, i64 0}
!400 = !{!"0x5603afebf9e0", !18, i64 0}
!401 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !402)
!402 = !{!403, !404, !405}
!403 = !DILocalVariable(name: "arg1", arg: 1, scope: !401, file: !1, type: !9)
!404 = !DILocalVariable(name: "arg2", arg: 2, scope: !401, file: !1, type: !9)
!405 = !DILocalVariable(name: "arg3", arg: 3, scope: !401, file: !1, type: !8)
!406 = !DILocation(line: 0, scope: !401)
!407 = !{!408, !408, i64 0}
!408 = !{!"float32", !409, i64 0}
!409 = !{!"0x5603afeb3880", !18, i64 0}
!410 = !{!411, !411, i64 0}
!411 = !{!"float32", !412, i64 0}
!412 = !{!"0x5603afdfd9e0", !18, i64 0}
!413 = distinct !{!413, !210}
!414 = distinct !{!414, !210}
!415 = distinct !{!415, !210}
!416 = !{!417, !417, i64 0}
!417 = !{!"0x5603ad2e18a0.w16.b0", !418, i64 0}
!418 = !{!"0x5603ad2e18a0.w32.b0", !419, i64 0}
!419 = !{!"0x5603ad2e18a0.w64.b0", !420, i64 0}
!420 = !{!"0x5603ad2e18a0.w128.b0", !421, i64 0}
!421 = !{!"0x5603ad2e18a0.w256.b0", !422, i64 0}
!422 = !{!"0x5603ad2e18a0.w512.b0", !423, i64 0}
!423 = !{!"0x5603ad2e18a0.w1024.b0", !424, i64 0}
!424 = !{!"float32", !425, i64 0}
!425 = !{!"0x5603ad2e18a0", !18, i64 0}
!426 = !{!427, !427, i64 0}
!427 = !{!"float32", !428, i64 0}
!428 = !{!"0x5603acf891a0", !18, i64 0}
!429 = !{!430, !430, i64 0}
!430 = !{!"float32", !431, i64 0}
!431 = !{!"0x5603ab801720", !18, i64 0}
!432 = !{!433, !433, i64 0}
!433 = !{!"float32", !434, i64 0}
!434 = !{!"0x5603acf8a810", !18, i64 0}
!435 = !{!436, !436, i64 0}
!436 = !{!"float32", !437, i64 0}
!437 = !{!"0x5603acf8aba0", !18, i64 0}
!438 = distinct !DISubprogram(name: "fused_layout_transform_nn_batch_flatten_nn_batch_flatten_multiply", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !439)
!439 = !{!440, !441, !442}
!440 = !DILocalVariable(name: "arg1", arg: 1, scope: !438, file: !1, type: !9)
!441 = !DILocalVariable(name: "arg2", arg: 2, scope: !438, file: !1, type: !9)
!442 = !DILocalVariable(name: "arg3", arg: 3, scope: !438, file: !1, type: !8)
!443 = !DILocation(line: 0, scope: !438)
!444 = !{!445, !445, i64 0}
!445 = !{!"float32", !446, i64 0}
!446 = !{!"0x5603afe00640", !18, i64 0}
!447 = !{!448, !448, i64 0}
!448 = !{!"float32", !449, i64 0}
!449 = !{!"0x5603afe007d0", !18, i64 0}
!450 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !451)
!451 = !{!452, !453, !454}
!452 = !DILocalVariable(name: "arg1", arg: 1, scope: !450, file: !1, type: !9)
!453 = !DILocalVariable(name: "arg2", arg: 2, scope: !450, file: !1, type: !9)
!454 = !DILocalVariable(name: "arg3", arg: 3, scope: !450, file: !1, type: !8)
!455 = !DILocation(line: 0, scope: !450)
!456 = !{!457, !457, i64 0}
!457 = !{!"float32", !458, i64 0}
!458 = !{!"0x5603aff36f20", !18, i64 0}
!459 = !{!460, !460, i64 0}
!460 = !{!"float32", !461, i64 0}
!461 = !{!"0x5603af922a70", !18, i64 0}
!462 = distinct !{!462, !210}
!463 = !{!464, !464, i64 0}
!464 = !{!"float32", !465, i64 0}
!465 = !{!"0x5603aff37ef0", !18, i64 0}
!466 = !{!467, !467, i64 0}
!467 = !{!"float32", !468, i64 0}
!468 = !{!"0x5603aff384d0", !18, i64 0}
!469 = !{!470, !470, i64 0}
!470 = !{!"float32", !471, i64 0}
!471 = !{!"0x5603aff372b0", !18, i64 0}
!472 = !{!473, !473, i64 0}
!473 = !{!"float32", !474, i64 0}
!474 = !{!"0x5603aff36970", !18, i64 0}
!475 = !{!476, !476, i64 0}
!476 = !{!"0x5603aff3d250.w32.b0", !477, i64 0}
!477 = !{!"0x5603aff3d250.w64.b0", !478, i64 0}
!478 = !{!"0x5603aff3d250.w128.b0", !479, i64 0}
!479 = !{!"0x5603aff3d250.w256.b0", !480, i64 0}
!480 = !{!"0x5603aff3d250.w512.b0", !481, i64 0}
!481 = !{!"0x5603aff3d250.w1024.b0", !482, i64 0}
!482 = !{!"float32", !483, i64 0}
!483 = !{!"0x5603aff3d250", !18, i64 0}
!484 = distinct !DISubprogram(name: "fused_nn_max_pool2d_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !485)
!485 = !{!486, !487, !488}
!486 = !DILocalVariable(name: "arg1", arg: 1, scope: !484, file: !1, type: !9)
!487 = !DILocalVariable(name: "arg2", arg: 2, scope: !484, file: !1, type: !9)
!488 = !DILocalVariable(name: "arg3", arg: 3, scope: !484, file: !1, type: !8)
!489 = !DILocation(line: 0, scope: !484)
!490 = !{!491, !491, i64 0}
!491 = !{!"float32", !492, i64 0}
!492 = !{!"0x5603af93f7d0", !18, i64 0}
!493 = !{!494, !494, i64 0}
!494 = !{!"float32", !495, i64 0}
!495 = !{!"0x5603af93fd70", !18, i64 0}
!496 = distinct !DISubprogram(name: "fused_nn_batch_flatten_nn_batch_flatten_multiply", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !497)
!497 = !{!498, !499, !500}
!498 = !DILocalVariable(name: "arg1", arg: 1, scope: !496, file: !1, type: !9)
!499 = !DILocalVariable(name: "arg2", arg: 2, scope: !496, file: !1, type: !9)
!500 = !DILocalVariable(name: "arg3", arg: 3, scope: !496, file: !1, type: !8)
!501 = !DILocation(line: 0, scope: !496)
!502 = distinct !DISubprogram(name: "fused_layout_transform_13", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !503)
!503 = !{!504, !505, !506}
!504 = !DILocalVariable(name: "arg1", arg: 1, scope: !502, file: !1, type: !9)
!505 = !DILocalVariable(name: "arg2", arg: 2, scope: !502, file: !1, type: !9)
!506 = !DILocalVariable(name: "arg3", arg: 3, scope: !502, file: !1, type: !8)
!507 = !DILocation(line: 0, scope: !502)
!508 = !{!509, !509, i64 0}
!509 = !{!"float32", !510, i64 0}
!510 = !{!"0x5603afe4e580", !18, i64 0}
!511 = !{!512, !512, i64 0}
!512 = !{!"float32", !513, i64 0}
!513 = !{!"0x5603afe4e7f0", !18, i64 0}
!514 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc_add_nn_relu", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !515)
!515 = !{!516, !517, !518}
!516 = !DILocalVariable(name: "arg1", arg: 1, scope: !514, file: !1, type: !9)
!517 = !DILocalVariable(name: "arg2", arg: 2, scope: !514, file: !1, type: !9)
!518 = !DILocalVariable(name: "arg3", arg: 3, scope: !514, file: !1, type: !8)
!519 = !DILocation(line: 0, scope: !514)
!520 = !{!521, !521, i64 0}
!521 = !{!"float32", !522, i64 0}
!522 = !{!"0x5603afece450", !18, i64 0}
!523 = !{!524, !524, i64 0}
!524 = !{!"float32", !525, i64 0}
!525 = !{!"0x5603acf55870", !18, i64 0}
!526 = distinct !{!526, !210}
!527 = distinct !{!527, !210}
!528 = distinct !{!528, !210}
!529 = distinct !{!529, !210}
!530 = distinct !{!530, !210}
!531 = distinct !{!531, !210}
!532 = distinct !{!532, !210}
!533 = distinct !{!533, !210}
!534 = distinct !{!534, !210}
!535 = distinct !{!535, !210}
!536 = distinct !{!536, !210}
!537 = distinct !{!537, !210}
!538 = distinct !{!538, !210}
!539 = distinct !{!539, !210}
!540 = distinct !{!540, !210}
!541 = distinct !{!541, !210}
!542 = distinct !{!542, !210}
!543 = distinct !{!543, !210}
!544 = !{!545, !545, i64 0}
!545 = !{!"float32", !546, i64 0}
!546 = !{!"0x5603afecee10", !18, i64 0}
!547 = !{!548, !548, i64 0}
!548 = !{!"float32", !549, i64 0}
!549 = !{!"0x5603afe84a40", !18, i64 0}
!550 = !{!551, !551, i64 0}
!551 = !{!"float32", !552, i64 0}
!552 = !{!"0x5603afe849b0", !18, i64 0}
!553 = distinct !DISubprogram(name: "fused_layout_transform_15", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !554)
!554 = !{!555, !556, !557}
!555 = !DILocalVariable(name: "arg1", arg: 1, scope: !553, file: !1, type: !9)
!556 = !DILocalVariable(name: "arg2", arg: 2, scope: !553, file: !1, type: !9)
!557 = !DILocalVariable(name: "arg3", arg: 3, scope: !553, file: !1, type: !8)
!558 = !DILocation(line: 0, scope: !553)
!559 = !{!560, !560, i64 0}
!560 = !{!"float32", !561, i64 0}
!561 = !{!"0x5603afea6ab0", !18, i64 0}
!562 = !{!563, !563, i64 0}
!563 = !{!"float32", !564, i64 0}
!564 = !{!"0x5603afea6d20", !18, i64 0}
