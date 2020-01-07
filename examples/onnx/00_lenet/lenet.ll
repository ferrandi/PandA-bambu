; ModuleID = 'fused_reshape_3'
source_filename = "fused_reshape_3"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [16 x i8] c"fused_reshape_3\00", align 1

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_reshape_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_reshape_3_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_reshape_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 40, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_nn_relu_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %4 = getelementptr inbounds float, float* %2, i64 %index
  %5 = bitcast float* %4 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !5
  %6 = getelementptr inbounds float, float* %4, i64 4
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !5
  %8 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %9 = fcmp ogt <4 x float> %wide.load2, zeroinitializer
  %10 = select <4 x i1> %8, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = select <4 x i1> %9, <4 x float> %wide.load2, <4 x float> zeroinitializer
  %12 = getelementptr inbounds float, float* %3, i64 %index
  %13 = bitcast float* %12 to <4 x float>*
  store <4 x float> %10, <4 x float>* %13, align 4, !tbaa !9
  %14 = getelementptr inbounds float, float* %12, i64 4
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %11, <4 x float>* %15, align 4, !tbaa !9
  %index.next = add i64 %index, 8
  %16 = icmp eq i64 %index.next, 1024
  br i1 %16, label %for_end, label %vector.body, !llvm.loop !12

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_add_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_add_1_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_add_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %6 = getelementptr inbounds float, float* %3, i64 %index
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !14
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !14
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !17
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !17
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !20
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !20
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 1024
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !23

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_nn_conv2d_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = alloca [2048 x <5 x float>], align 16
  %4 = alloca [1024 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_begin1.preheader ]
  %5 = shl i64 %indvar, 2
  %scevgep = getelementptr [1024 x float], [1024 x float]* %4, i64 0, i64 %5
  %scevgep19 = bitcast float* %scevgep to i8*
  %6 = shl i64 %indvar, 4
  %scevgep20 = getelementptr i8, i8* %0, i64 %6
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep19, i8* align 4 %scevgep20, i64 16, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond21 = icmp eq i64 %indvar.next, 256
  br i1 %exitcond21, label %for_begin4.preheader, label %for_begin1.preheader, !prof !24

for_begin4.preheader:                             ; preds = %for_begin1.preheader
  %7 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_begin10.preheader, %for_begin4.preheader
  %indvars.iv10 = phi i64 [ 0, %for_begin4.preheader ], [ %indvars.iv.next11, %for_begin10.preheader ]
  %8 = mul nuw nsw i64 %indvars.iv10, 20
  %9 = shl i64 %indvars.iv10, 2
  %10 = add nuw nsw i64 %9, 1024
  %11 = add nuw nsw i64 %9, 2048
  %12 = add nuw nsw i64 %9, 3072
  %13 = add nuw nsw i64 %9, 4096
  %14 = getelementptr inbounds float, float* %7, i64 %9
  %15 = load float, float* %14, align 4, !tbaa !25
  %16 = insertelement <5 x float> undef, float %15, i32 0
  %17 = getelementptr inbounds float, float* %7, i64 %10
  %18 = load float, float* %17, align 4, !tbaa !25
  %19 = insertelement <5 x float> %16, float %18, i32 1
  %20 = getelementptr inbounds float, float* %7, i64 %11
  %21 = load float, float* %20, align 4, !tbaa !25
  %22 = insertelement <5 x float> %19, float %21, i32 2
  %23 = getelementptr inbounds float, float* %7, i64 %12
  %24 = load float, float* %23, align 4, !tbaa !25
  %25 = insertelement <5 x float> %22, float %24, i32 3
  %26 = getelementptr inbounds float, float* %7, i64 %13
  %27 = load float, float* %26, align 4, !tbaa !25
  %28 = insertelement <5 x float> %25, float %27, i32 4
  %29 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %8
  %30 = bitcast float* %29 to <5 x float>*
  store <5 x float> %28, <5 x float>* %30, align 16, !tbaa !28
  %31 = add nuw nsw i64 %8, 5
  %32 = or i64 %9, 1
  %33 = add nuw nsw i64 %32, 1024
  %34 = add nuw nsw i64 %32, 2048
  %35 = add nuw nsw i64 %32, 3072
  %36 = add nuw nsw i64 %32, 4096
  %37 = getelementptr inbounds float, float* %7, i64 %32
  %38 = load float, float* %37, align 4, !tbaa !25
  %39 = insertelement <5 x float> undef, float %38, i32 0
  %40 = getelementptr inbounds float, float* %7, i64 %33
  %41 = load float, float* %40, align 4, !tbaa !25
  %42 = insertelement <5 x float> %39, float %41, i32 1
  %43 = getelementptr inbounds float, float* %7, i64 %34
  %44 = load float, float* %43, align 4, !tbaa !25
  %45 = insertelement <5 x float> %42, float %44, i32 2
  %46 = getelementptr inbounds float, float* %7, i64 %35
  %47 = load float, float* %46, align 4, !tbaa !25
  %48 = insertelement <5 x float> %45, float %47, i32 3
  %49 = getelementptr inbounds float, float* %7, i64 %36
  %50 = load float, float* %49, align 4, !tbaa !25
  %51 = insertelement <5 x float> %48, float %50, i32 4
  %52 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %31
  %53 = bitcast float* %52 to <5 x float>*
  store <5 x float> %51, <5 x float>* %53, align 4, !tbaa !28
  %54 = add nuw nsw i64 %8, 10
  %55 = or i64 %9, 2
  %56 = add nuw nsw i64 %55, 1024
  %57 = add nuw nsw i64 %55, 2048
  %58 = add nuw nsw i64 %55, 3072
  %59 = add nuw nsw i64 %55, 4096
  %60 = getelementptr inbounds float, float* %7, i64 %55
  %61 = load float, float* %60, align 4, !tbaa !25
  %62 = insertelement <5 x float> undef, float %61, i32 0
  %63 = getelementptr inbounds float, float* %7, i64 %56
  %64 = load float, float* %63, align 4, !tbaa !25
  %65 = insertelement <5 x float> %62, float %64, i32 1
  %66 = getelementptr inbounds float, float* %7, i64 %57
  %67 = load float, float* %66, align 4, !tbaa !25
  %68 = insertelement <5 x float> %65, float %67, i32 2
  %69 = getelementptr inbounds float, float* %7, i64 %58
  %70 = load float, float* %69, align 4, !tbaa !25
  %71 = insertelement <5 x float> %68, float %70, i32 3
  %72 = getelementptr inbounds float, float* %7, i64 %59
  %73 = load float, float* %72, align 4, !tbaa !25
  %74 = insertelement <5 x float> %71, float %73, i32 4
  %75 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %54
  %76 = bitcast float* %75 to <5 x float>*
  store <5 x float> %74, <5 x float>* %76, align 8, !tbaa !28
  %77 = add nuw nsw i64 %8, 15
  %78 = or i64 %9, 3
  %79 = add nuw nsw i64 %78, 1024
  %80 = add nuw nsw i64 %78, 2048
  %81 = add nuw nsw i64 %78, 3072
  %82 = add nuw nsw i64 %78, 4096
  %83 = getelementptr inbounds float, float* %7, i64 %78
  %84 = load float, float* %83, align 4, !tbaa !25
  %85 = insertelement <5 x float> undef, float %84, i32 0
  %86 = getelementptr inbounds float, float* %7, i64 %79
  %87 = load float, float* %86, align 4, !tbaa !25
  %88 = insertelement <5 x float> %85, float %87, i32 1
  %89 = getelementptr inbounds float, float* %7, i64 %80
  %90 = load float, float* %89, align 4, !tbaa !25
  %91 = insertelement <5 x float> %88, float %90, i32 2
  %92 = getelementptr inbounds float, float* %7, i64 %81
  %93 = load float, float* %92, align 4, !tbaa !25
  %94 = insertelement <5 x float> %91, float %93, i32 3
  %95 = getelementptr inbounds float, float* %7, i64 %82
  %96 = load float, float* %95, align 4, !tbaa !25
  %97 = insertelement <5 x float> %94, float %96, i32 4
  %98 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %77
  %99 = bitcast float* %98 to <5 x float>*
  store <5 x float> %97, <5 x float>* %99, align 4, !tbaa !28
  %indvars.iv.next11 = add nuw nsw i64 %indvars.iv10, 1
  %exitcond12 = icmp eq i64 %indvars.iv.next11, 256
  br i1 %exitcond12, label %for_begin10.preheader.1, label %for_begin10.preheader, !prof !24

for_begin19.preheader:                            ; preds = %for_begin10.preheader.1, %for_begin19.preheader
  %indvars.iv = phi i64 [ %indvars.iv.next, %for_begin19.preheader ], [ 0, %for_begin10.preheader.1 ]
  %.03 = phi <5 x float> [ %139, %for_begin19.preheader ], [ zeroinitializer, %for_begin10.preheader.1 ]
  %100 = shl i64 %indvars.iv, 2
  %101 = mul nuw nsw i64 %indvars.iv, 20
  %102 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %100
  %103 = load float, float* %102, align 16, !tbaa !31
  %104 = insertelement <5 x float> undef, float %103, i32 0
  %105 = shufflevector <5 x float> %104, <5 x float> undef, <5 x i32> zeroinitializer
  %106 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %101
  %107 = bitcast float* %106 to <5 x float>*
  %108 = load <5 x float>, <5 x float>* %107, align 16, !tbaa !28
  %109 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %105, <5 x float> %108, <5 x float> %.03)
  %110 = or i64 %100, 1
  %111 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %110
  %112 = load float, float* %111, align 4, !tbaa !31
  %113 = insertelement <5 x float> undef, float %112, i32 0
  %114 = shufflevector <5 x float> %113, <5 x float> undef, <5 x i32> zeroinitializer
  %115 = add nuw nsw i64 %101, 5
  %116 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %115
  %117 = bitcast float* %116 to <5 x float>*
  %118 = load <5 x float>, <5 x float>* %117, align 4, !tbaa !28
  %119 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %114, <5 x float> %118, <5 x float> %109)
  %120 = or i64 %100, 2
  %121 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %120
  %122 = load float, float* %121, align 8, !tbaa !31
  %123 = insertelement <5 x float> undef, float %122, i32 0
  %124 = shufflevector <5 x float> %123, <5 x float> undef, <5 x i32> zeroinitializer
  %125 = add nuw nsw i64 %101, 10
  %126 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %125
  %127 = bitcast float* %126 to <5 x float>*
  %128 = load <5 x float>, <5 x float>* %127, align 8, !tbaa !28
  %129 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %124, <5 x float> %128, <5 x float> %119)
  %130 = or i64 %100, 3
  %131 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !31
  %133 = insertelement <5 x float> undef, float %132, i32 0
  %134 = shufflevector <5 x float> %133, <5 x float> undef, <5 x i32> zeroinitializer
  %135 = add nuw nsw i64 %101, 15
  %136 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %135
  %137 = bitcast float* %136 to <5 x float>*
  %138 = load <5 x float>, <5 x float>* %137, align 4, !tbaa !28
  %139 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %134, <5 x float> %138, <5 x float> %129)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end18, label %for_begin19.preheader, !prof !24

for_end18:                                        ; preds = %for_begin19.preheader
  %140 = bitcast i8* %2 to <5 x float>*
  store <5 x float> %139, <5 x float>* %140, align 4, !tbaa !34
  br label %for_begin19.preheader.1

for_begin19.preheader.1:                          ; preds = %for_begin19.preheader.1, %for_end18
  %indvars.iv.1 = phi i64 [ 0, %for_end18 ], [ %indvars.iv.next.1, %for_begin19.preheader.1 ]
  %.03.1 = phi <5 x float> [ zeroinitializer, %for_end18 ], [ %181, %for_begin19.preheader.1 ]
  %141 = shl i64 %indvars.iv.1, 2
  %142 = mul nuw nsw i64 %indvars.iv.1, 20
  %143 = add nuw nsw i64 %142, 5120
  %144 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %141
  %145 = load float, float* %144, align 16, !tbaa !31
  %146 = insertelement <5 x float> undef, float %145, i32 0
  %147 = shufflevector <5 x float> %146, <5 x float> undef, <5 x i32> zeroinitializer
  %148 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %143
  %149 = bitcast float* %148 to <5 x float>*
  %150 = load <5 x float>, <5 x float>* %149, align 16, !tbaa !28
  %151 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %147, <5 x float> %150, <5 x float> %.03.1)
  %152 = or i64 %141, 1
  %153 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !31
  %155 = insertelement <5 x float> undef, float %154, i32 0
  %156 = shufflevector <5 x float> %155, <5 x float> undef, <5 x i32> zeroinitializer
  %157 = add nuw nsw i64 %142, 5125
  %158 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %157
  %159 = bitcast float* %158 to <5 x float>*
  %160 = load <5 x float>, <5 x float>* %159, align 4, !tbaa !28
  %161 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %156, <5 x float> %160, <5 x float> %151)
  %162 = or i64 %141, 2
  %163 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %162
  %164 = load float, float* %163, align 8, !tbaa !31
  %165 = insertelement <5 x float> undef, float %164, i32 0
  %166 = shufflevector <5 x float> %165, <5 x float> undef, <5 x i32> zeroinitializer
  %167 = add nuw nsw i64 %142, 5130
  %168 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %167
  %169 = bitcast float* %168 to <5 x float>*
  %170 = load <5 x float>, <5 x float>* %169, align 8, !tbaa !28
  %171 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %166, <5 x float> %170, <5 x float> %161)
  %172 = or i64 %141, 3
  %173 = getelementptr inbounds [1024 x float], [1024 x float]* %4, i64 0, i64 %172
  %174 = load float, float* %173, align 4, !tbaa !31
  %175 = insertelement <5 x float> undef, float %174, i32 0
  %176 = shufflevector <5 x float> %175, <5 x float> undef, <5 x i32> zeroinitializer
  %177 = add nuw nsw i64 %142, 5135
  %178 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %177
  %179 = bitcast float* %178 to <5 x float>*
  %180 = load <5 x float>, <5 x float>* %179, align 4, !tbaa !28
  %181 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %176, <5 x float> %180, <5 x float> %171)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 256
  br i1 %exitcond.1, label %for_end18.1, label %for_begin19.preheader.1, !prof !24

for_end18.1:                                      ; preds = %for_begin19.preheader.1
  %182 = getelementptr inbounds i8, i8* %2, i64 20
  %183 = bitcast i8* %182 to <5 x float>*
  store <5 x float> %181, <5 x float>* %183, align 4, !tbaa !34
  ret void

for_begin10.preheader.1:                          ; preds = %for_begin10.preheader, %for_begin10.preheader.1
  %indvars.iv10.1 = phi i64 [ %indvars.iv.next11.1, %for_begin10.preheader.1 ], [ 0, %for_begin10.preheader ]
  %184 = mul nuw nsw i64 %indvars.iv10.1, 20
  %185 = add nuw nsw i64 %184, 5120
  %186 = shl i64 %indvars.iv10.1, 2
  %187 = add nuw nsw i64 %186, 5120
  %188 = add nsw i64 %186, 6144
  %189 = add nsw i64 %186, 7168
  %190 = add nsw i64 %186, 8192
  %191 = add nsw i64 %186, 9216
  %192 = getelementptr inbounds float, float* %7, i64 %187
  %193 = load float, float* %192, align 4, !tbaa !25
  %194 = insertelement <5 x float> undef, float %193, i32 0
  %195 = getelementptr inbounds float, float* %7, i64 %188
  %196 = load float, float* %195, align 4, !tbaa !25
  %197 = insertelement <5 x float> %194, float %196, i32 1
  %198 = getelementptr inbounds float, float* %7, i64 %189
  %199 = load float, float* %198, align 4, !tbaa !25
  %200 = insertelement <5 x float> %197, float %199, i32 2
  %201 = getelementptr inbounds float, float* %7, i64 %190
  %202 = load float, float* %201, align 4, !tbaa !25
  %203 = insertelement <5 x float> %200, float %202, i32 3
  %204 = getelementptr inbounds float, float* %7, i64 %191
  %205 = load float, float* %204, align 4, !tbaa !25
  %206 = insertelement <5 x float> %203, float %205, i32 4
  %207 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %185
  %208 = bitcast float* %207 to <5 x float>*
  store <5 x float> %206, <5 x float>* %208, align 16, !tbaa !28
  %209 = add nuw nsw i64 %184, 5125
  %210 = add nsw i64 %186, 5121
  %211 = add nsw i64 %186, 6145
  %212 = add nsw i64 %186, 7169
  %213 = add nsw i64 %186, 8193
  %214 = add nsw i64 %186, 9217
  %215 = getelementptr inbounds float, float* %7, i64 %210
  %216 = load float, float* %215, align 4, !tbaa !25
  %217 = insertelement <5 x float> undef, float %216, i32 0
  %218 = getelementptr inbounds float, float* %7, i64 %211
  %219 = load float, float* %218, align 4, !tbaa !25
  %220 = insertelement <5 x float> %217, float %219, i32 1
  %221 = getelementptr inbounds float, float* %7, i64 %212
  %222 = load float, float* %221, align 4, !tbaa !25
  %223 = insertelement <5 x float> %220, float %222, i32 2
  %224 = getelementptr inbounds float, float* %7, i64 %213
  %225 = load float, float* %224, align 4, !tbaa !25
  %226 = insertelement <5 x float> %223, float %225, i32 3
  %227 = getelementptr inbounds float, float* %7, i64 %214
  %228 = load float, float* %227, align 4, !tbaa !25
  %229 = insertelement <5 x float> %226, float %228, i32 4
  %230 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %209
  %231 = bitcast float* %230 to <5 x float>*
  store <5 x float> %229, <5 x float>* %231, align 4, !tbaa !28
  %232 = add nuw nsw i64 %184, 5130
  %233 = add nsw i64 %186, 5122
  %234 = add nsw i64 %186, 6146
  %235 = add nsw i64 %186, 7170
  %236 = add nsw i64 %186, 8194
  %237 = add nsw i64 %186, 9218
  %238 = getelementptr inbounds float, float* %7, i64 %233
  %239 = load float, float* %238, align 4, !tbaa !25
  %240 = insertelement <5 x float> undef, float %239, i32 0
  %241 = getelementptr inbounds float, float* %7, i64 %234
  %242 = load float, float* %241, align 4, !tbaa !25
  %243 = insertelement <5 x float> %240, float %242, i32 1
  %244 = getelementptr inbounds float, float* %7, i64 %235
  %245 = load float, float* %244, align 4, !tbaa !25
  %246 = insertelement <5 x float> %243, float %245, i32 2
  %247 = getelementptr inbounds float, float* %7, i64 %236
  %248 = load float, float* %247, align 4, !tbaa !25
  %249 = insertelement <5 x float> %246, float %248, i32 3
  %250 = getelementptr inbounds float, float* %7, i64 %237
  %251 = load float, float* %250, align 4, !tbaa !25
  %252 = insertelement <5 x float> %249, float %251, i32 4
  %253 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %232
  %254 = bitcast float* %253 to <5 x float>*
  store <5 x float> %252, <5 x float>* %254, align 8, !tbaa !28
  %255 = add nuw nsw i64 %184, 5135
  %256 = add nsw i64 %186, 5123
  %257 = add nsw i64 %186, 6147
  %258 = add nsw i64 %186, 7171
  %259 = add nsw i64 %186, 8195
  %260 = add nsw i64 %186, 9219
  %261 = getelementptr inbounds float, float* %7, i64 %256
  %262 = load float, float* %261, align 4, !tbaa !25
  %263 = insertelement <5 x float> undef, float %262, i32 0
  %264 = getelementptr inbounds float, float* %7, i64 %257
  %265 = load float, float* %264, align 4, !tbaa !25
  %266 = insertelement <5 x float> %263, float %265, i32 1
  %267 = getelementptr inbounds float, float* %7, i64 %258
  %268 = load float, float* %267, align 4, !tbaa !25
  %269 = insertelement <5 x float> %266, float %268, i32 2
  %270 = getelementptr inbounds float, float* %7, i64 %259
  %271 = load float, float* %270, align 4, !tbaa !25
  %272 = insertelement <5 x float> %269, float %271, i32 3
  %273 = getelementptr inbounds float, float* %7, i64 %260
  %274 = load float, float* %273, align 4, !tbaa !25
  %275 = insertelement <5 x float> %272, float %274, i32 4
  %276 = getelementptr inbounds [2048 x <5 x float>], [2048 x <5 x float>]* %3, i64 0, i64 0, i64 %255
  %277 = bitcast float* %276 to <5 x float>*
  store <5 x float> %275, <5 x float>* %277, align 4, !tbaa !28
  %indvars.iv.next11.1 = add nuw nsw i64 %indvars.iv10.1, 1
  %exitcond12.1 = icmp eq i64 %indvars.iv.next11.1, 256
  br i1 %exitcond12.1, label %for_begin19.preheader, label %for_begin10.preheader.1, !prof !24
}

; Function Attrs: nounwind readnone speculatable
declare <5 x float> @llvm.fmuladd.v5f32(<5 x float>, <5 x float>, <5 x float>) #4

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_nn_conv2d_1_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_1_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = alloca [401408 x <8 x float>], align 16
  %4 = alloca [3136 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_begin1.preheader ]
  %5 = trunc i64 %indvar to i32
  %6 = mul nuw nsw i64 %indvar, 56
  %7 = trunc i64 %indvar to i32
  %8 = mul i32 %7, 7
  %9 = udiv i32 %5, 7
  %10 = mul i32 %9, 343
  %11 = add i32 %8, %10
  %12 = zext i32 %11 to i64
  %13 = shl nuw nsw i64 %12, 2
  %scevgep = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %6
  %scevgep30 = bitcast float* %scevgep to i8*
  %scevgep31 = getelementptr i8, i8* %0, i64 %13
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep30, i8* align 4 %scevgep31, i64 28, i1 false)
  %14 = or i64 %6, 7
  %scevgep.1 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %14
  %scevgep30.1 = bitcast float* %scevgep.1 to i8*
  %15 = add nuw nsw i64 %13, 196
  %scevgep31.1 = getelementptr i8, i8* %0, i64 %15
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep30.1, i8* align 4 %scevgep31.1, i64 28, i1 false)
  %16 = add nuw nsw i64 %6, 14
  %scevgep.2 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %16
  %scevgep30.2 = bitcast float* %scevgep.2 to i8*
  %17 = add nuw nsw i64 %13, 392
  %scevgep31.2 = getelementptr i8, i8* %0, i64 %17
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep30.2, i8* align 4 %scevgep31.2, i64 28, i1 false)
  %18 = add nuw nsw i64 %6, 21
  %scevgep.3 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %18
  %scevgep30.3 = bitcast float* %scevgep.3 to i8*
  %19 = add nuw nsw i64 %13, 588
  %scevgep31.3 = getelementptr i8, i8* %0, i64 %19
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep30.3, i8* align 4 %scevgep31.3, i64 28, i1 false)
  %20 = add nuw nsw i64 %6, 28
  %scevgep.4 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %20
  %scevgep30.4 = bitcast float* %scevgep.4 to i8*
  %21 = add nuw nsw i64 %13, 784
  %scevgep31.4 = getelementptr i8, i8* %0, i64 %21
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep30.4, i8* align 4 %scevgep31.4, i64 28, i1 false)
  %22 = add nuw nsw i64 %6, 35
  %scevgep.5 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %22
  %scevgep30.5 = bitcast float* %scevgep.5 to i8*
  %23 = add nuw nsw i64 %13, 980
  %scevgep31.5 = getelementptr i8, i8* %0, i64 %23
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep30.5, i8* align 4 %scevgep31.5, i64 28, i1 false)
  %24 = add nuw nsw i64 %6, 42
  %scevgep.6 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %24
  %scevgep30.6 = bitcast float* %scevgep.6 to i8*
  %25 = add nuw nsw i64 %13, 1176
  %scevgep31.6 = getelementptr i8, i8* %0, i64 %25
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep30.6, i8* align 4 %scevgep31.6, i64 28, i1 false)
  %26 = add nuw nsw i64 %6, 49
  %scevgep.7 = getelementptr [3136 x float], [3136 x float]* %4, i64 0, i64 %26
  %scevgep30.7 = bitcast float* %scevgep.7 to i8*
  %27 = add nuw nsw i64 %13, 1372
  %scevgep31.7 = getelementptr i8, i8* %0, i64 %27
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep30.7, i8* align 4 %scevgep31.7, i64 28, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond33 = icmp eq i64 %indvar.next, 56
  br i1 %exitcond33, label %for_begin7.preheader, label %for_begin1.preheader, !prof !24

for_begin7.preheader:                             ; preds = %for_begin1.preheader
  %28 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %29 = phi i32 [ 0, %for_begin7.preheader ], [ %44, %for_end12 ]
  %30 = urem i32 %29, 7
  %31 = mul nuw nsw i32 %30, 448
  %32 = udiv i32 %29, 7
  %33 = mul nsw i32 %32, 25088
  %34 = add nuw i32 %31, %33
  %35 = mul nuw nsw i32 %30, 7
  %36 = or i32 %35, %33
  %37 = zext i32 %36 to i64
  %38 = zext i32 %34 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %39 = bitcast i8* %2 to float*
  br label %for_begin22.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv21 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next22, %for_end15 ]
  %40 = mul nuw nsw i64 %indvars.iv21, 3136
  %41 = add nuw nsw i64 %40, %38
  %42 = mul nuw nsw i64 %indvars.iv21, 392
  %43 = add nuw nsw i64 %42, %37
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %44 = add nuw nsw i32 %29, 1
  %exitcond24 = icmp eq i32 %44, 896
  br i1 %exitcond24, label %for_begin19.preheader, label %for_begin10.preheader, !prof !24

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv18 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next19, %for_begin16.preheader ]
  %45 = shl i64 %indvars.iv18, 6
  %46 = add nuw nsw i64 %41, %45
  %47 = add nuw nsw i64 %43, %indvars.iv18
  %48 = getelementptr inbounds float, float* %28, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !37
  %50 = insertelement <8 x float> undef, float %49, i32 0
  %51 = shl i64 %47, 32
  %sext = add i64 %51, 13469017440256
  %52 = ashr exact i64 %sext, 32
  %53 = getelementptr inbounds float, float* %28, i64 %52
  %54 = load float, float* %53, align 4, !tbaa !37
  %55 = insertelement <8 x float> %50, float %54, i32 1
  %56 = shl i64 %47, 32
  %sext34 = add i64 %56, 26938034880512
  %57 = ashr exact i64 %sext34, 32
  %58 = getelementptr inbounds float, float* %28, i64 %57
  %59 = load float, float* %58, align 4, !tbaa !37
  %60 = insertelement <8 x float> %55, float %59, i32 2
  %61 = shl i64 %47, 32
  %sext35 = add i64 %61, 40407052320768
  %62 = ashr exact i64 %sext35, 32
  %63 = getelementptr inbounds float, float* %28, i64 %62
  %64 = load float, float* %63, align 4, !tbaa !37
  %65 = insertelement <8 x float> %60, float %64, i32 3
  %66 = shl i64 %47, 32
  %sext36 = add i64 %66, 53876069761024
  %67 = ashr exact i64 %sext36, 32
  %68 = getelementptr inbounds float, float* %28, i64 %67
  %69 = load float, float* %68, align 4, !tbaa !37
  %70 = insertelement <8 x float> %65, float %69, i32 4
  %71 = shl i64 %47, 32
  %sext37 = add i64 %71, 67345087201280
  %72 = ashr exact i64 %sext37, 32
  %73 = getelementptr inbounds float, float* %28, i64 %72
  %74 = load float, float* %73, align 4, !tbaa !37
  %75 = insertelement <8 x float> %70, float %74, i32 5
  %76 = shl i64 %47, 32
  %sext38 = add i64 %76, 80814104641536
  %77 = ashr exact i64 %sext38, 32
  %78 = getelementptr inbounds float, float* %28, i64 %77
  %79 = load float, float* %78, align 4, !tbaa !37
  %80 = insertelement <8 x float> %75, float %79, i32 6
  %81 = shl i64 %47, 32
  %sext39 = add i64 %81, 94283122081792
  %82 = ashr exact i64 %sext39, 32
  %83 = getelementptr inbounds float, float* %28, i64 %82
  %84 = load float, float* %83, align 4, !tbaa !37
  %85 = insertelement <8 x float> %80, float %84, i32 7
  %86 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %46
  %87 = bitcast float* %86 to <8 x float>*
  store <8 x float> %85, <8 x float>* %87, align 16, !tbaa !40
  %88 = or i64 %46, 8
  %89 = add nuw nsw i64 %47, 49
  %90 = getelementptr inbounds float, float* %28, i64 %89
  %91 = load float, float* %90, align 4, !tbaa !37
  %92 = insertelement <8 x float> undef, float %91, i32 0
  %93 = shl i64 %47, 32
  %sext40 = add i64 %93, 13679470837760
  %94 = ashr exact i64 %sext40, 32
  %95 = getelementptr inbounds float, float* %28, i64 %94
  %96 = load float, float* %95, align 4, !tbaa !37
  %97 = insertelement <8 x float> %92, float %96, i32 1
  %98 = shl i64 %47, 32
  %sext41 = add i64 %98, 27148488278016
  %99 = ashr exact i64 %sext41, 32
  %100 = getelementptr inbounds float, float* %28, i64 %99
  %101 = load float, float* %100, align 4, !tbaa !37
  %102 = insertelement <8 x float> %97, float %101, i32 2
  %103 = shl i64 %47, 32
  %sext42 = add i64 %103, 40617505718272
  %104 = ashr exact i64 %sext42, 32
  %105 = getelementptr inbounds float, float* %28, i64 %104
  %106 = load float, float* %105, align 4, !tbaa !37
  %107 = insertelement <8 x float> %102, float %106, i32 3
  %108 = shl i64 %47, 32
  %sext43 = add i64 %108, 54086523158528
  %109 = ashr exact i64 %sext43, 32
  %110 = getelementptr inbounds float, float* %28, i64 %109
  %111 = load float, float* %110, align 4, !tbaa !37
  %112 = insertelement <8 x float> %107, float %111, i32 4
  %113 = shl i64 %47, 32
  %sext44 = add i64 %113, 67555540598784
  %114 = ashr exact i64 %sext44, 32
  %115 = getelementptr inbounds float, float* %28, i64 %114
  %116 = load float, float* %115, align 4, !tbaa !37
  %117 = insertelement <8 x float> %112, float %116, i32 5
  %118 = shl i64 %47, 32
  %sext45 = add i64 %118, 81024558039040
  %119 = ashr exact i64 %sext45, 32
  %120 = getelementptr inbounds float, float* %28, i64 %119
  %121 = load float, float* %120, align 4, !tbaa !37
  %122 = insertelement <8 x float> %117, float %121, i32 6
  %123 = shl i64 %47, 32
  %sext46 = add i64 %123, 94493575479296
  %124 = ashr exact i64 %sext46, 32
  %125 = getelementptr inbounds float, float* %28, i64 %124
  %126 = load float, float* %125, align 4, !tbaa !37
  %127 = insertelement <8 x float> %122, float %126, i32 7
  %128 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %88
  %129 = bitcast float* %128 to <8 x float>*
  store <8 x float> %127, <8 x float>* %129, align 16, !tbaa !40
  %130 = or i64 %46, 16
  %131 = add nuw nsw i64 %47, 98
  %132 = getelementptr inbounds float, float* %28, i64 %131
  %133 = load float, float* %132, align 4, !tbaa !37
  %134 = insertelement <8 x float> undef, float %133, i32 0
  %135 = shl i64 %47, 32
  %sext47 = add i64 %135, 13889924235264
  %136 = ashr exact i64 %sext47, 32
  %137 = getelementptr inbounds float, float* %28, i64 %136
  %138 = load float, float* %137, align 4, !tbaa !37
  %139 = insertelement <8 x float> %134, float %138, i32 1
  %140 = shl i64 %47, 32
  %sext48 = add i64 %140, 27358941675520
  %141 = ashr exact i64 %sext48, 32
  %142 = getelementptr inbounds float, float* %28, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !37
  %144 = insertelement <8 x float> %139, float %143, i32 2
  %145 = shl i64 %47, 32
  %sext49 = add i64 %145, 40827959115776
  %146 = ashr exact i64 %sext49, 32
  %147 = getelementptr inbounds float, float* %28, i64 %146
  %148 = load float, float* %147, align 4, !tbaa !37
  %149 = insertelement <8 x float> %144, float %148, i32 3
  %150 = shl i64 %47, 32
  %sext50 = add i64 %150, 54296976556032
  %151 = ashr exact i64 %sext50, 32
  %152 = getelementptr inbounds float, float* %28, i64 %151
  %153 = load float, float* %152, align 4, !tbaa !37
  %154 = insertelement <8 x float> %149, float %153, i32 4
  %155 = shl i64 %47, 32
  %sext51 = add i64 %155, 67765993996288
  %156 = ashr exact i64 %sext51, 32
  %157 = getelementptr inbounds float, float* %28, i64 %156
  %158 = load float, float* %157, align 4, !tbaa !37
  %159 = insertelement <8 x float> %154, float %158, i32 5
  %160 = shl i64 %47, 32
  %sext52 = add i64 %160, 81235011436544
  %161 = ashr exact i64 %sext52, 32
  %162 = getelementptr inbounds float, float* %28, i64 %161
  %163 = load float, float* %162, align 4, !tbaa !37
  %164 = insertelement <8 x float> %159, float %163, i32 6
  %165 = shl i64 %47, 32
  %sext53 = add i64 %165, 94704028876800
  %166 = ashr exact i64 %sext53, 32
  %167 = getelementptr inbounds float, float* %28, i64 %166
  %168 = load float, float* %167, align 4, !tbaa !37
  %169 = insertelement <8 x float> %164, float %168, i32 7
  %170 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %130
  %171 = bitcast float* %170 to <8 x float>*
  store <8 x float> %169, <8 x float>* %171, align 16, !tbaa !40
  %172 = or i64 %46, 24
  %173 = add nuw nsw i64 %47, 147
  %174 = getelementptr inbounds float, float* %28, i64 %173
  %175 = load float, float* %174, align 4, !tbaa !37
  %176 = insertelement <8 x float> undef, float %175, i32 0
  %177 = shl i64 %47, 32
  %sext54 = add i64 %177, 14100377632768
  %178 = ashr exact i64 %sext54, 32
  %179 = getelementptr inbounds float, float* %28, i64 %178
  %180 = load float, float* %179, align 4, !tbaa !37
  %181 = insertelement <8 x float> %176, float %180, i32 1
  %182 = shl i64 %47, 32
  %sext55 = add i64 %182, 27569395073024
  %183 = ashr exact i64 %sext55, 32
  %184 = getelementptr inbounds float, float* %28, i64 %183
  %185 = load float, float* %184, align 4, !tbaa !37
  %186 = insertelement <8 x float> %181, float %185, i32 2
  %187 = shl i64 %47, 32
  %sext56 = add i64 %187, 41038412513280
  %188 = ashr exact i64 %sext56, 32
  %189 = getelementptr inbounds float, float* %28, i64 %188
  %190 = load float, float* %189, align 4, !tbaa !37
  %191 = insertelement <8 x float> %186, float %190, i32 3
  %192 = shl i64 %47, 32
  %sext57 = add i64 %192, 54507429953536
  %193 = ashr exact i64 %sext57, 32
  %194 = getelementptr inbounds float, float* %28, i64 %193
  %195 = load float, float* %194, align 4, !tbaa !37
  %196 = insertelement <8 x float> %191, float %195, i32 4
  %197 = shl i64 %47, 32
  %sext58 = add i64 %197, 67976447393792
  %198 = ashr exact i64 %sext58, 32
  %199 = getelementptr inbounds float, float* %28, i64 %198
  %200 = load float, float* %199, align 4, !tbaa !37
  %201 = insertelement <8 x float> %196, float %200, i32 5
  %202 = shl i64 %47, 32
  %sext59 = add i64 %202, 81445464834048
  %203 = ashr exact i64 %sext59, 32
  %204 = getelementptr inbounds float, float* %28, i64 %203
  %205 = load float, float* %204, align 4, !tbaa !37
  %206 = insertelement <8 x float> %201, float %205, i32 6
  %207 = shl i64 %47, 32
  %sext60 = add i64 %207, 94914482274304
  %208 = ashr exact i64 %sext60, 32
  %209 = getelementptr inbounds float, float* %28, i64 %208
  %210 = load float, float* %209, align 4, !tbaa !37
  %211 = insertelement <8 x float> %206, float %210, i32 7
  %212 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %172
  %213 = bitcast float* %212 to <8 x float>*
  store <8 x float> %211, <8 x float>* %213, align 16, !tbaa !40
  %214 = or i64 %46, 32
  %215 = add nuw nsw i64 %47, 196
  %216 = getelementptr inbounds float, float* %28, i64 %215
  %217 = load float, float* %216, align 4, !tbaa !37
  %218 = insertelement <8 x float> undef, float %217, i32 0
  %219 = shl i64 %47, 32
  %sext61 = add i64 %219, 14310831030272
  %220 = ashr exact i64 %sext61, 32
  %221 = getelementptr inbounds float, float* %28, i64 %220
  %222 = load float, float* %221, align 4, !tbaa !37
  %223 = insertelement <8 x float> %218, float %222, i32 1
  %224 = shl i64 %47, 32
  %sext62 = add i64 %224, 27779848470528
  %225 = ashr exact i64 %sext62, 32
  %226 = getelementptr inbounds float, float* %28, i64 %225
  %227 = load float, float* %226, align 4, !tbaa !37
  %228 = insertelement <8 x float> %223, float %227, i32 2
  %229 = shl i64 %47, 32
  %sext63 = add i64 %229, 41248865910784
  %230 = ashr exact i64 %sext63, 32
  %231 = getelementptr inbounds float, float* %28, i64 %230
  %232 = load float, float* %231, align 4, !tbaa !37
  %233 = insertelement <8 x float> %228, float %232, i32 3
  %234 = shl i64 %47, 32
  %sext64 = add i64 %234, 54717883351040
  %235 = ashr exact i64 %sext64, 32
  %236 = getelementptr inbounds float, float* %28, i64 %235
  %237 = load float, float* %236, align 4, !tbaa !37
  %238 = insertelement <8 x float> %233, float %237, i32 4
  %239 = shl i64 %47, 32
  %sext65 = add i64 %239, 68186900791296
  %240 = ashr exact i64 %sext65, 32
  %241 = getelementptr inbounds float, float* %28, i64 %240
  %242 = load float, float* %241, align 4, !tbaa !37
  %243 = insertelement <8 x float> %238, float %242, i32 5
  %244 = shl i64 %47, 32
  %sext66 = add i64 %244, 81655918231552
  %245 = ashr exact i64 %sext66, 32
  %246 = getelementptr inbounds float, float* %28, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !37
  %248 = insertelement <8 x float> %243, float %247, i32 6
  %249 = shl i64 %47, 32
  %sext67 = add i64 %249, 95124935671808
  %250 = ashr exact i64 %sext67, 32
  %251 = getelementptr inbounds float, float* %28, i64 %250
  %252 = load float, float* %251, align 4, !tbaa !37
  %253 = insertelement <8 x float> %248, float %252, i32 7
  %254 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %214
  %255 = bitcast float* %254 to <8 x float>*
  store <8 x float> %253, <8 x float>* %255, align 16, !tbaa !40
  %256 = or i64 %46, 40
  %257 = add nuw nsw i64 %47, 245
  %258 = getelementptr inbounds float, float* %28, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !37
  %260 = insertelement <8 x float> undef, float %259, i32 0
  %261 = shl i64 %47, 32
  %sext68 = add i64 %261, 14521284427776
  %262 = ashr exact i64 %sext68, 32
  %263 = getelementptr inbounds float, float* %28, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !37
  %265 = insertelement <8 x float> %260, float %264, i32 1
  %266 = shl i64 %47, 32
  %sext69 = add i64 %266, 27990301868032
  %267 = ashr exact i64 %sext69, 32
  %268 = getelementptr inbounds float, float* %28, i64 %267
  %269 = load float, float* %268, align 4, !tbaa !37
  %270 = insertelement <8 x float> %265, float %269, i32 2
  %271 = shl i64 %47, 32
  %sext70 = add i64 %271, 41459319308288
  %272 = ashr exact i64 %sext70, 32
  %273 = getelementptr inbounds float, float* %28, i64 %272
  %274 = load float, float* %273, align 4, !tbaa !37
  %275 = insertelement <8 x float> %270, float %274, i32 3
  %276 = shl i64 %47, 32
  %sext71 = add i64 %276, 54928336748544
  %277 = ashr exact i64 %sext71, 32
  %278 = getelementptr inbounds float, float* %28, i64 %277
  %279 = load float, float* %278, align 4, !tbaa !37
  %280 = insertelement <8 x float> %275, float %279, i32 4
  %281 = shl i64 %47, 32
  %sext72 = add i64 %281, 68397354188800
  %282 = ashr exact i64 %sext72, 32
  %283 = getelementptr inbounds float, float* %28, i64 %282
  %284 = load float, float* %283, align 4, !tbaa !37
  %285 = insertelement <8 x float> %280, float %284, i32 5
  %286 = shl i64 %47, 32
  %sext73 = add i64 %286, 81866371629056
  %287 = ashr exact i64 %sext73, 32
  %288 = getelementptr inbounds float, float* %28, i64 %287
  %289 = load float, float* %288, align 4, !tbaa !37
  %290 = insertelement <8 x float> %285, float %289, i32 6
  %291 = shl i64 %47, 32
  %sext74 = add i64 %291, 95335389069312
  %292 = ashr exact i64 %sext74, 32
  %293 = getelementptr inbounds float, float* %28, i64 %292
  %294 = load float, float* %293, align 4, !tbaa !37
  %295 = insertelement <8 x float> %290, float %294, i32 7
  %296 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %256
  %297 = bitcast float* %296 to <8 x float>*
  store <8 x float> %295, <8 x float>* %297, align 16, !tbaa !40
  %298 = or i64 %46, 48
  %299 = add nuw nsw i64 %47, 294
  %300 = getelementptr inbounds float, float* %28, i64 %299
  %301 = load float, float* %300, align 4, !tbaa !37
  %302 = insertelement <8 x float> undef, float %301, i32 0
  %303 = shl i64 %47, 32
  %sext75 = add i64 %303, 14731737825280
  %304 = ashr exact i64 %sext75, 32
  %305 = getelementptr inbounds float, float* %28, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !37
  %307 = insertelement <8 x float> %302, float %306, i32 1
  %308 = shl i64 %47, 32
  %sext76 = add i64 %308, 28200755265536
  %309 = ashr exact i64 %sext76, 32
  %310 = getelementptr inbounds float, float* %28, i64 %309
  %311 = load float, float* %310, align 4, !tbaa !37
  %312 = insertelement <8 x float> %307, float %311, i32 2
  %313 = shl i64 %47, 32
  %sext77 = add i64 %313, 41669772705792
  %314 = ashr exact i64 %sext77, 32
  %315 = getelementptr inbounds float, float* %28, i64 %314
  %316 = load float, float* %315, align 4, !tbaa !37
  %317 = insertelement <8 x float> %312, float %316, i32 3
  %318 = shl i64 %47, 32
  %sext78 = add i64 %318, 55138790146048
  %319 = ashr exact i64 %sext78, 32
  %320 = getelementptr inbounds float, float* %28, i64 %319
  %321 = load float, float* %320, align 4, !tbaa !37
  %322 = insertelement <8 x float> %317, float %321, i32 4
  %323 = shl i64 %47, 32
  %sext79 = add i64 %323, 68607807586304
  %324 = ashr exact i64 %sext79, 32
  %325 = getelementptr inbounds float, float* %28, i64 %324
  %326 = load float, float* %325, align 4, !tbaa !37
  %327 = insertelement <8 x float> %322, float %326, i32 5
  %328 = shl i64 %47, 32
  %sext80 = add i64 %328, 82076825026560
  %329 = ashr exact i64 %sext80, 32
  %330 = getelementptr inbounds float, float* %28, i64 %329
  %331 = load float, float* %330, align 4, !tbaa !37
  %332 = insertelement <8 x float> %327, float %331, i32 6
  %333 = shl i64 %47, 32
  %sext81 = add i64 %333, 95545842466816
  %334 = ashr exact i64 %sext81, 32
  %335 = getelementptr inbounds float, float* %28, i64 %334
  %336 = load float, float* %335, align 4, !tbaa !37
  %337 = insertelement <8 x float> %332, float %336, i32 7
  %338 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %298
  %339 = bitcast float* %338 to <8 x float>*
  store <8 x float> %337, <8 x float>* %339, align 16, !tbaa !40
  %340 = or i64 %46, 56
  %341 = add nuw nsw i64 %47, 343
  %342 = getelementptr inbounds float, float* %28, i64 %341
  %343 = load float, float* %342, align 4, !tbaa !37
  %344 = insertelement <8 x float> undef, float %343, i32 0
  %345 = shl i64 %47, 32
  %sext82 = add i64 %345, 14942191222784
  %346 = ashr exact i64 %sext82, 32
  %347 = getelementptr inbounds float, float* %28, i64 %346
  %348 = load float, float* %347, align 4, !tbaa !37
  %349 = insertelement <8 x float> %344, float %348, i32 1
  %350 = shl i64 %47, 32
  %sext83 = add i64 %350, 28411208663040
  %351 = ashr exact i64 %sext83, 32
  %352 = getelementptr inbounds float, float* %28, i64 %351
  %353 = load float, float* %352, align 4, !tbaa !37
  %354 = insertelement <8 x float> %349, float %353, i32 2
  %355 = shl i64 %47, 32
  %sext84 = add i64 %355, 41880226103296
  %356 = ashr exact i64 %sext84, 32
  %357 = getelementptr inbounds float, float* %28, i64 %356
  %358 = load float, float* %357, align 4, !tbaa !37
  %359 = insertelement <8 x float> %354, float %358, i32 3
  %360 = shl i64 %47, 32
  %sext85 = add i64 %360, 55349243543552
  %361 = ashr exact i64 %sext85, 32
  %362 = getelementptr inbounds float, float* %28, i64 %361
  %363 = load float, float* %362, align 4, !tbaa !37
  %364 = insertelement <8 x float> %359, float %363, i32 4
  %365 = shl i64 %47, 32
  %sext86 = add i64 %365, 68818260983808
  %366 = ashr exact i64 %sext86, 32
  %367 = getelementptr inbounds float, float* %28, i64 %366
  %368 = load float, float* %367, align 4, !tbaa !37
  %369 = insertelement <8 x float> %364, float %368, i32 5
  %370 = shl i64 %47, 32
  %sext87 = add i64 %370, 82287278424064
  %371 = ashr exact i64 %sext87, 32
  %372 = getelementptr inbounds float, float* %28, i64 %371
  %373 = load float, float* %372, align 4, !tbaa !37
  %374 = insertelement <8 x float> %369, float %373, i32 6
  %375 = shl i64 %47, 32
  %sext88 = add i64 %375, 95756295864320
  %376 = ashr exact i64 %sext88, 32
  %377 = getelementptr inbounds float, float* %28, i64 %376
  %378 = load float, float* %377, align 4, !tbaa !37
  %379 = insertelement <8 x float> %374, float %378, i32 7
  %380 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %340
  %381 = bitcast float* %380 to <8 x float>*
  store <8 x float> %379, <8 x float>* %381, align 16, !tbaa !40
  %indvars.iv.next19 = add nuw nsw i64 %indvars.iv18, 1
  %exitcond20 = icmp eq i64 %indvars.iv.next19, 7
  br i1 %exitcond20, label %for_end15, label %for_begin16.preheader, !prof !24

for_end15:                                        ; preds = %for_begin16.preheader
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond23 = icmp eq i64 %indvars.iv.next22, 8
  br i1 %exitcond23, label %for_end12, label %for_begin13.preheader, !prof !24

for_begin22.preheader:                            ; preds = %for_end24, %for_begin19.preheader
  %indvars.iv12 = phi i64 [ 0, %for_begin19.preheader ], [ %indvars.iv.next13, %for_end24 ]
  %382 = mul nuw nsw i64 %indvars.iv12, 25088
  br label %for_begin25.preheader

for_end21:                                        ; preds = %for_end24
  ret void

for_begin25.preheader:                            ; preds = %for_end27, %for_begin22.preheader
  %indvars.iv9 = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next10, %for_end27 ]
  %.05 = phi <8 x float> [ zeroinitializer, %for_begin22.preheader ], [ %473, %for_end27 ]
  %383 = mul nuw nsw i64 %indvars.iv9, 392
  %384 = mul nuw nsw i64 %indvars.iv9, 3136
  %385 = add nuw nsw i64 %384, %382
  br label %for_begin28.preheader

for_end24:                                        ; preds = %for_end27
  %386 = shl nsw i64 %indvars.iv12, 3
  %387 = getelementptr inbounds float, float* %39, i64 %386
  %388 = bitcast float* %387 to <8 x float>*
  store <8 x float> %473, <8 x float>* %388, align 32, !tbaa !43
  %indvars.iv.next13 = add nuw nsw i64 %indvars.iv12, 1
  %exitcond14 = icmp eq i64 %indvars.iv.next13, 128
  br i1 %exitcond14, label %for_end21, label %for_begin22.preheader, !prof !24

for_begin28.preheader:                            ; preds = %for_end30, %for_begin25.preheader
  %indvars.iv6 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next7, %for_end30 ]
  %.14 = phi <8 x float> [ %.05, %for_begin25.preheader ], [ %473, %for_end30 ]
  %389 = mul nuw nsw i64 %indvars.iv6, 56
  %390 = add nuw nsw i64 %389, %383
  %391 = mul nuw nsw i64 %indvars.iv6, 448
  %392 = add nuw nsw i64 %385, %391
  br label %for_begin31.preheader

for_end27:                                        ; preds = %for_end30
  %indvars.iv.next10 = add nuw nsw i64 %indvars.iv9, 1
  %exitcond11 = icmp eq i64 %indvars.iv.next10, 8
  br i1 %exitcond11, label %for_end24, label %for_begin25.preheader, !prof !24

for_begin31.preheader:                            ; preds = %for_begin31.preheader, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_begin31.preheader ]
  %.23 = phi <8 x float> [ %.14, %for_begin28.preheader ], [ %473, %for_begin31.preheader ]
  %393 = add nuw nsw i64 %390, %indvars.iv
  %394 = shl i64 %indvars.iv, 6
  %395 = add nuw nsw i64 %392, %394
  %396 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %393
  %397 = load float, float* %396, align 4, !tbaa !46
  %398 = insertelement <8 x float> undef, float %397, i32 0
  %399 = shufflevector <8 x float> %398, <8 x float> undef, <8 x i32> zeroinitializer
  %400 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %395
  %401 = bitcast float* %400 to <8 x float>*
  %402 = load <8 x float>, <8 x float>* %401, align 16, !tbaa !40
  %403 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %399, <8 x float> %402, <8 x float> %.23)
  %404 = add nuw nsw i64 %393, 7
  %405 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %404
  %406 = load float, float* %405, align 4, !tbaa !46
  %407 = insertelement <8 x float> undef, float %406, i32 0
  %408 = shufflevector <8 x float> %407, <8 x float> undef, <8 x i32> zeroinitializer
  %409 = or i64 %395, 8
  %410 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %409
  %411 = bitcast float* %410 to <8 x float>*
  %412 = load <8 x float>, <8 x float>* %411, align 16, !tbaa !40
  %413 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %408, <8 x float> %412, <8 x float> %403)
  %414 = add nuw nsw i64 %393, 14
  %415 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !46
  %417 = insertelement <8 x float> undef, float %416, i32 0
  %418 = shufflevector <8 x float> %417, <8 x float> undef, <8 x i32> zeroinitializer
  %419 = or i64 %395, 16
  %420 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %419
  %421 = bitcast float* %420 to <8 x float>*
  %422 = load <8 x float>, <8 x float>* %421, align 16, !tbaa !40
  %423 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %418, <8 x float> %422, <8 x float> %413)
  %424 = add nuw nsw i64 %393, 21
  %425 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %424
  %426 = load float, float* %425, align 4, !tbaa !46
  %427 = insertelement <8 x float> undef, float %426, i32 0
  %428 = shufflevector <8 x float> %427, <8 x float> undef, <8 x i32> zeroinitializer
  %429 = or i64 %395, 24
  %430 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %429
  %431 = bitcast float* %430 to <8 x float>*
  %432 = load <8 x float>, <8 x float>* %431, align 16, !tbaa !40
  %433 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %428, <8 x float> %432, <8 x float> %423)
  %434 = add nuw nsw i64 %393, 28
  %435 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %434
  %436 = load float, float* %435, align 4, !tbaa !46
  %437 = insertelement <8 x float> undef, float %436, i32 0
  %438 = shufflevector <8 x float> %437, <8 x float> undef, <8 x i32> zeroinitializer
  %439 = or i64 %395, 32
  %440 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %439
  %441 = bitcast float* %440 to <8 x float>*
  %442 = load <8 x float>, <8 x float>* %441, align 16, !tbaa !40
  %443 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %438, <8 x float> %442, <8 x float> %433)
  %444 = add nuw nsw i64 %393, 35
  %445 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %444
  %446 = load float, float* %445, align 4, !tbaa !46
  %447 = insertelement <8 x float> undef, float %446, i32 0
  %448 = shufflevector <8 x float> %447, <8 x float> undef, <8 x i32> zeroinitializer
  %449 = or i64 %395, 40
  %450 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %449
  %451 = bitcast float* %450 to <8 x float>*
  %452 = load <8 x float>, <8 x float>* %451, align 16, !tbaa !40
  %453 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %448, <8 x float> %452, <8 x float> %443)
  %454 = add nuw nsw i64 %393, 42
  %455 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %454
  %456 = load float, float* %455, align 4, !tbaa !46
  %457 = insertelement <8 x float> undef, float %456, i32 0
  %458 = shufflevector <8 x float> %457, <8 x float> undef, <8 x i32> zeroinitializer
  %459 = or i64 %395, 48
  %460 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %459
  %461 = bitcast float* %460 to <8 x float>*
  %462 = load <8 x float>, <8 x float>* %461, align 16, !tbaa !40
  %463 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %458, <8 x float> %462, <8 x float> %453)
  %464 = add nuw nsw i64 %393, 49
  %465 = getelementptr inbounds [3136 x float], [3136 x float]* %4, i64 0, i64 %464
  %466 = load float, float* %465, align 4, !tbaa !46
  %467 = insertelement <8 x float> undef, float %466, i32 0
  %468 = shufflevector <8 x float> %467, <8 x float> undef, <8 x i32> zeroinitializer
  %469 = or i64 %395, 56
  %470 = getelementptr inbounds [401408 x <8 x float>], [401408 x <8 x float>]* %3, i64 0, i64 0, i64 %469
  %471 = bitcast float* %470 to <8 x float>*
  %472 = load <8 x float>, <8 x float>* %471, align 16, !tbaa !40
  %473 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %468, <8 x float> %472, <8 x float> %463)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 7
  br i1 %exitcond, label %for_end30, label %for_begin31.preheader, !prof !24

for_end30:                                        ; preds = %for_begin31.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 7
  br i1 %exitcond8, label %for_end27, label %for_begin28.preheader, !prof !24
}

; Function Attrs: nounwind readnone speculatable
declare <8 x float> @llvm.fmuladd.v8f32(<8 x float>, <8 x float>, <8 x float>) #4

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_nn_conv2d_2_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_2_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = alloca [14 x <8 x float>], align 32
  %4 = alloca [6400 x <8 x float>], align 16
  %5 = alloca [10368 x float], align 16
  %6 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_end3 ]
  %7 = mul nuw nsw i64 %indvar, 144
  %8 = trunc i64 %indvar to i32
  %9 = urem i32 %8, 18
  %10 = udiv i32 %8, 18
  %.off = add nsw i32 %9, -2
  %11 = icmp ult i32 %.off, 14
  br i1 %11, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep159 = getelementptr [10368 x float], [10368 x float]* %5, i64 0, i64 %7
  %scevgep159160 = bitcast float* %scevgep159 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep159160, i8 0, i64 576, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %12 = mul nsw i32 %10, 1568
  %13 = add nsw i32 %12, -30
  %14 = mul nuw nsw i32 %9, 14
  %15 = add i32 %13, %14
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %7
  store float 0.000000e+00, float* %17, align 16, !tbaa !49
  %18 = or i64 %7, 1
  %19 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %18
  store float 0.000000e+00, float* %19, align 4, !tbaa !49
  %20 = or i64 %7, 2
  %21 = add nsw i64 %16, 2
  %22 = getelementptr inbounds float, float* %6, i64 %21
  %23 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %20
  %24 = bitcast float* %22 to <4 x i32>*
  %25 = load <4 x i32>, <4 x i32>* %24, align 4, !tbaa !52
  %26 = bitcast float* %23 to <4 x i32>*
  store <4 x i32> %25, <4 x i32>* %26, align 8, !tbaa !49
  %27 = or i64 %7, 6
  %28 = add nsw i64 %16, 6
  %29 = getelementptr inbounds float, float* %6, i64 %28
  %30 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %27
  %31 = bitcast float* %29 to <4 x i32>*
  %32 = load <4 x i32>, <4 x i32>* %31, align 4, !tbaa !52
  %33 = bitcast float* %30 to <4 x i32>*
  store <4 x i32> %32, <4 x i32>* %33, align 8, !tbaa !49
  %34 = or i64 %7, 10
  %35 = add nsw i64 %16, 10
  %36 = getelementptr inbounds float, float* %6, i64 %35
  %37 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %34
  %38 = bitcast float* %36 to <4 x i32>*
  %39 = load <4 x i32>, <4 x i32>* %38, align 4, !tbaa !52
  %40 = bitcast float* %37 to <4 x i32>*
  store <4 x i32> %39, <4 x i32>* %40, align 8, !tbaa !49
  %41 = or i64 %7, 14
  %42 = add nsw i64 %16, 14
  %43 = getelementptr inbounds float, float* %6, i64 %42
  %44 = bitcast float* %43 to i32*
  %45 = load i32, i32* %44, align 4, !tbaa !52
  %46 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %41
  %47 = bitcast float* %46 to i32*
  store i32 %45, i32* %47, align 8, !tbaa !49
  %48 = or i64 %7, 15
  %49 = add nsw i64 %16, 15
  %50 = getelementptr inbounds float, float* %6, i64 %49
  %51 = bitcast float* %50 to i32*
  %52 = load i32, i32* %51, align 4, !tbaa !52
  %53 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %48
  %54 = bitcast float* %53 to i32*
  store i32 %52, i32* %54, align 4, !tbaa !49
  %55 = add nuw nsw i64 %7, 16
  %56 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %55
  store float 0.000000e+00, float* %56, align 16, !tbaa !49
  %57 = add nuw nsw i64 %7, 17
  %58 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %57
  store float 0.000000e+00, float* %58, align 4, !tbaa !49
  %59 = add nuw nsw i64 %7, 18
  %60 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %59
  store float 0.000000e+00, float* %60, align 8, !tbaa !49
  %61 = or i64 %59, 1
  %62 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %61
  store float 0.000000e+00, float* %62, align 4, !tbaa !49
  %63 = add nuw nsw i64 %7, 20
  %64 = add nsw i64 %16, 198
  %65 = getelementptr inbounds float, float* %6, i64 %64
  %66 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %63
  %67 = bitcast float* %65 to <4 x i32>*
  %68 = load <4 x i32>, <4 x i32>* %67, align 4, !tbaa !52
  %69 = bitcast float* %66 to <4 x i32>*
  store <4 x i32> %68, <4 x i32>* %69, align 16, !tbaa !49
  %70 = add nuw nsw i64 %7, 24
  %71 = add nsw i64 %16, 202
  %72 = getelementptr inbounds float, float* %6, i64 %71
  %73 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %70
  %74 = bitcast float* %72 to <4 x i32>*
  %75 = load <4 x i32>, <4 x i32>* %74, align 4, !tbaa !52
  %76 = bitcast float* %73 to <4 x i32>*
  store <4 x i32> %75, <4 x i32>* %76, align 16, !tbaa !49
  %77 = add nuw nsw i64 %7, 28
  %78 = add nsw i64 %16, 206
  %79 = getelementptr inbounds float, float* %6, i64 %78
  %80 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %77
  %81 = bitcast float* %79 to <4 x i32>*
  %82 = load <4 x i32>, <4 x i32>* %81, align 4, !tbaa !52
  %83 = bitcast float* %80 to <4 x i32>*
  store <4 x i32> %82, <4 x i32>* %83, align 16, !tbaa !49
  %84 = add nuw nsw i64 %7, 32
  %85 = add nsw i64 %16, 210
  %86 = getelementptr inbounds float, float* %6, i64 %85
  %87 = bitcast float* %86 to i32*
  %88 = load i32, i32* %87, align 4, !tbaa !52
  %89 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %84
  %90 = bitcast float* %89 to i32*
  store i32 %88, i32* %90, align 16, !tbaa !49
  %91 = add nuw nsw i64 %7, 33
  %92 = add nsw i64 %16, 211
  %93 = getelementptr inbounds float, float* %6, i64 %92
  %94 = bitcast float* %93 to i32*
  %95 = load i32, i32* %94, align 4, !tbaa !52
  %96 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %91
  %97 = bitcast float* %96 to i32*
  store i32 %95, i32* %97, align 4, !tbaa !49
  %98 = add nuw nsw i64 %7, 34
  %99 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %98
  store float 0.000000e+00, float* %99, align 8, !tbaa !49
  %100 = add nuw nsw i64 %7, 35
  %101 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %100
  store float 0.000000e+00, float* %101, align 4, !tbaa !49
  %102 = add nuw nsw i64 %7, 36
  %103 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %102
  store float 0.000000e+00, float* %103, align 16, !tbaa !49
  %104 = or i64 %102, 1
  %105 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %104
  store float 0.000000e+00, float* %105, align 4, !tbaa !49
  %106 = add nuw nsw i64 %7, 38
  %107 = add nsw i64 %16, 394
  %108 = getelementptr inbounds float, float* %6, i64 %107
  %109 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %106
  %110 = bitcast float* %108 to <4 x i32>*
  %111 = load <4 x i32>, <4 x i32>* %110, align 4, !tbaa !52
  %112 = bitcast float* %109 to <4 x i32>*
  store <4 x i32> %111, <4 x i32>* %112, align 8, !tbaa !49
  %113 = add nuw nsw i64 %7, 42
  %114 = add nsw i64 %16, 398
  %115 = getelementptr inbounds float, float* %6, i64 %114
  %116 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %113
  %117 = bitcast float* %115 to <4 x i32>*
  %118 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !52
  %119 = bitcast float* %116 to <4 x i32>*
  store <4 x i32> %118, <4 x i32>* %119, align 8, !tbaa !49
  %120 = add nuw nsw i64 %7, 46
  %121 = add nsw i64 %16, 402
  %122 = getelementptr inbounds float, float* %6, i64 %121
  %123 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %120
  %124 = bitcast float* %122 to <4 x i32>*
  %125 = load <4 x i32>, <4 x i32>* %124, align 4, !tbaa !52
  %126 = bitcast float* %123 to <4 x i32>*
  store <4 x i32> %125, <4 x i32>* %126, align 8, !tbaa !49
  %127 = add nuw nsw i64 %7, 50
  %128 = add nsw i64 %16, 406
  %129 = getelementptr inbounds float, float* %6, i64 %128
  %130 = bitcast float* %129 to i32*
  %131 = load i32, i32* %130, align 4, !tbaa !52
  %132 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %127
  %133 = bitcast float* %132 to i32*
  store i32 %131, i32* %133, align 8, !tbaa !49
  %134 = add nuw nsw i64 %7, 51
  %135 = add nsw i64 %16, 407
  %136 = getelementptr inbounds float, float* %6, i64 %135
  %137 = bitcast float* %136 to i32*
  %138 = load i32, i32* %137, align 4, !tbaa !52
  %139 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %134
  %140 = bitcast float* %139 to i32*
  store i32 %138, i32* %140, align 4, !tbaa !49
  %141 = add nuw nsw i64 %7, 52
  %142 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %141
  store float 0.000000e+00, float* %142, align 16, !tbaa !49
  %143 = add nuw nsw i64 %7, 53
  %144 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %143
  store float 0.000000e+00, float* %144, align 4, !tbaa !49
  %145 = add nuw nsw i64 %7, 54
  %146 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %145
  store float 0.000000e+00, float* %146, align 8, !tbaa !49
  %147 = or i64 %145, 1
  %148 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %147
  store float 0.000000e+00, float* %148, align 4, !tbaa !49
  %149 = add nuw nsw i64 %7, 56
  %150 = add nsw i64 %16, 590
  %151 = getelementptr inbounds float, float* %6, i64 %150
  %152 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %149
  %153 = bitcast float* %151 to <4 x i32>*
  %154 = load <4 x i32>, <4 x i32>* %153, align 4, !tbaa !52
  %155 = bitcast float* %152 to <4 x i32>*
  store <4 x i32> %154, <4 x i32>* %155, align 16, !tbaa !49
  %156 = add nuw nsw i64 %7, 60
  %157 = add nsw i64 %16, 594
  %158 = getelementptr inbounds float, float* %6, i64 %157
  %159 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %156
  %160 = bitcast float* %158 to <4 x i32>*
  %161 = load <4 x i32>, <4 x i32>* %160, align 4, !tbaa !52
  %162 = bitcast float* %159 to <4 x i32>*
  store <4 x i32> %161, <4 x i32>* %162, align 16, !tbaa !49
  %163 = add nuw nsw i64 %7, 64
  %164 = add nsw i64 %16, 598
  %165 = getelementptr inbounds float, float* %6, i64 %164
  %166 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %163
  %167 = bitcast float* %165 to <4 x i32>*
  %168 = load <4 x i32>, <4 x i32>* %167, align 4, !tbaa !52
  %169 = bitcast float* %166 to <4 x i32>*
  store <4 x i32> %168, <4 x i32>* %169, align 16, !tbaa !49
  %170 = add nuw nsw i64 %7, 68
  %171 = add nsw i64 %16, 602
  %172 = getelementptr inbounds float, float* %6, i64 %171
  %173 = bitcast float* %172 to i32*
  %174 = load i32, i32* %173, align 4, !tbaa !52
  %175 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %170
  %176 = bitcast float* %175 to i32*
  store i32 %174, i32* %176, align 16, !tbaa !49
  %177 = add nuw nsw i64 %7, 69
  %178 = add nsw i64 %16, 603
  %179 = getelementptr inbounds float, float* %6, i64 %178
  %180 = bitcast float* %179 to i32*
  %181 = load i32, i32* %180, align 4, !tbaa !52
  %182 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %177
  %183 = bitcast float* %182 to i32*
  store i32 %181, i32* %183, align 4, !tbaa !49
  %184 = add nuw nsw i64 %7, 70
  %185 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %184
  store float 0.000000e+00, float* %185, align 8, !tbaa !49
  %186 = add nuw nsw i64 %7, 71
  %187 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %186
  store float 0.000000e+00, float* %187, align 4, !tbaa !49
  %188 = add nuw nsw i64 %7, 72
  %189 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %188
  store float 0.000000e+00, float* %189, align 16, !tbaa !49
  %190 = or i64 %188, 1
  %191 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %190
  store float 0.000000e+00, float* %191, align 4, !tbaa !49
  %192 = add nuw nsw i64 %7, 74
  %193 = add nsw i64 %16, 786
  %194 = getelementptr inbounds float, float* %6, i64 %193
  %195 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %192
  %196 = bitcast float* %194 to <4 x i32>*
  %197 = load <4 x i32>, <4 x i32>* %196, align 4, !tbaa !52
  %198 = bitcast float* %195 to <4 x i32>*
  store <4 x i32> %197, <4 x i32>* %198, align 8, !tbaa !49
  %199 = add nuw nsw i64 %7, 78
  %200 = add nsw i64 %16, 790
  %201 = getelementptr inbounds float, float* %6, i64 %200
  %202 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %199
  %203 = bitcast float* %201 to <4 x i32>*
  %204 = load <4 x i32>, <4 x i32>* %203, align 4, !tbaa !52
  %205 = bitcast float* %202 to <4 x i32>*
  store <4 x i32> %204, <4 x i32>* %205, align 8, !tbaa !49
  %206 = add nuw nsw i64 %7, 82
  %207 = add nsw i64 %16, 794
  %208 = getelementptr inbounds float, float* %6, i64 %207
  %209 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %206
  %210 = bitcast float* %208 to <4 x i32>*
  %211 = load <4 x i32>, <4 x i32>* %210, align 4, !tbaa !52
  %212 = bitcast float* %209 to <4 x i32>*
  store <4 x i32> %211, <4 x i32>* %212, align 8, !tbaa !49
  %213 = add nuw nsw i64 %7, 86
  %214 = add nsw i64 %16, 798
  %215 = getelementptr inbounds float, float* %6, i64 %214
  %216 = bitcast float* %215 to i32*
  %217 = load i32, i32* %216, align 4, !tbaa !52
  %218 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %213
  %219 = bitcast float* %218 to i32*
  store i32 %217, i32* %219, align 8, !tbaa !49
  %220 = add nuw nsw i64 %7, 87
  %221 = add nsw i64 %16, 799
  %222 = getelementptr inbounds float, float* %6, i64 %221
  %223 = bitcast float* %222 to i32*
  %224 = load i32, i32* %223, align 4, !tbaa !52
  %225 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %220
  %226 = bitcast float* %225 to i32*
  store i32 %224, i32* %226, align 4, !tbaa !49
  %227 = add nuw nsw i64 %7, 88
  %228 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %227
  store float 0.000000e+00, float* %228, align 16, !tbaa !49
  %229 = add nuw nsw i64 %7, 89
  %230 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %229
  store float 0.000000e+00, float* %230, align 4, !tbaa !49
  %231 = add nuw nsw i64 %7, 90
  %232 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %231
  store float 0.000000e+00, float* %232, align 8, !tbaa !49
  %233 = or i64 %231, 1
  %234 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %233
  store float 0.000000e+00, float* %234, align 4, !tbaa !49
  %235 = add nuw nsw i64 %7, 92
  %236 = add nsw i64 %16, 982
  %237 = getelementptr inbounds float, float* %6, i64 %236
  %238 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %235
  %239 = bitcast float* %237 to <4 x i32>*
  %240 = load <4 x i32>, <4 x i32>* %239, align 4, !tbaa !52
  %241 = bitcast float* %238 to <4 x i32>*
  store <4 x i32> %240, <4 x i32>* %241, align 16, !tbaa !49
  %242 = add nuw nsw i64 %7, 96
  %243 = add nsw i64 %16, 986
  %244 = getelementptr inbounds float, float* %6, i64 %243
  %245 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %242
  %246 = bitcast float* %244 to <4 x i32>*
  %247 = load <4 x i32>, <4 x i32>* %246, align 4, !tbaa !52
  %248 = bitcast float* %245 to <4 x i32>*
  store <4 x i32> %247, <4 x i32>* %248, align 16, !tbaa !49
  %249 = add nuw nsw i64 %7, 100
  %250 = add nsw i64 %16, 990
  %251 = getelementptr inbounds float, float* %6, i64 %250
  %252 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %249
  %253 = bitcast float* %251 to <4 x i32>*
  %254 = load <4 x i32>, <4 x i32>* %253, align 4, !tbaa !52
  %255 = bitcast float* %252 to <4 x i32>*
  store <4 x i32> %254, <4 x i32>* %255, align 16, !tbaa !49
  %256 = add nuw nsw i64 %7, 104
  %257 = add nsw i64 %16, 994
  %258 = getelementptr inbounds float, float* %6, i64 %257
  %259 = bitcast float* %258 to i32*
  %260 = load i32, i32* %259, align 4, !tbaa !52
  %261 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %256
  %262 = bitcast float* %261 to i32*
  store i32 %260, i32* %262, align 16, !tbaa !49
  %263 = add nuw nsw i64 %7, 105
  %264 = add nsw i64 %16, 995
  %265 = getelementptr inbounds float, float* %6, i64 %264
  %266 = bitcast float* %265 to i32*
  %267 = load i32, i32* %266, align 4, !tbaa !52
  %268 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %263
  %269 = bitcast float* %268 to i32*
  store i32 %267, i32* %269, align 4, !tbaa !49
  %270 = add nuw nsw i64 %7, 106
  %271 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %270
  store float 0.000000e+00, float* %271, align 8, !tbaa !49
  %272 = add nuw nsw i64 %7, 107
  %273 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %272
  store float 0.000000e+00, float* %273, align 4, !tbaa !49
  %274 = add nuw nsw i64 %7, 108
  %275 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %274
  store float 0.000000e+00, float* %275, align 16, !tbaa !49
  %276 = or i64 %274, 1
  %277 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %276
  store float 0.000000e+00, float* %277, align 4, !tbaa !49
  %278 = add nuw nsw i64 %7, 110
  %279 = add nsw i64 %16, 1178
  %280 = getelementptr inbounds float, float* %6, i64 %279
  %281 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %278
  %282 = bitcast float* %280 to <4 x i32>*
  %283 = load <4 x i32>, <4 x i32>* %282, align 4, !tbaa !52
  %284 = bitcast float* %281 to <4 x i32>*
  store <4 x i32> %283, <4 x i32>* %284, align 8, !tbaa !49
  %285 = add nuw nsw i64 %7, 114
  %286 = add nsw i64 %16, 1182
  %287 = getelementptr inbounds float, float* %6, i64 %286
  %288 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %285
  %289 = bitcast float* %287 to <4 x i32>*
  %290 = load <4 x i32>, <4 x i32>* %289, align 4, !tbaa !52
  %291 = bitcast float* %288 to <4 x i32>*
  store <4 x i32> %290, <4 x i32>* %291, align 8, !tbaa !49
  %292 = add nuw nsw i64 %7, 118
  %293 = add nsw i64 %16, 1186
  %294 = getelementptr inbounds float, float* %6, i64 %293
  %295 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %292
  %296 = bitcast float* %294 to <4 x i32>*
  %297 = load <4 x i32>, <4 x i32>* %296, align 4, !tbaa !52
  %298 = bitcast float* %295 to <4 x i32>*
  store <4 x i32> %297, <4 x i32>* %298, align 8, !tbaa !49
  %299 = add nuw nsw i64 %7, 122
  %300 = add nsw i64 %16, 1190
  %301 = getelementptr inbounds float, float* %6, i64 %300
  %302 = bitcast float* %301 to i32*
  %303 = load i32, i32* %302, align 4, !tbaa !52
  %304 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %299
  %305 = bitcast float* %304 to i32*
  store i32 %303, i32* %305, align 8, !tbaa !49
  %306 = add nuw nsw i64 %7, 123
  %307 = add nsw i64 %16, 1191
  %308 = getelementptr inbounds float, float* %6, i64 %307
  %309 = bitcast float* %308 to i32*
  %310 = load i32, i32* %309, align 4, !tbaa !52
  %311 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %306
  %312 = bitcast float* %311 to i32*
  store i32 %310, i32* %312, align 4, !tbaa !49
  %313 = add nuw nsw i64 %7, 124
  %314 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %313
  store float 0.000000e+00, float* %314, align 16, !tbaa !49
  %315 = add nuw nsw i64 %7, 125
  %316 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %315
  store float 0.000000e+00, float* %316, align 4, !tbaa !49
  %317 = add nuw nsw i64 %7, 126
  %318 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %317
  store float 0.000000e+00, float* %318, align 8, !tbaa !49
  %319 = or i64 %317, 1
  %320 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %319
  store float 0.000000e+00, float* %320, align 4, !tbaa !49
  %321 = add nuw nsw i64 %7, 128
  %322 = add nsw i64 %16, 1374
  %323 = getelementptr inbounds float, float* %6, i64 %322
  %324 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %321
  %325 = bitcast float* %323 to <4 x i32>*
  %326 = load <4 x i32>, <4 x i32>* %325, align 4, !tbaa !52
  %327 = bitcast float* %324 to <4 x i32>*
  store <4 x i32> %326, <4 x i32>* %327, align 16, !tbaa !49
  %328 = add nuw nsw i64 %7, 132
  %329 = add nsw i64 %16, 1378
  %330 = getelementptr inbounds float, float* %6, i64 %329
  %331 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %328
  %332 = bitcast float* %330 to <4 x i32>*
  %333 = load <4 x i32>, <4 x i32>* %332, align 4, !tbaa !52
  %334 = bitcast float* %331 to <4 x i32>*
  store <4 x i32> %333, <4 x i32>* %334, align 16, !tbaa !49
  %335 = add nuw nsw i64 %7, 136
  %336 = add nsw i64 %16, 1382
  %337 = getelementptr inbounds float, float* %6, i64 %336
  %338 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %335
  %339 = bitcast float* %337 to <4 x i32>*
  %340 = load <4 x i32>, <4 x i32>* %339, align 4, !tbaa !52
  %341 = bitcast float* %338 to <4 x i32>*
  store <4 x i32> %340, <4 x i32>* %341, align 16, !tbaa !49
  %342 = add nuw nsw i64 %7, 140
  %343 = add nsw i64 %16, 1386
  %344 = getelementptr inbounds float, float* %6, i64 %343
  %345 = bitcast float* %344 to i32*
  %346 = load i32, i32* %345, align 4, !tbaa !52
  %347 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %342
  %348 = bitcast float* %347 to i32*
  store i32 %346, i32* %348, align 16, !tbaa !49
  %349 = add nuw nsw i64 %7, 141
  %350 = add nsw i64 %16, 1387
  %351 = getelementptr inbounds float, float* %6, i64 %350
  %352 = bitcast float* %351 to i32*
  %353 = load i32, i32* %352, align 4, !tbaa !52
  %354 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %349
  %355 = bitcast float* %354 to i32*
  store i32 %353, i32* %355, align 4, !tbaa !49
  %356 = add nuw nsw i64 %7, 142
  %357 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %356
  store float 0.000000e+00, float* %357, align 8, !tbaa !49
  %358 = add nuw nsw i64 %7, 143
  %359 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %358
  store float 0.000000e+00, float* %359, align 4, !tbaa !49
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %.sub = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0
  %360 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_begin4.preheader.us.preheader, %for_begin4.preheader.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond168 = icmp eq i64 %indvar.next, 72
  br i1 %exitcond168, label %for_begin7.preheader, label %for_begin1.preheader, !prof !24

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %361 = phi i32 [ 0, %for_begin7.preheader ], [ %403, %for_end12 ]
  %362 = urem i32 %361, 5
  %363 = mul nuw nsw i32 %362, 320
  %364 = udiv i32 %361, 5
  %365 = mul nsw i32 %364, 6400
  %366 = add nuw i32 %363, %365
  %367 = mul nuw nsw i32 %362, 5
  %368 = or i32 %367, %365
  %369 = zext i32 %368 to i64
  %370 = zext i32 %366 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %371 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %372 = bitcast float* %371 to <8 x float>*
  %373 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %374 = bitcast float* %373 to <8 x float>*
  %375 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %376 = bitcast float* %375 to <8 x float>*
  %377 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %378 = bitcast float* %377 to <8 x float>*
  %379 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %380 = bitcast float* %379 to <8 x float>*
  %381 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %382 = bitcast float* %381 to <8 x float>*
  %383 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %384 = bitcast float* %383 to <8 x float>*
  %385 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %386 = bitcast float* %385 to <8 x float>*
  %387 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %388 = bitcast float* %387 to <8 x float>*
  %389 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %390 = bitcast float* %389 to <8 x float>*
  %391 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %392 = bitcast float* %391 to <8 x float>*
  %393 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 96
  %394 = bitcast float* %393 to <8 x float>*
  %395 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 104
  %396 = bitcast float* %395 to <8 x float>*
  %397 = bitcast i8* %2 to float*
  %398 = bitcast [14 x <8 x float>]* %3 to i8*
  br label %for_body20

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv148 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next149, %for_end15 ]
  %399 = mul nuw nsw i64 %indvars.iv148, 1600
  %400 = add nuw nsw i64 %399, %370
  %401 = mul nuw nsw i64 %indvars.iv148, 200
  %402 = add nuw nsw i64 %401, %369
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %403 = add nuw nsw i32 %361, 1
  %exitcond151 = icmp eq i32 %403, 40
  br i1 %exitcond151, label %for_begin19.preheader, label %for_begin10.preheader, !prof !24

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv145 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next146, %for_begin16.preheader ]
  %404 = shl i64 %indvars.iv145, 6
  %405 = add nuw nsw i64 %400, %404
  %406 = add nuw nsw i64 %402, %indvars.iv145
  %407 = getelementptr inbounds float, float* %360, i64 %406
  %408 = load float, float* %407, align 4, !tbaa !55
  %409 = insertelement <8 x float> undef, float %408, i32 0
  %410 = shl i64 %406, 32
  %sext = add i64 %410, 3435973836800
  %411 = ashr exact i64 %sext, 32
  %412 = getelementptr inbounds float, float* %360, i64 %411
  %413 = load float, float* %412, align 4, !tbaa !55
  %414 = insertelement <8 x float> %409, float %413, i32 1
  %415 = shl i64 %406, 32
  %sext169 = add i64 %415, 6871947673600
  %416 = ashr exact i64 %sext169, 32
  %417 = getelementptr inbounds float, float* %360, i64 %416
  %418 = load float, float* %417, align 4, !tbaa !55
  %419 = insertelement <8 x float> %414, float %418, i32 2
  %420 = shl i64 %406, 32
  %sext170 = add i64 %420, 10307921510400
  %421 = ashr exact i64 %sext170, 32
  %422 = getelementptr inbounds float, float* %360, i64 %421
  %423 = load float, float* %422, align 4, !tbaa !55
  %424 = insertelement <8 x float> %419, float %423, i32 3
  %425 = shl i64 %406, 32
  %sext171 = add i64 %425, 13743895347200
  %426 = ashr exact i64 %sext171, 32
  %427 = getelementptr inbounds float, float* %360, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !55
  %429 = insertelement <8 x float> %424, float %428, i32 4
  %430 = shl i64 %406, 32
  %sext172 = add i64 %430, 17179869184000
  %431 = ashr exact i64 %sext172, 32
  %432 = getelementptr inbounds float, float* %360, i64 %431
  %433 = load float, float* %432, align 4, !tbaa !55
  %434 = insertelement <8 x float> %429, float %433, i32 5
  %435 = shl i64 %406, 32
  %sext173 = add i64 %435, 20615843020800
  %436 = ashr exact i64 %sext173, 32
  %437 = getelementptr inbounds float, float* %360, i64 %436
  %438 = load float, float* %437, align 4, !tbaa !55
  %439 = insertelement <8 x float> %434, float %438, i32 6
  %440 = shl i64 %406, 32
  %sext174 = add i64 %440, 24051816857600
  %441 = ashr exact i64 %sext174, 32
  %442 = getelementptr inbounds float, float* %360, i64 %441
  %443 = load float, float* %442, align 4, !tbaa !55
  %444 = insertelement <8 x float> %439, float %443, i32 7
  %445 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %405
  %446 = bitcast float* %445 to <8 x float>*
  store <8 x float> %444, <8 x float>* %446, align 16, !tbaa !58
  %447 = or i64 %405, 8
  %448 = add nuw nsw i64 %406, 25
  %449 = getelementptr inbounds float, float* %360, i64 %448
  %450 = load float, float* %449, align 4, !tbaa !55
  %451 = insertelement <8 x float> undef, float %450, i32 0
  %452 = shl i64 %406, 32
  %sext175 = add i64 %452, 3543348019200
  %453 = ashr exact i64 %sext175, 32
  %454 = getelementptr inbounds float, float* %360, i64 %453
  %455 = load float, float* %454, align 4, !tbaa !55
  %456 = insertelement <8 x float> %451, float %455, i32 1
  %457 = shl i64 %406, 32
  %sext176 = add i64 %457, 6979321856000
  %458 = ashr exact i64 %sext176, 32
  %459 = getelementptr inbounds float, float* %360, i64 %458
  %460 = load float, float* %459, align 4, !tbaa !55
  %461 = insertelement <8 x float> %456, float %460, i32 2
  %462 = shl i64 %406, 32
  %sext177 = add i64 %462, 10415295692800
  %463 = ashr exact i64 %sext177, 32
  %464 = getelementptr inbounds float, float* %360, i64 %463
  %465 = load float, float* %464, align 4, !tbaa !55
  %466 = insertelement <8 x float> %461, float %465, i32 3
  %467 = shl i64 %406, 32
  %sext178 = add i64 %467, 13851269529600
  %468 = ashr exact i64 %sext178, 32
  %469 = getelementptr inbounds float, float* %360, i64 %468
  %470 = load float, float* %469, align 4, !tbaa !55
  %471 = insertelement <8 x float> %466, float %470, i32 4
  %472 = shl i64 %406, 32
  %sext179 = add i64 %472, 17287243366400
  %473 = ashr exact i64 %sext179, 32
  %474 = getelementptr inbounds float, float* %360, i64 %473
  %475 = load float, float* %474, align 4, !tbaa !55
  %476 = insertelement <8 x float> %471, float %475, i32 5
  %477 = shl i64 %406, 32
  %sext180 = add i64 %477, 20723217203200
  %478 = ashr exact i64 %sext180, 32
  %479 = getelementptr inbounds float, float* %360, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !55
  %481 = insertelement <8 x float> %476, float %480, i32 6
  %482 = shl i64 %406, 32
  %sext181 = add i64 %482, 24159191040000
  %483 = ashr exact i64 %sext181, 32
  %484 = getelementptr inbounds float, float* %360, i64 %483
  %485 = load float, float* %484, align 4, !tbaa !55
  %486 = insertelement <8 x float> %481, float %485, i32 7
  %487 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %447
  %488 = bitcast float* %487 to <8 x float>*
  store <8 x float> %486, <8 x float>* %488, align 16, !tbaa !58
  %489 = or i64 %405, 16
  %490 = add nuw nsw i64 %406, 50
  %491 = getelementptr inbounds float, float* %360, i64 %490
  %492 = load float, float* %491, align 4, !tbaa !55
  %493 = insertelement <8 x float> undef, float %492, i32 0
  %494 = shl i64 %406, 32
  %sext182 = add i64 %494, 3650722201600
  %495 = ashr exact i64 %sext182, 32
  %496 = getelementptr inbounds float, float* %360, i64 %495
  %497 = load float, float* %496, align 4, !tbaa !55
  %498 = insertelement <8 x float> %493, float %497, i32 1
  %499 = shl i64 %406, 32
  %sext183 = add i64 %499, 7086696038400
  %500 = ashr exact i64 %sext183, 32
  %501 = getelementptr inbounds float, float* %360, i64 %500
  %502 = load float, float* %501, align 4, !tbaa !55
  %503 = insertelement <8 x float> %498, float %502, i32 2
  %504 = shl i64 %406, 32
  %sext184 = add i64 %504, 10522669875200
  %505 = ashr exact i64 %sext184, 32
  %506 = getelementptr inbounds float, float* %360, i64 %505
  %507 = load float, float* %506, align 4, !tbaa !55
  %508 = insertelement <8 x float> %503, float %507, i32 3
  %509 = shl i64 %406, 32
  %sext185 = add i64 %509, 13958643712000
  %510 = ashr exact i64 %sext185, 32
  %511 = getelementptr inbounds float, float* %360, i64 %510
  %512 = load float, float* %511, align 4, !tbaa !55
  %513 = insertelement <8 x float> %508, float %512, i32 4
  %514 = shl i64 %406, 32
  %sext186 = add i64 %514, 17394617548800
  %515 = ashr exact i64 %sext186, 32
  %516 = getelementptr inbounds float, float* %360, i64 %515
  %517 = load float, float* %516, align 4, !tbaa !55
  %518 = insertelement <8 x float> %513, float %517, i32 5
  %519 = shl i64 %406, 32
  %sext187 = add i64 %519, 20830591385600
  %520 = ashr exact i64 %sext187, 32
  %521 = getelementptr inbounds float, float* %360, i64 %520
  %522 = load float, float* %521, align 4, !tbaa !55
  %523 = insertelement <8 x float> %518, float %522, i32 6
  %524 = shl i64 %406, 32
  %sext188 = add i64 %524, 24266565222400
  %525 = ashr exact i64 %sext188, 32
  %526 = getelementptr inbounds float, float* %360, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !55
  %528 = insertelement <8 x float> %523, float %527, i32 7
  %529 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %489
  %530 = bitcast float* %529 to <8 x float>*
  store <8 x float> %528, <8 x float>* %530, align 16, !tbaa !58
  %531 = or i64 %405, 24
  %532 = add nuw nsw i64 %406, 75
  %533 = getelementptr inbounds float, float* %360, i64 %532
  %534 = load float, float* %533, align 4, !tbaa !55
  %535 = insertelement <8 x float> undef, float %534, i32 0
  %536 = shl i64 %406, 32
  %sext189 = add i64 %536, 3758096384000
  %537 = ashr exact i64 %sext189, 32
  %538 = getelementptr inbounds float, float* %360, i64 %537
  %539 = load float, float* %538, align 4, !tbaa !55
  %540 = insertelement <8 x float> %535, float %539, i32 1
  %541 = shl i64 %406, 32
  %sext190 = add i64 %541, 7194070220800
  %542 = ashr exact i64 %sext190, 32
  %543 = getelementptr inbounds float, float* %360, i64 %542
  %544 = load float, float* %543, align 4, !tbaa !55
  %545 = insertelement <8 x float> %540, float %544, i32 2
  %546 = shl i64 %406, 32
  %sext191 = add i64 %546, 10630044057600
  %547 = ashr exact i64 %sext191, 32
  %548 = getelementptr inbounds float, float* %360, i64 %547
  %549 = load float, float* %548, align 4, !tbaa !55
  %550 = insertelement <8 x float> %545, float %549, i32 3
  %551 = shl i64 %406, 32
  %sext192 = add i64 %551, 14066017894400
  %552 = ashr exact i64 %sext192, 32
  %553 = getelementptr inbounds float, float* %360, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !55
  %555 = insertelement <8 x float> %550, float %554, i32 4
  %556 = shl i64 %406, 32
  %sext193 = add i64 %556, 17501991731200
  %557 = ashr exact i64 %sext193, 32
  %558 = getelementptr inbounds float, float* %360, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !55
  %560 = insertelement <8 x float> %555, float %559, i32 5
  %561 = shl i64 %406, 32
  %sext194 = add i64 %561, 20937965568000
  %562 = ashr exact i64 %sext194, 32
  %563 = getelementptr inbounds float, float* %360, i64 %562
  %564 = load float, float* %563, align 4, !tbaa !55
  %565 = insertelement <8 x float> %560, float %564, i32 6
  %566 = shl i64 %406, 32
  %sext195 = add i64 %566, 24373939404800
  %567 = ashr exact i64 %sext195, 32
  %568 = getelementptr inbounds float, float* %360, i64 %567
  %569 = load float, float* %568, align 4, !tbaa !55
  %570 = insertelement <8 x float> %565, float %569, i32 7
  %571 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %531
  %572 = bitcast float* %571 to <8 x float>*
  store <8 x float> %570, <8 x float>* %572, align 16, !tbaa !58
  %573 = or i64 %405, 32
  %574 = add nuw nsw i64 %406, 100
  %575 = getelementptr inbounds float, float* %360, i64 %574
  %576 = load float, float* %575, align 4, !tbaa !55
  %577 = insertelement <8 x float> undef, float %576, i32 0
  %578 = shl i64 %406, 32
  %sext196 = add i64 %578, 3865470566400
  %579 = ashr exact i64 %sext196, 32
  %580 = getelementptr inbounds float, float* %360, i64 %579
  %581 = load float, float* %580, align 4, !tbaa !55
  %582 = insertelement <8 x float> %577, float %581, i32 1
  %583 = shl i64 %406, 32
  %sext197 = add i64 %583, 7301444403200
  %584 = ashr exact i64 %sext197, 32
  %585 = getelementptr inbounds float, float* %360, i64 %584
  %586 = load float, float* %585, align 4, !tbaa !55
  %587 = insertelement <8 x float> %582, float %586, i32 2
  %588 = shl i64 %406, 32
  %sext198 = add i64 %588, 10737418240000
  %589 = ashr exact i64 %sext198, 32
  %590 = getelementptr inbounds float, float* %360, i64 %589
  %591 = load float, float* %590, align 4, !tbaa !55
  %592 = insertelement <8 x float> %587, float %591, i32 3
  %593 = shl i64 %406, 32
  %sext199 = add i64 %593, 14173392076800
  %594 = ashr exact i64 %sext199, 32
  %595 = getelementptr inbounds float, float* %360, i64 %594
  %596 = load float, float* %595, align 4, !tbaa !55
  %597 = insertelement <8 x float> %592, float %596, i32 4
  %598 = shl i64 %406, 32
  %sext200 = add i64 %598, 17609365913600
  %599 = ashr exact i64 %sext200, 32
  %600 = getelementptr inbounds float, float* %360, i64 %599
  %601 = load float, float* %600, align 4, !tbaa !55
  %602 = insertelement <8 x float> %597, float %601, i32 5
  %603 = shl i64 %406, 32
  %sext201 = add i64 %603, 21045339750400
  %604 = ashr exact i64 %sext201, 32
  %605 = getelementptr inbounds float, float* %360, i64 %604
  %606 = load float, float* %605, align 4, !tbaa !55
  %607 = insertelement <8 x float> %602, float %606, i32 6
  %608 = shl i64 %406, 32
  %sext202 = add i64 %608, 24481313587200
  %609 = ashr exact i64 %sext202, 32
  %610 = getelementptr inbounds float, float* %360, i64 %609
  %611 = load float, float* %610, align 4, !tbaa !55
  %612 = insertelement <8 x float> %607, float %611, i32 7
  %613 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %573
  %614 = bitcast float* %613 to <8 x float>*
  store <8 x float> %612, <8 x float>* %614, align 16, !tbaa !58
  %615 = or i64 %405, 40
  %616 = add nuw nsw i64 %406, 125
  %617 = getelementptr inbounds float, float* %360, i64 %616
  %618 = load float, float* %617, align 4, !tbaa !55
  %619 = insertelement <8 x float> undef, float %618, i32 0
  %620 = shl i64 %406, 32
  %sext203 = add i64 %620, 3972844748800
  %621 = ashr exact i64 %sext203, 32
  %622 = getelementptr inbounds float, float* %360, i64 %621
  %623 = load float, float* %622, align 4, !tbaa !55
  %624 = insertelement <8 x float> %619, float %623, i32 1
  %625 = shl i64 %406, 32
  %sext204 = add i64 %625, 7408818585600
  %626 = ashr exact i64 %sext204, 32
  %627 = getelementptr inbounds float, float* %360, i64 %626
  %628 = load float, float* %627, align 4, !tbaa !55
  %629 = insertelement <8 x float> %624, float %628, i32 2
  %630 = shl i64 %406, 32
  %sext205 = add i64 %630, 10844792422400
  %631 = ashr exact i64 %sext205, 32
  %632 = getelementptr inbounds float, float* %360, i64 %631
  %633 = load float, float* %632, align 4, !tbaa !55
  %634 = insertelement <8 x float> %629, float %633, i32 3
  %635 = shl i64 %406, 32
  %sext206 = add i64 %635, 14280766259200
  %636 = ashr exact i64 %sext206, 32
  %637 = getelementptr inbounds float, float* %360, i64 %636
  %638 = load float, float* %637, align 4, !tbaa !55
  %639 = insertelement <8 x float> %634, float %638, i32 4
  %640 = shl i64 %406, 32
  %sext207 = add i64 %640, 17716740096000
  %641 = ashr exact i64 %sext207, 32
  %642 = getelementptr inbounds float, float* %360, i64 %641
  %643 = load float, float* %642, align 4, !tbaa !55
  %644 = insertelement <8 x float> %639, float %643, i32 5
  %645 = shl i64 %406, 32
  %sext208 = add i64 %645, 21152713932800
  %646 = ashr exact i64 %sext208, 32
  %647 = getelementptr inbounds float, float* %360, i64 %646
  %648 = load float, float* %647, align 4, !tbaa !55
  %649 = insertelement <8 x float> %644, float %648, i32 6
  %650 = shl i64 %406, 32
  %sext209 = add i64 %650, 24588687769600
  %651 = ashr exact i64 %sext209, 32
  %652 = getelementptr inbounds float, float* %360, i64 %651
  %653 = load float, float* %652, align 4, !tbaa !55
  %654 = insertelement <8 x float> %649, float %653, i32 7
  %655 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %615
  %656 = bitcast float* %655 to <8 x float>*
  store <8 x float> %654, <8 x float>* %656, align 16, !tbaa !58
  %657 = or i64 %405, 48
  %658 = add nuw nsw i64 %406, 150
  %659 = getelementptr inbounds float, float* %360, i64 %658
  %660 = load float, float* %659, align 4, !tbaa !55
  %661 = insertelement <8 x float> undef, float %660, i32 0
  %662 = shl i64 %406, 32
  %sext210 = add i64 %662, 4080218931200
  %663 = ashr exact i64 %sext210, 32
  %664 = getelementptr inbounds float, float* %360, i64 %663
  %665 = load float, float* %664, align 4, !tbaa !55
  %666 = insertelement <8 x float> %661, float %665, i32 1
  %667 = shl i64 %406, 32
  %sext211 = add i64 %667, 7516192768000
  %668 = ashr exact i64 %sext211, 32
  %669 = getelementptr inbounds float, float* %360, i64 %668
  %670 = load float, float* %669, align 4, !tbaa !55
  %671 = insertelement <8 x float> %666, float %670, i32 2
  %672 = shl i64 %406, 32
  %sext212 = add i64 %672, 10952166604800
  %673 = ashr exact i64 %sext212, 32
  %674 = getelementptr inbounds float, float* %360, i64 %673
  %675 = load float, float* %674, align 4, !tbaa !55
  %676 = insertelement <8 x float> %671, float %675, i32 3
  %677 = shl i64 %406, 32
  %sext213 = add i64 %677, 14388140441600
  %678 = ashr exact i64 %sext213, 32
  %679 = getelementptr inbounds float, float* %360, i64 %678
  %680 = load float, float* %679, align 4, !tbaa !55
  %681 = insertelement <8 x float> %676, float %680, i32 4
  %682 = shl i64 %406, 32
  %sext214 = add i64 %682, 17824114278400
  %683 = ashr exact i64 %sext214, 32
  %684 = getelementptr inbounds float, float* %360, i64 %683
  %685 = load float, float* %684, align 4, !tbaa !55
  %686 = insertelement <8 x float> %681, float %685, i32 5
  %687 = shl i64 %406, 32
  %sext215 = add i64 %687, 21260088115200
  %688 = ashr exact i64 %sext215, 32
  %689 = getelementptr inbounds float, float* %360, i64 %688
  %690 = load float, float* %689, align 4, !tbaa !55
  %691 = insertelement <8 x float> %686, float %690, i32 6
  %692 = shl i64 %406, 32
  %sext216 = add i64 %692, 24696061952000
  %693 = ashr exact i64 %sext216, 32
  %694 = getelementptr inbounds float, float* %360, i64 %693
  %695 = load float, float* %694, align 4, !tbaa !55
  %696 = insertelement <8 x float> %691, float %695, i32 7
  %697 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %657
  %698 = bitcast float* %697 to <8 x float>*
  store <8 x float> %696, <8 x float>* %698, align 16, !tbaa !58
  %699 = or i64 %405, 56
  %700 = add nuw nsw i64 %406, 175
  %701 = getelementptr inbounds float, float* %360, i64 %700
  %702 = load float, float* %701, align 4, !tbaa !55
  %703 = insertelement <8 x float> undef, float %702, i32 0
  %704 = shl i64 %406, 32
  %sext217 = add i64 %704, 4187593113600
  %705 = ashr exact i64 %sext217, 32
  %706 = getelementptr inbounds float, float* %360, i64 %705
  %707 = load float, float* %706, align 4, !tbaa !55
  %708 = insertelement <8 x float> %703, float %707, i32 1
  %709 = shl i64 %406, 32
  %sext218 = add i64 %709, 7623566950400
  %710 = ashr exact i64 %sext218, 32
  %711 = getelementptr inbounds float, float* %360, i64 %710
  %712 = load float, float* %711, align 4, !tbaa !55
  %713 = insertelement <8 x float> %708, float %712, i32 2
  %714 = shl i64 %406, 32
  %sext219 = add i64 %714, 11059540787200
  %715 = ashr exact i64 %sext219, 32
  %716 = getelementptr inbounds float, float* %360, i64 %715
  %717 = load float, float* %716, align 4, !tbaa !55
  %718 = insertelement <8 x float> %713, float %717, i32 3
  %719 = shl i64 %406, 32
  %sext220 = add i64 %719, 14495514624000
  %720 = ashr exact i64 %sext220, 32
  %721 = getelementptr inbounds float, float* %360, i64 %720
  %722 = load float, float* %721, align 4, !tbaa !55
  %723 = insertelement <8 x float> %718, float %722, i32 4
  %724 = shl i64 %406, 32
  %sext221 = add i64 %724, 17931488460800
  %725 = ashr exact i64 %sext221, 32
  %726 = getelementptr inbounds float, float* %360, i64 %725
  %727 = load float, float* %726, align 4, !tbaa !55
  %728 = insertelement <8 x float> %723, float %727, i32 5
  %729 = shl i64 %406, 32
  %sext222 = add i64 %729, 21367462297600
  %730 = ashr exact i64 %sext222, 32
  %731 = getelementptr inbounds float, float* %360, i64 %730
  %732 = load float, float* %731, align 4, !tbaa !55
  %733 = insertelement <8 x float> %728, float %732, i32 6
  %734 = shl i64 %406, 32
  %sext223 = add i64 %734, 24803436134400
  %735 = ashr exact i64 %sext223, 32
  %736 = getelementptr inbounds float, float* %360, i64 %735
  %737 = load float, float* %736, align 4, !tbaa !55
  %738 = insertelement <8 x float> %733, float %737, i32 7
  %739 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %699
  %740 = bitcast float* %739 to <8 x float>*
  store <8 x float> %738, <8 x float>* %740, align 16, !tbaa !58
  %indvars.iv.next146 = add nuw nsw i64 %indvars.iv145, 1
  %exitcond147 = icmp eq i64 %indvars.iv.next146, 5
  br i1 %exitcond147, label %for_end15, label %for_begin16.preheader, !prof !24

for_end15:                                        ; preds = %for_begin16.preheader
  %indvars.iv.next149 = add nuw nsw i64 %indvars.iv148, 1
  %exitcond150 = icmp eq i64 %indvars.iv.next149, 4
  br i1 %exitcond150, label %for_end12, label %for_begin13.preheader, !prof !24

for_body20:                                       ; preds = %for_end36, %for_begin19.preheader
  %741 = phi i32 [ 0, %for_begin19.preheader ], [ %896, %for_end36 ]
  %742 = urem i32 %741, 14
  %743 = udiv i32 %741, 14
  %744 = mul nsw i32 %743, 6400
  %745 = zext i32 %744 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 32 %398, i8 0, i64 448, i1 false)
  br label %for_begin25.preheader

for_end21:                                        ; preds = %for_end36
  ret void

for_begin34.preheader:                            ; preds = %for_end27
  store <8 x float> %789, <8 x float>* %.sub, align 32, !tbaa !61
  store <8 x float> %795, <8 x float>* %372, align 32, !tbaa !61
  store <8 x float> %801, <8 x float>* %374, align 32, !tbaa !61
  store <8 x float> %807, <8 x float>* %376, align 32, !tbaa !61
  store <8 x float> %813, <8 x float>* %378, align 32, !tbaa !61
  store <8 x float> %819, <8 x float>* %380, align 32, !tbaa !61
  store <8 x float> %825, <8 x float>* %382, align 32, !tbaa !61
  store <8 x float> %831, <8 x float>* %384, align 32, !tbaa !61
  store <8 x float> %837, <8 x float>* %386, align 32, !tbaa !61
  store <8 x float> %843, <8 x float>* %388, align 32, !tbaa !61
  store <8 x float> %849, <8 x float>* %390, align 32, !tbaa !61
  store <8 x float> %855, <8 x float>* %392, align 32, !tbaa !61
  store <8 x float> %861, <8 x float>* %394, align 32, !tbaa !61
  store <8 x float> %867, <8 x float>* %396, align 32, !tbaa !61
  %746 = mul nuw nsw i32 %742, 14
  %747 = mul nsw i32 %743, 1568
  %748 = add nuw nsw i32 %747, %746
  %749 = zext i32 %748 to i64
  br label %for_body35

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv135 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next136, %for_end27 ]
  %.lcssa28.lcssa.lcssa108 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %867, %for_end27 ]
  %.lcssa26.lcssa.lcssa106 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %861, %for_end27 ]
  %.lcssa24.lcssa.lcssa104 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %855, %for_end27 ]
  %.lcssa22.lcssa.lcssa102 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %849, %for_end27 ]
  %.lcssa20.lcssa.lcssa100 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %843, %for_end27 ]
  %.lcssa18.lcssa.lcssa98 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %837, %for_end27 ]
  %.lcssa16.lcssa.lcssa96 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %831, %for_end27 ]
  %.lcssa14.lcssa.lcssa94 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %825, %for_end27 ]
  %.lcssa12.lcssa.lcssa92 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %819, %for_end27 ]
  %.lcssa10.lcssa.lcssa90 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %813, %for_end27 ]
  %.lcssa8.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %807, %for_end27 ]
  %.lcssa6.lcssa.lcssa87 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %801, %for_end27 ]
  %.lcssa4.lcssa.lcssa85 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %795, %for_end27 ]
  %.lcssa.lcssa.lcssa83 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %789, %for_end27 ]
  %750 = mul nuw nsw i64 %indvars.iv135, 1600
  %751 = add nuw nsw i64 %750, %745
  %752 = trunc i64 %indvars.iv135 to i32
  %753 = mul i32 %752, 2592
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_end30, %for_begin25.preheader
  %indvars.iv131 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next132, %for_end30 ]
  %.lcssa28.lcssa81 = phi <8 x float> [ %.lcssa28.lcssa.lcssa108, %for_begin25.preheader ], [ %867, %for_end30 ]
  %.lcssa26.lcssa79 = phi <8 x float> [ %.lcssa26.lcssa.lcssa106, %for_begin25.preheader ], [ %861, %for_end30 ]
  %.lcssa24.lcssa77 = phi <8 x float> [ %.lcssa24.lcssa.lcssa104, %for_begin25.preheader ], [ %855, %for_end30 ]
  %.lcssa22.lcssa75 = phi <8 x float> [ %.lcssa22.lcssa.lcssa102, %for_begin25.preheader ], [ %849, %for_end30 ]
  %.lcssa20.lcssa73 = phi <8 x float> [ %.lcssa20.lcssa.lcssa100, %for_begin25.preheader ], [ %843, %for_end30 ]
  %.lcssa18.lcssa71 = phi <8 x float> [ %.lcssa18.lcssa.lcssa98, %for_begin25.preheader ], [ %837, %for_end30 ]
  %.lcssa16.lcssa69 = phi <8 x float> [ %.lcssa16.lcssa.lcssa96, %for_begin25.preheader ], [ %831, %for_end30 ]
  %.lcssa14.lcssa67 = phi <8 x float> [ %.lcssa14.lcssa.lcssa94, %for_begin25.preheader ], [ %825, %for_end30 ]
  %.lcssa12.lcssa65 = phi <8 x float> [ %.lcssa12.lcssa.lcssa92, %for_begin25.preheader ], [ %819, %for_end30 ]
  %.lcssa10.lcssa63 = phi <8 x float> [ %.lcssa10.lcssa.lcssa90, %for_begin25.preheader ], [ %813, %for_end30 ]
  %.lcssa8.lcssa61 = phi <8 x float> [ %.lcssa8.lcssa.lcssa89, %for_begin25.preheader ], [ %807, %for_end30 ]
  %.lcssa6.lcssa60 = phi <8 x float> [ %.lcssa6.lcssa.lcssa87, %for_begin25.preheader ], [ %801, %for_end30 ]
  %.lcssa4.lcssa58 = phi <8 x float> [ %.lcssa4.lcssa.lcssa85, %for_begin25.preheader ], [ %795, %for_end30 ]
  %.lcssa.lcssa56 = phi <8 x float> [ %.lcssa.lcssa.lcssa83, %for_begin25.preheader ], [ %789, %for_end30 ]
  %754 = phi i32 [ 0, %for_begin25.preheader ], [ %762, %for_end30 ]
  %reass.add = add nuw nsw i32 %754, %742
  %reass.mul = mul i32 %reass.add, 144
  %755 = add nsw i32 %reass.mul, %753
  %756 = mul nuw nsw i64 %indvars.iv131, 320
  %757 = add nuw nsw i64 %751, %756
  %758 = sext i32 %755 to i64
  br label %for_begin31.preheader

for_end27:                                        ; preds = %for_end30
  %indvars.iv.next136 = add nuw nsw i64 %indvars.iv135, 1
  %exitcond137 = icmp eq i64 %indvars.iv.next136, 4
  br i1 %exitcond137, label %for_begin34.preheader, label %for_begin25.preheader, !prof !24

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv128 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next129, %for_end33 ]
  %.lcssa2855 = phi <8 x float> [ %.lcssa28.lcssa81, %for_begin28.preheader ], [ %867, %for_end33 ]
  %.lcssa2653 = phi <8 x float> [ %.lcssa26.lcssa79, %for_begin28.preheader ], [ %861, %for_end33 ]
  %.lcssa2451 = phi <8 x float> [ %.lcssa24.lcssa77, %for_begin28.preheader ], [ %855, %for_end33 ]
  %.lcssa2249 = phi <8 x float> [ %.lcssa22.lcssa75, %for_begin28.preheader ], [ %849, %for_end33 ]
  %.lcssa2047 = phi <8 x float> [ %.lcssa20.lcssa73, %for_begin28.preheader ], [ %843, %for_end33 ]
  %.lcssa1845 = phi <8 x float> [ %.lcssa18.lcssa71, %for_begin28.preheader ], [ %837, %for_end33 ]
  %.lcssa1643 = phi <8 x float> [ %.lcssa16.lcssa69, %for_begin28.preheader ], [ %831, %for_end33 ]
  %.lcssa1441 = phi <8 x float> [ %.lcssa14.lcssa67, %for_begin28.preheader ], [ %825, %for_end33 ]
  %.lcssa1239 = phi <8 x float> [ %.lcssa12.lcssa65, %for_begin28.preheader ], [ %819, %for_end33 ]
  %.lcssa1037 = phi <8 x float> [ %.lcssa10.lcssa63, %for_begin28.preheader ], [ %813, %for_end33 ]
  %.lcssa835 = phi <8 x float> [ %.lcssa8.lcssa61, %for_begin28.preheader ], [ %807, %for_end33 ]
  %.lcssa633 = phi <8 x float> [ %.lcssa6.lcssa60, %for_begin28.preheader ], [ %801, %for_end33 ]
  %.lcssa432 = phi <8 x float> [ %.lcssa4.lcssa58, %for_begin28.preheader ], [ %795, %for_end33 ]
  %.lcssa30 = phi <8 x float> [ %.lcssa.lcssa56, %for_begin28.preheader ], [ %789, %for_end33 ]
  %759 = add nsw i64 %indvars.iv128, %758
  %760 = shl i64 %indvars.iv128, 6
  %761 = add nuw nsw i64 %757, %760
  br label %for_body32

for_end30:                                        ; preds = %for_end33
  %indvars.iv.next132 = add nuw nsw i64 %indvars.iv131, 1
  %762 = add nuw nsw i32 %754, 1
  %exitcond134 = icmp eq i64 %indvars.iv.next132, 5
  br i1 %exitcond134, label %for_end27, label %for_begin28.preheader, !prof !24

for_body32:                                       ; preds = %for_body32, %for_begin31.preheader
  %indvars.iv = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next, %for_body32 ]
  %763 = phi <8 x float> [ %.lcssa2855, %for_begin31.preheader ], [ %867, %for_body32 ]
  %764 = phi <8 x float> [ %.lcssa2653, %for_begin31.preheader ], [ %861, %for_body32 ]
  %765 = phi <8 x float> [ %.lcssa2451, %for_begin31.preheader ], [ %855, %for_body32 ]
  %766 = phi <8 x float> [ %.lcssa2249, %for_begin31.preheader ], [ %849, %for_body32 ]
  %767 = phi <8 x float> [ %.lcssa2047, %for_begin31.preheader ], [ %843, %for_body32 ]
  %768 = phi <8 x float> [ %.lcssa1845, %for_begin31.preheader ], [ %837, %for_body32 ]
  %769 = phi <8 x float> [ %.lcssa1643, %for_begin31.preheader ], [ %831, %for_body32 ]
  %770 = phi <8 x float> [ %.lcssa1441, %for_begin31.preheader ], [ %825, %for_body32 ]
  %771 = phi <8 x float> [ %.lcssa1239, %for_begin31.preheader ], [ %819, %for_body32 ]
  %772 = phi <8 x float> [ %.lcssa1037, %for_begin31.preheader ], [ %813, %for_body32 ]
  %773 = phi <8 x float> [ %.lcssa835, %for_begin31.preheader ], [ %807, %for_body32 ]
  %774 = phi <8 x float> [ %.lcssa633, %for_begin31.preheader ], [ %801, %for_body32 ]
  %775 = phi <8 x float> [ %.lcssa432, %for_begin31.preheader ], [ %795, %for_body32 ]
  %776 = phi <8 x float> [ %.lcssa30, %for_begin31.preheader ], [ %789, %for_body32 ]
  %777 = mul nuw nsw i64 %indvars.iv, 18
  %778 = add nsw i64 %759, %777
  %779 = and i64 %778, 4294967295
  %780 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %779
  %781 = load float, float* %780, align 4, !tbaa !49
  %782 = insertelement <8 x float> undef, float %781, i32 0
  %783 = shufflevector <8 x float> %782, <8 x float> undef, <8 x i32> zeroinitializer
  %784 = shl i64 %indvars.iv, 3
  %785 = add nuw nsw i64 %761, %784
  %786 = getelementptr inbounds [6400 x <8 x float>], [6400 x <8 x float>]* %4, i64 0, i64 0, i64 %785
  %787 = bitcast float* %786 to <8 x float>*
  %788 = load <8 x float>, <8 x float>* %787, align 16, !tbaa !58
  %789 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %783, <8 x float> %788, <8 x float> %776)
  %790 = add nuw nsw i64 %778, 1
  %791 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %790
  %792 = load float, float* %791, align 4, !tbaa !49
  %793 = insertelement <8 x float> undef, float %792, i32 0
  %794 = shufflevector <8 x float> %793, <8 x float> undef, <8 x i32> zeroinitializer
  %795 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %794, <8 x float> %788, <8 x float> %775)
  %796 = add nuw nsw i64 %778, 2
  %797 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %796
  %798 = load float, float* %797, align 4, !tbaa !49
  %799 = insertelement <8 x float> undef, float %798, i32 0
  %800 = shufflevector <8 x float> %799, <8 x float> undef, <8 x i32> zeroinitializer
  %801 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %800, <8 x float> %788, <8 x float> %774)
  %802 = add nuw nsw i64 %778, 3
  %803 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %802
  %804 = load float, float* %803, align 4, !tbaa !49
  %805 = insertelement <8 x float> undef, float %804, i32 0
  %806 = shufflevector <8 x float> %805, <8 x float> undef, <8 x i32> zeroinitializer
  %807 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %806, <8 x float> %788, <8 x float> %773)
  %808 = add nuw nsw i64 %778, 4
  %809 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %808
  %810 = load float, float* %809, align 4, !tbaa !49
  %811 = insertelement <8 x float> undef, float %810, i32 0
  %812 = shufflevector <8 x float> %811, <8 x float> undef, <8 x i32> zeroinitializer
  %813 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %812, <8 x float> %788, <8 x float> %772)
  %814 = add nuw nsw i64 %778, 5
  %815 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %814
  %816 = load float, float* %815, align 4, !tbaa !49
  %817 = insertelement <8 x float> undef, float %816, i32 0
  %818 = shufflevector <8 x float> %817, <8 x float> undef, <8 x i32> zeroinitializer
  %819 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %818, <8 x float> %788, <8 x float> %771)
  %820 = add nuw nsw i64 %778, 6
  %821 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %820
  %822 = load float, float* %821, align 4, !tbaa !49
  %823 = insertelement <8 x float> undef, float %822, i32 0
  %824 = shufflevector <8 x float> %823, <8 x float> undef, <8 x i32> zeroinitializer
  %825 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %824, <8 x float> %788, <8 x float> %770)
  %826 = add nuw nsw i64 %778, 7
  %827 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %826
  %828 = load float, float* %827, align 4, !tbaa !49
  %829 = insertelement <8 x float> undef, float %828, i32 0
  %830 = shufflevector <8 x float> %829, <8 x float> undef, <8 x i32> zeroinitializer
  %831 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %830, <8 x float> %788, <8 x float> %769)
  %832 = add nuw nsw i64 %778, 8
  %833 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %832
  %834 = load float, float* %833, align 4, !tbaa !49
  %835 = insertelement <8 x float> undef, float %834, i32 0
  %836 = shufflevector <8 x float> %835, <8 x float> undef, <8 x i32> zeroinitializer
  %837 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %836, <8 x float> %788, <8 x float> %768)
  %838 = add nuw nsw i64 %778, 9
  %839 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %838
  %840 = load float, float* %839, align 4, !tbaa !49
  %841 = insertelement <8 x float> undef, float %840, i32 0
  %842 = shufflevector <8 x float> %841, <8 x float> undef, <8 x i32> zeroinitializer
  %843 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %842, <8 x float> %788, <8 x float> %767)
  %844 = add nuw nsw i64 %778, 10
  %845 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %844
  %846 = load float, float* %845, align 4, !tbaa !49
  %847 = insertelement <8 x float> undef, float %846, i32 0
  %848 = shufflevector <8 x float> %847, <8 x float> undef, <8 x i32> zeroinitializer
  %849 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %848, <8 x float> %788, <8 x float> %766)
  %850 = add nuw nsw i64 %778, 11
  %851 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %850
  %852 = load float, float* %851, align 4, !tbaa !49
  %853 = insertelement <8 x float> undef, float %852, i32 0
  %854 = shufflevector <8 x float> %853, <8 x float> undef, <8 x i32> zeroinitializer
  %855 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %854, <8 x float> %788, <8 x float> %765)
  %856 = add nuw nsw i64 %778, 12
  %857 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %856
  %858 = load float, float* %857, align 4, !tbaa !49
  %859 = insertelement <8 x float> undef, float %858, i32 0
  %860 = shufflevector <8 x float> %859, <8 x float> undef, <8 x i32> zeroinitializer
  %861 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %860, <8 x float> %788, <8 x float> %764)
  %862 = add nuw nsw i64 %778, 13
  %863 = getelementptr inbounds [10368 x float], [10368 x float]* %5, i64 0, i64 %862
  %864 = load float, float* %863, align 4, !tbaa !49
  %865 = insertelement <8 x float> undef, float %864, i32 0
  %866 = shufflevector <8 x float> %865, <8 x float> undef, <8 x i32> zeroinitializer
  %867 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %866, <8 x float> %788, <8 x float> %763)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !24

for_end33:                                        ; preds = %for_body32
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %exitcond130 = icmp eq i64 %indvars.iv.next129, 5
  br i1 %exitcond130, label %for_end30, label %for_begin31.preheader, !prof !24

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv138 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next139, %for_body35 ]
  %868 = add nuw nsw i64 %indvars.iv138, %749
  %869 = add nuw nsw i64 %868, 196
  %870 = add nuw nsw i64 %868, 392
  %871 = add nuw nsw i64 %868, 588
  %872 = add nuw nsw i64 %868, 784
  %873 = add nuw nsw i64 %868, 980
  %874 = add nuw nsw i64 %868, 1176
  %875 = add nuw nsw i64 %868, 1372
  %876 = shl nsw i64 %indvars.iv138, 3
  %877 = getelementptr inbounds [14 x <8 x float>], [14 x <8 x float>]* %3, i64 0, i64 0, i64 %876
  %878 = bitcast float* %877 to <8 x float>*
  %879 = load <8 x float>, <8 x float>* %878, align 32, !tbaa !72
  %880 = getelementptr inbounds float, float* %397, i64 %868
  %881 = extractelement <8 x float> %879, i64 0
  store float %881, float* %880, align 4, !tbaa !73
  %882 = getelementptr inbounds float, float* %397, i64 %869
  %883 = extractelement <8 x float> %879, i64 1
  store float %883, float* %882, align 4, !tbaa !73
  %884 = getelementptr inbounds float, float* %397, i64 %870
  %885 = extractelement <8 x float> %879, i64 2
  store float %885, float* %884, align 4, !tbaa !73
  %886 = getelementptr inbounds float, float* %397, i64 %871
  %887 = extractelement <8 x float> %879, i64 3
  store float %887, float* %886, align 4, !tbaa !73
  %888 = getelementptr inbounds float, float* %397, i64 %872
  %889 = extractelement <8 x float> %879, i64 4
  store float %889, float* %888, align 4, !tbaa !73
  %890 = getelementptr inbounds float, float* %397, i64 %873
  %891 = extractelement <8 x float> %879, i64 5
  store float %891, float* %890, align 4, !tbaa !73
  %892 = getelementptr inbounds float, float* %397, i64 %874
  %893 = extractelement <8 x float> %879, i64 6
  store float %893, float* %892, align 4, !tbaa !73
  %894 = getelementptr inbounds float, float* %397, i64 %875
  %895 = extractelement <8 x float> %879, i64 7
  store float %895, float* %894, align 4, !tbaa !73
  %indvars.iv.next139 = add nuw nsw i64 %indvars.iv138, 1
  %exitcond140 = icmp eq i64 %indvars.iv.next139, 14
  br i1 %exitcond140, label %for_end36, label %for_body35, !prof !24

for_end36:                                        ; preds = %for_body35
  %896 = add nuw nsw i32 %741, 1
  %exitcond141 = icmp eq i32 %896, 112
  br i1 %exitcond141, label %for_end21, label %for_body20, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_add(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_add_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_add_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to <4 x float>*
  %4 = load <4 x float>, <4 x float>* %3, align 4, !tbaa !76
  %5 = bitcast i8* %1 to <4 x float>*
  %6 = load <4 x float>, <4 x float>* %5, align 4, !tbaa !79
  %7 = fadd <4 x float> %4, %6
  %8 = bitcast i8* %0 to <4 x float>*
  store <4 x float> %7, <4 x float>* %8, align 4, !tbaa !82
  %9 = getelementptr inbounds i8, i8* %2, i64 16
  %10 = getelementptr inbounds i8, i8* %1, i64 16
  %11 = getelementptr inbounds i8, i8* %0, i64 16
  %12 = bitcast i8* %9 to <4 x float>*
  %13 = load <4 x float>, <4 x float>* %12, align 4, !tbaa !76
  %14 = bitcast i8* %10 to <4 x float>*
  %15 = load <4 x float>, <4 x float>* %14, align 4, !tbaa !79
  %16 = fadd <4 x float> %13, %15
  %17 = bitcast i8* %11 to <4 x float>*
  store <4 x float> %16, <4 x float>* %17, align 4, !tbaa !82
  %18 = getelementptr inbounds i8, i8* %2, i64 32
  %19 = bitcast i8* %18 to float*
  %20 = load float, float* %19, align 4, !tbaa !76
  %21 = getelementptr inbounds i8, i8* %1, i64 32
  %22 = bitcast i8* %21 to float*
  %23 = load float, float* %22, align 4, !tbaa !79
  %24 = fadd float %20, %23
  %25 = getelementptr inbounds i8, i8* %0, i64 32
  %26 = bitcast i8* %25 to float*
  store float %24, float* %26, align 4, !tbaa !82
  %27 = getelementptr inbounds i8, i8* %2, i64 36
  %28 = bitcast i8* %27 to float*
  %29 = load float, float* %28, align 4, !tbaa !76
  %30 = getelementptr inbounds i8, i8* %1, i64 36
  %31 = bitcast i8* %30 to float*
  %32 = load float, float* %31, align 4, !tbaa !79
  %33 = fadd float %29, %32
  %34 = getelementptr inbounds i8, i8* %0, i64 36
  %35 = bitcast i8* %34 to float*
  store float %33, float* %35, align 4, !tbaa !82
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_nn_relu_1_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv1, 196
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv, 14
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = getelementptr inbounds float, float* %3, i64 %6
  %9 = bitcast float* %7 to <4 x float>*
  %10 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !85
  %11 = fcmp ogt <4 x float> %10, zeroinitializer
  %12 = select <4 x i1> %11, <4 x float> %10, <4 x float> zeroinitializer
  %13 = bitcast float* %8 to <4 x float>*
  store <4 x float> %12, <4 x float>* %13, align 4, !tbaa !88
  %14 = add nuw nsw i64 %6, 4
  %15 = getelementptr inbounds float, float* %2, i64 %14
  %16 = getelementptr inbounds float, float* %3, i64 %14
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !85
  %19 = fcmp ogt <4 x float> %18, zeroinitializer
  %20 = select <4 x i1> %19, <4 x float> %18, <4 x float> zeroinitializer
  %21 = bitcast float* %16 to <4 x float>*
  store <4 x float> %20, <4 x float>* %21, align 4, !tbaa !88
  %22 = add nuw nsw i64 %6, 8
  %23 = getelementptr inbounds float, float* %2, i64 %22
  %24 = getelementptr inbounds float, float* %3, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !85
  %27 = fcmp ogt <4 x float> %26, zeroinitializer
  %28 = select <4 x i1> %27, <4 x float> %26, <4 x float> zeroinitializer
  %29 = bitcast float* %24 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !88
  %30 = add nuw nsw i64 %6, 12
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = load float, float* %31, align 4, !tbaa !85
  %33 = fcmp ogt float %32, 0.000000e+00
  %34 = select i1 %33, float %32, float 0.000000e+00
  %35 = getelementptr inbounds float, float* %3, i64 %30
  store float %34, float* %35, align 4, !tbaa !88
  %36 = add nuw nsw i64 %6, 13
  %37 = getelementptr inbounds float, float* %2, i64 %36
  %38 = load float, float* %37, align 4, !tbaa !85
  %39 = fcmp ogt float %38, 0.000000e+00
  %40 = select i1 %39, float %38, float 0.000000e+00
  %41 = getelementptr inbounds float, float* %3, i64 %36
  store float %40, float* %41, align 4, !tbaa !88
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 64
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_reshape(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_reshape_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_reshape_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 128, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_add_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_add_2_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_add_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv1, 196
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !91
  %9 = insertelement <4 x float> undef, float %8, i32 0
  %10 = shufflevector <4 x float> %9, <4 x float> undef, <4 x i32> zeroinitializer
  %11 = insertelement <4 x float> undef, float %8, i32 0
  %12 = shufflevector <4 x float> %11, <4 x float> undef, <4 x i32> zeroinitializer
  %13 = insertelement <4 x float> undef, float %8, i32 0
  %14 = shufflevector <4 x float> %13, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %15 = mul nuw nsw i64 %indvars.iv, 14
  %16 = add nuw nsw i64 %15, %6
  %17 = getelementptr inbounds float, float* %4, i64 %16
  %18 = getelementptr inbounds float, float* %5, i64 %16
  %19 = bitcast float* %17 to <4 x float>*
  %20 = load <4 x float>, <4 x float>* %19, align 4, !tbaa !94
  %21 = fadd <4 x float> %10, %20
  %22 = bitcast float* %18 to <4 x float>*
  store <4 x float> %21, <4 x float>* %22, align 4, !tbaa !97
  %23 = add nuw nsw i64 %16, 4
  %24 = getelementptr inbounds float, float* %4, i64 %23
  %25 = getelementptr inbounds float, float* %5, i64 %23
  %26 = bitcast float* %24 to <4 x float>*
  %27 = load <4 x float>, <4 x float>* %26, align 4, !tbaa !94
  %28 = fadd <4 x float> %12, %27
  %29 = bitcast float* %25 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !97
  %30 = add nuw nsw i64 %16, 8
  %31 = getelementptr inbounds float, float* %4, i64 %30
  %32 = getelementptr inbounds float, float* %5, i64 %30
  %33 = bitcast float* %31 to <4 x float>*
  %34 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !94
  %35 = fadd <4 x float> %14, %34
  %36 = bitcast float* %32 to <4 x float>*
  store <4 x float> %35, <4 x float>* %36, align 4, !tbaa !97
  %37 = add nuw nsw i64 %16, 12
  %38 = getelementptr inbounds float, float* %4, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !94
  %40 = fadd float %8, %39
  %41 = getelementptr inbounds float, float* %5, i64 %37
  store float %40, float* %41, align 4, !tbaa !97
  %42 = add nuw nsw i64 %16, 13
  %43 = getelementptr inbounds float, float* %4, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !94
  %45 = fadd float %8, %44
  %46 = getelementptr inbounds float, float* %5, i64 %42
  store float %45, float* %46, align 4, !tbaa !97
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 64
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_max_pool2d_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_nn_max_pool2d_1_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv6 = phi i64 [ 0, %entry ], [ %indvars.iv.next7, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv6, 196
  %5 = mul nuw nsw i64 %indvars.iv6, 784
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 14
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 56
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = load float, float* %11, align 4, !tbaa !100
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !100
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = add nuw nsw i64 %9, 28
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !100
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 29
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !100
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  store float %29, float* %10, align 4, !tbaa !103
  %30 = or i64 %7, 1
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = or i64 %9, 2
  %33 = getelementptr inbounds float, float* %3, i64 %32
  %34 = load float, float* %33, align 4, !tbaa !100
  %35 = fcmp olt float %34, 0xC7EFFFFFE0000000
  %36 = select i1 %35, float 0xC7EFFFFFE0000000, float %34
  %37 = or i64 %9, 3
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !100
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = add nuw nsw i64 %32, 28
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !100
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  %47 = add nuw nsw i64 %32, 29
  %48 = getelementptr inbounds float, float* %3, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !100
  %50 = fcmp ogt float %46, %49
  %51 = select i1 %50, float %46, float %49
  store float %51, float* %31, align 4, !tbaa !103
  %52 = add nuw nsw i64 %7, 2
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = or i64 %9, 4
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !100
  %57 = fcmp olt float %56, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %56
  %59 = or i64 %9, 5
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !100
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %54, 28
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !100
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = add nuw nsw i64 %54, 29
  %70 = getelementptr inbounds float, float* %3, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !100
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  store float %73, float* %53, align 4, !tbaa !103
  %74 = add nuw nsw i64 %7, 3
  %75 = getelementptr inbounds float, float* %2, i64 %74
  %76 = or i64 %9, 6
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !100
  %79 = fcmp olt float %78, 0xC7EFFFFFE0000000
  %80 = select i1 %79, float 0xC7EFFFFFE0000000, float %78
  %81 = or i64 %9, 7
  %82 = getelementptr inbounds float, float* %3, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !100
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = add nuw nsw i64 %76, 28
  %87 = getelementptr inbounds float, float* %3, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !100
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = add nuw nsw i64 %76, 29
  %92 = getelementptr inbounds float, float* %3, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !100
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  store float %95, float* %75, align 4, !tbaa !103
  %96 = add nuw nsw i64 %7, 4
  %97 = getelementptr inbounds float, float* %2, i64 %96
  %98 = add nuw nsw i64 %9, 8
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !100
  %101 = fcmp olt float %100, 0xC7EFFFFFE0000000
  %102 = select i1 %101, float 0xC7EFFFFFE0000000, float %100
  %103 = or i64 %98, 1
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !100
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 36
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !100
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 37
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !100
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  store float %117, float* %97, align 4, !tbaa !103
  %118 = add nuw nsw i64 %7, 5
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = add nuw nsw i64 %9, 10
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !100
  %123 = fcmp olt float %122, 0xC7EFFFFFE0000000
  %124 = select i1 %123, float 0xC7EFFFFFE0000000, float %122
  %125 = or i64 %120, 1
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !100
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 38
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !100
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 39
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !100
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  store float %139, float* %119, align 4, !tbaa !103
  %140 = add nuw nsw i64 %7, 6
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = add nuw nsw i64 %9, 12
  %143 = getelementptr inbounds float, float* %3, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !100
  %145 = fcmp olt float %144, 0xC7EFFFFFE0000000
  %146 = select i1 %145, float 0xC7EFFFFFE0000000, float %144
  %147 = or i64 %142, 1
  %148 = getelementptr inbounds float, float* %3, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !100
  %150 = fcmp ogt float %146, %149
  %151 = select i1 %150, float %146, float %149
  %152 = add nuw nsw i64 %9, 40
  %153 = getelementptr inbounds float, float* %3, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !100
  %155 = fcmp ogt float %151, %154
  %156 = select i1 %155, float %151, float %154
  %157 = add nuw nsw i64 %9, 41
  %158 = getelementptr inbounds float, float* %3, i64 %157
  %159 = load float, float* %158, align 4, !tbaa !100
  %160 = fcmp ogt float %156, %159
  %161 = select i1 %160, float %156, float %159
  store float %161, float* %141, align 4, !tbaa !103
  %162 = add nuw nsw i64 %7, 7
  %163 = getelementptr inbounds float, float* %2, i64 %162
  %164 = add nuw nsw i64 %9, 14
  %165 = getelementptr inbounds float, float* %3, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !100
  %167 = fcmp olt float %166, 0xC7EFFFFFE0000000
  %168 = select i1 %167, float 0xC7EFFFFFE0000000, float %166
  %169 = or i64 %164, 1
  %170 = getelementptr inbounds float, float* %3, i64 %169
  %171 = load float, float* %170, align 4, !tbaa !100
  %172 = fcmp ogt float %168, %171
  %173 = select i1 %172, float %168, float %171
  %174 = add nuw nsw i64 %9, 42
  %175 = getelementptr inbounds float, float* %3, i64 %174
  %176 = load float, float* %175, align 4, !tbaa !100
  %177 = fcmp ogt float %173, %176
  %178 = select i1 %177, float %173, float %176
  %179 = add nuw nsw i64 %9, 43
  %180 = getelementptr inbounds float, float* %3, i64 %179
  %181 = load float, float* %180, align 4, !tbaa !100
  %182 = fcmp ogt float %178, %181
  %183 = select i1 %182, float %178, float %181
  store float %183, float* %163, align 4, !tbaa !103
  %184 = add nuw nsw i64 %7, 8
  %185 = getelementptr inbounds float, float* %2, i64 %184
  %186 = add nuw nsw i64 %9, 16
  %187 = getelementptr inbounds float, float* %3, i64 %186
  %188 = load float, float* %187, align 4, !tbaa !100
  %189 = fcmp olt float %188, 0xC7EFFFFFE0000000
  %190 = select i1 %189, float 0xC7EFFFFFE0000000, float %188
  %191 = or i64 %186, 1
  %192 = getelementptr inbounds float, float* %3, i64 %191
  %193 = load float, float* %192, align 4, !tbaa !100
  %194 = fcmp ogt float %190, %193
  %195 = select i1 %194, float %190, float %193
  %196 = add nuw nsw i64 %9, 44
  %197 = getelementptr inbounds float, float* %3, i64 %196
  %198 = load float, float* %197, align 4, !tbaa !100
  %199 = fcmp ogt float %195, %198
  %200 = select i1 %199, float %195, float %198
  %201 = add nuw nsw i64 %9, 45
  %202 = getelementptr inbounds float, float* %3, i64 %201
  %203 = load float, float* %202, align 4, !tbaa !100
  %204 = fcmp ogt float %200, %203
  %205 = select i1 %204, float %200, float %203
  store float %205, float* %185, align 4, !tbaa !103
  %206 = add nuw nsw i64 %7, 9
  %207 = getelementptr inbounds float, float* %2, i64 %206
  %208 = add nuw nsw i64 %9, 18
  %209 = getelementptr inbounds float, float* %3, i64 %208
  %210 = load float, float* %209, align 4, !tbaa !100
  %211 = fcmp olt float %210, 0xC7EFFFFFE0000000
  %212 = select i1 %211, float 0xC7EFFFFFE0000000, float %210
  %213 = or i64 %208, 1
  %214 = getelementptr inbounds float, float* %3, i64 %213
  %215 = load float, float* %214, align 4, !tbaa !100
  %216 = fcmp ogt float %212, %215
  %217 = select i1 %216, float %212, float %215
  %218 = add nuw nsw i64 %9, 46
  %219 = getelementptr inbounds float, float* %3, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !100
  %221 = fcmp ogt float %217, %220
  %222 = select i1 %221, float %217, float %220
  %223 = add nuw nsw i64 %9, 47
  %224 = getelementptr inbounds float, float* %3, i64 %223
  %225 = load float, float* %224, align 4, !tbaa !100
  %226 = fcmp ogt float %222, %225
  %227 = select i1 %226, float %222, float %225
  store float %227, float* %207, align 4, !tbaa !103
  %228 = add nuw nsw i64 %7, 10
  %229 = getelementptr inbounds float, float* %2, i64 %228
  %230 = add nuw nsw i64 %9, 20
  %231 = getelementptr inbounds float, float* %3, i64 %230
  %232 = load float, float* %231, align 4, !tbaa !100
  %233 = fcmp olt float %232, 0xC7EFFFFFE0000000
  %234 = select i1 %233, float 0xC7EFFFFFE0000000, float %232
  %235 = or i64 %230, 1
  %236 = getelementptr inbounds float, float* %3, i64 %235
  %237 = load float, float* %236, align 4, !tbaa !100
  %238 = fcmp ogt float %234, %237
  %239 = select i1 %238, float %234, float %237
  %240 = add nuw nsw i64 %9, 48
  %241 = getelementptr inbounds float, float* %3, i64 %240
  %242 = load float, float* %241, align 4, !tbaa !100
  %243 = fcmp ogt float %239, %242
  %244 = select i1 %243, float %239, float %242
  %245 = add nuw nsw i64 %9, 49
  %246 = getelementptr inbounds float, float* %3, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !100
  %248 = fcmp ogt float %244, %247
  %249 = select i1 %248, float %244, float %247
  store float %249, float* %229, align 4, !tbaa !103
  %250 = add nuw nsw i64 %7, 11
  %251 = getelementptr inbounds float, float* %2, i64 %250
  %252 = add nuw nsw i64 %9, 22
  %253 = getelementptr inbounds float, float* %3, i64 %252
  %254 = load float, float* %253, align 4, !tbaa !100
  %255 = fcmp olt float %254, 0xC7EFFFFFE0000000
  %256 = select i1 %255, float 0xC7EFFFFFE0000000, float %254
  %257 = or i64 %252, 1
  %258 = getelementptr inbounds float, float* %3, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !100
  %260 = fcmp ogt float %256, %259
  %261 = select i1 %260, float %256, float %259
  %262 = add nuw nsw i64 %9, 50
  %263 = getelementptr inbounds float, float* %3, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !100
  %265 = fcmp ogt float %261, %264
  %266 = select i1 %265, float %261, float %264
  %267 = add nuw nsw i64 %9, 51
  %268 = getelementptr inbounds float, float* %3, i64 %267
  %269 = load float, float* %268, align 4, !tbaa !100
  %270 = fcmp ogt float %266, %269
  %271 = select i1 %270, float %266, float %269
  store float %271, float* %251, align 4, !tbaa !103
  %272 = add nuw nsw i64 %7, 12
  %273 = getelementptr inbounds float, float* %2, i64 %272
  %274 = add nuw nsw i64 %9, 24
  %275 = getelementptr inbounds float, float* %3, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !100
  %277 = fcmp olt float %276, 0xC7EFFFFFE0000000
  %278 = select i1 %277, float 0xC7EFFFFFE0000000, float %276
  %279 = or i64 %274, 1
  %280 = getelementptr inbounds float, float* %3, i64 %279
  %281 = load float, float* %280, align 4, !tbaa !100
  %282 = fcmp ogt float %278, %281
  %283 = select i1 %282, float %278, float %281
  %284 = add nuw nsw i64 %9, 52
  %285 = getelementptr inbounds float, float* %3, i64 %284
  %286 = load float, float* %285, align 4, !tbaa !100
  %287 = fcmp ogt float %283, %286
  %288 = select i1 %287, float %283, float %286
  %289 = add nuw nsw i64 %9, 53
  %290 = getelementptr inbounds float, float* %3, i64 %289
  %291 = load float, float* %290, align 4, !tbaa !100
  %292 = fcmp ogt float %288, %291
  %293 = select i1 %292, float %288, float %291
  store float %293, float* %273, align 4, !tbaa !103
  %294 = add nuw nsw i64 %7, 13
  %295 = getelementptr inbounds float, float* %2, i64 %294
  %296 = add nuw nsw i64 %9, 26
  %297 = getelementptr inbounds float, float* %3, i64 %296
  %298 = load float, float* %297, align 4, !tbaa !100
  %299 = fcmp olt float %298, 0xC7EFFFFFE0000000
  %300 = select i1 %299, float 0xC7EFFFFFE0000000, float %298
  %301 = or i64 %296, 1
  %302 = getelementptr inbounds float, float* %3, i64 %301
  %303 = load float, float* %302, align 4, !tbaa !100
  %304 = fcmp ogt float %300, %303
  %305 = select i1 %304, float %300, float %303
  %306 = add nuw nsw i64 %9, 54
  %307 = getelementptr inbounds float, float* %3, i64 %306
  %308 = load float, float* %307, align 4, !tbaa !100
  %309 = fcmp ogt float %305, %308
  %310 = select i1 %309, float %305, float %308
  %311 = add nuw nsw i64 %9, 55
  %312 = getelementptr inbounds float, float* %3, i64 %311
  %313 = load float, float* %312, align 4, !tbaa !100
  %314 = fcmp ogt float %310, %313
  %315 = select i1 %314, float %310, float %313
  store float %315, float* %295, align 4, !tbaa !103
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 14
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 32
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_nn_relu_2_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv1, 784
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv, 28
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = getelementptr inbounds float, float* %3, i64 %6
  %9 = bitcast float* %7 to <4 x float>*
  %10 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !106
  %11 = fcmp ogt <4 x float> %10, zeroinitializer
  %12 = select <4 x i1> %11, <4 x float> %10, <4 x float> zeroinitializer
  %13 = bitcast float* %8 to <4 x float>*
  store <4 x float> %12, <4 x float>* %13, align 4, !tbaa !109
  %14 = add nuw nsw i64 %6, 4
  %15 = getelementptr inbounds float, float* %2, i64 %14
  %16 = getelementptr inbounds float, float* %3, i64 %14
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !106
  %19 = fcmp ogt <4 x float> %18, zeroinitializer
  %20 = select <4 x i1> %19, <4 x float> %18, <4 x float> zeroinitializer
  %21 = bitcast float* %16 to <4 x float>*
  store <4 x float> %20, <4 x float>* %21, align 4, !tbaa !109
  %22 = add nuw nsw i64 %6, 8
  %23 = getelementptr inbounds float, float* %2, i64 %22
  %24 = getelementptr inbounds float, float* %3, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !106
  %27 = fcmp ogt <4 x float> %26, zeroinitializer
  %28 = select <4 x i1> %27, <4 x float> %26, <4 x float> zeroinitializer
  %29 = bitcast float* %24 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !109
  %30 = add nuw nsw i64 %6, 12
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = getelementptr inbounds float, float* %3, i64 %30
  %33 = bitcast float* %31 to <4 x float>*
  %34 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !106
  %35 = fcmp ogt <4 x float> %34, zeroinitializer
  %36 = select <4 x i1> %35, <4 x float> %34, <4 x float> zeroinitializer
  %37 = bitcast float* %32 to <4 x float>*
  store <4 x float> %36, <4 x float>* %37, align 4, !tbaa !109
  %38 = add nuw nsw i64 %6, 16
  %39 = getelementptr inbounds float, float* %2, i64 %38
  %40 = getelementptr inbounds float, float* %3, i64 %38
  %41 = bitcast float* %39 to <4 x float>*
  %42 = load <4 x float>, <4 x float>* %41, align 4, !tbaa !106
  %43 = fcmp ogt <4 x float> %42, zeroinitializer
  %44 = select <4 x i1> %43, <4 x float> %42, <4 x float> zeroinitializer
  %45 = bitcast float* %40 to <4 x float>*
  store <4 x float> %44, <4 x float>* %45, align 4, !tbaa !109
  %46 = add nuw nsw i64 %6, 20
  %47 = getelementptr inbounds float, float* %2, i64 %46
  %48 = getelementptr inbounds float, float* %3, i64 %46
  %49 = bitcast float* %47 to <4 x float>*
  %50 = load <4 x float>, <4 x float>* %49, align 4, !tbaa !106
  %51 = fcmp ogt <4 x float> %50, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %50, <4 x float> zeroinitializer
  %53 = bitcast float* %48 to <4 x float>*
  store <4 x float> %52, <4 x float>* %53, align 4, !tbaa !109
  %54 = add nuw nsw i64 %6, 24
  %55 = getelementptr inbounds float, float* %2, i64 %54
  %56 = getelementptr inbounds float, float* %3, i64 %54
  %57 = bitcast float* %55 to <4 x float>*
  %58 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !106
  %59 = fcmp ogt <4 x float> %58, zeroinitializer
  %60 = select <4 x i1> %59, <4 x float> %58, <4 x float> zeroinitializer
  %61 = bitcast float* %56 to <4 x float>*
  store <4 x float> %60, <4 x float>* %61, align 4, !tbaa !109
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 28
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 32
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_reshape_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_reshape_2_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_reshape_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 4096, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_reshape_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_reshape_1_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_reshape_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 256, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_add_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_add_3_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_add_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv1, 784
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !112
  %9 = insertelement <4 x float> undef, float %8, i32 0
  %10 = shufflevector <4 x float> %9, <4 x float> undef, <4 x i32> zeroinitializer
  %11 = insertelement <4 x float> undef, float %8, i32 0
  %12 = shufflevector <4 x float> %11, <4 x float> undef, <4 x i32> zeroinitializer
  %13 = insertelement <4 x float> undef, float %8, i32 0
  %14 = shufflevector <4 x float> %13, <4 x float> undef, <4 x i32> zeroinitializer
  %15 = insertelement <4 x float> undef, float %8, i32 0
  %16 = shufflevector <4 x float> %15, <4 x float> undef, <4 x i32> zeroinitializer
  %17 = insertelement <4 x float> undef, float %8, i32 0
  %18 = shufflevector <4 x float> %17, <4 x float> undef, <4 x i32> zeroinitializer
  %19 = insertelement <4 x float> undef, float %8, i32 0
  %20 = shufflevector <4 x float> %19, <4 x float> undef, <4 x i32> zeroinitializer
  %21 = insertelement <4 x float> undef, float %8, i32 0
  %22 = shufflevector <4 x float> %21, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %23 = mul nuw nsw i64 %indvars.iv, 28
  %24 = add nuw nsw i64 %23, %6
  %25 = getelementptr inbounds float, float* %4, i64 %24
  %26 = getelementptr inbounds float, float* %5, i64 %24
  %27 = bitcast float* %25 to <4 x float>*
  %28 = load <4 x float>, <4 x float>* %27, align 4, !tbaa !115
  %29 = fadd <4 x float> %10, %28
  %30 = bitcast float* %26 to <4 x float>*
  store <4 x float> %29, <4 x float>* %30, align 4, !tbaa !118
  %31 = add nuw nsw i64 %24, 4
  %32 = getelementptr inbounds float, float* %4, i64 %31
  %33 = getelementptr inbounds float, float* %5, i64 %31
  %34 = bitcast float* %32 to <4 x float>*
  %35 = load <4 x float>, <4 x float>* %34, align 4, !tbaa !115
  %36 = fadd <4 x float> %12, %35
  %37 = bitcast float* %33 to <4 x float>*
  store <4 x float> %36, <4 x float>* %37, align 4, !tbaa !118
  %38 = add nuw nsw i64 %24, 8
  %39 = getelementptr inbounds float, float* %4, i64 %38
  %40 = getelementptr inbounds float, float* %5, i64 %38
  %41 = bitcast float* %39 to <4 x float>*
  %42 = load <4 x float>, <4 x float>* %41, align 4, !tbaa !115
  %43 = fadd <4 x float> %14, %42
  %44 = bitcast float* %40 to <4 x float>*
  store <4 x float> %43, <4 x float>* %44, align 4, !tbaa !118
  %45 = add nuw nsw i64 %24, 12
  %46 = getelementptr inbounds float, float* %4, i64 %45
  %47 = getelementptr inbounds float, float* %5, i64 %45
  %48 = bitcast float* %46 to <4 x float>*
  %49 = load <4 x float>, <4 x float>* %48, align 4, !tbaa !115
  %50 = fadd <4 x float> %16, %49
  %51 = bitcast float* %47 to <4 x float>*
  store <4 x float> %50, <4 x float>* %51, align 4, !tbaa !118
  %52 = add nuw nsw i64 %24, 16
  %53 = getelementptr inbounds float, float* %4, i64 %52
  %54 = getelementptr inbounds float, float* %5, i64 %52
  %55 = bitcast float* %53 to <4 x float>*
  %56 = load <4 x float>, <4 x float>* %55, align 4, !tbaa !115
  %57 = fadd <4 x float> %18, %56
  %58 = bitcast float* %54 to <4 x float>*
  store <4 x float> %57, <4 x float>* %58, align 4, !tbaa !118
  %59 = add nuw nsw i64 %24, 20
  %60 = getelementptr inbounds float, float* %4, i64 %59
  %61 = getelementptr inbounds float, float* %5, i64 %59
  %62 = bitcast float* %60 to <4 x float>*
  %63 = load <4 x float>, <4 x float>* %62, align 4, !tbaa !115
  %64 = fadd <4 x float> %20, %63
  %65 = bitcast float* %61 to <4 x float>*
  store <4 x float> %64, <4 x float>* %65, align 4, !tbaa !118
  %66 = add nuw nsw i64 %24, 24
  %67 = getelementptr inbounds float, float* %4, i64 %66
  %68 = getelementptr inbounds float, float* %5, i64 %66
  %69 = bitcast float* %67 to <4 x float>*
  %70 = load <4 x float>, <4 x float>* %69, align 4, !tbaa !115
  %71 = fadd <4 x float> %22, %70
  %72 = bitcast float* %68 to <4 x float>*
  store <4 x float> %71, <4 x float>* %72, align 4, !tbaa !118
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 28
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 32
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_copy(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_copy_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_copy_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 40, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_max_pool2d(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %9 = load i8*, i8** %8, align 8
  %10 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %11 = load i8*, i8** %10, align 8
  tail call fastcc void @fused_nn_max_pool2d_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv4, 49
  %5 = mul nuw nsw i64 %indvars.iv4, 196
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv, 7
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv, 28
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = load float, float* %11, align 4, !tbaa !121
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !121
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = add nuw nsw i64 %9, 14
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !121
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 15
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !121
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  store float %29, float* %10, align 4, !tbaa !124
  %30 = add nuw nsw i64 %7, 1
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = or i64 %9, 2
  %33 = getelementptr inbounds float, float* %3, i64 %32
  %34 = load float, float* %33, align 4, !tbaa !121
  %35 = fcmp olt float %34, 0xC7EFFFFFE0000000
  %36 = select i1 %35, float 0xC7EFFFFFE0000000, float %34
  %37 = or i64 %9, 3
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !121
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = add nuw nsw i64 %32, 14
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !121
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  %47 = add nuw nsw i64 %32, 15
  %48 = getelementptr inbounds float, float* %3, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !121
  %50 = fcmp ogt float %46, %49
  %51 = select i1 %50, float %46, float %49
  store float %51, float* %31, align 4, !tbaa !124
  %52 = add nuw nsw i64 %7, 2
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = add nuw nsw i64 %9, 4
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !121
  %57 = fcmp olt float %56, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %56
  %59 = add nuw nsw i64 %9, 5
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !121
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %9, 18
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !121
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = add nuw nsw i64 %9, 19
  %70 = getelementptr inbounds float, float* %3, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !121
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  store float %73, float* %53, align 4, !tbaa !124
  %74 = add nuw nsw i64 %7, 3
  %75 = getelementptr inbounds float, float* %2, i64 %74
  %76 = add nuw nsw i64 %9, 6
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !121
  %79 = fcmp olt float %78, 0xC7EFFFFFE0000000
  %80 = select i1 %79, float 0xC7EFFFFFE0000000, float %78
  %81 = add nuw nsw i64 %9, 7
  %82 = getelementptr inbounds float, float* %3, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !121
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = add nuw nsw i64 %9, 20
  %87 = getelementptr inbounds float, float* %3, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !121
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = add nuw nsw i64 %9, 21
  %92 = getelementptr inbounds float, float* %3, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !121
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  store float %95, float* %75, align 4, !tbaa !124
  %96 = add nuw nsw i64 %7, 4
  %97 = getelementptr inbounds float, float* %2, i64 %96
  %98 = add nuw nsw i64 %9, 8
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !121
  %101 = fcmp olt float %100, 0xC7EFFFFFE0000000
  %102 = select i1 %101, float 0xC7EFFFFFE0000000, float %100
  %103 = add nuw nsw i64 %9, 9
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !121
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 22
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !121
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 23
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !121
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  store float %117, float* %97, align 4, !tbaa !124
  %118 = add nuw nsw i64 %7, 5
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = add nuw nsw i64 %9, 10
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !121
  %123 = fcmp olt float %122, 0xC7EFFFFFE0000000
  %124 = select i1 %123, float 0xC7EFFFFFE0000000, float %122
  %125 = add nuw nsw i64 %9, 11
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !121
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 24
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !121
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 25
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !121
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  store float %139, float* %119, align 4, !tbaa !124
  %140 = add nuw nsw i64 %7, 6
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = add nuw nsw i64 %9, 12
  %143 = getelementptr inbounds float, float* %3, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !121
  %145 = fcmp olt float %144, 0xC7EFFFFFE0000000
  %146 = select i1 %145, float 0xC7EFFFFFE0000000, float %144
  %147 = add nuw nsw i64 %9, 13
  %148 = getelementptr inbounds float, float* %3, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !121
  %150 = fcmp ogt float %146, %149
  %151 = select i1 %150, float %146, float %149
  %152 = add nuw nsw i64 %9, 26
  %153 = getelementptr inbounds float, float* %3, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !121
  %155 = fcmp ogt float %151, %154
  %156 = select i1 %155, float %151, float %154
  %157 = add nuw nsw i64 %9, 27
  %158 = getelementptr inbounds float, float* %3, i64 %157
  %159 = load float, float* %158, align 4, !tbaa !121
  %160 = fcmp ogt float %156, %159
  %161 = select i1 %160, float %156, float %159
  store float %161, float* %141, align 4, !tbaa !124
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 7
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !24

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 64
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !24
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
entry:
  %3 = bitcast i8* %0 to %0**
  %4 = load %0*, %0** %3, align 8
  %5 = getelementptr inbounds i8, i8* %0, i64 8
  %6 = bitcast i8* %5 to %0**
  %7 = load %0*, %0** %6, align 8
  %8 = getelementptr inbounds i8, i8* %0, i64 16
  %9 = bitcast i8* %8 to %0**
  %10 = load %0*, %0** %9, align 8
  %11 = getelementptr inbounds %0, %0* %4, i64 0, i32 0
  %12 = load i8*, i8** %11, align 8
  %13 = getelementptr inbounds %0, %0* %7, i64 0, i32 0
  %14 = load i8*, i8** %13, align 8
  %15 = getelementptr inbounds %0, %0* %10, i64 0, i32 0
  %16 = load i8*, i8** %15, align 8
  tail call fastcc void @fused_nn_conv2d_3_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_3_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = alloca [28 x <8 x float>], align 16
  %4 = alloca [100 x <8 x float>], align 16
  %5 = alloca [1024 x float], align 16
  %6 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_end3 ]
  %7 = shl i64 %indvar, 5
  %8 = trunc i64 %indvar to i32
  %9 = add i32 %8, -2
  %10 = icmp ult i32 %9, 28
  %11 = mul nuw nsw i64 %indvar, 28
  %12 = add nsw i64 %11, -58
  br i1 %10, label %for_body2.us, label %for_body2.preheader

for_body2.preheader:                              ; preds = %for_begin1.preheader
  %scevgep = getelementptr [1024 x float], [1024 x float]* %5, i64 0, i64 %7
  %scevgep156 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep156, i8 0, i64 128, i1 false)
  br label %for_end3

for_body2.us:                                     ; preds = %for_begin1.preheader, %if_end.us
  %indvars.iv157 = phi i64 [ %indvars.iv.next158, %if_end.us ], [ 0, %for_begin1.preheader ]
  %13 = add nuw nsw i64 %indvars.iv157, %7
  %14 = trunc i64 %indvars.iv157 to i32
  %15 = add i32 %14, -2
  %16 = icmp ult i32 %15, 28
  br i1 %16, label %if_then.us, label %if_end.us

if_then.us:                                       ; preds = %for_body2.us
  %17 = add nsw i64 %12, %indvars.iv157
  %18 = getelementptr inbounds float, float* %6, i64 %17
  %19 = load float, float* %18, align 4, !tbaa !127
  br label %if_end.us

if_end.us:                                        ; preds = %if_then.us, %for_body2.us
  %20 = phi float [ %19, %if_then.us ], [ 0.000000e+00, %for_body2.us ]
  %21 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %13
  store float %20, float* %21, align 4, !tbaa !130
  %indvars.iv.next158 = add nuw nsw i64 %indvars.iv157, 1
  %exitcond160 = icmp eq i64 %indvars.iv.next158, 32
  br i1 %exitcond160, label %for_end3, label %for_body2.us, !prof !24

for_begin4.preheader:                             ; preds = %for_end3
  %.sub = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0
  %22 = bitcast i8* %1 to float*
  br label %for_begin7.preheader

for_end3:                                         ; preds = %if_end.us, %for_body2.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond162 = icmp eq i64 %indvar.next, 32
  br i1 %exitcond162, label %for_begin4.preheader, label %for_begin1.preheader, !prof !24

for_begin7.preheader:                             ; preds = %for_begin7.preheader, %for_begin4.preheader
  %indvars.iv150 = phi i64 [ 0, %for_begin4.preheader ], [ %indvars.iv.next151, %for_begin7.preheader ]
  %23 = mul nuw nsw i64 %indvars.iv150, 40
  %24 = trunc i64 %indvars.iv150 to i32
  %25 = urem i32 %24, 5
  %26 = mul nuw nsw i32 %25, 5
  %27 = udiv i32 %24, 5
  %28 = mul nsw i32 %27, 200
  %29 = add nuw nsw i32 %28, %26
  %30 = zext i32 %29 to i64
  %31 = add nuw nsw i64 %30, 25
  %32 = add nuw nsw i64 %30, 50
  %33 = add nuw nsw i64 %30, 75
  %34 = add nuw nsw i64 %30, 100
  %35 = add nuw nsw i64 %30, 125
  %36 = add nuw nsw i64 %30, 150
  %37 = add nuw nsw i64 %30, 175
  %38 = getelementptr inbounds float, float* %22, i64 %30
  %39 = load float, float* %38, align 4, !tbaa !133
  %40 = insertelement <8 x float> undef, float %39, i32 0
  %41 = getelementptr inbounds float, float* %22, i64 %31
  %42 = load float, float* %41, align 4, !tbaa !133
  %43 = insertelement <8 x float> %40, float %42, i32 1
  %44 = getelementptr inbounds float, float* %22, i64 %32
  %45 = load float, float* %44, align 4, !tbaa !133
  %46 = insertelement <8 x float> %43, float %45, i32 2
  %47 = getelementptr inbounds float, float* %22, i64 %33
  %48 = load float, float* %47, align 4, !tbaa !133
  %49 = insertelement <8 x float> %46, float %48, i32 3
  %50 = getelementptr inbounds float, float* %22, i64 %34
  %51 = load float, float* %50, align 4, !tbaa !133
  %52 = insertelement <8 x float> %49, float %51, i32 4
  %53 = getelementptr inbounds float, float* %22, i64 %35
  %54 = load float, float* %53, align 4, !tbaa !133
  %55 = insertelement <8 x float> %52, float %54, i32 5
  %56 = getelementptr inbounds float, float* %22, i64 %36
  %57 = load float, float* %56, align 4, !tbaa !133
  %58 = insertelement <8 x float> %55, float %57, i32 6
  %59 = getelementptr inbounds float, float* %22, i64 %37
  %60 = load float, float* %59, align 4, !tbaa !133
  %61 = insertelement <8 x float> %58, float %60, i32 7
  %62 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %23
  %63 = bitcast float* %62 to <8 x float>*
  store <8 x float> %61, <8 x float>* %63, align 16, !tbaa !136
  %64 = add nuw nsw i64 %23, 8
  %65 = add nuw nsw i64 %30, 1
  %66 = add nuw nsw i64 %30, 26
  %67 = add nuw nsw i64 %30, 51
  %68 = add nuw nsw i64 %30, 76
  %69 = add nuw nsw i64 %30, 101
  %70 = add nuw nsw i64 %30, 126
  %71 = add nuw nsw i64 %30, 151
  %72 = add nuw nsw i64 %30, 176
  %73 = getelementptr inbounds float, float* %22, i64 %65
  %74 = load float, float* %73, align 4, !tbaa !133
  %75 = insertelement <8 x float> undef, float %74, i32 0
  %76 = getelementptr inbounds float, float* %22, i64 %66
  %77 = load float, float* %76, align 4, !tbaa !133
  %78 = insertelement <8 x float> %75, float %77, i32 1
  %79 = getelementptr inbounds float, float* %22, i64 %67
  %80 = load float, float* %79, align 4, !tbaa !133
  %81 = insertelement <8 x float> %78, float %80, i32 2
  %82 = getelementptr inbounds float, float* %22, i64 %68
  %83 = load float, float* %82, align 4, !tbaa !133
  %84 = insertelement <8 x float> %81, float %83, i32 3
  %85 = getelementptr inbounds float, float* %22, i64 %69
  %86 = load float, float* %85, align 4, !tbaa !133
  %87 = insertelement <8 x float> %84, float %86, i32 4
  %88 = getelementptr inbounds float, float* %22, i64 %70
  %89 = load float, float* %88, align 4, !tbaa !133
  %90 = insertelement <8 x float> %87, float %89, i32 5
  %91 = getelementptr inbounds float, float* %22, i64 %71
  %92 = load float, float* %91, align 4, !tbaa !133
  %93 = insertelement <8 x float> %90, float %92, i32 6
  %94 = getelementptr inbounds float, float* %22, i64 %72
  %95 = load float, float* %94, align 4, !tbaa !133
  %96 = insertelement <8 x float> %93, float %95, i32 7
  %97 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %64
  %98 = bitcast float* %97 to <8 x float>*
  store <8 x float> %96, <8 x float>* %98, align 16, !tbaa !136
  %99 = add nuw nsw i64 %23, 16
  %100 = add nuw nsw i64 %30, 2
  %101 = add nuw nsw i64 %30, 27
  %102 = add nuw nsw i64 %30, 52
  %103 = add nuw nsw i64 %30, 77
  %104 = add nuw nsw i64 %30, 102
  %105 = add nuw nsw i64 %30, 127
  %106 = add nuw nsw i64 %30, 152
  %107 = add nuw nsw i64 %30, 177
  %108 = getelementptr inbounds float, float* %22, i64 %100
  %109 = load float, float* %108, align 4, !tbaa !133
  %110 = insertelement <8 x float> undef, float %109, i32 0
  %111 = getelementptr inbounds float, float* %22, i64 %101
  %112 = load float, float* %111, align 4, !tbaa !133
  %113 = insertelement <8 x float> %110, float %112, i32 1
  %114 = getelementptr inbounds float, float* %22, i64 %102
  %115 = load float, float* %114, align 4, !tbaa !133
  %116 = insertelement <8 x float> %113, float %115, i32 2
  %117 = getelementptr inbounds float, float* %22, i64 %103
  %118 = load float, float* %117, align 4, !tbaa !133
  %119 = insertelement <8 x float> %116, float %118, i32 3
  %120 = getelementptr inbounds float, float* %22, i64 %104
  %121 = load float, float* %120, align 4, !tbaa !133
  %122 = insertelement <8 x float> %119, float %121, i32 4
  %123 = getelementptr inbounds float, float* %22, i64 %105
  %124 = load float, float* %123, align 4, !tbaa !133
  %125 = insertelement <8 x float> %122, float %124, i32 5
  %126 = getelementptr inbounds float, float* %22, i64 %106
  %127 = load float, float* %126, align 4, !tbaa !133
  %128 = insertelement <8 x float> %125, float %127, i32 6
  %129 = getelementptr inbounds float, float* %22, i64 %107
  %130 = load float, float* %129, align 4, !tbaa !133
  %131 = insertelement <8 x float> %128, float %130, i32 7
  %132 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %99
  %133 = bitcast float* %132 to <8 x float>*
  store <8 x float> %131, <8 x float>* %133, align 16, !tbaa !136
  %134 = add nuw nsw i64 %23, 24
  %135 = add nuw nsw i64 %30, 3
  %136 = add nuw nsw i64 %30, 28
  %137 = add nuw nsw i64 %30, 53
  %138 = add nuw nsw i64 %30, 78
  %139 = add nuw nsw i64 %30, 103
  %140 = add nuw nsw i64 %30, 128
  %141 = add nuw nsw i64 %30, 153
  %142 = add nuw nsw i64 %30, 178
  %143 = getelementptr inbounds float, float* %22, i64 %135
  %144 = load float, float* %143, align 4, !tbaa !133
  %145 = insertelement <8 x float> undef, float %144, i32 0
  %146 = getelementptr inbounds float, float* %22, i64 %136
  %147 = load float, float* %146, align 4, !tbaa !133
  %148 = insertelement <8 x float> %145, float %147, i32 1
  %149 = getelementptr inbounds float, float* %22, i64 %137
  %150 = load float, float* %149, align 4, !tbaa !133
  %151 = insertelement <8 x float> %148, float %150, i32 2
  %152 = getelementptr inbounds float, float* %22, i64 %138
  %153 = load float, float* %152, align 4, !tbaa !133
  %154 = insertelement <8 x float> %151, float %153, i32 3
  %155 = getelementptr inbounds float, float* %22, i64 %139
  %156 = load float, float* %155, align 4, !tbaa !133
  %157 = insertelement <8 x float> %154, float %156, i32 4
  %158 = getelementptr inbounds float, float* %22, i64 %140
  %159 = load float, float* %158, align 4, !tbaa !133
  %160 = insertelement <8 x float> %157, float %159, i32 5
  %161 = getelementptr inbounds float, float* %22, i64 %141
  %162 = load float, float* %161, align 4, !tbaa !133
  %163 = insertelement <8 x float> %160, float %162, i32 6
  %164 = getelementptr inbounds float, float* %22, i64 %142
  %165 = load float, float* %164, align 4, !tbaa !133
  %166 = insertelement <8 x float> %163, float %165, i32 7
  %167 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %134
  %168 = bitcast float* %167 to <8 x float>*
  store <8 x float> %166, <8 x float>* %168, align 16, !tbaa !136
  %169 = add nuw nsw i64 %23, 32
  %170 = add nuw nsw i64 %30, 4
  %171 = add nuw nsw i64 %30, 29
  %172 = add nuw nsw i64 %30, 54
  %173 = add nuw nsw i64 %30, 79
  %174 = add nuw nsw i64 %30, 104
  %175 = add nuw nsw i64 %30, 129
  %176 = add nuw nsw i64 %30, 154
  %177 = add nuw nsw i64 %30, 179
  %178 = getelementptr inbounds float, float* %22, i64 %170
  %179 = load float, float* %178, align 4, !tbaa !133
  %180 = insertelement <8 x float> undef, float %179, i32 0
  %181 = getelementptr inbounds float, float* %22, i64 %171
  %182 = load float, float* %181, align 4, !tbaa !133
  %183 = insertelement <8 x float> %180, float %182, i32 1
  %184 = getelementptr inbounds float, float* %22, i64 %172
  %185 = load float, float* %184, align 4, !tbaa !133
  %186 = insertelement <8 x float> %183, float %185, i32 2
  %187 = getelementptr inbounds float, float* %22, i64 %173
  %188 = load float, float* %187, align 4, !tbaa !133
  %189 = insertelement <8 x float> %186, float %188, i32 3
  %190 = getelementptr inbounds float, float* %22, i64 %174
  %191 = load float, float* %190, align 4, !tbaa !133
  %192 = insertelement <8 x float> %189, float %191, i32 4
  %193 = getelementptr inbounds float, float* %22, i64 %175
  %194 = load float, float* %193, align 4, !tbaa !133
  %195 = insertelement <8 x float> %192, float %194, i32 5
  %196 = getelementptr inbounds float, float* %22, i64 %176
  %197 = load float, float* %196, align 4, !tbaa !133
  %198 = insertelement <8 x float> %195, float %197, i32 6
  %199 = getelementptr inbounds float, float* %22, i64 %177
  %200 = load float, float* %199, align 4, !tbaa !133
  %201 = insertelement <8 x float> %198, float %200, i32 7
  %202 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %169
  %203 = bitcast float* %202 to <8 x float>*
  store <8 x float> %201, <8 x float>* %203, align 16, !tbaa !136
  %indvars.iv.next151 = add nuw nsw i64 %indvars.iv150, 1
  %exitcond152 = icmp eq i64 %indvars.iv.next151, 20
  br i1 %exitcond152, label %for_begin10.preheader, label %for_begin7.preheader, !prof !24

for_begin10.preheader:                            ; preds = %for_begin7.preheader
  %204 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %205 = bitcast float* %204 to <8 x float>*
  %206 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %207 = bitcast float* %206 to <8 x float>*
  %208 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %209 = bitcast float* %208 to <8 x float>*
  %210 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %211 = bitcast float* %210 to <8 x float>*
  %212 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %213 = bitcast float* %212 to <8 x float>*
  %214 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %215 = bitcast float* %214 to <8 x float>*
  %216 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %217 = bitcast float* %216 to <8 x float>*
  %218 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %219 = bitcast float* %218 to <8 x float>*
  %220 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %221 = bitcast float* %220 to <8 x float>*
  %222 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %223 = bitcast float* %222 to <8 x float>*
  %224 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %225 = bitcast float* %224 to <8 x float>*
  %226 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 96
  %227 = bitcast float* %226 to <8 x float>*
  %228 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 104
  %229 = bitcast float* %228 to <8 x float>*
  %230 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 112
  %231 = bitcast float* %230 to <8 x float>*
  %232 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 120
  %233 = bitcast float* %232 to <8 x float>*
  %234 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 128
  %235 = bitcast float* %234 to <8 x float>*
  %236 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 136
  %237 = bitcast float* %236 to <8 x float>*
  %238 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 144
  %239 = bitcast float* %238 to <8 x float>*
  %240 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 152
  %241 = bitcast float* %240 to <8 x float>*
  %242 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 160
  %243 = bitcast float* %242 to <8 x float>*
  %244 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 168
  %245 = bitcast float* %244 to <8 x float>*
  %246 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 176
  %247 = bitcast float* %246 to <8 x float>*
  %248 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 184
  %249 = bitcast float* %248 to <8 x float>*
  %250 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 192
  %251 = bitcast float* %250 to <8 x float>*
  %252 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 200
  %253 = bitcast float* %252 to <8 x float>*
  %254 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 208
  %255 = bitcast float* %254 to <8 x float>*
  %256 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 216
  %257 = bitcast float* %256 to <8 x float>*
  %258 = bitcast i8* %2 to float*
  %259 = bitcast [28 x <8 x float>]* %3 to i8*
  br label %for_body11

for_body11:                                       ; preds = %for_end21, %for_begin10.preheader
  %260 = phi i32 [ 0, %for_begin10.preheader ], [ %505, %for_end21 ]
  %261 = urem i32 %260, 28
  %262 = udiv i32 %260, 28
  %263 = mul nsw i32 %262, 200
  %264 = zext i32 %263 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %259, i8 0, i64 896, i1 false)
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end21
  ret void

for_begin19.preheader:                            ; preds = %for_end18
  store <8 x float> %313, <8 x float>* %.sub, align 16, !tbaa !139
  store <8 x float> %319, <8 x float>* %205, align 16, !tbaa !139
  store <8 x float> %325, <8 x float>* %207, align 16, !tbaa !139
  store <8 x float> %331, <8 x float>* %209, align 16, !tbaa !139
  store <8 x float> %337, <8 x float>* %211, align 16, !tbaa !139
  store <8 x float> %343, <8 x float>* %213, align 16, !tbaa !139
  store <8 x float> %349, <8 x float>* %215, align 16, !tbaa !139
  store <8 x float> %355, <8 x float>* %217, align 16, !tbaa !139
  store <8 x float> %361, <8 x float>* %219, align 16, !tbaa !139
  store <8 x float> %367, <8 x float>* %221, align 16, !tbaa !139
  store <8 x float> %373, <8 x float>* %223, align 16, !tbaa !139
  store <8 x float> %379, <8 x float>* %225, align 16, !tbaa !139
  store <8 x float> %385, <8 x float>* %227, align 16, !tbaa !139
  store <8 x float> %391, <8 x float>* %229, align 16, !tbaa !139
  store <8 x float> %397, <8 x float>* %231, align 16, !tbaa !139
  store <8 x float> %403, <8 x float>* %233, align 16, !tbaa !139
  store <8 x float> %409, <8 x float>* %235, align 16, !tbaa !139
  store <8 x float> %415, <8 x float>* %237, align 16, !tbaa !139
  store <8 x float> %421, <8 x float>* %239, align 16, !tbaa !139
  store <8 x float> %427, <8 x float>* %241, align 16, !tbaa !139
  store <8 x float> %433, <8 x float>* %243, align 16, !tbaa !139
  store <8 x float> %439, <8 x float>* %245, align 16, !tbaa !139
  store <8 x float> %445, <8 x float>* %247, align 16, !tbaa !139
  store <8 x float> %451, <8 x float>* %249, align 16, !tbaa !139
  store <8 x float> %457, <8 x float>* %251, align 16, !tbaa !139
  store <8 x float> %463, <8 x float>* %253, align 16, !tbaa !139
  store <8 x float> %469, <8 x float>* %255, align 16, !tbaa !139
  store <8 x float> %475, <8 x float>* %257, align 16, !tbaa !139
  %265 = mul nuw nsw i32 %261, 28
  %266 = mul nsw i32 %262, 6272
  %267 = add nuw nsw i32 %266, %265
  %268 = zext i32 %267 to i64
  br label %for_body20

for_begin16.preheader:                            ; preds = %for_end18, %for_body11
  %indvars.iv140 = phi i64 [ 0, %for_body11 ], [ %indvars.iv.next141, %for_end18 ]
  %.lcssa56111 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %475, %for_end18 ]
  %.lcssa54109 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %469, %for_end18 ]
  %.lcssa52107 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %463, %for_end18 ]
  %.lcssa50105 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %457, %for_end18 ]
  %.lcssa48103 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %451, %for_end18 ]
  %.lcssa46101 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %445, %for_end18 ]
  %.lcssa4499 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %439, %for_end18 ]
  %.lcssa4297 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %433, %for_end18 ]
  %.lcssa4095 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %427, %for_end18 ]
  %.lcssa3893 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %421, %for_end18 ]
  %.lcssa3691 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %415, %for_end18 ]
  %.lcssa3489 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %409, %for_end18 ]
  %.lcssa3287 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %403, %for_end18 ]
  %.lcssa3085 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %397, %for_end18 ]
  %.lcssa2883 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %391, %for_end18 ]
  %.lcssa2681 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %385, %for_end18 ]
  %.lcssa2479 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %379, %for_end18 ]
  %.lcssa2277 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %373, %for_end18 ]
  %.lcssa2075 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %367, %for_end18 ]
  %.lcssa1873 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %361, %for_end18 ]
  %.lcssa1671 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %355, %for_end18 ]
  %.lcssa1469 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %349, %for_end18 ]
  %.lcssa1267 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %343, %for_end18 ]
  %.lcssa1065 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %337, %for_end18 ]
  %.lcssa863 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %331, %for_end18 ]
  %.lcssa661 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %325, %for_end18 ]
  %.lcssa460 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %319, %for_end18 ]
  %.lcssa58 = phi <8 x float> [ zeroinitializer, %for_body11 ], [ %313, %for_end18 ]
  %269 = phi i32 [ 0, %for_body11 ], [ %476, %for_end18 ]
  %270 = add nuw nsw i32 %269, %261
  %271 = shl i32 %270, 5
  %272 = mul nuw nsw i64 %indvars.iv140, 40
  %273 = add nuw nsw i64 %272, %264
  %274 = sext i32 %271 to i64
  br label %for_body17

for_body17:                                       ; preds = %for_body17, %for_begin16.preheader
  %indvars.iv = phi i64 [ 0, %for_begin16.preheader ], [ %indvars.iv.next, %for_body17 ]
  %275 = phi <8 x float> [ %.lcssa56111, %for_begin16.preheader ], [ %475, %for_body17 ]
  %276 = phi <8 x float> [ %.lcssa54109, %for_begin16.preheader ], [ %469, %for_body17 ]
  %277 = phi <8 x float> [ %.lcssa52107, %for_begin16.preheader ], [ %463, %for_body17 ]
  %278 = phi <8 x float> [ %.lcssa50105, %for_begin16.preheader ], [ %457, %for_body17 ]
  %279 = phi <8 x float> [ %.lcssa48103, %for_begin16.preheader ], [ %451, %for_body17 ]
  %280 = phi <8 x float> [ %.lcssa46101, %for_begin16.preheader ], [ %445, %for_body17 ]
  %281 = phi <8 x float> [ %.lcssa4499, %for_begin16.preheader ], [ %439, %for_body17 ]
  %282 = phi <8 x float> [ %.lcssa4297, %for_begin16.preheader ], [ %433, %for_body17 ]
  %283 = phi <8 x float> [ %.lcssa4095, %for_begin16.preheader ], [ %427, %for_body17 ]
  %284 = phi <8 x float> [ %.lcssa3893, %for_begin16.preheader ], [ %421, %for_body17 ]
  %285 = phi <8 x float> [ %.lcssa3691, %for_begin16.preheader ], [ %415, %for_body17 ]
  %286 = phi <8 x float> [ %.lcssa3489, %for_begin16.preheader ], [ %409, %for_body17 ]
  %287 = phi <8 x float> [ %.lcssa3287, %for_begin16.preheader ], [ %403, %for_body17 ]
  %288 = phi <8 x float> [ %.lcssa3085, %for_begin16.preheader ], [ %397, %for_body17 ]
  %289 = phi <8 x float> [ %.lcssa2883, %for_begin16.preheader ], [ %391, %for_body17 ]
  %290 = phi <8 x float> [ %.lcssa2681, %for_begin16.preheader ], [ %385, %for_body17 ]
  %291 = phi <8 x float> [ %.lcssa2479, %for_begin16.preheader ], [ %379, %for_body17 ]
  %292 = phi <8 x float> [ %.lcssa2277, %for_begin16.preheader ], [ %373, %for_body17 ]
  %293 = phi <8 x float> [ %.lcssa2075, %for_begin16.preheader ], [ %367, %for_body17 ]
  %294 = phi <8 x float> [ %.lcssa1873, %for_begin16.preheader ], [ %361, %for_body17 ]
  %295 = phi <8 x float> [ %.lcssa1671, %for_begin16.preheader ], [ %355, %for_body17 ]
  %296 = phi <8 x float> [ %.lcssa1469, %for_begin16.preheader ], [ %349, %for_body17 ]
  %297 = phi <8 x float> [ %.lcssa1267, %for_begin16.preheader ], [ %343, %for_body17 ]
  %298 = phi <8 x float> [ %.lcssa1065, %for_begin16.preheader ], [ %337, %for_body17 ]
  %299 = phi <8 x float> [ %.lcssa863, %for_begin16.preheader ], [ %331, %for_body17 ]
  %300 = phi <8 x float> [ %.lcssa661, %for_begin16.preheader ], [ %325, %for_body17 ]
  %301 = phi <8 x float> [ %.lcssa460, %for_begin16.preheader ], [ %319, %for_body17 ]
  %302 = phi <8 x float> [ %.lcssa58, %for_begin16.preheader ], [ %313, %for_body17 ]
  %303 = add nsw i64 %indvars.iv, %274
  %304 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %303
  %305 = load float, float* %304, align 4, !tbaa !130
  %306 = insertelement <8 x float> undef, float %305, i32 0
  %307 = shufflevector <8 x float> %306, <8 x float> undef, <8 x i32> zeroinitializer
  %308 = shl i64 %indvars.iv, 3
  %309 = add nuw nsw i64 %273, %308
  %310 = getelementptr inbounds [100 x <8 x float>], [100 x <8 x float>]* %4, i64 0, i64 0, i64 %309
  %311 = bitcast float* %310 to <8 x float>*
  %312 = load <8 x float>, <8 x float>* %311, align 16, !tbaa !136
  %313 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %307, <8 x float> %312, <8 x float> %302)
  %314 = add nsw i64 %303, 1
  %315 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %314
  %316 = load float, float* %315, align 4, !tbaa !130
  %317 = insertelement <8 x float> undef, float %316, i32 0
  %318 = shufflevector <8 x float> %317, <8 x float> undef, <8 x i32> zeroinitializer
  %319 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %318, <8 x float> %312, <8 x float> %301)
  %320 = add nsw i64 %303, 2
  %321 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %320
  %322 = load float, float* %321, align 4, !tbaa !130
  %323 = insertelement <8 x float> undef, float %322, i32 0
  %324 = shufflevector <8 x float> %323, <8 x float> undef, <8 x i32> zeroinitializer
  %325 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %324, <8 x float> %312, <8 x float> %300)
  %326 = add nsw i64 %303, 3
  %327 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %326
  %328 = load float, float* %327, align 4, !tbaa !130
  %329 = insertelement <8 x float> undef, float %328, i32 0
  %330 = shufflevector <8 x float> %329, <8 x float> undef, <8 x i32> zeroinitializer
  %331 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %330, <8 x float> %312, <8 x float> %299)
  %332 = add nsw i64 %303, 4
  %333 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %332
  %334 = load float, float* %333, align 4, !tbaa !130
  %335 = insertelement <8 x float> undef, float %334, i32 0
  %336 = shufflevector <8 x float> %335, <8 x float> undef, <8 x i32> zeroinitializer
  %337 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %336, <8 x float> %312, <8 x float> %298)
  %338 = add nsw i64 %303, 5
  %339 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %338
  %340 = load float, float* %339, align 4, !tbaa !130
  %341 = insertelement <8 x float> undef, float %340, i32 0
  %342 = shufflevector <8 x float> %341, <8 x float> undef, <8 x i32> zeroinitializer
  %343 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %342, <8 x float> %312, <8 x float> %297)
  %344 = add nsw i64 %303, 6
  %345 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %344
  %346 = load float, float* %345, align 4, !tbaa !130
  %347 = insertelement <8 x float> undef, float %346, i32 0
  %348 = shufflevector <8 x float> %347, <8 x float> undef, <8 x i32> zeroinitializer
  %349 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %348, <8 x float> %312, <8 x float> %296)
  %350 = add nsw i64 %303, 7
  %351 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %350
  %352 = load float, float* %351, align 4, !tbaa !130
  %353 = insertelement <8 x float> undef, float %352, i32 0
  %354 = shufflevector <8 x float> %353, <8 x float> undef, <8 x i32> zeroinitializer
  %355 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %354, <8 x float> %312, <8 x float> %295)
  %356 = add nsw i64 %303, 8
  %357 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %356
  %358 = load float, float* %357, align 4, !tbaa !130
  %359 = insertelement <8 x float> undef, float %358, i32 0
  %360 = shufflevector <8 x float> %359, <8 x float> undef, <8 x i32> zeroinitializer
  %361 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %360, <8 x float> %312, <8 x float> %294)
  %362 = add nsw i64 %303, 9
  %363 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %362
  %364 = load float, float* %363, align 4, !tbaa !130
  %365 = insertelement <8 x float> undef, float %364, i32 0
  %366 = shufflevector <8 x float> %365, <8 x float> undef, <8 x i32> zeroinitializer
  %367 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %366, <8 x float> %312, <8 x float> %293)
  %368 = add nsw i64 %303, 10
  %369 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %368
  %370 = load float, float* %369, align 4, !tbaa !130
  %371 = insertelement <8 x float> undef, float %370, i32 0
  %372 = shufflevector <8 x float> %371, <8 x float> undef, <8 x i32> zeroinitializer
  %373 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %372, <8 x float> %312, <8 x float> %292)
  %374 = add nsw i64 %303, 11
  %375 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %374
  %376 = load float, float* %375, align 4, !tbaa !130
  %377 = insertelement <8 x float> undef, float %376, i32 0
  %378 = shufflevector <8 x float> %377, <8 x float> undef, <8 x i32> zeroinitializer
  %379 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %378, <8 x float> %312, <8 x float> %291)
  %380 = add nsw i64 %303, 12
  %381 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %380
  %382 = load float, float* %381, align 4, !tbaa !130
  %383 = insertelement <8 x float> undef, float %382, i32 0
  %384 = shufflevector <8 x float> %383, <8 x float> undef, <8 x i32> zeroinitializer
  %385 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %384, <8 x float> %312, <8 x float> %290)
  %386 = add nsw i64 %303, 13
  %387 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %386
  %388 = load float, float* %387, align 4, !tbaa !130
  %389 = insertelement <8 x float> undef, float %388, i32 0
  %390 = shufflevector <8 x float> %389, <8 x float> undef, <8 x i32> zeroinitializer
  %391 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %390, <8 x float> %312, <8 x float> %289)
  %392 = add nsw i64 %303, 14
  %393 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %392
  %394 = load float, float* %393, align 4, !tbaa !130
  %395 = insertelement <8 x float> undef, float %394, i32 0
  %396 = shufflevector <8 x float> %395, <8 x float> undef, <8 x i32> zeroinitializer
  %397 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %396, <8 x float> %312, <8 x float> %288)
  %398 = add nsw i64 %303, 15
  %399 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %398
  %400 = load float, float* %399, align 4, !tbaa !130
  %401 = insertelement <8 x float> undef, float %400, i32 0
  %402 = shufflevector <8 x float> %401, <8 x float> undef, <8 x i32> zeroinitializer
  %403 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %402, <8 x float> %312, <8 x float> %287)
  %404 = add nsw i64 %303, 16
  %405 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %404
  %406 = load float, float* %405, align 4, !tbaa !130
  %407 = insertelement <8 x float> undef, float %406, i32 0
  %408 = shufflevector <8 x float> %407, <8 x float> undef, <8 x i32> zeroinitializer
  %409 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %408, <8 x float> %312, <8 x float> %286)
  %410 = add nsw i64 %303, 17
  %411 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %410
  %412 = load float, float* %411, align 4, !tbaa !130
  %413 = insertelement <8 x float> undef, float %412, i32 0
  %414 = shufflevector <8 x float> %413, <8 x float> undef, <8 x i32> zeroinitializer
  %415 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %414, <8 x float> %312, <8 x float> %285)
  %416 = add nsw i64 %303, 18
  %417 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %416
  %418 = load float, float* %417, align 4, !tbaa !130
  %419 = insertelement <8 x float> undef, float %418, i32 0
  %420 = shufflevector <8 x float> %419, <8 x float> undef, <8 x i32> zeroinitializer
  %421 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %420, <8 x float> %312, <8 x float> %284)
  %422 = add nsw i64 %303, 19
  %423 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %422
  %424 = load float, float* %423, align 4, !tbaa !130
  %425 = insertelement <8 x float> undef, float %424, i32 0
  %426 = shufflevector <8 x float> %425, <8 x float> undef, <8 x i32> zeroinitializer
  %427 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %426, <8 x float> %312, <8 x float> %283)
  %428 = add nsw i64 %303, 20
  %429 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %428
  %430 = load float, float* %429, align 4, !tbaa !130
  %431 = insertelement <8 x float> undef, float %430, i32 0
  %432 = shufflevector <8 x float> %431, <8 x float> undef, <8 x i32> zeroinitializer
  %433 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %432, <8 x float> %312, <8 x float> %282)
  %434 = add nsw i64 %303, 21
  %435 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %434
  %436 = load float, float* %435, align 4, !tbaa !130
  %437 = insertelement <8 x float> undef, float %436, i32 0
  %438 = shufflevector <8 x float> %437, <8 x float> undef, <8 x i32> zeroinitializer
  %439 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %438, <8 x float> %312, <8 x float> %281)
  %440 = add nsw i64 %303, 22
  %441 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %440
  %442 = load float, float* %441, align 4, !tbaa !130
  %443 = insertelement <8 x float> undef, float %442, i32 0
  %444 = shufflevector <8 x float> %443, <8 x float> undef, <8 x i32> zeroinitializer
  %445 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %444, <8 x float> %312, <8 x float> %280)
  %446 = add nsw i64 %303, 23
  %447 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %446
  %448 = load float, float* %447, align 4, !tbaa !130
  %449 = insertelement <8 x float> undef, float %448, i32 0
  %450 = shufflevector <8 x float> %449, <8 x float> undef, <8 x i32> zeroinitializer
  %451 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %450, <8 x float> %312, <8 x float> %279)
  %452 = add nsw i64 %303, 24
  %453 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %452
  %454 = load float, float* %453, align 4, !tbaa !130
  %455 = insertelement <8 x float> undef, float %454, i32 0
  %456 = shufflevector <8 x float> %455, <8 x float> undef, <8 x i32> zeroinitializer
  %457 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %456, <8 x float> %312, <8 x float> %278)
  %458 = add nsw i64 %303, 25
  %459 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %458
  %460 = load float, float* %459, align 4, !tbaa !130
  %461 = insertelement <8 x float> undef, float %460, i32 0
  %462 = shufflevector <8 x float> %461, <8 x float> undef, <8 x i32> zeroinitializer
  %463 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %462, <8 x float> %312, <8 x float> %277)
  %464 = add nsw i64 %303, 26
  %465 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %464
  %466 = load float, float* %465, align 4, !tbaa !130
  %467 = insertelement <8 x float> undef, float %466, i32 0
  %468 = shufflevector <8 x float> %467, <8 x float> undef, <8 x i32> zeroinitializer
  %469 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %468, <8 x float> %312, <8 x float> %276)
  %470 = add nsw i64 %303, 27
  %471 = getelementptr inbounds [1024 x float], [1024 x float]* %5, i64 0, i64 %470
  %472 = load float, float* %471, align 4, !tbaa !130
  %473 = insertelement <8 x float> undef, float %472, i32 0
  %474 = shufflevector <8 x float> %473, <8 x float> undef, <8 x i32> zeroinitializer
  %475 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %474, <8 x float> %312, <8 x float> %275)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 5
  br i1 %exitcond, label %for_end18, label %for_body17, !prof !24

for_end18:                                        ; preds = %for_body17
  %indvars.iv.next141 = add nuw nsw i64 %indvars.iv140, 1
  %476 = add nuw nsw i32 %269, 1
  %exitcond142 = icmp eq i64 %indvars.iv.next141, 5
  br i1 %exitcond142, label %for_begin19.preheader, label %for_begin16.preheader, !prof !24

for_body20:                                       ; preds = %for_body20, %for_begin19.preheader
  %indvars.iv143 = phi i64 [ 0, %for_begin19.preheader ], [ %indvars.iv.next144, %for_body20 ]
  %477 = add nuw nsw i64 %indvars.iv143, %268
  %478 = add nuw nsw i64 %477, 784
  %479 = add nuw nsw i64 %477, 1568
  %480 = add nuw nsw i64 %477, 2352
  %481 = add nuw nsw i64 %477, 3136
  %482 = add nuw nsw i64 %477, 3920
  %483 = add nuw nsw i64 %477, 4704
  %484 = add nuw nsw i64 %477, 5488
  %485 = shl nsw i64 %indvars.iv143, 3
  %486 = getelementptr inbounds [28 x <8 x float>], [28 x <8 x float>]* %3, i64 0, i64 0, i64 %485
  %487 = bitcast float* %486 to <8 x float>*
  %488 = load <8 x float>, <8 x float>* %487, align 16, !tbaa !150
  %489 = getelementptr inbounds float, float* %258, i64 %477
  %490 = extractelement <8 x float> %488, i64 0
  store float %490, float* %489, align 4, !tbaa !151
  %491 = getelementptr inbounds float, float* %258, i64 %478
  %492 = extractelement <8 x float> %488, i64 1
  store float %492, float* %491, align 4, !tbaa !151
  %493 = getelementptr inbounds float, float* %258, i64 %479
  %494 = extractelement <8 x float> %488, i64 2
  store float %494, float* %493, align 4, !tbaa !151
  %495 = getelementptr inbounds float, float* %258, i64 %480
  %496 = extractelement <8 x float> %488, i64 3
  store float %496, float* %495, align 4, !tbaa !151
  %497 = getelementptr inbounds float, float* %258, i64 %481
  %498 = extractelement <8 x float> %488, i64 4
  store float %498, float* %497, align 4, !tbaa !151
  %499 = getelementptr inbounds float, float* %258, i64 %482
  %500 = extractelement <8 x float> %488, i64 5
  store float %500, float* %499, align 4, !tbaa !151
  %501 = getelementptr inbounds float, float* %258, i64 %483
  %502 = extractelement <8 x float> %488, i64 6
  store float %502, float* %501, align 4, !tbaa !151
  %503 = getelementptr inbounds float, float* %258, i64 %484
  %504 = extractelement <8 x float> %488, i64 7
  store float %504, float* %503, align 4, !tbaa !151
  %indvars.iv.next144 = add nuw nsw i64 %indvars.iv143, 1
  %exitcond145 = icmp eq i64 %indvars.iv.next144, 28
  br i1 %exitcond145, label %for_end21, label %for_body20, !prof !24

for_end21:                                        ; preds = %for_body20
  %505 = add nuw nsw i32 %260, 1
  %exitcond146 = icmp eq i32 %505, 112
  br i1 %exitcond146, label %for_end12, label %for_body11, !prof !24
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #5

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #5

attributes #0 = { norecurse nounwind }
attributes #1 = { noinline norecurse nounwind }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind }
attributes #4 = { nounwind readnone speculatable }
attributes #5 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = !{!6, !6, i64 0}
!6 = !{!"float32", !7, i64 0}
!7 = !{!"0x2425d50", !8, i64 0}
!8 = !{!"tvm-tbaa"}
!9 = !{!10, !10, i64 0}
!10 = !{!"float32", !11, i64 0}
!11 = !{!"0x2425d10", !8, i64 0}
!12 = distinct !{!12, !13}
!13 = !{!"llvm.loop.isvectorized", i32 1}
!14 = !{!15, !15, i64 0}
!15 = !{!"float32", !16, i64 0}
!16 = !{!"0x242c020", !8, i64 0}
!17 = !{!18, !18, i64 0}
!18 = !{!"float32", !19, i64 0}
!19 = !{!"0x242c730", !8, i64 0}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x242c060", !8, i64 0}
!23 = distinct !{!23, !13}
!24 = !{!"branch_weights", i32 1, i32 1048576}
!25 = !{!26, !26, i64 0}
!26 = !{!"float32", !27, i64 0}
!27 = !{!"0x24192e0", !8, i64 0}
!28 = !{!29, !29, i64 0}
!29 = !{!"float32", !30, i64 0}
!30 = !{!"0x2419580", !8, i64 0}
!31 = !{!32, !32, i64 0}
!32 = !{!"float32", !33, i64 0}
!33 = !{!"0x2418a30", !8, i64 0}
!34 = !{!35, !35, i64 0}
!35 = !{!"float32", !36, i64 0}
!36 = !{!"0x2418fd0", !8, i64 0}
!37 = !{!38, !38, i64 0}
!38 = !{!"float32", !39, i64 0}
!39 = !{!"0x2439280", !8, i64 0}
!40 = !{!41, !41, i64 0}
!41 = !{!"float32", !42, i64 0}
!42 = !{!"0x2439470", !8, i64 0}
!43 = !{!44, !44, i64 0}
!44 = !{!"float32", !45, i64 0}
!45 = !{!"0x24394b0", !8, i64 0}
!46 = !{!47, !47, i64 0}
!47 = !{!"float32", !48, i64 0}
!48 = !{!"0x2439700", !8, i64 0}
!49 = !{!50, !50, i64 0}
!50 = !{!"float32", !51, i64 0}
!51 = !{!"0x2467330", !8, i64 0}
!52 = !{!53, !53, i64 0}
!53 = !{!"float32", !54, i64 0}
!54 = !{!"0x2467400", !8, i64 0}
!55 = !{!56, !56, i64 0}
!56 = !{!"float32", !57, i64 0}
!57 = !{!"0x2470110", !8, i64 0}
!58 = !{!59, !59, i64 0}
!59 = !{!"float32", !60, i64 0}
!60 = !{!"0x2469f70", !8, i64 0}
!61 = !{!62, !62, i64 0}
!62 = !{!"0x246fb10.w8.b0", !63, i64 0}
!63 = !{!"0x246fb10.w16.b0", !64, i64 0}
!64 = !{!"0x246fb10.w32.b0", !65, i64 0}
!65 = !{!"0x246fb10.w64.b0", !66, i64 0}
!66 = !{!"0x246fb10.w128.b0", !67, i64 0}
!67 = !{!"0x246fb10.w256.b0", !68, i64 0}
!68 = !{!"0x246fb10.w512.b0", !69, i64 0}
!69 = !{!"0x246fb10.w1024.b0", !70, i64 0}
!70 = !{!"float32", !71, i64 0}
!71 = !{!"0x246fb10", !8, i64 0}
!72 = !{!70, !70, i64 0}
!73 = !{!74, !74, i64 0}
!74 = !{!"float32", !75, i64 0}
!75 = !{!"0x2467370", !8, i64 0}
!76 = !{!77, !77, i64 0}
!77 = !{!"float32", !78, i64 0}
!78 = !{!"0x305f340", !8, i64 0}
!79 = !{!80, !80, i64 0}
!80 = !{!"float32", !81, i64 0}
!81 = !{!"0x306dbc0", !8, i64 0}
!82 = !{!83, !83, i64 0}
!83 = !{!"float32", !84, i64 0}
!84 = !{!"0x305f300", !8, i64 0}
!85 = !{!86, !86, i64 0}
!86 = !{!"float32", !87, i64 0}
!87 = !{!"0x244dfd0", !8, i64 0}
!88 = !{!89, !89, i64 0}
!89 = !{!"float32", !90, i64 0}
!90 = !{!"0x244f1a0", !8, i64 0}
!91 = !{!92, !92, i64 0}
!92 = !{!"float32", !93, i64 0}
!93 = !{!"0x2455370", !8, i64 0}
!94 = !{!95, !95, i64 0}
!95 = !{!"float32", !96, i64 0}
!96 = !{!"0x24547c0", !8, i64 0}
!97 = !{!98, !98, i64 0}
!98 = !{!"float32", !99, i64 0}
!99 = !{!"0x2456480", !8, i64 0}
!100 = !{!101, !101, i64 0}
!101 = !{!"float32", !102, i64 0}
!102 = !{!"0x247d090", !8, i64 0}
!103 = !{!104, !104, i64 0}
!104 = !{!"float32", !105, i64 0}
!105 = !{!"0x247d050", !8, i64 0}
!106 = !{!107, !107, i64 0}
!107 = !{!"float32", !108, i64 0}
!108 = !{!"0x24823f0", !8, i64 0}
!109 = !{!110, !110, i64 0}
!110 = !{!"float32", !111, i64 0}
!111 = !{!"0x2483530", !8, i64 0}
!112 = !{!113, !113, i64 0}
!113 = !{!"float32", !114, i64 0}
!114 = !{!"0x2489280", !8, i64 0}
!115 = !{!116, !116, i64 0}
!116 = !{!"float32", !117, i64 0}
!117 = !{!"0x2488af0", !8, i64 0}
!118 = !{!119, !119, i64 0}
!119 = !{!"float32", !120, i64 0}
!120 = !{!"0x248a390", !8, i64 0}
!121 = !{!122, !122, i64 0}
!122 = !{!"float32", !123, i64 0}
!123 = !{!"0x2448630", !8, i64 0}
!124 = !{!125, !125, i64 0}
!125 = !{!"float32", !126, i64 0}
!126 = !{!"0x24485f0", !8, i64 0}
!127 = !{!128, !128, i64 0}
!128 = !{!"float32", !129, i64 0}
!129 = !{!"0x24a2a30", !8, i64 0}
!130 = !{!131, !131, i64 0}
!131 = !{!"float32", !132, i64 0}
!132 = !{!"0x24a27f0", !8, i64 0}
!133 = !{!134, !134, i64 0}
!134 = !{!"float32", !135, i64 0}
!135 = !{!"0x2499ed0", !8, i64 0}
!136 = !{!137, !137, i64 0}
!137 = !{!"float32", !138, i64 0}
!138 = !{!"0x249bc10", !8, i64 0}
!139 = !{!140, !140, i64 0}
!140 = !{!"0x24a2d40.w8.b0", !141, i64 0}
!141 = !{!"0x24a2d40.w16.b0", !142, i64 0}
!142 = !{!"0x24a2d40.w32.b0", !143, i64 0}
!143 = !{!"0x24a2d40.w64.b0", !144, i64 0}
!144 = !{!"0x24a2d40.w128.b0", !145, i64 0}
!145 = !{!"0x24a2d40.w256.b0", !146, i64 0}
!146 = !{!"0x24a2d40.w512.b0", !147, i64 0}
!147 = !{!"0x24a2d40.w1024.b0", !148, i64 0}
!148 = !{!"float32", !149, i64 0}
!149 = !{!"0x24a2d40", !8, i64 0}
!150 = !{!148, !148, i64 0}
!151 = !{!152, !152, i64 0}
!152 = !{!"float32", !153, i64 0}
!153 = !{!"0x249cdc0", !8, i64 0}
